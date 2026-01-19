#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "mobile_module_util.h"
#include "mobile_service_modem.h"
#include "mobile_service_led.h"
#include "mobile_service_voice.h"
/*
 * 文件名称：mobile_service_voice.c
 * 功能描述：
 *     VOICE语音服务模块，提供语音功能管理
 *     业务逻辑层，直接实现语音功能，不需要module层
 *
 * 作者：gaoweiming
 */

// 内部状态变量
static int g_voice_module_initialized = 0;

/**
 * 初始化VOICE服务模块
 * 提供语音功能的初始化框架，包括语音通道配置、编解码器初始化等
 *
 * @return 成功返回0，失败返回错误码
 */
int mobile_init_voice_service(void) {
    if (g_voice_module_initialized) {
        MOBILE_INFO("Voice service module already initialized\n");
        return 0;
    }

    int ret = 0;
    
    MOBILE_DEBUG("Initializing voice service module...\n");

    ret = mobile_init_voice_config();

    if (ret != 0) {
        MOBILE_ERROR("Voice service module initialization failed, ret=%d\n", ret);
        return ret;
    }

    g_voice_module_initialized = 1;
    MOBILE_INFO("Voice service module initialized successfully\n");
    return 0;
}

/**
 * 初始化uci voice配置
 *
 * @return 成功返回0，失败返回错误码
 */
int mobile_init_voice_config(void) {
    if (g_module_desc.pid == 0x7006 && g_module_desc.vid == 0x2c7c) {
        if (!strcmp(CUS_PARAMS_PRODUCT_TYPE, "ODU")) {
            g_module_desc.voice_support = 0;
            mobile_write_buff_to_file(VOLTECS_SUPPORT_FILE, "0", strlen("0"));
        } else {
            g_module_desc.voice_support = 1;
            mobile_write_buff_to_file(VOLTECS_SUPPORT_FILE, "1", strlen("1"));
        }
    }

    if(mobile_uci_get_int("mobile.@basic[0].imsEnable", &g_module_desc.ims) != 0) {
        g_module_desc.ims = 1;
    }

    return mobile_uci_get("mobile.@basic[0].voiceMode",  g_module_desc.voice_mode);
}

/**
 * 清理VOICE服务模块资源
 * 提供语音功能的资源清理，包括释放编解码器资源、关闭语音通道等
 *
 * @return 无返回值
 */
void mobile_deinit_voice_service(void) {
    if (!g_voice_module_initialized) {
        MOBILE_DEBUG("Voice service module not initialized, skipping deinitialization\n");
        return;
    }
    
    MOBILE_DEBUG("Deinitializing voice service module...\n");
    unlink(DEFAULT_MOBILE_VOLTE_FILE);
    system("killall -9 slic_switch");
    
    g_voice_module_initialized = 0;
    MOBILE_INFO("Voice service module deinitialized successfully\n");
}

/**
 * @brief 更新ims状态
 *
 */
void mobile_update_ims_status(void) {
    char result[256] = {0};
    char cmdline[128] = {0};

    memset(result, 0, 256);
    sprintf(cmdline, "at+qcfg=\"ims\"");
    mobile_at_cmd(cmdline, result, sizeof(result));
    if (result != NULL && strlen(result) > 0) {
        if (strcasestr(result, "+QCFG:\"ims\",0,1") != NULL) {
            g_module_desc.imsststus = 1;
        } else {
            g_module_desc.imsststus = 0;
        }
    }
    MOBILE_DEBUG("ims status %d\n", g_module_desc.imsststus);
}

/**
 * @brief 更新ims状态ext
 *
 */
void mobile_update_ims_status_ext(void) {
    char result[256] = {0};
    char reg_info[32] = {0};
    char ext_info[32] = {0};

    mobile_at_cmd("at+cireg?", result, sizeof(result));
    if (result != NULL && strlen(result) > 0) {
        if (strstr(result, "+CIREG:")) {
            sscanf(result, "%*[^,],%[^,],%[^,]", reg_info, ext_info);
        }
        MOBILE_DEBUG("reg_info=%s, ext_info=%s\n", reg_info, ext_info);

        if (strcasestr(ext_info, "5") != NULL) {
            mobile_write_buff_to_file(IMS_STATUS_FILE, "1", 1);
        } else {
            mobile_write_buff_to_file(IMS_STATUS_FILE, "0", 1);
        }
    }
}

/**
 * 检查语音功能是否支持
 *
 * @return 支持返回1，不支持返回0
 */
int mobile_voice_is_supported(void) {
    if (g_module_desc.voice_support == 0 && g_module_desc.regtype < REG_TYPE_LTE) {
        MOBILE_DEBUG("voice not support");
        return 0;
    }
    return 1;
}

/**
 * 配置语音网络参数
 */
void mobile_voice_configure_network(void) {
    char result[256] = {0};

    mobile_at_cmd(g_module_desc.at_config->at_cmni, result, sizeof(result));

    if (g_module_desc.ims == 1) {
        mobile_at_cmd(g_module_desc.at_config->at_dsci, result, sizeof(result));
    } else {
        mobile_at_cmd("AT+EIMSCFG=0,0,0,0,0,0", result, sizeof(result));
    }

    mobile_update_ims_status();
    mobile_update_ims_status_ext();
}

/**
 * 启动语音处理进程
 *
 */
void mobile_voice_launch_process(void) {
    char cmd[128] = {0};

    unlink(DEFAULT_MOBILE_VOLTE_FILE);
    sprintf(cmd, "touch %s", DEFAULT_MOBILE_VOLTE_FILE);
    system(cmd);
    sleep(1);

    system("killall -9 slic_switch");

    if (access("/bin/slic_switch", F_OK) != 0) {
        symlink("/usr/bin/sample_slic", "/bin/slic_switch");
    }

    system("/bin/slic_switch &");
}

/**
 * slic 语音
 *
 */
void mobile_network_slic(int reset) {
    static bool slic_sta = false;

    if (!mobile_voice_is_supported()) {
        slic_sta = false;
        return;
    }

    if (reset && slic_sta) {
        return;
    }

    if (!strcasecmp(g_module_desc.voice_mode, "VoIP")) {
        slic_sta = false;
        return;
    }

    mobile_voice_configure_network();

    if (!g_module_desc.imsststus) {
        MOBILE_DEBUG("ims status [%d], try again\n", g_module_desc.imsststus);
        slic_sta = false;
        return;
    }

    mobile_voice_launch_process();

    slic_sta = true;
}