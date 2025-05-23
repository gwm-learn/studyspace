#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>
#include<stdlib.h>

int main(int argc, char *argv[])
{
	char *host;
	int sockfd;
	int len, result;
	struct sockaddr_in address;
	struct hostent *hostinfo;
	struct servent *servinfo;
	char buffer[128];
	
	if (argc == 1)
		host = "localhost";
	else
		host = argv[1];
	
	//�z�X�g�A�h���X���擾���āA������Ȃ������ꍇ�ɂ́A�G���[���b�Z�[�W��\�����ďI������
	hostinfo = gethostbyname(host);
	if(!hostinfo)
	{
		fprintf(stderr, "no host: %s\n", host);
		exit(1);
	}
	
	//�ړI�̃z�X�g��daytime�T�[�r�X�����݂��邱�Ƃ��m���߂�
	servinfo = getservbyname("telnet", "tcp");  //�T�[�r�X�̖��O��[/etc/services]�ɋL�q�����
	if(!servinfo)
	{
		fprintf(stderr, "no telnet sevice\n");
		exit(1);
	}
	printf ("daytime port is %d\n", ntohs(servinfo -> s_port));
	
	//�\�P�b�g���쐬
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	
	//�ڑ����߂̃A�h���X��p�ӂ���
	address.sin_family = AF_INET;
	address.sin_port = servinfo -> s_port;
	address.sin_addr = *(struct in_addr *)(*(hostinfo -> h_addr_list));
	len = sizeof(address);
	
	printf ("address is %s\n", inet_ntoa(address.sin_addr));
	
	//�T�[�o�[��ڑ����A�ړI�̏����擾����
	result = connect(sockfd, (struct sockaddr *)&address, len);
	if(result == -1)
	{
		perror("oops: getdata");
		exit(1);
	}
	
	result = read(sockfd, buffer, sizeof(buffer));
	buffer[result] = '\0';
	printf("read %d bytes: %s\n", result, buffer);
	
	close(sockfd);
	exit(0);
}



