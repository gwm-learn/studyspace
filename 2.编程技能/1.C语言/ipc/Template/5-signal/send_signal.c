#include <sys/types.h>
#include <signal.h>
#include <stdio.h>

int main(int argc, char *argv[])
{
	int pid = 0;
	int res;
	
	if(argc < 2){
		printf("no input\n");
	}

	pid = atoi(argv[1]);
	printf("pid == [%d]\n", pid);
	
	res = kill(pid, SIGUSR1);
	if(res == 0){
		printf("kill OK\n");
	}else{
		printf("kill ERROR\n");
	}
	
}

