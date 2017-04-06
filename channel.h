#ifndef _CM_CHANNEL_H
#define _CM_CHANNEL_H



#include "cm_type.h"
#include <sys/time.h>

#define CHANNEL_BUFFER_LENGTH	(2*1000*1000) //2M
#define MAXTIME_IN_CHANNEL	(1000 * 1000) 

typedef struct
{
	char cFlag;
	struct timeval tPkgInTime; // 入队时间
	unsigned short shLength; // 包长度
} ChannelPkgHead;

typedef struct
{
	volatile unsigned CM_INT32 lTotalRequestPkgs;
	volatile unsigned CM_INT32 lTotalResponsePkgs;
	
    /* 请求队列 */
	volatile unsigned CM_INT32 lRequestHeadPos;
	volatile unsigned CM_INT32 lRequestTailPos;	
	char caRequestBuffer[CHANNEL_BUFFER_LENGTH];
	
    /* 回包队列 */
	volatile unsigned CM_INT32 lResponseHeadPos;
	volatile unsigned CM_INT32 lResponseTailPos;
	char caResponseBuffer[CHANNEL_BUFFER_LENGTH];
} Channel_Queue;


/*
 * 获取共享内存队列
 * 参数说明：
 *      iShmKey：共享内存key
 *      pstIdx：共享内存队列内存管理结构(出参)
 *      iFlag：共享内存flag
 */
int GetChannelQueueShm(int iShmKey, Channel_Queue **pstIdx, int iFlag);

/*
 * 入队
 */
int AddPackageToChannel(Channel_Queue *pstChannel, char *pBody, int iBodyLen,  int iIsRequest, const struct timeval *ptRecvTime);

/*
 * 出队
 */
int GetPackageFromChannel(Channel_Queue *pstChannel, char *pBody, int *piBodyLen, int iIsRequest, unsigned int *piDiffUs);



#endif
