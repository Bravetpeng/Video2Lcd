
#ifndef __VIDEO_MANAGER_H
#define __VIDEO_MANAGER_H

#include <config.h>

#define NB_BUFFER  4

struct VideoDevice;
struct VideoOpr;
typedef struct  VideoOpr T_VideoOpr, *PT_VideoOpr;
typedef struct  VideoDevice T_VideoDevice, *PT_VideoDevice;

typedef struct VideoBuf{
	int iWidth;  	 /* 宽度: 一行有多少个象素 */
	int iHeight; 	 /* 高度: 一列有多少个象素 */
	int iBpp;     	 /* 一个象素用多少位来表示 */
	int iLineBytes;  /* 一行数据有多少字节 */
	int iTotalBytes; /* 所有字节数 */ 
	unsigned char *aucPixelDatas;  /* 象素数据存储的地方 */

	int iPixFormat;  /* 像素格式 */
}T_VideoBuf, *PT_VideoBuf;

typedef struct VideoOpr{
	char *name;
	int (*InitDevice)(char * strDevName, PT_VideoDevice pt_VideoDevice);	
	int (*ExitDevice)(PT_VideoDevice pt_VideoDevice);
	int (*GetFrame)(PT_VideoDevice pt_VideoDevice, PT_VideoBuf pt_VideoBuf);
	int (*PutFrame)(PT_VideoDevice pt_VideoDevice, PT_VideoBuf pt_VideoBuf);
	int (*GetFormat)(PT_VideoDevice pt_VideoDevice);
	int (*StartDevice)(PT_VideoDevice pt_VideoDevice);
	int (*StopDevice)(PT_VideoDevice pt_VideoDevice);
	
	struct VideoOpr  *ptNext;
}T_VideoOpr, *PT_VideoOpr;

typedef struct VideoDevice{
	int iFd;
	int iPixFormat;
	int iwidth;
	int iHeigth;
	int iBuffCnt;

	unsigned char *pucV4L2Buf[NB_BUFFER];
	unsigned int  uiV4L2BufMaxLen;
	unsigned char ucIsStreaming;
	
	/* 操作函数 */
	PT_VideoOpr  pt_VideoOpr;

}T_VideoDevice, *PT_VideoDevice;

int VideoDevInit(char * DevName, PT_VideoDevice ptVideoDevice);
int VideoInit(void);

#endif /* __VIDEO_MANAGER_H */

