#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>
#include <signal.h>
#include <unistd.h>

int main()
{
	int server_sockfd, client_sockfd;
	int server_len, client_len;
	struct sockaddr_in server_address;
	struct sockaddr_in client_address;
	
	//�T�[�o�[�p�ɖ��O�̂Ȃ��\�P�b�g���쐬����
	server_sockfd = socket(AF_INET, SOCK_STREAM, 0);
	
	//bind���Ăяo���ă\�P�b�g�ɖ��O��t����
	server_address.sin_family = AF_INET;
	//�ڑ���������N���C�A���g�̃A�h���X
	server_address.sin_addr.s_addr = htonl(INADDR_ANY);
	server_address.sin_port = htons(9734);
	server_len = sizeof(server_address);
	bind(server_sockfd, (struct sockaddr *)&server_address, server_len);
	
	//�ڑ��L���[���쐬���A�N���C�A���g����̐ڑ���҂�
	listen(server_sockfd, 5);
	
	//signal(SIGCHLD, SIG_IGN);
	
	while(1)
	{
		char ch;
		
		printf ("server waiting\n");
		
		client_len = sizeof(client_address);
		client_sockfd = accept(server_sockfd, (struct sockaddr *)&client_address, &client_len);
		
		if(fork() == 0)
		{
			while(read(client_sockfd, &ch, 1))
			{
				//printf ("read ok\n");
				++ch;
				write(client_sockfd, &ch, 1);
				//printf("write ok\n");
			}
			close(client_sockfd);
			exit(0);
		}
		else
			close(client_sockfd);
	}
	
	return 0;
}



