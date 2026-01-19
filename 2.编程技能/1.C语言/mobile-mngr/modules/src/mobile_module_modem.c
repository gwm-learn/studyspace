#include "mobile_module_util.h"
#include "mobile_module_modem.h"

/*
 * 文件名称：mobile_module_modem.c
 * 功能描述：
 *     modem基础模块
 */

/**
 * @brief 预定义的AT命令配置数组
 * 替代原来的predefined_at_commands数组，使用清晰的结构化配置
 * 每个配置项都明确填写所有字段，未指定的使用默认值
 */
const at_config_t g_default_at_command_table[] = {
    /* 默认配置 - Android */
    {
        .module_name = "Android",
        .ifname = "",
        .vid = 0,
        .pid = 0,
        .dial_type = DIAL_DHCPC,
        
        /* 默认AT命令 */
        .at_init = "AT+CFUN=1",
        .at_config = "AT+CGDCONT=1,\\\"@ipver@\\\",\\\"@apn@\\\"",
        .at_dial = "AT\\^NDISDUP=1,1,\\\"@apn@\\\",\\\"@username@\\\",\\\"@password@\\\",@authmethod@",
        .at_get_csq = "AT+CSQ?",
        .at_get_pin = "AT+CPIN?",
        .at_get_id = "AT+CGMM",
        .at_get_reg = "AT+CREG?",
        .at_get_reg_lte = "AT+CEREG?",
        .at_get_reg_5g = NULL,
        .at_get_imei = "AT+CGSN",
        .at_sys_atchnet = "AT+COPS?",
        .at_sys_autosettins = "AT+COPS=3,2",
        .at_get_cgreg = "AT+CGREG?",
        .at_test = "AT",
        .at_urc = "AT+QURCCFG=\\\"URCPORT\\\",\\\"USBMODEM\\\"",
        .at_dsci = NULL,
        .at_cmni = NULL,
        .at_sys_nr5g_disable = NULL,
        .at_reset = NULL,
    },
    /* Quectel RG502Q-EA */
    {
        .module_name = "Quectel RG502Q-EA",
        .ifname = PCIE_NET_IFNAME,
        .vid = 0x2c7c,
        .pid = 0x0620,
        .dial_type = DIAL_AUTO_POWERON,
        
        /* 使用默认的AT_INIT */
        .at_init = "AT+CFUN=1",
        /* 自定义AT_CONFIG和AT_DIAL */
        .at_config = "AT+QICSGP=1,@ipver@,\\\"@apn@\\\",\\\"@username@\\\",\\\"@password@\\\",@authmethod@",
        .at_dial = "AT+QICSGP=1,@ipver@,\\\"@apn@\\\",\\\"@username@\\\",\\\"@password@\\\",@authmethod@",
        /* 使用默认的查询命令 */
        .at_get_csq = "AT+CSQ?",
        .at_get_pin = "AT+CPIN?",
        .at_get_id = "AT+CGMM",
        .at_get_reg = "AT+CREG?",
        .at_get_reg_lte = "AT+CEREG?",
        /* 自定义5G注册查询 */
        .at_get_reg_5g = "AT+C5GREG?",
        .at_get_imei = "AT+CGSN",
        .at_sys_atchnet = "AT+COPS?",
        .at_sys_autosettins = "AT+COPS=3,2",
        .at_get_cgreg = "AT+CGREG?",
        .at_test = "AT",
        .at_urc = "AT+QURCCFG=\\\"URCPORT\\\",\\\"USBMODEM\\\"",
        .at_dsci = NULL,
        .at_cmni = NULL,
        /* 自定义5G禁用命令 */
        .at_sys_nr5g_disable = "at+QNWPREFCFG=\\\"nr5g_disable_mode\\\",1",
        /* 自定义重置命令 */
        .at_reset = "AT+CFUN=1,1",
    },
    
    /* Quectel RG500L */
    {
        .module_name = "Quectel RG500L",
        .ifname = CCMNI_NET_IFNAME,
        .vid = 0x2c7c,
        .pid = 0x7003,
        .dial_type = DIAL_AUTO_POWERON,
        
        /* 使用默认的AT_INIT */
        .at_init = "AT+CFUN=1",
        /* 自定义AT_CONFIG和AT_DIAL */
        .at_config = "AT+QICSGP=1,@ipver@,\\\"@apn@\\\",\\\"@username@\\\",\\\"@password@\\\",@authmethod@",
        .at_dial = "AT+QICSGP=1,@ipver@,\\\"@apn@\\\",\\\"@username@\\\",\\\"@password@\\\",@authmethod@",
        /* 使用默认的查询命令 */
        .at_get_csq = "AT+CSQ?",
        .at_get_pin = "AT+CPIN?",
        .at_get_id = "AT+CGMM",
        .at_get_reg = "AT+CREG?",
        .at_get_reg_lte = "AT+CEREG?",
        /* 自定义5G注册查询 */
        .at_get_reg_5g = "AT+EgREG?",
        .at_get_imei = "AT+CGSN",
        .at_sys_atchnet = "AT+COPS?",
        .at_sys_autosettins = "AT+COPS=3,2",
        .at_get_cgreg = "AT+CGREG?",
        .at_test = "AT",
        .at_urc = "AT+QURCCFG=\\\"URCPORT\\\",\\\"USBMODEM\\\"",
        /* 自定义DSCI和CMNI命令 */
        .at_dsci = "AT+EIMSCFG=1,0,0,0,1,1",
        .at_cmni = "at+cnmi=2,1,0,0,0",
        /* 自定义5G禁用命令 */
        .at_sys_nr5g_disable = "at+e5gopt=5",
        /* 自定义重置命令 */
        .at_reset = "at;reboot",
    },
    
    /* Quectel RG620T-EU */
    {
#ifdef CUS_PARAMS_PRODUCT_TYPE_AX3000
        .module_name = "Quectel RG620T-EU",
#else
        .module_name = "Quectel RG620T-EU",
#endif
        .ifname = CCMNI_NET_IFNAME,
        .vid = 0x2c7c,
        .pid = 0x7006,
        .dial_type = DIAL_AUTO_POWERON,
        
        /* 使用默认的AT_INIT */
        .at_init = "AT+CFUN=1",
        /* 自定义AT_CONFIG和AT_DIAL */
        .at_config = "AT+QICSGP=1,@ipver@,\\\"@apn@\\\",\\\"@username@\\\",\\\"@password@\\\",@authmethod@",
        .at_dial = "AT+QICSGP=1,@ipver@,\\\"@apn@\\\",\\\"@username@\\\",\\\"@password@\\\",@authmethod@",
        /* 使用默认的查询命令 */
        .at_get_csq = "AT+CSQ?",
        .at_get_pin = "AT+CPIN?",
        .at_get_id = "AT+CGMM",
        .at_get_reg = "AT+CREG?",
        .at_get_reg_lte = "AT+CEREG?",
        /* 自定义5G注册查询 */
        .at_get_reg_5g = "AT+EgREG?",
        .at_get_imei = "AT+CGSN",
        .at_sys_atchnet = "AT+COPS?",
        .at_sys_autosettins = "AT+COPS=3,2",
        .at_get_cgreg = "AT+CGREG?",
        .at_test = "AT",
        .at_urc = "AT+QURCCFG=\\\"URCPORT\\\",\\\"USBMODEM\\\"",
        /* 自定义DSCI和CMNI命令 */
        .at_dsci = "AT+EIMSCFG=1,0,0,0,1,1",
        .at_cmni = "at+cnmi=2,1,0,0,0",
        /* 自定义5G禁用命令 */
        .at_sys_nr5g_disable = "at+e5gopt=5",
        /* 自定义重置命令 */
        .at_reset = "at;reboot",
    },
};

/**
 * @brief 根据PID和VID初始化AT命令配置
 *
 * 该函数通过对比传入的PID和VID与预定义的AT命令配置列表进行匹配，
 * 找到对应的AT命令配置信息，并将匹配的配置信息地址赋给输出参数。
 *
 * @param pid 产品ID (Product ID)
 * @param vid 厂商ID (Vendor ID)
 * @param at_config 输出参数，用于接收匹配的AT命令配置信息的地址
 *
 * @return int 返回匹配结果
 *         - 0: 匹配成功
 *         - -1: 未找到匹配的配置
 */
int mobile_init_at_config(unsigned int pid, unsigned int vid, const at_config_t **at_config) {
    const int g_default_at_command_table_count = sizeof(g_default_at_command_table) / sizeof(at_config_t);
    // 遍历预定义的AT命令配置列表，查找匹配的PID和VID
    for (int i = 0; i < g_default_at_command_table_count; i++) {
        if (g_default_at_command_table[i].vid == vid && g_default_at_command_table[i].pid == pid) {
            // 找到匹配的配置，将配置信息地址赋给输出参数
            *at_config = &g_default_at_command_table[i];
            return 0; // 匹配成功
        }
    }
    
    return -1; // 未找到匹配的配置
}
