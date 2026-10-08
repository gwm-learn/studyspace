#ifndef _QCMAP_MNGR_CONFIG_H_
#define _QCMAP_MNGR_CONFIG_H_

#include "qcmap_mngr_util.h"

#define QCMAP_MNGR_DIAL_TIME (10 * 1000)

typedef enum
{
    AUTH_TYPE_NONE = 0,
    AUTH_TYPE_PAP = 1,
    AUTH_TYPE_CHAP = 2,
    AUTH_TYPE_PAP_CHAP = 3,
} auth_type;

typedef enum
{
    WORK_TYPE_NONE = 0,
    WORK_TYPE_BASIC = 1,
    WORK_TYPE_MULTI = 2,
    WORK_TYPE_ALL = 3,
} work_type_t;

extern work_type_t work_type;
extern bool update_firewall;

bool qcmap_get_update_firewall(void);
void qcmap_set_update_firewall(bool update);
void qcmap_check_dial_connect(int profile_index, int config_index);
void qcmap_check_dial_disconnect(int profile_index);
void qcmap_release_config(void);
bool qcmap_get_profile_info(qcmap_net_profile_and_policy_info *profile_info, int profile_index);
void qcmap_show_profile(int profile_index);
bool qcmap_show_all_profile(void);
bool qcmap_get_profile_ip_family(int *ip_family, int config_index);
int qcmap_auth_str_to_int(char *auth);
void qcmap_get_wan5g_config(void);
int qcmap_get_call_type(int config_index);
void qcmap_update_interface(int config_index);
void qcmap_update_profile_index(int profile_index, int config_index);
bool qcmap_enable_profile(int profile_index, int config_index);
bool qcmap_disable_profile(int profile_index, int config_index);
void qcmap_dial_profile(void);
void qcmap_disdial_profile(void);

void qcmap_update_profile_config(void);
void qcmap_get_profile_status(qcmap_msgr_wwan_status_enum_v01 *v4_status, qcmap_msgr_wwan_status_enum_v01 *v6_status, int profile_index);

int qcmap_find_zone_index(char *zone_name);
int qcmap_apply_nat(int index);
void qcmap_init_nat(void);
int qcmap_check_apply_nat(void);
void qcmap_init_default_route(void);
void qcmap_update_default_route(void);
void qcmap_update_dns_link(void);
bool qcmap_check_and_set_default_route(bool is_init);
bool qcmap_profile_connected(int profile_index, int config_index);
void qcmap_check_profile_update(void);
int qcmap_check_config_update(void);
void qcmap_apply_config_update(void);
void qcmap_init_firewall(void);

int qcmap_timer_check_config(void);
void qcmap_timer_check_profile(void);

#endif