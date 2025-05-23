#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sem.h>
#include <unistd.h>

union semun
{
    int val;
    struct semid_ds *buf;
    unsigned short *arry;
};


void sem_init();

void sem_p();

void sem_v();

void sem_destroy();

