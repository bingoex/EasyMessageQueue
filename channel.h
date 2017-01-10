#ifndef _CM_CHANNEL_H
#define _CM_CHANNEL_H
#include "cm_type.h"

#define CHANNEL_BUFFER_LENGTH		(2*1000*1000) //2M
#define MAXTIME_IN_CHANNEL	(1000 * 1000) 

typedef struct
{
	char cFlag;
	struct timeval tPkgInTime;
	unsigned short shLength;
} ChannelPkgHead;

typedef struct
{
	volatile unsigned CM_INT32 lTotalRequestPkgs;
	volatile unsigned CM_INT32 lTotalResponsePkgs;
	
	volatile unsigned CM_INT32 lRequestHeadPos;
	volatile unsigned CM_INT32 lRequestTailPos;	
	char caRequestBuffer[CHANNEL_BUFFER_LENGTH];
	
	volatile unsigned CM_INT32 lResponseHeadPos;
	volatile unsigned CM_INT32 lResponseTailPos;
	char caResponseBuffer[CHANNEL_BUFFER_LENGTH];
} Channel_Queue;


int GetChannelQueueShm(int iShmKey, Channel_Queue **pstIdx, int iFlag);
int AddPackageToChannel(Channel_Queue *pstChannel, char *pBody, int iBodyLen,  int iIsRequest, const struct timeval *ptRecvTime);
int GetPackageFromChannel(Channel_Queue *pstChannel, char *pBody, int *piBodyLen, int iIsRequest, unsigned int *piDiffUs);

#endif
