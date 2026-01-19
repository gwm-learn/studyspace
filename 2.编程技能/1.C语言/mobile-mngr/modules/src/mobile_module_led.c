#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "mobile_module_util.h"
#include "mobile_module_led.h"

/*
 * 文件名称：mobile_module_led.c
 * 功能描述：
 *     LED控制模块，提供LED设备的sysfs操作接口
 *
 * 作者：gaoweiming
 */

/**
 * @brief 向LED sysfs文件写入数据
 * @param led_name LED设备名称
 * @param attribute 属性名称（如trigger、brightness等）
 * @param value 要写入的值
 * @return 成功返回0，失败返回-1
 *
 * 该函数是其他LED控制函数的基础，负责实际的sysfs文件操作。
 */
int mobile_led_write_sysfs(const char *led_name, const char *attribute, const char *value) {
    char path[BUFLEN_256];
    FILE *fp = NULL;
    int ret = -1;
    
    if (led_name == NULL || attribute == NULL || value == NULL) {
        MOBILE_ERROR("Invalid parameters: led_name=%p, attribute=%p, value=%p\n",
                    led_name, attribute, value);
        return -1;
    }
    
    // 构建完整的sysfs文件路径
    if (snprintf(path, sizeof(path), "%s/%s/%s", LED_SYSFS_PATH, led_name, attribute) >= (int)sizeof(path)) {
        MOBILE_ERROR("Path buffer overflow: led_name=%s, attribute=%s\n", led_name, attribute);
        return -1;
    }

    if (access(path, F_OK) != 0) {
        MOBILE_WARN("led %s is not exist\n", path);
        return -1;
    }
    
    // 打开sysfs文件进行写入
    fp = fopen(path, "w");
    if (fp == NULL) {
        MOBILE_ERROR("Failed to open sysfs file: %s, error: %s\n", path, strerror(errno));
        return -1;
    }
    
    // 写入数据
    if (fprintf(fp, "%s", value) < 0) {
        MOBILE_ERROR("Failed to write to sysfs file: %s, value=%s, error: %s\n",
                    path, value, strerror(errno));
        goto cleanup;
    }
    
    // 刷新文件缓冲区
    if (fflush(fp) != 0) {
        MOBILE_ERROR("Failed to flush sysfs file: %s, error: %s\n", path, strerror(errno));
        goto cleanup;
    }
    
    ret = 0;

cleanup:
    if (fclose(fp) != 0) {
        MOBILE_ERROR("Failed to close sysfs file: %s, error: %s\n", path, strerror(errno));
        if (ret == 0) ret = -1; // 只有在之前成功的情况下才标记为失败
    }
    
    return ret;
}

/**
 * @brief 设置LED触发器
 * @param led_name LED设备名称
 * @param trigger 触发器类型（none、default-on、timer、netdev等）
 * @return 成功返回0，失败返回-1
 *
 * 支持的触发器类型：
 * - "none": 关闭LED
 * - "default-on": 常亮
 * - "timer": 闪烁模式
 * - "netdev": 网络设备模式
 */
int mobile_led_set_trigger(const char *led_name, const char *trigger) {
    if (led_name == NULL || trigger == NULL) {
        MOBILE_ERROR("Invalid parameters: led_name=%p, trigger=%p\n", led_name, trigger);
        return -1;
    }
    
    MOBILE_DEBUG("Setting LED trigger: led=%s, trigger=%s\n", led_name, trigger);
    return mobile_led_write_sysfs(led_name, "trigger", trigger);
}

/**
 * @brief 设置LED亮度
 * @param led_name LED设备名称
 * @param brightness 亮度值：0-关闭，1-开启
 * @return 成功返回0，失败返回-1
 *
 * 直接设置LED的亮度值，0表示关闭，1表示最大亮度。
 */
int mobile_led_set_brightness(const char *led_name, int brightness) {
    char value[16];
    int ret;
    
    if (led_name == NULL) {
        MOBILE_ERROR("Invalid parameter: led_name is NULL\n");
        return -1;
    }
    
    if (brightness < 0 || brightness > 1) {
        MOBILE_ERROR("Invalid brightness value: %d, must be 0 or 1\n", brightness);
        return -1;
    }
    
    ret = snprintf(value, sizeof(value), "%d", brightness);
    if (ret < 0 || ret >= (int)sizeof(value)) {
        MOBILE_ERROR("Value buffer overflow: brightness=%d\n", brightness);
        return -1;
    }
    
    MOBILE_DEBUG("Setting LED brightness: led=%s, brightness=%d\n", led_name, brightness);
    return mobile_led_write_sysfs(led_name, "brightness", value);
}

/**
 * @brief 设置LED闪烁定时器
 * @param led_name LED设备名称
 * @param delay_on 亮灯时间（毫秒）
 * @param delay_off 灭灯时间（毫秒）
 * @return 成功返回0，失败返回-1
 *
 * 设置LED的闪烁模式，需要先设置触发器为"timer"。
 */
int mobile_led_set_timer(const char *led_name, int delay_on, int delay_off) {
    char value[16];
    int ret;
    
    if (led_name == NULL) {
        MOBILE_ERROR("Invalid parameter: led_name is NULL\n");
        return -1;
    }
    
    if (delay_on <= 0 || delay_off <= 0) {
        MOBILE_ERROR("Invalid delay values: delay_on=%d, delay_off=%d\n", delay_on, delay_off);
        return -1;
    }
    
    MOBILE_DEBUG("Setting LED timer: led=%s, delay_on=%d, delay_off=%d\n",
                led_name, delay_on, delay_off);
    
    // 首先设置触发器为timer模式
    ret = mobile_led_set_trigger(led_name, "timer");
    if (ret != 0) {
        MOBILE_ERROR("Failed to set LED trigger to timer: led=%s\n", led_name);
        return ret;
    }
    
    // 设置亮灯时间
    ret = snprintf(value, sizeof(value), "%d", delay_on);
    if (ret < 0 || ret >= (int)sizeof(value)) {
        MOBILE_ERROR("Value buffer overflow: delay_on=%d\n", delay_on);
        return -1;
    }
    
    ret = mobile_led_write_sysfs(led_name, "delay_on", value);
    if (ret != 0) {
        MOBILE_ERROR("Failed to set LED delay_on: led=%s, value=%s\n", led_name, value);
        return ret;
    }
    
    // 设置灭灯时间
    ret = snprintf(value, sizeof(value), "%d", delay_off);
    if (ret < 0 || ret >= (int)sizeof(value)) {
        MOBILE_ERROR("Value buffer overflow: delay_off=%d\n", delay_off);
        return -1;
    }
    
    ret = mobile_led_write_sysfs(led_name, "delay_off", value);
    if (ret != 0) {
        MOBILE_ERROR("Failed to set LED delay_off: led=%s, value=%s\n", led_name, value);
    }
    
    return ret;
}

/**
 * @brief 设置网络设备模式LED
 * @param led_name LED设备名称
 * @param device_name 网络设备名称
 * @return 成功返回0，失败返回-1
 *
 * 将LED设置为网络设备模式，LED状态会根据网络连接和流量自动变化。
 */
int mobile_led_set_netdev(const char *led_name, const char *device_name) {
    int ret;
    
    if (led_name == NULL || device_name == NULL) {
        MOBILE_ERROR("Invalid parameters: led_name=%p, device_name=%p\n", led_name, device_name);
        return -1;
    }
    
    MOBILE_DEBUG("Setting LED netdev mode: led=%s, device=%s\n", led_name, device_name);
    
    // 设置触发器为netdev模式
    ret = mobile_led_set_trigger(led_name, "netdev");
    if (ret != 0) {
        MOBILE_ERROR("Failed to set LED trigger to netdev: led=%s\n", led_name);
        return ret;
    }
    
    // 设置关联的网络设备
    ret = mobile_led_write_sysfs(led_name, "device_name", device_name);
    if (ret != 0) {
        MOBILE_ERROR("Failed to set LED device_name: led=%s, device=%s\n", led_name, device_name);
        return ret;
    }
    
    // 启用link模式
    ret = mobile_led_write_sysfs(led_name, "link", "1");
    if (ret != 0) {
        MOBILE_ERROR("Failed to enable LED link mode: led=%s\n", led_name);
        return ret;
    }
    
    // 启用tx模式
    ret = mobile_led_write_sysfs(led_name, "tx", "1");
    if (ret != 0) {
        MOBILE_ERROR("Failed to enable LED tx mode: led=%s\n", led_name);
        return ret;
    }
    
    // 启用rx模式
    ret = mobile_led_write_sysfs(led_name, "rx", "1");
    if (ret != 0) {
        MOBILE_ERROR("Failed to enable LED rx mode: led=%s\n", led_name);
    }
    
    return ret;
}