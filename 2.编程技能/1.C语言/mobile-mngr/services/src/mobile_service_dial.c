#include "mobile_module_util.h"
#include "mobile_service_modem.h"
#include "mobile_service_apn.h"
#include "mobile_service_state_machine.h"
#include "mobile_service_dial.h"

/*
 * 文件名称：mobile_service_dial.c
 * 功能描述：
 *     dial 拨号相关
 *
 * 作者：gaoweiming
 */

static int g_dial_module_initialized = 0;

/**
 * 初始化DIAL服务模块
 * 封装DIAL模块的初始化操作，提供标准的初始化框架
 *
 * @return 成功返回0，失败返回错误码
 */
int mobile_init_dial_service(void) {
    int ret = 0;
    if (g_dial_module_initialized) {
        MOBILE_INFO("dial service module already initialized\n");
        return 0;
    }
    
    g_dial_module_initialized = 1;
    if (ret == 0)
        MOBILE_INFO("dial service module initialized successfully\n");
    else
        MOBILE_ERROR("dial service module initialized fail\n");
    return ret;
}

/**
 * 清理DIAL服务模块资源
 * 封装DIAL模块的清理操作
 *
 * @return 无返回值
 */
void mobile_deinit_dial_service(void) {
    if (!g_dial_module_initialized) {
        return;
    }

    char cmd[128] = {0};
    module_mutil_config_t *current_config = g_module_desc.mutil_config;
    
    while (current_config != NULL) {
        if (mobile_check_interface_exists(current_config->iface) && mobile_check_interface_up(current_config->iface)) {
            snprintf(cmd, sizeof(cmd), "ifdown %s", current_config->iface);
            mobile_system_ex(cmd, 0);
            sleep(1);
        }
        current_config = current_config->next;
    }
    

    if (mobile_check_interface_exists(MOBILE_BASICWAN_NAME) && mobile_check_interface_up(MOBILE_BASICWAN_NAME)) {
        snprintf(cmd, sizeof(cmd), "ifdown %s", MOBILE_BASICWAN_NAME);
        mobile_system_ex(cmd, 0);
    }
    g_dial_module_initialized = 0;
    MOBILE_INFO("dial service module deinitialized\n");
}

/**
 * @brief init cfun
 * @param reload need to reset cfun 0/1
 */
void mobile_init_cfun(cfun_status status) {
    mobile_enable_state_machine();
    mobile_update_state_machine_status(MOBILE_STATE_NONE);
    mobile_execute_cfun_sequence(status);
}

/**
 * @brief set CFUN settings with enhanced state machine
 *
 * This function performs the following operations:
 * 1. If value is 1, set CFUN to 1 and return immediately
 * 2. If value is not 1, perform state machine operations:
 *    - Set CFUN to specified value
 *    - Set CFUN to 1 (full functionality mode)
 *    - Verify CFUN settings
 *
 * @param value CFUN value to set
 */
void mobile_execute_cfun_sequence(int value) {
    static cfun_state_t current_state = CFUN_STATE_INIT;
    
    MOBILE_DEBUG("Starting CFUN set, target value: %d\n", value);
    
    // If value is 1, set directly and return
    if (value == CFUN_1) {
        MOBILE_DEBUG("Setting CFUN to 1 directly\n");
        if (mobile_update_cfun(CFUN_1)) {
            MOBILE_DEBUG("CFUN set to 1 successfully\n");
        }
        current_state = CFUN_STATE_INIT; // Reset state machine
        return;
    }

    mobile_write_buff_to_file(DONGLE_DAILCTL_FILE, "cfun", strlen("cfun"));

    while (1) {
        switch (current_state) {
            case CFUN_STATE_INIT:
                // Step 1: Set CFUN to specified value
                MOBILE_DEBUG("Step 1: Setting CFUN to value %d\n", value);
                if (mobile_update_cfun(value)) {
                    current_state = CFUN_STATE_SET_VALUE;
                    MOBILE_DEBUG("Step 1 completed: CFUN set to %d\n", value);
                }
                break;
                
            case CFUN_STATE_SET_VALUE:
                // Step 2: Set CFUN to 1 (full functionality mode)
                MOBILE_DEBUG("Step 2: Setting CFUN to full functionality mode(1)\n");
                if (mobile_update_cfun(CFUN_1)) {
                    current_state = CFUN_STATE_SET_CFUN1;
                    MOBILE_DEBUG("Step 2 completed: CFUN set to full functionality mode\n");
                }
                break;
                
            case CFUN_STATE_SET_CFUN1:
                // Step 3: Verify CFUN settings
                MOBILE_DEBUG("Step 3: Verifying CFUN settings\n");
                if (mobile_check_cfun(CFUN_1)) {
                    current_state = CFUN_STATE_INIT;  // Reset state machine
                    MOBILE_DEBUG("Step 3 completed: CFUN verification successful, reset process completed\n");
                    return;  // Successfully completed, exit function
                }
                break;
                
            default:
                // Unknown state, reset to initial state
                MOBILE_DEBUG("Warning: Unknown state %d, resetting state machine\n", current_state);
                current_state = CFUN_STATE_INIT;
                break;
        }
        
        // Wait 1.5 second after each loop to avoid too frequent operations
        usleep(1.5 * 1000 * 1000);
    }
}

/**
 * @brief 设置at命令
 * @param value 设置的值
 * @return true - 执行成功, false - 执行失败
 */
bool mobile_update_cfun(int value){
    char result[256] = {0};
    char command[64] = {0};

    MOBILE_DEBUG("update cfun %d\n", value);
    snprintf(command, 256, "at+cfun=%d", value);
    if (mobile_at_cmd((const char*)command, result, sizeof(result)) == 0) {
        MOBILE_DEBUG("%s:[%s]\n", command, result);
        if (strcasestr(result, "OK")) {
            return true;
        }
        if (strcasestr(result, "Receive timed out")) {
            return false;
        }
    }

    return false;
}

/**
 * @brief Check if CFUN is set to the expected value
 *
 * @param expected_value Expected CFUN value to check
 * @return true if CFUN is set to expected value, false otherwise
 */
bool mobile_check_cfun(int expected_value){
    char result[256] = {0};
    char command[64] = {0};
    char expected_str[16] = {0};

    snprintf(command, 256, "at+cfun?");
    if (mobile_at_cmd((const char*)command, result, sizeof(result)) == 0) {
        MOBILE_DEBUG("[%s]\n", result);
        // Create the expected string pattern
        snprintf(expected_str, sizeof(expected_str), "+CFUN: %d", expected_value);
        if (strcasestr(result, expected_str)) {
            MOBILE_DEBUG("CFUN check passed: expected %d, actual %d\n", expected_value, expected_value);
            return true;
        }
        if (strcasestr(result, "Receive timed out")) {
            MOBILE_DEBUG("CFUN check failed: AT command timeout\n");
            return false;
        }
    }

    MOBILE_DEBUG("CFUN check failed: expected %d, but got different value\n", expected_value);
    return false;
}

/**
 * 拨号 
 *
 */
void mobile_network_dial_ex(const char *interface_name, bool enable, int vlanid) {
    char cmd[128] = {0};

    if (enable && (vlanid > 0 || strcmp(interface_name, MOBILE_BASICWAN_NAME) == 0)) {
        snprintf(cmd, sizeof(cmd), "ifup %s", interface_name);
        mobile_system_ex(cmd, 0);
        usleep(1.5 * 1000 * 1000);
    }
}

/**
 * 取消拨号
 *
 */
void mobile_network_dedial_ex(const char *interface_name, bool enable, int vlanid) {
    char cmd[128] = {0};

    if (mobile_check_interface_exists(interface_name) && mobile_check_interface_up(interface_name)) {
        snprintf(cmd, sizeof(cmd), "ifdown %s", interface_name);
        mobile_system_ex(cmd, 0);
        usleep(1.5 * 1000 * 1000);
    }
}

/**
 * 通用拨号函数
 *
 * @param interface_name 接口名称
 * @param enable 是否启用拨号
 * @param vlanid VLAN ID (仅multi模式需要，basic模式传0)
 */
void mobile_network_dial_common(const char *interface_name, bool enable, int vlanid) {
    unlink(DONGLE_DAILCTL_FILE);
    // 如果接口存在且已启动，先关闭
    mobile_network_dedial_ex(interface_name, enable, vlanid);
    // 如果启用拨号且满足条件，则启动接口
    mobile_network_dial_ex(interface_name, enable, vlanid);
}

/**
 * 拨号 basic
 *
 */
void mobile_network_dial_basic(void) {
    mobile_network_dial_common(MOBILE_BASICWAN_NAME, g_module_desc.basic_config.enable, 0);
}

/**
 * 拨号 单个 multi
 *
 */
void mobile_network_dial_multi_single(module_mutil_config_t *current_config) {
    mobile_network_dial_common(current_config->iface, current_config->enable, current_config->vlanid);
}

/**
 * 拨号 multi
 *
 */
static void mobile_network_dial_multi(void) {
    module_mutil_config_t *current_config = g_module_desc.mutil_config;

    while (current_config != NULL) {
        mobile_network_dial_multi_single(current_config);
        current_config = current_config->next;
    }
}

/**
 * 拨号
 *
 */
void mobile_network_dial(void) {
    mobile_network_dial_basic();
    mobile_network_dial_multi();
}
