#ifndef MOBILE_MODULE_MODEM_H
#define MOBILE_MODULE_MODEM_H

#ifdef SUPPORT_EXTERN_MOBILE_WAN
#define USB_NET_IFNAME "usb_ext"
#define PCIE_NET_IFNAME "rmnet_mhi_ext"
#else
#define USB_NET_IFNAME "usb0"
#define PCIE_NET_IFNAME "rmnet_mhi0.1"
#endif

#define CCMNI_NET_IFNAME "ccmniXX"
#define CCMNI_NET_IFNAME_PRE "ccmni"

typedef enum {
    DIAL_CM = 0,
    DIAL_DHCPC,
    DIAL_PPP,
    DIAL_NCM,
    DIAL_CM_COM,
    DIAL_AUTO_POWERON,
    DIAL_NONE,
} dial_t;

/**
 * @brief AT命令配置结构体
 * 用于定义每个模块的AT命令配置，替代原来的AT和ATS宏
 */
typedef struct {
    const char* module_name;        /**< 模块名称 */
    const char* ifname;             /**< 接口名称 */
    unsigned int vid;               /**< 厂商ID */
    unsigned int pid;               /**< 产品ID */
    dial_t dial_type;               /**< 拨号类型 */
    /* AT命令配置 - 使用明确的字段名替代数组索引 */
    const char* at_init;            /**< AT_INIT 命令 */
    const char* at_config;          /**< AT_CONFIG 命令 */
    const char* at_dial;            /**< AT_DIAL 命令 */
    const char* at_get_csq;         /**< AT_GET_CSQ 命令 */
    const char* at_get_pin;         /**< AT_GET_PIN 命令 */
    const char* at_get_id;          /**< AT_GET_ID 命令 */
    const char* at_get_reg;         /**< AT_GET_REG 命令 */
    const char* at_get_reg_lte;     /**< AT_GET_REG_LTE 命令 */
    const char* at_get_reg_5g;      /**< AT_GET_REG_5G 命令 */
    const char* at_get_imei;        /**< AT_GET_IMEI 命令 */
    const char* at_sys_atchnet;     /**< AT_SYS_ATCHNET 命令 */
    const char* at_sys_autosettins; /**< AT_SYS_AUTOSETTINS 命令 */
    const char* at_get_cgreg;       /**< AT_GET_CGREG 命令 */
    const char* at_test;            /**< AT_TEST 命令 */
    const char* at_urc;             /**< AT_URC 命令 */
    const char* at_dsci;            /**< AT_DSCI 命令 */
    const char* at_cmni;            /**< AT_CMNI 命令 */
    const char* at_sys_nr5g_disable;/**< AT_SYS_NR5G_DISABLE 命令 */
    const char* at_reset;           /**< AT_RESET 命令 */
} at_config_t;

int mobile_init_at_config(unsigned int pid, unsigned int vid, const at_config_t **at_config);

#endif /* MOBILE_MODULE_INFO_H */