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
	
	//ホストアドレスを取得して、見つからなかった場合には、エラーメッセージを表示して終了する
	hostinfo = gethostbyname(host);
	if(!hostinfo)
	{
		fprintf(stderr, "no host: %s\n", host);
		exit(1);
	}
	
	//目的のホストにdaytimeサービスが存在することを確かめる
	servinfo = getservbyname("telnet", "tcp");  //サービスの名前は[/etc/services]に記述される
	if(!servinfo)
	{
		fprintf(stderr, "no telnet sevice\n");
		exit(1);
	}
	printf ("daytime port is %d\n", ntohs(servinfo -> s_port));
	
	//ソケットを作成
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	
	//接続ためのアドレスを用意する
	address.sin_family = AF_INET;
	address.sin_port = servinfo -> s_port;
	address.sin_addr = *(struct in_addr *)(*(hostinfo -> h_addr_list));
	len = sizeof(address);
	
	printf ("address is %s\n", inet_ntoa(address.sin_addr));
	
	//サーバーを接続し、目的の情報を取得する
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



