
#include <config.h>
#include <video_manager.h>

static PT_VideoOpr g_ptVideoOprHead = NULL;

int RegisterVideoOpr(PT_VideoOpr ptVideoOpr)
{
	PT_VideoOpr ptTmp;
	if(!ptVideoOpr){
		g_ptVideoOprHead = ptVideoOpr;
		g_ptVideoOprHead->ptNext = NULL;
	}
	else{
		ptTmp = g_ptVideoOprHead;
		while(ptTmp->ptNext){
			ptTmp = ptTmp->ptNext;
		}
		ptTmp->ptNext 	= ptVideoOpr;
		ptVideoOpr->ptNext 	= NULL;
	}
	
	return 0;
}

void ShowVideoOpr(void)
{
	int i = 0;
	PT_VideoOpr ptTmp = g_ptVideoOprHead;

	while (ptTmp)
	{
		printf("%02d %s\n", i++, ptTmp->name);
		ptTmp = ptTmp->ptNext;
	}
}

PT_VideoOpr GetVideoOpr(char *pcName)
{
	PT_VideoOpr ptTmp = g_ptVideoOprHead;
	
	while (ptTmp)
	{
		if (strcmp(ptTmp->name, pcName) == 0)
		{
			return ptTmp;
		}
		ptTmp = ptTmp->ptNext;
	}
	
	return NULL;
}

int VideoInit(void)
{
	int iError;

    iError = V4l2Init();

	return iError;
}


