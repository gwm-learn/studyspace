#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>



#define PORT		1234
#define BUFF_SIZE	1024

main(void)
{
	struct sockaddr_in		sin;
	struct sockaddr_in		pin;
	int						sock;
	int						temp_sock;
	int						addr_size;
	char					buff[BUFF_SIZE];
	int						i, len;
	
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
	
	/* huzaigang 09/06/24: set for error exit */
	int opt = 1;
	if(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(int)) != 0){
		perror("setsocketopt");
		close(sock);
		return (-1);
	}
	
	/* Bind */
	if(bind(sock, (struct sockaddr*)&sin, sizeof(sin)) < 0){
		perror("call to bind!");
		exit(1);
	}
	
	/* Listen */
	if(listen(sock, 10) < 0){
		perror("call to listen!");
	}
	
	printf("[%d]Accepting connections ... \n",sock);
	for(;;){
		temp_sock = accept(sock, (struct sockaddr*)&pin, &addr_size);
		
		if(temp_sock < 0){
			perror("call to accept!");
			exit(1);
		}
		
		/* Receive data from client */
		memset(buff, '\0', BUFF_SIZE);
		if(recv(temp_sock, buff, BUFF_SIZE, 0) < 0){
			perror("call to recv!");
		}
		
		printf("receive from client: %s\n", buff);
		
		/* Data convert at server side */
		len = strlen(buff);
		for(i = 0; i < len; i ++){
			buff[i] = toupper(buff[i]);
		}
		
		/* Send data to client */
		if(send(temp_sock, buff, len, 0) < 0){
			perror("call to send!");
			exit(1);
		}
		
		/* Close */
		if(close(temp_sock) < 0){
			perror("call to close!");
			exit(1);
		}
		
	}

}


