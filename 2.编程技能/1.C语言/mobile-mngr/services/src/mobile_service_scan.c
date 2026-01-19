#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "mobile_module_util.h"
#include "mobile_service_modem.h"
#include "mobile_service_dial.h"
#include "mobile_service_sms.h"
#include "mobile_service_scan.h"
#include "mobile_service_state_machine.h"

/*
 * 文件名称：mobile_service_scan.c
 * 功能描述：
 *     cell scan服务模块
 *
 * 作者：gaoweiming
 */

static int g_scan_module_initialized = 0;

/* 扫描模块全局上下文 */
static mobile_scan_context_t g_scan_ctx = {
    .scan_thread = 0,
    .cell_band_scan_stat = SCAN_STAT_INDEL,
    .cell_band_scan_stat_lock = PTHREAD_RWLOCK_INITIALIZER,
    .scan_thread_running = 0,
    .scan_thread_running_lock = PTHREAD_MUTEX_INITIALIZER,
    .scan_cell_band_entry_list = {0},
    .cell_band_scan_req_info = {0}
};

/**
 * 初始化scan服务模块
 *
 * @return 成功返回0，失败返回错误码
 */
int mobile_init_scan_service(void) {
    if (g_scan_module_initialized) {
        MOBILE_INFO("scan service module already initialized\n");
        return 0;
    }

    mobile_pthread_band_cell_scan();

    g_scan_module_initialized = 1;
    MOBILE_INFO("Scan service module initialized successfully\n");
    return 0;
}

/**
 * 清理scan服务模块资源
 *
 * @return 无返回值
 */
void mobile_deinit_scan_service(void) {
    if (!g_scan_module_initialized) {
        return;
    }

    mobile_set_scan_thread_running(0);
    
    pthread_cancel(g_scan_ctx.scan_thread);
    // 等待线程完全退出
    pthread_join(g_scan_ctx.scan_thread, NULL);
    MOBILE_INFO("<<<<<********************** Scan thread stopped **********************>>>>>\n");

    g_scan_module_initialized = 0;
    MOBILE_INFO("scan service module deinitialized\n");
}

/**
 * 设置扫描线程运行状态（带锁）
 *
 * @param running 运行状态：1-运行，0-停止
 * @return 无返回值
 */
void mobile_set_scan_thread_running(int running) {
    pthread_mutex_lock(&g_scan_ctx.scan_thread_running_lock);
    g_scan_ctx.scan_thread_running = running;
    pthread_mutex_unlock(&g_scan_ctx.scan_thread_running_lock);
}

/**
 * 获取扫描线程运行状态（带锁）
 *
 * @return 运行状态：1-运行，0-停止
 */
int mobile_get_scan_thread_running(void) {
    int running;
    pthread_mutex_lock(&g_scan_ctx.scan_thread_running_lock);
    running = g_scan_ctx.scan_thread_running;
    pthread_mutex_unlock(&g_scan_ctx.scan_thread_running_lock);
    return running;
}

/**
 * 创建小区频段扫描线程
 * 启动小区频段扫描的后台线程，用于异步执行频段扫描操作
 *
 * @return 无返回值
 */
void mobile_pthread_band_cell_scan(void) {
    if (mobile_get_scan_thread_running()) {
        MOBILE_WARN("Scan thread is already running\n");
        return;
    }
    
    mobile_set_scan_thread_running(1);
    pthread_create(&g_scan_ctx.scan_thread, NULL, (void*)mobile_cell_band_scan_thread, NULL);
}

/**
 * 处理扫描结束状态
 * 封装扫描结束、中止、失败状态的公共处理逻辑
 *
 * @param result_file 结果文件路径（CELL_SCAN_SUCCESS_FILE 或 CELL_SCAN_FAIL_FILE）
 * @return 无返回值
 */
static void mobile_handle_scan_completion(const char* result_file) {
    mobile_at_cmd("AT+EGTYPE=1", NULL, 0);
    sleep(5);
    mobile_write_buff_to_file(result_file, "1", sizeof("1"));
    mobile_set_cell_band_scan_stat(SCAN_STAT_INDEL);
    mobile_system_ex("produce allledoff", 0);
    mobile_system_ex("produce factoryMode fm=0", 0);
    mobile_update_led_force();
}

/**
 * 开始扫描
 *
 * @return 无返回值
 */
static void mobile_handle_scan_start(void) {
    unlink(SCAN_CELLLIST_FLG);
    unlink(CELL_NR_LIST_FILE);
    unlink(CELL_LTE_LIST_FILE);
    unlink(CELL_NUM_FILE);
    mobile_at_cmd("'AT+ESBP=5,\"SBP_SMART_RELEASE\",1'", NULL, 0);
    sleep(5);
    mobile_at_cmd("AT+EGTYPE=0,1", NULL, 0);
    sleep(1);
    if (ql_nw_init(NW_IPC_MODE_DEFAULT) != QL_NW_SUCCESS) {
        mobile_set_cell_band_scan_stat(SCAN_STAT_FAIL);
        return;
    }
    mobile_uci_set("mobile.@celllock[0].state", "Scanning");
    mobile_set_cell_band_scan_stat(SCAN_STAT_START);
    mobile_system_ex("produce allledblink", 0);
    mobile_cell_band_scan(0, 5*60);
}

/**
 * 小区频段扫描线程函数
 * 监控小区频段扫描状态并执行相应的扫描操作
 * 处理扫描成功、中止和失败的情况，更新相关状态文件
 * 在SIM卡就绪且网络连接时启动扫描
 *
 * @return 线程返回值，总是返回0
 */
unsigned int mobile_cell_band_scan_thread(void) {
    int count_time = 0;

    MOBILE_INFO("<<<<<********************** Scan thread starting **********************>>>>>\n");
    while(mobile_get_scan_thread_running()) {
        pthread_testcancel();

        sleep(2);

        // 如果状态机正在运行，跳过小区扫描
        if (mobile_get_state_machine_running_status()) {
            MOBILE_DEBUG("State machine is running, skip mobile cell scan\n");
            continue;
        }

        if(mobile_check_cell_band_scan_stat(SCAN_STAT_START)) {
            count_time++;
        } else {
            count_time = 0;
        }

        if(count_time > 5*60) {//cell_band_scan_async_cb not return we need change the stat
            count_time = 0;
            mobile_set_cell_band_scan_stat(SCAN_STAT_ABORT);
        }

        if (access(SCAN_CELLLIST_FLG, F_OK)  == 0 && g_module_desc.simstatus && g_module_desc.wanstatus != NULL && 
            !strcmp(g_module_desc.wanstatus, DIALD_CONNECTED) && !mobile_check_cell_band_scan_stat(SCAN_STAT_START)) {
            MOBILE_INFO("<<<<<********************** scan cell start **********************>>>>>\n");
            mobile_handle_scan_start();
        }

        if(mobile_check_cell_band_scan_stat(SCAN_STAT_END)) {
            MOBILE_INFO("<<<<<********************** scan cell end **********************>>>>>\n");
            mobile_handle_scan_completion(CELL_SCAN_SUCCESS_FILE);
        } else if(mobile_check_cell_band_scan_stat(SCAN_STAT_ABORT)) {
            MOBILE_INFO("<<<<<********************** scan cell abort **********************>>>>>\n");
            mobile_handle_scan_completion(CELL_SCAN_FAIL_FILE);
        } else if(mobile_check_cell_band_scan_stat(SCAN_STAT_FAIL)) {
            MOBILE_INFO("<<<<<********************** scan cell fail **********************>>>>>\n");
            mobile_handle_scan_completion(CELL_SCAN_FAIL_FILE);
        }

        pthread_testcancel();
    }
    
    mobile_set_scan_thread_running(0);
    return 0;
}

/**
 * 执行小区频段扫描
 * 初始化小区频段扫描请求参数并启动异步扫描
 * 支持不同的RAT类型（LTE、NR或两者）和超时设置
 *
 * @param cell_rat 小区RAT类型：0-NR&LTE, 1-NR ONLY, 2-LTE ONLY
 * @param timeout 扫描超时时间（秒）
 * @return 无返回值
 */
void mobile_cell_band_scan(int cell_rat, int timeout) {
    int input = 0;
    int len = 0;
    int ret = 0;
    int i = 0;
    memset(&g_scan_ctx.cell_band_scan_req_info, 0, sizeof(ql_nw_cell_band_scan_mode_req_t));
    g_scan_ctx.cell_band_scan_req_info.mode = 1;               //0: ABORT CELL BAND SCAN   1: START CELL BAND SCAN
    g_scan_ctx.cell_band_scan_req_info.scan_type = 0;          //0: ONLINE_CUS_DEP , 1: ONLINE_NETWORK_DEP
    g_scan_ctx.cell_band_scan_req_info.cell_rat = cell_rat;    //0:NR&LTE , 1:NR ONLY, 2:LTE ONLY
    g_scan_ctx.cell_band_scan_req_info.timeout = timeout;      //scan timeout

    MOBILE_INFO("<<<<<********************** scan cell working **********************>>>>>\n");
    ret = ql_nw_cell_band_scan(&g_scan_ctx.cell_band_scan_req_info, mobile_cell_band_scan_async_cb);
    if (ret == QL_NW_SUCCESS) {
        MOBILE_DEBUG("cell band scan mode set success...\n");
    } else {
        MOBILE_ERROR("cell band scan fail!,ret = %d\n", ret);
        mobile_set_cell_band_scan_stat(SCAN_STAT_FAIL);
        ql_nw_release();
    }
}

/**
 * 修复NR扫描结果
 * 将当前连接的5G小区信息添加到NR扫描结果列表中
 * 如果当前连接的5G小区不在扫描结果中，则将其添加进去
 * 同时也会将之前扫描到的5G小区信息添加到列表中
 *
 * @return 无返回值
 */
void mobile_fix_nr_scan_result(void) {
    int i = 0;
    int find_flag = 0;
    int scan_before_find_flag = 0;

    if((strstr(g_module_desc.rf.net_type, "NR5G-SA")) && (strlen(g_module_desc.rf.global_cell_id)>0)) {
        for(i=0;i <g_scan_ctx.scan_cell_band_entry_list.nr_cell_band_entry_num;i++) {
            if(!strcmp(g_scan_ctx.scan_cell_band_entry_list.nr_cell_band_entry[i].cellid,g_module_desc.rf.global_cell_id)) {
                find_flag = 1;
                break;
            }
        }

        if(find_flag == 0) {//not find need add it to scan result
            MOBILE_DEBUG("add cellid: %s to nr scan list\n",g_module_desc.rf.global_cell_id);
            sprintf(g_scan_ctx.scan_cell_band_entry_list.nr_cell_band_entry[g_scan_ctx.scan_cell_band_entry_list.nr_cell_band_entry_num].cellid, "%s", g_module_desc.rf.global_cell_id);
            sprintf(g_scan_ctx.scan_cell_band_entry_list.nr_cell_band_entry[g_scan_ctx.scan_cell_band_entry_list.nr_cell_band_entry_num].rat, "%s", "");
            sprintf(g_scan_ctx.scan_cell_band_entry_list.nr_cell_band_entry[g_scan_ctx.scan_cell_band_entry_list.nr_cell_band_entry_num].arfcn, "%s", g_module_desc.rf.dl_earfcn);
            sprintf(g_scan_ctx.scan_cell_band_entry_list.nr_cell_band_entry[g_scan_ctx.scan_cell_band_entry_list.nr_cell_band_entry_num].pcid, "%s", g_module_desc.rf.physical_cell_id);
            sprintf(g_scan_ctx.scan_cell_band_entry_list.nr_cell_band_entry[g_scan_ctx.scan_cell_band_entry_list.nr_cell_band_entry_num].rsrp, "%s", g_module_desc.rf.rsrp);
            sprintf(g_scan_ctx.scan_cell_band_entry_list.nr_cell_band_entry[g_scan_ctx.scan_cell_band_entry_list.nr_cell_band_entry_num].rsrq, "%s", g_module_desc.rf.rsrq);
            sprintf(g_scan_ctx.scan_cell_band_entry_list.nr_cell_band_entry[g_scan_ctx.scan_cell_band_entry_list.nr_cell_band_entry_num].snr, "%s", g_module_desc.rf.sinr);
            sprintf(g_scan_ctx.scan_cell_band_entry_list.nr_cell_band_entry[g_scan_ctx.scan_cell_band_entry_list.nr_cell_band_entry_num].cell_band, "%s", g_module_desc.rf.frequency_band);
            sprintf(g_scan_ctx.scan_cell_band_entry_list.nr_cell_band_entry[g_scan_ctx.scan_cell_band_entry_list.nr_cell_band_entry_num].plmn_id, "%s", g_module_desc.plmn);
            sprintf(g_scan_ctx.scan_cell_band_entry_list.nr_cell_band_entry[g_scan_ctx.scan_cell_band_entry_list.nr_cell_band_entry_num++].plmn_name, "%s", g_module_desc.plmn);
            //g_scan_ctx.scan_cell_band_entry_list.lte_cell_band_entry_num++;
        }
    }
}

/**
 * 修复LTE扫描结果
 * 将当前连接的小区信息添加到LTE扫描结果列表中
 * 如果当前连接的小区不在扫描结果中，则将其添加进去
 * 同时也会将之前扫描到的小区信息添加到列表中
 *
 * @return 无返回值
 */
void mobile_fix_lte_scan_result(void) {
    int i = 0;
    int find_flag = 0;
    int scan_before_find_flag = 0;

    if((strstr(g_module_desc.rf.net_type, "NR5G-NSA") || strstr(g_module_desc.rf.net_type, "LTE")) && (strlen(g_module_desc.rf.global_cell_id)>0)) {
        for(i=0;i <g_scan_ctx.scan_cell_band_entry_list.lte_cell_band_entry_num;i++) {
            if(!strcmp(g_scan_ctx.scan_cell_band_entry_list.lte_cell_band_entry[i].cellid, g_module_desc.rf.global_cell_id)) {
                find_flag = 1;
                break;
            }
        }

        if(find_flag == 0) {//not find need add it to scan result
            MOBILE_DEBUG("add cellid: %s to lte scan list\n",g_module_desc.rf.global_cell_id);
            sprintf(g_scan_ctx.scan_cell_band_entry_list.lte_cell_band_entry[g_scan_ctx.scan_cell_band_entry_list.lte_cell_band_entry_num].cellid, "%s", g_module_desc.rf.global_cell_id);
            sprintf(g_scan_ctx.scan_cell_band_entry_list.lte_cell_band_entry[g_scan_ctx.scan_cell_band_entry_list.lte_cell_band_entry_num].rat, "%s", "");
            sprintf(g_scan_ctx.scan_cell_band_entry_list.lte_cell_band_entry[g_scan_ctx.scan_cell_band_entry_list.lte_cell_band_entry_num].arfcn, "%s", g_module_desc.rf.dl_earfcn);
            sprintf(g_scan_ctx.scan_cell_band_entry_list.lte_cell_band_entry[g_scan_ctx.scan_cell_band_entry_list.lte_cell_band_entry_num].pcid, "%s", g_module_desc.rf.physical_cell_id);
            sprintf(g_scan_ctx.scan_cell_band_entry_list.lte_cell_band_entry[g_scan_ctx.scan_cell_band_entry_list.lte_cell_band_entry_num].rsrp, "%s", g_module_desc.rf.rsrp);
            sprintf(g_scan_ctx.scan_cell_band_entry_list.lte_cell_band_entry[g_scan_ctx.scan_cell_band_entry_list.lte_cell_band_entry_num].rsrq, "%s", g_module_desc.rf.rsrq);
            sprintf(g_scan_ctx.scan_cell_band_entry_list.lte_cell_band_entry[g_scan_ctx.scan_cell_band_entry_list.lte_cell_band_entry_num].snr, "%s", g_module_desc.rf.sinr);
            sprintf(g_scan_ctx.scan_cell_band_entry_list.lte_cell_band_entry[g_scan_ctx.scan_cell_band_entry_list.lte_cell_band_entry_num].cell_band, "%s", g_module_desc.rf.frequency_band);
            sprintf(g_scan_ctx.scan_cell_band_entry_list.lte_cell_band_entry[g_scan_ctx.scan_cell_band_entry_list.lte_cell_band_entry_num].plmn_id, "%s", g_module_desc.plmn);
            sprintf(g_scan_ctx.scan_cell_band_entry_list.lte_cell_band_entry[g_scan_ctx.scan_cell_band_entry_list.lte_cell_band_entry_num++].plmn_name, "%s", g_module_desc.plmn);
            //g_scan_ctx.scan_cell_band_entry_list.lte_cell_band_entry_num++;
        }
    }
}

/**
 * 处理LTE频段扫描结果
 * 将扫描到的LTE小区信息保存到全局列表中
 *
 * @param p_info 扫描结果信息指针
 * @param lte_band_count LTE频段计数器指针
 * @return 无返回值
 */
void mobile_process_lte_scan_result(ql_nw_cell_band_scan_result_list_info_t* p_info, int* lte_band_count) {
    if (p_info->lte_band_count <= 0) {
        return;
    }

    MOBILE_DEBUG(" scan lte band:\n");
    g_scan_ctx.scan_cell_band_entry_list.lte_cell_band_entry_num = 0;
    for (int index = 0; index < p_info->lte_band_count; index++) {
        sprintf(g_scan_ctx.scan_cell_band_entry_list.lte_cell_band_entry[index].rat, "%d", p_info->lte_cell_band_info[index].rat);
        sprintf(g_scan_ctx.scan_cell_band_entry_list.lte_cell_band_entry[index].arfcn, "%d", p_info->lte_cell_band_info[index].arfcn);
        sprintf(g_scan_ctx.scan_cell_band_entry_list.lte_cell_band_entry[index].pcid, "%d", p_info->lte_cell_band_info[index].pci);
        sprintf(g_scan_ctx.scan_cell_band_entry_list.lte_cell_band_entry[index].rsrp, "%d", p_info->lte_cell_band_info[index].rsrp);
        sprintf(g_scan_ctx.scan_cell_band_entry_list.lte_cell_band_entry[index].rsrq, "%d", p_info->lte_cell_band_info[index].rsrq);
        sprintf(g_scan_ctx.scan_cell_band_entry_list.lte_cell_band_entry[index].snr, "%d", p_info->lte_cell_band_info[index].snr);
        sprintf(g_scan_ctx.scan_cell_band_entry_list.lte_cell_band_entry[index].cellid, "%ld", p_info->lte_cell_band_info[index].cid);
        sprintf(g_scan_ctx.scan_cell_band_entry_list.lte_cell_band_entry[index].cell_band, "%d", p_info->lte_cell_band_info[index].cell_band);
        sprintf(g_scan_ctx.scan_cell_band_entry_list.lte_cell_band_entry[index].plmn_id, "%s", p_info->lte_cell_plmn_info[index].plmn_id);
        sprintf(g_scan_ctx.scan_cell_band_entry_list.lte_cell_band_entry[index].plmn_name, "%s", p_info->lte_cell_plmn_info[index].plmn_name);
        g_scan_ctx.scan_cell_band_entry_list.lte_cell_band_entry_num = p_info->lte_band_count;
    }
    mobile_fix_lte_scan_result();
    mobile_lte_cell_band_save();
}

/**
 * 处理NR频段扫描结果
 * 将扫描到的NR小区信息保存到全局列表中
 *
 * @param p_info 扫描结果信息指针
 * @param nr_band_count NR频段计数器指针
 * @return 无返回值
 */
void mobile_process_nr_scan_result(ql_nw_cell_band_scan_result_list_info_t* p_info, int* nr_band_count) {
    if (p_info->nr_band_count <= 0) {
        return;
    }

    g_scan_ctx.scan_cell_band_entry_list.nr_cell_band_entry_num = p_info->nr_band_count;
    MOBILE_DEBUG(" scan nr band:\n");
    g_scan_ctx.scan_cell_band_entry_list.nr_cell_band_entry_num = 0;
    for (int index = 0; index < p_info->nr_band_count; index++) {
        sprintf(g_scan_ctx.scan_cell_band_entry_list.nr_cell_band_entry[index].rat, "%d", p_info->nr_cell_band_info[index].rat);
        sprintf(g_scan_ctx.scan_cell_band_entry_list.nr_cell_band_entry[index].arfcn, "%d", p_info->nr_cell_band_info[index].arfcn);
        sprintf(g_scan_ctx.scan_cell_band_entry_list.nr_cell_band_entry[index].pcid, "%d", p_info->nr_cell_band_info[index].pci);
        sprintf(g_scan_ctx.scan_cell_band_entry_list.nr_cell_band_entry[index].rsrp, "%d", p_info->nr_cell_band_info[index].rsrp);
        sprintf(g_scan_ctx.scan_cell_band_entry_list.nr_cell_band_entry[index].rsrq, "%d", p_info->nr_cell_band_info[index].rsrq);
        sprintf(g_scan_ctx.scan_cell_band_entry_list.nr_cell_band_entry[index].snr, "%d", p_info->nr_cell_band_info[index].snr);
        sprintf(g_scan_ctx.scan_cell_band_entry_list.nr_cell_band_entry[index].cellid, "%ld", p_info->nr_cell_band_info[index].cid);
        sprintf(g_scan_ctx.scan_cell_band_entry_list.nr_cell_band_entry[index].cell_band, "%d", p_info->nr_cell_band_info[index].cell_band);
        sprintf(g_scan_ctx.scan_cell_band_entry_list.nr_cell_band_entry[index].plmn_id, "%s", p_info->nr_cell_plmn_info[index].plmn_id);
        sprintf(g_scan_ctx.scan_cell_band_entry_list.nr_cell_band_entry[index].plmn_name, "%s", p_info->nr_cell_plmn_info[index].plmn_name);
        g_scan_ctx.scan_cell_band_entry_list.nr_cell_band_entry_num = p_info->nr_band_count;
    }
    mobile_fix_nr_scan_result();
    mobile_nr_cell_band_save();
}

/**
 * 处理成功的小区频段扫描结果
 * 处理成功扫描到的LTE和NR频段信息
 *
 * @param p_info 扫描结果信息指针
 * @return 无返回值
 */
void mobile_handle_success_scan(ql_nw_cell_band_scan_result_list_info_t* p_info) {
    int lte_band_count = 0;
    int nr_band_count = 0;

    if (p_info->lte_band_count > 0 || p_info->nr_band_count > 0) {
        MOBILE_INFO("<<<<<********************** cell band scan success **********************>>>>>\n");
        MOBILE_DEBUG("lte_band_count = %d\n", p_info->lte_band_count);
        MOBILE_DEBUG("nr_band_count = %d\n", p_info->nr_band_count);

        mobile_process_lte_scan_result(p_info, &lte_band_count);
        mobile_process_nr_scan_result(p_info, &nr_band_count);
        
        mobile_cell_band_num_save();
        mobile_set_cell_band_scan_stat(SCAN_STAT_END);
        mobile_print_cell_band_entry_list();
    }
}

/**
 * 处理中止的小区频段扫描结果
 * 处理中止扫描到的LTE和NR频段信息
 *
 * @param p_info 扫描结果信息指针
 * @return 无返回值
 */
void mobile_handle_abort_scan(ql_nw_cell_band_scan_result_list_info_t* p_info) {
    int lte_band_count = 0;
    int nr_band_count = 0;

    MOBILE_INFO("<<<<<********************** cell band scan abort **********************>>>>>\n");
    if (p_info->lte_band_count > 0 || p_info->nr_band_count > 0) {
        MOBILE_DEBUG("lte_band_count = %d\n", p_info->lte_band_count);
        MOBILE_DEBUG("nr_band_count = %d\n", p_info->nr_band_count);

        mobile_process_lte_scan_result(p_info, &lte_band_count);
        mobile_process_nr_scan_result(p_info, &nr_band_count);
        
        mobile_cell_band_num_save();
        mobile_set_cell_band_scan_stat(SCAN_STAT_ABORT);
        mobile_print_cell_band_entry_list();
    }
}

/**
 * 小区频段扫描异步回调函数（重构版本）
 * 处理小区频段扫描的异步回调结果，拆分为多个小函数
 *
 * @param p_info 扫描结果信息指针
 * @return 无返回值
 */
void mobile_cell_band_scan_async_cb(ql_nw_cell_band_scan_result_list_info_t* p_info) {
    if (!p_info) {
        MOBILE_ERROR("invalid parameter\n");
        mobile_set_cell_band_scan_stat(SCAN_STAT_END);
        ql_nw_release();
        return;
    }

    if (p_info->cell_band_result == QL_NW_ASYNC_SUCCESS) {
        mobile_handle_success_scan(p_info);
    } else if (p_info->cell_band_result == QL_NW_ASYNC_ABORT) {
        mobile_handle_abort_scan(p_info);
    } else {
        MOBILE_ERROR("cell band scan fail!\n");
        mobile_set_cell_band_scan_stat(SCAN_STAT_FAIL);
    }
    ql_nw_release();
}

/**
 * 设置小区频段扫描状态（带锁）
 *
 * @param stat 要设置的扫描状态值
 * @return 无返回值
 */
void mobile_set_cell_band_scan_stat(int stat) {
    pthread_rwlock_wrlock(&g_scan_ctx.cell_band_scan_stat_lock);
    g_scan_ctx.cell_band_scan_stat = stat;
    pthread_rwlock_unlock(&g_scan_ctx.cell_band_scan_stat_lock);
}

/**
 * 检查小区频段扫描状态
 *
 * @return true -- 状态相同于status， false -- 状态不相同于status
 */
bool mobile_check_cell_band_scan_stat(int stat) {
    return (mobile_get_cell_band_scan_stat() == stat);
}

/**
 * 获取小区频段扫描状态（带锁）
 *
 * @return 当前扫描状态值
 */
int mobile_get_cell_band_scan_stat(void) {
    int stat;
    pthread_rwlock_rdlock(&g_scan_ctx.cell_band_scan_stat_lock);
    stat = g_scan_ctx.cell_band_scan_stat;
    pthread_rwlock_unlock(&g_scan_ctx.cell_band_scan_stat_lock);
    return stat;
}

/**
 * @brief 冒泡排序算法实现
 *
 * 对cell_band_entry_t数组进行冒泡排序，按RSRP值降序排列
 *
 * @param arr cell_band_entry_t数组
 * @param size 数组元素个数
 * @return 无返回值
 */
void mobile_bubble_sort(cell_band_entry_t arr[], int size) {
    int j, i;
    cell_band_entry_t tmp;
    for (i = 0; i < size - 1; i++) {
        for (j = 0; j < size - 1 - i; j++) {
            if (atoi(arr[j].rsrp) < atoi(arr[j + 1].rsrp)) {
                memcpy(&tmp, &arr[j], sizeof(cell_band_entry_t));
                memcpy(&arr[j], &arr[j + 1], sizeof(cell_band_entry_t));
                memcpy(&arr[j + 1], &tmp, sizeof(cell_band_entry_t));
            }
        }
    }
    
    MOBILE_DEBUG("Bubble sort completed for %d elements\n", size);
}

/**
 * @brief 保存小区频段数量信息到文件
 *
 * 将LTE和NR小区频段的数量信息保存到指定文件中，格式为：
 * lte_num=<数量>
 * nr_num=<数量>
 *
 * @return 无返回值
 */
void mobile_cell_band_num_save(void) {
    FILE* fp = fopen(CELL_NUM_FILE, "w+");
    if (fp) {
        char buf_line[128] = {0};

        snprintf(buf_line, sizeof(buf_line), "lte_num=%d\n", g_scan_ctx.scan_cell_band_entry_list.lte_cell_band_entry_num);
        fputs(buf_line, fp);
        snprintf(buf_line, sizeof(buf_line), "nr_num=%d\n", g_scan_ctx.scan_cell_band_entry_list.nr_cell_band_entry_num);
        fputs(buf_line, fp);
        fclose(fp);
    }
}

/**
 * @brief 保存LTE小区频段信息到文件
 *
 * 将LTE小区的频段详细信息保存到文件中，包括：
 * - 物理小区ID (PCID)
 * - 绝对射频信道号 (ARFCN)
 * - 参考信号接收功率 (RSRP)
 * - 参考信号接收质量 (RSRQ)
 * - 信噪比 (SNR)
 * - 小区ID
 * - PLMN ID
 * - PLMN名称
 * - 小区频段
 *
 * 保存前会对小区列表进行排序。
 *
 * @return 无返回值
 */
void mobile_lte_cell_band_save(void) {
    char tmp_str[32] = {0};
    char buf_line[128] = {0};
    FILE* fp = fopen(CELL_LTE_LIST_FILE, "w+");
    mobile_bubble_sort(g_scan_ctx.scan_cell_band_entry_list.lte_cell_band_entry, g_scan_ctx.scan_cell_band_entry_list.lte_cell_band_entry_num);
    if (fp) {
        for (int i = 0; i < g_scan_ctx.scan_cell_band_entry_list.lte_cell_band_entry_num; i++) {
            memset(tmp_str, 0, sizeof(tmp_str));
            memset(buf_line, 0, sizeof(buf_line));
            sprintf(tmp_str, "lte_pcid%d", i);
            snprintf(buf_line, sizeof(buf_line), "%s=%s\n", tmp_str, g_scan_ctx.scan_cell_band_entry_list.lte_cell_band_entry[i].pcid);
            fputs(buf_line, fp);

            memset(tmp_str, 0, sizeof(tmp_str));
            memset(buf_line, 0, sizeof(buf_line));
            sprintf(tmp_str, "lte_arfcn%d", i);
            snprintf(buf_line, sizeof(buf_line), "%s=%s\n", tmp_str, g_scan_ctx.scan_cell_band_entry_list.lte_cell_band_entry[i].arfcn);
            fputs(buf_line, fp);

            memset(tmp_str, 0, sizeof(tmp_str));
            memset(buf_line, 0, sizeof(buf_line));
            sprintf(tmp_str, "lte_rsrp%d", i);
            snprintf(buf_line, sizeof(buf_line), "%s=%d\n", tmp_str, atoi(g_scan_ctx.scan_cell_band_entry_list.lte_cell_band_entry[i].rsrp)/4);
            fputs(buf_line, fp);

            memset(tmp_str, 0, sizeof(tmp_str));
            memset(buf_line, 0, sizeof(buf_line));
            sprintf(tmp_str, "lte_rsrq%d", i);
            snprintf(buf_line, sizeof(buf_line), "%s=%d\n", tmp_str, atoi(g_scan_ctx.scan_cell_band_entry_list.lte_cell_band_entry[i].rsrq)/4);
            fputs(buf_line, fp);

            memset(tmp_str, 0, sizeof(tmp_str));
            memset(buf_line, 0, sizeof(buf_line));
            sprintf(tmp_str, "lte_snr%d", i);
            snprintf(buf_line, sizeof(buf_line), "%s=%d\n", tmp_str, atoi(g_scan_ctx.scan_cell_band_entry_list.lte_cell_band_entry[i].snr)/4);
            fputs(buf_line, fp);

            memset(tmp_str, 0, sizeof(tmp_str));
            memset(buf_line, 0, sizeof(buf_line));
            sprintf(tmp_str, "lte_cellid%d", i);
            snprintf(buf_line, sizeof(buf_line), "%s=%s\n", tmp_str,
                     g_scan_ctx.scan_cell_band_entry_list.lte_cell_band_entry[i].cellid);
            fputs(buf_line, fp);

            memset(tmp_str, 0, sizeof(tmp_str));
            memset(buf_line, 0, sizeof(buf_line));
            sprintf(tmp_str, "lte_plmn_id%d", i);
            snprintf(buf_line, sizeof(buf_line), "%s=%s\n", tmp_str,
                     g_scan_ctx.scan_cell_band_entry_list.lte_cell_band_entry[i].plmn_id);
            fputs(buf_line, fp);

            memset(tmp_str, 0, sizeof(tmp_str));
            memset(buf_line, 0, sizeof(buf_line));
            sprintf(tmp_str, "lte_plmn_name%d", i);
            snprintf(buf_line, sizeof(buf_line), "%s=%s\n", tmp_str,
                     g_scan_ctx.scan_cell_band_entry_list.lte_cell_band_entry[i].plmn_name);
            fputs(buf_line, fp);

            memset(tmp_str, 0, sizeof(tmp_str));
            memset(buf_line, 0, sizeof(buf_line));
            sprintf(tmp_str, "lte_band%d", i);
            snprintf(buf_line, sizeof(buf_line), "%s=%s\n", tmp_str,
                     g_scan_ctx.scan_cell_band_entry_list.lte_cell_band_entry[i].cell_band);
            fputs(buf_line, fp);
        }
        fclose(fp);
    }
}

/**
 * @brief 保存NR小区频段信息到文件
 *
 * 将NR小区的频段详细信息保存到文件中，包括：
 * - 物理小区ID (PCID)
 * - 绝对射频信道号 (ARFCN)
 * - 参考信号接收功率 (RSRP)
 * - 参考信号接收质量 (RSRQ)
 * - 信噪比 (SNR)
 * - 小区ID
 * - PLMN ID
 * - PLMN名称
 * - 小区频段
 *
 * 保存前会对小区列表进行排序。
 *
 * @return 无返回值
 */
void mobile_nr_cell_band_save(void) {
    char tmp_str[32] = {0};
    char buf_line[128] = {0};
    FILE *fp = fopen(CELL_NR_LIST_FILE, "w+");
    mobile_bubble_sort(g_scan_ctx.scan_cell_band_entry_list.nr_cell_band_entry, g_scan_ctx.scan_cell_band_entry_list.nr_cell_band_entry_num);
    if (fp) {
        for(int i = 0; i < g_scan_ctx.scan_cell_band_entry_list.nr_cell_band_entry_num; i++)
        {
            memset(tmp_str, 0, sizeof(tmp_str));
            memset(buf_line, 0, sizeof(buf_line));
            sprintf(tmp_str, "nr_pcid%d", i);
            snprintf(buf_line, sizeof(buf_line), "%s=%s\n", tmp_str, g_scan_ctx.scan_cell_band_entry_list.nr_cell_band_entry[i].pcid);
            fputs(buf_line, fp);

            memset(tmp_str, 0, sizeof(tmp_str));
            memset(buf_line, 0, sizeof(buf_line));
            sprintf(tmp_str, "nr_arfcn%d", i);
            snprintf(buf_line, sizeof(buf_line), "%s=%s\n", tmp_str, g_scan_ctx.scan_cell_band_entry_list.nr_cell_band_entry[i].arfcn);
            fputs(buf_line, fp);

            memset(tmp_str, 0, sizeof(tmp_str));
            memset(buf_line, 0, sizeof(buf_line));
            sprintf(tmp_str, "nr_rsrp%d", i);
            snprintf(buf_line, sizeof(buf_line), "%s=%d\n", tmp_str, atoi(g_scan_ctx.scan_cell_band_entry_list.nr_cell_band_entry[i].rsrp)/4);
            fputs(buf_line, fp);

            memset(tmp_str, 0, sizeof(tmp_str));
            memset(buf_line, 0, sizeof(buf_line));
            sprintf(tmp_str, "nr_rsrq%d", i);
            snprintf(buf_line, sizeof(buf_line), "%s=%d\n", tmp_str, atoi(g_scan_ctx.scan_cell_band_entry_list.nr_cell_band_entry[i].rsrq)/4);
            fputs(buf_line, fp);

            memset(tmp_str, 0, sizeof(tmp_str));
            memset(buf_line, 0, sizeof(buf_line));
            sprintf(tmp_str, "nr_snr%d", i);
            snprintf(buf_line, sizeof(buf_line), "%s=%d\n", tmp_str, atoi(g_scan_ctx.scan_cell_band_entry_list.nr_cell_band_entry[i].snr)/4);
            fputs(buf_line, fp);

            memset(tmp_str, 0, sizeof(tmp_str));
            memset(buf_line, 0, sizeof(buf_line));
            sprintf(tmp_str, "nr_cellid%d", i);
            snprintf(buf_line, sizeof(buf_line), "%s=%s\n", tmp_str, g_scan_ctx.scan_cell_band_entry_list.nr_cell_band_entry[i].cellid);
            fputs(buf_line, fp);

            memset(tmp_str, 0, sizeof(tmp_str));
            memset(buf_line, 0, sizeof(buf_line));
            sprintf(tmp_str, "nr_plmn_id%d", i);
            snprintf(buf_line, sizeof(buf_line), "%s=%s\n", tmp_str, g_scan_ctx.scan_cell_band_entry_list.nr_cell_band_entry[i].plmn_id);
            fputs(buf_line, fp);

            memset(tmp_str, 0, sizeof(tmp_str));
            memset(buf_line, 0, sizeof(buf_line));
            sprintf(tmp_str, "nr_plmn_name%d", i);
            snprintf(buf_line, sizeof(buf_line), "%s=%s\n", tmp_str, g_scan_ctx.scan_cell_band_entry_list.nr_cell_band_entry[i].plmn_name);
            fputs(buf_line, fp);

            memset(tmp_str, 0, sizeof(tmp_str));
            memset(buf_line, 0, sizeof(buf_line));
            sprintf(tmp_str, "nr_band%d", i);
            snprintf(buf_line, sizeof(buf_line), "%s=%s\n", tmp_str, g_scan_ctx.scan_cell_band_entry_list.nr_cell_band_entry[i].cell_band);
            fputs(buf_line, fp);
            
        }
        fclose(fp);
    }
}

/**
 * 打印小区频段列表信息
 * 打印g_scan_ctx.scan_cell_band_entry_list结构体的内容,即小区扫描内容
 *
 * @return 无返回值
 */
void mobile_print_cell_band_entry_list(void) {
    int i;
    
    MOBILE_INFO("=== Cell Scan Band Entry List Information ===\n");
    MOBILE_INFO("LTE Cell Scan Band Count: %d\n", g_scan_ctx.scan_cell_band_entry_list.lte_cell_band_entry_num);
    MOBILE_INFO("NR Cell Scan Band Count: %d\n", g_scan_ctx.scan_cell_band_entry_list.nr_cell_band_entry_num);
    
    // 打印LTE小区频段信息
    if (g_scan_ctx.scan_cell_band_entry_list.lte_cell_band_entry_num > 0) {
        for (i = 0; i < g_scan_ctx.scan_cell_band_entry_list.lte_cell_band_entry_num; i++) {
            MOBILE_INFO("LTE Band[%d]: RAT=%s, PCID=%s, ARFCN=%s, RSRP=%s, RSRQ=%s, SNR=%s, CellID=%s, PLMN=%s, Band=%s\n", i,
                        g_scan_ctx.scan_cell_band_entry_list.lte_cell_band_entry[i].rat, g_scan_ctx.scan_cell_band_entry_list.lte_cell_band_entry[i].pcid,
                        g_scan_ctx.scan_cell_band_entry_list.lte_cell_band_entry[i].arfcn, g_scan_ctx.scan_cell_band_entry_list.lte_cell_band_entry[i].rsrp,
                        g_scan_ctx.scan_cell_band_entry_list.lte_cell_band_entry[i].rsrq, g_scan_ctx.scan_cell_band_entry_list.lte_cell_band_entry[i].snr,
                        g_scan_ctx.scan_cell_band_entry_list.lte_cell_band_entry[i].cellid, g_scan_ctx.scan_cell_band_entry_list.lte_cell_band_entry[i].plmn_id,
                        g_scan_ctx.scan_cell_band_entry_list.lte_cell_band_entry[i].cell_band);
        }
    }
    
    // 打印NR小区频段信息
    if (g_scan_ctx.scan_cell_band_entry_list.nr_cell_band_entry_num > 0) {
        for (i = 0; i < g_scan_ctx.scan_cell_band_entry_list.nr_cell_band_entry_num; i++) {
            MOBILE_INFO("NR Band[%d]: RAT=%s, PCID=%s, ARFCN=%s, RSRP=%s, RSRQ=%s, SNR=%s, CellID=%s, PLMN=%s, Band=%s\n", i,
                        g_scan_ctx.scan_cell_band_entry_list.nr_cell_band_entry[i].rat, g_scan_ctx.scan_cell_band_entry_list.nr_cell_band_entry[i].pcid,
                        g_scan_ctx.scan_cell_band_entry_list.nr_cell_band_entry[i].arfcn, g_scan_ctx.scan_cell_band_entry_list.nr_cell_band_entry[i].rsrp,
                        g_scan_ctx.scan_cell_band_entry_list.nr_cell_band_entry[i].rsrq, g_scan_ctx.scan_cell_band_entry_list.nr_cell_band_entry[i].snr,
                        g_scan_ctx.scan_cell_band_entry_list.nr_cell_band_entry[i].cellid, g_scan_ctx.scan_cell_band_entry_list.nr_cell_band_entry[i].plmn_id,
                        g_scan_ctx.scan_cell_band_entry_list.nr_cell_band_entry[i].cell_band);
        }
    }
}