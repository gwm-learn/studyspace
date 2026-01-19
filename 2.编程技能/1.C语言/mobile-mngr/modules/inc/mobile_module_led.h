#ifndef MOBILE_MODULE_LED_H
#define MOBILE_MODULE_LED_H

#define BUFLEN_256 256                    /**< 缓冲区大小定义 */
#define LED_SYSFS_PATH "/sys/class/leds"  /**< LED sysfs路径 */

int mobile_led_write_sysfs(const char *led_name, const char *attribute, const char *value);
int mobile_led_set_trigger(const char *led_name, const char *trigger);
int mobile_led_set_brightness(const char *led_name, int brightness);
int mobile_led_set_timer(const char *led_name, int delay_on, int delay_off);
int mobile_led_set_netdev(const char *led_name, const char *device_name);

#endif