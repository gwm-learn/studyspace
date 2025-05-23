#ifndef __IPC_CLIENT__
#define __IPC_CLIENT__

#include <common/rt_type.h>
#include <osal/interface.h>
#include <platform_h/cj_plat_shm.h>
#include <platform_h/cj_ctl_msg.h>

#define CJ_PLAT_SHM_PID 2408
#define CJ_PLAT_SHM_VLAN_PID 2409
#define UNIX_SOCKET_FILE "/usr/cj_unix.file"

extern struct plat2web_msg *msg;
extern struct plat2web_vlan_msg *vlan_msg;

void stopSnmpdHandler(int signum);
int cj_plat_shm_init(void);
int cj_client_init(void);
int snmp_ipc_startup(void);
int snmp_get_msg(cj_ctl_msg_t *send_msg, int msg_len, char *recvbuf, int recvbuf_len);
int snmp_set_msg(cj_ctl_msg_t *send_msg, int msg_len);

void cj_plat_semw_begin(void);
void cj_plat_semw_end(void);
void cj_plat_sem_vlan_begin(void);
void cj_plat_sem_vlan_end(void);
#endif