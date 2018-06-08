
#ifndef __CONVERT_MANAGER_H
#define __CONVERT_MANAGER_H

#include <config.h>
#include <video_manager.h>
#include <linux/videodev2.h>
#include <stdlib.h>
#include <string.h>

typedef struct VideoConvert{
	char* name;

	int (*isSupport)(int PixelFormatIn, int PixelFormatOut);
	int (*Convert)(PT_VideoBuf pt_VideoBufIn, PT_VideoBuf pt_VideoBufOut);
	int (*ConvertExit)(PT_VideoBuf pt_VideoBuf);

	struct VideoConvert *ptNext;
}T_VideoConvert, *PT_VideoConvert;

int RegisterVideoConvert(PT_VideoConvert ptVideoConvert);
PT_VideoConvert GetVideoConvertForFormats(int PixelFormatIn, int PixelFormatOut);
int VideoConvertInit(void);
int Yuv2RgbInit(void);
int Rgb2RgbInit(void);
int Mjpeg2RgbInit(void);

#endif

