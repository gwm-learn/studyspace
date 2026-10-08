#ifndef _QCMAP_MNGR_LED_H_
#define _QCMAP_MNGR_LED_H_

#include "qcmap_mngr_util.h"

#define LED_TRIGGER_ON_STR "default-on"
#define LED_TRIGGER_OFF_STR "none"

#define QCMAP_NETWORK_TYPE_5G "5G"
#define QCMAP_NETWORK_TYPE_4G "4G"
#define QCMAP_NETWORK_TYPE_LTE "LTE"

enum led_index {
    LED_SIGNALNONE = -1,
    LED_SIGNALLOW,
    LED_SIGNALMIDD,
    LED_SIGNALHIGH,
};

void qcmap_release_led(void);
void qcmap_all_led_off(void);
void qcmap_led_on(char *led_name);
void qcmap_led_off(char *led_name);

void qcmap_rsrp_signal_mode(void);
void qcmap_rssi_signal_mode(void);
bool qcmap_check_signal_mode(void);
void qcmap_timer_check_led(void);

#endif