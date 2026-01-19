#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>

#include "mobile_module_util.h"
#include "mobile_service_failover.h"
#include "mobile_service_scan.h"
#include "mobile_service_signal.h"
#include "mobile_service_modem.h"
#include "mobile_service_lock.h"

/*
 * 文件名称：mobile_service_signal.c
 * 功能描述：
 *     信号处理服务模块，提供程序信号处理和优雅退出功能
 *
 * 作者：gaoweiming
 */

int g_signal = 0;

// 内部状态变量
static int g_signal_module_initialized = 0;

/**
 * 初始化SIGNAL服务模块
 * 提供信号处理功能的初始化框架
 *
 * @return 成功返回0，失败返回错误码
 */
int mobile_init_signal_service(void) {
    if (g_signal_module_initialized) {
        MOBILE_INFO("signal service module already initialized\n");
        return 0;
    }
    
    // SIGNAL模块初始化逻辑
    mobile_setup_signal_handlers();
    
    g_signal_module_initialized = 1;
    MOBILE_INFO("signal service module initialized successfully\n");
    return 0;
}

/**
 * 清理SIGNAL服务模块资源
 * 提供信号处理功能的资源清理
 *
 * @return 无返回值
 */
void mobile_deinit_signal_service(void) {
    if (!g_signal_module_initialized) {
        return;
    }
    
    // SIGNAL模块清理逻辑
    // 这里可以添加信号处理功能的资源清理代码
    
    g_signal_module_initialized = 0;
    MOBILE_INFO("signal service module deinitialized\n");
}

// 信号处理函数
/**
 * 退出信号处理函数
 * 处理程序退出信号（SIGTERM、SIGINT），设置全局信号标志
 *
 * @param sig 接收到的信号编号
 * @return 无返回值
 */
static void exit_handler(int sig) {
    MOBILE_INFO("Received exit signal %d (%s), setting exit flag...\n",
                sig, (sig == SIGTERM) ? "SIGTERM" : "SIGINT");
    g_signal = 1;
    mobile_set_ping_check_thread_running(0);
    mobile_set_scan_thread_running(0);
    mobile_set_info_thread_running(0);
    // 记录详细的退出信息
    MOBILE_DEBUG("Global signal flag set to %d, program will exit gracefully\n", g_signal);
}

/**
 * SIGUSR1信号处理函数
 * 处理用户自定义信号1，用于程序调试和状态查询
 * killall -SIGUSR1 mobile-mngr
 * @param sig 接收到的信号编号
 * @return 无返回值
 */
static void usr1_handler(int sig) {
    // 打印模块描述信息
    mobile_print_module_desc_info();

    // 打印多APN配置信息
    mobile_print_apn_info();

    // 打印锁配置信息
    mobile_print_lock_config_info();

    // 打印小区列表信息
    mobile_print_cell_entry_list();
    
    // 打印小区扫描频段列表信息
    mobile_print_cell_band_entry_list();

#ifdef CUS_PARAMS_AREA_PRODUCT_STC
    mobile_print_pin_stc_info();
#endif
}

/**
 * SIGUSR2信号处理函数
 * 处理用户自定义信号2，强制退出
 * killall -SIGUSR2 mobile-mngr
 * @param sig 接收到的信号编号
 * @return 无返回值
 */
static void usr2_handler(int sig) {
    MOBILE_INFO("Received SIGUSR2 signal %d, processing user-defined operation 2\n", sig);
    exit(0);
    MOBILE_DEBUG("SIGUSR2 signal processing completed\n");
}

/**
 * 设置信号处理器
 * 注册各种信号的处理函数，确保程序能够优雅处理各种信号
 *
 * @return 成功返回0，失败返回错误码
 */
int mobile_setup_signal_handlers(void) {
    int ret = 0;
    
    // 忽略SIGPIPE信号（网络连接断开时产生）
    if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
        MOBILE_ERROR("Failed to set SIGPIPE handler\n");
        ret = -1;
    }
    
    // 设置退出信号处理器
    if (signal(SIGTERM, exit_handler) == SIG_ERR) {
        MOBILE_ERROR("Failed to set SIGTERM handler\n");
        ret = -1;
    }
    
    if (signal(SIGINT, exit_handler) == SIG_ERR) {
        MOBILE_ERROR("Failed to set SIGINT handler\n");
        ret = -1;
    }
    
    // 忽略SIGHUP信号（终端断开连接）
    if (signal(SIGHUP, SIG_IGN) == SIG_ERR) {
        MOBILE_WARN("Failed to set SIGHUP handler\n");
    }
    
    // 忽略SIGALRM信号（定时器信号）
    if (signal(SIGALRM, SIG_IGN) == SIG_ERR) {
        MOBILE_WARN("Failed to set SIGALRM handler\n");
    }
    
    // 设置用户自定义信号处理器
    if (signal(SIGUSR1, usr1_handler) == SIG_ERR) {
        MOBILE_ERROR("Failed to set SIGUSR1 handler\n");
        ret = -1;
    }
    
    if (signal(SIGUSR2, usr2_handler) == SIG_ERR) {
        MOBILE_ERROR("Failed to set SIGUSR2 handler\n");
        ret = -1;
    }
    
    if (ret == 0) {
        MOBILE_INFO("All signal handlers registered successfully\n");
    } else {
        MOBILE_ERROR("Some signal handlers failed to register\n");
    }
    
    return ret;
}

/**
 * 获取全局运行信号
 *
 * @return 返回全局信号值
 */
int mobile_get_global_signal(void) {
    return g_signal;
}
