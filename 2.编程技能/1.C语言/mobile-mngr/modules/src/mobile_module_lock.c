#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "mobile_module_util.h"
#include "mobile_module_lock.h"

/*
 * 文件名称：mobile_module_lock.c
 * 功能描述：
 *     提供基础的功能模块
 */

support_band_list_t g_support_band_list[] = {
    {
        .module_name = "Quectel RG502Q-EA",
        .vid = 0x2c7c,
        .pid = 0x0620,
        .band_list_3g = "1,3,5,6,8,19",
        .band_list_4g = "1,3,5,7,8,17,18,19,20,26,28,32,34,38,39,40,41,42,43",
        .band_list_5g_sa = "1,3,5,7,8,20,28,38,40,41,77,78,79",
        .band_list_5g_nsa = "1,3,5,7,8,20,28,38,40,41,77,78,79"
    },
    {
        .module_name = "Quectel RG500L",
        .vid = 0x2c7c,
        .pid = 0x7003,
        .band_list_3g = "1,5,8",
        .band_list_4g = "1,3,5,7,8,20,28,32,38,40,41,42,43",
        .band_list_5g_sa = "1,3,5,7,8,20,28,38,40,41,77,78",
        .band_list_5g_nsa = "1,3,5,7,8,20,28,38,40,41,77,78"
    },
#ifdef CUS_PARAMS_PRODUCT_TYPE_AX3000
    {
        .module_name = "Quectel RG620T-EU",
        .vid = 0x2c7c,
        .pid = 0x7006,
        .band_list_3g = "1,8",
        .band_list_4g = "1,3,5,7,8,20,28,32,38,40,41,42,43,46",
        .band_list_5g_sa = "1,3,5,7,8,20,28,38,40,41,75,76,77,78",
        .band_list_5g_nsa = "1,3,5,7,8,20,28,38,40,41,75,76,77,78"
    },
#else
    {
        .module_name = "Quectel RG620T-EU",
        .vid = 0x2c7c,
        .pid = 0x7006,
        .band_list_3g = "1,5,8",
        .band_list_4g = "1,3,5,7,8,20,28,32,38,40,41,42,43,46",
        .band_list_5g_sa = "1,3,5,7,8,20,28,38,40,41,75,76,77,78",
        .band_list_5g_nsa = "1,3,5,7,8,20,28,38,40,41,75,76,77,78"
    },
#endif
};

/**
 * @brief 根据PID和VID初始化支持的频段列表
 *
 * 该函数通过对比传入的PID和VID与预定义的设备列表进行匹配，
 * 找到对应的设备配置信息，并将匹配的设备信息地址赋给输出参数。
 *
 * @param pid 产品ID (Product ID)
 * @param vid 厂商ID (Vendor ID)
 * @param support_band_list 输出参数，用于接收匹配的设备频段配置信息的地址
 *
 * @return int 返回匹配结果
 *         - 0: 匹配成功
 *         - -1: 未找到匹配的设备
 */
int mobile_init_support_band_list(unsigned int pid, unsigned int vid, support_band_list_t **support_band_list) {
    int support_band_list_count = sizeof(g_support_band_list) / sizeof(support_band_list_t);
    
    // 遍历预定义的设备列表，查找匹配的PID和VID
    for (int i = 0; i < support_band_list_count; i++) {
        if (g_support_band_list[i].vid == vid && g_support_band_list[i].pid == pid) {
            // 找到匹配的设备，将设备信息地址赋给输出参数
            *support_band_list = &g_support_band_list[i];
            return 0; // 匹配成功
        }
    }
    
    return -1; // 未找到匹配的设备
}