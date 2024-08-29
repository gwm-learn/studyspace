#ifndef __IPC_CLIENT__
#define __IPC_CLIENT__

#include <common/rt_type.h>
#include <osal/interface.h>

#define MSG_LEN     128
#define CJ_PLAT_SHM_PID 2408
#define CJ_PLAT_SHM_VLAN_PID 2409
#define UNIX_SOCKET_FILE "/usr/cj_unix.file"

typedef enum cj_msg_type{
    CJ_MSG_TYPE_CLI = 1,
    CJ_MSG_TYPE_SDK,
    CJ_MSG_TYPE_CONFIG_SYNC,
    CJ_MSG_TYPE_STARTUP,
    CJ_MSG_TYPE_PLANT,
    CJ_MSG_TYPE_WEB_CTL,
    CJ_MSG_TYPE_SNMP,
    CJ_MSG_TYPE_ERROR_CODE,
    CJ_MSG_TYPE_END
}cj_msg_type;

typedef enum cj_protorl_type{
    CJ_CLIENT_LLDP = 1,
    CJ_CLIENT_STP,
    CJ_CLIENT_CLOUD,
    CJ_CLIENT_WEB,
    CJ_CLIENT_LACP,
    CJ_CLIENT_1X,
    CJ_CLIENT_SNMP,
    CJ_CLENT_END
}cj_protorl_type;

typedef enum cj_msg_snmp_sub_type{
    CJ_MSG_SNMP_SUB_TYPE_GET_MAC,
    CJ_MSG_SNMP_SUB_TYPE_SET_MAC,
    CJ_MSG_SNMP_SUB_TYPE_END
}cj_msg_snmp_sub_type;

typedef struct  cj_ctl_msg
{
    uint8 msg_type;
    uint8 protorl;
    union 
    {
        uint8 stp_type;
        uint8 plant_type;
        uint8 snmp_type;
    }sub_type;
    struct 
    {
        uint32 data;
        uint32 index;
        uint8  mac[6];
        uint32 ipv4;
        uint8  type;
        uint8  flag;
        uint8  enable;
        rtk_port_speed_t speed;
        uint8  link;
        char name[32];
    }msg;
    
}cj_ctl_msg_t;

void stopSnmpdHandler(int signum);
int cj_plat_shm_init(void);
int cj_client_init(void);
int snmp_ipc_startup(void);
int snmp_get_msg(cj_ctl_msg_t *send_msg, int msg_len, char *recvbuf, int recvbuf_len);
int snmp_set_msg(cj_ctl_msg_t *send_msg, int msg_len);

#endif