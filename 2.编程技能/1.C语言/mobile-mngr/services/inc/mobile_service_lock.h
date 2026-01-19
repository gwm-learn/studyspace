#ifndef MOBILE_SERVICE_LOCK_H
#define MOBILE_SERVICE_LOCK_H

#include "ql_nw.h"
#include "mobile_module_lock.h"

#define PLMN_LOCK_PASSWORD "12345678"
#define MAX_PLMN_NUM 10
#define MAX_PCIDCELL_LOCK_NUM 64
#define MAX_PCIDCELL_LOCK_5G_NUM 32
#define MAX_PCIDCELL_LOCK_4G_NUM 32

typedef enum _lock_net_type_t {
    OTHER_TYPE = 0,
    NO_LOCK,
    LOCK_NR_ONLY,
    LOCK_LTE_ONLY,
    LOCK_NR_ONLY_IN_NRNSA,
    LOCK_LTE_ONLY_IN_NRNSA,
    LOCK_LTE_AND_NR_BOTH,
} lock_net_type_t;

typedef struct {
    char mcc_mnc_list[128];   /**< ReadWrite */
    char password[16];      /**< ReadWrite */
    char old_password[16];   /**< ReadWrite */
    unsigned int mnc_length; /**< ReadWrite */
    unsigned char enable;   /**< ReadWrite */
} sim_lock_config_t;

typedef struct {
    char pincode[32];
    int pincode_set;
    int auto_un_lock;
    unsigned char enable;   /**< ReadWrite */
} pin_lock_config_t;

typedef struct {
    support_band_list_t *support_band_list;
    char cfg_3g_band_list[128]; /**< 3G频段列表配置 */
    char cfg_4g_band_list[128]; /**< 4G频段列表配置 */
    char cfg_5g_band_list[128]; /**< 5G频段列表配置 */
    char cfg_5gnsa_band_list[128]; /**< 5G NSA频段列表配置 */
    char net_type[32];       /**< 网络类型 */
    unsigned char enable;   /**< ReadWrite */
} band_lock_config_t;

typedef struct _pcid_lock_config_t{
    struct _pcid_lock_config_t *next;
    char nettype[16];
    char pcid[16];
    char freq[16];
    char scs[16];
    char band[16];
    char cellid[16];
    unsigned char enable;   /**< ReadWrite */
} pcid_lock_config_t;

typedef struct {
    pcid_lock_config_t *pcid_lock;
    unsigned char enable;
    lock_net_type_t lock_net_type;
    int cell_lock_type; /* 1-lock pcid 2-lock cell id*/
} cell_lock_config_t;

typedef struct {
    sim_lock_config_t sim_lock_config;
    pin_lock_config_t pin_lock_config;
    band_lock_config_t band_lock_config;
    cell_lock_config_t cell_lock_config;
} lock_config_t;

int mobile_init_lock_service(void);
void mobile_deinit_lock_service(void);

int mobile_init_simlock_config(void);
int mobile_init_pinlock_config(void);
int mobile_init_bandlock_config(void);
int mobile_init_celllock_config(void);

bool mobile_sub_sim_lock_p7006_v2c7c(void);
bool mobile_sub_pin_lock_p7006_v2c7c(void);
bool mobile_sub_band_lock_p7006_v2c7c(void);
bool mobile_sub_cell_lock_p7006_v2c7c(void);

bool mobile_sim_lock_handler(void);
bool mobile_pin_lock_handler(void);
bool mobile_band_lock_handler(void);
bool mobile_cell_lock_handler(void);
bool mobile_apply_lock(void);
void mobile_update_cell_change_stat(void);

int mobile_sim_is_ready(bool tag);
int mobile_get_pin_left_times(void);
bool mobile_is_sim_pin_locked(void);
void mobile_set_produce_pin(char *pin);
int mobile_change_pin(const char* old_pin, const char* new_pin);
int mobile_unlock_pin(const char* pin_code);
int mobile_enable_pinlock(const char* pin_code);
int mobile_disable_pinlock(const char* pin_code);
void mobile_update_pincode_set(int value);

int mobile_is_simlock_mccmnc(void);
int mobile_is_simcard_lock(void);

int mobile_apply_bandlock_set(ql_nw_set_band_mode_info_t* pband_info);
int mobile_write_bandlist_to_file(const support_band_list_t *band_list);
void mobile_band_lock_3g(char *band_list);
void mobile_band_lock_4g(char *band_list);
void mobile_band_lock_5g_sa(char *band_list);
void mobile_band_lock_5g_nsa(char *band_list);
void mobile_band_lock_restore_all(void);
int mobile_generate_band_hex_string(const ql_nw_get_band_mode_info_t* band_info, const char* band_type, char* hex_str, size_t hex_size);
int mobile_reverse_hex_string(const char* hex_str, char* reversed_hex, size_t hex_size, const char* band_type);
int mobile_extract_bands_from_binary(const char* bin_str, int* bands, size_t max_bands);
int mobile_build_band_string(const int* bands, int band_count, char* band_str, size_t band_size);
int mobile_process_band_data(const ql_nw_get_band_mode_info_t* band_info, const char* band_type, char* band_str, size_t band_size);
int mobile_sort_band_string(const char* band_str, char* sorted_str, size_t sorted_size);
int mobile_compare_band_strings(const char* band_str1, const char* band_str2);
int mobile_get_current_band_settings(char* current_3g_band, char* current_4g_band, char* current_5g_band, size_t band_size);
void mobile_apply_bandlock_by_net_type(void);
void mobile_handle_bandlock_compare_and_restore(const char* current_3g_band, const char* current_4g_band, const char* current_5g_band, const char* target_3g_band, const char* target_4g_band, const char* target_5g_band, const char* context, void (*action_func)(void));
void mobile_handle_bandlock(bool enable, const char* current_3g_band, const char* current_4g_band, const char* current_5g_band);

void mobile_apply_cell_lock_net_type(void);
void mobile_set_lock_default_config(int lte_cell_count, int nr_cell_count);
void mobile_update_cell_lock_status(bool status);
int mobile_cell_lock_set_pcid_list(void);
int mobile_cell_lock_clear_pcid_list(void);
int mobile_cellid_check_in_whitelist(void);

void mobile_update_pin_lock(void);
int mobile_update_sim_lock(void);
int mobile_update_band_lock(void);
int mobile_update_cell_lock(void);
void mobile_update_lock_status(void);

void mobile_print_lock_config_info(void);
void mobile_debug_cell_lock_list(void);

#endif