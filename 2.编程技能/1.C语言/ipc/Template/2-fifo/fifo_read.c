#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

int main(void)
{
	int fd;
	char buff[4];

	fd = open("test_fifo", O_RDONLY);

	if(fd < 0)
	{
		printf("open error, errno:%d %s\n", errno, strerror(errno));
		exit(1);
	}

	while(1)
	{
		sleep(1);
		if(read(fd, buff, sizeof(buff)) < 0)
		{
			printf("read error\n");
		}else{
			printf("read str == [%s]\n", buff);
		}
	}

	close(fd);

	return 0;
}





