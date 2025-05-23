#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>


#define PORT		1234
#define BUFF_SIZE	1024

char*	host_name = "127.0.0.1";

main(int argc, char* argv[])
{
	char				buff[BUFF_SIZE];
	char				message[256];
	int					sock;
	struct sockaddr_in	pin;
	struct hostent*		server_host_name;
	
	char*				str = "A default test string";
	
	if(argc < 2){
		printf("We will send a default test string.\n");
	}else{
		str = argv[1];
	}
	
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
	
	printf("Sending message %s to server ... \n", str);
	
	/* Send */
	if(send(sock, str, strlen(str), 0) < 0){
		perror("ERROR in send\n");
	}
	
	printf("..sent message.. wait for response... \n");
	
	/* Receive */
	memset(buff, '\0', BUFF_SIZE);
	if(recv(sock, buff, BUFF_SIZE, 0) < 0){
		perror("ERROR in receiving response from server \n");
		exit(1);
	}
	
	printf("\nRespose from server: \n\n%s\n", buff);
	
	close(sock);
	
}

