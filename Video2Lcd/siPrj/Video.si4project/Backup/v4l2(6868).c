
#include <config.h>
#include <video_manager.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

static int V4L2InitDevice(char * strDevName, PT_VideoDevice pt_VideoDevice);	
{
	int iFd;
	int iError;

	struct v4l2_capability tV4l2Cap;

	iFd = open(strDevName, O_RDWR);
	if (iFd < 0){
		DBG_PRINTF("can not open %s\n", strDevName);
		return -1;
	}

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

    

	return 0;

fatal:
	close(iFd);
	return -1;
}

static int V4L2ExitDevice(PT_VideoDevice pt_VideoDevice);
{
	return 0;
}

static int V4L2GetFrame(PT_VideoDevice pt_VideoDevice, PT_VideoBuf pt_VideoBuf);
{
	return 0;
}

static int V4L2PutFrame(PT_VideoDevice pt_VideoDevice, PT_VideoBuf pt_VideoBuf);
{
	return 0;
}

static int V4L2StartDevice(PT_VideoDevice pt_VideoDevice);
{
	return 0;
}

static int V4L2StopDevice(PT_VideoDevice pt_VideoDevice);
{
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


