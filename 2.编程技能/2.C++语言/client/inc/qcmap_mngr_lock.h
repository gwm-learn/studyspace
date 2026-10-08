#ifndef _QCMAP_MNGR_LOCK_H_
#define _QCMAP_MNGR_LOCK_H_

#include "qcmap_mngr_util.h"

typedef struct {
    int pinAutoUnlock;
    int pincodeset;
    int pinenable;
    char pinNumber[32];
} pinlock_config_t;

typedef struct _ST_BANDLOCK_CONFIG {
    unsigned char enable;                    /**< ReadWrite */
    char          netType[16];               /**< ReadWrite */
    char          bandList[128];             /**< ReadWrite */
    char          support3GBandList[128];    /**< ReadOnly */
    char          cfg3GBandList[128];        /**< ReadWrite */
    char          support4GBandList[128];    /**< ReadOnly */
    char          cfg4GBandList[128];        /**< ReadWrite */
    char          support5GBandList[128];    /**< ReadOnly */
    char          cfg5GBandList[128];        /**< ReadWrite */
    char          support5GnsaBandList[128]; /**< ReadOnly */
    char          cfg5GnsaBandList[128];     /**< ReadWrite */
    char          Rat_fibo[128];
    char          PreferredAct1_fibo[128];
    char          PreferredAct2_fibo[128];
} ST_BANDLOCK_CONFIG;

typedef struct _ST_SIMLOCK_CONFIG {
    unsigned char enable;          /**< ReadWrite */
    char          MCCMNCList[128]; /**< ReadWrite */
} ST_SIMLOCK_CONFIG;

typedef struct _ST_PCIDLOCK_CONFIG {
    unsigned char enable;      /**< ReadWrite */
    char          netType[16]; /**< ReadWrite */
    char          pcid[16];    /**< ReadWrite */
    char          freq[16];    /**< ReadWrite */
    char          SCS[16];     /**< ReadWrite */
    char          band[16];    /**< ReadWrite */
} ST_PCIDLOCK_CONFIG;

extern pinlock_config_t pinlock_config;

bool qcmap_init_bandlist_info(
    char *support3GBandList ,
    char *support4GBandList ,
    char *support5GBandList ,
    char *support5GnsaBandList ,
    char *Rat_fibo ,
    char *PreferredAct1_fibo ,
    char *PreferredAct2_fibo);
void qcmap_init_bandlist_info_ex(
    char *support3GBandList ,
    char *support4GBandList ,
    char *support5GBandList ,
    char *support5GnsaBandList ,
    char *Rat_fibo ,
    char *PreferredAct1_fibo ,
    char *PreferredAct2_fibo);
void qcmap_init_bandlist_config(void);
int qcmap_apply_bandLock(void *data);
void qcmap_restore_all_band(void);
void qcmap_get_all_band(char *lock_band);
void qcmap_get_all_lock_band(char *lock_band);
void qcmap_simlock_enable(void);
void qcmap_simlock_disable(void);
void qcmap_pcidlock_disable(void);
void qcmap_pcidlock_enable(void);
void qcmap_get_pcidlock_config(void);
bool qcmap_get_current_simlock_setting(int *current_mode, char *current_plmn_list);
bool qcmap_get_current_bandlock_setting(char *current_rat, char *current_preferred_act1, char *current_preferred_act2, char *current_bands);
int qcmap_apply_bandlock_setting(void);
int qcmap_apply_simlock_setting(void);
int qcmap_apply_pcidlock_setting(void);
void qcmap_init_bandlock_setting(void);
void qcmap_init_simlock_setting(void);
void qcmap_init_pcidlock_setting(void);
void qcmap_init_pinlock_config(void);
bool qcmap_init_lock(void);

int qcmap_check_lock_pin(void);
int qcmap_check_lock_sim(void);
int qcmap_check_lock_band(void);
int qcmap_check_lock_pcid(void);
int qcmap_timer_check_lock(void);

#endif