#ifndef MOBILE_SERVICE_INFO_H
#define MOBILE_SERVICE_INFO_H

#include "ql_nw.h"
#include "ql_type.h"
#include "ql_dm.h"
#include "ql_data_call.h"
#include <pthread.h>

/* 宏定义 */
#define MAX_CELL_ENTRY_NUM 32
#define G_NODEB_BIT_LEN 24

/* 网络配置参数结构体 - 用于优化函数参数 */
typedef struct {
    char net_type[32];
    char cellid[32];
    char pcid[32];
    char earfcn[32];
    char band[32];
    char bandwidth_str[32];
    char duplexmode[32];
    char tac[32];
    char rsrp[32];
    char rsrq[32];
    char rssi[32];
    char sinr[32];
    char e_nbid[32];
    char dl_mcs[32];
    char ul_mcs[32];
    char lte_dl_rate[32];
    char lte_ul_rate[32];
    char nr_dl_rate[32];
    char nr_ul_rate[32];
    char nr_rank[32];
    uint64_t g_nodeb;
} network_config_params_t;

/* 小区基本信息结构体 */
typedef struct {
    char cellid[32];        /**< 小区ID */
    char pcid[16];          /**< 物理小区ID */
    char arfcn[16];         /**< 绝对射频信道号 */
    char rsrp[16];          /**< 参考信号接收功率 */
    char rsrq[16];          /**< 参考信号接收质量 */
    char sinr[16];          /**< 信噪比 */
} cell_entry_t;

/* 小区列表结构体 */
typedef struct {
    cell_entry_t lte_cell_entry[MAX_CELL_ENTRY_NUM];      /**< LTE小区列表 */
    cell_entry_t nr_cell_entry[MAX_CELL_ENTRY_NUM];       /**< NR小区列表 */
    int lte_cell_entry_num;                             /**< LTE小区数量 */
    int nr_cell_entry_num;                              /**< NR小区数量 */
} cell_entry_list_t;

// Quectel API相关变量结构体
typedef struct {
    int return_flag;
    int dbler;
    int ubler;
    int dgrant;
    int ugrant;
} quectel_api_vars_t;

// 上次小区信息缓存结构体
typedef struct {
    char cellid[32];                  /**< 上次小区ID缓存 */
    char pcid[16];                    /**< 上次物理小区ID缓存 */
    char rsrq[16];                    /**< 上次RSRQ缓存 */
    char rsrp[16];                    /**< 上次RSRP缓存 */
    char rssi[16];                    /**< 上次RSSI缓存 */
    char sinr[16];                    /**< 上次SINR缓存 */
    char band1[16];                   /**< 上次频段1缓存 */
    char band2[16];                   /**< 上次频段2缓存 */
} last_cell_info_t;

// 小区缓存结构体（整合移植功能相关全局变量）
typedef struct {
    cell_entry_list_t cell_entry_list;          /**< 小区列表 */
    int need_update_cellinfo;                   /**< 需要更新小区信息标志 */
    int need_update_cell_change_stat;           /**< 需要更新小区变更状态标志 */
    last_cell_info_t last_cell;                 /**< 上次小区信息缓存 */
    quectel_api_vars_t quectel_api_vars;        /**< Quectel API变量 */
} cell_cache_t;

typedef struct {
    pthread_t thread_id;
    volatile int running;
    pthread_mutex_t running_lock;
} info_thread_ctrl_t;

int mobile_init_info_service(void);
void mobile_deinit_info_service(void);

int mobile_init_info_check_task(void);
void mobile_deinit_info_check_task(void);
void mobile_set_info_thread_running(int running);
int mobile_get_info_thread_running(void);
unsigned int mobile_info_check_loop(void);

bool mobile_get_more_info_handler(void);
bool mobile_get_more_info_p7006_v2c7c(void);
void mobile_get_modem_info(void);

int mobile_get_signal_strength_info(ql_nw_signal_strength_info_t *info, QL_NW_SIGNAL_STRENGTH_LEVEL_E *level);
int mobile_get_operator_info(ql_nw_mobile_operator_name_info_t *name_info);
int mobile_get_network_config_info(ql_nw_cell_info_t *cell_info, ql_nw_get_band_info_t *band_info, ql_nw_get_meas_info_resp_t *meas_info, ql_nw_get_ca_info_resp_t *ca_info);
void mobile_process_network_info_by_radio_tech(ql_nw_cell_info_t *cell_info, ql_nw_get_band_info_t *band_info, ql_nw_get_meas_info_resp_t *meas_info, ql_nw_get_ca_info_resp_t *ca_info, network_config_params_t *params);
void mobile_process_nr5g_sa_info(ql_nw_cell_info_t *cell_info, ql_nw_get_band_info_t *band_info, ql_nw_get_meas_info_resp_t *meas_info, ql_nw_get_ca_info_resp_t *ca_info, network_config_params_t *params);
void mobile_process_nr5g_nsa_info(ql_nw_cell_info_t *cell_info, ql_nw_get_band_info_t *band_info, ql_nw_get_meas_info_resp_t *meas_info, ql_nw_get_ca_info_resp_t *ca_info, network_config_params_t *params);
void mobile_process_lte_info(ql_nw_cell_info_t *cell_info, ql_nw_get_band_info_t *band_info, ql_nw_get_meas_info_resp_t *meas_info, ql_nw_get_ca_info_resp_t *ca_info, network_config_params_t *params);
void mobile_process_3g_info(ql_nw_cell_info_t *cell_info, network_config_params_t *params);
void mobile_nr_grant_bler_ind_cb(ql_nw_throughput_info_t *ind_data);
int mobile_process_nr_throughput_indication(void);
void mobile_check_cell_info_change(const network_config_params_t *params);

void mobile_get_gnodeb_by_cid(uint64_t cid, uint64_t *g_nodeb);
int mobile_get_signal_strength_level(QL_NW_SIGNAL_STRENGTH_LEVEL_E level, char *level_info, size_t buf_size);
void mobile_get_scs_str_by_index(int index, char *scs_str);

void mobile_save_rf_parameters_to_file(const network_config_params_t *params);
void mobile_update_global_module_desc(const network_config_params_t *params, ql_nw_get_band_info_t *band_info);

void mobile_print_cell_entry_list(void);

#endif