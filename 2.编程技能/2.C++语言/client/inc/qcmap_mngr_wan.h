#ifndef _QCMAP_MNGR_WAN_H_
#define _QCMAP_MNGR_WAN_H_

#include "qcmap_mngr_util.h"

typedef struct inf_status_s {
    int status;
    char uptime[128];
    char l3_device[32];
    char proto[32];
    char device[32];
    char ipv4_address[32];
    char mask[32];
    char ipv6_address[128];
    char ipv6_mask[32];
    char ipv6_prefix_address[128];
    char ipv6_prefix_mask[64];
    char nexthop[128];
    char dns[64];
} inf_status_t;

void qcmap_get_interface_info(char* inf, inf_status_t* wandeviceinfo);
void qcmap_wan_uptime_update(void);
void qcmap_wan_status_update(bool is_first_call);
void qcmap_timer_check_wan(void);

bool qcmap_get_ip_passthrough_feature(void);
bool qcmap_set_ip_passthrough_feature(bool state);
bool qcmap_get_ip_passthrough_config(void);
void qcmap_set_ip_passthrough_config(bool state);
bool qcmap_get_ip_passthrough_state(void);
int qcmap_check_apply_bridge(void);
void qcmap_init_bridge(void);

#endif