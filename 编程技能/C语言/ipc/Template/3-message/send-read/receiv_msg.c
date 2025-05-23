#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/sem.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <sys/msg.h>

#define BAC_MSG_KEY			20202

typedef	struct	Evtdat	{
	int						mtype;
	unsigned short			objtype;
	unsigned short			liuno;
	unsigned short			recno;
	unsigned char			dummy[3];
	unsigned char			dat[16];
} EVTDAT_receive;

/////////////////////
int		cp_msgQGet(int msgQKey, int* msgQId)
{
	int msgId = 0;

	msgId = msgget(msgQKey, IPC_EXCL | 0666);
	if(msgId == -1){
		return(-1);
	}

	*msgQId = msgId;
	return(0);
}

/////////////////////
int		cp_msgQReceive(int msgQId, char* buffer, int msgsize, int msgflg)
{
	int rtn = 0;

	if(buffer == NULL){
		return(-1);
	}

	rtn = msgrcv(msgQId, buffer, msgsize, 0, msgflg);
	if(rtn <= 0)
	{
		if(errno == ENOMSG)
		{
			printf("NO Message, errno:%d\n",errno);
		}
		
		printf("[cp_msgQReceive] msgrcv Error(msgQId:%d rtn:%d errno:%d msgsize:%d)\n", msgQId, rtn, errno, msgsize);
		return(-1);
	}

	return(0);
}

///////////////////// main
int main()
{
	EVTDAT_receive evtdat_receive;
	memset(&evtdat_receive, 0, sizeof(evtdat_receive));

	int msgQId_receive;
	int rtn;

	if(cp_msgQGet(BAC_MSG_KEY, &msgQId_receive) == -1)
	{
		printf("Message queue get Error\n");
		return 0;
	}

	//rtn = cp_msgQReceive(msgQId_receive, (char*)&evtdat_receive, sizeof(EVTDAT_receive), 0);
	rtn = cp_msgQReceive(msgQId_receive, (char*)&evtdat_receive, sizeof(EVTDAT_receive), IPC_NOWAIT);
	if(rtn == -1)
	{
		printf("Receive Meassage ERROR\n");
		return 0;
	}
	
	printf("Receive Meassage OK\n");
	printf("receive:%s\n",evtdat_receive.dat);
	printf("END!\n");
	return 0;
}
