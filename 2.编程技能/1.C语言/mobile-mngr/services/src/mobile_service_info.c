#include "mobile_module_util.h"
#include "mobile_service_modem.h"
#include "mobile_service_info.h"

/*
 * 文件名称：mobile_service_info.c
 * 功能描述：
 *     移动信息服务模块，提供设备信息查询功能
 *
 * 作者：gaoweiming
 */

// 内部状态变量
static int g_info_module_initialized = 0;
cell_cache_t g_cell_cache = {0};

// 线程控制变量
static info_thread_ctrl_t g_info_thread_ctrl = {
    .thread_id = 0,
    .running = 0,
    .running_lock = PTHREAD_MUTEX_INITIALIZER
};

/**
 * 初始化INFO服务模块
 * 提供设备信息查询的初始化框架
 *
 * @return 成功返回0，失败返回错误码
 */
int mobile_init_info_service(void) {
    if (g_info_module_initialized) {
        MOBILE_INFO("info service module already initialized\n");
        return 0;
    }

    if (mobile_init_info_check_task() != 0) {
        MOBILE_ERROR("Failed to initialize info check task\n");
        return -1;
    }

    g_info_module_initialized = 1;
    MOBILE_INFO("info service module initialized successfully\n");
    return 0;
}

/**
 * 清理INFO服务模块资源
 * 提供设备信息查询功能的资源清理
 *
 * @return 无返回值
 */
void mobile_deinit_info_service(void) {
    if (!g_info_module_initialized) {
        return;
    }

    mobile_deinit_info_check_task();
    
    g_info_module_initialized = 0;
    MOBILE_INFO("info service module deinitialized\n");
}

/**
 * 设置线程运行状态（带锁）
 *
 * @param running 运行状态：1-运行，0-停止
 * @return 无返回值
 */
void mobile_set_info_thread_running(int running) {
    pthread_mutex_lock(&g_info_thread_ctrl.running_lock);
    g_info_thread_ctrl.running = running;
    pthread_mutex_unlock(&g_info_thread_ctrl.running_lock);
}

/**
 * 获取线程运行状态（带锁）
 *
 * @return 运行状态：1-运行，0-停止
 */
int mobile_get_info_thread_running(void) {
    int running;
    pthread_mutex_lock(&g_info_thread_ctrl.running_lock);
    running = g_info_thread_ctrl.running;
    pthread_mutex_unlock(&g_info_thread_ctrl.running_lock);
    return running;
}

/**
 * 定时器检查线程循环函数
 * 循环调用 mobile_get_more_info_handler
 *
 * @param context 线程上下文（未使用）
 * @return 线程返回值，总是返回0
 */
unsigned int mobile_info_check_loop(void) {
    MOBILE_INFO("<<<<<********************** Info thread starting **********************>>>>>\n");
    while (mobile_get_info_thread_running()) {
        pthread_testcancel();

        sleep(5);

        if (mobile_get_state_machine_running_status()) {
            MOBILE_DEBUG("State machine is running, skip mobile info\n");
            continue;
        }

        mobile_get_more_info_handler();

        pthread_testcancel();
    }

    mobile_set_info_thread_running(0);
    return 0;
}

/**
 * 创建定时器检查线程
 * 启动定时器检查的后台线程，用于异步执行modem状态检查
 *
 * @return 无返回值
 */
static void mobile_pthread_info_check(void) {
    if (mobile_get_info_thread_running()) {
        MOBILE_WARN("Info check thread is already running\n");
        return;
    }

    mobile_set_info_thread_running(1);
    pthread_create(&g_info_thread_ctrl.thread_id, NULL, (void*)mobile_info_check_loop, NULL);
}

/**
 * 初始化定时器检查任务
 *
 * @return 成功返回0，失败返回错误码
 */
int mobile_init_info_check_task(void) {
    mobile_pthread_info_check();
    return 0;
}

/**
 * 清理定时器检查任务
 */
void mobile_deinit_info_check_task(void) {
    mobile_set_info_thread_running(0);

    pthread_cancel(g_info_thread_ctrl.thread_id);
    // 等待线程完全退出
    pthread_join(g_info_thread_ctrl.thread_id, NULL);
    MOBILE_INFO("<<<<<********************** Info thread stopped **********************>>>>>\n");
}

/**
 * get more info 处理函数
 *
 * @return true - 成功, false - 失败
 */
bool mobile_get_more_info_handler(void) {
    if (g_module_desc.pid == 0x7006 && g_module_desc.vid == 0x2c7c) {
        return mobile_get_more_info_p7006_v2c7c();
    } else {
        return true;
    }
}

/**
 * @brief 获取更多移动网络信息（Quectel模块专用）
 *
 * 综合获取移动网络的各种信息，包括：
 * - 网络配置信息
 * - 小区频段信息保存
 * - 小区信息保存
 * - 小区变更状态更新
 *
 * @return 成功返回true，失败返回false
 */
bool mobile_get_more_info_p7006_v2c7c(void) {

    mobile_get_modem_info();

    if (g_cell_cache.need_update_cellinfo == 1) {
        mobile_cellinfo_save();
        g_cell_cache.need_update_cellinfo = 0;
    }

    if (g_cell_cache.need_update_cell_change_stat == 1) {
        mobile_update_cell_change_stat();
        g_cell_cache.need_update_cell_change_stat = 0;
    }

    return true;
}

/**
 * @brief 获取Quectel 模块网络配置信息（新版本）
 *
 * 使用拆分后的函数获取详细的网络配置信息，包括：
 * - 信号强度信息
 * - 运营商信息
 * - 小区信息
 * - 注册状态
 * - 频段信息
 * - 测量信息
 * - 载波聚合信息
 * - 吞吐率信息
 * - 小区列表信息
 *
 * 支持的网络类型：
 * - NR5G-SA (独立组网)
 * - NR5G-NSA (非独立组网)
 * - LTE
 * - 3G/UMTS
 *
 * @return 无返回值
 */
void mobile_get_modem_info(void) {
    int i = 0;
    int ret = 0;
    char tmp[256] = {0};
    char level_info[32] = {0};

    network_config_params_t params = {0};
    ql_nw_cell_info_t p_cell_info;
    ql_nw_signal_strength_info_t info;
    ql_nw_get_band_info_t p_band_info;
    ql_nw_get_ca_info_resp_t get_ca_info;
    ql_nw_get_meas_info_resp_t get_meas_info;
    ql_nw_mobile_operator_name_info_t p_name_info;
    QL_NW_SIGNAL_STRENGTH_LEVEL_E level = QL_NW_SIGNAL_STRENGTH_LEVEL_NONE;

    ret = ql_nw_init(NW_IPC_MODE_DEFAULT);
    if (ret != QL_NW_SUCCESS) {
        MOBILE_ERROR("Failed to initialize Quectel network module: %d\n", ret);
        return;
    }

    if (mobile_get_signal_strength_info(&info, &level) != QL_NW_SUCCESS) {
        ql_nw_release();
        return;
    }

    mobile_get_signal_strength_level(level, level_info, sizeof(level_info));

    if (mobile_get_operator_info(&p_name_info) != QL_NW_SUCCESS) {
        ql_nw_release();
        return;
    }

    if (mobile_get_network_config_info(&p_cell_info, &p_band_info, &get_meas_info, &get_ca_info) != QL_NW_SUCCESS) {
        ql_nw_release();
        return;
    }

    mobile_process_network_info_by_radio_tech(&p_cell_info, &p_band_info, &get_meas_info, &get_ca_info, &params);

    mobile_process_nr_throughput_indication();

    mobile_save_rf_parameters_to_file(&params);

    mobile_update_global_module_desc(&params, &p_band_info);

    mobile_check_cell_info_change(&params);

    ql_nw_release();
}



/**
 * @brief 获取Quectel模块信号强度信息
 *
 * 使用Quectel API获取WCDMA、LTE、NR5G信号强度信息，
 * 包括RSSI、RSCP、ECIO、RSRP、RSRQ、SNR等参数
 *
 * @param info 信号强度信息结构体指针
 * @param level 信号强度等级指针
 * @return 成功返回QL_NW_SUCCESS，失败返回错误码
 */
int mobile_get_signal_strength_info(ql_nw_signal_strength_info_t *info, QL_NW_SIGNAL_STRENGTH_LEVEL_E *level) {
    int ret = 0;
    int change = 0;
    static ql_nw_signal_strength_info_t last_info = {0};
    
    if (info == NULL || level == NULL) {
        MOBILE_ERROR("Invalid parameters: info or level is NULL\n");
        return -1;
    }
    
    memset(info, 0, sizeof(ql_nw_signal_strength_info_t));
    ret = ql_nw_get_signal_strength(info, level);
    if (ret != QL_NW_SUCCESS) {
        MOBILE_ERROR("Failed to get signal strength: %d\n", ret);
        return ret;
    }
    
    // 打印信号强度信息（只有值发生变化时才打印）
    if (info->has_wcdma) {
        if (info->wcdma.rssi != last_info.wcdma.rssi || info->wcdma.rscp != last_info.wcdma.rscp ||
            info->wcdma.ecio != last_info.wcdma.ecio) {
            //MOBILE_DEBUG("wcdma_sig_info: rssi=%d, rscp=%d, ecio=%d\n", info->wcdma.rssi, info->wcdma.rscp, info->wcdma.ecio);
            change = 1;
        }
    }
    if (info->has_lte) {
        if (info->lte.rssi != last_info.lte.rssi || info->lte.rsrq != last_info.lte.rsrq ||
            info->lte.rsrp != last_info.lte.rsrp || info->lte.snr != last_info.lte.snr) {
            //MOBILE_DEBUG("lte_sig_info: rssi=%d, rsrq=%d, rsrp=%d, snr=%d\n", info->lte.rssi, info->lte.rsrq, info->lte.rsrp, info->lte.snr);
            change = 1;
        }
    }
    if (info->has_nr5g) {
        if (info->nr5g.rssi != last_info.nr5g.rssi || info->nr5g.rsrq != last_info.nr5g.rsrq ||
            info->nr5g.rsrp != last_info.nr5g.rsrp || info->nr5g.snr != last_info.nr5g.snr) {
            //MOBILE_DEBUG("nr5g_sig_info: rssi=%d, rsrq=%d, rsrp=%d, snr=%d\n", info->nr5g.rssi, info->nr5g.rsrq, info->nr5g.rsrp, info->nr5g.snr);
            change = 1;
        }
    }
    
    if (change) {
        memcpy(&last_info, info, sizeof(ql_nw_signal_strength_info_t));
    }

    return QL_NW_SUCCESS;
}

/**
 * @brief 获取网络运营商信息
 *
 * 使用Quectel API获取运营商名称、MCC、MNC等信息
 *
 * @param name_info 运营商信息结构体指针
 * @return 成功返回QL_NW_SUCCESS，失败返回错误码
 */
int mobile_get_operator_info(ql_nw_mobile_operator_name_info_t *name_info) {
    // 静态变量记录上一次的运营商信息
    static ql_nw_mobile_operator_name_info_t last_name_info = {0};
    
    if (name_info == NULL) {
        MOBILE_ERROR("Invalid parameter: name_info is NULL\n");
        return -1;
    }
    
    memset(name_info, 0, sizeof(ql_nw_mobile_operator_name_info_t));
    ql_nw_get_mobile_operator_name(name_info);
    
    // 打印运营商信息（只有值发生变化时才打印）
    if (strcmp(name_info->long_eons, last_name_info.long_eons) != 0 ||
        strcmp(name_info->mcc, last_name_info.mcc) != 0 ||
        strcmp(name_info->mnc, last_name_info.mnc) != 0) {
        MOBILE_DEBUG("long_eons = %s\n", name_info->long_eons);
        MOBILE_DEBUG("mcc = %s\n", name_info->mcc);
        MOBILE_DEBUG("mnc = %s\n", name_info->mnc);
        memcpy(&last_name_info, name_info, sizeof(ql_nw_mobile_operator_name_info_t));
    }
    return QL_NW_SUCCESS;
}

/**
 * @brief 获取网络配置信息
 * 
 * 使用Quectel API获取小区信息、注册状态、频段信息、测量信息、载波聚合信息
 * 
 * @param cell_info 小区信息结构体指针
 * @param band_info 频段信息结构体指针
 * @param meas_info 测量信息结构体指针
 * @param ca_info 载波聚合信息结构体指针
 * @return 成功返回QL_NW_SUCCESS，失败返回错误码
 */
int mobile_get_network_config_info(ql_nw_cell_info_t *cell_info, ql_nw_get_band_info_t *band_info, ql_nw_get_meas_info_resp_t *meas_info, ql_nw_get_ca_info_resp_t *ca_info) {
    if (cell_info == NULL || band_info == NULL || 
        meas_info == NULL || ca_info == NULL) {
        MOBILE_ERROR("Invalid parameters: one or more pointers are NULL\n");
        return -1;
    }
    
    memset(cell_info, 0, sizeof(ql_nw_cell_info_t));
    ql_nw_get_cell_info(cell_info);
    
    memset(band_info, 0, sizeof(ql_nw_get_band_info_t));
    ql_nw_get_band_info(band_info);
    
    memset(meas_info, 0, sizeof(ql_nw_get_meas_info_resp_t));
    ql_nw_get_meas_info(meas_info);
    
    memset(ca_info, 0, sizeof(ql_nw_get_ca_info_resp_t));
    ql_nw_get_ca_info(ca_info);
    
    return QL_NW_SUCCESS;
}

/**
 * @brief 根据无线技术类型处理网络信息
 *
 * 根据无线技术类型调用相应的处理函数，统一处理不同网络类型的信息
 *
 * @param cell_info 小区信息结构体指针
 * @param band_info 频段信息结构体指针
 * @param meas_info 测量信息结构体指针
 * @param ca_info 载波聚合信息结构体指针
 * @param params 网络配置参数结构体指针
 * @return 无返回值
 */
void mobile_process_network_info_by_radio_tech(ql_nw_cell_info_t *cell_info, ql_nw_get_band_info_t *band_info, ql_nw_get_meas_info_resp_t *meas_info, ql_nw_get_ca_info_resp_t *ca_info, network_config_params_t *params) {
    if (cell_info == NULL || band_info == NULL ||
        meas_info == NULL || ca_info == NULL || params == NULL) {
        MOBILE_ERROR("Invalid parameters: one or more pointers are NULL\n");
        return;
    }
    
    if (cell_info->serving_rat == QL_NW_RADIO_TECH_NR5G) {
        mobile_process_nr5g_sa_info(cell_info, band_info, meas_info, ca_info, params);
    } else if (cell_info->serving_rat == QL_NW_RADIO_TECH_NSA5G) {
        mobile_process_nr5g_nsa_info(cell_info, band_info, meas_info, ca_info, params);
    } else if (cell_info->serving_rat == QL_NW_RADIO_TECH_LTE) {
        mobile_process_lte_info(cell_info, band_info, meas_info, ca_info, params);
    } else if (cell_info->serving_rat == QL_NW_RADIO_TECH_UMTS || cell_info->serving_rat == QL_NW_RADIO_TECH_HSPA ||
               cell_info->serving_rat == QL_NW_RADIO_TECH_HSPAP || cell_info->serving_rat == QL_NW_RADIO_TECH_HSUPA ||
               cell_info->serving_rat == QL_NW_RADIO_TECH_HSDPA) {
        mobile_process_3g_info(cell_info, params);
    } else {
        MOBILE_WARN("Unknown radio technology: %d\n", cell_info->serving_rat);
    }

    if (strlen(params->net_type) > 0) {
        mobile_write_buff_to_file(NETWORK_TYPE_FILE, params->net_type, strlen(params->net_type));
    }
}

/**
 * @brief 处理NR5G SA网络类型信息
 *
 * 处理NR5G独立组网(SA)的网络配置信息，使用结构体减少参数数量
 *
 * @param cell_info 小区信息结构体指针
 * @param band_info 频段信息结构体指针
 * @param meas_info 测量信息结构体指针
 * @param ca_info 载波聚合信息结构体指针
 * @param params 网络配置参数结构体指针
 * @return 无返回值
 */
void mobile_process_nr5g_sa_info(ql_nw_cell_info_t *cell_info, ql_nw_get_band_info_t *band_info, ql_nw_get_meas_info_resp_t *meas_info, ql_nw_get_ca_info_resp_t *ca_info, network_config_params_t *params) {
    int i = 0;
    
    if (params == NULL) {
        MOBILE_ERROR("Invalid parameter: params is NULL\n");
        return;
    }
    
    strcpy(params->net_type, "NR5G-SA");
    
    sprintf(params->cellid, "%ld", cell_info->nr_info[i].cid);
    sprintf(params->pcid, "%d", cell_info->nr_info[i].pci);
    sprintf(params->tac, "%x", cell_info->nr_info[i].tac);
    sprintf(params->earfcn, "%d", cell_info->nr_info[i].nr_arfcn);
    sprintf(params->rssi, "%d", cell_info->nr_info[i].rssi);
    sprintf(params->rsrp, "%d", cell_info->nr_info[i].rsrp);
    sprintf(params->rsrq, "%d", cell_info->nr_info[i].rsrq);
    sprintf(params->sinr, "%d", cell_info->nr_info[i].sinr);

    sprintf(params->dl_mcs, "%d", ca_info->nr_pcc_ca_info.pcc_dl_mcs);
    sprintf(params->ul_mcs, "%d", ca_info->nr_pcc_ca_info.pcc_ul_mcs);
    sprintf(params->nr_dl_rate, "%d", ca_info->nr_pcc_ca_info.pcc_dl_tput);
    sprintf(params->nr_ul_rate, "%d", ca_info->nr_pcc_ca_info.pcc_ul_tput);
    sprintf(params->nr_rank, "%d", meas_info->nr_pcc_info.rank_indicator);
    sprintf(g_module_desc.ssb_rsrp, "%d", meas_info->nr_pcc_info.ssb_rsrp_avg);

    memset(params->e_nbid, '\0', sizeof(params->e_nbid));
    mobile_get_gnodeb_by_cid(cell_info->nr_info[i].cid, &params->g_nodeb);

    sprintf(params->band, "%d", band_info->nr_band);
    sprintf(params->bandwidth_str, "%s", band_info->nr_dl_bandwidth);
    sprintf(g_module_desc.rf.cqi, "%d", band_info->nr_cqi);
    memset(g_module_desc.rf2.frequency_band, '\0', sizeof(g_module_desc.rf2.frequency_band));

    sprintf(g_module_desc.rf.rank, "%d", meas_info->nr_pcc_info.rank_indicator);
    sprintf(g_module_desc.rf.rx_mcs, "%d", ca_info->nr_pcc_ca_info.pcc_dl_mcs);
    sprintf(g_module_desc.rf.transmission_mode, "%d", ca_info->nr_pcc_ca_info.pcc_lte_dl_tm);

    // 更新小区列表信息
    g_cell_cache.cell_entry_list.lte_cell_entry_num = 0;
    g_cell_cache.cell_entry_list.nr_cell_entry_num = cell_info->nr_info_len;
    for (i = 0; i < g_cell_cache.cell_entry_list.nr_cell_entry_num; i++) {
        sprintf(g_cell_cache.cell_entry_list.nr_cell_entry[i].pcid, "%d", cell_info->nr_info[i].pci);
        sprintf(g_cell_cache.cell_entry_list.nr_cell_entry[i].arfcn, "%d", cell_info->nr_info[i].nr_arfcn);
        sprintf(g_cell_cache.cell_entry_list.nr_cell_entry[i].rsrp, "%d", cell_info->nr_info[i].rsrp);
        sprintf(g_cell_cache.cell_entry_list.nr_cell_entry[i].rsrq, "%d", cell_info->nr_info[i].rsrq);
        sprintf(g_cell_cache.cell_entry_list.nr_cell_entry[i].sinr, "%d", cell_info->nr_info[i].sinr);
        sprintf(g_cell_cache.cell_entry_list.nr_cell_entry[i].cellid, "%ld", cell_info->nr_info[i].cid);
    }
}

/**
 * @brief 处理NR5G NSA网络类型信息
 *
 * 处理NR5G非独立组网(NSA)的网络配置信息，使用结构体减少参数数量
 *
 * @param cell_info 小区信息结构体指针
 * @param band_info 频段信息结构体指针
 * @param meas_info 测量信息结构体指针
 * @param ca_info 载波聚合信息结构体指针
 * @param params 网络配置参数结构体指针
 * @return 无返回值
 */
void mobile_process_nr5g_nsa_info(ql_nw_cell_info_t *cell_info, ql_nw_get_band_info_t *band_info, ql_nw_get_meas_info_resp_t *meas_info, ql_nw_get_ca_info_resp_t *ca_info, network_config_params_t *params) {
    int i = 0; // local cell
    
    if (params == NULL) {
        MOBILE_ERROR("Invalid parameter: params is NULL\n");
        return;
    }
    
    strcpy(params->net_type, "NR5G-NSA");
    
    // LTE信息
    sprintf(params->cellid, "%d", cell_info->lte_info[i].cid);
    sprintf(params->pcid, "%d", cell_info->lte_info[i].pci);
    sprintf(params->tac, "%x", cell_info->lte_info[i].tac);
    sprintf(params->earfcn, "%d", cell_info->lte_info[i].earfcn);
    sprintf(params->rsrp, "%d", cell_info->lte_info[i].rsrp);
    sprintf(params->rsrq, "%d", cell_info->lte_info[i].rsrq);
    sprintf(params->sinr, "%d", cell_info->lte_info[i].sinr);
    sprintf(params->rssi, "%d", cell_info->lte_info[i].rssi);

    // LTE速率和eNodeB信息
    sprintf(params->e_nbid, "%d", cell_info->lte_info[i].eNodeB_id);
    sprintf(params->lte_dl_rate, "%d", ca_info->lte_pcc_ca_info.pcc_dl_tput);
    sprintf(params->lte_ul_rate, "%d", ca_info->lte_pcc_ca_info.pcc_ul_tput);

    // NR信息
    sprintf(params->dl_mcs, "%d", ca_info->nr_pcc_ca_info.pcc_dl_mcs);
    sprintf(params->ul_mcs, "%d", ca_info->nr_pcc_ca_info.pcc_ul_mcs);
    sprintf(params->nr_dl_rate, "%d", ca_info->nr_pcc_ca_info.pcc_dl_tput);
    sprintf(params->nr_ul_rate, "%d", ca_info->nr_pcc_ca_info.pcc_ul_tput);
    sprintf(params->nr_rank, "%d", meas_info->nr_pcc_info.rank_indicator);
    sprintf(g_module_desc.ssb_rsrp, "%d", meas_info->nr_pcc_info.ssb_rsrp_avg);

    // 频段信息
    sprintf(params->band, "%d", band_info->lte_band);
    sprintf(params->bandwidth_str, "%s", band_info->lte_dl_bandwidth);
    sprintf(g_module_desc.rf.cqi, "%d", band_info->lte_cqi);

    // 更新射频信息
    sprintf(g_module_desc.rf.rank, "%d", meas_info->lte_pcc_info.rank_indicator);
    sprintf(g_module_desc.rf.rx_mcs, "%d", ca_info->lte_pcc_ca_info.pcc_dl_mcs);
    sprintf(g_module_desc.rf.transmission_mode, "%d", ca_info->lte_pcc_ca_info.pcc_lte_dl_tm);

    // NR小区信息（如果有）
    if (cell_info->nr_info_len > 0) {
        i = 0; // local +5g cell
        sprintf(g_module_desc.rf2.physical_cell_id, "%d", cell_info->nr_info[i].pci);
        sprintf(g_module_desc.rf2.dl_earfcn, "%d", cell_info->nr_info[i].nr_arfcn);
        sprintf(g_module_desc.rf2.rsrp, "%d", cell_info->nr_info[i].rsrp);
        sprintf(g_module_desc.rf2.rsrq, "%d", cell_info->nr_info[i].rsrq);
        sprintf(g_module_desc.rf2.rssi, "%d", cell_info->nr_info[i].rssi);
        sprintf(g_module_desc.rf2.sinr, "%d", cell_info->nr_info[i].sinr);
        sprintf(g_module_desc.rf2.global_cell_id, "%ld", cell_info->nr_info[i].cid);
        mobile_get_gnodeb_by_cid(cell_info->nr_info[i].cid, &params->g_nodeb);

        sprintf(g_module_desc.rf2.frequency_band, "%d", band_info->nr_band);
        sprintf(g_module_desc.rf2.cqi, "%d", band_info->nr_cqi);
        sprintf(g_module_desc.rf2.rank, "%d", meas_info->nr_pcc_info.rank_indicator);
        sprintf(g_module_desc.rf2.rx_mcs, "%d", ca_info->nr_pcc_ca_info.pcc_dl_mcs);
        sprintf(g_module_desc.rf2.tx_mcs, "%d", ca_info->nr_pcc_ca_info.pcc_ul_mcs);
        sprintf(g_module_desc.rf2.transmission_mode, "%d", ca_info->nr_pcc_ca_info.pcc_lte_dl_tm);
    } else {
        params->g_nodeb = 0;
    }
    
    // 更新小区列表信息
    g_cell_cache.cell_entry_list.lte_cell_entry_num = cell_info->lte_info_len;
    g_cell_cache.cell_entry_list.nr_cell_entry_num = cell_info->nr_info_len;
    
    for (i = 0; i < g_cell_cache.cell_entry_list.lte_cell_entry_num; i++) {
        sprintf(g_cell_cache.cell_entry_list.lte_cell_entry[i].pcid, "%d", cell_info->lte_info[i].pci);
        sprintf(g_cell_cache.cell_entry_list.lte_cell_entry[i].arfcn, "%d", cell_info->lte_info[i].earfcn);
        sprintf(g_cell_cache.cell_entry_list.lte_cell_entry[i].rsrp, "%d", cell_info->lte_info[i].rsrp);
        sprintf(g_cell_cache.cell_entry_list.lte_cell_entry[i].rsrq, "%d", cell_info->lte_info[i].rsrq);
        sprintf(g_cell_cache.cell_entry_list.lte_cell_entry[i].sinr, "%d", cell_info->lte_info[i].sinr);
        sprintf(g_cell_cache.cell_entry_list.lte_cell_entry[i].cellid, "%d", cell_info->lte_info[i].cid);
    }
    
    for (i = 0; i < g_cell_cache.cell_entry_list.nr_cell_entry_num; i++) {
        sprintf(g_cell_cache.cell_entry_list.nr_cell_entry[i].pcid, "%d", cell_info->nr_info[i].pci);
        sprintf(g_cell_cache.cell_entry_list.nr_cell_entry[i].arfcn, "%d", cell_info->nr_info[i].nr_arfcn);
        sprintf(g_cell_cache.cell_entry_list.nr_cell_entry[i].rsrp, "%d", cell_info->nr_info[i].rsrp);
        sprintf(g_cell_cache.cell_entry_list.nr_cell_entry[i].rsrq, "%d", cell_info->nr_info[i].rsrq);
        sprintf(g_cell_cache.cell_entry_list.nr_cell_entry[i].sinr, "%d", cell_info->nr_info[i].sinr);
        sprintf(g_cell_cache.cell_entry_list.nr_cell_entry[i].cellid, "%ld", cell_info->nr_info[i].cid);
    }
}

/**
 * @brief 处理LTE网络类型信息
 *
 * 处理LTE网络的配置信息，使用结构体减少参数数量
 *
 * @param cell_info 小区信息结构体指针
 * @param band_info 频段信息结构体指针
 * @param meas_info 测量信息结构体指针
 * @param ca_info 载波聚合信息结构体指针
 * @param params 网络配置参数结构体指针
 * @return 无返回值
 */
void mobile_process_lte_info(ql_nw_cell_info_t *cell_info, ql_nw_get_band_info_t *band_info, ql_nw_get_meas_info_resp_t *meas_info, ql_nw_get_ca_info_resp_t *ca_info, network_config_params_t *params) {
    int i = 0; // local cell
    
    if (params == NULL) {
        MOBILE_ERROR("Invalid parameter: params is NULL\n");
        return;
    }
    
    strcpy(params->net_type, "LTE");
    
    sprintf(params->cellid, "%d", cell_info->lte_info[i].cid);
    sprintf(params->pcid, "%d", cell_info->lte_info[i].pci);
    sprintf(params->tac, "%x", cell_info->lte_info[i].tac);
    sprintf(params->earfcn, "%d", cell_info->lte_info[i].earfcn);
    sprintf(params->rsrp, "%d", cell_info->lte_info[i].rsrp);
    sprintf(params->rsrq, "%d", cell_info->lte_info[i].rsrq);
    sprintf(params->sinr, "%d", cell_info->lte_info[i].sinr);
    sprintf(params->rssi, "%d", cell_info->lte_info[i].rssi);

    sprintf(params->e_nbid, "%d", cell_info->lte_info[i].eNodeB_id);
    sprintf(params->lte_dl_rate, "%d", ca_info->lte_pcc_ca_info.pcc_dl_tput);
    sprintf(params->lte_ul_rate, "%d", ca_info->lte_pcc_ca_info.pcc_ul_tput);

    sprintf(params->band, "%d", band_info->lte_band);
    sprintf(params->bandwidth_str, "%s", band_info->lte_dl_bandwidth);
    sprintf(g_module_desc.rf.cqi, "%d", band_info->lte_cqi);
    memset(g_module_desc.rf2.frequency_band, '\0', sizeof(g_module_desc.rf2.frequency_band));

    sprintf(g_module_desc.rf.rank, "%d", meas_info->lte_pcc_info.rank_indicator);
    sprintf(g_module_desc.rf.rx_mcs, "%d", ca_info->lte_pcc_ca_info.pcc_dl_mcs);
    sprintf(g_module_desc.rf.transmission_mode, "%d", ca_info->lte_pcc_ca_info.pcc_lte_dl_tm);

    // NR相关字段清零
    strcpy(params->nr_dl_rate, "0");
    strcpy(params->nr_ul_rate, "0");
    strcpy(params->nr_rank, "0");
    strcpy(params->dl_mcs, "0");
    strcpy(params->ul_mcs, "0");
    params->g_nodeb = 0;
    
    // 更新小区列表信息
    g_cell_cache.cell_entry_list.lte_cell_entry_num = cell_info->lte_info_len;
    g_cell_cache.cell_entry_list.nr_cell_entry_num = 0;
    
    for (i = 0; i < g_cell_cache.cell_entry_list.lte_cell_entry_num; i++) {
        sprintf(g_cell_cache.cell_entry_list.lte_cell_entry[i].pcid, "%d", cell_info->lte_info[i].pci);
        sprintf(g_cell_cache.cell_entry_list.lte_cell_entry[i].arfcn, "%d", cell_info->lte_info[i].earfcn);
        sprintf(g_cell_cache.cell_entry_list.lte_cell_entry[i].rsrp, "%d", cell_info->lte_info[i].rsrp);
        sprintf(g_cell_cache.cell_entry_list.lte_cell_entry[i].rsrq, "%d", cell_info->lte_info[i].rsrq);
        sprintf(g_cell_cache.cell_entry_list.lte_cell_entry[i].sinr, "%d", cell_info->lte_info[i].sinr);
        sprintf(g_cell_cache.cell_entry_list.lte_cell_entry[i].cellid, "%d", cell_info->lte_info[i].cid);
    }
}

/**
 * @brief 处理3G网络类型信息
 *
 * 处理3G/UMTS网络的配置信息，使用结构体减少参数数量
 *
 * @param cell_info 小区信息结构体指针
 * @param params 网络配置参数结构体指针
 * @return 无返回值
 */
void mobile_process_3g_info(ql_nw_cell_info_t *cell_info, network_config_params_t *params) {
    char tmp[256] = {0};
    
    if (params == NULL) {
        MOBILE_ERROR("Invalid parameter: params is NULL\n");
        return;
    }
    
    strcpy(params->net_type, "3G");
    sprintf(params->cellid, "%d", cell_info->umts_info[0].cid);
    sprintf(params->earfcn, "%d", cell_info->umts_info[0].uarfcn);
    sprintf(params->rssi, "%d", cell_info->umts_info[0].rssi);
    sprintf(tmp, "%d", (113 + cell_info->umts_info[0].rssi) / 2);
    g_module_desc.csq = atoi(tmp);
    mobile_write_buff_to_file(RSSI_STATUS_FILE, tmp, strlen(tmp));
    
    // 3G网络不需要设置其他参数，使用默认值
    strcpy(params->pcid, "0");
    strcpy(params->tac, "0");
    strcpy(params->rsrp, "0");
    strcpy(params->rsrq, "0");
    strcpy(params->sinr, "0");
    strcpy(params->band, "0");
    strcpy(params->bandwidth_str, "0");
    strcpy(params->e_nbid, "0");
    strcpy(params->dl_mcs, "0");
    strcpy(params->ul_mcs, "0");
    strcpy(params->lte_dl_rate, "0");
    strcpy(params->lte_ul_rate, "0");
    strcpy(params->nr_dl_rate, "0");
    strcpy(params->nr_ul_rate, "0");
    strcpy(params->nr_rank, "0");
    params->g_nodeb = 0;
}

/**
 * @brief NR吞吐率指示回调函数
 *
 * Quectel NR吞吐率和BLER指示回调函数
 *
 * @param ind_data 指示数据结构体指针
 * @return 无返回值
 */
void mobile_nr_grant_bler_ind_cb(ql_nw_throughput_info_t *ind_data) {
    // 静态变量记录上一次的吞吐率信息
    static ql_nw_throughput_info_t last_ind_data = {0};
    
    if (ind_data == NULL) {
        MOBILE_ERROR("Invalid parameter: ind_data is NULL\n");
        return;
    }
    g_cell_cache.quectel_api_vars.return_flag = 1;

    if (ind_data->dl_bler != last_ind_data.dl_bler || ind_data->ul_bler != last_ind_data.ul_bler ||
        ind_data->dl_grant != last_ind_data.dl_grant || ind_data->ul_grant != last_ind_data.ul_grant) {
        //MOBILE_DEBUG("NR throughput indication received: dl_bler=%d, ul_bler=%d, dl_grant=%d, ul_grant=%d\n", ind_data->dl_bler, ind_data->ul_bler, ind_data->dl_grant, ind_data->ul_grant);
        g_cell_cache.quectel_api_vars.dbler = ind_data->dl_bler;
        g_cell_cache.quectel_api_vars.ubler = ind_data->ul_bler;
        g_cell_cache.quectel_api_vars.dgrant = ind_data->dl_grant;
        g_cell_cache.quectel_api_vars.ugrant = ind_data->ul_grant;
        memcpy(&last_ind_data, ind_data, sizeof(ql_nw_throughput_info_t));
    }
}

/**
 * @brief NR吞吐率指示处理函数
 *
 * 处理NR吞吐率指示的订阅、等待和取消订阅操作：
 * 1. 订阅NR吞吐率指示回调
 * 2. 等待NR吞吐率指示接收（最长2秒）
 * 3. 取消订阅NR吞吐率指示回调
 * 4. 清理状态标志
 *
 * @return 成功返回0，失败返回错误码
 */
int mobile_process_nr_throughput_indication(void) {
    int i = 0;
    
    // 步骤1: 订阅NR吞吐率指示回调
    if (ql_nw_set_nr_throughput_ind_cb(mobile_nr_grant_bler_ind_cb) != 0) {
        MOBILE_ERROR("grant and bler subscribe fail!!!\n");
    }

    // 步骤2: 等待NR吞吐率指示接收（最长2秒）
    for (i = 0; i < 1000 * 20; i++) {  // max 2s
        if (g_cell_cache.quectel_api_vars.return_flag) {
            break;
        }
        usleep(1000);  // 1 ms
    }
    // 步骤3: 清理状态标志
    g_cell_cache.quectel_api_vars.return_flag = 0;
    usleep(1000 * 200);  // 200 ms

    // 步骤4: 取消订阅NR吞吐率指示回调
    if (ql_nw_set_nr_throughput_ind_cb(NULL) != 0) {
        MOBILE_ERROR("grant and bler unsubscribe fail!!!\n");
    }
    
    return 0;
}

/**
 * @brief 检查小区信息变更
 *
 * 比较当前小区信息与上次保存的信息，检测是否发生变化
 * 如果发生变化，则设置相应的更新标志
 *
 * @param params 网络配置参数结构体指针
 * @return 无返回值
 */
void mobile_check_cell_info_change(const network_config_params_t *params) {
    if (params == NULL) {
        MOBILE_ERROR("Invalid parameter: params is NULL\n");
        return;
    }
    
    // 检查小区信息是否发生变化
    if (strcmp(g_cell_cache.last_cell.pcid, g_module_desc.rf.physical_cell_id) ||
        strcmp(g_cell_cache.last_cell.rsrq, g_module_desc.rf.rsrq) ||
        strcmp(g_cell_cache.last_cell.rsrp, g_module_desc.rf.rsrp) ||
        strcmp(g_cell_cache.last_cell.rssi, g_module_desc.rf.rssi) ||
        strcmp(g_cell_cache.last_cell.sinr, g_module_desc.rf.sinr) ||
        strcmp(g_cell_cache.last_cell.band1, g_module_desc.rf.frequency_band) ||
        strcmp(g_cell_cache.last_cell.band2, g_module_desc.rf2.frequency_band) ||
        strcmp(g_cell_cache.last_cell.cellid, g_module_desc.rf.global_cell_id)) {
        
        // 如果小区ID发生变化，设置小区变更状态更新标志
        if (strcmp(g_cell_cache.last_cell.cellid, g_module_desc.rf.global_cell_id)) {
            g_cell_cache.need_update_cell_change_stat = 1;
            MOBILE_DEBUG("g_cell_cache.last_cell.cellid=%s  g_module_desc.rf.global_cell_id=%s\n",
                         g_cell_cache.last_cell.cellid, g_module_desc.rf.global_cell_id);
        }
        
        // 更新缓存的小区信息
        memset(g_cell_cache.last_cell.cellid, 0, sizeof(g_cell_cache.last_cell.cellid));
        sprintf(g_cell_cache.last_cell.cellid, "%s", params->cellid);
        memset(g_cell_cache.last_cell.pcid, 0, sizeof(g_cell_cache.last_cell.pcid));
        sprintf(g_cell_cache.last_cell.pcid, "%s", params->pcid);
        memset(g_cell_cache.last_cell.rsrq, 0, sizeof(g_cell_cache.last_cell.rsrq));
        sprintf(g_cell_cache.last_cell.rsrq, "%s", params->rsrq);
        memset(g_cell_cache.last_cell.rsrp, 0, sizeof(g_cell_cache.last_cell.rsrp));
        sprintf(g_cell_cache.last_cell.rsrp, "%s", params->rsrp);
        memset(g_cell_cache.last_cell.rssi, 0, sizeof(g_cell_cache.last_cell.rssi));
        sprintf(g_cell_cache.last_cell.rssi, "%s", params->rssi);
        memset(g_cell_cache.last_cell.sinr, 0, sizeof(g_cell_cache.last_cell.sinr));
        sprintf(g_cell_cache.last_cell.sinr, "%s", params->sinr);
        memset(g_cell_cache.last_cell.band1, 0, sizeof(g_cell_cache.last_cell.band1));
        sprintf(g_cell_cache.last_cell.band1, "%s", g_module_desc.rf.frequency_band);
        memset(g_cell_cache.last_cell.band2, 0, sizeof(g_cell_cache.last_cell.band2));
        sprintf(g_cell_cache.last_cell.band2, "%s", g_module_desc.rf2.frequency_band);
        
        // 设置小区信息更新标志
        g_cell_cache.need_update_cellinfo = 1;
    }
}

/**
 * @brief 根据小区ID计算gNodeB ID
 *
 * 通过小区ID计算对应的gNodeB ID
 *
 * @param cid 小区ID
 * @param g_nodeb 输出的gNodeB ID指针
 * @return 无返回值
 */
void mobile_get_gnodeb_by_cid(uint64_t cid, uint64_t *g_nodeb) {
    if (g_nodeb == NULL) {
        MOBILE_ERROR("Invalid parameter: g_nodeb is NULL\n");
        return;
    }

    uint64_t s_n = 1;
    uint64_t local_cid = cid;
    int last_bit_1 = 0;
    static uint64_t local_cid_t = 0;

    for (int i = sizeof(uint64_t) * 8 - 1; i >= 0; i--) {
        if (local_cid & (s_n << i)) {
            last_bit_1 = i;
            break;
        }
    }
    *g_nodeb = local_cid >> ((last_bit_1 + 1) - G_NODEB_BIT_LEN);
    if (local_cid_t != cid) {
        MOBILE_DEBUG("Calculated gNodeB ID: %lu from cell ID: %lu\n", *g_nodeb, cid);
        local_cid_t = cid;
    }
}

/**
 * @brief 获取信号强度等级字符串
 *
 * 将信号强度等级枚举值转换为对应的字符串描述
 *
 * @param level 信号强度等级枚举值
 * @param level_info 输出的信号强度等级字符串缓冲区
 * @param buf_size 缓冲区大小
 * @return 成功返回1，失败返回0
 */
int mobile_get_signal_strength_level(QL_NW_SIGNAL_STRENGTH_LEVEL_E level, char *level_info, size_t buf_size) {
    if (level_info == NULL || buf_size < 16) {
        MOBILE_ERROR("Invalid parameters for signal strength level\n");
        return 0;
    }

    static QL_NW_SIGNAL_STRENGTH_LEVEL_E local_level = QL_NW_SIGNAL_STRENGTH_LEVEL_NONE;
    if (local_level == level) {
        return;
    }

    local_level = level;
    
    const char *level_str = NULL;
    
    switch (level) {
        case QL_NW_SIGNAL_STRENGTH_LEVEL_NONE:
            level_str = "None";
            break;
        case QL_NW_SIGNAL_STRENGTH_LEVEL_POOR:
            level_str = "Poor";
            break;
        case QL_NW_SIGNAL_STRENGTH_LEVEL_MODERATE:
            level_str = "Moderate";
            break;
        case QL_NW_SIGNAL_STRENGTH_LEVEL_GOOD:
            level_str = "Good";
            break;
        case QL_NW_SIGNAL_STRENGTH_LEVEL_GREAT:
            level_str = "Great";
            break;
        default:
            level_str = "Unknown";
            break;
    }
    
    strncpy(level_info, level_str, buf_size - 1);
    level_info[buf_size - 1] = '\0';
    //MOBILE_DEBUG("Signal strength level: %s\n", level_info);
    return 1;
}

/**
 * @brief 根据索引获取子载波间隔字符串
 *
 * 将子载波间隔索引转换为对应的字符串描述
 *
 * @param index 子载波间隔索引
 * @param scs_str 输出的子载波间隔字符串缓冲区
 * @return 无返回值
 */
void mobile_get_scs_str_by_index(int index, char *scs_str) {
    if (scs_str == NULL) {
        return;
    }
    
    const char *scs_descriptions[] = {
        "15", "30", "60", "120", "240"
    };
    
    if (index >= 0 && index < (int)(sizeof(scs_descriptions) / sizeof(scs_descriptions[0]))) {
        strcpy(scs_str, scs_descriptions[index]);
    } else {
        strcpy(scs_str, "Unknown");
    }
}

/**
 * @brief 保存RF参数到文件
 *
 * 将网络配置参数保存到HOME_PAGE_NEED_FILE文件中，包括：
 * - 网络类型
 * - eNodeB ID
 * - 下行/上行MCS
 * - LTE/NR下行/上行速率
 * - NR秩指示
 * - NR下行/上行BLER
 * - NR下行/上行授权
 *
 * @param params 网络配置参数结构体指针
 * @return 无返回值
 */
void mobile_save_rf_parameters_to_file(const network_config_params_t *params) {
    char buff[512] = {0};
    int len = 0;
    
    if (params == NULL) {
        MOBILE_ERROR("Invalid parameter: params is NULL\n");
        return;
    }
    
    len = snprintf(buff, sizeof(buff), "netType=%s\n", params->net_type);
    len += snprintf(buff + len, sizeof(buff) - len, "e_NBID=%s\n", params->e_nbid);
    len += snprintf(buff + len, sizeof(buff) - len, "dl_mcs=%s\n", params->dl_mcs);
    len += snprintf(buff + len, sizeof(buff) - len, "ul_mcs=%s\n", params->ul_mcs);
    len += snprintf(buff + len, sizeof(buff) - len, "lte_dl_rate=%s\n", params->lte_dl_rate);
    len += snprintf(buff + len, sizeof(buff) - len, "lte_ul_rate=%s\n", params->lte_ul_rate);
    len += snprintf(buff + len, sizeof(buff) - len, "nr_dl_rate=%s\n", params->nr_dl_rate);
    len += snprintf(buff + len, sizeof(buff) - len, "nr_ul_rate=%s\n", params->nr_ul_rate);
    len += snprintf(buff + len, sizeof(buff) - len, "nr_rank=%s\n", params->nr_rank);
    len += snprintf(buff + len, sizeof(buff) - len, "nr_dl_bler=%d\n", g_cell_cache.quectel_api_vars.dbler);
    len += snprintf(buff + len, sizeof(buff) - len, "nr_ul_bler=%d\n", g_cell_cache.quectel_api_vars.ubler);
    len += snprintf(buff + len, sizeof(buff) - len, "nr_dl_grant=%d\n", g_cell_cache.quectel_api_vars.dgrant);
    len += snprintf(buff + len, sizeof(buff) - len, "nr_ul_grant=%d\n", g_cell_cache.quectel_api_vars.ugrant);
    
    mobile_write_buff_to_file(HOME_PAGE_NEED_FILE, buff, strlen(buff));
}

/**
 * @brief 更新全局模块描述符
 *
 * 根据网络配置参数更新全局模块描述符，包括：
 * - 射频信息
 * - 频段信息
 * - 信号质量参数
 * - 小区信息
 * - 网络参数
 * - 双工模式处理
 * - 扇区ID提取
 *
 * @param params 网络配置参数结构体指针
 * @param band_info 频段信息结构体指针
 * @return 无返回值
 */
void mobile_update_global_module_desc(const network_config_params_t *params, ql_nw_get_band_info_t *band_info) {
    char tmp[256] = {0};
    char* ptr = NULL;
    char duplexmode[32] = {0};
    char sectorID[32] = {0};
    
    if (params == NULL || band_info == NULL) {
        MOBILE_ERROR("Invalid parameters: params or band_info is NULL\n");
        return;
    }
    
    // 处理双工模式
    if (strstr(params->duplexmode, "TDD") != NULL) {
        strcpy(duplexmode, "TDD");
    } else if (strstr(params->duplexmode, "FDD") != NULL) {
        strcpy(duplexmode, "FDD");
    } else {
        strcpy(duplexmode, "");
    }
    
    // 提取扇区ID
    if (strlen(params->cellid) > 2) {
        strncpy(sectorID, params->cellid + strlen(params->cellid) - 2, 2);
    }
    
    // 更新RSRP值
    g_module_desc.rsrp = atoi(params->rsrp);
    mobile_write_buff_to_file(RSRP_RESULT_FILE, params->rsrp, strlen(params->rsrp));
    
    // 保存LTE信号信息到文件
    sprintf(tmp, "duplexmode:%s,band:%s,pcid:%s,cellid:%s", duplexmode, params->band, params->pcid, params->cellid);
    mobile_write_buff_to_file(LTE_SIGNAL_FILE, tmp, strlen(tmp));
    
    // 更新射频信息
    sprintf(g_module_desc.rf.downlink_max_thrp, "%s", band_info->net_ambr_dl);
    sprintf(g_module_desc.rf.uplink_max_thrp, "%s", band_info->net_ambr_ul);
    strncpy(g_module_desc.rf.net_type, params->net_type, sizeof(g_module_desc.rf.net_type) - 1);
    strncpy(g_module_desc.rf.frequency_band, params->band, sizeof(g_module_desc.rf.frequency_band) - 1);
    strncpy(g_module_desc.rf.duplexing_mode, duplexmode, sizeof(g_module_desc.rf.duplexing_mode) - 1);
    strncpy(g_module_desc.rf.rsrq, params->rsrq, sizeof(g_module_desc.rf.rsrq) - 1);
    strncpy(g_module_desc.rf.rsrp, params->rsrp, sizeof(g_module_desc.rf.rsrp) - 1);
    strncpy(g_module_desc.rf.rssi, params->rssi, sizeof(g_module_desc.rf.rssi) - 1);
    strncpy(g_module_desc.rf.sinr, params->sinr, sizeof(g_module_desc.rf.sinr) - 1);
    strncpy(g_module_desc.rf.tac, params->tac, sizeof(g_module_desc.rf.tac) - 1);
    strncpy(g_module_desc.rf.dl_earfcn, params->earfcn, sizeof(g_module_desc.rf.dl_earfcn) - 1);
    strncpy(g_module_desc.rf.bandwidth, params->bandwidth_str, sizeof(g_module_desc.rf.bandwidth) - 1);
    strncpy(g_module_desc.rf.physical_cell_id, params->pcid, sizeof(g_module_desc.rf.physical_cell_id) - 1);
    strncpy(g_module_desc.rf.global_cell_id, params->cellid, sizeof(g_module_desc.rf.global_cell_id) - 1);
    
    // 更新模块描述符其他字段
    strncpy(g_module_desc.frequency_band, params->band, sizeof(g_module_desc.frequency_band) - 1);
    
    memset(tmp, 0, sizeof(tmp));
    mobile_get_scs_str_by_index(0, tmp); // 使用默认值0
    snprintf(g_module_desc.scs, sizeof(g_module_desc.scs) - 1, "%d", atoi(tmp));
    
    snprintf(g_module_desc.eci, sizeof(g_module_desc.eci) - 1, "%ld", strtol(params->cellid, &ptr, 16));
    snprintf(g_module_desc.enb_id, sizeof(g_module_desc.enb_id) - 1, "%ld", strtol(params->e_nbid, &ptr, 16));
    snprintf(g_module_desc.gnb_id, sizeof(g_module_desc.gnb_id) - 1, "%lu", params->g_nodeb);
    
    if (strlen(sectorID) > 0) {
        snprintf(g_module_desc.cell_id, sizeof(g_module_desc.cell_id) - 1, "%ld", strtol(sectorID, &ptr, 16));
    }
}

/**
 * 打印小区列表信息
 * 打印g_cell_cache.cell_entry_list结构体的内容，即小区刷新内容
 *
 * @return 无返回值
 */
void mobile_print_cell_entry_list(void) {
    int i;
    
    MOBILE_INFO("=== Cell Entry List Information ===\n");
    MOBILE_INFO("LTE Cell Count: %d\n", g_cell_cache.cell_entry_list.lte_cell_entry_num);
    MOBILE_INFO("NR Cell Count: %d\n", g_cell_cache.cell_entry_list.nr_cell_entry_num);
    
    // 打印LTE小区信息
    if (g_cell_cache.cell_entry_list.lte_cell_entry_num > 0) {
        for (i = 0; i < g_cell_cache.cell_entry_list.lte_cell_entry_num; i++) {
            MOBILE_INFO("LTE Cell[%d]: PCID=%s, ARFCN=%s, RSRP=%s, RSRQ=%s, SINR=%s, CellID=%s\n", i,
                        g_cell_cache.cell_entry_list.lte_cell_entry[i].pcid, g_cell_cache.cell_entry_list.lte_cell_entry[i].arfcn, 
                        g_cell_cache.cell_entry_list.lte_cell_entry[i].rsrp, g_cell_cache.cell_entry_list.lte_cell_entry[i].rsrq, 
                        g_cell_cache.cell_entry_list.lte_cell_entry[i].sinr, g_cell_cache.cell_entry_list.lte_cell_entry[i].cellid);
        }
    }
    
    // 打印NR小区信息
    if (g_cell_cache.cell_entry_list.nr_cell_entry_num > 0) {
        for (i = 0; i < g_cell_cache.cell_entry_list.nr_cell_entry_num; i++) {
            MOBILE_INFO("NR Cell[%d]: PCID=%s, ARFCN=%s, RSRP=%s, RSRQ=%s, SINR=%s, CellID=%s\n", i,
                        g_cell_cache.cell_entry_list.nr_cell_entry[i].pcid, g_cell_cache.cell_entry_list.nr_cell_entry[i].arfcn,
                        g_cell_cache.cell_entry_list.nr_cell_entry[i].rsrp, g_cell_cache.cell_entry_list.nr_cell_entry[i].rsrq, 
                        g_cell_cache.cell_entry_list.nr_cell_entry[i].sinr, g_cell_cache.cell_entry_list.nr_cell_entry[i].cellid);
        }
    }
}
