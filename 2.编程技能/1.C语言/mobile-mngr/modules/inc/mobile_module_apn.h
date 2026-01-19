#ifndef MOBILE_MODULE_APN_H
#define MOBILE_MODULE_APN_H

typedef enum {
    APN_STATUS_UNUSED = 0,        // 未使用
    APN_STATUS_USED_VALID = 1,    // 已设置且有效
    APN_STATUS_USED_INVALID = -1, // 已设置但无效
    APN_STATUS_USED_UNVERIFIED = 2 // 已使用未验证
} apn_status_t;

typedef struct _apn_config {
    char apn_buf[64];
    char iaapn_buf[64];
    char username_buf[32];
    char password_buf[32];
    char auth_buf[16];
    char iptype_buf[16];
    char cid_buf[8];
    char mtu_buf[8];
    char vlan_buf[8];
} apn_config;

typedef struct {    //the dial info come from provider
    char mccmnc[32];
    char number[32];
    char apn[32];
    char username[32];
    char password[32];
} provider_t;

typedef struct {
    provider_t *providers;
    size_t count;
} provider_table_t;

typedef struct _auto_provider_t{
    struct _auto_provider_t *next;
    provider_t *provider;
    apn_status_t status; // APN配置状态
} auto_provider_t;

const char* mobile_get_apn_status_str(apn_status_t status);
int mobile_init_auto_apn_list(const char *mccmnc, auto_provider_t **auto_provider_list_head);
void mobile_print_auto_apn_list(auto_provider_t **auto_provider_list);
int mobile_apn_tables_count(void);
provider_table_t* mobile_apn_tables(void);
#endif