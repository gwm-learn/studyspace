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
	unsigned char			dummy[2];
	unsigned char			dat[16];
} EVTDAT_send;

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
int		cp_msgQSend(int msgQId, char* buffer, int msgsize, int msgflg)
{
	int rtn = 0;

	rtn = msgsnd(msgQId, buffer, msgsize, msgflg);
	if(rtn != 0){
		if(errno == EAGAIN){
			rtn = -1;
		}
		else{
			rtn = -2;
		}
		printf("[cp_msgQSend] msgsnd Error(msgQId:%d rtn:%d errno:%d)\n", msgQId, rtn, errno);
		return(rtn);
	}

	return(0);
}

///////////////////// main
int main()
{
	EVTDAT_send evtdat_send;
	memset(&evtdat_send, 0, sizeof(evtdat_send));
	evtdat_send.mtype	= 5;
	evtdat_send.objtype	= 6;
	evtdat_send.liuno	= 7;
	evtdat_send.recno	= 8;
	strcpy(evtdat_send.dummy,"H");
	strcpy(evtdat_send.dat,"Hello");

	int msgQId_send;
	int rtn;
	

	if(cp_msgQGet(BAC_MSG_KEY, &msgQId_send) == -1)
	{
		printf("Message queue get Error\n");
		return 0;
	}

	rtn = cp_msgQSend(msgQId_send, (char*)&evtdat_send, sizeof(EVTDAT_send), IPC_NOWAIT);
	if(rtn == -1)
	{
		printf("Send Meassage ERROR\n");
		return 0;
	}
	
	printf("Send Meassage OK\n");
	return 0;
}
