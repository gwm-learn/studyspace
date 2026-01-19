#include "mobile_module_log.h"
#include "mobile_service_log.h"

/*
 * 文件名称：mobile_service_log.c
 * 功能描述：
 *     log服务模块，提供log设置功能
 *
 * 作者：gaoweiming
 */

// 内部状态变量
static int g_log_module_initialized = 0;

/**
 * 初始化LOG服务模块
 * 提供日志配置的初始化框架
 *
 * @return 成功返回0，失败返回错误码
 */
int mobile_init_log_service(void) {
    if (g_log_module_initialized) {
        MOBILE_INFO("log service module already initialized\n");
        return 0;
    }
    
    // 调用日志配置初始化
    mobile_init_log_config();
    
    g_log_module_initialized = 1;
    MOBILE_INFO("log service module initialized successfully\n");
    return 0;
}

/**
 * 清理LOG服务模块资源
 * 提供日志配置的资源清理
 *
 * @return 无返回值
 */
void mobile_deinit_log_service(void) {
    if (!g_log_module_initialized) {
        return;
    }
    
    // LOG模块清理逻辑
    // 这里可以添加日志资源的清理代码（例如关闭日志文件）
    // 目前日志模块没有提供关闭函数，所以暂时为空
    
    g_log_module_initialized = 0;
    MOBILE_INFO("log service module deinitialized\n");
}

/**
 * 初始化 log 配置
 *
 */
void mobile_init_log_config(void) {
    int log_level = 0;
    char log_file[256] = {0};
    int log_size = 0;
    
    // 从mobile配置文件中读取日志配置
    if (mobile_uci_get_option_int("mobile", "@basic[0]", "log_level", &log_level) != 0) {
        // 如果读取失败，使用默认值
        log_level = 4; // INFO级别
    }
    
    if (mobile_uci_get_option("mobile", "@basic[0]", "log_file", log_file) != 0) {
        // 如果读取失败，使用默认值
        strcpy(log_file, "/tmp/mobile/log");
    }
    
    if (mobile_uci_get_option_int("mobile", "@basic[0]", "log_size", &log_size) != 0) {
        // 如果读取失败，使用默认值
        log_size = 10; // 10MB
    }
    
    // 调用日志初始化函数
    if (mobile_log_init((mobile_log_level_t)log_level, log_file, (unsigned int)log_size) == 0) {
        MOBILE_INFO("Log configuration initialized successfully: level=%d, file=%s, size=%dMB\n",
                   log_level, log_file, log_size);
    } else {
        MOBILE_ERROR("Failed to initialize log configuration: level=%d, file=%s, size=%dMB\n",
                    log_level, log_file, log_size);
    }
}