#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include "mobile_module_util.h"
#include "mobile_service_modem.h"

#define RE_BUF_LEN 256
/*
 * 文件名称：mobile_service_modem.c
 * 功能描述：
 *     modem服务模块，提供设备modem管理功能
 *
 * 作者：gaoweiming
 */

// 内部状态变量
static int g_modem_module_initialized = 0;
module_desc_t g_module_desc;

/**
 * 初始化modem服务模块
 * 提供设备信息管理功能的初始化框架
 *
 * @return 成功返回0，失败返回错误码
 */
int mobile_init_modem_service(void) {
    if (g_modem_module_initialized) {
        MOBILE_INFO("info service module already initialized\n");
        return 0;
    }
    
    if (mobile_init_mobile_dir() != 0) {
        MOBILE_ERROR("init mobile dir fail\n");
        return -1;
    }

    if (mobile_record_user_pid(getpid(), MOBILEMNGR_PID_FILE) != 1) {
        MOBILE_ERROR("record pid file fail\n");
        return -1;
    }

    if (mobile_get_pid_vid() != 1) {
        MOBILE_ERROR("get pid vid fail\n");
        return -1;
    }

    if (mobile_init_default_at_command_table() != 0) {
        MOBILE_ERROR("init predefined at command fail\n");
        return -1;
    }

    if (mobile_init_sim_type() != 0) {
        MOBILE_ERROR("init esim fail\n");
        return -1;
    }

    mobile_write_buff_to_file(DIALD_STATUS_FILE, DIALD_NONE, strlen(DIALD_NONE));
    g_modem_module_initialized = 1;
    MOBILE_INFO("info service module initialized successfully\n");
    return 0;
}

/**
 * 清理INFO服务模块资源
 * 提供设备信息查询和管理功能的资源清理
 *
 * @return 无返回值
 */
void mobile_deinit_modem_service(void) {
    if (!g_modem_module_initialized) {
        return;
    }
    
    unlink(MOBILEMNGR_PID_FILE);
    
    g_modem_module_initialized = 0;
    MOBILE_INFO("info service module deinitialized\n");
}

/**
 * apply nettype 处理函数
 *
 * @return true - 成功, false - 失败
 */
bool mobile_apply_nettype_handler(void) {
    if (g_module_desc.pid == 0x7006 && g_module_desc.vid == 0x2c7c) {
        return mobile_apply_nettype_p7006_v2c7c();
    } else {
        return true;
    }
}

/**
 * @brief 应用网络类型配置 (Quectel模块专用)
 *
 * 根据最终网络类型配置模块的网络模式，支持2G/3G/4G/5G网络模式切换。
 * 主要功能：
 * 1. 更新最终网络类型
 * 2. 检查NSA模式状态
 * 3. 根据网络类型发送相应的AT命令配置
 * 4. 管理NSA模式开关
 * 5. 应用小区锁定类型
 *
 * 网络类型与AT命令对应关系：
 * - '1' (4G only): at+erat=3
 * - '2' (3G only): at+erat=1
 * - '3' (2G only): at+erat=21 (自动模式)
 * - '4' (5G NSA): at+e5gopt=5 + at+erat=19
 * - '5' (5G SA): at+e5gopt=3 + at+erat=15
 * - '6' (5G NSA+SA): at+e5gopt=7 + at+erat=19
 * - 其他: at+e5gopt=7 + at+erat=19/21 (根据产品配置)
 *
 * @return 成功返回0，失败返回错误码
 */
int mobile_apply_nettype_p7006_v2c7c(void) {
    int ret = 0;
    int nsa_flag = 0;
    int nsa_type = 0;
    char cmdline[128] = {0};
    char result[RE_BUF_LEN] = {0};

    // 步骤1: 更新最终网络类型
    mobile_update_final_nettype();

    MOBILE_DEBUG("final_nettype=[%c]\n", g_module_desc.final_nettype);
    
    // 步骤2: 检查NSA模式状态
    snprintf(cmdline,sizeof(cmdline),"AT+ESBP=7,\"SBP_N1_MODE_NOT_ALLOWED_PERMANENT_DISABLING\"");
    mobile_at_cmd(cmdline, result, RE_BUF_LEN-1);
    if(strstr(result,"+ESBP: 1")) {
        nsa_flag = 1;
    }

    // 步骤3: 根据网络类型配置相应的AT命令
    if (g_module_desc.final_nettype == '1') {//4g only
        snprintf(cmdline, sizeof(cmdline), "at+erat=3");
    } else if (g_module_desc.final_nettype == '2') {//3g only
        snprintf(cmdline, sizeof(cmdline), "at+erat=1");
    } else if (g_module_desc.final_nettype == '3') {//2g only
        snprintf(cmdline, sizeof(cmdline), "at+erat=21");    //auto
    } else if (g_module_desc.final_nettype == '4') {//4+5g only (nsa)
        //NSA模式配置
        snprintf(cmdline, sizeof(cmdline), "at+e5gopt=5");
        mobile_at_cmd(cmdline, result, RE_BUF_LEN - 1);

        snprintf(cmdline, sizeof(cmdline), "at+erat=19");    //21->19
    	nsa_type = 1;
    } else if (g_module_desc.final_nettype == '5') { //5g only (sa) at+erat=15 at+e5gopt=7->3
        //SA模式配置
        snprintf(cmdline, sizeof(cmdline), "at+e5gopt=3");
        mobile_at_cmd(cmdline, result, RE_BUF_LEN - 1);

        snprintf(cmdline, sizeof(cmdline), "at+erat=15");
    } else if (g_module_desc.final_nettype == '6') {//4+5g only (nsa+sa)
        //NSA+SA混合模式配置
        snprintf(cmdline, sizeof(cmdline), "at+e5gopt=7");
        mobile_at_cmd(cmdline, result, RE_BUF_LEN - 1);

        snprintf(cmdline, sizeof(cmdline), "at+erat=19");
    } else {
        // 默认配置
        snprintf(cmdline, sizeof(cmdline), "at+e5gopt=7");
        mobile_at_cmd(cmdline, result, RE_BUF_LEN - 1);
        snprintf(cmdline, sizeof(cmdline), "at+erat=21");
    }

    // 执行主配置命令
    mobile_at_cmd(cmdline, result, RE_BUF_LEN - 1);

    // 步骤4: 管理NSA模式开关
    if((nsa_flag == 1) && (nsa_type != 1)) {  //close NSA mode
        snprintf(cmdline,sizeof(cmdline), "AT+ESBP=5,\"SBP_N1_MODE_NOT_ALLOWED_PERMANENT_DISABLING\",0");
        mobile_at_cmd(cmdline, result, RE_BUF_LEN-1);
    } else if((nsa_flag == 0) && (nsa_type == 1)) { //set NSA mode
        snprintf(cmdline,sizeof(cmdline), "AT+ESBP=5,\"SBP_N1_MODE_NOT_ALLOWED_PERMANENT_DISABLING\",1");
        mobile_at_cmd(cmdline, result, RE_BUF_LEN-1);
    }
    
    // 步骤5: 等待配置生效
    sleep(2);

    return ret;
}

/**
 * @brief 更新最终网络类型
 *
 * 根据基础配置中的网络类型和子类型，计算并设置最终的网络类型字符。
 * 网络类型映射规则：
 * - '5': 5G SA (独立组网)
 * - '4': 5G NSA (非独立组网)
 * - '6': 5G NSA+SA (混合组网) 或 4G+5G
 * - '1': 4G only
 * - '2': 3G only
 * - '3': 2G only
 * - '0': 其他或未知类型
 */
void mobile_update_final_nettype(void) {
    MOBILE_DEBUG("NetType=%s\n", g_module_desc.basic_config.net_type);
    if (strcmp(g_module_desc.basic_config.net_type, "5G") == 0) {
        if (strcasecmp(g_module_desc.basic_config.net_sub_type, "SA") == 0) {
            g_module_desc.final_nettype = '5';
        } else if (strcasecmp(g_module_desc.basic_config.net_sub_type, "NSA") == 0) {
            g_module_desc.final_nettype = '4';
        } else {
            g_module_desc.final_nettype = '6';
        }
    } else if (strcmp(g_module_desc.basic_config.net_type, "5G-NSA") == 0 ||
               strcmp(g_module_desc.basic_config.net_type, "4G+5G") == 0) {
        g_module_desc.final_nettype = '6';
    } else if (strcmp(g_module_desc.basic_config.net_type, "4G") == 0) {
        g_module_desc.final_nettype = '1';
    } else if (strcmp(g_module_desc.basic_config.net_type, "3G") == 0) {
        g_module_desc.final_nettype = '2';
    } else if (strcmp(g_module_desc.basic_config.net_type, "2G") == 0) {
        g_module_desc.final_nettype = '3';
    } else {
        g_module_desc.final_nettype = '0';
    }
}

/**
 * 创建运行目录
 * 用于保存进程运行文件，如果目录不存在则创建
 *
 * @return 成功返回0，失败返回错误码
 */
int mobile_init_mobile_dir(void) {
    struct stat dir_stat;

    // 检查目录是否已存在
    if (stat(DIR_MOBILE_PATH, &dir_stat) == 0) {
        MOBILE_DEBUG("Mobile directory already exists: %s\n", DIR_MOBILE_PATH);
        return 0;
    }

    // 创建目录
    if (mkdir(DIR_MOBILE_PATH, 0755) != 0) {
        MOBILE_ERROR("Failed to create mobile directory: %s, error: %s\n",
                     DIR_MOBILE_PATH, strerror(errno));
        return -1;
    }

    MOBILE_INFO("Mobile directory created successfully: %s\n", DIR_MOBILE_PATH);
    return 0;
}

/**
 * 记录用户进程ID到指定文件
 * 用于保存进程ID信息，便于后续进程管理和监控
 *
 * @param pid 要记录的进程ID，必须大于1（避免记录系统进程）
 * @param file_path 保存进程ID的文件路径，不能为空且必须有效
 * @return 成功返回1，失败返回0
 */
int mobile_record_user_pid(int pid, const char* file_path) {
    char buf[32] = {0};

    if (file_path && file_path[0] && pid > 1) {
        sprintf(buf, "%d", pid);
        mobile_write_buff_to_file(file_path, buf, strlen(buf));

        return 1;
    }

    return 0;
}

/**
 * 获取PID和VID
 * 从设备文件中读取PID和VID信息并保存到全局模块描述符
 *
 * @return 成功返回1，失败返回0
 */
int mobile_get_pid_vid(void) {
    int ret = 0;
    char buf[32] = {0};

    // 读取PID文件
    ret = mobile_read_first_line_from_file(MOBILE_PID_FILE, buf, sizeof(buf));
    if (ret < 0) {
        MOBILE_ERROR("Failed to read PID file: %s\n", MOBILE_PID_FILE);
        return 0;
    }
    g_module_desc.pid = strtoul(buf, NULL, 0);

    // 清空缓冲区并读取VID文件
    memset(buf, 0, sizeof(buf));
    ret = mobile_read_first_line_from_file(MOBILE_VID_FILE, buf, sizeof(buf));
    if (ret < 0) {
        MOBILE_ERROR("Failed to read VID file: %s\n", MOBILE_VID_FILE);
        return 0;
    }
    g_module_desc.vid = strtoul(buf, NULL, 0);

    // 验证PID和VID的有效性
    if (g_module_desc.pid <= 0 || g_module_desc.vid <= 0) {
        MOBILE_ERROR("Invalid PID or VID values: pid=%04x, vid=%04x\n", g_module_desc.pid, g_module_desc.vid);
        return 0;
    } else {
        MOBILE_INFO("Device PID/VID: pid=%04x, vid=%04x\n", g_module_desc.pid, g_module_desc.vid);
        snprintf(buf, sizeof(buf), "%04x %04x\n", g_module_desc.vid, g_module_desc.pid);
        mobile_write_buff_to_file(USB_VID_PID_FILE, buf, strlen(buf));
    }

    return 1;
}

/**
 * 初始化预置at命令
 *
 * @return 成功返回0，失败返回-1
 */
int mobile_init_default_at_command_table(void) {
    const at_config_t *matched_config = NULL;

    if (mobile_init_at_config(g_module_desc.pid, g_module_desc.vid, &matched_config) == 0) {
        // 匹配成功，进行赋值
        g_module_desc.at_config = matched_config;
        g_module_desc.dial_type = matched_config->dial_type;
        strncpy(g_module_desc.module_name, matched_config->module_name, sizeof(g_module_desc.module_name) - 1);
        strncpy(g_module_desc.device_name, matched_config->ifname, sizeof(g_module_desc.device_name));
        mobile_write_buff_to_file(NET_IFNAME_FILE, g_module_desc.device_name, strlen(g_module_desc.device_name));
        MOBILE_INFO("AT config matched successfully: %s\n", matched_config->module_name);
        return 0;
    }
    
    MOBILE_ERROR("No matching AT config found for PID=%04x, VID=%04x\n", g_module_desc.pid, g_module_desc.vid);
    return -1;
}

/**
 * @brief 初始化SIM类型
 *
 * 从UCI配置中读取SIM类型设置，并与调制解调器当前SIM类型进行比较。
 * 如果两者不一致，则将UCI配置同步到调制解调器。
 * 步骤：
 * 1. 从UCI配置获取SIM类型
 * 2. 从调制解调器获取当前SIM类型（最多重试10次）
 * 3. 如果两者不同，则应用UCI配置到调制解调器
 *
 * @return 成功返回0，失败返回错误码
 */
int mobile_init_sim_type(void) {
    int ret = 0;
    int count_get = 0;
    int count_set = 0;
    sim_type_t sim_type_uci = SIM_TYPE_NONE;
    sim_type_t sim_type_modem = SIM_TYPE_NONE;

    //1. 获取uci esim 配置
    ret = mobile_uci_get_option_int("mobile", "@basic[0]", "simType", &sim_type_uci);
    if (ret) { return ret; }

    if (sim_type_uci != SIM_TYPE_SIM && sim_type_uci != SIM_TYPE_ESIM) {
        MOBILE_ERROR("simType is %d, but must be %d or %d\n", sim_type_uci, SIM_TYPE_SIM, SIM_TYPE_ESIM);
        return -1;
    }

    //2. 获取系统esim设置,同步uci esim配置到esim上
    while(1) {
        if (count_get == 20 || count_set == 20) { 
            MOBILE_ERROR("count_get:%d count_set:%d\n", count_get, count_set);
            return -1; 
        }

        sim_type_modem = mobile_get_sim_type();
        if (!sim_type_modem) {
            count_get++;
            sleep(1);
            continue;
        }

        if (sim_type_uci != sim_type_modem) {
            mobile_apply_sim_type(sim_type_uci);
            count_set++;
            sleep(1);
            continue;
        }

        break;
    }

    return 0;
}

/**
 * @brief 获取调制解调器当前SIM类型
 *
 * 通过发送AT+QUIMSLOT?命令查询调制解调器当前使用的SIM类型。
 * 解析返回结果，判断是物理SIM卡(SIM_TYPE_SIM)还是eSIM(SIM_TYPE_ESIM)。
 * 如果命令执行失败或返回未知结果，则返回SIM_TYPE_NONE。
 *
 * @return sim_type_t SIM类型枚举值
 *   - SIM_TYPE_NONE: 未知或失败
 *   - SIM_TYPE_SIM: 物理SIM卡
 *   - SIM_TYPE_ESIM: eSIM
 */
sim_type_t mobile_get_sim_type(void) {
    char result[128] = {0};
    sim_type_t sim_type = SIM_TYPE_NONE;

    if (mobile_at_cmd("AT+QUIMSLOT?", result, sizeof(result) - 1) != 0) {
        MOBILE_ERROR("Failed to execute AT+QUIMSLOT? command\n");
        return sim_type;
    }

    if (strstr(result, "+QUIMSLOT: 2")) {
        sim_type = SIM_TYPE_ESIM;
    } else if (strstr(result, "+QUIMSLOT: 1")) {
        sim_type = SIM_TYPE_SIM;
    }

    return sim_type;
}

/**
 * @brief 应用SIM类型到调制解调器
 *
 * 通过发送AT命令序列将指定的SIM类型配置到调制解调器。
 * 步骤：
 * 1. 发送AT+EFUN=0,3 关闭射频功能
 * 2. 发送AT+QUIMSLOT=<sim_type> 设置SIM类型
 * 3. 发送AT+EFUN=1 重新开启射频功能
 *
 * @param sim_type 要设置的SIM类型，取值为SIM_TYPE_SIM或SIM_TYPE_ESIM
 * @return 无返回值
 */
void mobile_apply_sim_type(sim_type_t sim_type) {
    char command[32] = {0};
    char result[128] = {0};

    sprintf(command, "AT+EFUN=0,3");
    mobile_at_cmd(command, result, sizeof(result) - 1);

    memset(command, '\0', sizeof(command));
    sprintf(command, "AT+QUIMSLOT=%d", sim_type);
    mobile_at_cmd(command, result, sizeof(result) - 1);

    memset(command, '\0', sizeof(command));
    sprintf(command, "AT+EFUN=1");
    mobile_at_cmd(command, result, sizeof(result) - 1);
}

/**
 * 获取移动网络设备名称
 * 返回当前移动网络接口的设备名称
 *
 * @return 网络设备名称字符串指针，不会为NULL
 */
const char* mobile_get_net_device_name(void) {
    if (g_module_desc.device_name[0] == '\0') {
        MOBILE_WARN("Network device name is empty, using default\n");
        return "wan5g";  // 默认设备名称
    }
    
    MOBILE_DEBUG("Current network device name: %s\n", g_module_desc.device_name);
    return g_module_desc.device_name;
}

/**
 * 更新device name
 *
 */
void mobile_update_device_name(void) {
    if (!strcmp(g_module_desc.wanstatus, DIALD_CONNECTED)) {
        inf_status_t inf_status;
        memset(&inf_status, 0, sizeof(inf_status));
        mobile_get_interface_info(MOBILE_BASICWAN_NAME, &inf_status);
        strncpy(g_module_desc.device_name, inf_status.l3_device, sizeof(inf_status.l3_device));
        mobile_write_buff_to_file(NET_IFNAME_FILE, g_module_desc.device_name, strlen(g_module_desc.device_name));
    }
}


/**
 * @brief 检查字符串是否只包含有效的IMEI字符（数字）
 * @param str 要检查的字符串
 * @return 1 - 有效, 0 - 无效
 */
int mobile_is_valid_imei_char(const char* str) {
    if (str == NULL || *str == '\0') {
        return 0;
    }
    
    for (int i = 0; str[i] != '\0'; i++) {
        if (str[i] < '0' || str[i] > '9') {
            return 0;
        }
    }
    
    return 1;
}

/**
 * @brief 初始化IMEI信息
 * 通过发送AT+CGSN命令获取设备IMEI，并保存到文件和全局变量中
 *
 * @return 成功返回0，失败返回-1
 */
int mobile_init_imei(void) {
    char result[128] = {0};
    char imei[64] = {0};
    char* line_start = NULL;
    char* line_end = NULL;
    char* current_pos = result;
    int imei_found = 0;
    
    // 检查AT配置是否有效
    if (g_module_desc.at_config == NULL) {
        MOBILE_ERROR("AT config is NULL, cannot get IMEI\n");
        return -1;
    }
    
    // 使用mobile_at_cmd执行AT+CGSN命令
    if (mobile_at_cmd(g_module_desc.at_config->at_get_imei, result, sizeof(result) - 1) != 0) {
        MOBILE_ERROR("Failed to execute AT+CGSN command\n");
        return -1;
    }
    
    // 解析返回结果
    if (strlen(result) == 0) {
        MOBILE_ERROR("Empty response from AT+CGSN command\n");
        return -1;
    }
    
    // 逐行解析响应，寻找IMEI号码
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
        
        // 检查当前行是否是有效的IMEI号码
        if (strlen(line_start) >= 5 && strlen(line_start) <= 20 &&
            mobile_is_valid_imei_char(line_start)) {
            // 找到有效的IMEI号码
            strncpy(imei, line_start, sizeof(imei) - 1);
            imei[sizeof(imei) - 1] = '\0';
            imei_found = 1;
            *line_end = temp_char; // 恢复原始字符
            break;
        }
        
        // 恢复原始字符并移动到下一行
        *line_end = temp_char;
        current_pos = line_end;
        if (*current_pos != '\0') {
            current_pos++;
        }
    }
    
    if (!imei_found) {
        MOBILE_ERROR("No valid IMEI found in response: %s\n", result);
        return -1;
    }
    
    // 过滤掉无效的IMEI值
    if (strstr(imei, "tty") != NULL || strstr(imei, "TTY") != NULL ||
        strstr(imei, "No") != NULL || strstr(imei, "NO") != NULL) {
        MOBILE_ERROR("IMEI contains invalid keywords: %s\n", imei);
        return -1;
    }
    
    // 保存IMEI到文件
    if (mobile_write_buff_to_file(IMEI_RESULT_FILE, imei, strlen(imei)) != 1) {
        MOBILE_ERROR("Failed to write IMEI to file: %s\n", IMEI_RESULT_FILE);
        return -1;
    }
    
    // 保存IMEI到全局变量
    strncpy(g_module_desc.imei, imei, sizeof(g_module_desc.imei) - 1);
    g_module_desc.imei[sizeof(g_module_desc.imei) - 1] = '\0';
    
    MOBILE_INFO("IMEI initialized successfully: %s\n", imei);
    return 0;
}

/**
 * @brief 获取modem 版本
 *
 * 无返回值
 */
void mobile_init_version(void) {
    if (g_module_desc.pid == 0x7006 && g_module_desc.vid == 0x2c7c) {
        mobile_init_version_quectel();
    } else {
        return true;
    }
}

/**
 * @brief 初始化Quectel模块版本信息
 * 通过发送AT+QGMR命令获取模块版本，并保存到文件和全局变量中
 *
 * @return 成功返回0，失败返回-1
 */
int mobile_init_version_quectel(void) {
    char result[256] = {0};
    char module_version[64] = {0};
    char* line_start = NULL;
    char* line_end = NULL;
    char* current_pos = result;
    int version_found = 0;
    
    // 检查AT配置是否有效
    if (g_module_desc.at_config == NULL) {
        MOBILE_ERROR("AT config is NULL, cannot get module version\n");
        return -1;
    }
    
    // 使用mobile_at_cmd执行AT+QGMR命令
    if (mobile_at_cmd("AT+QGMR", result, sizeof(result) - 1) != 0) {
        MOBILE_ERROR("Failed to execute AT+QGMR command\n");
        return -1;
    }
    
    // 解析返回结果
    if (strlen(result) == 0) {
        MOBILE_ERROR("Empty response from AT+QGMR command\n");
        return -1;
    }
    
    // 过滤掉AT命令回显和错误信息
    if (strstr(result, "ERROR") != NULL || strstr(result, "error") != NULL) {
        MOBILE_ERROR("AT+QGMR command returned ERROR\n");
        return -1;
    }
    
    // 逐行解析响应，寻找版本信息
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
        
        // 检查当前行是否是有效的版本信息
        // 跳过AT命令回显行和OK行
        if (strlen(line_start) >= 3 &&
            strstr(line_start, "AT") == NULL &&
            strstr(line_start, "OK") == NULL &&
            strstr(line_start, "at") == NULL &&
            strstr(line_start, "ok") == NULL) {
            // 处理+QGMR:格式的响应
            char* version_ptr = line_start;
            if (strstr(line_start, "+QGMR:") != NULL) {
                version_ptr = line_start + strlen("+QGMR:");
            }
            
            // 去除前后空白字符
            while (*version_ptr == ' ' || *version_ptr == '\t') {
                version_ptr++;
            }
            
            // 去除引号
            if (*version_ptr == '"') {
                version_ptr++;
                char* quote_end = strchr(version_ptr, '"');
                if (quote_end != NULL) {
                    *quote_end = '\0';
                }
            }
            
            // 复制版本信息
            if (strlen(version_ptr) >= 3) {
                strncpy(module_version, version_ptr, sizeof(module_version) - 1);
                module_version[sizeof(module_version) - 1] = '\0';
                version_found = 1;
                *line_end = temp_char; // 恢复原始字符
                break;
            }
        }
        
        // 恢复原始字符并移动到下一行
        *line_end = temp_char;
        current_pos = line_end;
        if (*current_pos != '\0') {
            current_pos++;
        }
    }
    
    if (!version_found) {
        MOBILE_ERROR("No valid version found in response: %s\n", result);
        return -1;
    }
    
    // 过滤掉无效的版本值
    if (strstr(module_version, "tty") != NULL || strstr(module_version, "TTY") != NULL ||
        strstr(module_version, "No") != NULL || strstr(module_version, "NO") != NULL) {
        MOBILE_ERROR("Module version contains invalid keywords: %s\n", module_version);
        return -1;
    }
    
    // 保存版本信息到全局变量
    strncpy(g_module_desc.module_version, module_version, sizeof(g_module_desc.module_version) - 1);
    g_module_desc.module_version[sizeof(g_module_desc.module_version) - 1] = '\0';

    // 保存version到文件
    if (mobile_write_buff_to_file(MODULE_VERSION_FILE, g_module_desc.module_version, strlen(g_module_desc.module_version)) != 1) {
        MOBILE_ERROR("Failed to write version to file: %s\n", MODULE_VERSION_FILE);
        return -1;
    }
    
    MOBILE_INFO("Quectel module version initialized successfully: %s\n", module_version);
    return 0;
}

/**
 * @brief 初始化获取serialnum值
 *
 * @return 成功返回0，失败返回-1
 */
int mobile_init_serialnum(void) {
    FILE *fp;
    char buffer[256];
    char *start, *end;
    
    // 执行命令并读取输出
    fp = popen("produce sn", "r");
    if (fp == NULL) {
        perror("popen failed");
        return -1;
    }
    
    // 读取命令输出
    if (fgets(buffer, sizeof(buffer), fp) == NULL) {
        pclose(fp);
        return -1; // 命令没有输出
    }
    
    pclose(fp);
    
    // 检查是否为空行或只有换行符
    if (strlen(buffer) <= 1) {
        return -2; // SN没有值
    }
    
    // 查找第一个感叹号
    start = strchr(buffer, '!');
    if (start == NULL) {
        return -2; // 未找到感叹号，SN没有值
    }
    start++; // 跳过第一个感叹号
    
    // 查找第二个感叹号
    end = strchr(start, '!');
    if (end == NULL) {
        return -2; // 未找到第二个感叹号，SN没有值
    }
    
    // 检查两个感叹号之间是否有数据
    if (end == start) {
        return -2; // 两个感叹号之间没有数据
    }
    
    // 计算要复制的数据长度
    size_t length = end - start;
    
    // 复制两个感叹号之间的数据到输出参数
    strncpy(g_module_desc.serialnum, start, length);
    g_module_desc.serialnum[length] = '\0'; // 确保字符串以null结尾
    MOBILE_INFO("serialnum initialized successfully: %s\n", g_module_desc.serialnum);
    return 0; // 成功
}

/**
 * @brief 初始化获取imsi值
 *
 */
void mobile_init_imsi(void) {
    int i = 0;
    char* p = NULL;
    char cmd[256] = {0};
    char buff[256] = {0};
    char imsi[32] = {0};
    FILE* fp = NULL;

    sprintf(cmd, "at-mngr AT+CIMI");
    fp = popen(cmd, "r");
    if (fp == NULL) {
        return;
    }

    while (fgets(buff, sizeof(buff), fp) != NULL) {
        if (strstr(buff, "AT") != NULL || strstr(buff, "at") != NULL) {
            continue;
        }

        if (strlen(buff) < 5) {
            continue;
        }

        if (strstr(buff, "ERROR") != NULL || strstr(buff, "error") != NULL) {
            continue;
        }

        for (i = 0; i < strlen(buff); i++) {
            if (buff[i] == '\n' || buff[i] == '\r') {
                buff[i] = 0;
                break;
            }
        }

        if (strncmp(buff, "0x", 2) == 0) {
            if (mobile_check_valid_char(buff + 2) == 1) {
                strcpy(imsi, buff);
                break;
            }
        } else if ((p = strstr(buff, "CIMI:")) != NULL) {
            strcpy(buff, p + strlen("CIMI:") + 1);
        }

        if (buff[0] == '"') {
            sprintf(buff, "%s", &buff[1]);
            if ((p = strstr(buff, "\"")) != NULL) {
                *p = 0;
            }
        }

        if (mobile_check_valid_char(buff) == 0) {
            continue;
        }

        strcpy(imsi, buff);
        break;
    }
    pclose(fp);

    if (imsi[0] == 0 || strlen(imsi) < 5) {
        return;
    }

    if (strstr(imsi, "tty") != NULL || strstr(imsi, "TTY") != NULL || strstr(imsi, "No") != NULL ||
        strstr(imsi, "NO") != NULL) {
        return;
    }

    if (strlen(imsi) > 0) {
        mobile_write_buff_to_file(IMSI_RESULT_FILE, imsi, strlen(imsi));
        strncpy(g_module_desc.imsi, imsi, sizeof(g_module_desc.imsi) - 1);
        MOBILE_DEBUG("get imsi success [%s]\n", g_module_desc.imsi);
    }
}

/**
 * @brief 初始化获取ccid值
 *
 */
void mobile_init_ccid(void) {
    int i = 0;
    char* p = NULL;
    char cmd[256] = {0};
    char buff[256] = {0};
    char ccid[32] = {0};
    FILE* fp = NULL;

    sprintf(cmd, "at-mngr AT+ICCID");

    fp = popen(cmd, "r");
    if (fp == NULL) {
        return;
    }

    while (fgets(buff, sizeof(buff), fp) != NULL) {
        if (strstr(buff, "AT") != NULL || strstr(buff, "at") != NULL) {
            continue;
        }

        if (strlen(buff) < 5) {
            continue;
        }

        if (strstr(buff, "ERROR") != NULL || strstr(buff, "error") != NULL) {
            continue;
        }

        for (i = 0; i < strlen(buff); i++) {
            if (buff[i] == '\n' || buff[i] == '\r') {
                buff[i] = 0;
                break;
            }
        }

        if (strncmp(buff, "0x", 2) == 0) {
            if (mobile_check_valid_char(buff + 2) == 1) {
                strcpy(ccid, buff);
                break;
            }
        }
        else if ((p = strstr(buff, "ICCID:")) != NULL) {
            strcpy(buff, p + strlen("ICCID:") + 1);
        }

        if (buff[0] == '"') {
            sprintf(buff, "%s", &buff[1]);
            if ((p = strstr(buff, "\"")) != NULL) {
                *p = 0;
            }
        }

        if (mobile_check_valid_char(buff) == 0) {
            continue;
        }

        strcpy(ccid, buff);
        break;
    }
    pclose(fp);

    if (ccid[0] == 0 || strlen(ccid) < 5) {
        return;
    }

    if (strstr(ccid, "tty") != NULL || strstr(ccid, "TTY") != NULL || strstr(ccid, "No") != NULL ||
        strstr(ccid, "NO") != NULL) {
        return;
    }

    mobile_write_buff_to_file(ICCID_RESULT_FILE, ccid, strlen(ccid));
    strncpy(g_module_desc.ccid, ccid, sizeof(g_module_desc.ccid) - 1);
}

/**
 * @brief 初始化modem相关信息
 *
 * @return 成功返回0，失败返回-1
 */
int mobile_init_odu_basic(void) {
    int i = 0;
    char* p = NULL;
    char cmd[256] = {0};
    char buff[256] = {0};
    FILE* fp = NULL;
    char* pstart = NULL;
    char curr_sn[32] = {0};
    char sn[32] = {0};
    char manufacturer[256] = {0};

    sprintf(cmd, "at-mngr at+egmr=0,5");
    fp = popen(cmd, "r");
    if (fp == NULL) {
        return -1;
    }

    while (fgets(buff, sizeof(buff), fp) != NULL) {
        //+EGMR: "P1Q22CH0A0021230P"
        if (strcasestr(buff, "+EGMR: \"") != NULL) {
            for (i = 0; i < strlen(buff); i++) {
                if (buff[i] == '\n' || buff[i] == '\r') {
                    buff[i] = 0;
                    break;
                }
            }

            pstart = strstr(buff, "+EGMR: \"");
            if (pstart != NULL) {
                strcpy(curr_sn, pstart + strlen("+EGMR: \""));
            }

            curr_sn[strlen(curr_sn) - 1] = 0;
            break;
        }
    }
    pclose(fp);

    {
        strncpy(g_module_desc.module_serialnum, curr_sn, sizeof(g_module_desc.module_serialnum) - 1);
        mobile_write_buff_to_file(ODUSN_RESULT_FILE, curr_sn, strlen(curr_sn));
        strcpy(manufacturer, g_module_desc.module_name);

        char* ptr = strstr(manufacturer, " ");
        if (ptr) {
            ptr[0] = 0;
            ptr++;
            mobile_write_buff_to_file(ODUMANUF_RESULT_FILE, manufacturer, strlen(manufacturer));
            mobile_write_buff_to_file(ODUMODEL_RESULT_FILE, ptr, strlen(ptr));
        } else {
            mobile_write_buff_to_file(ODUMODEL_RESULT_FILE, manufacturer, strlen(manufacturer));
        }
    }

    return 0;
}

/**
 * @brief 获取provider
 *
 * @return 成功返回true，失败返回false
 */
bool mobile_get_provider(void) {
    char *p = NULL;
    char *p1 = NULL;
    char result[256] = {0};
    char command[64] = {0};


    if (!mobile_set_apn_mccmnc_str())
        return false;

    snprintf(command, sizeof(command), "%s?", "at+cops");
    if (mobile_at_cmd((const char*)command, result, sizeof(result)) == 0) {
        if (strcasestr(result, "OK")) {
            if ((p = strstr(result, ",\"")) != NULL) {
                p += 2;
        
                if ((p1 = strstr(p, "\"")) != NULL) {
                    *p1 = '\0';
                    mobile_write_buff_to_file(PROVIDER_RESULT_FILE, p, strlen(p));
                    MOBILE_DEBUG("provider : [%s]\n", p);
                    return true;
                }
            }
        }
    }
    MOBILE_DEBUG("get provider fail\n");

    return false;
}

/**
 * @brief 检查AT命令是否就绪
 * 通过发送AT命令并检查返回结果来判断设备是否就绪
 *
 * @return 成功返回1，失败返回0
 */
int mobile_at_is_ready(void) {
    int ret = 1;
    char result[32] = {0};

    // 发送AT命令检查设备响应
    mobile_at_cmd(g_module_desc.at_config->at_test, result, sizeof(result) - 1);

    // 检查响应结果是否包含"OK"
    if (strstr(result, "OK") == NULL && strstr(result, "ok") == NULL) {
        MOBILE_WARN("AT command response does not contain 'OK': %s\n", result);
        ret = 0;
    } else {
        MOBILE_DEBUG("AT command ready, response: %s\n", result);
    }
    
    return ret;
}

/**
 * @brief 检查网络是否注册
 *
 * @return 成功返回1，失败返回0
 */
int mobile_network_is_registered(bool tag) {
    int ret = 1;
    char result[128] = {0};
    g_module_desc.regtype = REG_TYPE_NONE;

    memset(result, 0, sizeof(result));
    if (mobile_at_cmd(g_module_desc.at_config->at_get_reg_5g, result, sizeof(result) - 1) == 0) {
        if ((strstr(result, "+EGREG: ")) && (strstr(result, ",1,\"") || strstr(result, ",5,\""))) {
            if (strstr(result, "32768")){
                g_module_desc.regtype = REG_TYPE_NR;
            } else if (strstr(result, "16384")) {
                g_module_desc.regtype = REG_TYPE_ENDC;
            } else if (strstr(result, "4096") || strstr(result, "8192")) {
                g_module_desc.regtype = REG_TYPE_LTE;
            } else if (strstr(result, "24") || strstr(result, "48")) {
                g_module_desc.regtype = REG_TYPE_WCDMA;
            }
        } else {
            g_module_desc.regtype = REG_TYPE_NONE;
        }
    }

    if (tag) {
        if (g_module_desc.regtype == REG_TYPE_NONE) {
            MOBILE_ERROR("network not register\n");
            ret = 0;
        } else {
            MOBILE_INFO("network registerd\n");
            mobile_write_buff_to_file(SIM_STATUS_FILE, "Registered", strlen("Registered"));
        }
    }
    return ret;
}

/**
 * @brief 保存LTE小区信息到文件
 *
 * 将LTE小区的详细信息保存到文件中，包括：
 * - 网络类型
 * - 小区ID、物理小区ID
 * - 跟踪区码、ARFCN
 * - 信号质量参数（RSSI、RSRP、RSRQ、SINR）
 * - 频段信息
 * - eNodeB ID
 * - NR相关字段（设置为空值）
 *
 * @return 无返回值
 */
void mobile_lte_cellinfo_save(void) {
    FILE* fp = fopen(CELL_INFO_FILE, "w+");
    if (fp) {
        char buf_line[128] = {0};

        snprintf(buf_line, sizeof(buf_line), "netType=%s\n", g_module_desc.rf.net_type);
        fputs(buf_line, fp);
        snprintf(buf_line, sizeof(buf_line), "lte_cellid=%s\n", g_module_desc.rf.global_cell_id);
        fputs(buf_line, fp);
        snprintf(buf_line, sizeof(buf_line), "lte_pcid=%s\n", g_module_desc.rf.physical_cell_id);
        fputs(buf_line, fp);
        snprintf(buf_line, sizeof(buf_line), "lte_tac=%s\n", g_module_desc.rf.tac);
        fputs(buf_line, fp);
        snprintf(buf_line, sizeof(buf_line), "lte_arfcn=%s\n", g_module_desc.rf.dl_earfcn);
        fputs(buf_line, fp);
        snprintf(buf_line, sizeof(buf_line), "lte_rssi=%s\n", g_module_desc.rf.rssi);
        fputs(buf_line, fp);
        snprintf(buf_line, sizeof(buf_line), "lte_rsrp=%s\n", g_module_desc.rf.rsrp);
        fputs(buf_line, fp);
        snprintf(buf_line, sizeof(buf_line), "lte_rsrq=%s\n", g_module_desc.rf.rsrq);
        fputs(buf_line, fp);
        snprintf(buf_line, sizeof(buf_line), "lte_sinr=%s\n", g_module_desc.rf.sinr);
        fputs(buf_line, fp);
        snprintf(buf_line, sizeof(buf_line), "lte_band=%s\n", g_module_desc.rf.frequency_band);
        fputs(buf_line, fp);
        snprintf(buf_line, sizeof(buf_line), "lte_nodeb=%s\n", g_module_desc.enb_id);
        fputs(buf_line, fp);
        snprintf(buf_line, sizeof(buf_line), "nr_cellid=%s\n", "");
        fputs(buf_line, fp);
        snprintf(buf_line, sizeof(buf_line), "nr_pcid=%s\n", "");
        fputs(buf_line, fp);
        snprintf(buf_line, sizeof(buf_line), "nr_tac=%s\n", "");
        fputs(buf_line, fp);
        snprintf(buf_line, sizeof(buf_line), "nr_arfcn=%s\n", "");
        fputs(buf_line, fp);
        snprintf(buf_line, sizeof(buf_line), "nr_rssi=%s\n", "");
        fputs(buf_line, fp);
        snprintf(buf_line, sizeof(buf_line), "nr_rsrp=%s\n", "");
        fputs(buf_line, fp);
        snprintf(buf_line, sizeof(buf_line), "nr_rsrq=%s\n", "");
        fputs(buf_line, fp);
        snprintf(buf_line, sizeof(buf_line), "nr_sinr=%s\n", "");
        fputs(buf_line, fp);
        snprintf(buf_line, sizeof(buf_line), "nr_band=%s\n", "");
        fputs(buf_line, fp);
        snprintf(buf_line, sizeof(buf_line), "nr_nodeb=%s\n", "");
        fputs(buf_line, fp);
        snprintf(buf_line, sizeof(buf_line), "nr_ssb_rsrp=%s\n", "");
        fputs(buf_line, fp);

        fclose(fp);
    }
}

/**
 * @brief 保存NR5G SA小区信息到文件
 *
 * 将NR5G独立组网(SA)小区的详细信息保存到文件中，包括：
 * - 网络类型
 * - NR小区ID、物理小区ID
 * - 跟踪区码、ARFCN
 * - 信号质量参数（RSSI、RSRP、RSRQ、SINR）
 * - 频段信息
 * - gNodeB ID
 * - SSB RSRP
 * - LTE相关字段（设置为空值）
 *
 * @return 无返回值
 */
void mobile_nr5g_sa_cellinfo_save(void) {
    FILE* fp = fopen(CELL_INFO_FILE, "w+");
    if (fp) {
        char buf_line[128] = {0};

        snprintf(buf_line, sizeof(buf_line), "netType=%s\n", g_module_desc.rf.net_type);
        fputs(buf_line, fp);
        snprintf(buf_line, sizeof(buf_line), "lte_cellid=%s\n", "");
        fputs(buf_line, fp);
        snprintf(buf_line, sizeof(buf_line), "lte_pcid=%s\n", "");
        fputs(buf_line, fp);
        snprintf(buf_line, sizeof(buf_line), "lte_tac=%s\n", "");
        fputs(buf_line, fp);
        snprintf(buf_line, sizeof(buf_line), "lte_arfcn=%s\n", "");
        fputs(buf_line, fp);
        snprintf(buf_line, sizeof(buf_line), "lte_rssi=%s\n", "");
        fputs(buf_line, fp);
        snprintf(buf_line, sizeof(buf_line), "lte_rsrp=%s\n", "");
        fputs(buf_line, fp);
        snprintf(buf_line, sizeof(buf_line), "lte_rsrq=%s\n", "");
        fputs(buf_line, fp);
        snprintf(buf_line, sizeof(buf_line), "lte_sinr=%s\n", "");
        fputs(buf_line, fp);
        snprintf(buf_line, sizeof(buf_line), "lte_band=%s\n", "");
        fputs(buf_line, fp);
        snprintf(buf_line, sizeof(buf_line), "lte_nodeb=%s\n", g_module_desc.enb_id);
        fputs(buf_line, fp);
        snprintf(buf_line, sizeof(buf_line), "nr_cellid=%s\n", g_module_desc.rf.global_cell_id);
        fputs(buf_line, fp);
        snprintf(buf_line, sizeof(buf_line), "nr_pcid=%s\n", g_module_desc.rf.physical_cell_id);
        fputs(buf_line, fp);
        snprintf(buf_line, sizeof(buf_line), "nr_tac=%s\n", g_module_desc.rf.tac);
        fputs(buf_line, fp);
        snprintf(buf_line, sizeof(buf_line), "nr_arfcn=%s\n", g_module_desc.rf.dl_earfcn);
        fputs(buf_line, fp);
        snprintf(buf_line, sizeof(buf_line), "nr_rssi=%s\n", g_module_desc.rf.rssi);
        fputs(buf_line, fp);
        snprintf(buf_line, sizeof(buf_line), "nr_rsrp=%s\n", g_module_desc.rf.rsrp);
        fputs(buf_line, fp);
        snprintf(buf_line, sizeof(buf_line), "nr_rsrq=%s\n", g_module_desc.rf.rsrq);
        fputs(buf_line, fp);
        snprintf(buf_line, sizeof(buf_line), "nr_sinr=%s\n", g_module_desc.rf.sinr);
        fputs(buf_line, fp);
        snprintf(buf_line, sizeof(buf_line), "nr_band=%s\n", g_module_desc.rf.frequency_band);
        fputs(buf_line, fp);
        snprintf(buf_line, sizeof(buf_line), "nr_nodeb=%s\n", g_module_desc.gnb_id);
        fputs(buf_line, fp);
        snprintf(buf_line, sizeof(buf_line), "nr_ssb_rsrp=%s\n", g_module_desc.ssb_rsrp);
        fputs(buf_line, fp);

        fclose(fp);
    }
}

/**
 * @brief 保存NR5G NSA小区信息到文件
 *
 * 将NR5G非独立组网(NSA)小区的详细信息保存到文件中，包括：
 * - 网络类型
 * - LTE小区ID、物理小区ID
 * - 跟踪区码、ARFCN
 * - 信号质量参数（RSSI、RSRP、RSRQ、SINR）
 * - 频段信息
 * - eNodeB ID
 * - NR小区ID、物理小区ID（如果可用）
 * - NR信号质量参数
 * - gNodeB ID
 * - SSB RSRP
 *
 * 注意：当NR小区ID为"268435455"（FFFFFFF）时，表示无法获取真实的小区ID
 *
 * @return 无返回值
 */
void mobile_nr5g_nsa_cellinfo_save(void) {
    FILE* fp = fopen(CELL_INFO_FILE, "w+");
    if (fp) {
        char buf_line[128] = {0};

        snprintf(buf_line, sizeof(buf_line), "netType=%s\n", g_module_desc.rf.net_type);
        fputs(buf_line, fp);
        snprintf(buf_line, sizeof(buf_line), "lte_cellid=%s\n", g_module_desc.rf.global_cell_id);
        fputs(buf_line, fp);
        snprintf(buf_line, sizeof(buf_line), "lte_pcid=%s\n", g_module_desc.rf.physical_cell_id);
        fputs(buf_line, fp);
        snprintf(buf_line, sizeof(buf_line), "lte_tac=%s\n", g_module_desc.rf.tac);
        fputs(buf_line, fp);
        snprintf(buf_line, sizeof(buf_line), "lte_arfcn=%s\n", g_module_desc.rf.dl_earfcn);
        fputs(buf_line, fp);
        snprintf(buf_line, sizeof(buf_line), "lte_rssi=%s\n", g_module_desc.rf.rssi);
        fputs(buf_line, fp);
        snprintf(buf_line, sizeof(buf_line), "lte_rsrp=%s\n", g_module_desc.rf.rsrp);
        fputs(buf_line, fp);
        snprintf(buf_line, sizeof(buf_line), "lte_rsrq=%s\n", g_module_desc.rf.rsrq);
        fputs(buf_line, fp);
        snprintf(buf_line, sizeof(buf_line), "lte_sinr=%s\n", g_module_desc.rf.sinr);
        fputs(buf_line, fp);
        snprintf(buf_line, sizeof(buf_line), "lte_band=%s\n", g_module_desc.rf.frequency_band);
        fputs(buf_line, fp);
        snprintf(buf_line, sizeof(buf_line), "lte_nodeb=%s\n", g_module_desc.enb_id);
        fputs(buf_line, fp);
        
        // 检查NR小区ID是否为无效值（268435455 = 0xFFFFFFF）
        if (!strcmp(g_module_desc.rf2.global_cell_id, "268435455")) {
            snprintf(buf_line, sizeof(buf_line), "nr_cellid=%s\n", "");
        } else {
            snprintf(buf_line, sizeof(buf_line), "nr_cellid=%s\n", g_module_desc.rf2.global_cell_id);
        }
        fputs(buf_line, fp);
        snprintf(buf_line, sizeof(buf_line), "nr_pcid=%s\n", g_module_desc.rf2.physical_cell_id);
        fputs(buf_line, fp);
        snprintf(buf_line, sizeof(buf_line), "nr_tac=%s\n", "");
        fputs(buf_line, fp);
        snprintf(buf_line, sizeof(buf_line), "nr_arfcn=%s\n", g_module_desc.rf2.dl_earfcn);
        fputs(buf_line, fp);
        snprintf(buf_line, sizeof(buf_line), "nr_rssi=%s\n", g_module_desc.rf2.rssi);
        fputs(buf_line, fp);
        snprintf(buf_line, sizeof(buf_line), "nr_rsrp=%s\n", g_module_desc.rf2.rsrp);
        fputs(buf_line, fp);
        snprintf(buf_line, sizeof(buf_line), "nr_rsrq=%s\n", g_module_desc.rf2.rsrq);
        fputs(buf_line, fp);
        snprintf(buf_line, sizeof(buf_line), "nr_sinr=%s\n", g_module_desc.rf2.sinr);
        fputs(buf_line, fp);
        snprintf(buf_line, sizeof(buf_line), "nr_band=%s\n", g_module_desc.rf2.frequency_band);
        fputs(buf_line, fp);
        snprintf(buf_line, sizeof(buf_line), "nr_nodeb=%s\n", g_module_desc.gnb_id);
        fputs(buf_line, fp);
        snprintf(buf_line, sizeof(buf_line), "nr_ssb_rsrp=%s\n", g_module_desc.ssb_rsrp);
        fputs(buf_line, fp);

        fclose(fp);
    }
}

/**
 * @brief 根据网络类型保存相应的小区信息到文件
 *
 * 根据当前网络类型调用相应的保存函数：
 * - NR5G-SA: 调用NR5G独立组网小区信息保存函数
 * - NR5G-NSA: 调用NR5G非独立组网小区信息保存函数
 * - LTE: 调用LTE小区信息保存函数
 *
 * @return 无返回值
 */
void mobile_cellinfo_save(void) {
    if (!strcmp(g_module_desc.rf.net_type, "NR5G-SA")) {
        mobile_nr5g_sa_cellinfo_save();
    } else if (!strcmp(g_module_desc.rf.net_type, "NR5G-NSA")) {
        mobile_nr5g_nsa_cellinfo_save();
    } else if (!strcmp(g_module_desc.rf.net_type, "LTE")) {
        mobile_lte_cellinfo_save();
    }
}

/**
 * @brief 更新信号强度等级
 *
 * 根据网络类型和信号强度值计算信号等级，并保存到文件和全局变量中
 * 支持的网络类型：
 * - LTE/4G/5G: 使用RSRP值计算信号等级
 * - 其他网络类型: 使用RSSI值计算信号等级
 *
 * 信号等级映射规则：
 * LTE/4G/5G网络 (基于RSRP):
 * - 等级0: RSRP < -140dBm 或 RSRP = 0
 * - 等级1: -140dBm ≤ RSRP < -115dBm
 * - 等级2: -115dBm ≤ RSRP < -105dBm
 * - 等级3: -105dBm ≤ RSRP < -95dBm
 * - 等级4: -95dBm ≤ RSRP < -85dBm
 * - 等级5: RSRP ≥ -85dBm
 *
 * 其他网络类型 (基于RSSI):
 * - 等级0: RSSI = 0 或 RSSI = 99
 * - 等级1: 0 < RSSI < 3
 * - 等级2: 3 ≤ RSSI < 11
 * - 等级3: 11 ≤ RSSI < 19
 * - 等级4: 19 ≤ RSSI < 26
 * - 等级5: RSSI ≥ 26
 *
 * @return int 成功返回信号等级(0-5)，失败返回-1
 */
int mobile_update_signal_level(void) {
    int nettype_change = 0, rsrp_change = 0, rssi_change = 0;
    int i_signal_val = 0;
    int i_signal_level = 0;
    char signal_val[16] = {0};
    char signal_level[16] = {0};
    static int local_rsrp = 0;
    static int local_rssi = 0;
    static int local_signal_level = 0;
    static char local_nettype[16] = {0};

    if (strcmp(local_nettype, g_module_desc.rf.net_type)) {
        MOBILE_DEBUG("nettype [%s] change to [%s]\n", local_nettype, g_module_desc.rf.net_type);
        strncpy(local_nettype, g_module_desc.rf.net_type, sizeof(local_nettype));
        nettype_change = 1;
    }

    if (local_rsrp != g_module_desc.rsrp) {
        MOBILE_DEBUG("rsrp [%d] change to [%d]\n", local_rsrp, g_module_desc.rsrp);
        local_rsrp = g_module_desc.rsrp;
        rsrp_change = 1;
    }

    if (local_rssi != g_module_desc.csq) {
        MOBILE_DEBUG("rssi [%d] change to [%d]\n", local_rssi, g_module_desc.csq);
        local_rssi = g_module_desc.csq;
        rssi_change = 1;
    }

    if (nettype_change == 0 && rsrp_change == 0 && rssi_change == 0) {
        return -1;
    }

    if (strcasestr(g_module_desc.rf.net_type, "LTE") != NULL || strcasestr(g_module_desc.rf.net_type, "4G") != NULL || strcasestr(g_module_desc.rf.net_type, "5G") != NULL) {
        i_signal_val = g_module_desc.rsrp;
        if (i_signal_val == 0 && (strcasestr(g_module_desc.rf.net_type, "LTE") != NULL || strcasestr(g_module_desc.rf.net_type, "4G") != NULL)) {
            goto RSSI_CHECK;
        }
        if (i_signal_val == 0) {
            i_signal_level = 0;
        } else if (i_signal_val < -140) {
            i_signal_level = 0;
        } else if (i_signal_val < -115) {
            i_signal_level = 1;
        } else if (i_signal_val < -105) {
            i_signal_level = 2;
        } else if (i_signal_val < -95) {
            i_signal_level = 3;
        } else if (i_signal_val < -85) {
            i_signal_level = 4;
        } else {
            i_signal_level = 5;
        }
    } else if (g_module_desc.rf.net_type[0] != 0) {
    RSSI_CHECK:
        i_signal_val = g_module_desc.csq;
        if (i_signal_val == 0 || i_signal_val == 99) {
            i_signal_level = 0;
        } else if (i_signal_val < 3) {
            i_signal_level = 1;
        } else if (i_signal_val < 11) {
            i_signal_level = 2;
        } else if (i_signal_val < 19) {
            i_signal_level = 3;
        } else if (i_signal_val < 26) {
            i_signal_level = 4;
        } else {
            i_signal_level = 5;
        }
    } else {
        MOBILE_ERROR("No valid network type available\n");
        return -1;
    }

    if (local_signal_level == i_signal_level) {
        return;
    }

    local_signal_level = i_signal_level;

    snprintf(signal_level, sizeof(signal_level), "%d", i_signal_level);
    mobile_write_buff_to_file(SIGNAL_LEVEL_FILE, signal_level, strlen(signal_level));
    g_module_desc.signalLevel = i_signal_level;
    MOBILE_INFO("Signal level updated successfully: level=%d\n", i_signal_level);
    return i_signal_level;
}

/**
 * 打印模块描述信息
 * 详细打印g_module_desc结构体的所有字段
 *
 * @return 无返回值
 */
void mobile_print_module_desc_info(void) {
    MOBILE_INFO("=== Module Description Information ===\n");
    MOBILE_INFO("Dial Type: %d\n", g_module_desc.dial_type);
    MOBILE_INFO("WAN Status: %s\n", g_module_desc.wanstatus ? g_module_desc.wanstatus : "NULL");
    MOBILE_INFO("Final Net Type: %c\n", g_module_desc.final_nettype);
    MOBILE_INFO("VID: 0x%04X, PID: 0x%04X\n", g_module_desc.vid, g_module_desc.pid);
    MOBILE_INFO("IMS Status: %d\n", g_module_desc.imsststus);
    MOBILE_INFO("APN MCCMNC: %s\n", g_module_desc.apn_mccmnc);
    MOBILE_INFO("SIM MCCMNC: %s\n", g_module_desc.sim_mccmnc);
    MOBILE_INFO("Module Name: %s\n", g_module_desc.module_name);
    MOBILE_INFO("Device Name: %s\n", g_module_desc.device_name);
    MOBILE_INFO("IMEI: %s\n", g_module_desc.imei);
    MOBILE_INFO("IMSI: %s\n", g_module_desc.imsi);
    MOBILE_INFO("CCID: %s\n", g_module_desc.ccid);
    MOBILE_INFO("Module Version: %s\n", g_module_desc.module_version);
    MOBILE_INFO("Serial Number: %s\n", g_module_desc.serialnum);
    MOBILE_INFO("Module Serial Number: %s\n", g_module_desc.module_serialnum);
    MOBILE_INFO("RF Info - Net Type: %s\n", g_module_desc.rf.net_type);
    MOBILE_INFO("RF Info - Frequency Band: %s\n", g_module_desc.rf.frequency_band);
    MOBILE_INFO("RF Info - RSRP: %s\n", g_module_desc.rf.rsrp);
    MOBILE_INFO("RF Info - RSSI: %s\n", g_module_desc.rf.rssi);
    MOBILE_INFO("RF Info - SINR: %s\n", g_module_desc.rf.sinr);
    MOBILE_INFO("Frequency Band: %s\n", g_module_desc.frequency_band);
    MOBILE_INFO("SCS: %s\n", g_module_desc.scs);
    MOBILE_INFO("ECI: %s\n", g_module_desc.eci);
    MOBILE_INFO("eNB ID: %s\n", g_module_desc.enb_id);
    MOBILE_INFO("gNB ID: %s\n", g_module_desc.gnb_id);
    MOBILE_INFO("Cell ID: %s\n", g_module_desc.cell_id);
    MOBILE_INFO("PLMN: %s\n", g_module_desc.plmn);
    MOBILE_INFO("Registration Type: %d\n", g_module_desc.regtype);
    MOBILE_INFO("SIM Status: %d\n", g_module_desc.simstatus);
    MOBILE_INFO("CSQ: %d\n", g_module_desc.csq);
    MOBILE_INFO("RSRP: %d\n", g_module_desc.rsrp);
    MOBILE_INFO("Signal Level: %d\n", g_module_desc.signalLevel);
    MOBILE_INFO("Voice Support: %d\n", g_module_desc.voice_support);
    MOBILE_INFO("Voice Mode: %s\n", g_module_desc.voice_mode);
    MOBILE_INFO("\n");
}
