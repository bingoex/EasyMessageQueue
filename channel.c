#include "cm_shm.h"
#include "cm_net.h"
#include "channel.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

#define STX 0xa
#define ETX 0x8
#define USDIFF(new, old) (1000000 * (uint64_t)((new).tv_sec - (old).tv_sec) + (new).tv_usec - (old).tv_usec)

// get channel share memory
int GetChannelQueueShm(int iShmKey, Channel_Queue **pstIdx, int iFlag)
{
	char* sShm;

	if (!(sShm = (char *)GetShm(iShmKey, sizeof(Channel_Queue), iFlag & (~IPC_CREAT)))) {
		if (!(iFlag & IPC_CREAT)) return -1;
		if (!(sShm = (char *)GetShm(iShmKey, sizeof(Channel_Queue), iFlag))) 
			return -1;
		
		*pstIdx = (Channel_Queue *)sShm;
		memset(sShm, 0, sizeof(Channel_Queue));
		
		return 0;
	}
	
	*pstIdx = (Channel_Queue *)sShm;
	
	return 0;
}

// add package to channel with time
int AddPackageToChannel(Channel_Queue *pstChannel, char *pBody, int iBodyLen,  int iIsRequest, const struct timeval *ptRecvTime)
{
	ChannelPkgHead stChannelPkgHead;
	char *pBuffer, *pCur;
	volatile unsigned CM_INT32 *plHeadPos, *plTailPos, lHeadPos, lTailPos;//TODO
	int iPkgLen;
	
	if (iIsRequest) {
		pBuffer = pstChannel->caRequestBuffer;
		plHeadPos = &(pstChannel->lRequestHeadPos);
		plTailPos = &(pstChannel->lRequestTailPos);
	}
	else {
		pBuffer = pstChannel->caResponseBuffer;
		plHeadPos = &(pstChannel->lResponseHeadPos);
		plTailPos = &(pstChannel->lResponseTailPos);
	}
	
	lHeadPos = *plHeadPos;
	lTailPos = *plTailPos;
	
	//check pointer
	if (lHeadPos >= CHANNEL_BUFFER_LENGTH || lTailPos >= CHANNEL_BUFFER_LENGTH) {
		*plHeadPos = *plTailPos = 0;
		return -1;
	}
	
	//package length
	iPkgLen = sizeof(stChannelPkgHead) + iBodyLen;
	
	if (lTailPos >= lHeadPos) {		
		if (CHANNEL_BUFFER_LENGTH - lTailPos == iPkgLen && lHeadPos == 0) return -2;//not enough
		if (CHANNEL_BUFFER_LENGTH - lTailPos < iPkgLen)
		{
			if (lHeadPos <= iPkgLen) return -3; //not enough 
			
			*(pBuffer + lTailPos) = ETX;
			lTailPos = 0;
		}
	}
	else {
		if (lHeadPos - lTailPos <= iPkgLen) return -4;//not enough
	}
	
	//address to add package
	pCur = pBuffer + lTailPos;
	
	//add package
	stChannelPkgHead.cFlag = STX;
	stChannelPkgHead.shLength = iBodyLen;
	if(ptRecvTime) {
		stChannelPkgHead.tPkgInTime = *ptRecvTime;
	}else {
		memset(&stChannelPkgHead.tPkgInTime, 0, sizeof(stChannelPkgHead.tPkgInTime));
	}

	
	memcpy(pCur, &stChannelPkgHead, sizeof(stChannelPkgHead));
	memcpy(pCur+sizeof(stChannelPkgHead), pBody, iBodyLen);
	
	//change tail pointer 
	//Note that the last to modify it
	*plTailPos = (lTailPos + iPkgLen) % CHANNEL_BUFFER_LENGTH;
	
	return 0;
}

// get package from channel
int GetPackageFromChannel(Channel_Queue *pstChannel, char *pBody, int *piBodyLen, int iIsRequest, unsigned int *piDiffUs)
{
	ChannelPkgHead *pstChannelPkgHead;
	char *pBuffer, *pCur;
	volatile unsigned CM_INT32 *plHeadPos, *plTailPos, lHeadPos, lTailPos;
	int iPkgLen, iBufLen;
	struct timeval tNow;
	unsigned int iDiffTime;
	
	if (iIsRequest) {
		pBuffer = pstChannel->caRequestBuffer;
		plHeadPos = &(pstChannel->lRequestHeadPos);
		plTailPos = &(pstChannel->lRequestTailPos);
	}
	else {
		pBuffer = pstChannel->caResponseBuffer;
		plHeadPos = &(pstChannel->lResponseHeadPos);
		plTailPos = &(pstChannel->lResponseTailPos);
	}
	
	lHeadPos = *plHeadPos;
	lTailPos = *plTailPos;
	
	//check pointer
	if (lHeadPos >= CHANNEL_BUFFER_LENGTH || lTailPos >= CHANNEL_BUFFER_LENGTH) {
		*plHeadPos = *plTailPos = 0;
		return -1;
	}
	
	if (lHeadPos == lTailPos) return -2;
	
	if (lHeadPos > lTailPos && *(pBuffer + lHeadPos) == ETX) {
		*plHeadPos = lHeadPos = 0;
		if (lHeadPos == lTailPos) return -3;
	}
	
	//get buffer length
	if (lTailPos > lHeadPos) iBufLen = lTailPos - lHeadPos;
	else iBufLen = CHANNEL_BUFFER_LENGTH - lHeadPos;
	
	
	if (iBufLen < sizeof(ChannelPkgHead)) {
		*plHeadPos = lTailPos;
		return -4;
	}
	
	pstChannelPkgHead = (ChannelPkgHead *)(pBuffer + lHeadPos);
	if (pstChannelPkgHead->cFlag != STX) {
		*plHeadPos = lTailPos;
		return -5;
	}
	
	iPkgLen = sizeof(ChannelPkgHead) + pstChannelPkgHead->shLength;
	
	if (iBufLen < iPkgLen || *piBodyLen < pstChannelPkgHead->shLength) {
		*plHeadPos = lTailPos;
		return -6;
	}
	
	//get package
	pCur = pBuffer + lHeadPos + sizeof(ChannelPkgHead);
	*piBodyLen = pstChannelPkgHead->shLength;
	memcpy(pBody, pCur, *piBodyLen);

	if (pstChannelPkgHead->tPkgInTime.tv_sec != 0) {
		gettimeofday(&tNow, NULL);
		iDiffTime = (unsigned int)USDIFF(tNow, pstChannelPkgHead->tPkgInTime);
		if (iDiffTime > MAXTIME_IN_CHANNEL) { 
			//TODO
		}

		if (piDiffUs){ *piDiffUs = iDiffTime;}
	}
	
	//change head pointer
	*plHeadPos = (lHeadPos + iPkgLen) % CHANNEL_BUFFER_LENGTH;
	
	return 0;
}

