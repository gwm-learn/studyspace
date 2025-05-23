#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include "sem.h"

int main()
{
	sem_init();
	int i;
	for(i=0;i<10;++i)
	{
		sem_p();
		printf("B");
		fflush(stdout);

		int n = rand()%3;
		sleep(n);

		printf("B");
		fflush(stdout);

		sem_v();
		n = rand()%3;//产生随机睡眠时间
		sleep(n);

	}
	sleep(10);
	sem_destroy();

}

