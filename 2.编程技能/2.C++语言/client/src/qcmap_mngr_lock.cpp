#include "qcmap_mngr_lock.h"
#include "qcmap_mngr_info.h"

#define SIMLOCK_DEFAULT_PASSWORD "12345678"
#define MAX_PCIDCELL_LOCK_NUM    64

pinlock_config_t pinlock_config;
ST_BANDLOCK_CONFIG bandlock_config;
ST_SIMLOCK_CONFIG simlock_config;
ST_PCIDLOCK_CONFIG pcidlock_config;

bool qcmap_init_bandlist_info(
    char *support3GBandList ,
    char *support4GBandList ,
    char *support5GBandList ,
    char *support5GnsaBandList ,
    char *Rat_fibo ,
    char *PreferredAct1_fibo ,
    char *PreferredAct2_fibo) {

    bool ret = true;
    int count = 0;
    FILE *fp  = NULL;
    char *ptr_s = NULL;
    char *ptr_e = NULL;
    char buf[1024] = {0};
    char bandinfo[9][128] = {0};

    PRINT_DEBUG("get support bandlist info\n");

    fp = popen("at-mngr -i 2 AT+GTACT=?", "r");
    if (!fp) {
        PRINT_DEBUG("run at-mngr AT+GTACT=? fail\n");
        return;
    }

    while ((fgets(buf, sizeof(buf), fp)) != NULL) {
        if (!(ptr_s = strstr(buf, "+GTACT:")))
            continue;
        while((ptr_s = strstr(ptr_s, "(")) && (ptr_e = strstr(ptr_s, ")"))) {
            *ptr_e = '\0';
            strncpy(bandinfo[count], ptr_s + 1, 128);
            if (++count == 9)
                break;
            ptr_s = ptr_e + 1;
        }
    }
    pclose(fp);

    strncpy(Rat_fibo, bandinfo[0], 128);
    strncpy(PreferredAct1_fibo, bandinfo[1], 128);
    strncpy(PreferredAct2_fibo, bandinfo[2], 128);
    strncpy(support3GBandList, bandinfo[4], 128);
    strncpy(support4GBandList, bandinfo[5], 128);
    strncpy(support5GBandList, bandinfo[8], 128);
    strncpy(support5GnsaBandList, bandinfo[8], 128);

    if (bandinfo[8][0] == 0) {
        ret = false;
    }

    return ret;
}

void qcmap_init_bandlist_info_ex(
    char *support3GBandList ,
    char *support4GBandList ,
    char *support5GBandList ,
    char *support5GnsaBandList ,
    char *Rat_fibo ,
    char *PreferredAct1_fibo ,
    char *PreferredAct2_fibo) {

    PRINT_DEBUG("get support bandlist info ex\n");
    PRINT_DEBUG("SUB_PROJECT : [%s]\n", CUS_PARAMS_SUB_PROJECT);
    if (CUS_PARAMS_SUB_PROJECT, "PRJ_FG190W_EAU_00_00") {
        sprintf(Rat_fibo, "%s", "1,2,4,10,14,16,17,20");
        sprintf(PreferredAct1_fibo, "%s", "2,3,6");
        sprintf(PreferredAct2_fibo, "%s", "2,3,6");
        sprintf(support3GBandList, "%s", "1,5,8");
        sprintf(support4GBandList, "%s", "101,103,105,107,108,120,128,132,138,140,141,142,143");
        sprintf(support5GBandList, "%s", "501,503,505,507,508,5020,5026,5028,5038,5040,5041,5075,5077,5078,5257,5258,5260,5261");
        sprintf(support5GnsaBandList, "%s", "501,503,505,507,508,5020,5026,5028,5038,5040,5041,5075,5077,5078,5257,5258,5260,5261");
    } else if (CUS_PARAMS_SUB_PROJECT, "PRJ_FG190W_NA_00_00") {
        sprintf(Rat_fibo, "%s", "2,10,14,17");
        sprintf(PreferredAct1_fibo, "%s", "3,6");
        sprintf(PreferredAct2_fibo, "%s", "");
        sprintf(support3GBandList, "%s", "");
        sprintf(support4GBandList, "%s", "102,104,105,107,112,113,114,117,125,126,129,130,138,141,142,143,148,166,171");
        sprintf(support5GBandList, "%s", "502,505,507,5012,5013,5014,5025,5026,5029,5030,5038,5041,5048,5066,5070,5071,5077,5078,5257,5258,5260,5261");
        sprintf(support5GnsaBandList, "%s", "502,505,507,5012,5013,5014,5025,5026,5029,5030,5038,5041,5048,5066,5070,5071,5077,5078,5257,5258,5260,5261");
    }
}

void qcmap_init_bandlist_config(void) {
    char support2GBandList[128]    = {0};
    char support3GBandList[128]    = {0};
    char support4GBandList[128]    = {0};
    char support5GBandList[128]    = {0};
    char support5GnsaBandList[128] = {0};
    char Rat_fibo[128]             = {0};
    char PreferredAct1_fibo[128]   = {0};
    char PreferredAct2_fibo[128]   = {0};

    PRINT_DEBUG("init band info\n");

// 直接使用bandlock_config结构体替代局部变量
    qcmap_init_bandlist_info_ex(bandlock_config.support3GBandList,
                               bandlock_config.support4GBandList,
                               bandlock_config.support5GBandList,
                               bandlock_config.support5GnsaBandList,
                               bandlock_config.Rat_fibo,
                               bandlock_config.PreferredAct1_fibo,
                               bandlock_config.PreferredAct2_fibo);

    PRINT_DEBUG("Rat_fibo             =[%s]\n", bandlock_config.Rat_fibo);
    PRINT_DEBUG("PreferredAct1_fibo   =[%s]\n", bandlock_config.PreferredAct1_fibo);
    PRINT_DEBUG("PreferredAct2_fibo   =[%s]\n", bandlock_config.PreferredAct2_fibo);
    PRINT_DEBUG("support3GBandList    =[%s]\n", bandlock_config.support3GBandList);
    PRINT_DEBUG("support4GBandList    =[%s]\n", bandlock_config.support4GBandList);
    PRINT_DEBUG("support5GBandList    =[%s]\n", bandlock_config.support5GBandList);
    PRINT_DEBUG("support5GNsaBandList =[%s]\n", bandlock_config.support5GnsaBandList);

    if (access(BANDLIST_INFOS_FILE, F_OK) == 0) {
        unlink(BANDLIST_INFOS_FILE);
    }

    if (bandlock_config.support3GBandList[0] || bandlock_config.support4GBandList[0] ||
        bandlock_config.support5GBandList[0] || bandlock_config.support5GnsaBandList[0]) {
        FILE *fp = fopen(BANDLIST_INFOS_FILE, "w+");
        if (fp) {
            char bufLine[128] = {0};

            if (bandlock_config.Rat_fibo[0]) {
                snprintf(bufLine, sizeof(bufLine), "Rat_fibo=%s\n", bandlock_config.Rat_fibo);
                fputs(bufLine, fp);
            }

            if (bandlock_config.PreferredAct1_fibo[0]) {
                snprintf(bufLine, sizeof(bufLine), "PreferredAct1_fibo=%s\n", bandlock_config.PreferredAct1_fibo);
                fputs(bufLine, fp);
            }

            if (bandlock_config.PreferredAct2_fibo[0]) {
                snprintf(bufLine, sizeof(bufLine), "PreferredAct2_fibo=%s\n", bandlock_config.PreferredAct2_fibo);
                fputs(bufLine, fp);
            }

            if (bandlock_config.support3GBandList[0]) {
                snprintf(bufLine, sizeof(bufLine), "support3GBandList=%s\n", bandlock_config.support3GBandList);
                fputs(bufLine, fp);
            }

            if (bandlock_config.support4GBandList[0]) {
                snprintf(bufLine, sizeof(bufLine), "support4GBandList=%s\n", bandlock_config.support4GBandList);
                fputs(bufLine, fp);
            }

            if (bandlock_config.support5GBandList[0]) {
                snprintf(bufLine, sizeof(bufLine), "support5GBandList=%s\n", bandlock_config.support5GBandList);
                fputs(bufLine, fp);
            }

            if (bandlock_config.support5GnsaBandList[0]) {
                snprintf(bufLine, sizeof(bufLine), "support5GnsaBandList=%s\n", bandlock_config.support5GnsaBandList);
                fputs(bufLine, fp);
            }

            fclose(fp);
        }
    }
}

int qcmap_apply_bandLock(void *data) {
    char param[16] = {0};
    char cmd[2048]  = {0};
    char *lock_band = (char *)data;

    if (lock_band && strlen(lock_band) > 0) {
        if (!strcmp(qcmap_wan5g_list[0].nettype, "AUTO")) {
            sprintf(param, "%s", "10");
        } else if (!strcmp(qcmap_wan5g_list[0].nettype, "5G")) {
            if (!strcmp(qcmap_wan5g_list[0].subnettype, "AUTO")) {
                sprintf(param, "%s", "17,6");
            } else if (!strcmp(qcmap_wan5g_list[0].subnettype, "SA")) {
                sprintf(param, "%s", "14");
            } else if (!strcmp(qcmap_wan5g_list[0].subnettype, "NSA")) {
                sprintf(param, "%s", "17,6");
            } else {
                PRINT_DEBUG("nettype [%s] subnettype [%s] not suppport\n", qcmap_wan5g_list[0].nettype, qcmap_wan5g_list[0].subnettype);
                return true;
            }
            PRINT_DEBUG("setting nettype [%s] subnettype [%s]\n", qcmap_wan5g_list[0].nettype, qcmap_wan5g_list[0].subnettype);
        } else {
            if (!strcmp(qcmap_wan5g_list[0].nettype, "4G")) {
                sprintf(param, "%s", "3");
            } else if (!strcmp(qcmap_wan5g_list[0].nettype, "3G")) {
                sprintf(param, "%s", "2");
            } else {
                PRINT_DEBUG("nettype [%s] not suppport\n", qcmap_wan5g_list[0].nettype);
                return true;
            }
            PRINT_DEBUG("setting nettype [%s]\n", qcmap_wan5g_list[0].nettype);
        }

        sprintf(cmd, "at-mngr 'AT+GTACT=%s,,,%s'", param, lock_band);
        system(cmd);
        PRINT_DEBUG("%s,%d,%s\n", __func__, __LINE__, cmd);
    }

    return 0;
}

void qcmap_restore_all_band(void) {
    char lock_band[256] = {0};

    qcmap_get_all_band(lock_band);

    if (strlen(lock_band) > 0) {
        qcmap_apply_bandLock(lock_band);
        PRINT_DEBUG("restore all band [%s] success\n", lock_band);
    }
}

/**
 * @brief 根据RAT值添加对应的频段到频段列表
 * @param lock_band 输出参数，频段列表
 * @param rat_value RAT值
 * @param first_band 是否是第一个频段
 * @return 更新后的first_band状态
 */
static bool qcmap_add_bands_by_rat(char *lock_band, int rat_value, bool first_band) {
    switch (rat_value) {
        case 2: // UMTS (3G)
            if (strlen(bandlock_config.support3GBandList) > 0) {
                if (!first_band) strcat(lock_band, ",");
                strcat(lock_band, bandlock_config.support3GBandList);
                first_band = false;
            }
            break;
            
        case 3: // LTE (4G)
            if (strlen(bandlock_config.support4GBandList) > 0) {
                if (!first_band) strcat(lock_band, ",");
                strcat(lock_band, bandlock_config.support4GBandList);
                first_band = false;
            }
            break;
            
        case 4: // LTE/UMTS (4G+3G)
            if (strlen(bandlock_config.support3GBandList) > 0) {
                if (!first_band) strcat(lock_band, ",");
                strcat(lock_band, bandlock_config.support3GBandList);
                first_band = false;
            }
            if (strlen(bandlock_config.support4GBandList) > 0) {
                if (!first_band) strcat(lock_band, ",");
                strcat(lock_band, bandlock_config.support4GBandList);
                first_band = false;
            }
            break;
            
        case 10: // Automatic (所有频段)
            if (strlen(bandlock_config.support3GBandList) > 0) {
                if (!first_band) strcat(lock_band, ",");
                strcat(lock_band, bandlock_config.support3GBandList);
                first_band = false;
            }
            if (strlen(bandlock_config.support4GBandList) > 0) {
                if (!first_band) strcat(lock_band, ",");
                strcat(lock_band, bandlock_config.support4GBandList);
                first_band = false;
            }
            if (strlen(bandlock_config.support5GBandList) > 0) {
                if (!first_band) strcat(lock_band, ",");
                strcat(lock_band, bandlock_config.support5GBandList);
                first_band = false;
            }
            break;
            
        case 14: // NR-RAN (5G SA)
            if (strlen(bandlock_config.support5GBandList) > 0) {
                if (!first_band) strcat(lock_band, ",");
                strcat(lock_band, bandlock_config.support5GBandList);
                first_band = false;
            }
            break;
            
        case 16: // NR-RAN/WCDMA (5G NSA + 3G)
            if (strlen(bandlock_config.support3GBandList) > 0) {
                if (!first_band) strcat(lock_band, ",");
                strcat(lock_band, bandlock_config.support3GBandList);
                first_band = false;
            }
            if (strlen(bandlock_config.support5GBandList) > 0) {
                if (!first_band) strcat(lock_band, ",");
                strcat(lock_band, bandlock_config.support5GBandList);
                first_band = false;
            }
            break;
            
        case 17: // NR-RAN/LTE (5G NSA + 4G)
            if (strlen(bandlock_config.support4GBandList) > 0) {
                if (!first_band) strcat(lock_band, ",");
                strcat(lock_band, bandlock_config.support4GBandList);
                first_band = false;
            }
            if (strlen(bandlock_config.support5GBandList) > 0) {
                if (!first_band) strcat(lock_band, ",");
                strcat(lock_band, bandlock_config.support5GBandList);
                first_band = false;
            }
            break;
            
        case 20: // NR-RAN/WCDMA/LTE (5G NSA + 4G + 3G)
            if (strlen(bandlock_config.support3GBandList) > 0) {
                if (!first_band) strcat(lock_band, ",");
                strcat(lock_band, bandlock_config.support3GBandList);
                first_band = false;
            }
            if (strlen(bandlock_config.support4GBandList) > 0) {
                if (!first_band) strcat(lock_band, ",");
                strcat(lock_band, bandlock_config.support4GBandList);
                first_band = false;
            }
            if (strlen(bandlock_config.support5GBandList) > 0) {
                if (!first_band) strcat(lock_band, ",");
                strcat(lock_band, bandlock_config.support5GBandList);
                first_band = false;
            }
            break;
            
        default:
            PRINT_DEBUG("Unknown RAT value: %d\n", rat_value);
            break;
    }
    
    return first_band;
}

/**
 * @brief 获取所有支持的频段（默认逻辑）
 * @param lock_band 输出参数，频段列表
 */
static void qcmap_get_all_bands_default(char *lock_band) {
    if (strlen(bandlock_config.support3GBandList) > 0) {
        strcat(lock_band, bandlock_config.support3GBandList);
    }

    if (strlen(bandlock_config.support4GBandList) > 0) {
        if (strlen(lock_band) > 0) {
            strcat(lock_band, ",");
            strcat(lock_band, bandlock_config.support4GBandList);
        } else {
            strcat(lock_band, bandlock_config.support4GBandList);
        }
    }

    if (strlen(bandlock_config.support5GBandList) > 0) {
        if (strlen(lock_band) > 0) {
            strcat(lock_band, ",");
            strcat(lock_band, bandlock_config.support5GBandList);
        } else {
            strcat(lock_band, bandlock_config.support5GBandList);
        }
    }
}

/**
 * @brief 根据nettype_num获取对应RAT的所有频段
 * @param lock_band 输出参数，频段列表
 */
void qcmap_get_all_band(char *lock_band) {
    // 如果nettype_num为空，则使用原来的逻辑获取所有频段
    if (strlen(qcmap_wan5g_list[0].nettype_num) == 0) {
        qcmap_get_all_bands_default(lock_band);
        PRINT_DEBUG("Using default all bands: [%s]\n", lock_band);
        return;
    }

    // 根据nettype_num获取对应RAT的频段
    char *nettype_num = qcmap_wan5g_list[0].nettype_num;
    PRINT_DEBUG("Getting bands for nettype_num: [%s]\n", nettype_num);

    // 解析nettype_num，格式为 <Act>[,<PreferredAct1>[,<PreferredAct2>]]
    // 我们只需要第一个参数 Act 作为 RAT 值
    char rat_list[32] = {0};
    strncpy(rat_list, nettype_num, sizeof(rat_list) - 1);
    
    // 只取第一个逗号前的值作为 RAT
    char *comma_pos = strchr(rat_list, ',');
    if (comma_pos != NULL) {
        *comma_pos = '\0'; // 截断字符串，只保留 Act 部分
    }
    
    int rat_value = atoi(rat_list);
    bool first_band = true;
    first_band = qcmap_add_bands_by_rat(lock_band, rat_value, first_band);
    
    PRINT_DEBUG("Selected bands for nettype_num [%s] (Act=%d): [%s]\n", nettype_num, rat_value, lock_band);
}

/**
 * @brief 对频段字符串进行排序
 * @param bands_str 输入输出参数，频段字符串（逗号分隔）
 */
static void qcmap_sort_bands(char *bands_str) {
    if (!bands_str || strlen(bands_str) == 0) {
        return;
    }
    
    // 将频段字符串分割为数组
    char bands_copy[256] = {0};
    strncpy(bands_copy, bands_str, sizeof(bands_copy) - 1);
    
    char *bands_array[64] = {0};
    int band_count = 0;
    char *token = strtok(bands_copy, ",");
    
    while (token && band_count < 64) {
        bands_array[band_count++] = token;
        token = strtok(NULL, ",");
    }
    
    if (band_count <= 1) {
        return; // 不需要排序
    }
    
    // 对频段进行排序（按数值大小）
    for (int i = 0; i < band_count - 1; i++) {
        for (int j = 0; j < band_count - i - 1; j++) {
            int band1 = atoi(bands_array[j]);
            int band2 = atoi(bands_array[j + 1]);
            if (band1 > band2) {
                char *temp = bands_array[j];
                bands_array[j] = bands_array[j + 1];
                bands_array[j + 1] = temp;
            }
        }
    }
    
    // 重新构建排序后的频段字符串
    memset(bands_str, 0, 256);
    for (int i = 0; i < band_count; i++) {
        if (i > 0) {
            strcat(bands_str, ",");
        }
        strcat(bands_str, bands_array[i]);
    }
}

void qcmap_get_all_lock_band(char *lock_band) {
    char temp_bands[256] = {0};
    
    if (strlen(bandlock_config.cfg3GBandList) > 0) {
        if (strlen(temp_bands) > 0) {
            strcat(temp_bands, ",");
        }
        strcat(temp_bands, bandlock_config.cfg3GBandList);
    }

    if (strlen(bandlock_config.cfg4GBandList) > 0) {
        if (strlen(temp_bands) > 0) {
            strcat(temp_bands, ",");
        }
        strcat(temp_bands, bandlock_config.cfg4GBandList);
    }

    if (strlen(bandlock_config.cfg5GBandList) > 0) {
        if (strlen(temp_bands) > 0) {
            strcat(temp_bands, ",");
        }
        strcat(temp_bands, bandlock_config.cfg5GBandList);
    }

    if (strlen(bandlock_config.cfg5GnsaBandList) > 0) {
        if (strlen(temp_bands) > 0) {
            strcat(temp_bands, ",");
        }
        strcat(temp_bands, bandlock_config.cfg5GnsaBandList);
    }
    
    // 对频段进行排序
    qcmap_sort_bands(temp_bands);
    
    // 复制排序后的结果到输出参数
    strncpy(lock_band, temp_bands, 256);
}

/**
 * @brief 获取当前系统的PLMN锁设置
 * @param current_mode 输出参数，当前模式
 * @param current_plmn_list 输出参数，当前PLMN列表
 * @return true 成功获取设置，false 获取失败
 */
bool qcmap_get_current_simlock_setting(int *current_mode, char *current_plmn_list) {
    char result[QCMAP_MAX_STRING_LEN] = {0};
    char command[QCMAP_MAX_STRING_LEN] = {0};
    char *ptr = NULL;
    char *end_ptr = NULL;
    
    if (!current_mode || !current_plmn_list) {
        PRINT_DEBUG("Invalid parameters for qcmap_get_current_simlock_setting\n");
        return false;
    }
    
    // 初始化输出参数
    *current_mode = 0;
    memset(current_plmn_list, 0, QCMAP_MAX_STRING_LEN);
    
    // 发送查询命令 AT+GTPLMNLOCK?
    snprintf(command, QCMAP_MAX_STRING_LEN, "%s -i 2 %s?", QCMAP_MNGR_AT_MNGR_COMMAND, QCMAP_MNGR_AT_COMMAND_PLMNLOCK);
    
    if (!execute_cmd((const char*)command, result, sizeof(result))) {
        PRINT_DEBUG("Failed to execute simlock query command\n");
        return false;
    }
    
    // 解析响应格式: +GTPLMNLOCK: <mode>[,<PLMN List>]
    ptr = strstr(result, "+GTPLMNLOCK: ");
    if (!ptr) {
        PRINT_DEBUG("No valid simlock response found\n");
        return false;
    }
    
    // 跳过 "+GTPLMNLOCK:" 前缀
    ptr += strlen("+GTPLMNLOCK: ");
    
    // 解析模式
    *current_mode = strtol(ptr, &end_ptr, 10);
    if (end_ptr == ptr) {
        PRINT_DEBUG("Failed to parse simlock mode\n");
        return false;
    }
    
    ptr = end_ptr;
    
    // 如果有PLMN列表，解析PLMN列表
    if (*ptr == ',') {
        ptr++; // 跳过逗号
        
        // 查找PLMN列表的结束位置（换行符或字符串结束）
        end_ptr = strchr(ptr, '\n');
        if (!end_ptr) {
            end_ptr = strchr(ptr, '\r');
        }
        if (!end_ptr) {
            end_ptr = ptr + strlen(ptr);
        }
        
        // 复制PLMN列表，去除可能的引号
        if (*ptr == '\"') {
            ptr++; // 跳过开头的引号
            if (end_ptr > ptr && *(end_ptr - 1) == '\"') {
                end_ptr--; // 跳过结尾的引号
            }
        }
        
        size_t plmn_len = end_ptr - ptr + 1;
        if (plmn_len > 0 && plmn_len < QCMAP_MAX_STRING_LEN) {
            strncpy(current_plmn_list, ptr, plmn_len);
            current_plmn_list[plmn_len] = '\0';
            
            // 删除特殊符号 \n、\r 和 "
            char *src = current_plmn_list;
            char *dst = current_plmn_list;
            while (*src) {
                if (*src != '\n' && *src != '\r' && *src != '\"') {
                    *dst++ = *src;
                }
                src++;
            }
            *dst = '\0';
        }
    }
    
    PRINT_DEBUG("Current simlock setting: mode=[%d], plmn_list=[%s]\n", *current_mode, current_plmn_list);
    return true;
}

/**
 * @brief 获取当前系统的频段锁设置
 * @param current_rat 输出参数，当前RAT
 * @param current_preferred_act1 输出参数，当前首选Act1
 * @param current_preferred_act2 输出参数，当前首选Act2
 * @param current_bands 输出参数，当前频段列表
 * @return true 成功获取设置，false 获取失败
 */
bool qcmap_get_current_bandlock_setting(char *current_rat, char *current_preferred_act1, char *current_preferred_act2, char *current_bands) {
    char result[QCMAP_MAX_STRING_LEN] = {0};
    char command[QCMAP_MAX_STRING_LEN] = {0};
    char *ptr = NULL;
    char *end_ptr = NULL;
    int param_count = 0;
    
    if (!current_rat || !current_preferred_act1 || !current_preferred_act2 || !current_bands) {
        PRINT_DEBUG("Invalid parameters for qcmap_get_current_bandlock_setting\n");
        return false;
    }
    
    // 初始化输出参数
    memset(current_rat, 0, QCMAP_MAX_STRING_LEN);
    memset(current_preferred_act1, 0, QCMAP_MAX_STRING_LEN);
    memset(current_preferred_act2, 0, QCMAP_MAX_STRING_LEN);
    memset(current_bands, 0, QCMAP_MAX_STRING_LEN);
    
    // 发送查询命令 AT+GTACT?
    snprintf(command, QCMAP_MAX_STRING_LEN, "%s -i 2 AT+GTACT?", QCMAP_MNGR_AT_MNGR_COMMAND);
    
    if (!execute_cmd((const char*)command, result, sizeof(result))) {
        PRINT_DEBUG("Failed to execute bandlock query command\n");
        return false;
    }
    PRINT_DEBUG("result:[%s]\n", result);
    // 解析响应格式: +GTACT:[<rat>[,[<PreferredAct1>],[<PreferredAct2>][,<band_1>[,<band_2>[,……[,<band_n>]]]]]]
    ptr = strstr(result, "+GTACT: ");
    if (!ptr) {
        PRINT_DEBUG("No valid bandlock response found\n");
        return false;
    }
    
    // 跳过 "+GTACT:" 前缀
    ptr += strlen("+GTACT: ");
    
    // 解析RAT
    end_ptr = strchr(ptr, ',');
    if (!end_ptr) {
        end_ptr = ptr + strlen(ptr);
    }
    size_t rat_len = end_ptr - ptr;
    if (rat_len > 0 && rat_len < QCMAP_MAX_STRING_LEN) {
        strncpy(current_rat, ptr, rat_len);
        current_rat[rat_len] = '\0';
    }
    param_count++;
    
    // 解析PreferredAct1
    if (*end_ptr == ',') {
        ptr = end_ptr + 1;
        end_ptr = strchr(ptr, ',');
        if (!end_ptr) {
            end_ptr = ptr + strlen(ptr);
        }
        size_t act1_len = end_ptr - ptr;
        if (act1_len > 0 && act1_len < QCMAP_MAX_STRING_LEN) {
            strncpy(current_preferred_act1, ptr, act1_len);
            current_preferred_act1[act1_len] = '\0';
        }
        param_count++;
    }
    
    // 解析PreferredAct2
    if (*end_ptr == ',') {
        ptr = end_ptr + 1;
        end_ptr = strchr(ptr, ',');
        if (!end_ptr) {
            end_ptr = ptr + strlen(ptr);
        }
        size_t act2_len = end_ptr - ptr;
        if (act2_len > 0 && act2_len < QCMAP_MAX_STRING_LEN) {
            strncpy(current_preferred_act2, ptr, act2_len);
            current_preferred_act2[act2_len] = '\0';
        }
        param_count++;
    }
    
    // 解析频段列表
    // 响应格式: +GTACT: rat,act1,act2,band1,band2,...,bandn
    // 从第4个参数开始就是所有频段列表
    if (*end_ptr == ',') {
        ptr = end_ptr + 1;
        
        // 查找频段列表的结束位置（换行符或字符串结束）
        end_ptr = strchr(ptr, '\n');
        if (!end_ptr) {
            end_ptr = strchr(ptr, '\r');
        }
        if (!end_ptr) {
            end_ptr = ptr + strlen(ptr);
        }
        
        size_t bands_len = end_ptr - ptr + 1;
        if (bands_len > 0 && bands_len < QCMAP_MAX_STRING_LEN) {
            strncpy(current_bands, ptr, bands_len);
            current_bands[bands_len] = '\0';
            
            // 删除特殊符号 \n、\r 和 "
            char *src = current_bands;
            char *dst = current_bands;
            while (*src) {
                if (*src != '\n' && *src != '\r' && *src != '\"') {
                    *dst++ = *src;
                }
                src++;
            }
            *dst = '\0';
        }
        param_count++;
    }
    
    PRINT_DEBUG("Current bandlock setting:\n");
    PRINT_DEBUG("rat=[%s], act1=[%s], act2=[%s]\n",
                current_rat, current_preferred_act1, current_preferred_act2);
    PRINT_DEBUG("bands=[%s]\n", current_bands);
    return true;
}

/**
 * @brief 根据config设置bandlock
 * @return 1 -- 已设置，需cfun 0/1；0 -- 未设置，无需cfun 0/1
 */
int qcmap_apply_bandlock_setting(void) {
    bool ret = false;
    char all_band[256] = {0};
    char lock_band[256] = {0};
    char current_rat[QCMAP_MAX_STRING_LEN] = {0};
    char current_preferred_act1[QCMAP_MAX_STRING_LEN] = {0};
    char current_preferred_act2[QCMAP_MAX_STRING_LEN] = {0};
    char current_bands[QCMAP_MAX_STRING_LEN] = {0};

    qcmap_get_all_band(all_band);
    qcmap_get_all_lock_band(lock_band);
    ret = qcmap_get_current_bandlock_setting(current_rat, current_preferred_act1, current_preferred_act2, current_bands);
    if (!ret || (bandlock_config.enable && strcmp(lock_band, current_bands) != 0) ||
        (!bandlock_config.enable && strcmp(all_band, current_bands) != 0)) {
        
        if (!ret) {
            PRINT_DEBUG("Failed to get current bandlock setting in modem\n");
        } else if (bandlock_config.enable && strcmp(lock_band, current_bands) != 0) {
            PRINT_DEBUG("Band list mismatch\n");
            PRINT_DEBUG("in modem  [%s]\n", current_bands);
            PRINT_DEBUG("in config [%s]\n", lock_band);
        } else if (!bandlock_config.enable && strcmp(all_band, current_bands) != 0) {
            PRINT_DEBUG("Bandlock disabled but modem bands don't match all bands\n");
            PRINT_DEBUG("modem bands [%s]\n", current_bands);
            PRINT_DEBUG("all bands   [%s]\n", all_band);
        }

        if (bandlock_config.enable) {
            if (strlen(lock_band) > 0) {
                PRINT_DEBUG("Enabling band lock with bands: %s\n", lock_band);
                qcmap_apply_bandLock(lock_band);
            } else {
                PRINT_DEBUG("Bandlock enabled but no bands configured, restoring all bands\n");
                qcmap_restore_all_band();
            }
        } else {
            PRINT_DEBUG("Disabling band lock, restoring all bands\n");
            qcmap_restore_all_band();
        }
        return 1;
    } else {
        PRINT_DEBUG("Bandlock setting matches config, no update needed\n");
        return 0;
    }
}

void qcmap_simlock_enable(void) {
    char result[QCMAP_MAX_STRING_LEN] = {0};
    char command[QCMAP_MAX_STRING_LEN] = {0};

    snprintf(command, QCMAP_MAX_STRING_LEN, "%s \'%s=1,\"%s\"\'", QCMAP_MNGR_AT_MNGR_COMMAND, QCMAP_MNGR_AT_COMMAND_PLMNLOCK, simlock_config.MCCMNCList);
    if (execute_cmd((const char*)command, result, sizeof(result))) {
        if (strcasestr(result, "OK")) {
            PRINT_DEBUG("simlock enable [%s] success\n", simlock_config.MCCMNCList);
        }
    }
}

void qcmap_simlock_disable(void) {
    char result[QCMAP_MAX_STRING_LEN] = {0};
    char command[QCMAP_MAX_STRING_LEN] = {0};

    snprintf(command, QCMAP_MAX_STRING_LEN, "%s %s=0", QCMAP_MNGR_AT_MNGR_COMMAND, QCMAP_MNGR_AT_COMMAND_PLMNLOCK);
    if (execute_cmd((const char*)command, result, sizeof(result))) {
        if (strcasestr(result, "OK")) {
            PRINT_DEBUG("simlock disable success\n");
        }
    }
}

/**
 * @brief 根据config设置simlock
 * @return 1 -- 已设置，需cfun 0/1；0 -- 未设置，无需cfun 0/1
 */
int qcmap_apply_simlock_setting(void) {
    bool ret = false;
    int current_mode = 0;
    char current_plmn_list[QCMAP_MAX_STRING_LEN] = {0};

    ret = qcmap_get_current_simlock_setting(&current_mode, current_plmn_list);
    if (!ret || current_mode != simlock_config.enable ||
        (simlock_config.enable == 1 && strcmp(current_plmn_list, simlock_config.MCCMNCList) != 0)) {
        if (!ret) {
            PRINT_DEBUG("Failed to get current simlock setting in modem\n");
        } else if (current_mode != simlock_config.enable) {
            PRINT_DEBUG("enable mismatch, in modem = [%d], in config = [%d]\n", current_mode, simlock_config.enable);
        } else if (simlock_config.enable == 1 && strcmp(current_plmn_list, simlock_config.MCCMNCList) != 0) {
            PRINT_DEBUG("PLMN list mismatch, in modem = [%s], in config = [%s]\n", current_plmn_list, simlock_config.MCCMNCList);
        }

        if (simlock_config.enable) {
            PRINT_DEBUG("Enabling SIM lock with PLMN: %s\n", simlock_config.MCCMNCList);
            qcmap_simlock_enable();
        } else {
            PRINT_DEBUG("Disabling SIM lock\n");
            qcmap_simlock_disable();
        }
        return 1;
    } else {
        PRINT_DEBUG("SIM lock setting matches config, no update needed\n");
    }
    return 0;
}

void qcmap_pcidlock_disable(void) {
    char result[QCMAP_MAX_STRING_LEN] = {0};
    char command[QCMAP_MAX_STRING_LEN] = {0};

    snprintf(command, QCMAP_MAX_STRING_LEN, "%s %s=0", QCMAP_MNGR_AT_MNGR_COMMAND, QCMAP_MNGR_AT_COMMAND_CELLLOCK);
    if (execute_cmd((const char*)command, result, sizeof(result))) {
        if (strcasestr(result, "OK")) {
            PRINT_DEBUG("pcidlock disable success\n");
        }
    }
}

void qcmap_pcidlock_enable(void) {
    char result[QCMAP_MAX_STRING_LEN] = {0};
    char command[QCMAP_MAX_STRING_LEN] = {0};

    if (strcasecmp(pcidlock_config.netType,"5G")==0 || strcasecmp(pcidlock_config.netType,"NR")==0) //5g
        snprintf(command, QCMAP_MAX_STRING_LEN, "%s '%s=%d,%d,%d,%s,,%d'",QCMAP_MNGR_AT_MNGR_COMMAND,QCMAP_MNGR_AT_COMMAND_CELLLOCK,1,1,1,hex_str_to_dec_str(pcidlock_config.freq),1);
    else if (strcasecmp(pcidlock_config.netType,"4G")==0 || strcasecmp(pcidlock_config.netType,"LTE")==0 ) //4g
        snprintf(command, QCMAP_MAX_STRING_LEN, "%s '%s=%d,%d,%d,%s'",QCMAP_MNGR_AT_MNGR_COMMAND,QCMAP_MNGR_AT_COMMAND_CELLLOCK,1,0,1,hex_str_to_dec_str(pcidlock_config.freq));

    if (execute_cmd((const char*)command, result, sizeof(result))) {
        if (strcasestr(result, "OK")) {
            PRINT_DEBUG("pcidlock enable success\n");
        }
    }
}

int qcmap_apply_pcidlock_setting(void) {
    if (pcidlock_config.enable) {
        qcmap_pcidlock_disable();
        usleep(1 * 1000 * 1000);
        qcmap_pcidlock_enable();
    } else {
        qcmap_pcidlock_disable();
    }
    return 0;
}

void qcmap_init_bandlock_setting(void) {
    FILE *fp = NULL;
    char result[256] = {0};

    memset(result, 0, sizeof(result));
    if (qcmap_get_uci_value(QCMAP_MNGR_MOBILE_CONFIG_FILE, "bandlock", "enable", result, sizeof(result))) {
        bandlock_config.enable = atoi(result);
    }

    memset(result, 0, sizeof(result));
    if (qcmap_get_uci_value(QCMAP_MNGR_MOBILE_CONFIG_FILE, "bandlock", "netType", result, sizeof(result))) {
        sprintf(bandlock_config.netType, "%s", result);
    }

    memset(result, 0, sizeof(result));
    if (qcmap_get_uci_value(QCMAP_MNGR_MOBILE_CONFIG_FILE, "bandlock", "cfg4GBandList", result, sizeof(result))) {
        sprintf(bandlock_config.cfg4GBandList, "%s", result);
    }

    memset(result, 0, sizeof(result));
    if (qcmap_get_uci_value(QCMAP_MNGR_MOBILE_CONFIG_FILE, "bandlock", "cfg5GBandList", result, sizeof(result))) {
        sprintf(bandlock_config.cfg5GBandList, "%s", result);
    }

    memset(result, 0, sizeof(result));
    if (qcmap_get_uci_value(QCMAP_MNGR_MOBILE_CONFIG_FILE, "bandlock", "cfg5GnsaBandList", result, sizeof(result))) {
        sprintf(bandlock_config.cfg5GnsaBandList, "%s", result);
    }

    // 统一打印bandlock配置信息
    PRINT_DEBUG("Bandlock: enable=%d, netType=%s, cfg4GBandList=%s, cfg5GBandList=%s, cfg5GnsaBandList=%s\n",
               bandlock_config.enable,
               bandlock_config.netType,
               bandlock_config.cfg4GBandList,
               bandlock_config.cfg5GBandList,
               bandlock_config.cfg5GnsaBandList);
}

void qcmap_init_simlock_setting(void) {
    char result[64] = {0};

    memset(result, 0, sizeof(result));
    if (qcmap_get_uci_value(QCMAP_MNGR_MOBILE_CONFIG_FILE, "simlock", "enable", result, sizeof(result))) {
        simlock_config.enable = atoi(result);
    }

    memset(result, 0, sizeof(result));
    if (qcmap_get_uci_value(QCMAP_MNGR_MOBILE_CONFIG_FILE, "simlock", "MCCMNCList", result, sizeof(result))) {
        sprintf(simlock_config.MCCMNCList, "%s", result);
    }

    // 统一打印simlock配置信息
    PRINT_DEBUG("Simlock: enable=%d, MCCMNCList=%s\n",
               simlock_config.enable,
               simlock_config.MCCMNCList);
}

void qcmap_init_pcidlock_setting(void) {
    char result[64] = {0};

    memset(result, 0, sizeof(result));
    if (qcmap_get_uci_value(QCMAP_MNGR_MOBILE_CONFIG_FILE, "pcidlock", "enable", result, sizeof(result))) {
        pcidlock_config.enable = atoi(result);
    }

    memset(result, 0, sizeof(result));
    if (qcmap_get_uci_value(QCMAP_MNGR_MOBILE_CONFIG_FILE, "pcidlock", "netType", result, sizeof(result))) {
        sprintf(pcidlock_config.netType, "%s", result);
    }

    memset(result, 0, sizeof(result));
    if (qcmap_get_uci_value(QCMAP_MNGR_MOBILE_CONFIG_FILE, "pcidlock", "pcid", result, sizeof(result))) {
        sprintf(pcidlock_config.pcid, "%s", result);
    }

    memset(result, 0, sizeof(result));
    if (qcmap_get_uci_value(QCMAP_MNGR_MOBILE_CONFIG_FILE, "pcidlock", "freq", result, sizeof(result))) {
        sprintf(pcidlock_config.freq, "%s", result);
    }

    memset(result, 0, sizeof(result));
    if (qcmap_get_uci_value(QCMAP_MNGR_MOBILE_CONFIG_FILE, "pcidlock", "SCS", result, sizeof(result))) {
        sprintf(pcidlock_config.SCS, "%s", result);
    }

    memset(result, 0, sizeof(result));
    if (qcmap_get_uci_value(QCMAP_MNGR_MOBILE_CONFIG_FILE, "pcidlock", "band", result, sizeof(result))) {
        sprintf(pcidlock_config.band, "%s", result);
    }

    // 统一打印pcidlock配置信息
    PRINT_DEBUG("PCIDlock: enable=%d, netType=%s, pcid=%s, freq=%s, SCS=%s, band=%s\n",
               pcidlock_config.enable,
               pcidlock_config.netType,
               pcidlock_config.pcid,
               pcidlock_config.freq,
               pcidlock_config.SCS,
               pcidlock_config.band);
}

void qcmap_init_pinlock_config(void) {
    char result[64] = {0};
    char wan_name[16] = {0};

    memset(&pinlock_config, 0, sizeof(pinlock_config));
    sprintf(wan_name, "%s", QCMAP_MNGR_DEFAULT_WAN_NAME);

    memset(result, 0, sizeof(result));
    if (qcmap_get_uci_value(QCMAP_MNGR_MOBILE_CONFIG_FILE, wan_name, QCMAP_MNGR_CONFIG_PINAUTOLOCK, result, sizeof(result)))
        pinlock_config.pinAutoUnlock = atoi(result);

    memset(result, 0, sizeof(result));
    if (qcmap_get_uci_value(QCMAP_MNGR_MOBILE_CONFIG_FILE, wan_name, QCMAP_MNGR_CONFIG_PINENABLE, result, sizeof(result))) {
        pinlock_config.pinenable = atoi(result);
    }

    memset(result, 0, sizeof(result));
    if (qcmap_get_uci_value(QCMAP_MNGR_MOBILE_CONFIG_FILE, wan_name, QCMAP_MNGR_CONFIG_PINNUMBER, result, sizeof(result)))
        sprintf(pinlock_config.pinNumber, "%s", result);

    // 统一打印pinlock配置信息
    PRINT_DEBUG("Pinlock: pinAutoUnlock=%d, pinenable=%d, pinNumber=%s\n",
               pinlock_config.pinAutoUnlock,
               pinlock_config.pinenable,
               pinlock_config.pinNumber);
}

bool qcmap_init_lock(void) {
    int ret = 0;
    static bool is_first_time_init = true;
    if (is_first_time_init) {
        qcmap_init_pinlock_config();
        qcmap_init_simlock_setting();
        qcmap_init_bandlock_setting();
        //qcmap_init_pcidlock_setting();

        ret |= qcmap_apply_simlock_setting();
        ret |= qcmap_apply_bandlock_setting();
        //qcmap_apply_pcidlock_setting();
    
        is_first_time_init = false;
        if (!ret) {
            PRINT_DEBUG("all lock no need to setting\n");
            return true;
        }
        qcmap_init_cfun(true);
        return false;
    }
    return true;
}

int qcmap_check_lock_pin(void) {
    char result[64] = {0};
    char wan_name[16] = {0};
    pinlock_config_t pinlock;
    int update = 0;

    static unsigned int s_last_check_time = 0;
    unsigned int now_time = qcmap_get_uptime_in_ms();
    if (POS_DIFF_VAL(now_time, s_last_check_time) < MOD_STATUS_UPDATE_INTERVAL) {
        return update;
    }
    s_last_check_time = now_time;

    memset(&pinlock, 0, sizeof(pinlock));
    sprintf(wan_name, "%s", QCMAP_MNGR_DEFAULT_WAN_NAME);
    memset(result, 0, sizeof(result));
    if (qcmap_get_uci_value(QCMAP_MNGR_MOBILE_CONFIG_FILE, wan_name, QCMAP_MNGR_CONFIG_PINAUTOLOCK, result, sizeof(result))) {
        pinlock.pinAutoUnlock = atoi(result);
        if (pinlock.pinAutoUnlock != pinlock_config.pinAutoUnlock) {
            PRINT_DEBUG("pinlock pinAutoUnlock [%d] update to [%d]\n", pinlock_config.pinAutoUnlock, pinlock.pinAutoUnlock);
            pinlock_config.pinAutoUnlock = pinlock.pinAutoUnlock;
            update |= 1;
        }
    }
    memset(result, 0, sizeof(result));
    if (qcmap_get_uci_value(QCMAP_MNGR_MOBILE_CONFIG_FILE, wan_name, QCMAP_MNGR_CONFIG_PINENABLE, result, sizeof(result))) {
        pinlock.pinenable = atoi(result);
        if (pinlock.pinenable != pinlock_config.pinenable) {
            PRINT_DEBUG("pinlock pinenable [%d] update to [%d]\n", pinlock_config.pinenable, pinlock.pinenable);
            pinlock_config.pinenable = pinlock.pinenable;
            update |= 1;
        }
    }

    memset(result, 0, sizeof(result));
    if (qcmap_get_uci_value(QCMAP_MNGR_MOBILE_CONFIG_FILE, wan_name, QCMAP_MNGR_CONFIG_PINNUMBER, result, sizeof(result))) {
        sprintf(pinlock.pinNumber, "%s", result);
        if (strcmp(pinlock.pinNumber, pinlock_config.pinNumber)) {
            PRINT_DEBUG("pinlock pinNumber [%s] update to [%s]\n", pinlock_config.pinNumber, pinlock.pinNumber);
            sprintf(pinlock_config.pinNumber, "%s", pinlock.pinNumber);
            update |= 1;
        }
    }

    return update;
}

int qcmap_check_lock_sim(void) {
    int ret = 0;

    if (access(DEFAULT_MOBILESIMLOCK_CONFIG_UPDATE, F_OK) == 0) {
        PRINT_DEBUG("check [%s] update\n", DEFAULT_MOBILESIMLOCK_CONFIG_UPDATE);
        qcmap_init_simlock_setting();
        if (qcmap_apply_simlock_setting()) {
            qcmap_init_cfun(true);
        }
        unlink(DEFAULT_MOBILESIMLOCK_CONFIG_UPDATE);
        ret = 1;
    }

    return ret;
}

int qcmap_check_lock_band(void) {
    int ret = 0;

    if (access(DEFAULT_MOBILEBANDLOCK_CONFIG_UPDATE, F_OK) == 0) {
        PRINT_DEBUG("check [%s] update\n", DEFAULT_MOBILEBANDLOCK_CONFIG_UPDATE);
        qcmap_init_bandlock_setting();
        if (qcmap_apply_bandlock_setting()) {
            qcmap_init_cfun(true);
        }
        unlink(DEFAULT_MOBILEBANDLOCK_CONFIG_UPDATE);
        ret = 1;
    }

    return ret;
}

int qcmap_check_lock_pcid(void) {
    int ret = 0;

    if (access(DEFAULT_MOBILEPCIDLOCK_CONFIG_UPDATE, F_OK) == 0) {
        PRINT_DEBUG("check [%s] update\n", DEFAULT_MOBILEPCIDLOCK_CONFIG_UPDATE);
        qcmap_init_pcidlock_setting();
        if (qcmap_apply_pcidlock_setting()) {
            qcmap_init_cfun(true);
        }
        unlink(DEFAULT_MOBILEPCIDLOCK_CONFIG_UPDATE);
        ret = 1;
    }

    return ret;
}

int qcmap_timer_check_lock(void) {
    int ret = 0;

    ret |= qcmap_check_lock_pin();
    ret |= qcmap_check_lock_sim();
    ret |= qcmap_check_lock_band();
    //ret |= qcmap_check_lock_pcid();

    return ret;
}