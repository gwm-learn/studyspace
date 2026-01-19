#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

#include "mobile_module_util.h"
#include "mobile_service_modem.h"
#include "mobile_service_dial.h"
#include "mobile_service_sms.h"
#include "mobile_service_lock.h"
#include "mobile_service_scan.h"
#include "mobile_service_state_machine.h"

/*
 * 文件名称：mobile_service_lock.c
 * 功能描述：
 *     lock服务模块，提供lock控制功能
 *     业务逻辑层，实现具体回调函数和init功能
 *
 * 作者：gaoweiming
 */

// 内部状态变量
static int g_lock_module_initialized = 0;
lock_config_t g_lock_config;

/**
 * 初始化LOCK服务模块
 * 注册所有锁的回调函数并初始化模块
 *
 * @return 成功返回0，失败返回错误码
 */
int mobile_init_lock_service(void) {
    if (g_lock_module_initialized) {
        MOBILE_INFO("lock service module already initialized\n");
        return 0;
    }

    if (mobile_init_simlock_config() != 0) {
        MOBILE_ERROR("Failed to init simlock config\n");
        return -1;
    }

    if (mobile_init_pinlock_config() != 0) {
        MOBILE_ERROR("Failed to init pinlock config\n");
        return -1;
    }

    if (mobile_init_bandlock_config() != 0) {
        MOBILE_ERROR("Failed to init bandlock config\n");
        return -1;
    }

    if (mobile_init_celllock_config() != 0) {
        MOBILE_ERROR("Failed to init celllock config\n");
        return -1;
    }

    g_lock_module_initialized = 1;
    MOBILE_INFO("lock service module initialized successfully\n");
    return 0;
}

/**
 * 清理LOCK服务模块资源
 * 清理所有锁的回调函数并释放资源
 *
 * @return 无返回值
 */
void mobile_deinit_lock_service(void) {
    if (!g_lock_module_initialized) {
        return;
    }

    mobile_free_whole_comm_list(&g_lock_config.cell_lock_config.pcid_lock);
    memset(&g_lock_config, '\0', sizeof(g_lock_config));
    g_lock_module_initialized = 0;
    MOBILE_INFO("lock service module deinitialized\n");
}

/**
 * 初始化simlock服务配置
 * 从UCI配置文件中读取SIM锁配置参数并初始化全局配置结构体
 * 包括SIM锁使能、密码、MCCMNC列表等配置项
 *
 * @return 成功返回0，失败返回错误码
 */
int mobile_init_simlock_config(void) {
    int ret = 0;

    ret = mobile_uci_get_int("mobile.simlock.enable", &g_lock_config.sim_lock_config.enable);
    if (g_lock_config.sim_lock_config.enable) {
        //ret |= mobile_uci_get("mobile.simlock.password", g_lock_config.sim_lock_config.password);
        ret |= mobile_uci_get("mobile.simlock.MCCMNCList", g_lock_config.sim_lock_config.mcc_mnc_list);
    }

    if (ret != 0) {
        MOBILE_WARN("Some UCI configuration parameters failed to load for SIM lock\n");
    }
    
    // 验证密码长度（如果设置了密码）
    if (g_lock_config.sim_lock_config.password[0] != '\0' && strlen(g_lock_config.sim_lock_config.password) < 4) {
        MOBILE_WARN("SIM lock password is too short (minimum 4 characters)\n");
    }
    
    mobile_write_buff_to_file(SIMLOCK_SUPPORT_FILE, "1", strlen("1"));
    MOBILE_INFO("SIM lock configuration loaded: enable=%d, password=%s, MCCMNCList=%s\n",
                g_lock_config.sim_lock_config.enable, g_lock_config.sim_lock_config.password, g_lock_config.sim_lock_config.mcc_mnc_list);
    return 0;
}

/**
 * 初始化pinlock服务配置
 * 从UCI配置文件中读取pin锁配置参数并初始化全局配置结构体
 *
 * @return 成功返回0，失败返回错误码
 */
int mobile_init_pinlock_config(void) {
    int ret = 0;

    ret = mobile_uci_get_int("mobile.@basic[0].pinEnable", &g_module_desc.basic_config.pin_enable);
    if (g_module_desc.basic_config.pin_enable) {
        ret |= mobile_uci_get_int("mobile.@basic[0].pinAutoUnlock", &g_module_desc.basic_config.pin_auto_unlock);
        ret |= mobile_uci_get("mobile.@basic[0].pinNumber",  g_module_desc.basic_config.pin_number);
    }

    if (ret != 0) {
        MOBILE_WARN("Some UCI configuration parameters failed to load\n");
    }

    MOBILE_INFO("PIN lock configuration loaded: enable=%d, pinAutoUnlock=%d, pinNumber=%s\n",
                g_module_desc.basic_config.pin_enable, g_module_desc.basic_config.pin_auto_unlock, g_module_desc.basic_config.pin_number);

    return 0;
}

/**
 * 初始化bandlock服务配置
 * 从UCI配置文件中读取band锁配置参数并初始化全局配置结构体
 *
 * @return 成功返回0，失败返回错误码
 */
int mobile_init_bandlock_config(void) {
    int ret = 0;

    // 从UCI配置读取频段锁配置参数
    ret = mobile_uci_get_int("mobile.bandlock.enable", &g_lock_config.band_lock_config.enable);
    if (g_lock_config.band_lock_config.enable) {
        ret |= mobile_uci_get("mobile.bandlock.netType", g_lock_config.band_lock_config.net_type);
        ret |= mobile_uci_get("mobile.bandlock.cfg3GBandList", g_lock_config.band_lock_config.cfg_3g_band_list);
        ret |= mobile_uci_get("mobile.bandlock.cfg4GBandList", g_lock_config.band_lock_config.cfg_4g_band_list);
        ret |= mobile_uci_get("mobile.bandlock.cfg5GBandList", g_lock_config.band_lock_config.cfg_5g_band_list);
        ret |= mobile_uci_get("mobile.bandlock.cfg5GnsaBandList", g_lock_config.band_lock_config.cfg_5gnsa_band_list);
    }

    if (ret != 0) {
        MOBILE_WARN("Some UCI configuration parameters failed to load for band lock\n");
    }

    // 初始化支持的频段列表
    ret = mobile_init_support_band_list(g_module_desc.pid, g_module_desc.vid, &g_lock_config.band_lock_config.support_band_list);
    if (ret != 0) {
        MOBILE_ERROR("Failed to initialize support band list for device PID=0x%04x, VID=0x%04x\n",
                     g_module_desc.pid, g_module_desc.vid);
        return -1;
    }

    // 成功获取频段列表，将支持的频段写入文件
    if (g_lock_config.band_lock_config.support_band_list != NULL) {
        mobile_write_bandlist_to_file(g_lock_config.band_lock_config.support_band_list);
    }

    mobile_write_buff_to_file(BANDLOCK_SUPPORT_FILE, "1", strlen("1"));
    MOBILE_INFO("Band lock configuration loaded: enable=%d, net_type=%s, device=%s (PID=0x%04x, VID=0x%04x)\n",
                g_lock_config.band_lock_config.enable, g_lock_config.band_lock_config.net_type,
                g_lock_config.band_lock_config.support_band_list ? g_lock_config.band_lock_config.support_band_list->module_name : "Unknown",
                g_module_desc.pid, g_module_desc.vid);

    MOBILE_DEBUG("3G Band List: %s\n", g_lock_config.band_lock_config.cfg_3g_band_list);
    MOBILE_DEBUG("4G Band List: %s\n", g_lock_config.band_lock_config.cfg_4g_band_list);
    MOBILE_DEBUG("5G Band List: %s\n", g_lock_config.band_lock_config.cfg_5g_band_list);
    MOBILE_DEBUG("5G NSA Band List: %s\n", g_lock_config.band_lock_config.cfg_5gnsa_band_list);

    return 0;
}

/**
 * 初始化celllock服务配置
 * 从UCI配置文件中读取cell锁配置参数并初始化全局配置结构体
 *
 * @return 成功返回0，失败返回错误码
 */
int mobile_init_celllock_config(void) {
    int ret = 0, enable = 0;
    char section[64] = {0};
    pcid_lock_config_t* new_node;

    g_lock_config.cell_lock_config.enable = false;
    g_lock_config.cell_lock_config.lock_net_type = OTHER_TYPE;

    mobile_uci_get_int("mobile.@basic[0].lastlockType", &g_lock_config.cell_lock_config.lock_net_type);
    if(mobile_uci_get_int("mobile.@basic[0].lastlockType", &g_lock_config.cell_lock_config.cell_lock_type) != 0) {
        g_lock_config.cell_lock_config.cell_lock_type = 1;
    }

    mobile_free_whole_comm_list(&g_lock_config.cell_lock_config.pcid_lock);
    for (int i = 0; i < MAX_PCIDCELL_LOCK_NUM; i++) {
        memset(section, 0, sizeof(section));
        new_node = (pcid_lock_config_t*)mobile_malloc_in_comm_list_tail((void**)&g_lock_config.cell_lock_config.pcid_lock, sizeof(pcid_lock_config_t));
        if (new_node == NULL) {
            mobile_free_whole_comm_list(&g_lock_config.cell_lock_config.pcid_lock);
            MOBILE_ERROR("malloc mobile %s fail\n", section);
            return -1;
        }

        sprintf(section, "pcidlock%d", i);
        ret = mobile_uci_get_option_int("mobile", section, "enable", &new_node->enable);
        if (ret != 0) {
            mobile_free_whole_comm_list(&g_lock_config.cell_lock_config.pcid_lock);
            MOBILE_ERROR("get mobile %s enable fail\n", section);
            return -1;
        }

        if (new_node->enable) {
            enable = 1;
            ret |= mobile_uci_get_option("mobile", section, "netType", new_node->nettype);
            ret |= mobile_uci_get_option("mobile", section, "pcid", new_node->pcid);
            ret |= mobile_uci_get_option("mobile", section, "freq", new_node->freq);
            ret |= mobile_uci_get_option("mobile", section, "SCS", new_node->scs);
            ret |= mobile_uci_get_option("mobile", section, "band", new_node->band);
            ret |= mobile_uci_get_option("mobile", section, "cellid", new_node->cellid);
            MOBILE_INFO("cell lock configuration loaded: enable=%d, nettype=%s, pcid=%s freq=%s, scs=%s, band=%s, cellid=%s\n",
                new_node->enable, new_node->nettype, new_node->pcid, new_node->freq, new_node->scs, new_node->band, new_node->cellid);
        }
    }

    mobile_debug_cell_lock_list();

    if (enable) {
        mobile_update_cell_lock_status(true);
    } else {
        mobile_update_cell_lock_status(false);
    }

    mobile_write_buff_to_file(CELLLOCK_SUPPORT_FILE, "1", strlen("1"));
    return 0;
}

/**
 * sim lock 执行函数
 *
 * @return true - 成功, false - 失败
 */
bool mobile_sub_sim_lock_p7006_v2c7c(void) {
    char cmd_buf[256] = {0};
    char result_buf[64] = {0};
    char plmn_list[MAX_PLMN_NUM][8] = {0};
    int ret = 0;
    int lock_stat = mobile_is_simcard_lock();
    
    MOBILE_INFO("SIM lock status: lock_stat=%d, enable=%d, MCCMNCList=%s\n",
                lock_stat, g_lock_config.sim_lock_config.enable,
                g_lock_config.sim_lock_config.mcc_mnc_list);

    // SIM锁启用：设置PLMN锁定
    if (g_lock_config.sim_lock_config.enable &&
        strlen(g_lock_config.sim_lock_config.mcc_mnc_list)) {
        
        // 解析MCCMNC列表
        char* token = strtok(g_lock_config.sim_lock_config.mcc_mnc_list, ",");
        int plmn_count = 0;
        
        while (token && plmn_count < MAX_PLMN_NUM) {
            strcpy(plmn_list[plmn_count++], token);
            token = strtok(NULL, ",");
        }

        // 处理每个PLMN
        for (int i = 0; i < plmn_count; i++) {
            // 解锁PLMN
            sprintf(cmd_buf, "'at+esmlck=0,0,\"%s\"'", PLMN_LOCK_PASSWORD);
            ret = mobile_at_cmd(cmd_buf, result_buf, sizeof(result_buf) - 1);
            MOBILE_DEBUG("Unlock PLMN: %s, ret=%d\n", cmd_buf, ret);

            // 第一次循环时清除所有频段锁定
            if (i == 0) {
                sprintf(cmd_buf, "'at+esmlck=0,3,\"%s\"'", PLMN_LOCK_PASSWORD);
                ret |= mobile_at_cmd(cmd_buf, result_buf, sizeof(result_buf) - 1);
                sleep(2);
                MOBILE_DEBUG("Clear all band locks: %s, ret=%d\n", cmd_buf, ret);
            }

            // 设置PLMN锁定
            sprintf(cmd_buf, "'at+esmlck=0,2,\"%s\",\"%s\"'", PLMN_LOCK_PASSWORD, plmn_list[i]);
            ret |= mobile_at_cmd(cmd_buf, result_buf, sizeof(result_buf) - 1);
            MOBILE_DEBUG("Set PLMN lock: %s, ret=%d\n", cmd_buf, ret);
        }
        sleep(2);
    }
    // SIM锁禁用：清除PLMN锁定
    else if (!g_lock_config.sim_lock_config.enable && lock_stat == 1) {
        sprintf(cmd_buf, "'at+esmlck=0,0,\"%s\"'", PLMN_LOCK_PASSWORD);
        ret = mobile_at_cmd(cmd_buf, result_buf, sizeof(result_buf) - 1);
        MOBILE_DEBUG("Disable PLMN lock: %s, ret=%d\n", cmd_buf, ret);

        sprintf(cmd_buf, "'at+esmlck=0,3,\"%s\"'", PLMN_LOCK_PASSWORD);
        ret |= mobile_at_cmd(cmd_buf, result_buf, sizeof(result_buf) - 1);
        MOBILE_DEBUG("Clear all band locks: %s, ret=%d\n", cmd_buf, ret);
        sleep(2);
    }
    
    MOBILE_INFO("SIM lock operation completed, final ret=%d\n", ret);
    return ret == 0 ? true : false;
}

/**
 * @brief pin 处理
 *
 */
bool mobile_sub_pin_lock_p7006_v2c7c(void) {
    bool ret = false;
    int pin_left_times = 0;

    pin_left_times = mobile_get_pin_left_times();
    MOBILE_DEBUG("pin_left_times:%d\n", pin_left_times);

#ifdef CUS_PARAMS_AREA_PRODUCT_STC
    ret = mobile_sub_pin_lock_stc();
#else
    MOBILE_DEBUG("pin_auto_unlock=%d, pincode_set=%d\n", g_module_desc.basic_config.pin_auto_unlock, g_lock_config.pin_lock_config.pincode_set);
    if (g_module_desc.basic_config.pin_auto_unlock && !g_lock_config.pin_lock_config.pincode_set) {
        mobile_update_pincode_set(1);
        mobile_write_buff_to_file(PINLOCK_STATUS_FILE, "pin lock", strlen("pin lock"));
        MOBILE_DEBUG("pincode_set=%d, pincode=%s\n", g_lock_config.pin_lock_config.pincode_set, g_module_desc.basic_config.pin_number);
        if (strlen(g_module_desc.basic_config.pin_number) > 3) {
            mobile_unlock_pin(g_module_desc.basic_config.pin_number);
            ret = true;
            sleep(2);
        }
    }
#endif

    return ret;
}

/**
 * 频段锁定主处理函数
 * 根据配置的网络类型和频段列表设置相应的频段锁定
 * 如果频段锁定未启用，则恢复所有支持的频段
 * 5g sa 和5g nsa 都使用mobile_band_lock_5g_sa
 * 在设置band之前获取当前系统的band设置，如果不同才进行设置
 * @return 总是返回true
 */
bool mobile_sub_band_lock_p7006_v2c7c(void) {
    char current_3g_band[128] = {0};
    char current_4g_band[128] = {0};
    char current_5g_band[128] = {0};
    
    // 获取当前系统band设置
    if (mobile_get_current_band_settings(current_3g_band, current_4g_band, current_5g_band, sizeof(current_3g_band)) != 0) {
        MOBILE_WARN("Failed to get current band settings, proceed with default behavior\n");
        // 获取失败时使用默认行为
        if (g_lock_config.band_lock_config.enable) {
            // 频段锁定启用时，直接设置配置的band
            mobile_apply_bandlock_by_net_type();
        } else {
            // 频段锁定未启用时，直接恢复所有支持的频段
            mobile_band_lock_restore_all();
        }
        return true;
    }
    
    // 调用合并的频段锁定处理函数
    mobile_handle_bandlock(g_lock_config.band_lock_config.enable, current_3g_band, current_4g_band, current_5g_band);
    return true;
}

/**
 * 小区锁子处理函数
 * 根据小区锁使能状态执行相应的锁定或清除操作
 * 当启用时先清除再设置，禁用时只清除
 *
 * @return 总是返回true
 */
bool mobile_sub_cell_lock_p7006_v2c7c(void) {
    if (g_lock_config.cell_lock_config.enable && g_lock_config.cell_lock_config.cell_lock_type == 1) {
        mobile_cell_lock_clear_pcid_list();
        mobile_cell_lock_set_pcid_list();
    } else {
        mobile_cell_lock_clear_pcid_list();
    }

    return true;
}


/**
 * SIM锁状态变化处理函数
 * 当SIM锁状态发生变化时被调用，处理SIM锁相关的状态变化
 *
 * @return true - 成功, false - 失败
 */
bool mobile_sim_lock_handler(void) {
    if (g_module_desc.pid == 0x7006 && g_module_desc.vid == 0x2c7c) {
        return mobile_sub_sim_lock_p7006_v2c7c();
    } else {
        return true;
    }
}

/**
 * PIN锁状态变化处理函数
 * 当PIN锁状态发生变化时被调用，处理PIN锁相关的状态变化
 *
 * @return true - 成功, false - 失败
 */
bool mobile_pin_lock_handler(void) {
    if (g_module_desc.pid == 0x7006 && g_module_desc.vid == 0x2c7c) {
        return mobile_sub_pin_lock_p7006_v2c7c();
    } else {
        return true;
    }
}

/**
 * 频段锁状态变化处理函数
 * 当频段锁状态发生变化时被调用，处理频段锁相关的状态变化
 *
 * @return true - 成功, false - 失败
 */
bool mobile_band_lock_handler(void) {
    if (g_module_desc.pid == 0x7006 && g_module_desc.vid == 0x2c7c) {
        return mobile_sub_band_lock_p7006_v2c7c();
    } else {
        return true;
    }
}

/**
 * 小区锁状态变化处理函数
 * 当小区锁状态发生变化时被调用，处理小区锁相关的状态变化
 *
 * @return true - 成功, false - 失败
 */
bool mobile_cell_lock_handler(void) {
    if (g_module_desc.pid == 0x7006 && g_module_desc.vid == 0x2c7c) {
        return mobile_sub_cell_lock_p7006_v2c7c();
    } else {
        return true;
    }
}

/**
 * 使能锁
 * @return 成功返回true，失败返回false
 */
bool mobile_apply_lock(void) {
    int ret = 0;
    static bool is_first_time_init = true;

    if (is_first_time_init) {
        ret = mobile_sim_lock_handler();
        ret |= mobile_band_lock_handler();
        ret |= mobile_cell_lock_handler();
    
        is_first_time_init = false;
        if (!ret) {
            MOBILE_DEBUG("all lock no need to setting\n");
            return true;
        }
        
        mobile_init_cfun(CFUN_4);
        return false;
    }
    return true;
}

/**
 * @brief 更新小区变更状态
 *
 * 检查小区ID是否在白名单中，做对应动作 need todo
 *
 * @return 无返回值
 */
void mobile_update_cell_change_stat(void) {
    if(g_lock_config.cell_lock_config.cell_lock_type != 1) {
        MOBILE_DEBUG("g_lock_config.cell_lock_config.cell_lock_type is %d\n", g_lock_config.cell_lock_config.cell_lock_type);
        return;
    }

    if (mobile_cellid_check_in_whitelist() == 2) {
        mobile_write_buff_to_file(CELL_CHANGE_STATUS_FILE, "1", strlen("1"));
        mobile_write_buff_to_file(CELL_CHANGE_STATUS_NOTE_WEB_FILE, "1", strlen("1"));
        
        MOBILE_DEBUG("cell change status detected, restarting network interfaces\n");
        mobile_network_dedial_ex("wan5g01", 0, 0);
        sleep(1);
        mobile_network_dedial_ex("wan5g02", 0, 0);
    } else {
        unlink(CELL_CHANGE_STATUS_NOTE_WEB_FILE);
    }
}

/**
 * @brief 检查sim卡是否就绪
 * 通过发送at+cpin?命令并检查返回结果来判断设备是否就绪
 *
 * @return -1 -- no sim
 *          0 -- sim ready
 *          1 -- pin lock
 *          2 -- puk lock
 *          3 -- MCCMNC Lock
 */
int mobile_sim_is_ready(bool tag) {
    char result[32] = {0};
    int sim_status = -1;
    const char* status_text = "Unknown";

    // 发送AT命令检查SIM卡状态
    mobile_at_cmd(g_module_desc.at_config->at_get_pin, result, sizeof(result) - 1);

    // Case 1: SIM READY
    if (strstr(result, "READY") != NULL || strstr(result, "ready") != NULL) {
        sim_status = 0;
        status_text = "Sim Ready";
        g_module_desc.simstatus = 1;
        mobile_update_pincode_set(0);
        //MOBILE_DEBUG("SIM card is ready\n");
        goto write_status;
    }

    // Case 2: SIM failure or not inserted
    if (strstr(result, "failure") != NULL || strstr(result, "not inserted") != NULL) {
        sim_status = -1;
        status_text = "No Sim";
        g_module_desc.simstatus = 0;
        unlink(PINLOCK_STATUS_FILE);
        MOBILE_WARN("SIM card failure or not inserted\n");
        goto write_status;
    }

    // Case 3: Not PIN/SIM related or contains ERROR
    if (strstr(result, "PIN") == NULL && strstr(result, "pin") == NULL &&
        strstr(result, "SIM") == NULL && strstr(result, "sim") == NULL) {
        sim_status = -1;
        status_text = "No Sim";
        g_module_desc.simstatus = 0;
        MOBILE_WARN("Response not related to PIN/SIM\n");
        goto write_status;
    }

    if (strstr(result, "ERROR") != NULL || strstr(result, "error") != NULL) {
        sim_status = -1;
        status_text = "No Sim";
        g_module_desc.simstatus = 0;
        if (strstr(result, "ERROR: 10") != NULL ) {
            unlink(PINLOCK_STATUS_FILE);
        }
        MOBILE_WARN("AT command returned ERROR\n");
        goto write_status;
    }

    // Case 4: PH-NET PIN (MCCMNC Lock)
    if (strstr(result, "PH-NET") != NULL || strstr(result, "ph-net") != NULL) {
        sim_status = 3;
        status_text = "MCCMNC Lock";
        g_module_desc.simstatus = 0;
        MOBILE_WARN("SIM card has MCCMNC lock\n");
        goto write_status;
    }

    // Case 5: PUK locked
    if (strstr(result, "PUK") != NULL || strstr(result, "puk") != NULL) {
        sim_status = 2;
        status_text = "Sim Puk";
        g_module_desc.simstatus = 0;
        MOBILE_WARN("SIM card is PUK locked\n");
        goto write_status;
    }

    // Case 6: PIN locked
    sim_status = 1;
    status_text = "Sim Pin";
    g_module_desc.simstatus = 0;
    MOBILE_WARN("SIM card is PIN locked\n");

write_status:
    if (tag) {
        MOBILE_INFO("sim :%s\n", status_text);
        mobile_write_buff_to_file(SIM_STATUS_FILE, status_text, strlen(status_text));
    }
    return sim_status;
}

/**
 * @brief 获取pin left try times
 *
 * @return pin left try times
 */
int mobile_get_pin_left_times(void) {
    char result[256] = {0};
    char* line_start = NULL;
    char* line_end = NULL;
    char* current_pos = result;
    int pin_left_times = 0;
    int puk_left_times = 0;
    
    // 检查AT配置是否有效
    if (g_module_desc.at_config == NULL) {
        MOBILE_ERROR("AT config is NULL, cannot get PIN left times\n");
        return 0;
    }
    
    // 使用mobile_at_cmd执行AT+QPINC?命令
    if (mobile_at_cmd("AT+QPINC?", result, sizeof(result) - 1) != 0) {
        MOBILE_ERROR("Failed to execute AT+QPINC? command\n");
        return 0;
    }
    
    // 解析返回结果
    if (strlen(result) == 0) {
        MOBILE_ERROR("Empty response from AT+QPINC? command\n");
        return 0;
    }
    
    MOBILE_DEBUG("AT+QPINC? response: %s\n", result);
    
    // 逐行解析响应，寻找"SC"行的PIN和PUK剩余次数
    while (*current_pos != '\0') {
        // 跳过空白字符
        while (*current_pos == ' ' || *current_pos == '\t' || *current_pos == '\r' || *current_pos == '\n') {
            current_pos++;
        }
        
        if (*current_pos == '\0') {
            break;
        }
        
        // 找到当前行的开始
        line_start = current_pos;
        
        // 找到当前行的结束（换行符或字符串结束）
        line_end = line_start;
        while (*line_end != '\0' && *line_end != '\r' && *line_end != '\n') {
            line_end++;
        }
        
        // 临时终止当前行
        char temp_char = *line_end;
        *line_end = '\0';
        
        // 检查当前行是否包含"SC"信息
        if (strstr(line_start, "+QPINC:") != NULL && strstr(line_start, "\"SC\"") != NULL) {
            // 解析"SC"行的PIN和PUK剩余次数
            // 格式: +QPINC: "SC",3,10
            char* comma1 = strchr(line_start, ',');
            if (comma1 != NULL) {
                char* comma2 = strchr(comma1 + 1, ',');
                if (comma2 != NULL) {
                    // 提取第一个逗号后的数字（PIN剩余次数）
                    char pin_str[8] = {0};
                    strncpy(pin_str, comma1 + 1, comma2 - comma1 - 1);
                    pin_str[comma2 - comma1 - 1] = '\0';
                    pin_left_times = atoi(pin_str);
                    
                    // 提取第二个逗号后的数字（PUK剩余次数）
                    char puk_str[8] = {0};
                    char* line_end_ptr = line_end;
                    while (*line_end_ptr != '\0' && (*line_end_ptr == '\r' || *line_end_ptr == '\n')) {
                        line_end_ptr++;
                    }
                    strncpy(puk_str, comma2 + 1, sizeof(puk_str) - 1);
                    puk_str[sizeof(puk_str) - 1] = '\0';
                    puk_left_times = atoi(puk_str);
                    
                    MOBILE_DEBUG("Found PIN left times: %d, PUK left times: %d\n", pin_left_times, puk_left_times);
                    *line_end = temp_char; // 恢复原始字符
                    break;
                }
            }
        }
        
        // 恢复原始字符并移动到下一行
        *line_end = temp_char;
        current_pos = line_end;
        if (*current_pos != '\0') {
            current_pos++;
        }
    }
    
    MOBILE_INFO("PIN left times: %d, PUK left times: %d\n", pin_left_times, puk_left_times);
    return pin_left_times;
}

/**
 * @brief 检查sim lock
 *
 * @return true : lock, false : not lock
 */
bool mobile_is_sim_pin_locked(void) {
    if (access(SIM_STATUS_FILE, F_OK) != 0) {
        return false;
    }
    FILE *file = fopen(SIM_STATUS_FILE, "r");
    if (file == NULL) {
        return false;
    }

    char buffer[16] = {0};
    bool result = false;

    if (fgets(buffer, sizeof(buffer) - 1, file) != NULL) {
        size_t len = strlen(buffer);
        while (len > 0 && isspace((unsigned char)buffer[len - 1])) {
            buffer[len - 1] = '\0';
            len--;
        }
        if (strcmp(buffer, "Sim Pin") == 0) {
            result = true;
        }
    }
    
    fclose(file);
    return result;
}

/**
 * 设置生产PIN码
 * 通过执行系统命令设置生产环境的PIN码
 *
 * @param pin 要设置的PIN码字符串，如果为NULL则直接返回
 * @return 无返回值
 */
void mobile_set_produce_pin(char *pin) {
    char command[32] = {0};

    if (pin == NULL)
        return;

    sprintf(command, "produce pinCode \"%s\"", pin);
    mobile_system_ex(command, 0);
}

/**
 * 更改SIM卡PIN码
 * 使用AT命令更改SIM卡的PIN码，支持从旧PIN码更改为新PIN码
 *
 * @param old_pin 旧的PIN码，不能为NULL
 * @param new_pin 新的PIN码，不能为NULL
 * @return 成功返回0，失败返回-1并在文件中记录错误状态
 */
int mobile_change_pin(const char* old_pin, const char* new_pin) {
    char cmdline[128] = {0};
    char result[256] = {0};

    sprintf(cmdline, "'at+CPWD=\"SC\",\"%s\",\"%s\"'", old_pin, new_pin);
    mobile_at_cmd(cmdline, result, sizeof(result));

    if (strlen(result) > 0 && strcasestr(result, "OK") != NULL) {
        MOBILE_DEBUG("change pin %s to %s success\n", old_pin, new_pin);
        return 0;
    } else {
        mobile_write_buff_to_file(SIM_STATUS_FILE, "PinError", strlen("PinError"));
        MOBILE_DEBUG("change pin %s to %s fail\n", old_pin, new_pin);
        return -1;
    }
}

/**
 * 解锁pin码
 *
 * @param pin_code 旧的PIN码，不能为NULL
 * @return 成功返回0，失败返回-1并在文件中记录错误状态
 */
int mobile_unlock_pin(const char* pin_code) {
    char cmdline[128] = {0};
    char result[256] = {0};

    sprintf(cmdline, "'AT+CPIN=\"%s\"'", pin_code);
    mobile_at_cmd(cmdline, result, sizeof(result));
    
    if (strlen(result) > 0 && strcasestr(result, "ERROR") != NULL) {
        mobile_write_buff_to_file(SIM_STATUS_FILE, "PinError", strlen("PinError"));
        MOBILE_DEBUG("unlock pin %s fail\n", pin_code);
        return -1;
    }
    MOBILE_DEBUG("unlock pin %s success\n", pin_code);
    return 0;
}

/**
 * 启用SIM卡PIN锁
 * 使用AT命令启用SIM卡的PIN码锁定功能
 *
 * @param pin_code 用于启用PIN锁的PIN码，不能为NULL
 * @return 成功返回0，失败返回-1
 */
int mobile_enable_pinlock(const char* pin_code) {
    char cmdline[128] = {0};
    char result[256] = {0};

    sprintf(cmdline, "'at+CLCK=\"SC\",1,\"%s\"'", pin_code);
    mobile_at_cmd(cmdline, result, sizeof(result));
    
    if (strlen(result) > 0 && strcasestr(result, "ERROR") != NULL) {
        MOBILE_DEBUG("enable pin %s fail\n", pin_code);
        return -1;
    }
    MOBILE_DEBUG("enable pin %s success\n", pin_code);
    return 0;
}

/**
 * 禁用SIM卡PIN锁
 * 使用AT命令禁用SIM卡的PIN码锁定功能
 *
 * @param pin_code 用于禁用PIN锁的PIN码，不能为NULL
 * @return 成功返回0，失败返回-1
 */
int mobile_disable_pinlock(const char* pin_code) {
    char cmdline[128] = {0};
    char result[256] = {0};

    sprintf(cmdline, "'at+CLCK=\"SC\",0,\"%s\"'", pin_code);
    mobile_at_cmd(cmdline, result, sizeof(result));
    
    if (strlen(result) > 0 && strcasestr(result, "ERROR") != NULL) {
        MOBILE_DEBUG("disable pin %s fail\n", pin_code);
        return -1;
    }
    MOBILE_DEBUG("disable pin %s success\n", pin_code);
    return 0;
}

/**
 * 设置g_lock_config.pin_lock_config.pincode_set
 *
 * @return 无返回值
 */
void mobile_update_pincode_set(int value) {
    g_lock_config.pin_lock_config.pincode_set = value;
}

/**
 * @brief 检查SIM卡MCCMNC是否在锁定的MCCMNC列表中
 *
 * 该函数用于检查当前SIM卡的MCCMNC是否在配置的锁定列表中。
 * 如果SIM锁功能启用且配置了MCCMNC列表，则检查SIM卡MCCMNC或APN MCCMNC是否在列表中。
 *
 * @return int
 *   - 0: MCCMNC在锁定列表中（允许使用）或SIM锁未启用
 *   - 1: MCCMNC不在锁定列表中（禁止使用）
 */
int mobile_is_simlock_mccmnc(void) {
    // 检查SIM锁是否启用且MCCMNC列表不为空
    if (g_lock_config.sim_lock_config.enable && g_lock_config.sim_lock_config.mcc_mnc_list[0] != '\0') {
        // 检查SIM卡MCCMNC是否在锁定列表中
        if (g_module_desc.sim_mccmnc[0] != '\0' &&
            strstr(g_lock_config.sim_lock_config.mcc_mnc_list, g_module_desc.sim_mccmnc) != NULL) {
            // SIM卡MCCMNC在锁定列表中，允许使用
            MOBILE_INFO("SIM MCCMNC %s is in SIM lock list, allowing usage\n", g_module_desc.sim_mccmnc);
            return 0;
        }
        
        // 检查APN MCCMNC是否在锁定列表中
        if (g_module_desc.apn_mccmnc[0] != '\0' &&
            strstr(g_lock_config.sim_lock_config.mcc_mnc_list, g_module_desc.apn_mccmnc) != NULL) {
            // APN MCCMNC在锁定列表中，允许使用
            MOBILE_INFO("APN MCCMNC %s is in SIM lock list, allowing usage\n", g_module_desc.apn_mccmnc);
            return 0;
        }
        
        // MCCMNC不在锁定列表中，禁止使用
        MOBILE_WARN("MCCMNC not in SIM lock list, denying usage. SIM MCCMNC: %s, APN MCCMNC: %s, Lock list: %s\n",
                    g_module_desc.sim_mccmnc, g_module_desc.apn_mccmnc, g_lock_config.sim_lock_config.mcc_mnc_list);
        return 1;
    }

    // SIM锁未启用或MCCMNC列表为空，允许使用
    MOBILE_DEBUG("SIM lock not enabled or MCCMNC list is empty, allowing usage\n");
    return 0;
}

/**
 * 查询simcard lock状态
 *
 * @return 成功返回lock状态，失败返回-1
 */
int mobile_is_simcard_lock(void) {
    int ret = -1;
    int lock_stat = 0;

    char result[64] = {0};

    if (mobile_at_cmd("'at+clck=\"pn\",2'", result, sizeof(result) - 1) == 0) {
        if (strcasestr(result, "+CLCK: 0") != NULL) {
            ret = 0;
        } else if (strcasestr(result, "+CLCK: 1") != NULL) {
            ret = 1;
        }
    }

    return ret;
}

/**
 * 应用频段锁定设置
 * 初始化网络模块并设置指定的频段模式
 *
 * @param pband_info 频段模式信息结构体指针
 * @return 成功返回QL_NW_SUCCESS，失败返回错误码
 */
int mobile_apply_bandlock_set(ql_nw_set_band_mode_info_t* pband_info) {
    int ret = -1;

    ret = ql_nw_init(NW_IPC_MODE_DEFAULT);
    if (ret != QL_NW_SUCCESS) {
        return ret;
    }

    ret = ql_nw_set_band_mode(pband_info);

    if (ret == QL_NW_SUCCESS) {
        MOBILE_INFO("set or restore band success!\n");
    }

    ret = ql_nw_release();

    return ret;
}

/**
 * @brief 将支持的频段列表写入文件
 *
 * 该函数将设备支持的频段列表写入指定的文件，供其他模块使用。
 *
 * @param band_list 支持的频段列表结构体指针
 * @return int 成功返回0，失败返回-1
 */
int mobile_write_bandlist_to_file(const support_band_list_t *band_list) {
    if (band_list == NULL) {
        MOBILE_ERROR("Band list is NULL, cannot write to file\n");
        return -1;
    }

    // 检查是否有支持的频段
    if (!band_list->band_list_3g[0] && !band_list->band_list_4g[0] &&
        !band_list->band_list_5g_sa[0] && !band_list->band_list_5g_nsa[0]) {
        MOBILE_WARN("No supported band lists found for device\n");
        return 0;
    }

    FILE* fp = fopen(BANDLIST_INFOS_FILE, "w+");
    if (fp == NULL) {
        MOBILE_ERROR("Failed to open band list file: %s\n", BANDLIST_INFOS_FILE);
        return -1;
    }

    char buf_line[128] = {0};

    // 写入3G频段列表
    if (band_list->band_list_3g[0]) {
        snprintf(buf_line, sizeof(buf_line), "support3GBandList=%s\n", band_list->band_list_3g);
        fputs(buf_line, fp);
    }

    // 写入4G频段列表
    if (band_list->band_list_4g[0]) {
        snprintf(buf_line, sizeof(buf_line), "support4GBandList=%s\n", band_list->band_list_4g);
        fputs(buf_line, fp);
    }

    // 写入5G SA频段列表
    if (band_list->band_list_5g_sa[0]) {
        snprintf(buf_line, sizeof(buf_line), "support5GBandList=%s\n", band_list->band_list_5g_sa);
        fputs(buf_line, fp);
    }

    // 写入5G NSA频段列表
    if (band_list->band_list_5g_nsa[0]) {
        snprintf(buf_line, sizeof(buf_line), "support5GnsaBandList=%s\n", band_list->band_list_5g_nsa);
        fputs(buf_line, fp);
    }

    fclose(fp);
    MOBILE_INFO("Band list information written to file: %s\n", BANDLIST_INFOS_FILE);
    return 0;
}

/**
 * 3G频段锁定处理函数
 * 解析3G频段列表并设置相应的频段锁定
 * 支持的3G频段：1, 5, 8
 *
 * @param band_list 3G频段列表字符串，格式为逗号分隔的频段编号
 * @return 无返回值
 */
void mobile_band_lock_3g(char *band_list) {
    int i = 0;
    int ret = -1;
    int num = 0;
    int change = 0;
    char* pstr = NULL;
    int band_num[128];
    char cmd[256] = {0};
    char tmpbuf[128] = {0};
    ql_nw_set_band_mode_info_t band_info;

    if (band_list == NULL || strlen(band_list) <= 0) {
        MOBILE_ERROR("band_list is null or band_list is empty\n");
        return;
    }

    memset(&band_info, 0, sizeof(band_info));
    memset(band_num, 0, sizeof(band_num));

    strncpy(tmpbuf, band_list, sizeof(tmpbuf) - 1);
    pstr = strtok(tmpbuf, ",");
    while (pstr != NULL && i < sizeof(band_num) / sizeof(band_num[0])) {
        band_num[i++] = atoi(pstr);
        pstr = strtok(NULL, ",");
    }

    num = i;
    band_info.set_band_option = 4;
    for (i = 0; i < num; i++) {
        if (band_num[i] == 1 || band_num[i] == 5 || band_num[i] == 8) {
            if (band_num[i] > 0 && band_num[i] < 33) {
                band_info.umts_band_class = band_info.umts_band_class | 1 << (band_num[i] - 1);
                change = 1;
            }
        } else {
            MOBILE_WARN("3g band not support, please re-try\n");
            change = 0;
            break;
        }
    }

    if (change == 1) {
        mobile_apply_bandlock_set(&band_info);
        MOBILE_DEBUG("band lock with %s\n", band_list);
    }
}

/**
 * 4G频段锁定处理函数
 * 解析4G频段列表并设置相应的频段锁定
 * 支持的4G频段：1, 3, 5, 7, 8, 20, 28, 32, 38, 40, 41, 42, 43, 46
 *
 * @param band_list 4G频段列表字符串，格式为逗号分隔的频段编号
 * @return 无返回值
 */
void mobile_band_lock_4g(char *band_list) {
    int i = 0;
    int num = 0;
    int ret = -1;
    int change = 0;
    char* pstr = NULL;
    int band_num[128];
    char cmd[256] = {0};
    char tmpbuf[128] = {0};
    ql_nw_set_band_mode_info_t band_info;

    if (band_list == NULL || strlen(band_list) <= 0) {
        MOBILE_ERROR("band_list is null or band_list is empty\n");
        return;
    }

    memset(&band_info, 0, sizeof(band_info));
    memset(band_num, 0, sizeof(band_num));

    strncpy(tmpbuf, band_list, sizeof(tmpbuf) - 1);
    pstr = strtok(tmpbuf, ",");
    while (pstr != NULL && i < sizeof(band_num) / sizeof(band_num[0])) {
        band_num[i++] = atoi(pstr);
        pstr = strtok(NULL, ",");
    }

    num = i;
    band_info.set_band_option = 1;
    for (i = 0; i < num; i++) {
        if (band_num[i] == 1 || band_num[i] == 3 || band_num[i] == 5 || band_num[i] == 7 || band_num[i] == 8 ||
            band_num[i] == 20 || band_num[i] == 28 || band_num[i] == 32 || band_num[i] == 38 || band_num[i] == 40 ||
            band_num[i] == 41 || band_num[i] == 42 || band_num[i] == 43 || band_num[i] == 46) {
            if (band_num[i] > 0 && band_num[i] < 33) {
                band_info.lte_band_class[0] = band_info.lte_band_class[0] | 1 << (band_num[i] - 1);
                change = 1;
            } else if (band_num[i] > 32 && band_num[i] < 65) {
                band_info.lte_band_class[1] = band_info.lte_band_class[1] | 1 << (band_num[i] - 33);
                change = 1;
            }
        } else {
            MOBILE_WARN("4g band not support, please re-try\n");
            change = 0;
            break;
        }
    }

    if (change == 1) {
        mobile_apply_bandlock_set(&band_info);
        MOBILE_DEBUG("band lock with %s\n", band_list);
    }
}

/**
 * 5G SA频段锁定处理函数
 * 解析5G SA频段列表并设置相应的频段锁定
 * 支持的5G SA频段：1, 3, 5, 7, 8, 20, 28, 38, 40, 41, 75, 76, 77, 78
 *
 * @param band_list 5G SA频段列表字符串，格式为逗号分隔的频段编号
 * @return 无返回值
 */
void mobile_band_lock_5g_sa(char *band_list) {
    int i = 0;
    int num = 0;
    int ret = -1;
    int change = 0;
    char* pstr = NULL;
    int band_num[128];
    char cmd[256] = {0};
    char tmpbuf[128] = {0};
    ql_nw_set_band_mode_info_t band_info;

    if (band_list == NULL || strlen(band_list) <= 0) {
        MOBILE_ERROR("band_list is null or band_list is empty\n");
        return;
    }

    memset(&band_info, 0, sizeof(band_info));
    memset(band_num, 0, sizeof(band_num));

    strncpy(tmpbuf, band_list, sizeof(tmpbuf) - 1);
    pstr = strtok(tmpbuf, ",");
    while (pstr != NULL && i < sizeof(band_num) / sizeof(band_num[0])) {
        band_num[i++] = atoi(pstr);
        pstr = strtok(NULL, ",");
    }

    num = i;
    band_info.set_band_option = 2;
    for (i = 0; i < num; i++) {
        if (band_num[i] == 1 || band_num[i] == 3 || band_num[i] == 5 || band_num[i] == 7 || band_num[i] == 8 ||
            band_num[i] == 20 || band_num[i] == 28 || band_num[i] == 38 || band_num[i] == 40 || band_num[i] == 41 ||
            band_num[i] == 77 || band_num[i] == 78 || band_num[i] == 75 || band_num[i] == 76) {
            if (band_num[i] > 0 && band_num[i] < 33) {
                band_info.nr_band_class[0] = band_info.nr_band_class[0] | 1 << (band_num[i] - 1);
                change = 1;
            } else if (band_num[i] > 32 && band_num[i] < 65) {
                band_info.nr_band_class[1] = band_info.nr_band_class[1] | 1 << (band_num[i] - 33);
                change = 1;
            } else if (band_num[i] > 64 && band_num[i] < 97) {
                band_info.nr_band_class[2] = band_info.nr_band_class[2] | 1 << (band_num[i] - 65);
                change = 1;
            }
        } else {
            MOBILE_WARN("5g sa band not support, please re-try\n");
            change = 0;
            break;
        }
    }

    if (change == 1) {
        mobile_apply_bandlock_set(&band_info);
        MOBILE_DEBUG("band lock with %s\n", band_list);
    }
}

/**
 * 5G NSA频段锁定处理函数
 * 解析5G NSA频段列表并设置相应的频段锁定
 * 支持的5G NSA频段：1, 3, 5, 7, 8, 20, 28, 38, 40, 41, 75, 76, 77, 78
 *
 * @param band_list 5G NSA频段列表字符串，格式为逗号分隔的频段编号
 * @return 无返回值
 */
void mobile_band_lock_5g_nsa(char *band_list) {
    int i = 0;
    int num = 0;
    int ret = -1;
    int change = 0;
    char* pstr = NULL;
    int band_num[128];
    char cmd[256] = {0};
    char tmpbuf[128] = {0};
    ql_nw_set_band_mode_info_t band_info;

    if (band_list == NULL || strlen(band_list) <= 0) {
        MOBILE_ERROR("band_list is null or band_list is empty\n");
        return;
    }

    memset(&band_info, 0, sizeof(band_info));
    memset(band_num, 0, sizeof(band_num));

    strncpy(tmpbuf, band_list, sizeof(tmpbuf) - 1);
    pstr = strtok(tmpbuf, ",");
    while (pstr != NULL && i < sizeof(band_num) / sizeof(band_num[0])) {
        band_num[i++] = atoi(pstr);
        pstr = strtok(NULL, ",");
    }

    num = i;
    band_info.set_band_option = 3;
    for (i = 0; i < num; i++) {
        if (band_num[i] == 1 || band_num[i] == 3 || band_num[i] == 5 || band_num[i] == 7 || band_num[i] == 8 ||
            band_num[i] == 20 || band_num[i] == 28 || band_num[i] == 38 || band_num[i] == 40 || band_num[i] == 41 ||
            band_num[i] == 77 || band_num[i] == 78 || band_num[i] == 75 || band_num[i] == 76) {
            if (band_num[i] > 0 && band_num[i] < 33) {
                band_info.nr_band_class[0] = band_info.nr_band_class[0] | 1 << (band_num[i] - 1);
                change = 1;
            } else if (band_num[i] > 32 && band_num[i] < 65) {
                band_info.nr_band_class[1] = band_info.nr_band_class[1] | 1 << (band_num[i] - 33);
                change = 1;
            } else if (band_num[i] > 64 && band_num[i] < 97) {
                band_info.nr_band_class[2] = band_info.nr_band_class[2] | 1 << (band_num[i] - 65);
                change = 1;
            }
        } else {
            MOBILE_WARN("5g nsa band not support, please re-try\n");
            change = 0;
            break;
        }
    }

    if (change == 1) {
        mobile_apply_bandlock_set(&band_info);
        MOBILE_DEBUG("band lock with %s\n", band_list);
    }
}

/**
 * 恢复所有频段锁定
 * 根据设备支持的频段列表恢复所有网络类型的频段锁定
 * 包括3G、4G、5G SA和5G NSA频段
 *
 * @return 无返回值
 */
void mobile_band_lock_restore_all(void) {
    if (strlen(g_lock_config.band_lock_config.support_band_list->band_list_5g_sa) > 0) {
        mobile_band_lock_5g_sa(g_lock_config.band_lock_config.support_band_list->band_list_5g_sa);
    }

    // if (strlen(g_lock_config.band_lock_config.support_band_list->band_list_5g_nsa) > 0) {
    //     mobile_band_lock_5g_nsa(g_lock_config.band_lock_config.support_band_list->band_list_5g_nsa);
    // }

    if (strlen(g_lock_config.band_lock_config.support_band_list->band_list_4g) > 0) {
        mobile_band_lock_4g(g_lock_config.band_lock_config.support_band_list->band_list_4g);
    }

    if (strlen(g_lock_config.band_lock_config.support_band_list->band_list_3g) > 0) {
        mobile_band_lock_3g(g_lock_config.band_lock_config.support_band_list->band_list_3g);
    }
}

/**
 * 生成频段十六进制字符串
 *
 * @param band_info 频段信息结构体
 * @param band_type 频段类型 (3G/4G/5G)
 * @param hex_str 输出十六进制字符串缓冲区
 * @param hex_size 缓冲区大小
 * @return 成功返回0，失败返回-1
 */
int mobile_generate_band_hex_string(const ql_nw_get_band_mode_info_t* band_info, const char* band_type, char* hex_str, size_t hex_size) {
    if (band_info == NULL || hex_str == NULL) {
        return -1;
    }
    
    if (strcmp(band_type, "3G") == 0) {
        // 3G频段数据：8个字符
        snprintf(hex_str, hex_size, "%08X", band_info->umts_band_class);
    } else if (strcmp(band_type, "4G") == 0) {
        // 4G频段数据：动态长度，只输出非零部分
        if (band_info->lte_band_class[3] != 0) {
            snprintf(hex_str, hex_size, "%08X%08X%08X%08X",
                     band_info->lte_band_class[0], band_info->lte_band_class[1],
                     band_info->lte_band_class[2], band_info->lte_band_class[3]);
        } else if (band_info->lte_band_class[2] != 0) {
            snprintf(hex_str, hex_size, "%08X%08X%08X",
                     band_info->lte_band_class[0], band_info->lte_band_class[1],
                     band_info->lte_band_class[2]);
        } else {
            snprintf(hex_str, hex_size, "%08X%08X",
                     band_info->lte_band_class[0], band_info->lte_band_class[1]);
        }
    } else if (strcmp(band_type, "5G") == 0) {
        // 5G频段数据：动态长度，只输出非零部分
        if (band_info->nr_band_class[3] != 0) {
            snprintf(hex_str, hex_size, "%08X%08X%08X%08X",
                     band_info->nr_band_class[0], band_info->nr_band_class[1],
                     band_info->nr_band_class[2], band_info->nr_band_class[3]);
        } else if (band_info->nr_band_class[2] != 0) {
            snprintf(hex_str, hex_size, "%08X%08X%08X",
                     band_info->nr_band_class[0], band_info->nr_band_class[1],
                     band_info->nr_band_class[2]);
        } else {
            snprintf(hex_str, hex_size, "%08X%08X",
                     band_info->nr_band_class[0], band_info->nr_band_class[1]);
        }
    } else {
        MOBILE_ERROR("Unknown band type: %s\n", band_type);
        return -1;
    }
    
    return 0;
}

/**
 * 逆序十六进制字符串
 *
 * @param hex_str 输入十六进制字符串
 * @param reversed_hex 输出逆序后的十六进制字符串
 * @param hex_size 缓冲区大小
 * @param band_type 频段类型 (3G/4G/5G)
 * @return 成功返回0，失败返回-1
 */
int mobile_reverse_hex_string(const char* hex_str, char* reversed_hex, size_t hex_size, const char* band_type) {
    if (hex_str == NULL || reversed_hex == NULL) {
        return -1;
    }
    
    size_t hex_len = strlen(hex_str);
    
    // 3G频段数据已经是正确的格式，不需要逆序
    if (strcmp(band_type, "3G") == 0) {
        strncpy(reversed_hex, hex_str, hex_size - 1);
        reversed_hex[hex_size - 1] = '\0';
        return 0;
    }
    
    // 4G和5G频段数据需要逆序合并
    if (hex_len == 32) {
        // 32字符格式：分割为四部分并逆序合并
        const char* part1 = hex_str;
        const char* part2 = hex_str + 8;
        const char* part3 = hex_str + 16;
        const char* part4 = hex_str + 24;
        snprintf(reversed_hex, hex_size, "%.8s%.8s%.8s%.8s", part4, part3, part2, part1);
    } else if (hex_len == 24) {
        // 24字符格式：分割为三部分并逆序合并
        const char* part1 = hex_str;
        const char* part2 = hex_str + 8;
        const char* part3 = hex_str + 16;
        snprintf(reversed_hex, hex_size, "%.8s%.8s%.8s", part3, part2, part1);
    } else if (hex_len == 16) {
        // 16字符格式：分割为两部分并逆序合并
        const char* part1 = hex_str;
        const char* part2 = hex_str + 8;
        snprintf(reversed_hex, hex_size, "%.8s%.8s", part2, part1);
    } else {
        MOBILE_ERROR("Invalid %s band hex length: %zu, expected 16, 24 or 32\n", band_type, hex_len);
        return -1;
    }
    
    return 0;
}

/**
 * 从二进制字符串提取频段列表
 *
 * @param bin_str 二进制字符串
 * @param bands 输出频段数组
 * @param max_bands 最大频段数量
 * @return 频段数量
 */
int mobile_extract_bands_from_binary(const char* bin_str, int* bands, size_t max_bands) {
    if (bin_str == NULL || bands == NULL) {
        return 0;
    }
    
    size_t bin_len = strlen(bin_str);
    int band_count = 0;
    
    // 反转二进制字符串以检查支持的band
    char reversed_bin[512] = {0};
    for (size_t i = 0; i < bin_len; i++) {
        reversed_bin[i] = bin_str[bin_len - 1 - i];
    }
    reversed_bin[bin_len] = '\0';
    
    // 计算支持的频段
    for (size_t i = 0; i < bin_len && band_count < (int)max_bands; i++) {
        // 从左往右计算，第0位（最左边）对应频段1，第1位对应频段2，依此类推
        if (reversed_bin[i] == '1') {
            bands[band_count++] = (int)(i + 1);
        }
    }
    
    return band_count;
}

/**
 * 构建逗号分隔的频段字符串
 *
 * @param bands 频段数组
 * @param band_count 频段数量
 * @param band_str 输出频段字符串缓冲区
 * @param band_size 缓冲区大小
 * @return 成功返回0，失败返回-1
 */
int mobile_build_band_string(const int* bands, int band_count, char* band_str, size_t band_size) {
    if (bands == NULL || band_str == NULL) {
        return -1;
    }
    
    if (band_count == 0) {
        snprintf(band_str, band_size, "");
        return 0;
    }
    
    // 构建逗号分隔的频段字符串
    memset(band_str, 0, band_size);
    
    for (int i = 0; i < band_count; i++) {
        char temp_band[16] = {0};
        snprintf(temp_band, sizeof(temp_band), "%d", bands[i]);
        if (i > 0) {
            strncat(band_str, ",", band_size - strlen(band_str) - 1);
        }
        strncat(band_str, temp_band, band_size - strlen(band_str) - 1);
    }
    
    return 0;
}

/**
 * 处理频段数据并转换为十进制字符串
 *
 * @param band_info 频段信息结构体
 * @param band_type 频段类型 (3G/4G/5G)
 * @param band_str 输出十进制字符串缓冲区
 * @param band_size 缓冲区大小
 * @return 成功返回0，失败返回-1
 */
int mobile_process_band_data(const ql_nw_get_band_mode_info_t* band_info, const char* band_type, char* band_str, size_t band_size) {
    char bin_str[512] = {0};
    char combined_hex[64] = {0};
    char processed_hex[64] = {0};
    int bands[128] = {0};
    int band_count = 0;
    
    if (band_info == NULL) {
        snprintf(band_str, band_size, "");
        return 0;
    }
    
    // 生成十六进制字符串
    if (mobile_generate_band_hex_string(band_info, band_type, combined_hex, sizeof(combined_hex)) != 0) {
        return -1;
    }
    
    MOBILE_DEBUG("band hex:[%s]\n", combined_hex);
    
    // 逆序十六进制字符串（4G和5G需要逆序）
    if (mobile_reverse_hex_string(combined_hex, processed_hex, sizeof(processed_hex), band_type) != 0) {
        return -1;
    }
    
    MOBILE_DEBUG("band hex:[%s]\n", processed_hex);
    
    // 转换为二进制
    if (mobile_hex_to_bin(processed_hex, bin_str, sizeof(bin_str)) != 0) {
        return -1;
    }
    
    // 从二进制字符串提取频段列表
    band_count = mobile_extract_bands_from_binary(bin_str, bands, sizeof(bands) / sizeof(bands[0]));
    
    // 构建频段字符串
    if (mobile_build_band_string(bands, band_count, band_str, band_size) != 0) {
        return -1;
    }
    
    MOBILE_INFO("bands:[%s]\n", band_str);
    
    return 0;
}

/**
 * 对频段字符串进行排序（按数字大小）
 * 
 * @param band_str 频段字符串（逗号分隔）
 * @param sorted_str 输出排序后的字符串缓冲区
 * @param sorted_size 缓冲区大小
 * @return 成功返回0，失败返回-1
 */
int mobile_sort_band_string(const char* band_str, char* sorted_str, size_t sorted_size) {
    if (band_str == NULL || strlen(band_str) == 0) {
        snprintf(sorted_str, sorted_size, "");
        return 0;
    }
    
    int bands[128] = {0};
    int band_count = 0;
    char temp[256] = {0};
    
    strncpy(temp, band_str, sizeof(temp) - 1);
    
    // 解析频段字符串
    char* token = strtok(temp, ",");
    while (token != NULL && band_count < sizeof(bands) / sizeof(bands[0])) {
        bands[band_count++] = atoi(token);
        token = strtok(NULL, ",");
    }
    
    // 排序频段
    for (int i = 0; i < band_count - 1; i++) {
        for (int j = i + 1; j < band_count; j++) {
            if (bands[i] > bands[j]) {
                int temp_band = bands[i];
                bands[i] = bands[j];
                bands[j] = temp_band;
            }
        }
    }
    
    // 构建排序后的字符串
    memset(sorted_str, 0, sorted_size);
    for (int i = 0; i < band_count; i++) {
        char temp_band[16];
        snprintf(temp_band, sizeof(temp_band), "%d", bands[i]);
        if (i > 0) {
            strncat(sorted_str, ",", sorted_size - strlen(sorted_str) - 1);
        }
        strncat(sorted_str, temp_band, sorted_size - strlen(sorted_str) - 1);
    }
    
    return 0;
}

/**
 * 比较两个频段字符串是否相同（忽略顺序）
 * 
 * @param band_str1 第一个频段字符串
 * @param band_str2 第二个频段字符串
 * @return 相同返回1，不同返回0，错误返回-1
 */
int mobile_compare_band_strings(const char* band_str1, const char* band_str2) {
    if (band_str1 == NULL || band_str2 == NULL) {
        return -1;
    }
    
    // 如果两个字符串都为空，则认为相同
    if (strlen(band_str1) == 0 && strlen(band_str2) == 0) {
        return 1;
    }
    
    // 如果只有一个为空，则认为不同
    if (strlen(band_str1) == 0 || strlen(band_str2) == 0) {
        return 0;
    }
    
    char sorted1[256] = {0};
    char sorted2[256] = {0};
    
    if (mobile_sort_band_string(band_str1, sorted1, sizeof(sorted1)) != 0 ||
        mobile_sort_band_string(band_str2, sorted2, sizeof(sorted2)) != 0) {
        return -1;
    }
    
    return strcmp(sorted1, sorted2) == 0 ? 1 : 0;
}

/**
 * 获取当前系统频段设置
 * 
 * @param current_3g_band 输出当前3G频段字符串
 * @param current_4g_band 输出当前4G频段字符串
 * @param current_5g_band 输出当前5G频段字符串
 * @param band_size 缓冲区大小
 * @return 成功返回0，失败返回-1
 */
int mobile_get_current_band_settings(char* current_3g_band, char* current_4g_band, char* current_5g_band, size_t band_size) {
    int ret = -1;
    ql_nw_get_band_mode_info_t band_info;
    
    if (current_3g_band == NULL || current_4g_band == NULL || current_5g_band == NULL) {
        MOBILE_ERROR("Invalid parameters: output buffers cannot be NULL\n");
        return -1;
    }
    
    memset(&band_info, 0, sizeof(band_info));
    band_info.get_band_option = 0;  // 获取当前设置的bands
    
    ret = ql_nw_init(NW_IPC_MODE_DEFAULT);
    if (ret != QL_NW_SUCCESS) {
        MOBILE_ERROR("Failed to initialize network module\n");
        return -1;
    }
    
    ret = ql_nw_get_band_mode(&band_info);
    if (ret != QL_NW_SUCCESS) {
        MOBILE_ERROR("Failed to get current band mode, ret=%d\n", ret);
        ql_nw_release();
        return -1;
    }
    
    ql_nw_release();
    
    // 转换3G频段
    if (mobile_process_band_data(&band_info, "3G", current_3g_band, band_size) != 0) {
        MOBILE_ERROR("Failed to process 3G band data\n");
        return -1;
    }
    
    // 转换4G频段
    if (mobile_process_band_data(&band_info, "4G", current_4g_band, band_size) != 0) {
        MOBILE_ERROR("Failed to process 4G band data\n");
        return -1;
    }
    
    // 转换5G频段
    if (mobile_process_band_data(&band_info, "5G", current_5g_band, band_size) != 0) {
        MOBILE_ERROR("Failed to process 5G band data\n");
        return -1;
    }
    
    MOBILE_INFO("Current band settings: 3G=%s, 4G=%s, 5G=%s\n", 
                current_3g_band, current_4g_band, current_5g_band);
    
    return 0;
}

/**
 * 根据网络类型设置频段锁定
 *
 * @return 无返回值
 */
void mobile_apply_bandlock_by_net_type(void) {
    if (strcasecmp(g_lock_config.band_lock_config.net_type, "5G") == 0 ||
        strcasecmp(g_lock_config.band_lock_config.net_type, "5GNSA") == 0) {
        mobile_band_lock_5g_sa(g_lock_config.band_lock_config.cfg_5g_band_list);
    } else if (strcasecmp(g_lock_config.band_lock_config.net_type, "4G") == 0) {
        mobile_band_lock_4g(g_lock_config.band_lock_config.cfg_4g_band_list);
    } else if (strcasecmp(g_lock_config.band_lock_config.net_type, "3G") == 0) {
        mobile_band_lock_3g(g_lock_config.band_lock_config.cfg_3g_band_list);
    } else {
        mobile_band_lock_restore_all();
    }
}

/**
 * 频段比对和处理函数
 * 比对当前频段和目标频段，如果不同则执行相应的处理函数
 *
 * @param current_3g_band 当前3G频段
 * @param current_4g_band 当前4G频段
 * @param current_5g_band 当前5G频段
 * @param target_3g_band 目标3G频段
 * @param target_4g_band 目标4G频段
 * @param target_5g_band 目标5G频段
 * @param context 上下文描述，用于日志输出
 * @param action_func 处理函数指针，当频段不同时调用
 * @return 无返回值
 */
void mobile_handle_bandlock_compare_and_restore(const char* current_3g_band, const char* current_4g_band, const char* current_5g_band, const char* target_3g_band, const char* target_4g_band, const char* target_5g_band, const char* context, void (*action_func)(void)) {
    // 比对当前频段和目标频段
    int same_3g = mobile_compare_band_strings(current_3g_band, target_3g_band);
    int same_4g = mobile_compare_band_strings(current_4g_band, target_4g_band);
    int same_5g = mobile_compare_band_strings(current_5g_band, target_5g_band);
    
    MOBILE_DEBUG("Band comparison (%s): 3G(same=%d), 4G(same=%d), 5G(same=%d)\n", context, same_3g, same_4g, same_5g);
    
    // 如果所有频段都相同，则不需要处理
    if (same_3g == 1 && same_4g == 1 && same_5g == 1) {
        MOBILE_INFO("Current band settings are the same as %s bands, no need to %s\n", context, action_func == mobile_band_lock_restore_all ? "restore" : "set band");
    } else {
        MOBILE_INFO("Band settings are different from %s bands, need to %s\n", context, action_func == mobile_band_lock_restore_all ? "restore" : "set band");
        action_func();
    }
}

/**
 * 处理频段锁定状态
 * 根据频段锁定使能状态执行相应的频段设置操作
 * 启用时比对配置的band和系统band，不同才设置
 * 禁用时比对系统band和support_band_list_t中的band，如果不同则重置
 *
 * @param enable 频段锁定使能状态
 * @param current_3g_band 当前3G频段
 * @param current_4g_band 当前4G频段
 * @param current_5g_band 当前5G频段
 * @return 无返回值
 */
void mobile_handle_bandlock(bool enable, const char* current_3g_band, const char* current_4g_band, const char* current_5g_band) {
    if (enable) {
        // 频段锁定启用：比对band_lock_config中的band和系统band，不同才设置
        // 检查配置的band列表是否都为空
        if (strlen(g_lock_config.band_lock_config.cfg_3g_band_list) == 0 &&
            strlen(g_lock_config.band_lock_config.cfg_4g_band_list) == 0 &&
            strlen(g_lock_config.band_lock_config.cfg_5g_band_list) == 0) {
            MOBILE_INFO("All band lists are empty, need to check if current bands match supported bands\n");
            
            // 检查支持频段列表是否存在
            if (g_lock_config.band_lock_config.support_band_list == NULL) {
                MOBILE_WARN("Support band list is NULL, restoring all bands\n");
                mobile_band_lock_restore_all();
                return;
            }
            
            // 使用子函数比对系统band和support_band_list_t中的band
            mobile_handle_bandlock_compare_and_restore(current_3g_band, current_4g_band, current_5g_band,
                g_lock_config.band_lock_config.support_band_list->band_list_3g, g_lock_config.band_lock_config.support_band_list->band_list_4g,
                g_lock_config.band_lock_config.support_band_list->band_list_5g_sa, "supported", mobile_band_lock_restore_all);
            return;
        }
        
        // 使用子函数比对band_lock_config中的band和系统band
        mobile_handle_bandlock_compare_and_restore(current_3g_band, current_4g_band, current_5g_band,
            g_lock_config.band_lock_config.cfg_3g_band_list, g_lock_config.band_lock_config.cfg_4g_band_list,
            g_lock_config.band_lock_config.cfg_5g_band_list, "configured", mobile_apply_bandlock_by_net_type);
    } else {
        // 频段锁定未启用：比对系统band和support_band_list_t中的band，如果不同则重置
        // 检查支持频段列表是否存在
        if (g_lock_config.band_lock_config.support_band_list == NULL) {
            MOBILE_WARN("Support band list is NULL, restoring all bands\n");
            mobile_band_lock_restore_all();
            return;
        }
        
        // 使用子函数比对系统band和support_band_list_t中的band
        mobile_handle_bandlock_compare_and_restore(current_3g_band, current_4g_band, current_5g_band,
            g_lock_config.band_lock_config.support_band_list->band_list_3g, g_lock_config.band_lock_config.support_band_list->band_list_4g,
            g_lock_config.band_lock_config.support_band_list->band_list_5g_sa, "supported", mobile_band_lock_restore_all);
    }
}


/**
 * 应用小区锁类型设置
 * 根据配置的小区锁类型发送相应的AT命令设置网络模式
 * 支持LTE和NR的不同锁定组合模式
 *
 * @return 无返回值
 */
void mobile_apply_cell_lock_net_type(void) {
    char result[128] = {0};
    char cmdline[128] = {0};

    switch (g_lock_config.cell_lock_config.lock_net_type) {
        case NO_LOCK:
        case OTHER_TYPE:
            break;

        case LOCK_LTE_AND_NR_BOTH:
        case LOCK_NR_ONLY_IN_NRNSA:
        case LOCK_LTE_ONLY_IN_NRNSA:
            snprintf(cmdline, sizeof(cmdline), "at+erat=19");
            mobile_at_cmd(cmdline, result, sizeof(result));
            break;

        case LOCK_LTE_ONLY:
            snprintf(cmdline, sizeof(cmdline), "at+erat=3");
            mobile_at_cmd(cmdline, result, sizeof(result));
            break;

        case LOCK_NR_ONLY:
            snprintf(cmdline, sizeof(cmdline), "at+erat=15");
            mobile_at_cmd(cmdline, result, sizeof(result));
            break;

        default:
            break;
    }
}

/**
 * 设置默认锁配置
 * 根据LTE和NR小区数量自动确定最佳的锁定类型
 * 考虑网络类型（NSA/SA）和小区数量组合
 *
 * @param lte_cell_count LTE小区数量
 * @param nr_cell_count NR小区数量
 * @return 无返回值
 */
void mobile_set_lock_default_config(int lte_cell_count, int nr_cell_count) {
    static lock_net_type_t lock_net_type = OTHER_TYPE;

    if (0 == (lte_cell_count + nr_cell_count)) {
        lock_net_type = NO_LOCK;
        MOBILE_DEBUG("lte cell lock count:%d, nr cell lock count:%d, do not cell lock\n", lte_cell_count, nr_cell_count);
        return;
    }

    if ((0 != lte_cell_count && 0 != nr_cell_count)) {
        lock_net_type = LOCK_LTE_AND_NR_BOTH;
        MOBILE_DEBUG("lte cell lock count:%d, nr cell lock count:%d, lock lte and nr both\n", lte_cell_count, nr_cell_count);
        return;
    }

    if (0 == lte_cell_count) {
        if (g_lock_config.cell_lock_config.lock_net_type == LOCK_NR_ONLY_IN_NRNSA || strcasecmp(g_module_desc.rf.net_type, "NR5G-NSA") == 0) {
            lock_net_type = LOCK_NR_ONLY_IN_NRNSA;
            MOBILE_DEBUG("lte cell lock count:%d, nr cell lock count:%d, only lock nr(nsa)\n", lte_cell_count, nr_cell_count);
        } else {
            lock_net_type = LOCK_NR_ONLY;
            MOBILE_DEBUG("lte cell lock count:%d, nr cell lock count:%d, only lock nr\n", lte_cell_count, nr_cell_count);
        }
    }

    if (0 == nr_cell_count) {
        if (g_lock_config.cell_lock_config.lock_net_type == LOCK_LTE_ONLY_IN_NRNSA || strcasecmp(g_module_desc.rf.net_type, "NR5G-NSA") == 0) {
            lock_net_type = LOCK_LTE_ONLY_IN_NRNSA;
            MOBILE_DEBUG("lte cell lock count:%d, nr cell lock count:%d, only lock lte(nr-nsa)\n", lte_cell_count, nr_cell_count);
        } else {
            lock_net_type = LOCK_LTE_ONLY;
            MOBILE_DEBUG("lte cell lock count:%d, nr cell lock count:%d, only lock lte\n", lte_cell_count, nr_cell_count);
        }
    }
    mobile_uci_set_int("mobile.@basic[0].lastlockType", lock_net_type);
}

/**
 * 更新小区锁状态
 * 当小区锁状态发生变化时更新全局配置中的使能状态
 *
 * @param status 新的小区锁状态，true表示启用，false表示禁用
 * @return 无返回值
 */
void mobile_update_cell_lock_status(bool status) {
    if (g_lock_config.cell_lock_config.enable != status) {
        MOBILE_DEBUG("cell lock status [%d] change to [%d]\n", g_lock_config.cell_lock_config.enable, status);
        g_lock_config.cell_lock_config.enable = status;
    }
}

/**
 * 设置小区锁PCI列表
 * 遍历配置的小区锁列表，构建LTE和NR小区信息结构
 * 初始化网络模块并设置小区ARFCN列表锁定
 *
 * @return 成功返回QL_NW_SUCCESS，失败返回错误码
 */
int mobile_cell_lock_set_pcid_list(void) {
    int ret = -1;
    int lte_cell_count = 0;
    lte_cell_lock_info lte_cell_list[MAX_PCIDCELL_LOCK_4G_NUM] = {0};
    int nr_cell_count = 0;
    nr_cell_lock_info nr_cell_list[MAX_PCIDCELL_LOCK_5G_NUM] = {0};
    ql_nw_set_cell_arfcn_list_lock_req_t set_cell_arfcn_list_info;
    pcid_lock_config_t* curr_node;

    curr_node = g_lock_config.cell_lock_config.pcid_lock;
    if (curr_node == NULL) {
        MOBILE_ERROR("no cell need to lock\n");
        return;
    }

    memset(&set_cell_arfcn_list_info, 0, sizeof(set_cell_arfcn_list_info));

    for (int i = 0; i < MAX_PCIDCELL_LOCK_NUM; i++) {
        if (curr_node == NULL) {
            break;
        }
        if (!curr_node->enable) {
            curr_node = curr_node->next;
            continue;
        }
        if (strcasecmp(curr_node->nettype, "5G") == 0 || strcasecmp(curr_node->nettype, "NR") == 0) {
            if (nr_cell_count >= MAX_PCIDCELL_LOCK_5G_NUM) {
                continue;
            }
            set_cell_arfcn_list_info.nr_cell_list[nr_cell_count].nr_arfcn = atoi(curr_node->freq);
            set_cell_arfcn_list_info.nr_cell_list[nr_cell_count].pci = atoi(curr_node->pcid);
            set_cell_arfcn_list_info.nr_cell_list[nr_cell_count].freq_band = atoi(curr_node->band);
            MOBILE_DEBUG("cell lock: id=%d,nr_arfcn=%d, pci=%d freq_band=%d\n", nr_cell_count,
                                 set_cell_arfcn_list_info.nr_cell_list[nr_cell_count].nr_arfcn,
                                 set_cell_arfcn_list_info.nr_cell_list[nr_cell_count].pci,
                                 set_cell_arfcn_list_info.nr_cell_list[nr_cell_count].freq_band);
            nr_cell_count++;
        } else if (strcasecmp(curr_node->nettype, "4G") == 0 || strcasecmp(curr_node->nettype, "LTE") == 0) {
            if (lte_cell_count >= MAX_PCIDCELL_LOCK_4G_NUM) {
                continue;
            }
            set_cell_arfcn_list_info.lte_cell_list[lte_cell_count].earfcn = atoi(curr_node->freq);
                set_cell_arfcn_list_info.lte_cell_list[lte_cell_count].pci = atoi(curr_node->pcid);
                MOBILE_DEBUG("cell lock: id=%d,earfcn=%d, pci=%d\n", lte_cell_count,
                                 set_cell_arfcn_list_info.lte_cell_list[lte_cell_count].earfcn,
                                 set_cell_arfcn_list_info.lte_cell_list[lte_cell_count].pci);
            lte_cell_count++;
        } else {
            MOBILE_DEBUG("nettype [%s] isn't support!\n", curr_node->nettype);
        }
        curr_node = curr_node->next;
    }

    mobile_set_lock_default_config(lte_cell_count, nr_cell_count);
    mobile_apply_cell_lock_net_type();
    set_cell_arfcn_list_info.opt = 1;
    set_cell_arfcn_list_info.lte_cell_count = lte_cell_count;
    set_cell_arfcn_list_info.nr_cell_count = nr_cell_count;

    ret = ql_nw_init(NW_IPC_MODE_DEFAULT);
    if (ret != QL_NW_SUCCESS) {
        MOBILE_DEBUG("ql_nw_init fail!\n");
        return ret;
    }
    ret = ql_nw_set_cell_arfcn_list_lock(&set_cell_arfcn_list_info);
    if (ret == QL_NW_SUCCESS) {
        MOBILE_DEBUG("set cell arfcn list lock success!\n");
    } else {
        MOBILE_DEBUG("set cell arfcn list lock fail!\n");
    }

    ret = ql_nw_release();

    return ret;
}

/**
 * 清除小区锁PCI列表
 * 清除所有已设置的小区ARFCN列表锁定
 * 恢复默认网络模式设置
 *
 * @return 成功返回QL_NW_SUCCESS，失败返回错误码
 */
int mobile_cell_lock_clear_pcid_list(void) {
    int ret = -1;
    char result[256] = {0};
    char cmdline[128] = {0};
    ql_nw_set_cell_arfcn_list_lock_req_t set_cell_arfcn_list_info;

    ret = ql_nw_init(NW_IPC_MODE_DEFAULT);
    if (ret != QL_NW_SUCCESS) {
        return ret;
    }

    memset(&set_cell_arfcn_list_info, 0, sizeof(set_cell_arfcn_list_info));
    set_cell_arfcn_list_info.opt = 0;
    ql_nw_set_cell_arfcn_list_lock(&set_cell_arfcn_list_info);
    if (ret == QL_NW_SUCCESS) {
        snprintf(cmdline, sizeof(cmdline), "at+erat=19");
        mobile_at_cmd(cmdline, result, sizeof(result));
        MOBILE_DEBUG("clear cell arfcn list lock success!\n");
    } else {
        MOBILE_ERROR("clear cell arfcn list lock fail!\n");
    }

    ret = ql_nw_release();

    return ret;
}

/**
 * @brief 检查小区ID是否在白名单中
 *
 * 检查当前小区ID是否在预定义的白名单中
 *
 * @return 0 - 未开启cell锁, 1 - 在白名单中, 2 - 不在白名单
 */
int mobile_cellid_check_in_whitelist(void) {
    unsigned int i = 0;
    unsigned int ret = 0;
    unsigned int current_pcid = 0;
    unsigned int current_arfcn = 0;
    int lock_enable = 0;
    int find_flag = 0;
    pcid_lock_config_t* curr_node;

    mobile_debug_cell_lock_list();

    curr_node = g_lock_config.cell_lock_config.pcid_lock;
    if (curr_node == NULL) {
        MOBILE_ERROR("no cell found\n");
        return 0;
    }

    current_pcid = (unsigned int)atoi(g_module_desc.rf.physical_cell_id);
    current_arfcn = (unsigned int)atoi(g_module_desc.rf.dl_earfcn);
    if ((current_pcid > 0) && (current_arfcn > 0)) {
        for (i = 0; i < MAX_PCIDCELL_LOCK_NUM; i++) {
            if (curr_node == NULL) {
                break;
            }
            if (!curr_node->enable) {
                curr_node = curr_node->next;
                continue;
            }
            lock_enable = 1;
            if(!strcmp(g_module_desc.rf.global_cell_id, curr_node->cellid)) //find in cell lock list
            {
                find_flag = 1;
                break;
            }
        }
    }

    if (lock_enable == 0) {
        return 0;
    }

    if (find_flag == 0) {
        return 2;
    }

    return 1;
}

/**
 * 更新PIN锁配置
 * 检查配置文件更新标志，重新初始化PIN锁配置
 *
 * @return 无返回值
 */
void mobile_update_pin_lock(void) {
    if ((access(DEFAULT_MOBILE_CONFIG_UPDATE, F_OK)) == 0) {
        MOBILE_DEBUG("***** check %s start update pin lock *****\n", DEFAULT_MOBILE_CONFIG_UPDATE);
        mobile_init_pinlock_config();
        unlink(DEFAULT_MOBILE_CONFIG_UPDATE);
    }
#ifdef CUS_PARAMS_AREA_PRODUCT_STC
    mobile_update_pin_lock_stc();
#endif
}

/**
 * 更新SIM锁配置
 * 检查SIM锁配置文件更新标志，重新初始化SIM锁配置并执行回调
 *
 * @return 成功更新返回1，未更新返回0
 */
int mobile_update_sim_lock(void) {
    if (access(DEFAULT_MOBILESIMLOCK_CONFIG_UPDATE, F_OK) == 0) {
        MOBILE_DEBUG("***** check %s start update sim lock *****\n", DEFAULT_MOBILESIMLOCK_CONFIG_UPDATE);
        mobile_init_simlock_config();
        mobile_sim_lock_handler();
        unlink(DEFAULT_MOBILESIMLOCK_CONFIG_UPDATE);
        return 1;
    }
    return 0;
}

/**
 * 更新频段锁配置
 * 检查频段锁配置文件更新标志，重新初始化频段锁配置并执行回调
 *
 * @return 成功更新返回1，未更新返回0
 */
int mobile_update_band_lock(void) {
    if (access(DEFAULT_MOBILEBANDLOCK_CONFIG_UPDATE, F_OK) == 0) {
        MOBILE_DEBUG("***** check %s start update band lock *****\n", DEFAULT_MOBILEBANDLOCK_CONFIG_UPDATE);
        mobile_init_bandlock_config();
        mobile_band_lock_handler();
        unlink(DEFAULT_MOBILEBANDLOCK_CONFIG_UPDATE);
        return 1;
    }
    return 0;
}

/**
 * 更新小区锁配置
 * 检查小区锁配置文件更新标志，重新初始化小区锁配置并执行回调
 *
 * @return 成功更新返回1，未更新返回0
 */
int mobile_update_cell_lock(void) {
    if (access(DEFAULT_MOBILEPCIDLOCK_CONFIG_UPDATE, F_OK) == 0) {
        MOBILE_DEBUG("***** check %s start update cell lock *****\n", DEFAULT_MOBILEPCIDLOCK_CONFIG_UPDATE);
        mobile_init_celllock_config();
        mobile_cell_lock_handler();
        unlink(DEFAULT_MOBILEPCIDLOCK_CONFIG_UPDATE);
        return 1;
    }
    return 0;
}

/**
 * 更新所有锁状态
 * 依次更新PIN锁、SIM锁、频段锁和小区锁的配置
 * 根据锁状态变化重新初始化网络功能并更新状态机
 *
 * @return 无返回值
 */
void mobile_update_lock_status(void) {
    int sim = 0, cell = 0;

    mobile_update_pin_lock();
    sim = mobile_update_sim_lock();
    mobile_update_band_lock();
    cell = mobile_update_cell_lock();

    if (sim || cell) {
        if (cell) {
            mobile_init_cfun(CFUN_4);
        } else {
            mobile_init_cfun(CFUN_0);
        }
        mobile_enable_state_machine();
        mobile_update_wan_status(DIALD_NONE);
        mobile_update_state_machine_status(MOBILE_STATE_NONE);
    }
}

/**
 * 打印锁配置信息
 * 打印g_lock_config结构体的内容，使用MOBILE_INFO宏
 *
 * @return 无返回值
 */
void mobile_print_lock_config_info(void) {
    MOBILE_INFO("=== Lock Configuration Information ===\n");
    MOBILE_INFO("SIM Lock - Enable: %d\n", g_lock_config.sim_lock_config.enable);
    MOBILE_INFO("SIM Lock - MNC Length: %d\n", g_lock_config.sim_lock_config.mnc_length);
    MOBILE_INFO("SIM Lock - Password: %s\n", g_lock_config.sim_lock_config.password);
    MOBILE_INFO("SIM Lock - Old Password: %s\n", g_lock_config.sim_lock_config.old_password);
    MOBILE_INFO("SIM Lock - MCC MNC List: %s\n", g_lock_config.sim_lock_config.mcc_mnc_list);
    MOBILE_INFO("\n");
    MOBILE_INFO("PIN Lock - Enable: %d\n", g_lock_config.pin_lock_config.enable);
    MOBILE_INFO("PIN Lock - PIN Code: %s\n", g_lock_config.pin_lock_config.pincode);
    MOBILE_INFO("PIN Lock - PIN Code Set: %d\n", g_lock_config.pin_lock_config.pincode_set);
    MOBILE_INFO("PIN Lock - Auto Unlock: %d\n", g_lock_config.pin_lock_config.auto_un_lock);
    MOBILE_INFO("\n");
    MOBILE_INFO("Band Lock - Enable: %d\n", g_lock_config.band_lock_config.enable);
    MOBILE_INFO("Band Lock - Net Type: %s\n", g_lock_config.band_lock_config.net_type);
    MOBILE_INFO("Band Lock - 3G Band List: %s\n", g_lock_config.band_lock_config.cfg_3g_band_list);
    MOBILE_INFO("Band Lock - 4G Band List: %s\n", g_lock_config.band_lock_config.cfg_4g_band_list);
    MOBILE_INFO("Band Lock - 5G Band List: %s\n", g_lock_config.band_lock_config.cfg_5g_band_list);
    MOBILE_INFO("Band Lock - 5G NSA Band List: %s\n", g_lock_config.band_lock_config.cfg_5gnsa_band_list);
    MOBILE_INFO("Band Lock - Support Band List: %p\n", g_lock_config.band_lock_config.support_band_list);
    MOBILE_INFO("\n");
    MOBILE_INFO("Cell Lock - Enable: %d\n", g_lock_config.cell_lock_config.enable);
    MOBILE_INFO("Cell Lock - Lock net type: %d\n", g_lock_config.cell_lock_config.lock_net_type);
    
    // 打印PCID锁链表信息
    pcid_lock_config_t *pcid_lock = g_lock_config.cell_lock_config.pcid_lock;
    int pcid_count = 0;
    while (pcid_lock != NULL) {
        if (pcid_lock->enable) {
            MOBILE_INFO("PCID Lock[%d] - Enable: %d, NetType: %s, PCID: %s, Freq: %s, SCS: %s, Band: %s, CellID: %s\n",
                   pcid_count, pcid_lock->enable, pcid_lock->nettype, pcid_lock->pcid, pcid_lock->freq, pcid_lock->scs, pcid_lock->band, pcid_lock->cellid);
        }
        pcid_lock = pcid_lock->next;
        pcid_count++;
    }
    MOBILE_INFO("\n");
}

/**
 * 打印cell_lock_list
 *
 * @return no return
 */
void mobile_debug_cell_lock_list(void) {
    pcid_lock_config_t* curr_node;
    int count = 0;

    curr_node = g_lock_config.cell_lock_config.pcid_lock;
    if (curr_node == NULL) {
        MOBILE_ERROR("no cell found\n");
        return;
    }
    
    for (int i = 0; i < MAX_PCIDCELL_LOCK_NUM; i++) {
        if (curr_node == NULL) {
            break;
        }
        if (!curr_node->enable) {
            curr_node = curr_node->next;
            continue;
        }

        count++;
        MOBILE_INFO("Node %d:\n", i);
        MOBILE_INFO("  Enable: %d\n", curr_node->enable);
        MOBILE_INFO("  NetType: %s\n", curr_node->nettype);
        MOBILE_INFO("  PCI: %s\n", curr_node->pcid);
        MOBILE_INFO("  Frequency: %s\n", curr_node->freq);
        MOBILE_INFO("  SCS: %s\n", curr_node->scs);
        MOBILE_INFO("  Band: %s\n", curr_node->band);
        MOBILE_INFO("  CellID: %s\n", curr_node->cellid);
        MOBILE_INFO("  --------------------\n");

        curr_node = curr_node->next;
    }

    if (count == 0) {
        MOBILE_INFO("No enabled cell lock nodes found\n");
    } else {
        MOBILE_INFO("Total enabled nodes: %d\n", count);
    }
}


