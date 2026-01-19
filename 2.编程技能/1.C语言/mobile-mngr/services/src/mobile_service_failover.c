#include <stdlib.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <net/if.h>
#include <net/route.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "mobile_module_util.h"
#include "mobile_module_modem.h"
#include "mobile_service_state_machine.h"
#include "mobile_service_failover.h"

/*
 * 文件名称：mobile_service_failover.c
 * 功能描述：
 *     FAILOVER故障切换服务模块，提供故障切换功能管理
 *     业务逻辑层，直接实现故障切换功能，不需要module层
 *     移植自dongle-mngr的pingcheck线程，提供ICMP网络检测和故障切换功能
 *
 * 作者：gaoweiming
 */

failover_st g_failover;
static int g_failover_module_initialized = 0;
static ping_check_thread_t g_ping_check_thread = {
    .thread_id = 0,
    .running = 0,
    .running_lock = PTHREAD_MUTEX_INITIALIZER
};

/**
 * 初始化FAILOVER服务模块
 * 提供故障切换功能的初始化框架
 *
 * @return 成功返回0，失败返回错误码
 */
int mobile_init_failover_service(void) {
    if (g_failover_module_initialized) {
        MOBILE_INFO("failover service module already initialized\n");
        return 0;
    }
    
    mobile_init_failover_config();
    
    // 启动ping检查线程
    if (mobile_init_ping_check_task() != 0) {
        MOBILE_ERROR("Failed to initialize ping check task\n");
        return -1;
    }
    
    g_failover_module_initialized = 1;
    MOBILE_INFO("failover service module initialized successfully\n");
    return 0;
}

/**
 * 清理FAILOVER服务模块资源
 * 提供故障切换功能的资源清理
 *
 * @return 无返回值
 */
void mobile_deinit_failover_service(void) {
    if (!g_failover_module_initialized) {
        return;
    }
    
    // 停止ping检查线程
    mobile_deinit_ping_check_task();
    
    memset(&g_failover, '\0', sizeof(g_failover));
    
    g_failover_module_initialized = 0;
    MOBILE_INFO("failover service module deinitialized\n");
}

/**
 * 初始化FAILOVER服务配置
 * 从UCI配置文件中读取故障切换配置参数并初始化全局配置结构体
 * 包括故障切换使能、ICMP检查、重试次数、周期时间等配置项
 *
 * @return 成功返回0，失败返回错误码
 */
int mobile_init_failover_config(void) {
    int ret = 0;
    char curr_default_ifname[32] = {0};

    // 检查全局故障切换配置结构体指针
    if (&g_failover == NULL) {
        MOBILE_ERROR("Global failover configuration structure is NULL\n");
        return -1;
    }

    ret = mobile_uci_get_int("mobile.failover.enable", &g_failover.enable);
    if (g_failover.enable) {
        ret |= mobile_uci_get_int("mobile.failover.ICMPCheckEnable", &g_failover.icmp_check_enable);
        ret |= mobile_uci_get_int("mobile.failover.retryTimes", &g_failover.retry_times);
        ret |= mobile_uci_get_int("mobile.failover.periodTime", &g_failover.period_time);
        ret |= mobile_uci_get("mobile.failover.IPAddress1", g_failover.ip_address1);
        ret |= mobile_uci_get("mobile.failover.IPAddress2", g_failover.ip_address2);
    }

    mobile_get_default_gateway_if_name_in_route_table(curr_default_ifname);
    if ((strlen(curr_default_ifname) > 0) && strstr(curr_default_ifname, CCMNI_NET_IFNAME_PRE) == NULL) {
        strncpy(g_failover.eth_wan_gw_if_name, curr_default_ifname, sizeof(curr_default_ifname) - 1);
    } else {
        strncpy(g_failover.eth_wan_gw_if_name, "none", strlen("none"));
    }
    
    if (ret != 0) {
        MOBILE_WARN("Some UCI configuration parameters failed to load for failover\n");
    }

    MOBILE_INFO("Failover configuration loaded: enable=%d, ICMP=%d, retry=%d, period=%d, IP1=%s, IP2=%s\n",
                g_failover.enable, g_failover.icmp_check_enable,
                g_failover.retry_times, g_failover.period_time,
                g_failover.ip_address1, g_failover.ip_address2);
    return 0;
}

/**
 * 设置ping检查线程运行状态（带锁）
 *
 * @param running 运行状态：1-运行，0-停止
 * @return 无返回值
 */
void mobile_set_ping_check_thread_running(int running) {
    pthread_mutex_lock(&g_ping_check_thread.running_lock);
    g_ping_check_thread.running = running;
    pthread_mutex_unlock(&g_ping_check_thread.running_lock);
}

/**
 * 获取ping检查线程运行状态（带锁）
 *
 * @return 运行状态：1-运行，0-停止
 */
int mobile_get_ping_check_thread_running(void) {
    int running;
    pthread_mutex_lock(&g_ping_check_thread.running_lock);
    running = g_ping_check_thread.running;
    pthread_mutex_unlock(&g_ping_check_thread.running_lock);
    return running;
}

/**
 * 创建ping检查线程
 * 启动ping检查的后台线程，用于异步执行网络连通性检测和故障切换
 *
 * @return 无返回值
 */
void mobile_pthread_ping_check(void) {
    if (mobile_get_ping_check_thread_running()) {
        MOBILE_WARN("Failover is already running\n");
        return;
    }
    
    mobile_set_ping_check_thread_running(1);
    pthread_create(&g_ping_check_thread.thread_id, NULL, (void*)mobile_ping_check_loop, NULL);
}

/**
 * 初始化ping检查任务
 *
 * @return 成功返回0，失败返回错误码
 */
int mobile_init_ping_check_task(void) {
    mobile_pthread_ping_check();
    return 0;
}

/**
 * 清理ping检查任务
 */
void mobile_deinit_ping_check_task(void) {
    mobile_set_ping_check_thread_running(0);
    
    pthread_cancel(g_ping_check_thread.thread_id);
    // 等待线程完全退出
    pthread_join(g_ping_check_thread.thread_id, NULL);
    MOBILE_INFO("<<<<<********************** Failover thread stopped **********************>>>>>\n");
}

/**
 * 执行ping测试
 *
 * @param ip_addr 目标IP地址
 * @param if_name 网络接口名称
 * @return ping成功返回1，失败返回0
 */
int mobile_ping_test(char* ip_addr, char* if_name) {
    char* p = NULL;
    char str[256] = {0};
    FILE* fs = NULL;
    char cmd_str[256] = {0};
    int received_number = 0;
    int wait_times = 3;
    int ping_count = 1;
    char curr_sip_address[64] = {0};

    if (ip_addr == NULL) {
        return 0;
    }

    mobile_get_if_ipv4_addr(if_name, curr_sip_address);

    // wait_times范围 [1 ~ 3]
    if (wait_times > g_failover.period_time) {
        wait_times = g_failover.period_time - 1;
        if (wait_times == 0)
            wait_times = 1;
    }

    // ping -c 1 -W 1 -I ppp0.1 8.8.8.8
    sprintf(cmd_str, "%s -c %d -W %d -I %s  %s > %s", "ping", ping_count, wait_times,
            ((curr_sip_address[0] == 0) ? if_name : curr_sip_address), ip_addr, PING_TEMP_FILE);
    MOBILE_DEBUG("ping cmd: %s\n", cmd_str);
    system(cmd_str);

    fs = fopen(PING_TEMP_FILE, "r");
    if (fs != NULL) {
        while (fgets(str, 256, fs) > 0) {
            p = strstr(str, "transmitted, ");
            if (p != NULL) {
                p += strlen("transmitted, ");
                received_number = atoi(p);
                break;
            }
        }
        fclose(fs);
    }

    unlink(PING_TEMP_FILE);

    if (received_number == 0) {
        return 0;
    } else {
        return 1;
    }
}


/**
 * 检查以太网网络是否连通
 *
 * @param active_gw_if_name 活动网关接口名称
 * @return 连通返回1，不连通返回0
 */
unsigned char mobile_check_eth_net_is_link(char* active_gw_if_name) {
    int i = 0;
    unsigned char link_flag = 0;
    char ip_addr_list[MAX_PING_ADDRESS_NUM][16] = {0};

    strcpy(ip_addr_list[0], g_failover.ip_address1);
    strcpy(ip_addr_list[1], g_failover.ip_address2);

    for (i = 0; i < MAX_PING_ADDRESS_NUM; i++) {
        if (ip_addr_list[i][0] == 0) {
            continue;
        }

        if (mobile_is_valid_ipv4_address(ip_addr_list[i]) == 1) {
            if (mobile_ping_test(ip_addr_list[i], active_gw_if_name) == 1) {
                link_flag = 1;
                break;
            }
        }
    }

    return link_flag;
}

/**
 * 检查以太网网络是否断开
 *
 * @param active_gw_if_name 活动网关接口名称
 * @return 断开返回1，连通返回0
 */
unsigned char mobile_check_eth_net_is_no_link(char* active_gw_if_name) {
    int i = 0;
    unsigned char no_link_flag = 0;
    char ip_addr_list[MAX_PING_ADDRESS_NUM][16] = {0};
    int i_count = 0;

    strcpy(ip_addr_list[0], g_failover.ip_address1);
    strcpy(ip_addr_list[1], g_failover.ip_address2);

    for (i = 0; i < MAX_PING_ADDRESS_NUM; i++) {
        if (ip_addr_list[i][0] == 0) {
            continue;
        }

        if (mobile_is_valid_ipv4_address(ip_addr_list[i]) == 1) {
            if (mobile_ping_test(ip_addr_list[i], active_gw_if_name) == 0) {
                i_count += 1;
                break;
            }
        }
    }

    if (i_count == MAX_PING_ADDRESS_NUM) {
        no_link_flag = 1;
    }

    return no_link_flag;
}

/**
 * 为ping地址添加路由规则
 */
void mobile_add_route_for_ping_addresses(void) {
    int i = 0;
    char ip_addr_list[MAX_PING_ADDRESS_NUM][16] = {0};
    char gateway_ip_addr[64] = {0};
    char buf_line[512] = {0};

    // 使用移植的函数获取默认网关
    mobile_get_ifname_default_gw(g_failover.eth_wan_gw_if_name, gateway_ip_addr, sizeof(gateway_ip_addr) - 1);

    strcpy(ip_addr_list[0], g_failover.ip_address1);
    strcpy(ip_addr_list[1], g_failover.ip_address2);

    // 删除旧的路由
    if (access(PING_RECORD_LASTROUTE, F_OK) == 0) {
        mobile_read_first_line_from_file(PING_RECORD_LASTROUTE, buf_line, sizeof(buf_line) - 1);
        if (buf_line[0]) {
            system(buf_line);
            MOBILE_DEBUG("delete rules: %s\n", buf_line);
            buf_line[0] = 0;
        }
        unlink(PING_RECORD_LASTROUTE);
    }

    // 添加新的路由
    for (i = 0; i < MAX_PING_ADDRESS_NUM; i++) {
        if (ip_addr_list[i][0] == 0) {
            continue;
        }

        if (mobile_is_valid_ipv4_address(ip_addr_list[i]) == 1 && gateway_ip_addr[0] && strcmp(gateway_ip_addr, "0.0.0.0")) {
            char buf_cmd[256] = {0};
            snprintf(buf_cmd, sizeof(buf_cmd), "ip ro del %s", ip_addr_list[i]);
            // system(buf_cmd);
            strcat(buf_line, buf_cmd);
            strcat(buf_line, "; ");
            snprintf(buf_cmd, sizeof(buf_cmd), "ip ro add %s via %s dev %s", ip_addr_list[i], gateway_ip_addr,
                     g_failover.eth_wan_gw_if_name);
            MOBILE_DEBUG("add rule: %s\n", buf_cmd);
            system(buf_cmd);
        }
    }
    if (buf_line[0]) {
        mobile_write_buff_to_file(PING_RECORD_LASTROUTE, buf_line, strlen(buf_line));
        MOBILE_DEBUG("record rules: %s\n", buf_line);
        buf_line[0] = 0;
    }

    return;
}

/**
 * 获取接口的网关IP地址
 *
 * @param wan_interface WAN接口名称
 * @param if_name 输出参数，存储接口名称
 * @param gw 输出参数，存储网关IP地址
 * @return 成功返回1，失败返回0
 */
int mobile_get_ifname_gw_ip(char* wan_interface, char* if_name, char* gw) {
    char cmd[256] = {0};
    char tmp_str[256] = {0};
    FILE* fp = NULL;
    char* pstart = NULL;
    int is_found = 0;

    sprintf(cmd, "ifstatus %s |grep -E \"l3_device|nexthop\"", wan_interface);
    fp = popen(cmd, "r");
    if (fp == NULL) {
        return 0;
    }

    while (fgets(tmp_str, 256, fp) != NULL) {
        if ((pstart = strstr(tmp_str, "\"nexthop\": \"")) != NULL) {
            if ((tmp_str[strlen(tmp_str) - 1] == '\n')) {
                tmp_str[strlen(tmp_str) - 1] = '\0';
            }

            strcpy(gw, pstart + strlen("\"nexthop\": \""));
            if (strstr(gw, ":")) {
                continue;
            }
            gw[strlen(gw) - 2] = '\0';
            is_found = 1;
        } else if ((pstart = strstr(tmp_str, "\"l3_device\": \"")) != NULL) {
            if ((tmp_str[strlen(tmp_str) - 1] == '\n')) {
                tmp_str[strlen(tmp_str) - 1] = '\0';
            }

            strcpy(if_name, pstart + strlen("\"l3_device\": \""));
            if_name[strlen(if_name) - 2] = '\0';
            // is_found = 1;
        }
    }

    pclose(fp);

    return is_found;
}

/**
 * 通过ping结果切换默认路由
 *
 * @param def_route 当前默认路由接口
 * @param to_route 要切换到的路由接口
 */
void mobile_switch_default_route_by_ping(char* def_route, char* to_route) {
    char gateway_ip_addr[64] = {0};
    char cur_gateway_ip_addr[64] = {0};
    char buf_line[512] = {0};

    // 使用移植的函数获取默认网关
    mobile_get_ifname_default_gw(def_route, gateway_ip_addr, sizeof(gateway_ip_addr) - 1);
    mobile_get_ifname_default_gw(to_route, cur_gateway_ip_addr, sizeof(gateway_ip_addr) - 1);

    if (strstr(to_route, "ccmni")) {
        mobile_get_ifname_gw_ip("wan5g", to_route, cur_gateway_ip_addr);
        MOBILE_DEBUG("ifname: %s, gateway ip: %s\n", to_route, cur_gateway_ip_addr);
    }

    if ((gateway_ip_addr[0] && strcmp(gateway_ip_addr, "0.0.0.0")) && cur_gateway_ip_addr[0] &&
        strcmp(cur_gateway_ip_addr, "0.0.0.0")) {
        snprintf(buf_line, sizeof(buf_line), "route del default gw 0.0.0.0 dev %s", def_route);
        system(buf_line);
        snprintf(buf_line, sizeof(buf_line), "route add default gw %s dev %s", cur_gateway_ip_addr, to_route);
        MOBILE_DEBUG("add rule: %s\n", buf_line);
        system(buf_line);
    }
}

/**
 * 检查是否需要进行故障切换检测
 * 验证故障切换配置是否启用且配置有效
 *
 * @return 需要检测返回1，不需要返回0
 */
static int mobile_should_perform_failover_check(void) {
    return (g_failover.enable &&
            g_failover.icmp_check_enable &&
            (g_failover.ip_address1[0] != 0 || g_failover.ip_address2[0] != 0));
}

/**
 * 获取当前网络状态信息
 * 包括活动网关接口、以太网连通性和移动网络设备名称
 *
 * @param curr_active_gw_if_name 输出参数，存储当前活动网关接口名称
 * @param eth_wan_net_up 输出参数，存储以太网连通状态
 * @param mobile_net_device_name 输出参数，存储移动网络设备名称
 * @return 成功获取信息返回1，失败返回0
 */
static int mobile_get_network_status_info(char* curr_active_gw_if_name,
                                         unsigned char* eth_wan_net_up,
                                         const char** mobile_net_device_name) {
    // 获取当前活动网关接口
    mobile_get_default_gateway_if_name_in_route_table(curr_active_gw_if_name);
    
    // 如果没有当前活动网关接口，跳过检查
    if (curr_active_gw_if_name[0] == '\0') {
        MOBILE_DEBUG("curr_active_gw_if_name is empty, skip this\n");
        return 0;
    }
    
    // 获取以太网连通状态
    *eth_wan_net_up = mobile_check_eth_net_is_link(g_failover.eth_wan_gw_if_name);
    
    // 获取移动网络设备名称
    *mobile_net_device_name = mobile_get_net_device_name();
    
    MOBILE_DEBUG("curr_active_gw_if_name:%s eth_wan_gw_if_name:%s mobile_net_device_name:%s eth_wan_net_up:%d\n",
                curr_active_gw_if_name, g_failover.eth_wan_gw_if_name, *mobile_net_device_name, *eth_wan_net_up);
    
    return 1;
}

/**
 * 更新故障切换统计信息
 * 根据以太网连通状态更新成功和失败次数
 *
 * @param eth_wan_net_up 以太网连通状态
 * @param failed_times 失败次数指针
 * @param succ_times 成功次数指针
 */
static void mobile_update_failover_statistics(unsigned char eth_wan_net_up,
                                             int* failed_times,
                                             int* succ_times) {
    if (!eth_wan_net_up) {
        (*failed_times)++;
        *succ_times = 0;
    } else {
        *failed_times = 0;
        (*succ_times)++;
    }
}

/**
 * 执行以太网WAN到移动WAN的故障切换
 * 当以太网WAN是活动网关但网络不通时触发切换
 *
 * @param curr_active_gw_if_name 当前活动网关接口名称
 * @param mobile_net_device_name 移动网络设备名称
 * @param failed_times 失败次数指针
 */
static void mobile_perform_eth_to_mobile_switch(const char* curr_active_gw_if_name,
                                               const char* mobile_net_device_name,
                                               int* failed_times) {
    MOBILE_INFO("ethWan(%s) is activeDefaultGW but ethWan network is blocked, change activeDefaultGW to mobileWan(%s)\n",
                curr_active_gw_if_name, mobile_net_device_name);

    mobile_write_buff_to_file(DEFAULT_ETHWAN_PING_FAILOVER, curr_active_gw_if_name,
                              strlen(curr_active_gw_if_name));

    char cmd[128] = {0};
    sprintf(cmd, "ifswitch route %s %s", g_failover.eth_wan_gw_if_name, mobile_net_device_name);
    mobile_switch_default_route_by_ping(g_failover.eth_wan_gw_if_name, mobile_net_device_name);
    MOBILE_INFO("switch cmd: %s\n", cmd);
    *failed_times = 0;
}

/**
 * 执行移动WAN到以太网WAN的故障切换
 * 当移动WAN是活动网关但以太网网络恢复时触发切换
 *
 * @param curr_active_gw_if_name 当前活动网关接口名称
 * @param mobile_net_device_name 移动网络设备名称
 * @param succ_times 成功次数指针
 */
static void mobile_perform_mobile_to_eth_switch(const char* curr_active_gw_if_name,
                                               const char* mobile_net_device_name,
                                               int* succ_times) {
    MOBILE_INFO("mobileWan(%s) is activeDefaultGW but ethWan(%s) network is passed, change activeDefaultGW to ethWan\n",
                curr_active_gw_if_name, g_failover.eth_wan_gw_if_name);

    mobile_write_buff_to_file(DEFAULT_ETHWAN_PING_FAILOVER, curr_active_gw_if_name,
                              strlen(curr_active_gw_if_name));
    
    char cmd[128] = {0};
    sprintf(cmd, "ifswitch route %s %s", mobile_net_device_name, g_failover.eth_wan_gw_if_name);
    mobile_switch_default_route_by_ping(mobile_net_device_name, g_failover.eth_wan_gw_if_name);
    MOBILE_INFO("switch cmd: %s\n", cmd);
    *succ_times = 0;
}

/**
 * 执行故障切换检测逻辑
 * 根据当前网络状态决定是否进行故障切换
 *
 * @param sleep_time 睡眠时间指针
 * @param failed_times 失败次数指针
 * @param succ_times 成功次数指针
 */
static void mobile_perform_failover_detection(int* sleep_time, int* failed_times, int* succ_times) {
    char curr_active_gw_if_name[32] = {0};
    unsigned char eth_wan_net_up = 0;
    const char* mobile_net_device_name = NULL;
    
    // 获取网络状态信息
    if (!mobile_get_network_status_info(curr_active_gw_if_name, &eth_wan_net_up, &mobile_net_device_name)) {
        *sleep_time = 0;
        return;
    }
    
    // 更新故障切换统计信息
    mobile_update_failover_statistics(eth_wan_net_up, failed_times, succ_times);
    
    MOBILE_DEBUG("failover statistics: failed_times=%d, succ_times=%d\n", *failed_times, *succ_times);
    
    // 检查当前活动网关类型并执行相应切换
    if (strcmp(g_failover.eth_wan_gw_if_name, curr_active_gw_if_name) == 0) {
        // 以太网WAN是活动网关
        if (*failed_times >= g_failover.retry_times) {
            mobile_perform_eth_to_mobile_switch(curr_active_gw_if_name, mobile_net_device_name, failed_times);
        }
    } else if (strcmp(mobile_net_device_name, curr_active_gw_if_name) == 0) {
        // 移动WAN是活动网关
        if (*succ_times >= g_failover.retry_times) {
            mobile_perform_mobile_to_eth_switch(curr_active_gw_if_name, mobile_net_device_name, succ_times);
        }
    } else {
        MOBILE_DEBUG("curr_active_gw_if_name(%s) is out of expect values(%s %s), skip this\n",
                    curr_active_gw_if_name, g_failover.eth_wan_gw_if_name, mobile_net_device_name);
    }
    
    // 重置计时器
    *sleep_time = 0;
}

/**
 * ping检查主循环线程函数
 * 监控网络连通性并执行故障切换操作
 * 处理以太网WAN和移动WAN之间的故障切换
 * 在移动WAN连接时启动检查
 *
 * @param context 线程上下文（未使用）
 * @return 线程返回值，总是返回0
 */
unsigned int mobile_ping_check_loop(void) {
    int sleep_time = 0;
    int failed_times = 0;
    int succ_times = 0;
    char curr_active_gw_if_name[32] = {0};
    int b_update_rule = 0;

    unlink(DEFAULT_ETHWAN_PING_FAILOVER);
    MOBILE_INFO("<<<<<********************** Failover thread starting **********************>>>>>\n");
    while(mobile_get_ping_check_thread_running()) {
        pthread_testcancel();

        sleep(2);

        // 更新配置（如果需要）
        if ((access(DEFAULT_MOBILEFAILOVER_CONFIG_UPDATE, F_OK)) == 0) {
            MOBILE_DEBUG("failover check config update\n");
            mobile_init_failover_config();

            unlink(DEFAULT_MOBILEFAILOVER_CONFIG_UPDATE);
            sleep_time = 0;
            failed_times = 0;
            b_update_rule = 1;
        }

        // 检查是否需要进行故障切换检测
        if (!mobile_should_perform_failover_check()) {
            continue;
        }

        // 更新以太网WAN网关接口（如果需要）
        if ((access(DEFAULT_ETHWAN_GATEWAY_IF_UPDATE, F_OK)) == 0) {
            char buf_line[32] = {0};
            mobile_read_first_line_from_file(DEFAULT_ETHWAN_GATEWAY_IF, buf_line, sizeof(buf_line) - 1);
            MOBILE_DEBUG("failover update ethwan gateway(%s -> %s)\n", g_failover.eth_wan_gw_if_name, buf_line);
            strncpy(g_failover.eth_wan_gw_if_name, buf_line, sizeof(g_failover.eth_wan_gw_if_name) - 1);

            unlink(DEFAULT_ETHWAN_GATEWAY_IF_UPDATE);
            sleep_time = 0;
            failed_times = 0;
            b_update_rule = 1;
        }

        // 如果状态机正在运行，跳过ping检查
        if (mobile_get_state_machine_running_status()) {
            MOBILE_DEBUG("State machine is running, skip ping check\n");
            continue;
        }

        // 如果移动WAN未连接，跳过检查
        const char* wan_status = mobile_get_wan_status();
        if (!(wan_status && !strcmp(wan_status, "CONNECTED"))) {
            MOBILE_DEBUG("Mobile WAN is not connected, skip ping check\n");
            sleep_time = 0;
            failed_times = 0;
            continue;
        }

        // 必须添加（或更新）路由
        if (b_update_rule) {
            mobile_add_route_for_ping_addresses();
            b_update_rule = 0;
        }

        // 如果没有以太网WAN网关接口，跳过检查
        if (g_failover.eth_wan_gw_if_name[0] == 0) {
            MOBILE_DEBUG("eth_wan_gw_if_name is empty, skip ping check\n");
            sleep_time = 0;
            failed_times = 0;
            continue;
        }

        sleep_time++;

        // 检查是否达到检测周期
        if (sleep_time != g_failover.period_time) {
            continue;
        }

        // 执行故障切换检测逻辑
        mobile_perform_failover_detection(&sleep_time, &failed_times, &succ_times);

        pthread_testcancel();
    }
    
    mobile_set_ping_check_thread_running(0);
    return 0;
}
