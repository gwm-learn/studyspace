#include "qcmap_mngr_led.h"
#include "qcmap_mngr_info.h"

char* led_list[] = {
    "led_siglow",//LED_SIGNALLOW - 0
    "led_sigmidd",//LED_SIGNALMIDD - 1
    "led_sighigh"//LED_SIGNALHIGH - 2
};

static int signal_mode = LED_SIGNALNONE;
static int last_signal_mode = LED_SIGNALNONE;

#define NUM_OF_LEDLIST (sizeof(led_list) / sizeof(char*))

void qcmap_release_led(void) {
    signal_mode = LED_SIGNALNONE;
    last_signal_mode = LED_SIGNALNONE;
    qcmap_all_led_off();
}

void qcmap_all_led_off(void) {
    for (int i = 0; i < NUM_OF_LEDLIST; i++) {
        qcmap_led_off(led_list[i]);
    }
}

void qcmap_led_on(char *led_name) {
    char file_name[128] = {0};

    if (led_name != nullptr) {
        snprintf(file_name, sizeof(file_name) - 1, "/sys/class/leds/%s/trigger", led_name);
        write_buf_to_file(file_name, LED_TRIGGER_ON_STR, sizeof(LED_TRIGGER_ON_STR) - 1);
    }
}

void qcmap_led_off(char *led_name){
    char file_name[128] = {0};

    if (led_name != nullptr) {
        snprintf(file_name, sizeof(file_name) - 1, "/sys/class/leds/%s/trigger", led_name);
        write_buf_to_file(file_name, LED_TRIGGER_OFF_STR, sizeof(LED_TRIGGER_OFF_STR) - 1);
    }
}

void qcmap_rsrp_signal_mode(void) {
    int rsrp = 0;

    if (strlen(glb_module_desc.rf.RSRP) > 0) {
        rsrp = atoi(glb_module_desc.rf.RSRP);
        if (rsrp < -115) {
            signal_mode = LED_SIGNALLOW;
        } else if (rsrp < -95) {
            signal_mode = LED_SIGNALMIDD;
        } else {
            signal_mode = LED_SIGNALHIGH;
        }
    } else {
        signal_mode = LED_SIGNALNONE;
    }
}

void qcmap_rssi_signal_mode(void) {
    int rssi = 0;

    if (strlen(glb_module_desc.rf.RSSI) > 0) {
        rssi = atoi(glb_module_desc.rf.RSSI);
    } else if (glb_module_desc.csq > 0) {
        rssi = glb_module_desc.csq;
    } else {
        signal_mode = LED_SIGNALNONE;
        return;
    }

    if (rssi < 12) {
        signal_mode = LED_SIGNALLOW;
    } else if (rssi < 19) {
        signal_mode = LED_SIGNALMIDD;
    } else {
        signal_mode = LED_SIGNALHIGH;
    }
}

bool qcmap_check_signal_mode(void) {
    if (glb_module_desc.wanstatus != NULL && !strcmp(glb_module_desc.wanstatus, DIALD_DISCONNECTED)) {
        signal_mode = LED_SIGNALNONE;
        return true;
    }

    if (strlen(glb_module_desc.rf.netType) > 0) {
        if (strcasestr(glb_module_desc.rf.netType, QCMAP_NETWORK_TYPE_5G)) {
            qcmap_rsrp_signal_mode();
        } else if (strcasestr(glb_module_desc.rf.netType, QCMAP_NETWORK_TYPE_4G) || strcasestr(glb_module_desc.rf.netType, QCMAP_NETWORK_TYPE_LTE) ) {
            if (!access(RSRP_RESULT_FILE, F_OK)) {
                qcmap_rsrp_signal_mode();
            } else {
                qcmap_rssi_signal_mode();
            }
        } else {
            signal_mode = LED_SIGNALNONE;
        }
    } else {
        signal_mode = LED_SIGNALNONE;
    }

    if (last_signal_mode != signal_mode) {
        PRINT_DEBUG("signal mode update, led [%d] change to [%d]\n", last_signal_mode, signal_mode);
        last_signal_mode = signal_mode;
        return true;
    } else {
        return false;
    }
}

void qcmap_timer_check_led(void) {
    if(!qcmap_check_signal_mode())
        return;

    switch (signal_mode) {
        case LED_SIGNALLOW:
            qcmap_led_on(led_list[LED_SIGNALLOW]);
            break;

        case LED_SIGNALMIDD:
            qcmap_led_on(led_list[LED_SIGNALLOW]);
            qcmap_led_on(led_list[LED_SIGNALMIDD]);
            break;

        case LED_SIGNALHIGH:
            qcmap_led_on(led_list[LED_SIGNALLOW]);
            qcmap_led_on(led_list[LED_SIGNALMIDD]);
            qcmap_led_on(led_list[LED_SIGNALHIGH]);
            break;

        case LED_SIGNALNONE:
            qcmap_all_led_off();
            break;

        default:
            qcmap_all_led_off();
            break;
    }
}
