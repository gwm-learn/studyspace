#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include "shm_com.h"

int main()
{
	int running=1;
	shm_data* shm_buff;
	int shmid;

	srand((unsigned int)getpid());

	shmid = shmget((key_t)1234, sizeof(shm_data), 0666|IPC_CREAT);

	shm_buff = (shm_data *)shmat(shmid, (void*)0, 0);

	shm_buff->is_written=0;

	while (running){
		if (shm_buff->is_written)
		{
			printf ("You wrote : %s", shm_buff->data);
			sleep(rand()%4);
			shm_buff->is_written=0;
			if (strncmp(shm_buff->data, "end", 3)==0)
				running=0;
		}
	}

	shmdt(shm_buff);

	shmctl(shmid, IPC_RMID, 0);//delet shm

	exit(EXIT_SUCCESS);

}



