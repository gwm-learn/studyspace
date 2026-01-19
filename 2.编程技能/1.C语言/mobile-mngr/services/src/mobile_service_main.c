
#include "mobile_module_util.h"
#include "mobile_service_modem.h"
#include "mobile_service_sms.h"
#include "mobile_service_led.h"
#include "mobile_service_apn.h"
#include "mobile_service_lock.h"
#include "mobile_service_scan.h"
#include "mobile_service_voice.h"
#include "mobile_service_failover.h"
#include "mobile_service_signal.h"
#include "mobile_service_dial.h"
#include "mobile_service_state_machine.h"
#include "mobile_service_timer.h"
#include "mobile_service_log.h"
#include "mobile_service_info.h"
#include "mobile_service_main.h"
/*
 * 文件名称：mobile_service_main.c
 * 功能描述：
 *     移动管理器主服务程序，负责完整的移动网络管理
      包括状态机管理、信号处理、定时检查等功能
 *
 * 作者：gaoweiming
 */

/**
 * 初始化所有服务模块资源
 *
 * @return 0--成功 -1--失败
 */
int mobile_init_all_services(void) {
    if (mobile_init_log_service()) {
        MOBILE_ERROR("Failed to initialize log module\n");
        return -1;
    }
    if (mobile_init_signal_service()) {
        MOBILE_ERROR("Failed to initialize signal module\n");
        return -1;
    }
    if (mobile_init_modem_service()) {
        MOBILE_ERROR("Failed to initialize modem module\n");
        return -1;
    }
    if (mobile_init_sms_service()) {
        MOBILE_ERROR("Failed to initialize sms module\n");
        return -1;
    }
    if (mobile_init_apn_service()) {
        MOBILE_ERROR("Failed to initialize apn module\n");
        return -1;
    }
    if (mobile_init_lock_service()) {
        MOBILE_ERROR("Failed to initialize lock module\n");
        return -1;
    }
    if (mobile_init_voice_service()) {
        MOBILE_ERROR("Failed to initialize voice module\n");
        return -1;
    }
    if (mobile_init_led_service()) {
        MOBILE_ERROR("Failed to initialize led module\n");
        return -1;
    }
    if (mobile_init_dial_service()) {
        MOBILE_ERROR("Failed to initialize dial module\n");
        return -1;
    }
    if (mobile_init_failover_service()) {
        MOBILE_ERROR("Failed to initialize failover module\n");
        return -1;
    }
    if (mobile_init_scan_service()) {
        MOBILE_ERROR("Failed to initialize scan module\n");
        return -1;
    }
    if (mobile_init_info_service()) {
        MOBILE_ERROR("Failed to initialize info module\n");
        return -1;
    }
    if (mobile_init_state_machine_service()) {
        MOBILE_ERROR("Failed to initialize state machine module\n");
        return -1;
    }
    if (mobile_init_timer_service()) {
        MOBILE_ERROR("Failed to initialize timer module\n");
        return -1;
    }
    return 0;
}

/**
 * 清理所有服务模块资源
 * 集中管理所有模块的资源清理
 *
 * @return 无返回值
 */
int mobile_deinit_all_services(void) {
    mobile_deinit_timer_service();
    mobile_deinit_state_machine_service();
    mobile_deinit_info_service();
    mobile_deinit_scan_service();
    mobile_deinit_failover_service();
    mobile_deinit_dial_service();
    mobile_deinit_led_service();
    mobile_deinit_voice_service();
    mobile_deinit_lock_service();
    mobile_deinit_apn_service();
    mobile_deinit_sms_service();
    mobile_deinit_modem_service();
    mobile_deinit_signal_service();
    mobile_deinit_log_service();
    return 0;
}

int main(int argc, char* argv[]) {
    if (mobile_init_all_services()) {
        goto clean_all;
    }
    MOBILE_INFO("---------------------------------->>> mobile-mngr start ---------------------------------->>>\n");

    // 主循环
    while (!mobile_get_global_signal()) {
        if(!mobile_check_cell_band_scan_stat(SCAN_STAT_START)) {
            mobile_state_machine_main();
            mobile_timer_main();
        }
        sleep(2);
    }
    
    // 清理资源
clean_all:
    mobile_deinit_all_services();
    MOBILE_INFO("---------------------------------->>> mobile-mngr exit ---------------------------------->>>\n");
    return 0;
}