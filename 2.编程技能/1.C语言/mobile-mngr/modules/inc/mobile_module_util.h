#ifndef MOBILE_MODULE_UTIL_H
#define MOBILE_MODULE_UTIL_H

#include <stddef.h>
#include <suci.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <dirent.h>
#include <signal.h>
#include <limits.h>
#include <time.h>
#include <fcntl.h>

#include "mobile_module_log.h"

#ifndef DIR_MOBILE_PATH
#define DIR_MOBILE_PATH "/var/mobile"
#endif

#define MOBILEMNGR_PID_FILE                     DIR_MOBILE_PATH "/mobilemngr_pidfile"
#define USB_VID_PID_FILE                        DIR_MOBILE_PATH "/devinfo"
#define DIALD_STATUS_FILE                       DIR_MOBILE_PATH "/diald"
#define IMEI_RESULT_FILE                        DIR_MOBILE_PATH "/imei"
#define MODULE_VERSION_FILE                     DIR_MOBILE_PATH "/lteversion"
#define SIM_STATUS_FILE                         DIR_MOBILE_PATH "/simstatus"
#define IMS_STATUS_FILE                         DIR_MOBILE_PATH "/imsstatus"
#define PINLOCK_STATUS_FILE                     DIR_MOBILE_PATH "/pinlock"
#define BANDLIST_INFOS_FILE                     DIR_MOBILE_PATH "/bandlist"
#define IMSI_RESULT_FILE                        DIR_MOBILE_PATH "/SIMCardIMSI"
#define ICCID_RESULT_FILE                       DIR_MOBILE_PATH "/SIMCardICCID"
#define ODUSN_RESULT_FILE                       DIR_MOBILE_PATH "/ODUSerialNumber"
#define ODUMANUF_RESULT_FILE                    DIR_MOBILE_PATH "/ODUManufacturer"
#define ODUMODEL_RESULT_FILE                    DIR_MOBILE_PATH "/ODUModelName"
#define PROVIDER_MCCMNC_FILE                    DIR_MOBILE_PATH "/providerMCCMNC"
#define SIM_MCCMNC_FILE                         DIR_MOBILE_PATH "/simMCCMNC"
#define DONGLE_DAILCTL_FILE                     DIR_MOBILE_PATH "/ql_mipc_dailctl"
#define NET_IFNAME_FILE                         DIR_MOBILE_PATH "/ifname"
#define PROVIDER_RESULT_FILE                    DIR_MOBILE_PATH "/provider"
#define SIGNAL_LEVEL_FILE                       DIR_MOBILE_PATH "/LedSignalLevel"
#define CELL_NUM_FILE                           DIR_MOBILE_PATH "/cell_num"
#define CELL_LTE_LIST_FILE                      DIR_MOBILE_PATH "/cell_lte_list"
#define CELL_NR_LIST_FILE                       DIR_MOBILE_PATH "/cell_nr_list"
#define CELL_INFO_FILE                          DIR_MOBILE_PATH "/cell_info"
#define CELL_CHANGE_STATUS_FILE                 DIR_MOBILE_PATH "/cell_change_status"
#define NETWORK_TYPE_FILE                       DIR_MOBILE_PATH "/network_type"
#define RSRP_RESULT_FILE                        DIR_MOBILE_PATH "/rsrp_result"
#define LTE_SIGNAL_FILE                         DIR_MOBILE_PATH "/lte_signal"
#define HOME_PAGE_NEED_FILE                     DIR_MOBILE_PATH "/home_page_need"
#define RSSI_STATUS_FILE                        DIR_MOBILE_PATH "/rssi_status"
#define SCAN_CELLLIST_FLG                       DIR_MOBILE_PATH "/scan_selllist_flg"
#define MOBILE_MOBILE_CODES                     DIR_MOBILE_PATH "/mobile_codes"
#define USSD_EXE_FILE                           DIR_MOBILE_PATH "/ussd"
#define SIMLOCK_SUPPORT_FILE                    DIR_MOBILE_PATH "/simlock_support"
#define BANDLOCK_SUPPORT_FILE                   DIR_MOBILE_PATH "/bandlock_support"
#define CELLLOCK_SUPPORT_FILE                   DIR_MOBILE_PATH "/celllock_support"
#define VOLTECS_SUPPORT_FILE                    DIR_MOBILE_PATH "/voltecs_support"
#define CELL_SCAN_SUCCESS_FILE                  DIR_MOBILE_PATH "/cell_scan_success"
#define CELL_SCAN_FAIL_FILE                     DIR_MOBILE_PATH "/cell_scan_fail"
#define WAN_5G_CONNECT_TIME_FILE                DIR_MOBILE_PATH "/wan_5g_connect_time"
#define WAN_STATUS_FILE                         DIR_MOBILE_PATH "/wanstatus"
#define CELL_CHANGE_STATUS_NOTE_WEB_FILE        DIR_MOBILE_PATH "/cell_change_status_note_web"
#define DEFAULT_ETHWAN_PING_FAILOVER            DIR_MOBILE_PATH "/ethwan_ping_failover"
#define DEFAULT_ETHWAN_GATEWAY_IF               DIR_MOBILE_PATH "/ethwangatewayif"
#define DEFAULT_ETHWAN_GATEWAY_IF_UPDATE        DIR_MOBILE_PATH "/ethwangatewayif.update"
#define DEFAULT_MOBILEFAILOVER_CONFIG_UPDATE    DIR_MOBILE_PATH "/mobilefailover.update"
#define PING_RECORD_LASTROUTE                   DIR_MOBILE_PATH "/lastroutes"
#define DEFAULT_MOBILE_VOLTE_FILE               DIR_MOBILE_PATH "/volte.conf"
#define DEFAULT_MOBILE_CONFIG_UPDATE            DIR_MOBILE_PATH "/mobilecfg.update"
#define DEFAULT_MOBILESIMLOCK_CONFIG_UPDATE     DIR_MOBILE_PATH "/mobilesimlock.update"
#define DEFAULT_MOBILEBANDLOCK_CONFIG_UPDATE    DIR_MOBILE_PATH "/mobilebandlock.update"
#define DEFAULT_MOBILEPCIDLOCK_CONFIG_UPDATE    DIR_MOBILE_PATH "/mobilepcidlock.update"
#define DEFAULT_MOBILEMULTIAPN_CONFIG_UPDATE    DIR_MOBILE_PATH "/mobilemultiapns.update"
#define DEFAULT_MOBILEBASICAPN_CONFIG_UPDATE    DIR_MOBILE_PATH "/mobilebasicapn.update"
#define DEFAULT_MOBILEPINLOCK_CONFIG_ENABLE     DIR_MOBILE_PATH "/mobilepinlock.enable"
#define DEFAULT_MOBILEPINLOCK_CONFIG_DISABLE    DIR_MOBILE_PATH "/mobilepinlock.disable"

typedef struct _comm_list_head {
    struct _comm_list_head* next;
    unsigned char data[0];
} comm_list_head_t;

typedef struct inf_status_s {
    int status;
    char uptime[128];
    char ipv6_address[128];
    char ipv6_prefix_address[128];
    char nexthop[128];
    char ipv6_prefix_mask[64];
    char dns[64];
    char l3_device[32];
    char proto[32];
    char device[32];
    char ipv4_address[32];
    char mask[32];
    char ipv6_mask[32];
} inf_status_t;

int mobile_uci_get(char* key, char* value);
int mobile_uci_get_int(char* key, int* value);
int mobile_uci_get_uint(char* key, unsigned int* value);
int mobile_uci_get_option(char* package_name, char* section_name, char* option_name, char* option_value);
int mobile_uci_get_option_int(char* package_name, char* section_name, char* option_name, int* value);
int mobile_uci_set(char* key, char* value);
int mobile_uci_set_int(char* key, int value);
int mobile_uci_set_uint(char* key, unsigned int value);
int mobile_uci_set_option(char* package_name, char* section_name, char* option_name, char* option_value);
int mobile_uci_set_option_int(char* package_name, char* section_name, char* option_name, int option_value);
int mobile_uci_del_option(char* package_name, char* section_name, char* option_value);
int mobile_uci_del(char* key);
int mobile_uci_add(char* package_name, char* section_name, char* option_name);
int mobile_uci_add_list(char* package_name, char* section_name, char* option_name, char* option_value);
int mobile_uci_del_list(char* package_name, char* section_name, char* option_name, char* option_value);
int mobile_uci_get_section_number(char* package, char* section_type);

void mobile_free_whole_comm_list(void** listhead);
void* mobile_malloc_in_comm_list(void** listhead, int size);
void* mobile_malloc_in_comm_list_tail(void **listhead, int size);

int mobile_write_buff_to_file(const char* filename, const char* buf, int len);
int mobile_read_first_line_from_file(char* filename, char* line, int size);

int mobile_system_ex(char* command, int print_flag);
int mobile_at_script_and_get_ret(char* script, char* buf, int len);
int mobile_at_cmd(const char* cmd, char* rbuf, int len);

int mobile_check_valid_char(char* str);
unsigned int mobile_get_uptime_in_ms(void);

// 网络工具函数
unsigned char mobile_get_if_ipv4_addr(const char* dev_name, char* ip);
unsigned char mobile_is_valid_ipv4_address(const char* ip);
unsigned char mobile_get_default_gateway_if_name_in_route_table(char* if_name);

int mobile_get_ifname_default_gw(const char* ifname, char* default_gw, int max_size);

int mobile_hex_char_to_dec(char hex_char);
int mobile_hex_to_bin(const char* hex_str, char* bin_str, size_t bin_size);

void mobile_get_interface_info(char* inf, inf_status_t* wandeviceinfo);
bool mobile_check_interface_up(char *interface);
bool mobile_check_interface_exists(const char* interface);

#endif