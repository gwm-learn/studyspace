#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <stdio.h>
#include <stdlib.h>

int main(void)
{
	int fd, ret;
	int i = 0;
	char buff[4];
    printf("sizeof:%lu\n",sizeof(buff));
	ret = mkfifo("test_fifo", 0666);
    if(0 != ret)
        printf("mkfifo error\n");

	fd = open("test_fifo", O_WRONLY|O_CREAT);

	printf("w open OK\n");

	if(fd < 0)
	{
		printf("open error\n");
		exit(1);
	}

	while(1)
	{
		sleep(1);
		sprintf(buff, "%d", ++i);
		if(write(fd, buff, sizeof(buff)) < 0)
		{
			printf("write error\n");
		}
		else
		{
			printf("wrote [%s] ok\n", buff);
		}
	}

	close(fd);

	return 0;
}





