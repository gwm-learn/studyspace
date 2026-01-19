#include "mobile_module_util.h"
#include "mobile_service_modem.h"
#include "mobile_service_sms.h"
#include "mobile_service_lock.h"
#include "mobile_service_dial.h"
#include "mobile_service_voice.h"
#include "mobile_service_state_machine.h"
/*
 * 文件名称：mobile_service_state_machine.c
 * 功能描述：
 *     状态机管理函数
 *
 * 作者：gaoweiming
 */

// 内部状态变量
static int g_state_machine_initialized = 0;

state_machine_t g_state_machine = {
    .enable = false,
    .current_state = MOBILE_STATE_NONE,
    .lock = PTHREAD_MUTEX_INITIALIZER
};

/**
 * 初始化状态机服务模块
 * 提供状态机服务的初始化框架
 *
 * @return 成功返回0，失败返回错误码
 */
int mobile_init_state_machine_service(void) {
    if (g_state_machine_initialized) {
        MOBILE_INFO("State machine service module already initialized\n");
        return 0;
    }
    
    mobile_enable_state_machine();
    mobile_update_state_machine_status(MOBILE_STATE_NONE);
    
    g_state_machine_initialized = 1;
    MOBILE_INFO("State machine service module initialized successfully\n");
    return 0;
}

/**
 * 清理状态机服务模块资源
 * 提供状态机服务的资源清理
 *
 * @return 无返回值
 */
void mobile_deinit_state_machine_service(void) {
    if (!g_state_machine_initialized) {
        return;
    }

    mobile_disable_state_machine();
    mobile_update_state_machine_status(MOBILE_STATE_NONE);

    g_state_machine_initialized = 0;
    MOBILE_INFO("State machine service module deinitialized\n");
}

/**
 * 启用状态机
 */
void mobile_enable_state_machine(void)
{
    pthread_mutex_lock(&g_state_machine.lock);
    g_state_machine.enable = true;
    MOBILE_INFO("State machine enabled\n");
    pthread_mutex_unlock(&g_state_machine.lock);
}

/**
 * 禁用状态机
 */
void mobile_disable_state_machine(void)
{
    pthread_mutex_lock(&g_state_machine.lock);
    g_state_machine.enable = false;
    MOBILE_INFO("State machine disabled\n");
    pthread_mutex_unlock(&g_state_machine.lock);
}

/**
 * 获取状态机运行状态
 */
bool mobile_get_state_machine_running_status(void)
{
    bool enabled;
    pthread_mutex_lock(&g_state_machine.lock);
    enabled = g_state_machine.enable;
    pthread_mutex_unlock(&g_state_machine.lock);
    return enabled;
}

/**
 * 更新状态机状态
 */
void mobile_update_state_machine_status(mobile_state_t status)
{
    pthread_mutex_lock(&g_state_machine.lock);
    if (g_state_machine.current_state != status) {
        MOBILE_INFO("State changed: %d -> %d\n", g_state_machine.current_state, status);
        g_state_machine.current_state = status;
    }
    pthread_mutex_unlock(&g_state_machine.lock);
}

/**
 * 获取状态机状态
 */
mobile_state_t mobile_get_state_machine_status(void)
{
    mobile_state_t state;
    pthread_mutex_lock(&g_state_machine.lock);
    state = g_state_machine.current_state;
    pthread_mutex_unlock(&g_state_machine.lock);
    return state;
}

/**
 * 配置modem相关
 *
 * @return 无返回值
 */
void mobile_modem_start(void) {
    mobile_at_cmd("ATE0", NULL, 0);//close echo func
    mobile_init_imei();
    mobile_init_version();
    mobile_init_serialnum();
    mobile_init_sms_codes();
    mobile_init_pin();
    mobile_apply_lock();
}

/**
 * 配置simcard相关
 *
 * @return 无返回值
 */
void mobile_simcard_start(void) {
    mobile_init_imsi();
    mobile_init_ccid();
    mobile_init_odu_basic();
    mobile_init_auto_apn();
    mobile_init_apn_in_modem();
    mobile_apply_nettype_handler();
}

/**
 * 检查SIM卡就绪状态，并处理PIN锁尝试。
 * 返回SIM状态：0表示就绪，>0表示未就绪，<0表示错误。
 * 内部维护一个静态标志，避免重复尝试处理PIN锁。
 */
int mobile_handle_sim_ready_check(void) {
    static bool pin_lock_handler_failed = false; // true 表示处理失败，false 表示未失败
    int sim_status = mobile_sim_is_ready(true);
    if (sim_status > 0) {
        if (mobile_is_sim_pin_locked() && !pin_lock_handler_failed) {
            // 首次未失败时尝试处理；处理成功则 SIM 状态通常变为就绪，不再进入此分支；
            // 处理失败后标志置为 true，避免重复尝试。
            pin_lock_handler_failed = !mobile_sim_lock_handler();
        }
        goto check_sim_result;
    }
    pin_lock_handler_failed = false;
    if (sim_status < 0) {
        MOBILE_ERROR("Failed to check SIM status (error=%d)\n", sim_status);
        goto check_sim_result;
    }

check_sim_result:
    return sim_status;
}

/**
 * 更新wan状态
 *
 * @return 无返回值
 */
void mobile_update_wan_status(char *wanstatus) {
    bool status_change = false;
    char connect_time_str[16] = {0};
    static unsigned long connect_time = 0;
    static unsigned long last_update_time = 0;
    static unsigned long last_connect_time = 0; // 保存上一次的connect_time值
    unsigned long current_time = mobile_get_uptime_in_ms() / 1000; // 转换为秒

    if (g_module_desc.wanstatus == NULL) {
        g_module_desc.wanstatus = DIALD_NONE;
        status_change = true;
    }

    if (wanstatus != NULL && strcmp(wanstatus, g_module_desc.wanstatus)) {
        g_module_desc.wanstatus = wanstatus;
        status_change = true;
        mobile_update_device_name();
    }

    if (!strcmp(g_module_desc.wanstatus, DIALD_CONNECTED) && strstr(g_module_desc.rf.net_type, "5G")) {
        // 计算connect时间
        if (last_update_time == 0) {
            // 第一次进入连接状态，初始化时间
            last_update_time = current_time;
            connect_time = 0;
        } else {
            // 计算时间增量并累加
            unsigned long time_diff = current_time - last_update_time;
            if (time_diff > 0) {
                connect_time += time_diff;
                last_update_time = current_time;
            }
        }
    } else {
        // 非连接状态或非5G网络，重置时间
        connect_time = 0;
        last_update_time = 0;
    }
    
    // 只有当connect_time发生变化时才写入文件
    if (connect_time != last_connect_time) {
        sprintf(connect_time_str, "%d:%02d:%02d", (int)connect_time / 3600, (int)(connect_time % 3600 / 60), (int)connect_time % 60);
        mobile_write_buff_to_file(WAN_5G_CONNECT_TIME_FILE, connect_time_str, strlen(connect_time_str));
        last_connect_time = connect_time; // 更新上一次的connect_time值
    }
    
    if (status_change) {
        mobile_write_buff_to_file(WAN_STATUS_FILE, g_module_desc.wanstatus, strlen(g_module_desc.wanstatus));
    }
}

/**
 * 获取移动WAN连接状态
 * 返回当前移动WAN的连接状态字符串
 *
 * @return WAN状态字符串指针，可能为NULL
 */
const char* mobile_get_wan_status(void) {
    if (g_module_desc.wanstatus == NULL) {
        MOBILE_DEBUG("WAN status is NULL, returning DISCONNECTED\n");
        return DIALD_DISCONNECTED;
    }
    return g_module_desc.wanstatus;
}


/**
 * 状态机主循环函数
 * 执行状态机的状态转换逻辑，处理各个状态的条件检查和状态迁移
 * 提供详细的调试信息和错误处理
 *
 * @return 无返回值
 */
void mobile_state_machine_main(void) {
    if (!mobile_get_state_machine_running_status()) {
        return;
    }

    mobile_state_t current_state = mobile_get_state_machine_status();
    MOBILE_DEBUG("State machine main loop: current state = %d\n", current_state);

    switch (current_state) {
        case MOBILE_STATE_NONE:
            if (!mobile_at_is_ready()) {
                MOBILE_WARN("AT command not ready, staying in NONE state\n");
                break;
            }
            mobile_modem_start();
            mobile_update_wan_status(DIALD_INIT);
            mobile_update_state_machine_status(MOBILE_STATE_AT_READY);
            mobile_write_buff_to_file(DIALD_STATUS_FILE, DIALD_INIT, strlen(DIALD_INIT));
            break;

        case MOBILE_STATE_AT_READY:
            if (mobile_handle_sim_ready_check()) {
                MOBILE_WARN("SIM card not ready, staying in AT_READY state\n");
                break;
            }
            mobile_simcard_start();
            mobile_update_wan_status(DIALD_READY);
            mobile_update_state_machine_status(MOBILE_STATE_SIM_READY);
            mobile_write_buff_to_file(DIALD_STATUS_FILE, DIALD_READY, strlen(DIALD_READY));
            break;

        case MOBILE_STATE_SIM_READY:
            if (!mobile_get_basic_apn_status() || mobile_is_simlock_mccmnc()) {
                MOBILE_WARN("APN not configured or SIM locked, staying in SIM_READY state\n");
                break;
            }
            if (!mobile_network_is_registered(true)) {
                MOBILE_WARN("Network not registered, staying in SIM_READY state\n");
                if (!mobile_sim_is_ready(true)) {
                    MOBILE_WARN("Sim not ready, return AT_READY state\n");
                    mobile_update_state_machine_status(MOBILE_STATE_AT_READY);
                    break;
                }
                mobile_check_auto_apn();
                break;
            }
            mobile_get_provider();
            mobile_update_wan_status(DIALD_REG);
            mobile_update_state_machine_status(MOBILE_STATE_NETWORK_REGISTERED);
            mobile_write_buff_to_file(DIALD_STATUS_FILE, DIALD_REG, strlen(DIALD_REG));
            break;

        case MOBILE_STATE_NETWORK_REGISTERED:
            if (!mobile_get_basic_apn_status() || mobile_is_simlock_mccmnc()) {
                MOBILE_WARN("APN not configured or SIM locked, staying in NETWORK_REGISTERED state\n");
                break;
            }
            mobile_network_dial();
            mobile_update_wan_status(DIALD_CONNECTING);
            mobile_update_state_machine_status(MOBILE_STATE_CONFIGURED);
            mobile_write_buff_to_file(DIALD_STATUS_FILE, DIALD_CONNECTING, strlen(DIALD_CONNECTING));
            break;

        case MOBILE_STATE_CONFIGURED:
            if (!mobile_get_basic_apn_status()) {
                MOBILE_WARN("APN not configured, staying in CONFIGURED state\n");
                break;
            }
            if (!mobile_check_interface_up(MOBILE_BASICWAN_NAME)) {
                MOBILE_WARN("%s is not up yet\n", MOBILE_BASICWAN_NAME);
                break;
            }
            mobile_sync_auto_apn();
            mobile_network_slic(0);
            mobile_update_wan_status(DIALD_CONNECTED);
            mobile_write_buff_to_file(DIALD_STATUS_FILE, DIALD_CONNECTED, strlen(DIALD_CONNECTED));
            mobile_update_state_machine_status(MOBILE_STATE_CONNECTED);
            break;

        case MOBILE_STATE_CONNECTED:
            mobile_disable_state_machine();
            mobile_update_wan_status(DIALD_CONNECTED);
            MOBILE_INFO("State machine completed: reached CONNECTED state\n");
            break;

        default:
            mobile_update_wan_status(DIALD_NONE);
            mobile_update_state_machine_status(MOBILE_STATE_NONE);
            MOBILE_WARN("Reset state machine to NONE due to unknown state\n");
            break;
    }
}
