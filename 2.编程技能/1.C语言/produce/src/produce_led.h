#ifndef __PRODUCE_LED_H__
#define __PRODUCE_LED_H__

#define LED_TRIGGER_ON_STR "default-on"
#define LED_TRIGGER_OFF_STR "none"
#define LED_TRIGGER_OFF_BLINK "timer"

char **get_led_list(void);
void produce_all_led_on(int param_flag, char* param);
void produce_all_led_off(int param_flag, char* param);
void produce_all_led_blink(int param_flag, char* param);
void produce_all_led_list(int param_flag, char* param);
void produce_led_on(int param_flag, char* param);
void produce_led_off(int param_flag, char* param);
void produce_led_blink(int param_flag, char* param);

#endif