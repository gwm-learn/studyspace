#ifndef __QCMAP_MNGR_H__
#define __QCMAP_MNGR_H__

#include "qcmap_mngr_util.h"

void qcmap_get_profile_num(int* profile_num);
void qcmap_add_profile_num(int old_num);
void qcmap_create_profile(int profile_index);
void qcmap_state_machine_running(void);
void qcmap_init_config(void);
void qcmap_init_signal(void);
void qcmap_init_client(void);
void qcmap_init_mobileap(void);
void qcmap_init_profile(void);
void qcmap_init_basic(void);
void qcmap_init_multi(void);
void qcmap_timer_check_state_machine_running_status(void);
void qcmap_timer_check(void);

#endif