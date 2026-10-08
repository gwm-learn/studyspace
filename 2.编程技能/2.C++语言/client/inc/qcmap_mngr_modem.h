#ifndef _QCMAP_MNGR_MODEM_H_
#define _QCMAP_MNGR_MODEM_H_

#include "qcmap_mngr_util.h"

typedef enum {
    MODEM_NONE = 0,
    MODEM_AT_READY,
    MODEM_SIM_READY,
    MODEM_NETWORK_REGISTERED,
    MODEM_END,
} modem_status_t;

typedef enum {
    SIM_NONE = 0,
    SIM_READY,
    SIM_TIMEOUT,
    SIM_END,
} modem_sim_t;

typedef enum {
    AT_NONE = 0,
    AT_READY,
    AT_TIMEOUT,
    AT_END,
} modem_at_t;

typedef enum {
    NETWORK_NONE = 0,
    NETWORK_REGISTERED,
    NETWORK_NOT_REGISTERED,
    NETWORK_TIMEOUT,
    NETWORK_END,
} modem_network_t;

char *qcmap_signal_num_to_str(int signal);
void qcmap_signal_handler(int signal);

int qcmap_get_ims_status(void);
void qcmap_set_ims_status(int status);
void qcmap_init_ims(int status);

bool qcmap_get_nettype_with_modem(char *value);
void qcmap_sync_profile_with_modem(void);
bool qcmap_sync_nettype_with_modem(void);

int qcmap_check_at(void);
int qcmap_check_sim(void);
int qcmap_check_registration(void);
void qcmap_release_interface(void);
void qcmap_release_modem(void);
void qcmap_print_modem_status(int index);
void qcmap_timer_check_modem(void);
void qcmap_mark_signal_file(void);

#endif