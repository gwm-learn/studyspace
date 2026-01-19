#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "ql_sms.h"

#include "mobile_module_util.h"
#include "mobile_module_sms.h"
#include "mobile_service_modem.h"
#include "mobile_service_lock.h"
#include "mobile_service_sms.h"
/*
 * 文件名称：mobile_service_sms.c
 * 功能描述：
 *     SMS短信服务模块，提供用户自定义的PDU接收处理功能
 *
 * 作者：gaoweiming
 */

mobile_codes_t g_sms_code = {
    .code1 = {"0001"},
    .code2 = {"0002"},
    .code3 = {"0003"},
    .code4 = {"0004"},
};
// 内部状态变量
static int g_sms_module_initialized = 0;
mobile_pin_info_t g_mobile_pin_info;

 /**
  * 初始化SMS服务模块
  * 封装SMS模块的初始化操作，直接注册mobile_sms_pdu_handler作为回调函数
  *
  * @return 成功返回0，失败返回错误码
  */
int mobile_init_sms_service(void) {
    if (g_sms_module_initialized) {
        MOBILE_INFO("sms service module already initialized\n");
        return 0;
    }
    
    // 初始化SMS模块
    int result = ql_sms_init(SMS_IPC_MODE_DEFAULT);
    if (result != 0) {
        MOBILE_ERROR("Failed to initialize sms module, error: %d\n", result);
        return result;
    }
    
    result = ql_sms_set_pdu_recv_cb(mobile_sms_pdu_handler);
    if (result == 0) {
        MOBILE_INFO("SMS PDU callback registered successfully\n");
    } else {
        MOBILE_ERROR("Failed to register SMS PDU callback, error: %d\n", result);
        ql_sms_release();
        return result;
    }
    
    g_sms_module_initialized = 1;
    MOBILE_INFO("sms service module initialized successfully\n");
    return 0;
}

/**
 * 清理SMS服务模块资源
 * 封装SMS模块的清理操作
 *
 * @return 无返回值
 */
void mobile_deinit_sms_service(void) {
    if (!g_sms_module_initialized) {
        return;
    }
    
    ql_sms_release();
    g_sms_module_initialized = 0;
    MOBILE_INFO("sms service module deinitialized\n");
}

/**
 * 处理验证码1对应的操作
 * 当接收到验证码1时执行的操作：禁用SIM锁并更新配置文件
 *
 * @return 无返回值
 */
void mobile_process_with_code1(void){
    MOBILE_DEBUG("PDU Processing with code1\n");
    mobile_uci_set_option("mobile", "simlock", "enable", "0");
    mobile_write_buff_to_file(DEFAULT_MOBILESIMLOCK_CONFIG_UPDATE, "true", strlen("true"));
};

/**
 * 处理验证码2对应的操作
 * 当接收到验证码2时执行的操作：处理PIN锁状态
 *
 * @return 无返回值
 */
void mobile_process_with_code2(void){
    MOBILE_DEBUG("Processing with code2\n");
#ifdef CUS_PARAMS_STC_DEFAULT_PIN_CODE
    if (access(PINLOCK_STATUS_FILE, F_OK) == 0 && mobile_at_is_ready()) {
        if (mobile_change_pin(g_mobile_pin_info.pin_code, CUS_PARAMS_STC_DEFAULT_PIN_CODE) == 0) {
            mobile_set_produce_pin("");
            sleep(5);
            if (mobile_disable_pinlock(CUS_PARAMS_STC_DEFAULT_PIN_CODE) == 0) {
                unlink(PINLOCK_STATUS_FILE);
            }
        }
    }
#endif
};

/**
 * 处理验证码3对应的操作
 * 当接收到验证码3时执行的操作（当前为空实现，预留扩展）
 *
 * @return 无返回值
 */
void mobile_process_with_code3(void){
    MOBILE_DEBUG("Processing with code3\n");
    char section[16 + 1] = {0};
    for (int i = 0; i < MAX_PCIDCELL_LOCK_NUM; i++) {
        sprintf(section, "pcidlock%d", i);
        mobile_uci_set_option_int("mobile", section, "enable", 0);
        mobile_uci_del_option("mobile", section, "netType");
        mobile_uci_del_option("mobile", section, "pcid");
        mobile_uci_del_option("mobile", section, "freq");
        mobile_uci_del_option("mobile", section, "SCS");
        mobile_uci_del_option("mobile", section, "band");
        mobile_uci_del_option("mobile", section, "cellid");
    }
    mobile_write_buff_to_file(DEFAULT_MOBILEPCIDLOCK_CONFIG_UPDATE, "true", strlen("true"));
};

/**
 * 处理验证码4对应的操作
 * 当接收到验证码4时执行的操作（当前为空实现，预留扩展）
 *
 * @return 无返回值
 */
void mobile_process_with_code4(void){
    MOBILE_DEBUG("Processing with code4\n");
    char section[16 + 1] = {0};
    for (int i = 0; i < MAX_PCIDCELL_LOCK_NUM; i++) {
        sprintf(section, "pcidlock%d", i);
        mobile_uci_set_option_int("mobile", section, "enable", 0);
    }
    mobile_write_buff_to_file(DEFAULT_MOBILEPCIDLOCK_CONFIG_UPDATE, "true", strlen("true"));
};

/**
 * 生成锁定验证码字符串
 * 基于serialnum和IMEI生成8位数字验证码，使用特定算法确保唯一性
 * 算法：取serialnum和IMEI的后8位，对serialnum的每位加上num值后与IMEI对应位相乘，取个位数
 *
 * @param sn 设备序列号，不能为空且必须包含数字
 * @param imei 设备IMEI号，不能为空且必须包含数字
 * @param code 输出的验证码缓冲区，必须至少有9字节空间
 * @param num 验证码编号(1-4)，用于生成不同的验证码
 * @return 无返回值
 */
void mobile_generate_lock_code_str(const char* serialnum, const char* imei, char* code, int num) {
    // 参数有效性检查
    if (serialnum == NULL || imei == NULL || code == NULL) {
        MOBILE_ERROR("Invalid parameters: serialnum=%p, imei=%p, code=%p\n", serialnum, imei, code);
        if (code) code[0] = '\0';
        return;
    }
    
    // 验证码编号范围检查
    if (num < 1 || num > 4) {
        MOBILE_ERROR("Invalid code number: %d, must be between 1-4\n", num);
        code[0] = '\0';
        return;
    }
    
    // 获取serialnum和IMEI的长度
    int sn_len = strlen(serialnum);
    int imei_len = strlen(imei);
    
    // 检查输入字符串是否为空
    if (sn_len == 0 || imei_len == 0) {
        MOBILE_ERROR("Empty input string: sn_len=%d, imei_len=%d\n", sn_len, imei_len);
        code[0] = '\0';
        return;
    }
    
    char sn_last8[9] = {0};
    char imei_last8[9] = {0};
    
    // 提取后8位，不足8位时前面补0
    int sn_start = (sn_len > 8) ? sn_len - 8 : 0;
    int imei_start = (imei_len > 8) ? imei_len - 8 : 0;
    
    // 安全地复制字符串
    strncpy(sn_last8, serialnum + sn_start, 8);
    strncpy(imei_last8, imei + imei_start, 8);
    sn_last8[8] = '\0';  // 确保字符串结束
    imei_last8[8] = '\0';
    
    // 如果长度不足8，在前面补'0'
    if (strlen(sn_last8) < 8) {
        memmove(sn_last8 + (8 - strlen(sn_last8)), sn_last8, strlen(sn_last8) + 1);
        memset(sn_last8, '0', 8 - strlen(sn_last8));
    }
    if (strlen(imei_last8) < 8) {
        memmove(imei_last8 + (8 - strlen(imei_last8)), imei_last8, strlen(imei_last8) + 1);
        memset(imei_last8, '0', 8 - strlen(imei_last8));
    }
    
    // 验证提取的字符串是否都是数字
    for (int i = 0; i < 8; i++) {
        if (!isdigit((unsigned char)sn_last8[i]) || !isdigit((unsigned char)imei_last8[i])) {
            MOBILE_ERROR("Invalid character in extracted strings: sn_last8[%d]=%c, imei_last8[%d]=%c\n",
                        i, sn_last8[i], i, imei_last8[i]);
            code[0] = '\0';
            return;
        }
    }
    
    // 生成单个code，直接存储为字符串
    for (int i = 0; i < 8; i++) {
        // SN后8位数字加上相应的值
        int sn_digit = sn_last8[i] - '0';
        int new_sn_digit = (sn_digit + num) % 10;  // 确保是单个数字
        
        // IMEI后8位数字
        int imei_digit = imei_last8[i] - '0';
        
        // 相乘并取余，然后转换为字符
        int product = new_sn_digit * imei_digit;
        code[i] = (product % 10) + '0';  // 将数字转换为字符
    }

    code[8] = '\0';  // 添加字符串结束符
    MOBILE_INFO("Generated code%d=%s (from sn=%s, imei=%s)\n", num, code, serialnum, imei);
}

/**
 * 初始化SMS验证码
 * 根据IMEI和SN生成4个验证码，并保存到配置文件中
 * 用于SMS短信验证功能，支持不同的操作码
 *
 * @return 成功返回0，失败返回错误码
 */
int mobile_init_sms_codes(void) {
    FILE* fp = NULL;
    char bufLine[32] = {0};
    int ret = 0;

    // 检查IMEI和SN是否为空字符串
    if (g_module_desc.imei[0] == '\0' || g_module_desc.serialnum[0] == '\0') {
        MOBILE_ERROR("Empty IMEI or SN string: imei='%s', serialnum='%s'\n", g_module_desc.imei, g_module_desc.serialnum);
        return -1;
    }

    MOBILE_INFO("Initializing SMS codes with IMEI: %s, serialnum: %s\n", g_module_desc.imei, g_module_desc.serialnum);
    
    // 生成4个不同的验证码
    mobile_generate_lock_code_str(g_module_desc.serialnum, g_module_desc.imei, g_sms_code.code1, 1);
    mobile_generate_lock_code_str(g_module_desc.serialnum, g_module_desc.imei, g_sms_code.code2, 2);
    mobile_generate_lock_code_str(g_module_desc.serialnum, g_module_desc.imei, g_sms_code.code3, 3);
    mobile_generate_lock_code_str(g_module_desc.serialnum, g_module_desc.imei, g_sms_code.code4, 4);

    // 验证生成的验证码是否有效
    if (g_sms_code.code1[0] == '\0' || g_sms_code.code2[0] == '\0' ||
        g_sms_code.code3[0] == '\0' || g_sms_code.code4[0] == '\0') {
        MOBILE_ERROR("Failed to generate one or more SMS codes\n");
        return -1;
    }

    // 将验证码保存到配置文件
    fp = fopen(MOBILE_MOBILE_CODES, "w+");
    if (fp == NULL) {
        MOBILE_ERROR("Failed to open SMS codes file: %s, error: %s\n",
                     MOBILE_MOBILE_CODES, strerror(errno));
        return -1;
    }

    // 写入4个验证码到文件
    memset(bufLine, 0, sizeof(bufLine));
    snprintf(bufLine, sizeof(bufLine), "code1=%s\n", g_sms_code.code1);
    if (fputs(bufLine, fp) == EOF) {
        MOBILE_ERROR("Failed to write code1 to file\n");
        ret = -1;
    }

    memset(bufLine, 0, sizeof(bufLine));
    snprintf(bufLine, sizeof(bufLine), "code2=%s\n", g_sms_code.code2);
    if (fputs(bufLine, fp) == EOF) {
        MOBILE_ERROR("Failed to write code2 to file\n");
        ret = -1;
    }

    memset(bufLine, 0, sizeof(bufLine));
    snprintf(bufLine, sizeof(bufLine), "code3=%s\n", g_sms_code.code3);
    if (fputs(bufLine, fp) == EOF) {
        MOBILE_ERROR("Failed to write code3 to file\n");
        ret = -1;
    }

    memset(bufLine, 0, sizeof(bufLine));
    snprintf(bufLine, sizeof(bufLine), "code4=%s\n", g_sms_code.code4);
    if (fputs(bufLine, fp) == EOF) {
        MOBILE_ERROR("Failed to write code4 to file\n");
        ret = -1;
    }

    // 关闭文件
    if (fclose(fp) != 0) {
        MOBILE_ERROR("Failed to close SMS codes file\n");
        ret = -1;
    }

    if (ret == 0) {
        MOBILE_INFO("SMS codes initialized and saved successfully: code1=%s, code2=%s, code3=%s, code4=%s\n",
                    g_sms_code.code1, g_sms_code.code2, g_sms_code.code3, g_sms_code.code4);
    } else {
        MOBILE_ERROR("SMS codes initialization completed with errors\n");
    }

    return ret;
}

/**
 * 生成pin码
 *
 * @return 0-无 1-有
 */
int mobile_generate_pin(const char *imei, char *output) {
    // 参数检查
    if (imei == NULL || output == NULL) {
        return -1;
    }
    
    // 清空输出缓冲区
    output[0] = '\0';
    
    size_t len = strlen(imei);
    
    // 检查IMEI长度是否足够
    if (len < 4) {
        return -1;
    }

    // 截取最后四个字符
    const char *last_four = imei + len - 4;

    // 安全地复制到输出缓冲区
    strncpy(output, last_four, 4);
    output[4] = '\0'; // 确保字符串终止
    
    return 0;
}

/**
 * 从produce中获取pincode
 *
 * @return 0-无 1-有
 */
int mobile_get_pincode_robust(char* output) {
    if (output == NULL) {
        return 0;
    }
    
    FILE* fp = popen("produce pinCode", "r");
    if (fp == NULL) {
        printf("run produce pinCode fail\n");
        return 0;
    }
    
    char buffer[32];
    int found = 0;
    
    while (fgets(buffer, sizeof(buffer), fp) != NULL) {
        buffer[strcspn(buffer, "\n")] = 0;
        
        if (strlen(buffer) == 0) {
            continue;
        }
        
        printf("produce pinCode: %s\n", buffer);
        
        // 方法1: 检查 ~!1234!~ 格式
        if (strlen(buffer) >= 6 && 
            buffer[0] == '~' && buffer[1] == '!' && 
            buffer[strlen(buffer)-2] == '!' && buffer[strlen(buffer)-1] == '~') {
            
            char pin[5];
            strncpy(pin, buffer + 2, 4);
            pin[4] = '\0';
            
            bool valid = true;
            for (int i = 0; i < 4; i++) {
                if (!isdigit(pin[i])) {
                    valid = false;
                    break;
                }
            }
            
            if (valid) {
                strcpy(output, pin);
                found = 1;
                break;
            }
        }
    }

    pclose(fp);

    return found;
}

/**
 * 初始化pincode
 *
 * @return 无返回值
 */
void mobile_init_pin(void) {
    g_mobile_pin_info.first_time = !mobile_get_pincode_robust(g_mobile_pin_info.pin_code);
    if (g_mobile_pin_info.first_time) {
        mobile_generate_pin(g_module_desc.imei, g_mobile_pin_info.pin_code);
        MOBILE_DEBUG("pin first time\n");
    }
    MOBILE_DEBUG("pin:%s\n", g_mobile_pin_info.pin_code);
}


/**
 * 用户自定义的PDU接收处理函数示例
 * 当接收到SMS PDU时被调用，处理特定号码的短信验证码
 *
 * @param p_pdu PDU指示结构体指针
 * @return 无返回值
 */
void mobile_sms_pdu_handler(void *p_pdu) {
    pdu_decode_result_t result;
    MOBILE_DEBUG("*************** New PDU received ***************\n");

    // 参数有效性检查
    if (p_pdu == NULL) {
        MOBILE_ERROR("Invalid PDU pointer: NULL\n");
        return;
    }

    ql_sms_pdu_ind_t *sms_pdu = (ql_sms_pdu_ind_t *)p_pdu;
    
    // 检查存储状态
    if(sms_pdu->store_flag & QL_SMS_STORE_FLAG_STORE_FULL) {
        MOBILE_WARN("PDU storage full, store_flag: 0x%x\n", sms_pdu->store_flag);
        MOBILE_WARN("Please check the preferred storage\n");
        return;
    }

    // 记录PDU基本信息
    MOBILE_DEBUG("PDU store_flag: 0x%x, message_index: %d, format: %d, content_size: %d\n",
                 sms_pdu->store_flag, sms_pdu->message_index, sms_pdu->format, sms_pdu->content_size);

    // 解码PDU内容
    int decode_ret = mobile_pdu_decode_from_bin_hex(sms_pdu->content, sms_pdu->content_size, &result);
    if (decode_ret != 0) {
        MOBILE_ERROR("Failed to decode PDU content, ret=%d\n", decode_ret);
        return;
    }

    mobile_print_decode_result(&result);

    // 检查发送者号码是否为目标号码
    MOBILE_DEBUG("Checking sender number: %s\n", result.sender_number);

    // 检查是否为目标号码
    int is_target_number = 0;
    if (strcmp(result.sender_number, "907") == 0 ||
        strcmp(result.sender_number, "909") == 0 ||
        strcmp(result.sender_number, "900") == 0) {
        is_target_number = 1;
        MOBILE_INFO("Target number matched: %s\n", result.sender_number);
    } else {
        MOBILE_DEBUG("Number does not match target numbers, skipping processing\n");
        return; // 如果不是目标号码，直接退出处理
    }

    // 如果是目标号码，继续检查短信内容
    if (is_target_number) {
        MOBILE_DEBUG("Checking SMS content from target number: %s\n", result.message);

        // 比较短信内容与验证码
        if (strcmp(result.message, g_sms_code.code1) == 0) {
            MOBILE_INFO("Matched code1: %s, executing mobile_process_with_code1()\n", g_sms_code.code1);
            mobile_process_with_code1();
        } else if (strcmp(result.message, g_sms_code.code2) == 0) {
            MOBILE_INFO("Matched code2: %s, executing mobile_process_with_code2()\n", g_sms_code.code2);
            mobile_process_with_code2();
        } else if (strcmp(result.message, g_sms_code.code3) == 0) {
            MOBILE_INFO("Matched code3: %s, executing mobile_process_with_code3()\n", g_sms_code.code3);
            mobile_process_with_code3();
        } else if (strcmp(result.message, g_sms_code.code4) == 0) {
            MOBILE_INFO("Matched code4: %s, executing mobile_process_with_code4()\n", g_sms_code.code4);
            mobile_process_with_code4();
        } else {
            MOBILE_WARN("SMS content does not match any verification code: %s\n", result.message);
        }
    }
    
    MOBILE_DEBUG("PDU processing completed\n");
}

/**
 * 打印PIN信息
 * 打印g_mobile_pin_info结构体的内容
 *
 * @return 无返回值
 */
void mobile_print_pin_stc_info(void) {
    MOBILE_INFO("=== PIN Information for STC===\n");
    MOBILE_INFO("First Time: %d\n", g_mobile_pin_info.first_time);
    MOBILE_INFO("PIN Code: %s\n", g_mobile_pin_info.pin_code);
    MOBILE_INFO("\n");
}

/**
 * stc 处理pin lock
 *
 * @return 无返回值
 */
bool mobile_sub_pin_lock_stc(void) {
    bool ret = false;
    mobile_update_pincode_set(1);
    mobile_write_buff_to_file(PINLOCK_STATUS_FILE, "pin lock", strlen("pin lock"));
#ifdef CUS_PARAMS_STC_DEFAULT_PIN_CODE
    if (g_mobile_pin_info.first_time) {
        if (mobile_unlock_pin(CUS_PARAMS_STC_DEFAULT_PIN_CODE) == 0) {
            sleep(5);
            if (mobile_change_pin(CUS_PARAMS_STC_DEFAULT_PIN_CODE, g_mobile_pin_info.pin_code) == 0) {
                mobile_set_produce_pin(g_mobile_pin_info.pin_code);
                g_mobile_pin_info.first_time = 0;
                ret = true;
            }
        }
    } else {
        if (mobile_unlock_pin(g_mobile_pin_info.pin_code) != 0) {
            sleep(5);
            if (mobile_unlock_pin(CUS_PARAMS_STC_DEFAULT_PIN_CODE) == 0) {
                sleep(5);
                if (mobile_change_pin(CUS_PARAMS_STC_DEFAULT_PIN_CODE, g_mobile_pin_info.pin_code) == 0) {
                    mobile_set_produce_pin(g_mobile_pin_info.pin_code);
                    ret = true;
                }
            }
        }
    }
#endif
    return ret;
}

/**
 * stc 更新pin lock
 *
 * @return 无返回值
 */
void mobile_update_pin_lock_stc(void) {
    if ((access(DEFAULT_MOBILEPINLOCK_CONFIG_ENABLE, F_OK)) == 0) {
        MOBILE_DEBUG("enable pinlock with sim.\n");
        if (access(PINLOCK_STATUS_FILE, F_OK) != 0) {
            if (mobile_enable_pinlock(g_mobile_pin_info.pin_code) == 0) {
                mobile_write_buff_to_file(PINLOCK_STATUS_FILE, "pin lock", strlen("pin lock"));
            }
        }
        unlink(DEFAULT_MOBILEPINLOCK_CONFIG_ENABLE);
    }

    if ((access(DEFAULT_MOBILEPINLOCK_CONFIG_DISABLE, F_OK)) == 0) {
        MOBILE_DEBUG("disable pinlock with sim.\n");
        if (access(PINLOCK_STATUS_FILE, F_OK) == 0) {
            if (mobile_disable_pinlock(g_mobile_pin_info.pin_code) == 0) {
                unlink(PINLOCK_STATUS_FILE);
            }
        }
        unlink(DEFAULT_MOBILEPINLOCK_CONFIG_DISABLE);
    }
}