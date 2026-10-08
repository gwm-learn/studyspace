#ifndef _QCMAP_MNGR_APN_H_
#define _QCMAP_MNGR_APN_H_

#include "qcmap_mngr_util.h"

typedef struct { // the dial info come from provider
    char MCC_MNC[16];
    char number[16];
    char apn[32];
    char username[32];
    char password[16];
} provider_t;

typedef struct {
    provider_t *providers;
    size_t count;
} provider_table_t;

typedef enum {
    AUTO_APN_DEALUTL = -1,
    AUTO_APN_INVALID,
    AUTO_APN_VALID,
    AUTO_APN_USING,
} auto_apn_status_t;

typedef struct {
    int status; //-1:default, 0:Invalid, 1:valid
    provider_t provider;
} AUTO_PROVIDER;

typedef struct {
    int cont;
    int provider_use_index;
    AUTO_PROVIDER auto_provider[64];
} AUTO_PROVIDER_LIST;

extern AUTO_PROVIDER_LIST auto_provider_list;

void qcmap_release_apn(void);

bool qcmap_set_apn_mccmnc_num(void);
bool qcmap_set_apn_mccmnc_str(void);
bool qcmap_get_apn_mccmnc(void);
bool qcmap_get_sim_mccmnc(void);
bool qcmap_get_provider(void);

void qcmap_update_auto_apn_status(int status);
provider_t *qcmap_get_auto_apn(void);

bool qcmap_init_auto_apn_with_apn_mccmnc(void);
bool qcmap_init_auto_apn_with_sim_mccmnc(void);
bool qcmap_init_auto_apn_with_no_mccmnc(void);
void qcmap_init_auto_apn_list(void);

bool qcmap_update_apn_name(int profile_index, int config_index);
bool qcmap_sync_apn_info_with_modem(int profile_index, int config_index);
bool qcmap_sync_apn_auth_with_modem(int profile_index, int config_index);
bool qcmap_get_apn_auth_info(int config_index, qcmap_wan5g_config_t *config);

bool qcmap_init_auth_apn_num(void);
int qcmap_get_apn_num(void);
int qcmap_get_max_apn_num_with_sim(void);
bool qcmap_get_apn_by_index(int profile_index, char *apn_name, int name_lan);
bool qcmap_init_apn_num(void);
bool qcmap_init_apn_num_in_modem(void);
#endif