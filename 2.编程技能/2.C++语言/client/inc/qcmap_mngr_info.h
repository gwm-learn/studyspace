#ifndef _QCMAP_MNGR_INFO_H_
#define _QCMAP_MNGR_INFO_H_

#include "qcmap_mngr_util.h"

typedef struct {
    char netType[16];
    char tac[16];
    char lac[16];
    char cellid[16];
    char earfcn[16];
    char pcid[16];
    char band[16];
    char bandwidth[16];
    char rsrp[16];
    char rsrq[16];
    char ss_rsrp[16];
    char ss_rsrq[16];
    char rscp[16];
    char sinr[16];
    char tac_rf2[16];
    char cellid_rf2[16];
    char earfcn_rf2[16];
    char pcid_rf2[16];
    char band_rf2[16];
    char bandwidth_rf2[16];
    char sinr_rf2[16];
    char rsrp_rf2[16];
    char rsrq_rf2[16];
} cell_info_t;

typedef struct _CELL_ENTRY {
    char pcid[8];
    char arfcn[8];
    char rsrp[8];
    char rsrq[8];
    char sinr[8];
    char cellid[16];
} CELL_ENTRY;

typedef struct _CELL_ENTRY_LIST {
    int lte_cell_entry_num;
    int nr_cell_entry_num;

    CELL_ENTRY lte_cell_entry[20];
    CELL_ENTRY nr_cell_entry[20];
} CELL_ENTRY_LIST;

extern ST_MODULE_DESC  glb_module_desc;
extern CELL_ENTRY_LIST g_cell_entry_list;

void qcmap_get_sn(void);
void qcmap_get_modem_name(void);
void qcmap_init_modem_info(void);
void qcmap_release_info(void);
void qcmap_mobile_nr_cell_save(void);
void qcmap_mobile_lte_cell_save(void);
void qcmap_mobile_celllist_save(void);
void qcmap_mobile_cell_num_save(void);
void qcmap_get_cell_list(void);
void qcmap_get_imei(void);
void qcmap_get_imsi(void);
void qcmap_get_module_version(void);
void qcmap_get_csq(void);
void qcmap_update_mobile_info(void);
void qcmap_get_iccid(void);
int qcmap_update_signal_level(void);
void qcmap_get_rate(void);
void qcmap_update_more_rf(void);
void qcmap_get_cell_info(void);
void qcmap_update_mobilestatus_rf(void);
void qcmap_update_mobilestatus_rf2(void);
void qcmap_mobile_cellinfo_save(void);
void mobile_lte_cellinfo_save(void);
void mobile_nr5g_sa_cellinfo_save(void);
void mobile_nr5g_nsa_cellinfo_save(void);
void qcmap_timer_check_info(void);
void qcmap_reset_signal(void);

#endif