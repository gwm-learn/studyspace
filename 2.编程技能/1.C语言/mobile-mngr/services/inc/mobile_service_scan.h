#ifndef MOBILE_SERVICE_SCAN_H
#define MOBILE_SERVICE_SCAN_H

#include <pthread.h>
#include "ql_nw.h"

#define SCAN_STAT_INDEL 0
#define SCAN_STAT_START 1
#define SCAN_STAT_READ 2
#define SCAN_STAT_END 3
#define SCAN_STAT_FAIL 4
#define SCAN_STAT_ABORT 5

#define MAX_CELL_BAND_ENTRY_NUM 64

/* 小区频段信息结构体 */
typedef struct {
    char plmn_name[32];     /**< PLMN名称 */
    char cellid[32];        /**< 小区ID */
    char pcid[16];          /**< 物理小区ID */
    char arfcn[16];         /**< 绝对射频信道号 */
    char rsrp[16];          /**< 参考信号接收功率 */
    char rsrq[16];          /**< 参考信号接收质量 */
    char snr[16];           /**< 信噪比 */
    char plmn_id[16];       /**< PLMN ID */
    char cell_band[16];     /**< 小区频段 */
    char rat[8];
} cell_band_entry_t;

/* 小区频段列表结构体 */
typedef struct {
    cell_band_entry_t lte_cell_band_entry[MAX_CELL_BAND_ENTRY_NUM];   /**< LTE小区频段列表 */
    cell_band_entry_t nr_cell_band_entry[MAX_CELL_BAND_ENTRY_NUM];    /**< NR小区频段列表 */
    int lte_cell_band_entry_num;                                    /**< LTE小区频段数量 */
    int nr_cell_band_entry_num;                                     /**< NR小区频段数量 */
} cell_band_entry_list_t;

/* 扫描模块上下文结构体 */
typedef struct {
    pthread_t scan_thread;                                          /**< 扫描线程句柄 */
    int cell_band_scan_stat;                                        /**< 扫描状态 */
    pthread_rwlock_t cell_band_scan_stat_lock;                      /**< 扫描状态读写锁 */
    volatile int scan_thread_running;                               /**< 扫描线程运行标志 */
    pthread_mutex_t scan_thread_running_lock;                       /**< 扫描线程运行标志互斥锁 */
    cell_band_entry_list_t scan_cell_band_entry_list;               /**< 扫描结果列表 */
    ql_nw_cell_band_scan_mode_req_t cell_band_scan_req_info;        /**< 扫描请求信息 */
} mobile_scan_context_t;

int mobile_init_scan_service(void);
void mobile_deinit_scan_service(void);

void mobile_set_scan_thread_running(int running);
int mobile_get_scan_thread_running(void);

void mobile_pthread_band_cell_scan(void);
unsigned int mobile_cell_band_scan_thread(void);
void mobile_cell_band_scan(int cell_rat, int timeout);
void mobile_fix_nr_scan_result(void);
void mobile_fix_lte_scan_result(void);
void mobile_process_lte_scan_result(ql_nw_cell_band_scan_result_list_info_t* p_info, int* lte_band_count);
void mobile_process_nr_scan_result(ql_nw_cell_band_scan_result_list_info_t* p_info, int* nr_band_count);
void mobile_handle_success_scan(ql_nw_cell_band_scan_result_list_info_t* p_info);
void mobile_handle_abort_scan(ql_nw_cell_band_scan_result_list_info_t* p_info);
void mobile_cell_band_scan_async_cb(ql_nw_cell_band_scan_result_list_info_t* p_info);

void mobile_set_cell_band_scan_stat(int stat);
int mobile_get_cell_band_scan_stat(void);
bool mobile_check_cell_band_scan_stat(int stat);

void mobile_bubble_sort(cell_band_entry_t arr[], int size);
void mobile_cell_band_num_save(void);
void mobile_lte_cell_band_save(void);
void mobile_nr_cell_band_save(void);

void mobile_print_cell_band_entry_list(void);

#endif