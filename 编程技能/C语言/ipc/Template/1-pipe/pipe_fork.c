#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


int main()
{
	int fd[2];
	int size=0;
	const char final_chars[]="123456";
	char buff[BUFSIZ+1];
	pid_t pid;
	
	memset(buff, '\0', sizeof(buff));
	
	if (pipe(fd)==0)
	{
		pid=fork();
		if (pid==0)
		{
			printf("I am son...\n");
			size=read(fd[0], buff, BUFSIZ);
			printf("Son read chars is : [%s]\n", buff);
		}
		else if (pid==-1)
		{
			printf ("fork failure\n");
			exit(EXIT_FAILURE);
		}
		else
		{
			printf("I am father...\n");
			size=write(fd[1], final_chars, strlen(final_chars));
			printf ("Father wrote %d chars\n", size);
			wait(0);
			exit(EXIT_SUCCESS);
		}
	}
	else
	{
		exit(EXIT_FAILURE);
	}
}
