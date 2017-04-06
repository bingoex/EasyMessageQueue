#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

#include "channel.h"
#include "cm_shm.h"
#include "cm_debug.h" 
#include "cm_net.h"

int main(int argc, char * argv[])
{
	Channel_Queue * pstChannel = NULL;
	struct timeval tNow;
	char Body[100] = "abc";
	int iBodyLen = sizeof(Body);

	int iRet = GetChannelQueueShm(123456, &pstChannel, 0666 | IPC_CREAT);
	printf("GetChannelQueueShm %d pstChannel %p\n", iRet, pstChannel);

	if (iRet != 0) return 0;

	gettimeofday(&tNow, NULL);
	iRet = AddPackageToChannel(pstChannel, Body, 3,1, &tNow);
	printf("AddPackageToChannel %d\n", iRet);

	if (iRet != 0) return 0;

	iBodyLen = sizeof(Body);
	memset(Body, 0, iBodyLen);
	unsigned int iDiffUs = 0;
	iRet =  GetPackageFromChannel(pstChannel, Body, &iBodyLen, 1, &iDiffUs);
	printf("GetPackageFromChannel %d iDiffUs %u Body(%d) %s\n", iRet, iDiffUs, iBodyLen, DumpPackage(Body, iBodyLen));

	return 0;
}
