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
#include "pspmpeg.h"
#include "mp4/mp4.h"



int SetupCallbacks();

PSP_MODULE_INFO("AVCDecoder", 0, 1, 1);
PSP_MAIN_THREAD_ATTR(0);
PSP_HEAP_SIZE_KB(-1024);

uint8_t buffer[512*272*4];

int main(void)
{	
	SetupCallbacks();
	pspDebugScreenInit();
	pspDebugScreenSetXY(0, 0);

	
	int frame = 0;
	struct mp4_read_struct reader;
	Mp4AvcDecoderStruct avc;
	
	mp4Init(&reader, &avc);
	int res = 0;
	for(frame = 0; frame < 20; frame++){
		res = mp4DecodeFrame(&reader, &avc, frame, buffer);
		if(res == -1)
			break;
	}
	
	for(int i = 0; i < 512*272; i++){
		buffer[i*4+3] = 0xff;
	}

	FILE* raw = fopen("yes.raw", "wb");
	fwrite(buffer, sizeof(buffer), 1, raw);
	fclose(raw);

	if(res == -1)
		pspDebugScreenPrintf("dead");
	else
		pspDebugScreenPrintf("alive");
	sceKernelDelayThread(100000);
	/*while(1){
		//bool res = player.decodeFrame();
		//if(res == false){
		//	pspDebugScreenSetXY(1, 0);
		//	pspDebugScreenPrintf("error on frame %d", frame);
		//}
		//std::string filename = "./frame_" + frame;
		//std::ofstream ofs(filename, std::ios::binary);
		//ofs.write((char*)player.RGBAbuffer, sizeof(player.RGBAbuffer));
		//ofs.close();
		pspDebugScreenPrintf("frame: %d", frame);
		pspDebugScreenSetXY(0, 0);
		frame++;
	}*/
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
