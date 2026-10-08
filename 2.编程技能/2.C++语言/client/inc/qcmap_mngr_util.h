#ifndef _QCMAP_MNGR_UTIL_H_
#define _QCMAP_MNGR_UTIL_H_

#include <string>
#ifndef FEATURE_EXTERNAL_AP
#include "comdef.h"
#endif /*FEATURE_EXTERNAL_AP */
#include "QCMAP_Client.h"
#include "limits.h"
#include "ds_util.h"
#include "QCMAP_LAN_Client.h"
#include "QCMAP_LAN_Multimedia.h"
#include <algorithm>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "uci.h"
#include "uci_internal.h"
#include <sys/stat.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#define MOD_STATUS_UPDATE_INTERVAL 5000 // unit: ms
#define QCMAP_MNGR_DEFAULT_PROFILE_NUM 3
#define QCMAP_MNGR_MULTI_APN_NUM 2
#define INET_ADDRSTRLEN 16
#define INET6_ADDRSTRLEN 46
#define MAX_BACKHAUL_TYPE_LENGTH 128
#define RE_BUF_LEN 1024
#define BUFLEN_512 512
#define BUFLEN_256 256
#define BUFLEN_128 128
#define BUFLEN_64 64
#define BUFLEN_32 32
#define BUFLEN_16 16
#define BUFLEN_8 8
#define STR_LEN_16 16

#define QCMAP_MNGR_AT_MNGR_COMMAND      "at-mngr"
#define QCMAP_MNGR_AT_COMMAND_AT        "AT"
#define QCMAP_MNGR_AT_COMMAND_CFUN      "AT+CFUN"
#define QCMAP_MNGR_AT_COMMAND_CPIN      "AT+CPIN"
#define QCMAP_MNGR_AT_COMMAND_CGDCONT   "AT+CGDCONT"
#define QCMAP_MNGR_AT_COMMAND_CGAUTH    "AT+CGAUTH"
#define QCMAP_MNGR_AT_COMMAND_COPS      "AT+COPS"
#define QCMAP_MNGR_AT_COMMAND_GTRAT     "AT+GTRAT"
#define QCMAP_MNGR_AT_COMMAND_PLMNLOCK  "AT+GTPLMNLOCK"
#define QCMAP_MNGR_AT_COMMAND_CELLLOCK  "AT+GTCELLLOCK"
#define QCMAP_MNGR_AT_COMMAND_CFSN      "AT+CFSN"
#define QCMAP_MNGR_AT_COMMAND_CREG      "AT+CREG"
#define QCMAP_MNGR_AT_COMMAND_CEREG     "AT+CEREG"
#define QCMAP_MNGR_AT_COMMAND_C5GREG    "AT+C5GREG"
#define QCMAP_MNGR_AT_COMMAND_CAVIMS    "AT+CAVIMS"

#define QCMAP_MNGR_DEFAULT_WAN_NAME    "wan5g"
#define QCMAP_MNGR_DEFAULT_CONFIG_FILE "network"
#define QCMAP_MNGR_MOBILE_CONFIG_FILE  "mobile"

#define QCMAP_MNGR_CONFIG_DISABLE           "disabled"
#define QCMAP_MNGR_CONFIG_PROFILE           "profile"
#define QCMAP_MNGR_CONFIG_IPTYPE            "iptype"
#define QCMAP_MNGR_CONFIG_APNNAME           "apn"
#define QCMAP_MNGR_CONFIG_USERNAME          "username"
#define QCMAP_MNGR_CONFIG_PASSWORD          "password"
#define QCMAP_MNGR_CONFIG_AUTH              "auth"
#define QCMAP_MNGR_CONFIG_MANUALAPN         "manualApn"
#define QCMAP_MNGR_CONFIG_NETTYPE           "nettype"
#define QCMAP_MNGR_CONFIG_SUBNETTYPE        "subnettype"
#define QCMAP_MNGR_CONFIG_PINAUTOLOCK       "pinAutoUnlock"
#define QCMAP_MNGR_CONFIG_PINNUMBER         "pinNumber"
#define QCMAP_MNGR_CONFIG_PINENABLE         "pinEnable"
#define QCMAP_MNGR_CONFIG_NATENABLE         "natEnable"
#define QCMAP_MNGR_CONFIG_BRIDGEENABLE      "bridgeEnable"
#define QCMAP_MNGR_CONFIG_DEFAULTROUTE      "defaultroute"
#define QCMAP_MNGR_CONFIG_LOCKMTU           "lockmtu"

#define DIALD_CONNECTING          "CONNECTING"
#define DIALD_CONNECTED           "CONNECTED"
#define DIALD_DISCONNECTED        "DISCONNECTED"

#define QCMAP_MNGR_PID_FILE "/sys/kernel/config/usb_gadget/g1/idProduct"
#define QCMAP_MNGR_VID_FILE "/sys/kernel/config/usb_gadget/g1/idVendor"

#define DIR_MOBILE_PATH           "/var/mobile"
#define USB_VID_PID_FILE          DIR_MOBILE_PATH "/devinfo"
#define NETWORK_TYPE_FILE         DIR_MOBILE_PATH "/network_type"
#define RSRP_RESULT_FILE          DIR_MOBILE_PATH "/rsrp"
#define RSSI_STATUS_FILE          DIR_MOBILE_PATH "/rssi"
#define CELL_INFO_FILE            DIR_MOBILE_PATH "/cellinfo"
#define WAN_STATUS_FILE           DIR_MOBILE_PATH "/wanstatus"
#define WAN_5G_CONNECT_TIME_FILE  DIR_MOBILE_PATH "/wan_5g_connect_time"
#define MODULE_VERSION_FILE       DIR_MOBILE_PATH "/lteversion"
#define SIM_MCCMNC_FILE           DIR_MOBILE_PATH "/simMCCMNC"
#define PROVIDER_MCCMNC_FILE      DIR_MOBILE_PATH "/providerMCCMNC"
#define IMSI_RESULT_FILE          DIR_MOBILE_PATH "/SIMCardIMSI"
#define PROVIDER_RESULT_FILE      DIR_MOBILE_PATH "/provider"
#define IMEI_RESULT_FILE          DIR_MOBILE_PATH "/imei"
#define CELL_NUM_FILE             DIR_MOBILE_PATH "/cellnum"
#define CELL_LTE_LIST_FILE        DIR_MOBILE_PATH "/cell_lte_list"
#define CELL_NR_LIST_FILE         DIR_MOBILE_PATH "/cell_nr_list"
#define SCAN_CELLLIST_FLG         DIR_MOBILE_PATH "/scan_selllist_flg"
#define SIM_STATUS_FILE           DIR_MOBILE_PATH "/simstatus"
#define PINLOCK_STATUS_FILE       DIR_MOBILE_PATH "/pinlock"
#define SIMLOCK_SUPPORT_FILE      DIR_MOBILE_PATH "/simlock_support"
#define BANDLOCK_SUPPORT_FILE     DIR_MOBILE_PATH "/bandlock_support"
#define CELLLOCK_SUPPORT_FILE     DIR_MOBILE_PATH "/celllock_support"
#define BANDLIST_INFOS_FILE       DIR_MOBILE_PATH "/bandlist"
#define SIGNAL_LEVEL_FILE         DIR_MOBILE_PATH "/LedSignalLevel"
#define AUTOAPN_INFOS_FILE        DIR_MOBILE_PATH "/autoapninfos"
#define ICCID_RESULT_FILE         DIR_MOBILE_PATH "/SIMCardICCID"
#define ODUMANUF_RESULT_FILE      DIR_MOBILE_PATH "/ODUManufacturer"
#define ODUMODEL_RESULT_FILE      DIR_MOBILE_PATH "/ODUModelName"
#define ODUSN_RESULT_FILE         DIR_MOBILE_PATH "/ODUSerialNumber"
#define NSA_SIGNAL_FILE           DIR_MOBILE_PATH "/nsa_signal"
#define RFPARAM_INFOS_FILE        DIR_MOBILE_PATH "/rfinfos"
#define MORE_RF_PARA_FILE         DIR_MOBILE_PATH "/otherRFpara"
#define QCMAP_BOOT_FILE           DIR_MOBILE_PATH "/qcmap_mngr_boot"

#define QCMAP_MNGR_DIAL_END_FROM_SIGNAL "/tmp/dial_end_from_signal"
#define QCMAP_MNGR_NETWORK_WAN5G_UPDATE DIR_MOBILE_PATH"/qcmap_mngr_network_wan5g"
#define QCMAP_MNGR_NETWORK_WAN5G_NAT    DIR_MOBILE_PATH"/qcmap_mngr_network_nat"
#define QCMAP_MNGR_NETWORK_WAN5G_BRIDGE DIR_MOBILE_PATH"/qcmap_mngr_network_bridge"
#define QCMAP_MNGR_DIAL_CONNECT_IPV4    DIR_MOBILE_PATH"/dial_connect_ipv4"
#define QCMAP_MNGR_DIAL_CONNECT_IPV6    DIR_MOBILE_PATH"/dial_connect_ipv6"
#define QCMAP_MNGR_DIAL_DISCONNECT_IPV4 DIR_MOBILE_PATH"/dial_disconnect_ipv4"
#define QCMAP_MNGR_DIAL_DISCONNECT_IPV6 DIR_MOBILE_PATH"/dial_disconnect_ipv6"

#define DEFAULT_MOBILEBANDLOCK_CONFIG_UPDATE DIR_MOBILE_PATH "/mobilebandlock.update"
#define DEFAULT_MOBILESIMLOCK_CONFIG_UPDATE  DIR_MOBILE_PATH "/mobilesimlock.update"
#define DEFAULT_MOBILEPCIDLOCK_CONFIG_UPDATE DIR_MOBILE_PATH "/mobilepcidlock.update"

#define QCMAP_MNGR_MODEL_NAME   "FG190W"
#define QCMAP_MNGR_MANUFACTURER "Fibocom"

#define POS_DIFF_VAL(a, b)       ((a > b) ? (a - b) : (b - a))

#define PRINT_DEBUG(fmt, args...)                                                    \
    do {                                                                             \
        printf("Function:[%s][%d] ", __FUNCTION__, __LINE__);               \
        printf(fmt, ##args);                                                         \
    } while (0)

#define SHOW_MANDATORY_RESPONSE_TO_USER(userText, param) \
    { printf("   " #userText " : %d\n", param); }

#define SHOW_OPTIONAL_RESPONSE_TO_USER(userText, param) \
    {                                                   \
        if (param##_valid)                              \
            printf("   " #userText " : %d\n", param);   \
    }

#define SHOW_OPTIONAL_RESPONSE_LIST_TO_USER(userText, param)      \
    {                                                             \
        if (param##_valid) {                                      \
            printf("   " #userText, " Length=%d\n", param##_len); \
            for (int i = 0; i < param##_len; i++) {               \
                printf("      Item[%d]=%d\n", i, param[i]);       \
            }                                                     \
        }                                                         \
    }

#define GET_STR_MOBILE_WAN_INFOS_FROM_BUFFLINE(bufLine, matchStr, outData)                                                                                     \
{                                                                                                                                                          \
    if (!strncmp(bufLine, matchStr, sizeof(matchStr) - 1)) {                                                                                               \
        char *pHead = strchr(bufLine, '=');                                                                                                                \
        if (pHead) {                                                                                                                                       \
            char *pTail = &pHead[strlen(pHead) - 1];                                                                                                       \
            if (pTail[0] == '\r' || pTail[0] == '\n') {                                                                                                    \
                pTail[0] = 0;                                                                                                                              \
            }                                                                                                                                              \
            pHead++;                                                                                                                                       \
            strncpy(outData, pHead, sizeof(outData) - 1);                                                                                                  \
        }                                                                                                                                                  \
    }                                                                                                                                                      \
}

#define WWAN_TECH_TYPE(type) ((type == QCMAP_MSGR_MASK_TECH_PREF_ANY_V01) ? "ANY" : ((type == QCMAP_MSGR_MASK_TECH_PREF_3GPP_V01) ? "3gpp" : "3gpp2"))

#define IP_FAMILY_TYPE(ip_family)  (ip_family == QCMAP_MSGR_IP_FAMILY_V4_V01)      ? "V4"    : \
                                      (ip_family == QCMAP_MSGR_IP_FAMILY_V6_V01)   ? "V6"    : \
                                      (ip_family == QCMAP_MSGR_IP_FAMILY_V4V6_V01) ? "V4V6"  : \
                                      (ip_family == QCMAP_MSGR_IP_FAMILY_ETH_V01)  ? "ETH"   : \
                                                                                     "Unknown"

#define IP_FAMILY_TYPE_STR(ip_family)   (ip_family == 4)   ? "IP"    : \
                                        (ip_family == 6)   ? "IPV6"    : \
                                        (ip_family == 10) ? "IPV4V6"  : "IPV4V6"

#define SUBSCRIPTION_TYPE(subs_id) (subs_id == QCMAP_MSGR_DEFAULT_SUBS_V01) ? "Default" : (subs_id == QCMAP_MSGR_PRIMARY_SUBS_V01) ? "Primary" : (subs_id == QCMAP_MSGR_SECONDARY_SUBS_V01) ? "Secondary" : "Unknown"
#define CONVERT_IPV4_NETWORK_ADDRESS_TO_STRING(ipv4_nw_address, ipv4_str)     \
    {                                                                         \
        struct sockaddr_in ipv4_addr;                                         \
        ipv4_addr.sin_addr.s_addr = ipv4_nw_address;                          \
        inet_ntop(AF_INET, &(ipv4_addr.sin_addr), ipv4_str, INET_ADDRSTRLEN); \
    }
#define CONVERT_IPV6_NETWORK_ADDRESS_TO_STRING(ipv6_nw_address, ipv6_str)                          \
    {                                                                                              \
        struct sockaddr_in6 ipv6_addr;                                                             \
        memcpy(ipv6_addr.sin6_addr.s6_addr, ipv6_nw_address, sizeof(ipv6_addr.sin6_addr.s6_addr)); \
        inet_ntop(AF_INET6, &(ipv6_addr.sin6_addr), ipv6_str, INET6_ADDRSTRLEN);                   \
    }
/* Print Backhaul WWAN Details */
#define PRINT_BACKHAUL_WWAN_DETAILS(wwan_ind_msg)                                                  \
    {                                                                                              \
        char addr_str[INET6_ADDRSTRLEN];                                                           \
        printf("\n   Interface Name                 : %s\n", wwan_ind_msg.wwan_info.iface_name);   \
        CONVERT_IPV4_NETWORK_ADDRESS_TO_STRING(wwan_ind_msg.wwan_info.v4_addr, addr_str);          \
        printf("   Public IPv4 Address            : %s\n", addr_str);                              \
        CONVERT_IPV4_NETWORK_ADDRESS_TO_STRING(wwan_ind_msg.wwan_info.v4_prim_dns_addr, addr_str); \
        printf("   Primary DNS IPv4 Address       : %s\n", addr_str);                              \
        CONVERT_IPV4_NETWORK_ADDRESS_TO_STRING(wwan_ind_msg.wwan_info.v4_sec_dns_addr, addr_str);  \
        printf("   Secondary DNS IPv4 Address    : %s\n", addr_str);                               \
        CONVERT_IPV6_NETWORK_ADDRESS_TO_STRING(wwan_ind_msg.wwan_info.v6_addr, addr_str);          \
        printf("   Public IPv6 Address            : %s\n", addr_str);                              \
        CONVERT_IPV6_NETWORK_ADDRESS_TO_STRING(wwan_ind_msg.wwan_info.v6_prim_dns_addr, addr_str); \
        printf("   Primary DNS IPv6 Address       : %s\n", addr_str);                              \
        CONVERT_IPV6_NETWORK_ADDRESS_TO_STRING(wwan_ind_msg.wwan_info.v6_sec_dns_addr, addr_str);  \
        printf("   Secondary DNS IPv6 Address    : %s\n\n", addr_str);                             \
    }

#define RETRY_UNTIL_SUCCESS(func, retries, interval, ...) ({ \
    bool _success = false; \
    for (int _attempt = 0; !_success && _attempt < (retries); _attempt++) { \
        _success = func(__VA_ARGS__); \
        if (!_success && _attempt != (retries) - 1) { \
            sleep(interval); \
        } \
    } \
    _success; \
})

#define RETRY_TIMES 1
#define SLEEP_TIME  0

enum {
        UCI_LOOKUP_DONE =     (1 << 0),
        UCI_LOOKUP_COMPLETE = (1 << 1),
        UCI_LOOKUP_EXTENDED = (1 << 2),
} uci_flags;

typedef struct {
    bool enable;
    bool update;
    bool bridge_enable; //todo 待完成桥接功能
    bool nat_enable;//只有主apn的配置有效，控制所有apn接口一起nat与否
    bool manaul_apn; //用于主apn
    bool defaultroute;//同时只能一个值为1，其余必须为0
    bool lockmtu;
    int profile_index;
    int ip_family;//IPV4-4 IPV6-6 IPV4V6-10
    qcmap_msgr_wwan_call_type_v01 call_type;    //1-IPV4; 2-IPV6; 3-IPV4V6; 4-V2X; 5-ETH
    int auth;// 0-NONE; 1-PAP; 2-CHAP; 3-PAP+CHAP
    int dial_lose_count_v4;
    int dial_lose_count_v6;
    char nettype_num[16];//保存AT+GTRAT对应的数值参数
    char apn_name[32];
    char username[32];
    char password[32];
    char nettype[32];
    char subnettype[32];
} qcmap_wan5g_config_t;

typedef struct _ST_MODULE_MobileRFparameter {
    char netType[16];
    char duplexingMode[8];             /**< ReadOnly */
    char PLMN[8];                      /**< ReadOnly */
    char CQI[8];                       /**< ReadOnly */
    char rxMCS[8];                     /**< ReadOnly */
    char txMCS[8];                     /**< ReadOnly */
    char dltput[8];                    /**< ReadOnly */
    char ultput[8];                    /**< ReadOnly */
    char txPower[8];                   /**< ReadOnly */
    char RANK[8];                      /**< ReadOnly */
    char SINR[8];                      /**< ReadOnly */
    char RSRQ[8];                      /**< ReadOnly */
    char RSRP[8];                      /**< ReadOnly */
    char RSSI[8];                      /**< ReadOnly */
    char TAC[8];                       /**< ReadOnly */
    char physicalCellID[8];            /**< ReadOnly */
    char globalCellID[16];             /**< ReadOnly */
    char DLEARFCN[8];                  /**< ReadOnly */
    char transmissionMode[8];          /**< ReadOnly */
    char frequencyBand[8];             /**< ReadOnly */
    char bandWidth[8];                 /**< ReadOnly */
    char CAActive;                     /**< ReadOnly */
    char CAInfo[8];                    /**< ReadOnly */
    char currentDownstreamRate[32];    /**< ReadOnly */
    char currentUpstreamRate[32];      /**< ReadOnly */
    char downlinkMaxThrp[32];          /**< ReadOnly */
    char uplinkMaxThrp[32];            /**< ReadOnly */
    char txPowerPPusch[8];             /**< ReadOnly */
    char txPowerPPucch[8];             /**< ReadOnly */
    char txPowerPSrs[8];               /**< ReadOnly */
    char txPowerPPrach[8];             /**< ReadOnly */
    char ul_grant[8];                  /**< ReadOnly */
    char dl_grant[8];                  /**< ReadOnly */
    char ul_bler[8];                   /**< ReadOnly */
    char dl_bler[8];                   /**< ReadOnly */
    char neighbourcell_pci_list[128];  /**< ReadOnly */
    char neighbourcell_rsrq_list[128]; /**< ReadOnly */
    char neighbourcell_rsrp_list[128]; /**< ReadOnly */

} ST_MODULE_MobileRFparameter;

typedef struct _ST_MODULE_MobileRFparameterEx {
    char netType[16];
    char duplexingMode[8];    /**< ReadOnly */
    char PLMN[8];             /**< ReadOnly */
    char CQI[8];              /**< ReadOnly */
    char rxMCS[8];            /**< ReadOnly */
    char txMCS[8];            /**< ReadOnly */
    char txPower[8];          /**< ReadOnly */
    char RANK[8];             /**< ReadOnly */
    char SINR[8];             /**< ReadOnly */
    char RSRQ[8];             /**< ReadOnly */
    char RSRP[8];             /**< ReadOnly */
    char RSSI[8];             /**< ReadOnly */
    char physicalCellID[8];   /**< ReadOnly */
    char globalCellID[16];    /**< ReadOnly */
    char DLEARFCN[8];         /**< ReadOnly */
    char transmissionMode[8]; /**< ReadOnly */
    char frequencyBand[8];    /**< ReadOnly */
    char CAActive;            /**< ReadOnly */
    char CAInfo[8];           /**< ReadOnly */
} ST_MODULE_MobileRFparameterEx;

typedef struct _ST_MODULE_More_rf {
    char dlrate[32];
    char ulrate[32];
} ST_MODULE_More_rf;

typedef struct _ST_MODULE_DESC {
    int         csq;
    int         rsrp;
    float       rsrq;
    float       sinr;
    uint32_t    wanipv4addr;
    const char *wanstatus;

    unsigned int vid;
    unsigned int pid;

    char        apn_mccmnc[16];
    char        sim_mccmnc[16];
    char        frequencyBand[32];
    char        imsi[32];
    char        moduleVersion[64];
    char        imei[64];
    char        ccid[64];
    char        wan_addr6[128];

    // module RFparameter
    ST_MODULE_MobileRFparameter   rf;
    ST_MODULE_MobileRFparameterEx rf2;
    ST_MODULE_More_rf rf3;
} ST_MODULE_DESC;

enum {
    CFUN_0 = 0,
    CFUN_1 = 1,
    CFUN_4 = 4,
} cfun_status;

enum {
    STATE_MACHINE_NONE = 0,
    STATE_MACHINE_AT_READY,
    STATE_MACHINE_SIM_READY,
    STATE_MACHINE_NETWORK_REGISTED,
} state_machine_status;

/**
 * @brief CFUN set state machine states
 */
typedef enum {
    CFUN_STATE_INIT = 0,      // Initial state
    CFUN_STATE_SET_VALUE = 1,  // Set specified value state
    CFUN_STATE_SET_CFUN1 = 2,  // Set CFUN=1 state
} cfun_state_t;

struct cwmp_message {
    int msg_type;
    int msg_datatype;
    void*   msg_data;
};

enum { MSG_ACTIVE_NOTIFY = 1 };

#define MSG_SIZE (sizeof(struct cwmp_message) - sizeof(int))

extern int g_apn_num;
extern int g_max_apn_num;
//extern int g_apn_auth_num;
extern int global_state_machine_status;
extern QCMAP_Client *qcmap_client;
extern unsigned long indication_mask;
extern qcmap_wan5g_config_t qcmap_wan5g_list[QCMAP_MNGR_DEFAULT_PROFILE_NUM];/* main apn + 2 multi apn*/

void qcmap_release_client(void);
boolean qcmap_get_state_machine_running_status(void);
void qcmap_enable_state_machine(void);
void qcmap_disable_state_machine(void);
void qcmap_update_state_machine_status(int status);
int qcmap_get_state_machine_status(void);
void qcmap_init_cfun(bool reload);
void qcmap_execute_cfun_sequence(int value);
bool qcmap_update_cfun(int value);
bool qcmap_check_cfun(int expected_value);
unsigned int qcmap_get_uptime_in_ms(void);
void qcmap_init_info_file(void);
void qcmap_release_util(void);
boolean qcmap_get_bridge_enable(int index);
void qcmap_set_bridge_enable(int index, bool enable);
int read_interface_info_v6(const char *interface, int *ifindex, char *addr6, uint8_t *arp);
int read_interface_info_v4(const char *interface, int *ifindex, uint32_t *addr, uint8_t *arp);
char *hex_str_to_dec_str(const char *hex_str);
int is_valid_char(char *str);
int system_ex(char *command, int printFlag);
bool execute_cmd(const char *cmd, char *result, uint8_t result_len);
bool qcmap_get_uci_value(char* package, char* section, char* name, char* value, int value_len);
int qcmap_set_uci_value(char* package, char* section, char* name, char* value);
int qcmap_uci_addlist(const char *name, const char *value);
int qcmap_uci_dellist(const char *name, const char *value);
int read_first_line_from_file(char *fileName, char *line, int lineSize);
int write_buf_to_file(const char *filename, const char *buf, int len);
int split_string_ext(char *rule_str, char ge, char **array, int max);
int split_string(const char *src, const char *delim, char dest[][STR_LEN_16], int max);
void get_ip_from_route(const char* interface, const char* ip, char *gateway);
void get_default_route(char *addr);
bool check_default_route(void);
bool qcmap_check_ipv4_address(const char *addr);
void print_mtpe_entry(mtpe_history_entry* entry);

void qcmap_msgr_qmi_qcmap_ind(qmi_client_type user_handle, unsigned int msg_id, void* ind_buf, unsigned int ind_buf_len, void* ind_cb_data);

boolean ProcessMTPEHistoryIndication(qcmap_msgr_mtpe_history_ind_msg_v01* ind_data, mtpe_history_entry* in_mtpe_history_entries, uint32_t* len_mtpe_history, uint32 txn_id, boolean* mtpe_history_ready);

boolean convert_backhaul_enum_to_string(qcmap_msgr_backhaul_type_enum_v01 backhaul_type, char* backhaul_type_string);

void DisplayClientInfo(qcmap_msgr_packet_stats_status_ind_msg_v01 ind_data);

void qcmap_send_cwmp_notify(void);

#endif