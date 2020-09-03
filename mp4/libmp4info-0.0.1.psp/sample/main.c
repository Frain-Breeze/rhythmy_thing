/* 
 *	Copyright (C) 2008 cooleyes
 *	eyes.cooleyes@gmail.com 
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *   
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *   
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. 
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */
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
#include "common/mem64.h"
#include "mp4info.h"
#include "mp4_read.h"

int SetupCallbacks();

PSP_MODULE_INFO("LibMp4Info Test", 0, 1, 1);
PSP_MAIN_THREAD_ATTR(0);
PSP_HEAP_SIZE_KB(18*1024);

SceCtrlData input;


char filename[1024];


int main(void)
{
	SetupCallbacks();
	
	pspDebugScreenInit();
	pspDebugScreenSetXY(0, 2);
	//scePowerSetClockFrequency(333,333,166);
	//scePowerSetCpuClockFrequency(333);
	//scePowerSetBusClockFrequency(166);
	scePowerSetClockFrequency(133,133,66);
	scePowerSetCpuClockFrequency(133);
	scePowerSetBusClockFrequency(66);
	u32 cpu = scePowerGetCpuClockFrequency();
	u32 bus = scePowerGetBusClockFrequency();
	
	pspDebugScreenPrintf("cpu=%d, bus=%d\n", cpu, bus);
	
	mp4info_t* info = mp4info_open("ms0:/VIDEO/Prison.Break.S03E01.MP4");
	
	pspDebugScreenPrintf("read info ok\n");
	
	mp4info_close(info);
	
	struct mp4_read_struct reader;
	char* result;
	result = mp4_read_open(&reader, "ms0:/VIDEO/Test3.MP4");
	if ( result ) {
		pspDebugScreenPrintf("mp4_read_open : %s\n", result);
		goto wait;
	}
	pspDebugScreenPrintf("v_rate %d, v_scale %d, a_rate %d, a_scale %d\n",
		reader.file.video_rate,
		reader.file.video_scale,
		reader.file.audio_rate,
		reader.file.audio_scale);
	
	pspDebugScreenPrintf("max_v_trunk %d, max_a_trunk %d, max_a_num %d, max_a_sample %d\n",
		reader.file.maximum_video_trunk_size,
		reader.file.maximum_audio_trunk_size,
		reader.file.maximun_audio_sample_number,
		reader.file.maximum_audio_sample_size);
		
	
	int video_frame = 0;
	struct mp4_read_output_struct output;
	
	while(!(input.Buttons & PSP_CTRL_TRIANGLE)) {
		if ( video_frame >= reader.file.number_of_video_frames)
			break;
		result = mp4_read_get(&reader, video_frame, 0, &output);
		if ( result ) {
			pspDebugScreenPrintf("mp4_read_get : %s\n", result);
			break;
		}
		video_frame++;
		sceCtrlReadBufferPositive(&input, 1);
	}
	

wait:
	pspDebugScreenPrintf("\n");
	pspDebugScreenPrintf("press triangle to exit...\n");
	sceCtrlReadBufferPositive(&input, 1);
	while(!(input.Buttons & PSP_CTRL_TRIANGLE))
	{
		sceKernelDelayThread(10000);	// wait 10 milliseconds
		sceCtrlReadBufferPositive(&input, 1);
	}
	
	sceKernelExitGame();
	return 0;
}


/* Exit callback */
int exit_callback(int arg1, int arg2, void *common)
{
	sceKernelExitGame();
	return 0;
}


/* Callback thread */
int CallbackThread(SceSize args, void *argp)
{
	int cbid;

	cbid = sceKernelCreateCallback("Exit Callback", exit_callback, NULL);
	sceKernelRegisterExitCallback(cbid);

	sceKernelSleepThreadCB();

	return 0;
}


/* Sets up the callback thread and returns its thread id */
int SetupCallbacks(void)
{
	int thid = 0;

	thid = sceKernelCreateThread("update_thread", CallbackThread, 0x11, 0xFA0, 0, 0);
	if(thid >= 0)
	{
		sceKernelStartThread(thid, 0, 0);
	}

	return thid;
}
