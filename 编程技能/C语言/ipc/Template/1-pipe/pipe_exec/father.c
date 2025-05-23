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
			sprintf (buff, "%d", fd[0]);
			/* execl(const char * path,const char * arg,....). arg[0] is usually exe's name */
			(void)execl("son", "son", buff, (char*)0);
		}
		else if (pid==-1)
		{
			printf ("fork failure\n");
			exit(EXIT_FAILURE);
		}
		else
		{
			printf ("I am father...\n");
        	size=write(fd[1], final_chars, sizeof(final_chars));
			printf ("father wrote %d chars\n", size);
		}
		exit(EXIT_SUCCESS);
	}
	else
	{
		exit(EXIT_FAILURE);
	}
}
