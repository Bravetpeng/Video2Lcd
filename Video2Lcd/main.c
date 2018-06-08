#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <config.h>
#include <disp_manager.h>
#include <video_manager.h>
#include <convert_manager.h>
#include <render.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>

int PicZoom1(PT_VideoBuf ptOriginPic, PT_VideoBuf ptZoomPic)
{
    unsigned long dwDstWidth = ptZoomPic->iWidth;
    unsigned long* pdwSrcXTable;
	unsigned long x;
	unsigned long y;
	unsigned long dwSrcY;
	unsigned char *pucDest;
	unsigned char *pucSrc;
	unsigned long dwPixelBytes = ptOriginPic->iBpp/8;

	if (ptOriginPic->iBpp != ptZoomPic->iBpp)
	{
		return -1;
	}

    pdwSrcXTable = malloc(sizeof(unsigned long) * dwDstWidth);
    if (NULL == pdwSrcXTable)
    {
        DBG_PRINTF("malloc error!\n");
        return -1;
    }

    for (x = 0; x < dwDstWidth; x++)//���ɱ� pdwSrcXTable
    {
        pdwSrcXTable[x]=(x*ptOriginPic->iWidth/ptZoomPic->iWidth);
    }

    for (y = 0; y < ptZoomPic->iHeight; y++)
    {			
        dwSrcY = (y * ptOriginPic->iHeight / ptZoomPic->iHeight);

		pucDest = ptZoomPic->aucPixelDatas + y*ptZoomPic->iLineBytes;
		pucSrc  = ptOriginPic->aucPixelDatas + dwSrcY*ptOriginPic->iLineBytes;
		
        for (x = 0; x <dwDstWidth; x++)
        {
            /* ԭͼ����: pdwSrcXTable[x]��srcy
             * ��������: x, y
			 */
			 memcpy(pucDest+x*dwPixelBytes, pucSrc+pdwSrcXTable[x]*dwPixelBytes, dwPixelBytes);
        }
    }

    free(pdwSrcXTable);
	return 0;
}

int PicMerge1(int iX, int iY, PT_VideoBuf ptSmallPic, PT_VideoBuf ptBigPic)
{
	int i;
	unsigned char *pucSrc;
	unsigned char *pucDst;
	
	if ((ptSmallPic->iWidth > ptBigPic->iWidth)  ||
		(ptSmallPic->iHeight > ptBigPic->iHeight) ||
		(ptSmallPic->iBpp != ptBigPic->iBpp))
	{
		return -1;
	}

	pucSrc = ptSmallPic->aucPixelDatas;
	pucDst = ptBigPic->aucPixelDatas + iY * ptBigPic->iLineBytes + iX * ptBigPic->iBpp / 8;
	for (i = 0; i < ptSmallPic->iHeight; i++)
	{
		memcpy(pucDst, pucSrc, ptSmallPic->iLineBytes);
		pucSrc += ptSmallPic->iLineBytes;
		pucDst += ptBigPic->iLineBytes;
	}
	return 0;
}


/* video </dev/video0...> */
int main(int argc, char **argv)
{	
	int iError;
	float k;
	T_VideoDevice t_VideoDev;
	T_VideoBuf t_VideoBuf;
	T_VideoBuf t_VideoBufDisp;
	T_VideoBuf t_ZoomBuf;
	T_VideoBuf t_FrameBuf;
	PT_VideoBuf pt_VideoBufCur;
	PT_VideoConvert ptVideoConvert;

	int iPixelFormatOfVideo;
	int iPixelFormatOfDisp;

	int iLcdWidth;
	int iLcdHeigth;
	int iLcdBpp;

	int iTopLeftX;
	int iTopLeftY;

	if (argc != 2)
	{
		DBG_PRINTF("Usage:\n");
		DBG_PRINTF("%s </dev/video0...>\n", argv[0]);
		return -1;
	}

	/* 注册显示设备 */
	DisplayInit();	
	/* 可能可支持多个显示设备: 选择和初始化指定的显示设备 */
	SelectAndInitDefaultDispDev("fb");	
	GetDispResolution(&iLcdWidth, &iLcdHeigth, &iLcdBpp);	
	GetVideoBufForDisplay(&t_FrameBuf);
    iPixelFormatOfDisp = t_FrameBuf.iPixFormat;
	
	VideoInit();
	iError = VideoDevInit(argv[1], &t_VideoDev);
	if (iError){
		DBG_PRINTF(" VideoDevInit error!\n ");
		return -1;
	}
	
	iPixelFormatOfVideo = t_VideoDev.pt_VideoOpr->GetFormat(&t_VideoDev);
	VideoConvertInit();
	ptVideoConvert = GetVideoConvertForFormats(iPixelFormatOfVideo, iPixelFormatOfDisp);
	if (NULL == ptVideoConvert){
		DBG_PRINTF("can not support this format\n");
		return -1;
	}
	
	/* 启动摄像头设备 */
	iError = t_VideoDev.pt_VideoOpr->StartDevice(&t_VideoDev);
	if (iError){
		DBG_PRINTF(" StartDevice error!\n ");
		return -1;
	}
	memset(&t_VideoBuf, 0, sizeof (t_VideoBuf));
	memset(&t_VideoBufDisp, 0, sizeof (t_VideoBuf));
	t_VideoBufDisp.iPixFormat = iPixelFormatOfVideo;
	t_VideoBufDisp.iBpp = iLcdBpp;
	
	memset(&t_ZoomBuf, 0, sizeof (t_VideoBuf));
	while (1){
		/* 读入摄像头数据 */
		iError = t_VideoDev.pt_VideoOpr->GetFrame(&t_VideoDev, &t_VideoBuf);
		if (iError){
			DBG_PRINTF(" GetFrame error!\n ");
			return -1;
		}
		pt_VideoBufCur = &t_VideoBuf;
				
		/* 转换为RGB数据 */
		if(iPixelFormatOfVideo != iPixelFormatOfDisp){
			iError = ptVideoConvert->Convert(&t_VideoBuf, &t_VideoBufDisp);
			if (iError){
			DBG_PRINTF(" Convert error!\n ");
			return -1;
			}
			pt_VideoBufCur = &t_VideoBufDisp;
		}
		
		/* 若图片分辨率大于LCD,则进行缩放 */
		if(pt_VideoBufCur->iWidth > iLcdWidth || pt_VideoBufCur->iHeight > iLcdHeigth){
			/* 确定缩放后的分辨率 */
            /* 把图片按比例缩放到VideoMem上, 居中显示
             * 1. 先算出缩放后的大小
             */
			k = (float)pt_VideoBufCur->iHeight / pt_VideoBufCur->iWidth;
            t_ZoomBuf.iWidth  = iLcdWidth;
            t_ZoomBuf.iHeight = iLcdWidth * k;
            if ( t_ZoomBuf.iHeight > iLcdHeigth)
            {
                t_ZoomBuf.iWidth  = iLcdHeigth / k;
                t_ZoomBuf.iHeight = iLcdHeigth;
            }
            t_ZoomBuf.iBpp        = iLcdBpp;
            t_ZoomBuf.iLineBytes  = t_ZoomBuf.iWidth * t_ZoomBuf.iBpp / 8;
            t_ZoomBuf.iTotalBytes = t_ZoomBuf.iLineBytes * t_ZoomBuf.iHeight;

            if (!t_ZoomBuf.aucPixelDatas)
            {
                t_ZoomBuf.aucPixelDatas = malloc(t_ZoomBuf.iTotalBytes);
            }
            
			PicZoom1(pt_VideoBufCur, &t_ZoomBuf);
			pt_VideoBufCur = &t_ZoomBuf;
		}
		
		/* 合并进framebuffer */
        /* 接着算出居中显示时左上角坐标 */
		iTopLeftX = (iLcdWidth - pt_VideoBufCur->iWidth) / 2;
        iTopLeftY = (iLcdHeigth - pt_VideoBufCur->iHeight) / 2;

        PicMerge1(iTopLeftX, iTopLeftY, pt_VideoBufCur, &t_FrameBuf);
		
        FlushPixelDatasToDev(&t_FrameBuf);
		
        iError = t_VideoDev.pt_VideoOpr->PutFrame(&t_VideoDev, &t_VideoBuf);
        if (iError)
        {
            DBG_PRINTF("PutFrame for %s error!\n", argv[1]);
            return -1;
        }                    
		
		/* 把数据存入framebuffer, 显示 */
		
	}

	
	return 0;
}

