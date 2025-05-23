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
	int						mtype;						/*	“Øæ∞ºﬁ¿≤Ãﬂ(ïKê{)		*/
	unsigned short			objtype;					/*	µÃﬁºﬁ™∏ƒ¿≤Ãﬂ			*/
	unsigned short			liuno;						/*	ånìùî‘çÜ				*/
	unsigned short			recno;						/*	êÆóùî‘çÜ				*/
	unsigned char			dummy[2];					/*	ó\îı					*/
	unsigned char			dat[16];			/*	√ﬁ∞¿					*/
} EVTDAT;

/////////////////////
int cp_msgQCreate(int msgQKey, int* msgQId)
{
	int msgId = 0;

	printf("----- test 2-1 -----\n");
	msgId = msgget(msgQKey, IPC_CREAT | IPC_EXCL | 0666);
	printf("----- test 2-2 -----\n");
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

/////////////////////
int		cp_msgQReceive(int msgQId, char* buffer, int msgsize, int msgflg)
{
	int rtn = 0;

	if(buffer == NULL){
		return(-1);
	}

	rtn = msgrcv(msgQId, buffer, msgsize, 0, msgflg);
	if(rtn <= 0){
		printf("[cp_msgQReceive] msgrcv Error(msgQId:%d rtn:%d errno:%d msgsize:%d)\n", msgQId, rtn, errno, msgsize);
		return(-1);
	}

	return(0);
}



///////////////////// main
int main()
{
	EVTDAT evtdat;
	memset(&evtdat, 0, sizeof(evtdat));
	evtdat.mtype	= 5;
	evtdat.objtype	= 6;
	evtdat.liuno	= 7;
	evtdat.recno	= 8;
	strcpy(evtdat.dummy,"H");
	strcpy(evtdat.dat,"Hello");

	EVTDAT evtdat_rec;
	
	int msgQId_create;
	int msgQId;
	int rtn;
	
	printf("----- test 1 -----\n");
	if(cp_msgQGet(BAC_MSG_KEY, &msgQId) == -1)
	{
		printf("Error END\n");
	}
	else
	{
		if(cp_msgQDelete(msgQId) == -1)
		{
			printf("Error END\n");
			return 0;
		}
	}
	printf("----- test 2 -----\n");
	
	if(cp_msgQCreate(BAC_MSG_KEY, &msgQId_create) == -1)
	{
		printf("Error END\n");
		return 0;
	}
	
	printf("----- test 3 -----\n");
	if(cp_msgQGet(BAC_MSG_KEY, &msgQId) == -1)
	{
		printf("Error END\n");
		return 0;
	}
	
	printf("----- test 4 -----\n");
	//rtn = cp_msgQSend(msgQId, &evtdat, sizeof(evtdat), IPC_NOWAIT);
	rtn = cp_msgQSend(msgQId, (char*)&evtdat, sizeof(EVTDAT), IPC_NOWAIT);
	if(rtn == -1)
	{
		printf("Error END\n");
		return 0;
	}
	
	printf("----- test 5 -----\n");
	rtn = cp_msgQReceive(msgQId, (char*)&evtdat_rec, sizeof(EVTDAT), 0);
	if(rtn == -1)
	{
		printf("Error END\n");
		return 0;
	}
	
	/*printf("----- test 6 -----\n");
	printf("receive:%s\n",evtdat_rec);*/
	
	printf("----- test 7 -----\n");
	printf("receive:%s\n",evtdat_rec.dat);
	
	printf("END!\n");
	return 0;
}




















/* hzg
msgsz = sizeof(struct mymsgbuf) - sizeof(long);



//test

struct msg {
	struct 		msg *msg_next; 			// next message on queue 
	long 		msg_type;
	char 		*msg_spot; 				// message text address 
	time_t 		msg_stime; 				// msgsnd time 
	short 		msg_ts; 				// message text size
};


int open_queue( key_t keyval )
{
	int qid;
	if((qid = msgget( keyval, IPC_CREAT | 0660 )) == -1)
	{
		return(-1);
	}
	return(qid);
}



int send_message( int qid, struct mymsgbuf *qbuf )
{
	int result, length;
	
	// The length is essentially the size of the structure minus sizeof(mtype)
	length = sizeof(struct mymsgbuf) - sizeof(long);
	if((result = msgsnd( qid, qbuf, length, 0)) == -1)
	{
		return(-1);
	}
	return(result);
}


main()
{
	int qid;
	key_t msgkey;
	struct mymsgbuf
	{
		long mtype; 				// Message type 
		int request; 				// Work request number
		double salary; 				// Employee's salary
	} msg;
	
	// Generate our IPC key value
	msgkey = ftok(".", 'm');
	
	// Open/create the queue
	if(( qid = open_queue( msgkey)) == -1) 
	{
		perror("open_queue");
		exit(1);
	}
	
	// Load up the message with arbitrary test data 
	msg.mtype = 1;					// Message type must be a positive number! 
	msg.request = 1; 				// Data element #1 
	msg.salary = 1000.00; 			// Data element #2 (my yearly salary!) 
	
	// Bombs away! 
	if((send_message( qid, &msg )) == -1) 
	{
		perror("send_message");
		exit(1);
	}
}











int read_message( int qid, long type, struct mymsgbuf *qbuf )
{
int result, length;
// The length is essentially the size of the structure minus sizeof(mtype) 
length = sizeof(struct mymsgbuf) - sizeof(long);
if((result = msgrcv( qid, qbuf, length, type, 0)) == -1)
{
return(-1);
}
return(result);
}

*/
