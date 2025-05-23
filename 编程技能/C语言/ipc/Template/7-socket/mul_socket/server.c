#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
//#include <pthread.h>



#define PORT		1234
#define BUFF_SIZE	1024

/*------ createThread ---------*/
long createThread(void *func, void *arg)
{
	pthread_t threadId;

	if(pthread_create(&threadId,NULL,(void *)func,(void *)arg) == 0){
		pthread_detach(threadId);
		return 0;
	}
	return (-1);
}

/*------ printFunc ---------*/
long printFunc(void *arg)
{
	int paramSock = *(int *)arg;
	int back;
	char					buff[BUFF_SIZE];
	int						i, len;
    //printf("entry printFunc!\n");
	while(1){
			// Receive data from client
            memset(buff, 0, sizeof(buff));
			back = recv(paramSock, buff, BUFF_SIZE, 0);
			if(back < 0){
				perror("call to recv!");
			}
			printf("---sockect[%d]---receive from client: %s\n", paramSock, buff);

			len = strlen(buff);
			if(send(paramSock, buff, len, 0) < 0){
				perror("call to send!");
				exit(1);
			}

			if(strcmp(buff, "exit") == 0){
				printf("---sockect[%d]---END\n", paramSock);
				close(paramSock);
				return 0;
			}
		}
}

/*------ main ---------*/
main(void)
{
	struct sockaddr_in		sin;
	struct sockaddr_in		pin;
	int						sock;
	int						temp_sock;
	int						addr_size;

	/* Create socket */
	if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		perror("call to socket!");
		exit(1);
	}

	/* Init */
	bzero(&sin, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = INADDR_ANY;
	sin.sin_port = htons(PORT);

	/* Bind */
	if(bind(sock, (struct sockaddr*)&sin, sizeof(sin)) < 0){
		perror("call to bind!");
		exit(1);
	}

	/* Listen */
	if(listen(sock, 10) < 0){
		perror("call to listen!");
	}

	for(;;)
    {
	    printf("waiting connections ... \n");
		temp_sock = accept(sock, (struct sockaddr*)&pin, &addr_size);
		printf("addr_size=%d\n",addr_size);
		if(temp_sock < 0)
        {
			printf("temp_sock = %d\n", temp_sock);
			perror("call to accept!\n");
			exit(1);
		}
        else
        {
            printf("connected success!\n");
        }

		createThread(printFunc, (void *)&temp_sock);
		sleep(1);
	}
	// Close
	if(close(temp_sock) < 0){
		perror("call to close!");
		exit(1);
	}
}


