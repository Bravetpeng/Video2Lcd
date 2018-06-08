
#include <config.h>
#include <video_manager.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <string.h>
 
static int g_aiSupportedFormats[] = {V4L2_PIX_FMT_YUYV, V4L2_PIX_FMT_MJPEG, V4L2_PIX_FMT_RGB565};

static int isSupportFormat(int pixelformat)
{
	int i;
	for (i = 0; i < sizeof(g_aiSupportedFormats)/g_aiSupportedFormats[0]; i++){
		if (g_aiSupportedFormats[i] == pixelformat){
			return 1;
		}
	}
	return 0;
}

/* 参考 luvcview */

/* open
 * VIDIOC_QUERYCAP 确定它是否视频捕捉设备,支持哪种接口(streaming/read,write)
 * VIDIOC_ENUM_FMT 查询支持哪种格式
 * VIDIOC_S_FMT    设置摄像头使用哪种格式
 * VIDIOC_REQBUFS  申请buffer
 对于 streaming:
 * VIDIOC_QUERYBUF 确定每一个buffer的信息 并且 mmap
 * VIDIOC_QBUF     放入队列
 * VIDIOC_STREAMON 启动设备
 * poll            等待有数据
 * VIDIOC_DQBUF    从队列中取出
 * 处理....
 * VIDIOC_QBUF     放入队列
 * ....
 对于read,write:
    read
    处理....
    read
 * VIDIOC_STREAMOFF 停止设备
 *
 */

PT_VideoDevice pt_VideoDevice;
static int V4L2InitDevice(char * strDevName, PT_VideoDevice pt_VideoDevice);	
{
	int i;
	int iFd;
	int iError;
	struct v4l2_fmtdesc tFmtDesc;
	struct v4l2_capability tV4l2Cap;
	struct v4l2_format tFmt;
	struct v4l2_requestbuffers tRequestBuffers;
	struct v4l2_buffer tV4L2Buf;

	int iLcdWidth;
	int iLcdHeigth;
	int iLcdBpp;

	iFd = open(strDevName, O_RDWR);
	if (iFd < 0){
		DBG_PRINTF("can not open %s\n", strDevName);
		return -1;
	}

	pt_VideoDevice->iFd = iFd;

	memset(&tV4l2Cap, 0, sizeof(struct v4l2_capability));
    iError = ioctl(iFd, VIDIOC_QUERYCAP, &tV4l2Cap);
    if (iError < 0) {
		DBG_PRINTF("Error opening device %s: unable to query device.\n", strDevName);
		goto fatal;
    }

    if ((tV4l2Cap & V4L2_CAP_VIDEO_CAPTURE) == 0) {
		DBG_PRINTF("Error opening device %s: video capture not supported.\n",
		       strDevName);
		goto fatal;;
    }

    if (tV4l2Cap & V4L2_CAP_STREAMING) {
	    DBG_PRINTF("%s support streaming i/o\n", strDevName);
    }
    if (tV4l2Cap & V4L2_CAP_READWRITE) {
	    DBG_PRINTF("%s support read i/o\n", strDevName);
    }

	memset(&tFmtDesc, 0, sizeof(tFmtDesc));
	tFmtDesc.index = 0;
	tFmtDesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	while ((ret = ioctl(dev, VIDIOC_ENUM_FMT, &tFmtDesc)) == 0) {
		if(isSupportFormat(tFmtDesc.pixelformat)){
			pt_VideoDevice->pixelformat = tFmtDesc.pixelformat;
			break;
		}
		tFmtDesc.index++;
	}
	if (!pt_VideoDevice->pixelformat) {
		DBG_PRINTF("can not surpport this format\n");
		goto fatal;
	}

	/* 读出LCD分辨率 */
	GetDispResolution(&iLcdWidth, &iLcdHeigth, &iLcdBpp);

	/* set format in */
    memset(&tFmt, 0, sizeof(struct v4l2_format));
    tFmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    tFmt.fmt.pix.width 	     = iLcdWidth;
    tFmt.fmt.pix.height 	 = iLcdHeigth;
    tFmt.fmt.pix.pixelformat = pt_VideoDevice->pixelformat;
    tFmt.fmt.pix.field 		 = V4L2_FIELD_ANY;
    iError = ioctl(iFd, VIDIOC_S_FMT, &tFmt);
    if (iError < 0) {
		DBG_PRINTF("Unable to set format: %d.\n", iError);
		goto fatal;
    }

	pt_VideoDevice->iwidth  = tFmt.fmt.pix.width;
	pt_VideoDevice->iHeigth = tFmt.fmt.pix.height;

	/* request buffers */
    memset(&tRequestBuffers, 0, sizeof(struct v4l2_requestbuffers));
    tRequestBuffers.count  = NB_BUFFER;
    tRequestBuffers.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    tRequestBuffers.memory = V4L2_MEMORY_MMAP;

    iError = ioctl(iFd, VIDIOC_REQBUFS, &tRequestBuffers);
    if (iError < 0) {
		DBG_PRINTF("Unable to allocate buffers: %d.\n", iError);
		goto fatal;
    }

	pt_VideoDevice->iBuffCnt = tRequestBuffers.count;

	/* streaming 接口需要mmap */
	if (tV4l2Cap & V4L2_CAP_STREAMING){
		pt_VideoDevice->ucStreamOrRW = STREAMING;
		for (i = 0; i < NB_BUFFER; i++) {
			memset(&tV4L2Buf, 0, sizeof(struct v4l2_buffer));
			tV4L2Buf.index = i;
			tV4L2Buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			tV4L2Buf.memory = V4L2_MEMORY_MMAP;
			iError = ioctl(iFd, VIDIOC_QUERYBUF, &tV4L2Buf);
			if (iError < 0) {
			    DBG_PRINTF("Unable to query buffer (%d).\n", iError);
			    goto fatal;
			}
		    DBG_PRINTF("length: %u offset: %u\n", tV4L2Buf.length,
			   tV4L2Buf.m.offset);

			pt_VideoDevice->uiV4L2BufMaxLen = tV4L2Buf.length;
			pt_VideoDevice->pucV4L2Buf[i] = mmap(0 /* start anywhere */ ,
						  tV4L2Buf.length, PROT_READ, MAP_SHARED, iFd,
						  tV4L2Buf.m.offset);
			if (pt_VideoDevice->pucV4L2Buf[i] == MAP_FAILED) {
			    DBG_PRINTF("Unable to map buffer\n");
			    goto fatal;
			}
		    DBG_PRINTF("Buffer mapped at address %p.\n", pt_VideoDevice->pucV4L2Buf[i]);
	    }
	}
	else if (tV4l2Cap & V4L2_CAP_READWRITE){
		pt_VideoDevice->ucStreamOrRW 	= READWRITE;
		pt_VideoDevice->iBuffCnt  		= 1;
		/* 在本程序所支持的所有格式下，最大buf */
		pt_VideoDevice->uiV4L2BufMaxLen = pt_VideoDevice->iwidth * pt_VideoDevice->iwidth * 4;
		pt_VideoDevice->pucV4L2Buf[0]	= (char *)malloc(pt_VideoDevice->uiV4L2BufMaxLen);
	}

	/* Queue the buffers. */
    for (i = 0; i < NB_BUFFER; ++i) {
		memset(&tV4L2Buf, 0, sizeof(struct v4l2_buffer));
		tV4L2Buf.index   = i;
		tV4L2Buf.type 	 = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		tV4L2Buf.memory  = V4L2_MEMORY_MMAP;
		iError = ioctl(iFd, VIDIOC_QBUF, &tV4L2Buf);
		if (iError < 0) {
		    DBG_PRINTF("Unable to queue buffer \n");
		    goto fatal;;
		}
    }
	
	return 0;

fatal:
	close(iFd);
	return -1;
}

static int V4L2ExitDevice(PT_VideoDevice pt_VideoDevice);
{
	int i;
	for (i = 0; i < NB_BUFFER; i++){
		if(pt_VideoDevice->pucV4L2Buf[i]){
			munmap(pt_VideoDevice->pucV4L2Buf[i], pt_VideoDevice->uiV4L2BufMaxLen);
			pt_VideoDevice->pucV4L2Buf[i] = NULL;
		}
	}
	close(pt_VideoDevice->iFd);
	
	return 0;
}

static int V4L2GetFrame(PT_VideoDevice pt_VideoDevice, PT_VideoBuf pt_VideoBuf);
{
    struct pollfd tFds[1];
    int iRet;
    struct v4l2_buffer tV4l2Buf;

    tFds[0].fd 		= pt_VideoDevice->iFd;
    tFds[0].events 	= POLLIN;

    iRet = poll(tFds, 1, -1);  /* timeout : -1 一直等待数据 */
	if(iRet <= 0){
		DBG_PRINTF("poll ....\n");
		return -1;
	}
			
	if (pt_VideoDevice->ucStreamOrRW == STREAMING){
		memset(&tV4l2Buf, 0, sizeof(struct v4l2_buffer));
		tV4l2Buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		tV4l2Buf.memory = V4L2_MEMORY_MMAP;
		iRet = ioctl(pt_VideoDevice->iFd, VIDIOC_DQBUF, &tV4l2Buf);
		if (iRet < 0) {
			DBG_PRINTF("Unable to dequeue buffer\n");
			return -1;
		}
		pt_VideoDevice->iVideoBufCurIndex = tV4l2Buf.index;

	    pt_VideoBuf->iPixelFormat        = pt_VideoDevice->iPixFormat;
	    pt_VideoBuf->tPixelDatas.iWidth  = pt_VideoDevice->iwidth;
	    pt_VideoBuf->tPixelDatas.iHeight = pt_VideoDevice->iHeigth;
	    pt_VideoBuf->tPixelDatas.iBpp    = (pt_VideoDevice->iPixFormat == V4L2_PIX_FMT_YUYV)  ? 16 : \
	                                       (pt_VideoDevice->iPixFormat == V4L2_PIX_FMT_MJPEG) ? 0 : \ 
	                                       (pt_VideoDevice->iPixFormat == V4L2_PIX_FMT_RGB565) : 16;
	    pt_VideoBuf->tPixelDatas.iLineBytes    = pt_VideoDevice->iwidth * pt_VideoBuf->tPixelDatas.iBpp / 8;
	    pt_VideoBuf->tPixelDatas.iTotalBytes   = tV4l2Buf->bytesused;
	    pt_VideoBuf->tPixelDatas.aucPixelDatas = pt_VideoDevice->pucV4L2Buf[tV4l2Buf.index];  
	}
	else if(pt_VideoDevice->ucStreamOrRW == READWRITE){
		 iRet = read(pt_VideoDevice->iFd, pt_VideoDevice->pucV4L2Buf[0], pt_VideoDevice->uiV4L2BufMaxLen);
	    if (iRet <= 0){
	        return -1;
	    }
	    
	    pt_VideoBuf->iPixelFormat        = pt_VideoDevice->iPixFormat;
	    pt_VideoBuf->tPixelDatas.iWidth  = pt_VideoDevice->iwidth;
	    pt_VideoBuf->tPixelDatas.iHeight = pt_VideoDevice->iHeigth;
	    pt_VideoBuf->tPixelDatas.iBpp    = (pt_VideoDevice->iPixFormat == V4L2_PIX_FMT_YUYV) ? 16 : \
	                                        (pt_VideoDevice->iPixFormat == V4L2_PIX_FMT_MJPEG) ? 0 : \ 
	                                        (pt_VideoDevice->iPixFormat == V4L2_PIX_FMT_RGB565) : 16;
	    pt_VideoBuf->tPixelDatas.iLineBytes    = pt_VideoDevice->iwidth * pt_VideoBuf->tPixelDatas.iBpp / 8;
	    pt_VideoBuf->tPixelDatas.iTotalBytes   = iRet;
	    pt_VideoBuf->tPixelDatas.aucPixelDatas = pt_VideoDevice->pucV4L2Buf[0];    
	}

	return 0;
}

static int V4L2PutFrame(PT_VideoDevice pt_VideoDevice, PT_VideoBuf pt_VideoBuf);
{
	int iRet;
	struct v4l2_buffer tV4l2Buf;

	if (pt_VideoDevice->ucStreamOrRW == STREAMING){
		memset(&tV4l2Buf, 0, sizeof(struct v4l2_buffer));
		tV4l2Buf.index  = pt_VideoDevice->iVideoBufCurIndex;
		tV4l2Buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		tV4l2Buf.memory = V4L2_MEMORY_MMAP;
		iError = ioctl(pt_VideoDevice->iFd, VIDIOC_QBUF, &tV4l2Buf);
		if (iError) 
	    {
		    DBG_PRINTF("Unable to queue buffer.\n");
		    return -1;
		}
	}
	else if(pt_VideoDevice->ucStreamOrRW == READWRITE){
		return 0;
	}

	return 0;
}

static int V4L2StartDevice(PT_VideoDevice pt_VideoDevice);
{
	int iType = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    int iError;

    iError = ioctl(pt_VideoDevice->iFd, VIDIOC_STREAMON, &type);
    if (iError < 0) {
		DBG_PRINTF("Unable to %s capture: %d.\n", "start", ret);
		return -1;
    }
	pt_VideoDevice->ucIsStreaming = 1;
	
	return 0;
}

static int V4L2StopDevice(PT_VideoDevice pt_VideoDevice);
{
	int iType = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    int iError;

    iError = ioctl(pt_VideoDevice->iFd, VIDIOC_STREAMOFF, &iType);
    if (iError < 0) {
		DBG_PRINTF("Unable to %s capture: %d.\n", "stop", iError);
		return -1;
    }
	pt_VideoDevice->ucIsStreaming = 0;

	return 0;
}

/* 构造VideoOpr结构体 */
static struct VideoOpr g_tV4L2VideoOpr{
	.name 			= "V4L2Video";
	.InitDevice		= V4L2InitDevice;
	.ExitDevice		= V4L2ExitDevice;
	.GetFrame		= V4L2GetFrame;
	.PutFrame		= V4L2PutFrame;
	.StartDevice	= V4L2StartDevice;
	.StopDevice		= V4L2StopDevice;
};

/* 注册 */
int V4l2Init()
{
	return RegisterVideoOpr(&g_tV4L2VideoOpr);
}


