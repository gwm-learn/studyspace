#ifndef MOBILE_SERVICE_STATE_MACHINE_H
#define MOBILE_SERVICE_STATE_MACHINE_H

#include <pthread.h>

/* 状态机状态定义 */
typedef enum {
    MOBILE_STATE_NONE = 0,           /* 初始状态 */
    MOBILE_STATE_AT_READY,           /* AT命令就绪 */
    MOBILE_STATE_SIM_READY,          /* SIM卡就绪 */
    MOBILE_STATE_NETWORK_REGISTERED, /* 网络注册完成 */
    MOBILE_STATE_CONFIGURED,         /* 配置完成 */
    MOBILE_STATE_CONNECTED           /* 网络连接完成 */
} mobile_state_t;

/* 状态机结构体 */
typedef struct {
    bool enable;                     /* 状态机使能标志 */
    mobile_state_t current_state;    /* 当前状态 */
    pthread_mutex_t lock;            /* 线程锁 */
} state_machine_t;

int mobile_init_state_machine_service(void);
void mobile_deinit_state_machine_service(void);

void mobile_enable_state_machine(void);
void mobile_disable_state_machine(void);
bool mobile_get_state_machine_running_status(void);
void mobile_update_state_machine_status(mobile_state_t status);
mobile_state_t mobile_get_state_machine_status(void);

void mobile_modem_start(void);
void mobile_simcard_start(void);
int mobile_handle_sim_ready_check(void);
void mobile_update_wan_status(char *wanstatus);
const char* mobile_get_wan_status(void);

void mobile_state_machine_main(void);

#endif