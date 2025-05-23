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

/////////////////////
int cp_msgQCreate(int msgQKey, int* msgQId)
{
	int msgId = 0;

	msgId = msgget(msgQKey, IPC_CREAT | IPC_EXCL | 0666);
	if(msgId == -1){
		printf("[cp_msgQCreate] msgget Error(msgQKey:%d)\n", msgQKey);
		return(-1);
	}

	*msgQId = msgId;

	return(0);
}

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
int	cp_msgQDelete(int msgQId)
{
	int rtn = 0;

	rtn = msgctl(msgQId, IPC_RMID, NULL);
	if(rtn != 0){
		return(-1);
	}

	return(0);
}

///////////////////// main
int main()
{
	int msgQId;
	
	if(cp_msgQGet(BAC_MSG_KEY, &msgQId) == 0)
	{
		cp_msgQDelete(msgQId);
	}
	
	if(cp_msgQCreate(BAC_MSG_KEY, &msgQId) == -1)
	{
		printf("Message queue create ERROR!\n");
		return 0;
	}
	
	printf("Message queue create OK!\n");
	return 0;
}

