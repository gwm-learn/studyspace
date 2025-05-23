#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[])
{
	int size=0;
	char buff[BUFSIZ+1];
	int fd;

	memset(buff, '\0', sizeof(buff));

	sscanf(argv[1], "%d", &fd);
    printf("fd=%d\n",fd);

	size=read(fd, buff, 1);
	while (size > 0){
		printf("\nson's size=[%d]\n", size);
		printf("\nson read chars is : [%s]\n", buff);
		size=read(fd, buff, 1);
	}
	exit(EXIT_SUCCESS);
}
