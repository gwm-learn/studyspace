#include "produce_led.h"
#include "produce_util.h"

const char* all_led_list[] = {
    "led_wps",
    "led_internet",
    "led_run",
    NULL
};

char **get_led_list(void) {
    return all_led_list;
}

void produce_all_led_on(int param_flag, char* param) {
    system("echo 1 > " LED_FACTORY_MODE_FILE);
    char **led_list = get_led_list();
    for (; *led_list != NULL; led_list++) {
        produce_led_on(0, *led_list);
    }
}

void produce_all_led_off(int param_flag, char* param) {
    char **led_list = get_led_list();
    for (; *led_list != NULL; led_list++) {
        produce_led_off(0, *led_list);
    }
}

void produce_all_led_blink(int param_flag, char* param) {
    char **led_list = get_led_list();
    for (; *led_list != NULL; led_list++) {
        produce_led_blink(0, *led_list);
    }
}

void produce_all_led_list(int param_flag, char* param) {
    char **led_list = get_led_list();
    for (; *led_list != NULL; led_list++) {
        printf("%s\n", *led_list);
    }
}

void produce_led_on(int param_flag, char* param) {
    char file_name[128] = {0};

    if (param == NULL) {
        return;
    }

    snprintf(file_name, sizeof(file_name) - 1, "/sys/class/leds/%s/trigger", param);
    write_buf_to_file(file_name, LED_TRIGGER_ON_STR, sizeof(LED_TRIGGER_ON_STR) - 1);
}

void produce_led_off(int param_flag, char* param) {
    char file_name[128] = {0};

    if (param == NULL) {
        return;
    }

    snprintf(file_name, sizeof(file_name) - 1, "/sys/class/leds/%s/trigger", param);
    write_buf_to_file(file_name, LED_TRIGGER_OFF_STR, sizeof(LED_TRIGGER_OFF_STR) - 1);
}

void produce_led_blink(int param_flag, char* param) {
    char file_name[128] = {0};

    if (param == NULL) {
        return;
    }

    snprintf(file_name, sizeof(file_name) - 1, "/sys/class/leds/%s/trigger", param);
    write_buf_to_file(file_name, LED_TRIGGER_OFF_BLINK, sizeof(LED_TRIGGER_OFF_BLINK) - 1);
}
