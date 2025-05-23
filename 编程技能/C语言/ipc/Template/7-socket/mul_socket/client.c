#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <pthread.h>


#define PORT		1234
#define BUFF_SIZE	1024

char*	host_name = "127.0.0.1";

/*------ createThread ---------*/
/*long createThread(void *func, void *arg)
{
	pthread_t threadId;

	if(pthread_create(&threadId,NULL,(void *)func,(void *)arg) == 0){
		pthread_detach(threadId);
		return 0;
	}
	return (-1);
}*/

/*------ main ---------*/
main(int argc, char* argv[])
{
	char				buff[BUFF_SIZE];
	char				sendStr[BUFF_SIZE];
	char				message[256];
	int					sock;
	struct sockaddr_in	pin;
	struct hostent*		server_host_name;

	if((server_host_name = gethostbyname(host_name)) == 0){
		perror("Error resolving local host\n");
		exit(1);
	}

	bzero(&pin, sizeof(pin));
	pin.sin_family = AF_INET;
	pin.sin_addr.s_addr = htonl(INADDR_ANY);
	pin.sin_addr.s_addr = ((struct in_addr*)(server_host_name->h_addr))->s_addr;
	pin.sin_port = htons(PORT);

	/* Creat socket */
	if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		perror("ERROR: opening socket\n");
		exit(1);
	}

	/* Connect */
	if(connect(sock, (void*)&pin, sizeof(pin)) < 0){
		perror("Error connecting to socket\n");
		exit(1);
	}
	printf("client connect OK\n");

	while(1){
        printf("please input massages\n");
		if(fgets(sendStr, sizeof(sendStr), stdin) == NULL){
			printf("fgets string error, next\n");
			continue;
		}
		if(send(sock, sendStr, strlen(sendStr)-1, 0) < 0){
			perror("ERROR in send\n");
		}

		memset(buff, '\0', sizeof(buff));

		if(recv(sock, buff, BUFF_SIZE, 0) < 0){
			perror("ERROR in receiving response from server \n");
			exit(1);
		}
		printf("\nRespose from server: \n\n%s\n", buff);

		if(strcmp(buff, "exit") == 0){
			printf("---sockect[%d]---END", sock);
			close(sock);
			return 0;
		}
	}
}

