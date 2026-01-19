#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "mobile_module_util.h"
#include "mobile_module_led.h"
#include "mobile_service_modem.h"
#include "mobile_service_state_machine.h"
#include "mobile_service_led.h"

/*
 * 文件名称：mobile_service_led.c
 * 功能描述：
 *     LED服务模块，提供网络模式、信号强度和互联网状态LED控制
 *
 * 作者：gaoweiming
 */

// 内部状态变量
static int g_led_module_initialized = 0;

/* 全局 LED 数组常量定义 */
const char * const NETWORK_LEDS[] = {"led_mobile2g", "led_mobile3g", "led_mobile4g", "led_mobile5g"};
const int NETWORK_LEDS_COUNT = sizeof(NETWORK_LEDS) / sizeof(NETWORK_LEDS[0]);

const char * const SIGNAL_LEDS[] = {"led_siglow", "led_sigmidd", "led_sighigh"};
const int SIGNAL_LEDS_COUNT = sizeof(SIGNAL_LEDS) / sizeof(SIGNAL_LEDS[0]);


/**
 * 初始化LED服务模块
 * 初始化LED控制模块
 *
 * @return 成功返回0，失败返回错误码
 */
int mobile_init_led_service(void) {
    if (g_led_module_initialized) {
        MOBILE_INFO("led service module already initialized\n");
        return 0;
    }
    
    mobile_light_mode_led(NULL, NULL, 0);
    mobile_light_signal_led(0, 0);
    mobile_light_pots_led(0);
    
    MOBILE_INFO("led service module initialized successfully\n");
    g_led_module_initialized = 1;
    return 0;
}

/**
 * 清理LED服务模块资源
 * 清理LED控制模块资源
 *
 * @return 无返回值
 */
void mobile_deinit_led_service(void) {
    if (!g_led_module_initialized) {
        return;
    }
    
    mobile_light_mode_led(NULL, NULL, 0);
    mobile_light_signal_led(0, 0);
    mobile_light_pots_led(0);
    
    MOBILE_INFO("led service module deinitialized\n");
    g_led_module_initialized = 0;
}


/**
 * @brief 控制网络模式LED
 * 
 * @param network_type 网络类型
 * @param led_name_sub LED名称
 *
 */
void mobile_light_mode_led_sub(const char *network_type, const char *led_name_sub) {
    if (led_name_sub == NULL) {
        MOBILE_ERROR("led_name_sub can not be null\n");
        return;
    }

    if (network_type == NULL) {
        MOBILE_WARN("No LED name or network type specified\n");
        return;
    }

    if (!strcmp(network_type, "5G") || !strcmp(network_type, "NR5G") ||
        !strcmp(network_type, "NR5G-SA") || !strcmp(network_type, "NR5G-NSA") ) {
        sprintf(led_name_sub, "%s", "led_mobile5g");
        MOBILE_DEBUG("Network type: 5G, using LED: %s\n", led_name_sub);
    } else if (!strcmp(network_type, "LTE") || !strcmp(network_type, "4G") ) {
        sprintf(led_name_sub, "%s", "led_mobile4g");
        MOBILE_DEBUG("Network type: 4G, using LED: %s\n", led_name_sub);
    } else if (!strcmp(network_type, "3G")) {
        sprintf(led_name_sub, "%s", "led_mobile3g");
        MOBILE_DEBUG("Network type: 3G, using LED: %s\n", led_name_sub);
    } else if (!strcmp(network_type, "2G")) {
        sprintf(led_name_sub, "%s", "led_mobile2g");
        MOBILE_DEBUG("Network type: 2G, using LED: %s\n", led_name_sub);
    } else {
        MOBILE_WARN("Unknown network type: %s\n", network_type);
        return;
    }
}

/**
 * @brief 控制网络模式LED
 * @param led_name LED名称，如果为NULL则根据网络类型自动选择
 * @param network_type 网络类型
 * @param on_off 控制开关：1-开启，0-关闭
 *
 * 根据网络类型控制对应的LED指示灯：
 * - 5G/NR5G: led_mobile5g
 * - LTE/4G: led_mobile4g
 * - 3G: led_mobile3g
 * - 2G: led_mobile2g
 * - 其他: led_nosignal
 */
void mobile_light_mode_led(const char *led_name, const char *network_type, int on_off) {
    int change = 0;
    static int local_on_off;
    char local_led_name_sub[16] = {0};
    static char local_network_type[16] = {"5G"};
    static char local_led_name[16] = {"led_mobile5g"}; // 默认使用5G LED

    if (led_name == NULL) {
        mobile_light_mode_led_sub(network_type, local_led_name_sub);
    } else {
        sprintf(local_led_name_sub, "%s", led_name);
    }

    if (local_on_off != on_off ) {
        local_on_off = on_off;
        change = 1;
    }

    if (network_type != NULL && strcmp(local_network_type, network_type)) {
        sprintf(local_network_type, "%s", network_type);
        change = 1;
    }

    if (strcmp(local_led_name, local_led_name_sub)) {
        sprintf(local_led_name, "%s", local_led_name_sub);
        change = 1;
    }

    if (!change) {
        return;
    }

    if (on_off) {
        mobile_led_set_trigger(local_led_name, "default-on");
    } else {
        for (int i = 0; i < NETWORK_LEDS_COUNT; i++) {
            mobile_led_set_trigger(NETWORK_LEDS[i], "none");
        }
        MOBILE_DEBUG("All network mode LEDs turned OFF\n");
    }
}

/**
 * @brief 控制信号强度LED
 * @param signal_mode 信号强度级别 (1-弱, 2-中等, 3-强)
 * @param on_off 控制开关：1-开启，0-关闭
 *
 * 根据信号强度级别控制三个信号LED（同时只能亮一个灯）：
 * - 信号弱(1): 点亮led_siglow
 * - 信号中等(2): 点亮led_sigmidd
 * - 信号强(3): 点亮led_sighigh
 */
static void mobile_light_signal_led_odu(int signal_mode, int on_off) {
    int change = 0;
    static int local_on_off;
    static int local_signal_mode;

    if (local_on_off != on_off ) {
        local_on_off = on_off;
        change = 1;
    }

    if (local_signal_mode != signal_mode) {
        local_signal_mode = signal_mode;
        change = 1;
    }

    if (!change) {
        return;
    }

    if (signal_mode < 0 || signal_mode > SIGNAL_LEDS_COUNT) {
        MOBILE_WARN("Invalid signal mode: %d, valid range: 0-%d\n", signal_mode, SIGNAL_LEDS_COUNT);
        signal_mode = 0;
    }

    if (!on_off) {
        for (int i = 0; i < SIGNAL_LEDS_COUNT; i++) {
            mobile_led_set_trigger(SIGNAL_LEDS[i], "none");
        }
        return;
    }

    for (int i = 0; i < SIGNAL_LEDS_COUNT; i++) {
        if (i < signal_mode) {
            mobile_led_set_trigger(SIGNAL_LEDS[i], "default-on");
        } else {
            mobile_led_set_trigger(SIGNAL_LEDS[i], "none");
        }
    }
    
    MOBILE_DEBUG("Signal LEDs set to mode %d\n", signal_mode);
}

/**
 * @brief 控制信号强度LED
 * @param signal_mode 信号强度级别 (1-弱, 2-中等, 3-强)
 * @param on_off 控制开关：1-开启，0-关闭
 *
 * 根据信号强度级别控制三个信号LED（同时只能亮一个灯）：
 * - 信号弱(1): 点亮led_siglow
 * - 信号中等(2): 点亮led_sigmidd
 * - 信号强(3): 点亮led_sighigh
 */
static void mobile_light_signal_led_cpe(int signal_mode, int on_off) {
    int change = 0;
    static int local_on_off;
    static int local_signal_mode;

    if (local_on_off != on_off ) {
        local_on_off = on_off;
        change = 1;
    }

    if (local_signal_mode != signal_mode) {
        local_signal_mode = signal_mode;
        change = 1;
    }

    if (!change) {
        return;
    }

    if (signal_mode < 0 || signal_mode > SIGNAL_LEDS_COUNT) {
        MOBILE_WARN("Invalid signal mode: %d, valid range: 0-%d\n", signal_mode, SIGNAL_LEDS_COUNT);
        signal_mode = 0;
    }

    for (int i = 0; i < SIGNAL_LEDS_COUNT; i++) {
        if (signal_mode != 0 && signal_mode - 1 == i){
            mobile_led_set_trigger(SIGNAL_LEDS[i], "default-on");
        } else {
            mobile_led_set_trigger(SIGNAL_LEDS[i], "none");
        }
    }
    
    MOBILE_DEBUG("Signal LEDs set to mode %d\n", signal_mode);
}

/**
 * @brief 控制信号强度LED
 * @param signal_mode 信号强度级别 (1-弱, 2-中等, 3-强)
 * @param on_off 控制开关：1-开启，0-关闭
 *
 * 根据信号强度级别控制三个信号LED（同时只能亮一个灯）：
 * - 信号弱(1): 点亮led_siglow
 * - 信号中等(2): 点亮led_sigmidd
 * - 信号强(3): 点亮led_sighigh
 */
void mobile_light_signal_led(int signal_mode, int on_off) {
    if (!strcmp(CUS_PARAMS_PRODUCT_TYPE, "ODU")) {
        mobile_light_signal_led_odu(signal_mode, on_off);
    } else {
        mobile_light_signal_led_cpe(signal_mode, on_off);
    }
}

/**
 * @brief 控制电话服务LED
 * @param on_off 控制开关：1-开启，0-关闭
 *
 * 控制VoLTE电话服务的LED指示灯。
 */
void mobile_light_pots_led(int on_off) {
    int change = 0;
    static int local_on_off;

    if (g_module_desc.voice_support != 1) {
        return;
    }

    if (local_on_off == on_off ) {
        return;
    }

    local_on_off = on_off;

    mobile_update_voice_led();

    /* 如果已配置且IMS状态正常，点亮对应的LED */
    if (strcmp(g_module_desc.voice_led, LED_VOLTE)) {
        mobile_led_set_trigger(LED_VOLTE, "none");
    } else if (on_off && current_state >= MOBILE_STATE_CONFIGURED && g_module_desc.imsststus) {
        mobile_led_set_trigger(g_module_desc.voice_led, "default-on");
    } else {
        mobile_led_set_trigger(g_module_desc.voice_led, "none");
    }
}

/**
 * @brief 控制互联网状态LED
 * @param on_off 控制开关：1-开启，0-关闭
 *
 * 控制互联网连接状态的LED指示灯。
 * 使用led_internet LED。
 */
void mobile_light_internet_led(int on_off) {
    int change = 0;
    static int local_on_off;
    const char *led_name = "led_internet";

    if (local_on_off == on_off ) {
        return;
    }

    local_on_off = on_off;

    if (on_off) {
        mobile_led_set_trigger(led_name, "default-on");
    } else {
        mobile_led_set_trigger(led_name, "none");
    }
}

/**
 * @brief 自动互联网LED控制
 *
 * 将互联网LED设置为网络设备模式，根据LTE网络流量自动控制LED状态。
 * 使用rmnet_mhi0.1作为网络设备。
 *
 * @param device_name 网络设备名称，如果为NULL则使用默认设备
 */
void mobile_light_internet_autoled(const char *device_name) {
    const char *led_name = "led_internet";
    const char *default_device = "rmnet_mhi0.1"; // 默认网络设备
    
    const char *target_device = (device_name != NULL && device_name[0] != '\0') ?
                               device_name : default_device;
    
    // 设置为网络设备模式
    int ret = mobile_led_set_netdev(led_name, target_device);
    if (ret != 0) {
        MOBILE_ERROR("Failed to set internet LED %s to netdev mode for device %s, ret=%d\n",
                     led_name, target_device, ret);
    } else {
        MOBILE_INFO("Internet LED %s set to netdev mode for device %s\n",
                    led_name, target_device);
    }
}

/**
 * 更新网络模式LED状态
 *
 * @param current_state 当前状态机状态
 * @param change_flag 变化标志（状态变化或网络类型变化）
 */
static void mobile_update_network_mode_led(mobile_state_t current_state, int change_flag) {
    if (!change_flag) {
        return;
    }
    
    if (current_state >= MOBILE_STATE_NETWORK_REGISTERED) {
        mobile_light_mode_led(NULL, g_module_desc.rf.net_type, 1);
    } else {
        mobile_light_mode_led(NULL, NULL, 0);
    }
}

/**
 * 更新信号强度LED状态
 *
 * @param current_state 当前状态机状态
 * @param state_change 状态是否发生变化
 * @param signal_change 信号强度是否发生变化
 */
static void mobile_update_signal_led(mobile_state_t current_state, int state_change, int signal_change) {
    if (!state_change && !signal_change) {
        return;
    }
    
    if (current_state < MOBILE_STATE_SIM_READY) {
        mobile_light_signal_led(0, 0);
        return;
    }
    
    int signal_level = g_module_desc.signalLevel;
    int signal_mode = 0;
    
    /* 根据信号强度级别确定LED模式 */
    if (signal_level == 1 || signal_level == 2) {
        signal_mode = 1;  /* 弱信号 */
    } else if (signal_level == 3 || signal_level == 4) {
        signal_mode = 2;  /* 中等信号 */
    } else if (signal_level == 5) {
        signal_mode = 3;  /* 强信号 */
    }
    
    mobile_light_signal_led(signal_mode, (signal_mode > 0));
}

/**
 * 更新电话服务LED状态
 *
 * @param current_state 当前状态机状态
 * @param state_change 状态是否发生变化
 * @param imsstatus_change IMS状态是否发生变化
 */
static void mobile_update_pots_led(mobile_state_t current_state, int state_change, int imsstatus_change) {
    if (!state_change && !imsstatus_change) {
        return;
    }
    
    int pots_enabled = (current_state >= MOBILE_STATE_CONFIGURED && g_module_desc.imsststus);
    mobile_light_pots_led(pots_enabled);
}

/**
 * 更新voice led name
 *
 * @return 无返回值
 */
void mobile_update_voice_led(void) {
    if (strcasecmp(g_module_desc.voice_mode, "VoIP") == 0) {
        g_module_desc.voice_led = LED_VOIP;
    } else {
        g_module_desc.voice_led = LED_VOLTE;
    }
}


/**
 * 更新LED状态
 *
 * 根据当前状态机状态、信号强度、IMS状态和网络类型更新对应的LED指示灯
 * - 网络模式LED: 显示当前网络类型(5G/4G/3G/2G)
 * - 信号强度LED: 显示信号强度级别(弱/中/强)
 * - 电话服务LED: 显示VoIP/VoLTE服务状态
 *
 * @return 无返回值
 */
void mobile_update_led_status(void) {
    static int last_signal = 0;
    static int last_imsstatus = 0;
    static char last_net_type[16] = {0};
    static mobile_state_t last_state = MOBILE_STATE_NONE;
    mobile_state_t current_state = MOBILE_STATE_NONE;

    mobile_update_signal_level();
    current_state = mobile_get_state_machine_status();
    
    /* 检测状态变化 */
    int state_change = (last_state != current_state);
    int signal_change = (last_signal != g_module_desc.signalLevel);
    int imsstatus_change = (last_imsstatus != g_module_desc.imsststus);
    int nettype_change = 0;
    
    /* 检测网络类型变化 */
    if (g_module_desc.rf.net_type != NULL) {
        if (strcmp(last_net_type, g_module_desc.rf.net_type) != 0) {
            nettype_change = 1;
        }
    } else if (last_net_type[0] != '\0') {
        nettype_change = 1;
    }
    
    /* 更新历史状态 */
    if (state_change) {
        last_state = current_state;
    }
    if (signal_change) {
        last_signal = g_module_desc.signalLevel;
    }
    if (imsstatus_change) {
        last_imsstatus = g_module_desc.imsststus;
    }
    if (nettype_change) {
        if (g_module_desc.rf.net_type != NULL) {
            strncpy(last_net_type, g_module_desc.rf.net_type, sizeof(last_net_type) - 1);
            last_net_type[sizeof(last_net_type) - 1] = '\0';
        } else {
            last_net_type[0] = '\0';
        }
    }
    
    /* 更新网络模式LED */
    mobile_update_network_mode_led(current_state, state_change || nettype_change);
    
    /* 更新信号强度LED */
    mobile_update_signal_led(current_state, state_change, signal_change);
    
    /* 更新电话服务LED */
    mobile_update_pots_led(current_state, state_change, imsstatus_change);
}

/**
 * 强制更新网络模式LED
 * 直接操作LED硬件，完全绕过状态缓存
 *
 * @param current_state 当前状态机状态
 * @return 无返回值
 */
static void mobile_force_update_network_mode_led(mobile_state_t current_state) {

    if (current_state >= MOBILE_STATE_NETWORK_REGISTERED) {
        const char *network_type = g_module_desc.rf.net_type;
        
        if (network_type != NULL) {
            if (strcmp(network_type, "5G") == 0 ||
                strcmp(network_type, "NR5G") == 0 ||
                strcmp(network_type, "NR5G-SA") == 0 ||
                strcmp(network_type, "NR5G-NSA") == 0) {
                mobile_led_set_trigger("led_mobile5g", "default-on");
            } else if (strcmp(network_type, "LTE") == 0 || strcmp(network_type, "4G") == 0) {
                mobile_led_set_trigger("led_mobile4g", "default-on");
            } else if (strcmp(network_type, "3G") == 0) {
                mobile_led_set_trigger("led_mobile3g", "default-on");
            } else if (strcmp(network_type, "2G") == 0) {
                mobile_led_set_trigger("led_mobile2g", "default-on");
            }
        }
    }
}

/**
 * 强制更新信号强度LED
 * 直接操作LED硬件，完全绕过状态缓存
 *
 * @param current_state 当前状态机状态
 * @return 无返回值
 */
static void mobile_force_update_signal_led(mobile_state_t current_state) {

    if (current_state >= MOBILE_STATE_SIM_READY) {
        int signal_level = g_module_desc.signalLevel;
        int signal_mode = 0;
        
        /* 根据信号强度级别确定LED模式 */
        if (signal_level == 1 || signal_level == 2) {
            signal_mode = 1;  /* 弱信号 */
        } else if (signal_level == 3 || signal_level == 4) {
            signal_mode = 2;  /* 中等信号 */
        } else if (signal_level == 5) {
            signal_mode = 3;  /* 强信号 */
        }
        
        /* 根据产品类型设置信号LED */
        if (!strcmp(CUS_PARAMS_PRODUCT_TYPE, "ODU")) {
            /* ODU产品：点亮多个LED表示信号强度 */
            for (int i = 0; i < SIGNAL_LEDS_COUNT; i++) {
                if (i < signal_mode) {
                    mobile_led_set_trigger(SIGNAL_LEDS[i], "default-on");
                }
            }
        } else {
            /* CPE产品：只点亮一个LED表示信号强度 */
            if (signal_mode > 0) {
                mobile_led_set_trigger(SIGNAL_LEDS[signal_mode - 1], "default-on");
            }
        }
    }
}

/**
 * 强制更新电话服务LED
 * 直接操作LED硬件，完全绕过状态缓存
 *
 * @param current_state 当前状态机状态
 * @return 无返回值
 */
static void mobile_force_update_pots_led(mobile_state_t current_state) {
    /* 检查是否支持语音功能 */
    if (g_module_desc.voice_support != 1) {
        return;
    }
    
    /* 更新语音LED类型 */
    mobile_update_voice_led();

    /* 如果已配置且IMS状态正常，点亮对应的LED */
    if (strcmp(g_module_desc.voice_led, LED_VOLTE)) {
        mobile_led_set_trigger(LED_VOLTE, "none");
    } else if (current_state >= MOBILE_STATE_CONFIGURED && g_module_desc.imsststus) {
        mobile_led_set_trigger(g_module_desc.voice_led, "default-on");
    } else {
        mobile_led_set_trigger(g_module_desc.voice_led, "none");
    }
}

/**
 * 强制更新所有LED状态
 *
 * 强制更新所有LED指示灯状态，不检查状态变化条件。
 * 与mobile_update_led_status()不同，此函数会忽略状态变化检测，
 * 直接强制更新所有LED到当前状态。
 *
 * 使用场景：
 * - 系统初始化时确保LED状态正确
 * - 手动强制刷新LED显示
 * - 状态异常时恢复LED显示
 *
 * @return 无返回值
 */
void mobile_update_led_force(void) {
    mobile_state_t current_state = mobile_get_state_machine_status();
    
    /* 强制更新网络模式LED */
    mobile_force_update_network_mode_led(current_state);
    
    /* 强制更新信号强度LED */
    mobile_force_update_signal_led(current_state);
    
    /* 强制更新电话服务LED */
    mobile_force_update_pots_led(current_state);
}