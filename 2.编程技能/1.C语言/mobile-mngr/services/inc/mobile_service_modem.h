#ifndef MOBILE_SERVICE_MODEM_H
#define MOBILE_SERVICE_MODEM_H

#include "mobile_module_modem.h"

/* 其他常量定义 */
#define DIALD_NONE "NO USB CARD"
#define DIALD_INIT "USB CARD INIT"
#define DIALD_READY "SIM READY"
#define DIALD_REG "NET REG"
#define DIALD_CONNECTING "CONNECTING"
#define DIALD_CONNECTED "CONNECTED"
#define DIALD_DISCONNECTED "DISCONNECTED"

#define MOBILE_BASICWAN_NAME "wan5g"
#define MOBILE_PID_FILE "/config/usb_gadget/g1/idProduct"
#define MOBILE_VID_FILE "/config/usb_gadget/g1/idVendor"

typedef enum _sim_type_t {
   SIM_TYPE_NONE = 0,
   SIM_TYPE_SIM = 1,
   SIM_TYPE_ESIM = 2,
} sim_type_t;

typedef enum _reg_type_t {
   REG_TYPE_NONE = 0,
   REG_TYPE_WCDMA = 2,
   REG_TYPE_LTE = 3,
   REG_TYPE_ENDC = 4,
   REG_TYPE_NR = 5,
} reg_type_t;

/* 模块配置结构体 */
typedef struct {
    int mtu;                           /**< ReadWrite */
    int bus_type;                      /**< ReadWrite */ /*0: default(do nothing); 1: usb; 2: pcie*/
    char username[32];                 /**< ReadWrite */
    char password[32];                 /**< ReadWrite */
    char dial_number[32];              /**< ReadWrite */
    char apn[32];                      /**< ReadWrite */
    char pin_number[32];               /**< ReadWrite */
    char net_type[16];                 /**< ReadWrite */
    char net_sub_type[16];             /**< ReadWrite */
    char auth_type[16];                /**< ReadWrite */
    char ip_version[8];                /**< ReadWrite */
    char ip_ver_num[8];                /**< ReadWrite */
    unsigned char enable;              /**< ReadWrite */
    unsigned char manual_apn_enable;   /**< ReadWrite */
    unsigned char pin_enable;          /**< ReadWrite */
    unsigned char pin_auto_unlock;     /**< ReadWrite */
    unsigned char bridge_enable;       /**< ReadWrite */
    unsigned char nat_enable;          /**< ReadWrite */
    unsigned char delegate;
} module_config_t;

/* 多APN配置结构体 */
typedef struct _module_mutil_config_t{
    struct _module_mutil_config_t *next;
    int cid;              /**< ReadWrite */
    int vlanid;
    int nat_enable;
    int bridge_enable;
    char apn[32];         /**< ReadWrite */
    char username[32];    /**< ReadWrite */
    char password[32];    /**< ReadWrite */
    char iface[16];
    char status[16];      /**< ReadOnly */
    char dial_number[16];  /**< ReadWrite */
    char auth_type[8];     /**< ReadWrite */
    char ip_version[8]; /**< ReadWrite */
    unsigned char enable; /**< ReadWrite */
    unsigned char active; /**< ReadWrite */
    unsigned char delegate;
} module_mutil_config_t;

/* 射频信息结构体 */
typedef struct {
    char global_cell_id[32];         /**< 全局小区ID */
    char downlink_max_thrp[32];      /**< 下行最大吞吐量 */
    char uplink_max_thrp[32];        /**< 上行最大吞吐量 */
    char net_type[16];               /**< 网络类型 */
    char frequency_band[16];         /**< 频段 */
    char duplexing_mode[16];         /**< 双工模式 */
    char transmission_mode[16];      /**< 传输模式 */
    char rsrq[16];                   /**< 参考信号接收质量 */
    char rsrp[16];                   /**< 参考信号接收功率 */
    char rssi[16];                   /**< 接收信号强度指示 */
    char sinr[16];                   /**< 信噪比 */
    char tac[16];                    /**< 跟踪区码 */
    char dl_earfcn[16];              /**< 下行绝对射频信道号 */
    char bandwidth[16];              /**< 带宽 */
    char physical_cell_id[16];       /**< 物理小区ID */
    char cqi[16];                    /**< 信道质量指示 */
    char rank[16];                   /**< 秩指示 */
    char rx_mcs[16];                 /**< 下行MCS */
    char tx_mcs[16];                 /**< 上行MCS */
} rf_info_t;

/* 模块描述结构体扩展 */
typedef struct {
    module_mutil_config_t *mutil_config;
    at_config_t* at_config;
    char* wanstatus;
    char *voice_led;
    unsigned int vid;
    unsigned int pid;
    int imsststus;
    int voice_support;
    reg_type_t regtype;
    int ims;
    int simstatus;
    int csq;                         /**< 信号质量 */
    int rsrp;                        /**< RSRP值 */
    int signalLevel;
    dial_t dial_type;
    char final_nettype;
    char module_name[64];
    char device_name[64];
    char imei[64];
    char imsi[64];
    char ccid[64];
    char module_version[64];
    char serialnum[64];
    char module_serialnum[64];
    char eci[32];                     /**< E-UTRAN小区标识 */
    char enb_id[32];                  /**< eNodeB ID */
    char gnb_id[32];                  /**< gNodeB ID */
    char apn_mccmnc[32];
    char sim_mccmnc[32];
    char frequency_band[16];          /**< 频段 */
    char scs[16];                     /**< 子载波间隔 */
    char cell_id[16];                 /**< 小区ID */
    char ssb_rsrp[16];                /**< SSB参考信号接收功率 */
    char plmn[16];                /**< SSB参考信号接收功率 */
    char voice_mode[8];                /**< ReadWrite */
    module_config_t basic_config;
    rf_info_t rf;                      /**< 主射频信息 */
    rf_info_t rf2;                     /**< 辅射频信息 (NSA模式使用) */
} module_desc_t;

extern module_desc_t g_module_desc;

int mobile_init_modem_service(void);
void mobile_deinit_modem_service(void);

bool mobile_apply_nettype_handler(void);
int mobile_apply_nettype_p7006_v2c7c(void);
void mobile_update_final_nettype(void);

int mobile_init_mobile_dir(void);
int mobile_record_user_pid(int pid, const char* file_path);
int mobile_get_pid_vid(void);
int mobile_init_default_at_command_table(void);
int mobile_init_sim_type(void);
sim_type_t mobile_get_sim_type(void);
void mobile_apply_sim_type(sim_type_t sim_type);
const char* mobile_get_net_device_name(void);
void mobile_update_device_name(void);
int mobile_is_valid_imei_char(const char* str);
int mobile_init_imei(void);
void mobile_init_version(void);
int mobile_init_version_quectel(void);
int mobile_init_serialnum(void);
void mobile_init_imsi(void);
void mobile_init_ccid(void);
int mobile_init_odu_basic(void);
bool mobile_get_provider(void);
int mobile_at_is_ready(void);
int mobile_network_is_registered(bool tag);

void mobile_lte_cellinfo_save(void);
void mobile_nr5g_sa_cellinfo_save(void);
void mobile_nr5g_nsa_cellinfo_save(void);
void mobile_cellinfo_save(void);
int mobile_update_signal_level(void);
void mobile_print_module_desc_info(void);

#endif