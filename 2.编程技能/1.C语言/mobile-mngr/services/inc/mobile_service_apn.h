#ifndef MOBILE_SERVICE_APN_H
#define MOBILE_SERVICE_APN_H

#include "mobile_module_apn.h"

#define MAX_MULTAPNS_COUNT 0x04

// 自动APN上下文结构体
typedef struct {
    auto_provider_t *list;          // 自动APN提供者列表
    auto_provider_t *current;       // 当前使用的自动APN提供者
    int retry_count;                // 重试计数器
} auto_apn_context_t;

int mobile_init_apn_service(void);
void mobile_deinit_apn_service(void);

int mobile_init_auto_apn(void);
int mobile_init_basic_apn(void);
int mobile_init_multi_apn(void);

void mobile_network_basic_interface_config(void);
void mobile_network_multi_interface_settings(void);
void mobile_fw_config_basic_settings(void);
void mobile_fw_config_multi_setting(void);
void mobile_check_update_basic_config(void);
void mobile_check_update_multi_config(void);
void mobile_check_update_apn(void);

int mobile_get_basic_apn_status(void);
bool mobile_set_apn_mccmnc_num(void);
bool mobile_get_apn_mccmnc(void);
bool mobile_set_apn_mccmnc_str(void);
bool mobile_get_sim_mccmnc(void);

void mobile_init_apn_in_modem(void);
void mobile_check_auto_apn(void);
void mobile_sync_auto_apn(void);

void mobile_print_apn_info(void);
void mobile_print_auto_provider_info(auto_provider_t *provider);
#endif