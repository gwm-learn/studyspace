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
#include <sys/time.h>
#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>
#include "ipc_client.h"
#include "util.h"

static int sockfd;

void stopSnmpdHandler(int signum)
{
    if (sockfd >= 0){
        snmp_log(LOG_ERR, "close sockfd\n");
        close(sockfd);
    }
}

int cj_plat_shm_init(void)
{
    int cj_shmid = 0;
    int cj_snmp_shmid = 0;
    void *cj_shm = NULL;
    void *cj_snmp_shm = NULL;

    snmp_log(LOG_DEBUG, "cj_plat_shm_init\n");
    cj_shmid = shmget((key_t)CJ_PLAT_SHM_PID, 90000, 0666|IPC_CREAT);
    if(cj_shmid == -1)
    {  
        snmp_log(LOG_ERR, "shmget failed\n");
        return -1;
    }
    cj_shm = shmat(cj_shmid, (void*)0, 0);
    if(cj_shm == (void*)-1)
    {  
        snmp_log(LOG_ERR, "shmat failed\n");
        return -1;
    }  
    cj_snmp_shmid = shmget((key_t)CJ_PLAT_SHM_VLAN_PID, 60000, 0666|IPC_CREAT);
    if(cj_snmp_shmid == -1)
    {  
        snmp_log(LOG_ERR, "shmget failed\n");
        return -1;
    }
    cj_snmp_shm = shmat(cj_snmp_shmid, (void*)0, 0);
    if(cj_snmp_shm == (void*)-1)
    {  
        snmp_log(LOG_ERR, "shmat failed\n");
        return -1;
    }
    return 0;
}

int cj_client_init(void)
{
    struct sockaddr_un servaddr;
    struct timeval timeout={2,0};

    snmp_log(LOG_DEBUG, "cj_client_init\n");
    sockfd = socket(AF_UNIX, SOCK_STREAM,0);
    if (sockfd <= 0)
    {
        snmp_log(LOG_ERR, "socket fail\n");
        return -1;
    }

    setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout, sizeof(timeout));
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));

    memset(&servaddr,0,sizeof(servaddr));
    servaddr.sun_family = AF_UNIX;
    strcpy(servaddr.sun_path, UNIX_SOCKET_FILE);
    int serlen = sizeof(servaddr.sun_family) + strlen(servaddr.sun_path);
    if(connect(sockfd, (struct sockaddr *)&servaddr, serlen) == -1)
    {
        snmp_log(LOG_ERR, "connect fail\n");
        close(sockfd);
        return -1;
    }
    return 0;
}

int snmp_ipc_startup(void)
{
    cj_ctl_msg_t msg;

    snmp_log(LOG_DEBUG, "snmp_ipc_startup\n");
    memset(&msg, 0, sizeof(msg));
    msg.msg_type = CJ_MSG_TYPE_STARTUP;
    msg.protorl = CJ_CLIENT_SNMP;

    if (send(sockfd, (void *)&msg, sizeof(msg), 0) < 0){
        snmp_log(LOG_ERR, "sockfd err\n");
        return -1;
    }

    return 0;
}

int snmp_get_msg(cj_ctl_msg_t *send_msg, int msg_len, char *recvbuf, int recvbuf_len)
{
    if (sockfd < 0){
        snmp_log(LOG_ERR, "sockfd err\n");
        return -1;
    }

    snmp_log(LOG_DEBUG, "msg_type:%d protorl:%d sub_type:%d\n", send_msg->msg_type, send_msg->protorl, send_msg->sub_type.snmp_type);

    if (send(sockfd, send_msg, msg_len, 0) < 0){
        snmp_log(LOG_ERR, "send msg fail\n");
        return -1;
    }

    if (recv(sockfd, recvbuf, recvbuf_len, 0) < 0){
        snmp_log(LOG_ERR, "recv msg fail\n");
        return -1;
    }

    return 0;
}

int snmp_set_msg(cj_ctl_msg_t *send_msg, int msg_len)
{
    if (sockfd < 0){
        snmp_log(LOG_ERR, "sockfd err\n");
        return -1;
    }

    snmp_log(LOG_DEBUG, "msg_type:%d protorl:%d sub_type:%d\n", send_msg->msg_type, send_msg->protorl, send_msg->sub_type.snmp_type);

    if (send(sockfd, send_msg, msg_len, 0) < 0){
        snmp_log(LOG_ERR, "send msg fail\n");
        return -1;
    }

    return 0;
}