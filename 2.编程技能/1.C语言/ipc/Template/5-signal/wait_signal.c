#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

void fun_ctrl_c(int signal)
{
	printf("Hello Ctrl-c\n");
	exit(0);
}


void fun_SIGUSR1(int signal)
{
	printf("Hello SIGUSR1\n");
	exit(0);
}


int main(void)
{
	signal(SIGINT,fun_ctrl_c);
	signal(SIGUSR1,fun_SIGUSR1);

	while(1)
	{
	   printf("wait signal\n");
	   sleep(1);
	}
}

