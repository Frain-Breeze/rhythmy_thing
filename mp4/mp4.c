#include <pspkernel.h>
#include <pspctrl.h>
#include <pspdisplay.h>
#include <psputils.h>
#include <pspgu.h>
#include <pspdebug.h>
#include <psppower.h>
#include <stdio.h>
#include <stdlib.h>
#include <psprtc.h>
#include <pspsdk.h>
#include <string.h>
#include <malloc.h>
#include "mp4.h"
#include "common/mem64.h"
#include "pspmpeg.h"
#include "mp4_read.h"



int mp4Init(struct mp4_read_struct* readerPointer, Mp4AvcDecoderStruct* avc){
    char filename[1024];

    getcwd(filename, 256);
	
	int devkitVersion = sceKernelDevkitVersion();
	if ( devkitVersion < 0x03050000)
		strcat(filename, "/mpeg_vsh330.prx");
	else if ( devkitVersion < 0x03070000)
		strcat(filename, "/mpeg_vsh350.prx");
	else
		strcat(filename, "/mpeg_vsh370.prx");
	
	//pspDebugScreenPrintf("%s\n", filename);
	
	mp4_read_safe_constructor(readerPointer);
	memset(avc, 0, sizeof(Mp4AvcDecoderStruct));
	
	int result;
	result = sceUtilityLoadAvModule(0);
	if ( result < 0 ) {
		//pspDebugScreenPrintf("\nerr: sceUtilityLoadAvModule(0)\n");
		goto wait;
	}
	
	SceUID modid;
	int status;
	modid = sceKernelLoadModule(filename, 0, NULL);
	if(modid >= 0) {
		modid = sceKernelStartModule(modid, 0, 0, &status, NULL);
	}
	else {
		//pspDebugScreenPrintf("\nerr=0x%08X : sceKernelLoadModule\n", modid);
		goto wait;
	}
	
	
	getcwd(filename, 256);
	strcat(filename, "/cooleyesBridge.prx");
	modid = sceKernelLoadModule(filename, 0, NULL);
	if(modid >= 0) {
		modid = sceKernelStartModule(modid, 0, 0, &status, NULL);
	}
	else {
		//pspDebugScreenPrintf("\nerr=0x%08X : sceKernelLoadModule(cooleyesBridge)\n", modid);
		goto wait;
	}
	
	char* res;
	//res = mp4_read_open(&reader, "ms0:/VIDEO/Test.MP4");
	res = mp4_read_open(readerPointer, "/loli.mp4");
	if ( res ) {
		//pspDebugScreenPrintf("mp4_read_open : %s\n", res);
		goto wait;
	}
	
	//pspDebugScreenPrintf("video width %d, height %d\n",
		//reader.file.video_width,
		//reader.file.video_height);
		
	if ( readerPointer->file.info->tracks[readerPointer->file.video_track_id]->avc_profile == 0x4D && 
		(readerPointer->file.video_width > 480 || readerPointer->file.video_height > 272 ) ) {
		//set ME to main profile 480P mode
		cooleyesMeBootStart(devkitVersion, 1);
		avc->mpeg_mode = 5;
	}
	else if (readerPointer->file.info->tracks[readerPointer->file.video_track_id]->avc_profile == 0x4D ){
		//set ME to main profile mode ( <=480*272 )
		cooleyesMeBootStart(devkitVersion, 3);
		avc->mpeg_mode = 4;
	}
	else if ( readerPointer->file.info->tracks[readerPointer->file.video_track_id]->avc_profile == 0x42 ) {
		//set ME to baseline profile mode ( <=480*272 )
		cooleyesMeBootStart(devkitVersion, 4);
		avc->mpeg_mode = 4;
	}
	
	result = sceMpegInit();
	if ( result != 0 ){
		//pspDebugScreenPrintf("\nerr: sceMpegInit=0x%08X\n", result);
		goto wait;
	}
	
	avc->mpeg_ddrtop =  memalign(0x400000, 0x400000);
	avc->mpeg_au_buffer = avc->mpeg_ddrtop + 0x10000;

	
	result = sceMpegQueryMemSize(avc->mpeg_mode);
	if ( result < 0 ){
		//pspDebugScreenPrintf("\nerr: sceMpegQueryMemSize(0x%08X)=0x%08X\n", avc->mpeg_mode, result);
		goto wait;
	}
	
	avc->mpeg_buffer_size = result;
	
	if ( (result & 0xF) != 0 )
		result = (result & 0xFFFFFFF0) + 16;
			
	avc->mpeg_buffer = malloc_64(result);
	if ( avc->mpeg_buffer == 0 ) {
		//pspDebugScreenPrintf("\nerr: alloc\n");
		goto wait;
	}
	
	result = sceMpegCreate(&avc->mpeg, avc->mpeg_buffer, avc->mpeg_buffer_size, &avc->mpeg_ringbuffer, 512, avc->mpeg_mode, avc->mpeg_ddrtop);	
	if ( result != 0){
		//pspDebugScreenPrintf("\nerr: sceMpegCreate(...)=0x%08X\n", result);
		goto wait;
	}
	
	avc->mpeg_au = (SceMpegAu*)malloc_64(64);
	if ( avc->mpeg_au == 0 ) {
		//pspDebugScreenPrintf("\nerr: alloc\n");
		goto wait;
	}
	memset(avc->mpeg_au, 0xFF, 64);
	if ( sceMpegInitAu(&avc->mpeg, avc->mpeg_au_buffer, avc->mpeg_au) != 0 ){
		//pspDebugScreenPrintf("\nerr: sceMpegInitAu(...)=0x%08X\n", result);
		goto wait;
	}
	
	unsigned char* nal_buffer = (unsigned char*)malloc_64(1024*1024);
	
	//pspDebugScreenPrintf("sps %d, pps %d, nal_prefix %d\n",
		//reader.file.info->tracks[reader.file.video_track_id]->avc_sps_size,
		//reader.file.info->tracks[reader.file.video_track_id]->avc_pps_size,
		//reader.file.info->tracks[reader.file.video_track_id]->avc_nal_prefix_size);
	
	avc->mpeg_sps_size = readerPointer->file.info->tracks[readerPointer->file.video_track_id]->avc_sps_size;
	avc->mpeg_pps_size = readerPointer->file.info->tracks[readerPointer->file.video_track_id]->avc_pps_size;
	avc->mpeg_nal_prefix_size = readerPointer->file.info->tracks[readerPointer->file.video_track_id]->avc_nal_prefix_size;
	avc->mpeg_sps_pps_buffer = malloc_64(avc->mpeg_sps_size + avc->mpeg_pps_size);
	if ( avc->mpeg_sps_pps_buffer == 0 ) {
		goto wait;
	}
	memcpy(avc->mpeg_sps_pps_buffer, readerPointer->file.info->tracks[readerPointer->file.video_track_id]->avc_sps, avc->mpeg_sps_size);
	memcpy(avc->mpeg_sps_pps_buffer+avc->mpeg_sps_size, readerPointer->file.info->tracks[readerPointer->file.video_track_id]->avc_pps, avc->mpeg_pps_size);

    return 0;
wait:
	mp4_read_close(readerPointer);
	return -1;
}

int mp4Delete(struct mp4_read_struct* readerPointer){
	mp4_read_close(readerPointer);
	return 0;
}

int mp4DecodeFrame(struct mp4_read_struct* readerPointer, Mp4AvcDecoderStruct* avc, int frame, uint8_t* buffer){
	Mp4AvcNalStruct nal;
	nal.sps_buffer = avc->mpeg_sps_pps_buffer;
    nal.sps_size = avc->mpeg_sps_size;
    nal.pps_buffer = avc->mpeg_sps_pps_buffer+avc->mpeg_sps_size;
    nal.pps_size = avc->mpeg_pps_size;
    nal.nal_prefix_size = avc->mpeg_nal_prefix_size;

	struct mp4_video_read_output_struct v_packet;

    int res = mp4_read_get_video(readerPointer, frame, &v_packet);
    if (res != 0)
        return -1;

    nal.nal_buffer = v_packet.video_buffer;
    nal.nal_size = v_packet.video_length ;
    if ( frame == 0 )
        nal.mode = 3;
    else
        nal.mode = 0;
    
    res = sceMpegGetAvcNalAu(&avc->mpeg, &nal, avc->mpeg_au);
    res = sceMpegAvcDecode(&avc->mpeg, avc->mpeg_au, 512, 0, &avc->mpeg_pic_num);
    res = sceMpegAvcDecodeDetail2(&avc->mpeg, &avc->mpeg_detail2);

    if(avc->mpeg_pic_num > 0){
        //should be for avc->mpeg_pic_num, but im lazy

        Mp4AvcCscStruct csc;
        csc.height = (avc->mpeg_detail2->info_buffer->height+15) >> 4;
        csc.width = (avc->mpeg_detail2->info_buffer->width+15) >> 4;
        csc.mode0 = 0;
        csc.mode1 = 0;
        csc.buffer0 = avc->mpeg_detail2->yuv_buffer->buffer0 ;
        csc.buffer1 = avc->mpeg_detail2->yuv_buffer->buffer1 ;
        csc.buffer2 = avc->mpeg_detail2->yuv_buffer->buffer2 ;
        csc.buffer3 = avc->mpeg_detail2->yuv_buffer->buffer3 ;
        csc.buffer4 = avc->mpeg_detail2->yuv_buffer->buffer4 ;
        csc.buffer5 = avc->mpeg_detail2->yuv_buffer->buffer5 ;
        csc.buffer6 = avc->mpeg_detail2->yuv_buffer->buffer6 ;
        csc.buffer7 = avc->mpeg_detail2->yuv_buffer->buffer7 ;
        int csc_width = (avc->mpeg_mode == 4) ? 512 : 768;

        if ( sceMpegBaseCscAvc(buffer, 0, csc_width, &csc) != 0 )
            return -1; 
    }
    return 0;
}
