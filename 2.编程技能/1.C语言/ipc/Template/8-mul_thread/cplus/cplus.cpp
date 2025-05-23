#include <stdio.h>
#include <stdlib.h>
#include<unistd.h>
#include <pthread.h>
#include <signal.h>


static long createThread(void (*func)());	//g++

static void thread1(void);
static void thread2(void);

int exitFlg=0;;
/////////////////////// sigrecv
static void sigrecv(int sig)
{
    if (sig == SIGINT) 
    {
        exitFlg = 1;
    }

}
/////////////////////// main
int main()
{
	signal(SIGINT, sigrecv);
	
	if(createThread(thread1) == -1)
	{
		printf("---------- Create Thread 1 ERROR\n");
		return -1;
	}
	
	if(createThread(thread2) == -1)
	{
		printf("---------- Create Thread 2 ERROR\n");
		return -1;
	}
	
	while (!exitFlg)
	{
		printf("I am alive...\n");
		sleep(5);
	}
	
	printf("Exit...\n");
	return 0;
} // main


static long createThread(void (*func)())	//g++
{
	pthread_t	threadId;
	if(pthread_create(&threadId,NULL,(void *(*)(void *))func,(void *)NULL) == 0)	//g++
	{
		pthread_detach(threadId);
		return 0;
	}
	return -1;
}

/////////////////////// thread1
static void thread1(void)
{
	while (!exitFlg)
	{
		printf("----- I am thread 1 -----\n");
		sleep(2);
	}
	return;
}

/////////////////////// thread2
static void thread2(void)
{
	while (!exitFlg)
	{
		printf("***** I am thread 2 *****\n");
		sleep(2);
	}
	return;
}
