#ifndef MOBILE_SERVICE_LED_H
#define MOBILE_SERVICE_LED_H

#define LED_VOIP  "led_voip"
#define LED_VOLTE "led_volte"

int mobile_init_led_service(void);
void mobile_deinit_led_service(void);

void mobile_light_mode_led(const char *led_name, const char *network_type, int on_off);
void mobile_light_signal_led(int signal_mode, int on_off);
void mobile_light_pots_led(int on_off);
void mobile_light_internet_led(int on_off);
void mobile_light_internet_autoled(const char *device_name);

void mobile_update_voice_led(void);

void mobile_update_led_status(void);
void mobile_update_led_force(void);

#endif