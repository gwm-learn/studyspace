#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <string.h>

#include "shm_com.h"

int main()
{
	int running=1;
	shm_data* shm_buff;
	int shmid;
	char buff[BUFSIZ];

	shmid = shmget((key_t)1234, sizeof(shm_data), 0666|IPC_CREAT);

	shm_buff = (shm_data*)shmat(shmid, (void*)0, 0);

	while (running){
		while (shm_buff->is_written==1){
			sleep(1);
			printf ("waitting for client...\n");
		}
		printf ("Enter some text :   ");
		fgets(buff, BUFSIZ, stdin);
		strncpy(shm_buff->data, buff, TEXT_SZ);
		shm_buff->is_written=1;

		if (strncmp(buff, "end", 3)==0)
			running=0;
	}

	shmdt(shm_buff);

	exit(EXIT_SUCCESS);
}



