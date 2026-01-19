#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <pthread.h>

#include "mobile_module_util.h"
#include "mobile_service_modem.h"
#include "mobile_service_lock.h"
#include "mobile_service_led.h"
#include "mobile_service_voice.h"
#include "mobile_service_state_machine.h"
#include "mobile_service_apn.h"
#include "mobile_service_timer.h"

/*
 * 文件名称：mobile_service_timer.c
 * 功能描述：
 *     TIMER定时器服务模块，提供定时器功能管理
 *     业务逻辑层，直接实现定时器功能，不需要module层
 *
 * 作者：gaoweiming
 */

// 内部状态变量
static int g_timer_module_initialized = 0;

/**
 * 初始化TIMER服务模块
 * 提供定时器功能的初始化框架
 *
 * @return 成功返回0，失败返回错误码
 */
int mobile_init_timer_service(void) {
    if (g_timer_module_initialized) {
        MOBILE_INFO("timer service module already initialized\n");
        return 0;
    }
    
    g_timer_module_initialized = 1;
    MOBILE_INFO("timer service module initialized successfully\n");
    return 0;
}

/**
 * 清理TIMER服务模块资源
 * 提供定时器功能的资源清理
 *
 * @return 无返回值
 */
void mobile_deinit_timer_service(void) {
    if (!g_timer_module_initialized) {
        return;
    }
    
    g_timer_module_initialized = 0;
    MOBILE_INFO("timer service module deinitialized\n");
}

/**
 * @brief 检查模组状态，AT，CPIN，并作对应动作，用状态机方式实现
 *
 * 该函数用于定时检查modem的状态，包括AT命令就绪、SIM卡就绪和网络注册状态。
 * 当状态机未运行时，执行modem状态检查，并根据检查结果更新状态机状态。
 *
 * @return 无返回值
 */
void mobile_timer_check_modem(void) {
    char sim_mccmnc[32] = {0};
    static unsigned int s_last_modem_check_time = 0;
    unsigned int now_time = mobile_get_uptime_in_ms();

    if (mobile_get_state_machine_running_status()) {
        return;
    }

    if ((now_time - s_last_modem_check_time) < 4000) {
        return;
    }

    s_last_modem_check_time = now_time;

    if (mobile_sim_is_ready(false) != 0) {
        mobile_enable_state_machine();
        mobile_update_wan_status(DIALD_NONE);
        mobile_update_state_machine_status(MOBILE_STATE_NONE);
        return;
    }

    snprintf(sim_mccmnc, sizeof(sim_mccmnc), g_module_desc.sim_mccmnc);
    mobile_get_sim_mccmnc();
    if (strcmp(sim_mccmnc, g_module_desc.sim_mccmnc)) {
        MOBILE_DEBUG("sim mccmnc [%s] change to [%s] !\n", sim_mccmnc, g_module_desc.sim_mccmnc);
        mobile_enable_state_machine();
        mobile_update_wan_status(DIALD_NONE);
        mobile_update_state_machine_status(MOBILE_STATE_NONE);
        return;
    }
}

/**
 * 处理ussd
 *
 * @return 无返回值
 */
void mobile_process_ussd(void) {
    FILE* fp = NULL;
    char buf[256] = {0};
    char ussd[128] = {0};
    char result[128] = {0};

    if (mobile_get_state_machine_running_status()) {
        return;
    }

    if ((access(USSD_EXE_FILE, F_OK)) != 0) {
        return;
    }

    MOBILE_DEBUG("process ussd feature.\n");
    fp = fopen(USSD_EXE_FILE, "r");
    if (fp == NULL) {
        return;
    }

    if (fgets(ussd, sizeof(ussd), fp) > 0) {
        for (int i = 0; i < strlen(ussd); i++) {
            if (ussd[i] == '\n') {
                ussd[i] = 0;
                break;
            }
        }
        sprintf(buf, "AT+BCUSD='%s' -w", ussd);
        mobile_at_cmd(buf, result, sizeof(result));
    }
    fclose(fp);
    unlink(USSD_EXE_FILE);
}

/**
 * 定时器主循环函数
 * 执行定时器相关的周期性任务，提供定时检查和维护功能
 * 当状态机运行时暂停定时器功能
 *
 * @return 无返回值
 */
void mobile_timer_main(void) {
    if (!g_timer_module_initialized) {
        MOBILE_WARN("Timer service not initialized, skipping timer main loop\n");
        return;
    }

    mobile_update_lock_status();

    mobile_check_update_apn();

    if (mobile_get_state_machine_running_status()) {
        MOBILE_DEBUG("State machine is running, pausing timer functions\n");
        return;
    }

    mobile_network_slic(1);

    mobile_timer_check_modem();

    mobile_update_wan_status(NULL);

    mobile_update_led_status();

    mobile_process_ussd();
}