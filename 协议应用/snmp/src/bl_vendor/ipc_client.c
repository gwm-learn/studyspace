#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include "ipc_client.h"

int sockfd;
int cj_shmid = 0;
int cj_snmp_shmid = 0;
void *cj_shm = NULL;
void *cj_snmp_shm = NULL;

int cj_plat_shm_init(void)
{
    int i;
    printf("[SNMPD] %s\n", __func__);
    cj_shmid = shmget((key_t)CJ_PLAT_SHM_PID, 90000, 0666|IPC_CREAT);
    if(cj_shmid == -1)
    {  
        printf("[SNMPD] %s shmget failed\n", __func__);
        return -1;
    }
    cj_shm = shmat(cj_shmid, (void*)0, 0);
    if(cj_shm == (void*)-1)
    {  
        printf("[SNMPD] %s shmat failed\n", __func__);
        return -1;
    }  
    cj_snmp_shmid = shmget((key_t)CJ_PLAT_SHM_VLAN_PID, 60000, 0666|IPC_CREAT);
    if(cj_snmp_shmid == -1)
    {  
        printf("[SNMPD] %s shmget failed\n", __func__);
        return -1;
    }
    cj_snmp_shm = shmat(cj_snmp_shmid, (void*)0, 0);
    if(cj_snmp_shm == (void*)-1)
    {  
        printf("[SNMPD] %s shmat failed\n", __func__);
        return -1;
    }
    return 0;
}

int cj_client_init(void)
{
    struct sockaddr_un servaddr;

    printf("[SNMPD] %s \n", __func__);
    sockfd = socket(AF_UNIX, SOCK_STREAM,0);
    memset(&servaddr,0,sizeof(servaddr));
    servaddr.sun_family = AF_UNIX;
    strcpy(servaddr.sun_path, UNIX_SOCKET_FILE);
    int serlen = sizeof(servaddr.sun_family) + strlen(servaddr.sun_path);
    if(connect(sockfd, (struct sockaddr *)&servaddr, serlen) == -1)
    {
        printf("[SNMPD] %s connect fail \n", __func__);
        close(sockfd);
        return -1;
    }
    return sockfd;
}

void snmp_ipc_startup(void)
{
    cj_ctl_msg_t msg;

    printf("[SNMPD] %s\n", __func__);
    memset(&msg, 0, sizeof(msg));
    msg.msg_type = CJ_MSG_TYPE_STARTUP;
    msg.protorl = CJ_CLIENT_SNMP;
    send(sockfd, (void *)&msg, sizeof(msg), 0);
}

void snmp_get_msg(cj_ctl_msg_t *send_msg, int msg_len, char *recvbuf, int recvbuf_len, int fd)
{
    printf("[SNMPD] %s\n", __func__);
    send(fd, &send_msg, msg_len, 0);
    recv(fd, recvbuf, recvbuf_len,0);
}

void snmp_set_msg(cj_ctl_msg_t *send_msg, int msg_len, int fd)
{
    printf("[SNMPD] %s\n", __func__);
    send(fd, &send_msg, msg_len, 0);
}