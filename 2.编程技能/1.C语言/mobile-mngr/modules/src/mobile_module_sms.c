#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>
#include <time.h>
#include "ql_sms.h"
#include "ql_dm.h"

#include "mobile_module_util.h"
#include "mobile_module_sms.h"

/*
 * 文件名称：mobile_module_sms.c
 * 功能描述：
 *     SMS短信处理模块，提供PDU解码和长短信组装功能
 *
 * 作者：gaoweiming
 */

/**
 * GSM 7-bit特殊字符映射表
 * 用于将GSM 7-bit编码的特殊字符转换为ISO-8859字符
 * 部分字符需要前导0x1B转义字符
 */
struct gsm_7bit_info gsm7b_info[] = {
    {0x00, 0x40}, {0x02, 0x24}, {0x11, 0x5f}, {0x14, 0x5e}, /* 下面的都需要上一个字符为 0x1B */
    {0x28, 0x7b}, {0x29, 0x7d}, {0x2f, 0x5c}, {0x3c, 0x5b}, {0x3d, 0x7e}, {0x3e, 0x5d}, {0x40, 0x7c},
};

/**
 * GSM 7-bit默认字符集映射表
 * 包含128个字符，对应GSM 7-bit编码的0x00-0x7F
 * 包括基本ASCII字符、特殊符号和扩展字符
 */
static const char gsm_7bit_chars[] = {
    '@', '£', '$', '¥', 'è', 'é', 'ù', 'ì', 'ò', 'Ç', '\n', 'Ø', 'ø', '\r', 'Å', 'å',
    'Δ', '_', 'Φ', 'Γ', 'Λ', 'Ω', 'Π', 'Ψ', 'Σ', 'Θ', 'Ξ', '\x1B', 'Æ', 'æ', 'ß', 'É',
    ' ', '!', '"', '#', '¤', '%', '&', '\'', '(', ')', '*', '+', ',', '-', '.', '/',
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', ':', ';', '<', '=', '>', '?',
    '¡', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O',
    'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'Ä', 'Ö', 'Ñ', 'Ü', '§',
    '¿', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o',
    'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', 'ä', 'ö', 'ñ', 'ü', 'à'
};

/**
 * 转换GSM 7-bit特殊字符到ISO-8859字符
 * 用于处理GSM字符集中的特殊字符映射
 *
 * @param ch 指向要转换的字符的指针
 * @return 无返回值，直接修改传入的字符
 */
static void chg_special_char(unsigned char* ch) {
    if (ch == NULL) {
        MOBILE_ERROR("Invalid parameter: ch is NULL\n");
        return;
    }
    
    for (int i = 0; i < sizeof(gsm7b_info) / sizeof(gsm7b_info[0]); i++) {
        if (*ch == gsm7b_info[i].gsm_7bit_ch) {
            *ch = gsm7b_info[i].iso_8859_ch;
            break;
        }
    }
}

/**
 * 将ISO-8859字符转换回GSM 7-bit特殊字符
 * 用于编码过程中的字符反向映射
 *
 * @param ch 指向要转换的字符的指针
 * @return 无返回值，直接修改传入的字符
 */
static void chg2gsm7b_special_char(unsigned char* ch) {
    if (ch == NULL) {
        MOBILE_ERROR("Invalid parameter: ch is NULL\n");
        return;
    }
    
    for (int i = 0; i < sizeof(gsm7b_info) / sizeof(gsm7b_info[0]); i++) {
        if (*ch == gsm7b_info[i].iso_8859_ch) {
            *ch = gsm7b_info[i].gsm_7bit_ch;
            break;
        }
    }
}

/**
 * GSM扩展字符映射表
 * 这些字符需要前导0x1B转义字符才能正确解码
 * 包括花括号、方括号、反斜杠等特殊符号
 */
static const struct {
    uint8_t gsm_char;
    char ascii_char;
} gsm_extended_chars[] = {
    {0x0A, '\f'}, {0x14, '^'}, {0x28, '{'}, {0x29, '}'},
    {0x2F, '\\'}, {0x3C, '['}, {0x3D, '~'}, {0x3E, ']'},
    {0x40, '|'}, {0x65, '€'}
};

// ==================== 工具函数 ====================

/**
 * 将十六进制字符转换为对应的整数值
 * 支持大小写十六进制字符(0-9, A-F, a-f)
 *
 * @param c 要转换的十六进制字符
 * @return 对应的整数值(0-15)，失败返回-1
 */
static int hex_char_to_int(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return -1;
}

/**
 * 将十六进制字符串转换为字节数组
 * 将每两个十六进制字符转换为一个字节
 *
 * @param hex_str 输入的十六进制字符串
 * @param bytes 输出的字节数组缓冲区
 * @param max_len 字节数组的最大长度
 * @return 转换后的字节数，失败返回-1
 */
static int hex_string_to_bytes(const char* hex_str, uint8_t* bytes, int max_len) {
    if (hex_str == NULL || bytes == NULL) {
        MOBILE_ERROR("Invalid parameters: hex_str=%p, bytes=%p\n", hex_str, bytes);
        return -1;
    }
    
    if (max_len <= 0) {
        MOBILE_ERROR("Invalid max_len: %d\n", max_len);
        return -1;
    }
    
    int len = strlen(hex_str);
    if (len % 2 != 0) {
        MOBILE_ERROR("Hex string length not even: %d\n", len);
        return -1;
    }
    
    int byte_count = len / 2;
    if (byte_count > max_len) {
        MOBILE_ERROR("Hex string too long: %d bytes, max=%d\n", byte_count, max_len);
        return -1;
    }
    
    for (int i = 0; i < len; i += 2) {
        int high = hex_char_to_int(hex_str[i]);
        int low = hex_char_to_int(hex_str[i + 1]);
        
        if (high < 0 || low < 0) {
            MOBILE_ERROR("Invalid hex character at position %d: '%c%c'\n",
                        i, hex_str[i], hex_str[i+1]);
            return -1;
        }
        
        bytes[i / 2] = (high << 4) | low;
    }
    
    return byte_count;
}

/**
 * 交换字节中的高低半字节(nibble)
 * 用于BCD编码的电话号码和时间戳处理
 *
 * @param src 源字符串
 * @param dst 目标字符串缓冲区
 * @param length 要处理的字符串长度
 * @return 无返回值
 */
static void swap_nibbles(const char* src, char* dst, int length) {
    if (src == NULL || dst == NULL) {
        MOBILE_ERROR("Invalid parameters: src=%p, dst=%p\n", src, dst);
        return;
    }
    
    if (length <= 0) {
        MOBILE_ERROR("Invalid length: %d\n", length);
        return;
    }
    
    for (int i = 0; i < length; i += 2) {
        if (i + 1 < length) {
            dst[i] = src[i + 1];
            dst[i + 1] = src[i];
        } else {
            dst[i] = src[i];
        }
    }
    dst[length] = '\0';
}

/**
 * 解码BCD编码的电话号码
 * 处理国际号码前缀和号码格式转换
 *
 * @param src 源BCD编码字符串
 * @param dst 目标电话号码缓冲区
 * @param length 源字符串长度
 * @param type 输出地址类型
 * @return 解码后的电话号码长度，失败返回-1
 */
static int decode_bcd_number(const char* src, char* dst, int length, address_type_t* type) {
    if (src == NULL || dst == NULL || type == NULL) {
        MOBILE_ERROR("Invalid parameters: src=%p, dst=%p, type=%p\n", src, dst, type);
        return -1;
    }
    
    if (length <= 0) {
        MOBILE_ERROR("Invalid length: %d\n", length);
        return -1;
    }
    
    char swapped[32];
    if (length >= sizeof(swapped)) {
        MOBILE_ERROR("BCD number too long: %d, max=%zu\n", length, sizeof(swapped)-1);
        return -1;
    }
    
    swap_nibbles(src, swapped, length);
    
    // 移除末尾的F
    int actual_len = length;
    if (actual_len > 0 && swapped[length - 1] == 'F') {
        actual_len--;
    }
    
    // 添加国际号码前缀
    int dst_index = 0;
    *type = ADDRESS_TYPE_UNKNOWN;
    
    if (actual_len >= 2 && swapped[0] == '9' && swapped[1] == '1') {
        dst[dst_index++] = '+';
        *type = ADDRESS_TYPE_INTERNATIONAL;
        // 跳过类型字节
        int copy_len = actual_len - 2;
        if (copy_len > 0) {
            strncpy(dst + dst_index, swapped + 2, copy_len);
            dst_index += copy_len;
        }
    } else {
        strncpy(dst, swapped, actual_len);
        dst_index = actual_len;
    }
    
    dst[dst_index] = '\0';
    return dst_index;
}

/**
 * 解码SMS时间戳
 * 将BCD编码的时间戳转换为可读的日期时间格式
 * 格式: YY MM DD HH MM SS TZ
 *
 * @param src 源时间戳字符串
 * @param dst 目标时间戳缓冲区
 * @return 无返回值
 */
static void decode_timestamp(const char* src, char* dst) {
    if (src == NULL || dst == NULL) {
        MOBILE_ERROR("Invalid parameters: src=%p, dst=%p\n", src, dst);
        return;
    }
    
    char swapped[15];
    swap_nibbles(src, swapped, 14);
    
    // 格式: YY MM DD HH MM SS TZ
    int year = (swapped[0] - '0') * 10 + (swapped[1] - '0');
    int month = (swapped[2] - '0') * 10 + (swapped[3] - '0');
    int day = (swapped[4] - '0') * 10 + (swapped[5] - '0');
    int hour = (swapped[6] - '0') * 10 + (swapped[7] - '0');
    int minute = (swapped[8] - '0') * 10 + (swapped[9] - '0');
    int second = (swapped[10] - '0') * 10 + (swapped[11] - '0');
    
    // 验证日期时间值的有效性
    if (year < 0 || year > 99) {
        MOBILE_WARN("Invalid year in timestamp: %d\n", year);
        year = 0;
    }
    if (month < 1 || month > 12) {
        MOBILE_WARN("Invalid month in timestamp: %d\n", month);
        month = 1;
    }
    if (day < 1 || day > 31) {
        MOBILE_WARN("Invalid day in timestamp: %d\n", day);
        day = 1;
    }
    if (hour < 0 || hour > 23) {
        MOBILE_WARN("Invalid hour in timestamp: %d\n", hour);
        hour = 0;
    }
    if (minute < 0 || minute > 59) {
        MOBILE_WARN("Invalid minute in timestamp: %d\n", minute);
        minute = 0;
    }
    if (second < 0 || second > 59) {
        MOBILE_WARN("Invalid second in timestamp: %d\n", second);
        second = 0;
    }
    
    int timezone = (swapped[12] - '0') * 10 + (swapped[13] - '0');
    int tz_hours = (timezone & 0x7F) / 4;
    if (timezone & 0x80) {
        tz_hours = -tz_hours;  // 西时区
    }
    
    // 计算时区符号
    char tz_sign = (timezone & 0x80) ? '-' : '+';
    tz_hours = abs(tz_hours);  // 取绝对值用于显示
    
    int ret = snprintf(dst, MAX_TIMESTAMP_LENGTH, "20%02d-%02d-%02d %02d:%02d:%02d %c%02d:00",
             year, month, day, hour, minute, second, tz_sign, tz_hours);
    
    if (ret < 0 || ret >= MAX_TIMESTAMP_LENGTH) {
        MOBILE_ERROR("Timestamp buffer overflow: ret=%d, max=%d\n", ret, MAX_TIMESTAMP_LENGTH);
        dst[0] = '\0';
    }
}

// ==================== 解码函数 ====================
/**
 * 统一的7-bit解码函数 - 基于SHORTMSG_Decode7bit重写
 * 输入: src - 源编码数据指针
 *       src_len - 源数据长度
 *       dst - 目标字符串指针
 *       dst_size - 目标缓冲区大小
 *       use_char_mapping - 是否使用GSM字符集映射
 * 返回: 目标字符串长度，失败返回-1
 */
static int decode_7bit_gsm_unified(const uint8_t* src, int src_len, char* dst, int dst_size, int use_char_mapping) {
    // 参数验证
    if (!src || !dst) {
        MOBILE_ERROR("Invalid parameters: src=%p, dst=%p\n", src, dst);
        return -1;
    }
    
    if (src_len <= 0) {
        MOBILE_ERROR("Invalid source length: %d\n", src_len);
        return -1;
    }
    
    if (dst_size <= 1) {
        MOBILE_ERROR("Invalid destination size: %d (must be at least 2)\n", dst_size);
        return -1;
    }

    int src_index = 0;           // 源数据索引
    int dst_index = 0;           // 目标数据索引
    int byte_position = 0;       // 当前处理的组内字节序号 (0-6)
    uint8_t carry_data = 0;      // 上一字节残余的数据
    uint8_t extended_flag = 0;   // 扩展字符标志
    uint8_t temp_char;

    // 初始化目标缓冲区
    memset(dst, 0, dst_size);

    // 将源数据每7个字节分为一组，解压缩成8个字符
    while (src_index < src_len && dst_index < dst_size - 1) {
        // 提取当前字节
        uint8_t current_byte = src[src_index];
        
        // 将源字节右边部分与残余数据相加，去掉最高位，得到一个目标解码字节
        temp_char = ((current_byte << byte_position) | carry_data) & 0x7F;
        
        // 处理特殊字符
        if (temp_char == 0x00 || temp_char == 0x02 || temp_char == 0x11) {
            chg_special_char(&temp_char);
        } else if (temp_char == 0x1B) {
            // 遇到转义字符，设置标志等待下一个字符
            extended_flag = 1;
        } else if (extended_flag) {
            // 处理扩展字符
            chg_special_char(&temp_char);
            // 回退一个字符位置（覆盖之前的0x1B）
            if (dst_index > 0) {
                dst_index--;
            }
            extended_flag = 0;
        }
        
        // 存储解码后的字符（除非是单独的0x1B）
        if (!(temp_char == 0x1B && !extended_flag)) {
            if (use_char_mapping) {
                // 使用GSM字符集映射
                if (temp_char < sizeof(gsm_7bit_chars)) {
                    dst[dst_index] = gsm_7bit_chars[temp_char];
                } else {
                    dst[dst_index] = '?'; // 未知字符
                    MOBILE_DEBUG("Unknown GSM 7-bit character: 0x%02X\n", temp_char);
                }
            } else {
                // 直接输出原始字节值
                dst[dst_index] = temp_char;
            }
            dst_index++;
        }
        
        // 将该字节剩下的左边部分，作为残余数据保存起来
        carry_data = current_byte >> (7 - byte_position);
        
        // 更新字节位置
        byte_position++;
        
        // 检查是否到了一组的最后一个字节
        if (byte_position == 7) {
            // 处理残余数据，得到一个额外的解码字节
            if (dst_index < dst_size - 1) {
                temp_char = carry_data & 0x7F;
                
                // 处理特殊字符
                if (temp_char == 0x00 || temp_char == 0x02 || temp_char == 0x11) {
                    chg_special_char(&temp_char);
                } else if (temp_char == 0x1B) {
                    extended_flag = 1;
                } else if (extended_flag) {
                    chg_special_char(&temp_char);
                    if (dst_index > 0) {
                        dst_index--;
                    }
                    extended_flag = 0;
                }
                
                // 存储解码后的字符
                if (!(temp_char == 0x1B && !extended_flag)) {
                    if (use_char_mapping) {
                        // 使用GSM字符集映射
                        if (temp_char < sizeof(gsm_7bit_chars)) {
                            dst[dst_index] = gsm_7bit_chars[temp_char];
                        } else {
                            dst[dst_index] = '?'; // 未知字符
                            MOBILE_DEBUG("Unknown GSM 7-bit character in carry: 0x%02X\n", temp_char);
                        }
                    } else {
                        // 直接输出原始字节值
                        dst[dst_index] = temp_char;
                    }
                    dst_index++;
                }
            }
            
            // 重置组内位置和残余数据
            byte_position = 0;
            carry_data = 0;
        }
        
        // 移动到下一个源字节
        src_index++;
        
        // 防止无限循环（安全检查）
        if (src_index > src_len * 2) {
            MOBILE_ERROR("Possible infinite loop detected in 7-bit decoding\n");
            break;
        }
    }
    
    // 确保字符串以null结尾
    if (dst_index < dst_size) {
        dst[dst_index] = '\0';
    } else {
        dst[dst_size - 1] = '\0';
        MOBILE_WARN("7-bit decoding buffer overflow: dst_index=%d, dst_size=%d\n", dst_index, dst_size);
    }
    
    MOBILE_DEBUG("7-bit decoding completed: src_len=%d, dst_len=%d\n", src_len, dst_index);
    return dst_index;
}

/**
 * 7-bit解码函数 - 基于SHORTMSG_Decode7bit重写
 * 输入: src - 源编码数据指针
 *       src_len - 源数据长度
 *       dst - 目标字符串指针
 *       dst_size - 目标缓冲区大小
 * 返回: 目标字符串长度
 */
int decode_7bit_gsm(const uint8_t* src, int src_len, char* dst, int dst_size) {
    return decode_7bit_gsm_unified(src, src_len, dst, dst_size, 0); // 不使用字符映射
}

/**
 * 简化的7-bit解码函数 - 基于SHORTMSG_Decode7bit重写
 * 专门处理无UDH的情况
 */
static int decode_7bit_simple(const uint8_t* src, char* dst, int src_len) {
    return decode_7bit_gsm_unified(src, src_len, dst, MAX_SINGLE_MESSAGE, 1); // 使用字符映射
}

/**
 * 8-bit解码函数
 * 将8-bit编码数据转换为ASCII字符串
 * 只显示可打印ASCII字符(32-126)，其他字符替换为'.'
 *
 * @param src 源编码数据指针
 * @param dst 目标字符串指针
 * @param src_len 源数据长度
 * @return 目标字符串长度，失败返回-1
 */
static int decode_8bit(const uint8_t* src, char* dst, int src_len) {
    // 参数验证
    if (!src || !dst) {
        MOBILE_ERROR("Invalid parameters: src=%p, dst=%p\n", src, dst);
        return -1;
    }
    
    if (src_len <= 0) {
        MOBILE_ERROR("Invalid source length: %d\n", src_len);
        return -1;
    }
    
    if (src_len > MAX_SINGLE_MESSAGE - 1) {
        MOBILE_WARN("Source length %d exceeds maximum %d, truncating\n",
                   src_len, MAX_SINGLE_MESSAGE - 1);
        src_len = MAX_SINGLE_MESSAGE - 1;
    }
    
    int i;
    for (i = 0; i < src_len; i++) {
        // 只处理可打印ASCII字符(32-126)
        if (src[i] >= 32 && src[i] < 127) {
            dst[i] = src[i];
        } else {
            dst[i] = '.'; // 不可打印字符替换为点号
            MOBILE_DEBUG("Non-printable character at position %d: 0x%02X\n", i, src[i]);
        }
    }
    
    // 确保字符串以null结尾
    dst[i] = '\0';
    
    MOBILE_DEBUG("8-bit decoding completed: src_len=%d, dst_len=%d\n", src_len, i);
    return i;
}

/**
 * UCS-2解码函数
 * 将UCS-2编码的Unicode字符转换为UTF-8或转义序列
 * 基本ASCII字符直接输出，其他字符转换为\\uXXXX格式
 *
 * @param src 源编码数据指针
 * @param dst 目标字符串指针
 * @param src_len 源数据长度
 * @return 目标字符串长度
 */
static int decode_ucs2(const uint8_t* src, char* dst, int src_len) {
    // 参数验证
    if (!src || !dst) {
        MOBILE_ERROR("Invalid parameters: src=%p, dst=%p\n", src, dst);
        return -1;
    }
    
    if (src_len <= 0) {
        MOBILE_ERROR("Invalid source length: %d\n", src_len);
        return -1;
    }
    
    if (src_len % 2 != 0) {
        MOBILE_WARN("UCS-2 source length %d is not even, truncating last byte\n", src_len);
        src_len--; // 确保长度是偶数
    }
    
    int dst_index = 0;
    int processed_chars = 0;
    
    for (int i = 0; i < src_len && dst_index < MAX_SINGLE_MESSAGE - 6; i += 2) {
        // 检查是否有足够的源数据
        if (i + 1 >= src_len) {
            MOBILE_WARN("Incomplete UCS-2 character at position %d\n", i);
            break;
        }
        
        // 组合UCS-2字符
        uint16_t ucs2_char = (src[i] << 8) | src[i + 1];
        processed_chars++;
        
        // 处理字符串结束符
        if (ucs2_char == 0x0000) {
            MOBILE_DEBUG("UCS-2 null terminator found at position %d\n", i);
            break;
        }
        
        // 处理ASCII字符
        if (ucs2_char < 128) {
            dst[dst_index++] = (char)ucs2_char;
            MOBILE_DEBUG("UCS-2 ASCII character: 0x%04X -> '%c'\n", ucs2_char, (char)ucs2_char);
        }
        // 处理Unicode字符
        else {
            // 检查缓冲区是否足够容纳转义序列
            if (dst_index + 6 >= MAX_SINGLE_MESSAGE) {
                MOBILE_WARN("UCS-2 buffer overflow, stopping at position %d\n", i);
                break;
            }
            
            // 转换为\uXXXX格式
            int written = snprintf(dst + dst_index, 7, "\\u%04X", ucs2_char);
            if (written < 0 || written >= 7) {
                MOBILE_ERROR("Failed to format UCS-2 character 0x%04X\n", ucs2_char);
                break;
            }
            
            dst_index += written;
            MOBILE_DEBUG("UCS-2 Unicode character: 0x%04X -> \\u%04X\n", ucs2_char, ucs2_char);
        }
        
        // 防止无限循环（安全检查）
        if (processed_chars > src_len) {
            MOBILE_ERROR("Possible infinite loop in UCS-2 decoding\n");
            break;
        }
    }
    
    // 确保字符串以null结尾
    if (dst_index < MAX_SINGLE_MESSAGE) {
        dst[dst_index] = '\0';
    } else {
        dst[MAX_SINGLE_MESSAGE - 1] = '\0';
        MOBILE_WARN("UCS-2 decoding buffer overflow: dst_index=%d\n", dst_index);
    }
    
    MOBILE_DEBUG("UCS-2 decoding completed: src_len=%d, dst_len=%d, processed_chars=%d\n",
                src_len, dst_index, processed_chars);
    return dst_index;
}

// ==================== UDH解析 ====================

/**
 * 解析用户数据头(UDH)
 * 提取长短信的拼接信息，包括参考号、总部分数和当前部分号
 * 支持IEI 0x00和0x08两种信息元素标识符
 *
 * @param udh_data UDH数据指针
 * @param udh_len UDH数据长度
 * @param long_sms 长短信信息结构体指针
 * @return 成功返回0，失败返回-1
 */
static int parse_udh(const uint8_t* udh_data, int udh_len, long_sms_info_t* long_sms) {
    // 参数验证
    if (!udh_data) {
        MOBILE_ERROR("Invalid UDH data pointer: NULL\n");
        return -1;
    }
    
    if (udh_len < 3) {
        MOBILE_ERROR("Invalid UDH length: %d (minimum 3 required)\n", udh_len);
        return -1;
    }
    
    if (!long_sms) {
        MOBILE_ERROR("Invalid long_sms pointer: NULL\n");
        return -1;
    }
    
    // 初始化长短信信息
    memset(long_sms, 0, sizeof(long_sms_info_t));
    
    const uint8_t* p = udh_data;
    int remaining = udh_len;
    int element_count = 0;
    
    MOBILE_DEBUG("Parsing UDH: length=%d\n", udh_len);
    
    while (remaining >= 2) {
        element_count++;
        
        // 防止无限循环
        if (element_count > 10) {
            MOBILE_WARN("Too many UDH elements, possible corrupted data\n");
            break;
        }
        
        uint8_t iei = *p++;
        uint8_t iel = *p++;
        remaining -= 2;
        
        MOBILE_DEBUG("UDH Element %d: IEI=0x%02X, IEL=%d\n", element_count, iei, iel);
        
        // 验证信息元素长度
        if (iel > remaining) {
            MOBILE_ERROR("Invalid UDH element length: IEL=%d, remaining=%d\n", iel, remaining);
            break;
        }
        
        // 处理长短信拼接信息元素
        if (iei == 0x00 || iei == 0x08) {
            if (iel >= 3) {
                long_sms->is_long_sms = 1;
                long_sms->reference_number = p[0];
                long_sms->total_parts = p[1];
                long_sms->part_number = p[2];
                
                // 验证部分号的有效性
                if (long_sms->part_number < 1 || long_sms->part_number > long_sms->total_parts) {
                    MOBILE_WARN("Invalid part number: %d/%d\n",
                               long_sms->part_number, long_sms->total_parts);
                    // 尝试修复：如果部分号无效，设为1
                    if (long_sms->part_number < 1) {
                        long_sms->part_number = 1;
                    } else if (long_sms->part_number > long_sms->total_parts) {
                        long_sms->part_number = long_sms->total_parts;
                    }
                }
                
                // 复制消息ID（如果存在）
                if (iel > 3) {
                    int message_id_len = (iel - 3 > 4) ? 4 : (iel - 3);
                    memcpy(long_sms->message_id, p + 3, message_id_len);
                    MOBILE_DEBUG("Message ID copied: %d bytes\n", message_id_len);
                }
                
                MOBILE_DEBUG("Long SMS detected: ref=%d, part=%d/%d\n",
                           long_sms->reference_number,
                           long_sms->part_number,
                           long_sms->total_parts);
                break;
            } else {
                MOBILE_WARN("Invalid long SMS element length: %d (minimum 3 required)\n", iel);
            }
        } else {
            MOBILE_DEBUG("Skipping non-long-SMS UDH element: IEI=0x%02X\n", iei);
        }
        
        // 移动到下一个信息元素
        p += iel;
        remaining -= iel;
    }
    
    if (!long_sms->is_long_sms) {
        MOBILE_DEBUG("No long SMS information found in UDH\n");
    }
    
    return 0;
}

/**
 * 组装长短信
 * 将多部分短信按照参考号和部分号进行组装
 * 支持并发处理多个长短信的组装过程
 *
 * @param assembler 短信组装器结构体指针
 * @param part 当前接收到的短信部分解码结果
 * @return 组装状态：0-未完成，1-组装完成，-1-组装失败
 */
static int assemble_long_sms(sms_assembler_t* assembler, const pdu_decode_result_t* part) {
    // 检查是否为长短信部分
    if (!part->long_sms.is_long_sms) {
        MOBILE_DEBUG("Not a long SMS part, skipping assembly\n");
        return 0;
    }
    
    // 清理过期的组装
    mobile_cleanup_expired_assemblies(assembler, ASSEMBLY_TIMEOUT);
    
    // 查找现有的组装
    long_sms_assembly_t* assembly = NULL;
    for (int i = 0; i < assembler->count; i++) {
        if (assembler->assemblies[i].reference_number == part->long_sms.reference_number &&
            strcmp(assembler->assemblies[i].sender_number, part->sender_number) == 0) {
            assembly = &assembler->assemblies[i];
            MOBILE_DEBUG("Found existing assembly for ref=%d, sender=%s\n",
                        part->long_sms.reference_number, part->sender_number);
            break;
        }
    }
    
    // 如果没有找到现有组装，创建新的组装
    if (!assembly) {
        if (assembler->count >= MAX_CONCURRENT_ASSEMBLIES) {
            MOBILE_ERROR("Too many concurrent assemblies, cannot create new one\n");
            return -1;
        }
        assembly = &assembler->assemblies[assembler->count++];
        memset(assembly, 0, sizeof(long_sms_assembly_t));
        assembly->reference_number = part->long_sms.reference_number;
        strncpy(assembly->sender_number, part->sender_number, MAX_PHONE_LENGTH);
        assembly->sender_number[MAX_PHONE_LENGTH - 1] = '\0';
        assembly->total_parts = part->long_sms.total_parts;
        assembly->first_received_time = time(NULL);
        MOBILE_DEBUG("Created new assembly for ref=%d, sender=%s, total_parts=%d\n",
                    part->long_sms.reference_number, part->sender_number, assembly->total_parts);
    }
    
    // 存储当前部分
    int part_idx = part->long_sms.part_number - 1;
    if (part_idx >= 0 && part_idx < MAX_SMS_PARTS && part_idx < assembly->total_parts) {
        // 如果是新接收的部分，增加接收计数
        if (assembly->parts[part_idx].message[0] == '\0') {
            assembly->received_parts++;
            MOBILE_DEBUG("New part received: %d/%d\n", assembly->received_parts, assembly->total_parts);
        }
        
        // 存储部分消息
        strncpy(assembly->parts[part_idx].message, part->message, MAX_SINGLE_MESSAGE);
        assembly->parts[part_idx].message[MAX_SINGLE_MESSAGE - 1] = '\0';
        assembly->parts[part_idx].part_number = part->long_sms.part_number;
        assembly->parts[part_idx].received_time = time(NULL);
        assembly->last_received_time = time(NULL);
        
        MOBILE_DEBUG("Stored part %d: '%s'\n", part->long_sms.part_number, part->message);
    } else {
        MOBILE_WARN("Invalid part index: %d (total_parts=%d, max_parts=%d)\n",
                   part_idx, assembly->total_parts, MAX_SMS_PARTS);
    }
    
    // 检查是否所有部分都已接收
    if (assembly->received_parts >= assembly->total_parts) {
        assembly->is_complete = 1;
        
        // 组装完整消息
        char* dest = assembly->complete_message;
        dest[0] = '\0';
        
        for (int i = 0; i < assembly->total_parts; i++) {
            if (assembly->parts[i].message[0] != '\0') {
                // 安全地拼接消息部分
                size_t current_len = strlen(dest);
                size_t part_len = strlen(assembly->parts[i].message);
                
                if (current_len + part_len < MAX_LONG_MESSAGE) {
                    strcat(dest, assembly->parts[i].message);
                } else {
                    MOBILE_WARN("Long message buffer overflow, truncating\n");
                    strncat(dest, assembly->parts[i].message, MAX_LONG_MESSAGE - current_len - 1);
                    break;
                }
            }
        }
        
        MOBILE_INFO("Long SMS assembly completed: ref=%d, sender=%s, total_length=%zu\n",
                   assembly->reference_number, assembly->sender_number, strlen(dest));
        return 1;
    }
    
    MOBILE_DEBUG("Assembly in progress: %d/%d parts received\n", assembly->received_parts, assembly->total_parts);
    return 0;
}

/**
 * 测试特定的PDU解码
 * 使用预定义的PDU字符串测试解码功能
 * 验证解码结果的正确性
 *
 * @return 无返回值
 */
void mobile_test_specific_pdu() {
    MOBILE_DEBUG("=== Testing Fixed PDU Decoding ===\n");
    
    char pdu[] = "0891683109520000F0240BA15181419166F400005201027155402312F4F29C0E7ABBCBA0F65B5E06D1D3ED32";
    
    MOBILE_DEBUG("Input PDU: %s\n", pdu);
    MOBILE_DEBUG("PDU Length: %zu characters\n\n", strlen(pdu));
    
    // Validate PDU
    int valid = mobile_validate_pdu_length(pdu);
    if (valid != 0) {
        MOBILE_DEBUG("PDU validation failed, error code: %d\n", valid);
        return;
    }
    MOBILE_DEBUG("✓ PDU format validation passed\n");
    
    // Decode PDU
    pdu_decode_result_t result;
    int decode_result = mobile_pdu_decode(pdu, &result);
    
    if (decode_result == 0) {
        MOBILE_DEBUG("✓ PDU decoding successful\n\n");
        
        MOBILE_DEBUG("=== Decoding Results ===\n");
        MOBILE_DEBUG("SMSC: %s\n", result.smsc_number);
        MOBILE_DEBUG("Sender: %s\n", result.sender_number);
        MOBILE_DEBUG("Timestamp: %s\n", result.timestamp);
        
        const char* encoding_str;
        switch (result.encoding) {
            case SMS_ENCODING_7BIT: encoding_str = "7-bit GSM"; break;
            case SMS_ENCODING_8BIT: encoding_str = "8-bit"; break;
            case SMS_ENCODING_UCS2: encoding_str = "UCS-2"; break;
            default: encoding_str = "Unknown"; break;
        }
        MOBILE_DEBUG("Encoding: %s\n", encoding_str);
        
        MOBILE_DEBUG("Protocol ID: 0x%02X\n", result.protocol_id);
        MOBILE_DEBUG("Data Coding Scheme: 0x%02X\n", result.data_coding_scheme);
        
        if (result.long_sms.is_long_sms) {
            MOBILE_DEBUG("Long SMS: Yes (Reference: %d, Part %d/%d)\n",
                   result.long_sms.reference_number,
                   result.long_sms.part_number,
                   result.long_sms.total_parts);
        } else {
            MOBILE_DEBUG("Long SMS: No\n");
        }
        
        MOBILE_DEBUG("Message Status: %s\n", result.has_more_messages ? "More messages" : "No more messages");
        MOBILE_DEBUG("Message Length: %d characters\n", result.message_length);
        MOBILE_DEBUG("Message Content: '%s'\n", result.message);
        
        // Verify expected results
        MOBILE_DEBUG("\n=== Verification Results ===\n");
        if (strcmp(result.message, "test one more time") == 0) {
            MOBILE_DEBUG("✓ Message content verification passed: Correctly decoded as 'test one more time'\n");
        } else {
            MOBILE_DEBUG("✗ Message content verification failed\n");
            MOBILE_DEBUG("  Expected: 'test one more time'\n");
            MOBILE_DEBUG("  Actual: '%s'\n", result.message);
        }
        
    } else {
        MOBILE_DEBUG("✗ PDU decoding failed\n");
        MOBILE_DEBUG("Error Code: %d\n", result.error_code);
        MOBILE_DEBUG("Error Message: %s\n", result.error_message);
    }
    
    MOBILE_DEBUG("\n=== PDU Structure Analysis ===\n");
    MOBILE_DEBUG("SMSC Section: 08 91 68 31 09 52 00 00 F0\n");
    MOBILE_DEBUG("TPDU First Byte: 24\n");
    MOBILE_DEBUG("Sender Length: 0B\n");
    MOBILE_DEBUG("Sender Type: A1\n");
    MOBILE_DEBUG("Sender Number: 51 81 41 91 66 F4\n");
    MOBILE_DEBUG("Protocol ID: 00\n");
    MOBILE_DEBUG("Encoding Scheme: 00\n");
    MOBILE_DEBUG("Timestamp: 52 01 02 71 55 40 23\n");
    MOBILE_DEBUG("User Data Length: 12\n");
    MOBILE_DEBUG("User Data: F4 F2 9C 0E 7A BB CB A0 F6 5B 5E 06 D1 D3 ED 32\n");
}

/**
 * 主PDU解码函数
 * 解析完整的SMS PDU字符串，提取所有相关信息
 * 包括SMSC、发送方、时间戳、编码类型和消息内容
 *
 * @param pdu_str PDU十六进制字符串
 * @param result 解码结果结构体指针
 * @return 成功返回0，失败返回错误码
 */
int mobile_pdu_decode(const char* pdu_str, pdu_decode_result_t* result) {
    if (!pdu_str || !result) {
        return -1;
    }
    
    memset(result, 0, sizeof(pdu_decode_result_t));
    result->encoding = SMS_ENCODING_UNKNOWN;
    result->message_type = SMS_TYPE_DELIVER;
    
    int pdu_len = strlen(pdu_str);
    if (mobile_validate_pdu_length(pdu_str) != 0) {
        snprintf(result->error_message, sizeof(result->error_message), 
                "Invalid PDU length: %d", pdu_len);
        result->error_code = -1;
        return -1;
    }
    
    uint8_t pdu_bytes[MAX_PDU_BINARY_LENGTH];
    int byte_len = hex_string_to_bytes(pdu_str, pdu_bytes, MAX_PDU_BINARY_LENGTH);
    if (byte_len < 0) {
        snprintf(result->error_message, sizeof(result->error_message),
                "Invalid hex string");
        result->error_code = -2;
        return -1;
    }
    
    const uint8_t* p = pdu_bytes;
    const uint8_t* end = pdu_bytes + byte_len;
    
    // 1. 解析SMSC信息
    if (p >= end) {
        snprintf(result->error_message, sizeof(result->error_message),
                "PDU too short for SMSC");
        result->error_code = -3;
        return -1;
    }
    
    uint8_t smsc_len = *p++;
    if (smsc_len > 0) {
        if (p + smsc_len > end) {
            snprintf(result->error_message, sizeof(result->error_message),
                    "SMSC length exceeds PDU");
            result->error_code = -4;
            return -1;
        }
        
        uint8_t smsc_type = *p++;
        char smsc_hex[32];
        int hex_len = (smsc_len - 1) * 2;
        if (hex_len >= sizeof(smsc_hex)) {
            hex_len = sizeof(smsc_hex) - 1;
        }
        
        for (int i = 0; i < smsc_len - 1 && i < 15; i++) {
            snprintf(smsc_hex + i * 2, 3, "%02X", p[i]);
        }
        
        address_type_t type;
        decode_bcd_number(smsc_hex, result->smsc_number, hex_len, &type);
        p += smsc_len - 1;
    }
    
    // 2. 解析TPDU第一个字节
    if (p >= end) {
        snprintf(result->error_message, sizeof(result->error_message),
                "PDU too short for TPDU");
        result->error_code = -5;
        return -1;
    }
    
    uint8_t first_byte = *p++;
    result->message_type = (first_byte & 0x03);
    result->has_more_messages = (first_byte & 0x04) ? 1 : 0;
    
    // 3. 解析发信人号码
    if (p >= end) {
        snprintf(result->error_message, sizeof(result->error_message),
                "PDU too short for sender");
        result->error_code = -6;
        return -1;
    }
    
    uint8_t sender_len = *p++;
    if (sender_len > 0) {
        if (p + 1 > end) {
            snprintf(result->error_message, sizeof(result->error_message),
                    "PDU too short for sender type");
            result->error_code = -7;
            return -1;
        }
        
        uint8_t sender_type = *p++;
        int byte_count = (sender_len + 1) / 2;
        
        if (p + byte_count > end) {
            snprintf(result->error_message, sizeof(result->error_message),
                    "Sender number exceeds PDU");
            result->error_code = -8;
            return -1;
        }
        
        char sender_hex[32];
        int hex_len = byte_count * 2;
        if (hex_len >= sizeof(sender_hex)) {
            hex_len = sizeof(sender_hex) - 1;
        }
        
        for (int i = 0; i < byte_count && i < 15; i++) {
            snprintf(sender_hex + i * 2, 3, "%02X", p[i]);
        }
        
        decode_bcd_number(sender_hex, result->sender_number, hex_len, &result->sender_type);
        p += byte_count;
    }
    
    // 4. 解析协议标识和编码方案
    if (p + 2 > end) {
        snprintf(result->error_message, sizeof(result->error_message),
                "PDU too short for PID/DCS");
        result->error_code = -9;
        return -1;
    }
    
    result->protocol_id = *p++;
    result->data_coding_scheme = *p++;
    
    // 确定编码类型
    switch (result->data_coding_scheme & 0x0C) {
        case 0x00: result->encoding = SMS_ENCODING_7BIT; break;
        case 0x04: result->encoding = SMS_ENCODING_8BIT; break;
        case 0x08: result->encoding = SMS_ENCODING_UCS2; break;
        default: result->encoding = SMS_ENCODING_UNKNOWN; break;
    }
    
    // 5. 解析时间戳
    if (result->message_type == SMS_TYPE_DELIVER) {
        if (p + 7 > end) {
            snprintf(result->error_message, sizeof(result->error_message),
                    "PDU too short for timestamp");
            result->error_code = -10;
            return -1;
        }
        
        char timestamp_hex[15];
        for (int i = 0; i < 7; i++) {
            snprintf(timestamp_hex + i * 2, 3, "%02X", p[i]);
        }
        decode_timestamp(timestamp_hex, result->timestamp);
        p += 7;
    }
    
    // 6. 解析用户数据长度
    if (p >= end) {
        snprintf(result->error_message, sizeof(result->error_message),
                "PDU too short for UDL");
        result->error_code = -11;
        return -1;
    }
    
    uint8_t user_data_len = *p++;
    result->message_length = user_data_len;
    
    // 7. 解析用户数据
    if (user_data_len > 0) {
        int data_bytes_needed;
        
        switch (result->encoding) {
            case SMS_ENCODING_7BIT:
                data_bytes_needed = (user_data_len * 7 + 7) / 8;
                break;
            case SMS_ENCODING_8BIT:
            case SMS_ENCODING_UCS2:
                data_bytes_needed = user_data_len;
                break;
            default:
                data_bytes_needed = user_data_len;
                break;
        }
        
        if (p + data_bytes_needed > end) {
            snprintf(result->error_message, sizeof(result->error_message),
                    "User data exceeds PDU");
            result->error_code = -12;
            return -1;
        }
        
        // 检查UDH
        uint8_t udh_len = 0;
        long_sms_info_t long_sms = {0};
        
        // 检查是否有UDH（用户数据头）
        if (p < end && *p > 0 && *p <= data_bytes_needed - 1) {
            udh_len = *p;
            if (udh_len > 0 && p + udh_len + 1 <= end) {
                parse_udh(p + 1, udh_len, &long_sms);
                result->long_sms = long_sms;
            }
        }
        
        // 解码用户数据
        int decoded_len = 0;
        switch (result->encoding) {
            case SMS_ENCODING_7BIT:
                if (udh_len > 0) {
                    // 有UDH：跳过UDH部分，不使用字符映射
                    decoded_len = decode_7bit_gsm(p + udh_len + 1, data_bytes_needed - udh_len - 1, result->message, MAX_SINGLE_MESSAGE);
                } else {
                    // 无UDH：使用字符映射
                    decoded_len = decode_7bit_simple(p, result->message, data_bytes_needed);
                }
                break;
            case SMS_ENCODING_8BIT:
                decoded_len = decode_8bit(p, result->message, data_bytes_needed);
                break;
            case SMS_ENCODING_UCS2:
                decoded_len = decode_ucs2(p, result->message, data_bytes_needed);
                break;
            default:
                decoded_len = decode_8bit(p, result->message, data_bytes_needed);
                break;
        }
        
        if (decoded_len < 0) {
            snprintf(result->error_message, sizeof(result->error_message),
                    "Failed to decode message");
            result->error_code = -13;
            return -1;
        }
        
        result->message_length = decoded_len;
    }
    
    return 0;
}

/**
 * 带长短信组装的PDU解码函数
 * 解析PDU并自动组装长短信的各个部分
 * 支持多部分短信的自动拼接和超时清理
 *
 * @param pdu_str PDU十六进制字符串
 * @param result 解码结果结构体指针
 * @param assembler 短信组装器指针
 * @return 成功返回0，失败返回错误码
 */
int mobile_pdu_decode_with_assembly(const char* pdu_str, pdu_decode_result_t* result, sms_assembler_t* assembler) {
    // 参数验证
    if (!pdu_str || !result) {
        MOBILE_ERROR("Invalid parameters: pdu_str=%p, result=%p\n", pdu_str, result);
        return -1;
    }
    
    // 首先进行基本的PDU解码
    int ret = mobile_pdu_decode(pdu_str, result);
    if (ret != 0) {
        MOBILE_ERROR("Failed to decode PDU: error=%d\n", ret);
        return ret;
    }
    
    // 检查是否为长短信
    if (result->long_sms.is_long_sms) {
        result->is_part_of_long_sms = 1;
        result->part_index = result->long_sms.part_number;
        result->total_parts = result->long_sms.total_parts;
        
        MOBILE_DEBUG("Long SMS part detected: part %d/%d, ref=%d\n",
                   result->part_index, result->total_parts, result->long_sms.reference_number);
        
        // 如果有组装器，进行长短信组装
        if (assembler) {
            int assembly_status = assemble_long_sms(assembler, result);
            MOBILE_DEBUG("Assembly status: %d\n", assembly_status);
            
            if (assembly_status == 1) {
                // 组装完成，查找完整的组装消息
                for (int i = 0; i < assembler->count; i++) {
                    if (assembler->assemblies[i].reference_number == result->long_sms.reference_number &&
                        assembler->assemblies[i].is_complete) {
                        
                        // 复制组装后的完整消息
                        strncpy(result->assembled_message,
                               assembler->assemblies[i].complete_message,
                               MAX_LONG_MESSAGE);
                        result->assembled_message[MAX_LONG_MESSAGE - 1] = '\0'; // 确保null终止
                        result->assembly_complete = 1;
                        
                        MOBILE_DEBUG("Long SMS assembly completed: total_length=%zu\n",
                                   strlen(result->assembled_message));
                        break;
                    }
                }
            } else if (assembly_status < 0) {
                MOBILE_WARN("Long SMS assembly failed: status=%d\n", assembly_status);
            }
        } else {
            MOBILE_DEBUG("No assembler provided, skipping long SMS assembly\n");
        }
    } else {
        MOBILE_DEBUG("Single SMS, no assembly needed\n");
    }
    
    return 0;
}

/**
 * 初始化短信组装器
 * 用于初始化长短信组装器结构体，设置初始状态和清理时间
 * 为多部分短信的自动拼接提供基础数据结构
 *
 * @param assembler 短信组装器结构体指针
 * @return 无返回值
 */
void mobile_init_sms_service_assembler(sms_assembler_t* assembler) {
    if (assembler) {
        memset(assembler, 0, sizeof(sms_assembler_t));
        assembler->last_cleanup_time = time(NULL);
        MOBILE_DEBUG("SMS service assembler initialized successfully\n");
    } else {
        MOBILE_ERROR("Invalid assembler pointer: NULL\n");
    }
}

/**
 * 清理过期的短信组装
 * 定期清理超时未完成的短信组装，释放资源并防止内存泄漏
 * 只保留已完成的组装或最近接收到的部分短信
 *
 * @param assembler 短信组装器结构体指针
 * @param timeout_seconds 超时时间（秒），超过此时间的未完成组装将被清理
 * @return 无返回值
 */
void mobile_cleanup_expired_assemblies(sms_assembler_t* assembler, int timeout_seconds) {
    if (!assembler) {
        MOBILE_ERROR("Invalid assembler pointer: NULL\n");
        return;
    }
    
    time_t now = time(NULL);
    // 限制清理频率，避免过于频繁的清理操作
    if (now - assembler->last_cleanup_time < 60) {
        MOBILE_DEBUG("Cleanup too frequent, skipping this time\n");
        return;
    }
    
    int write_index = 0;
    int cleaned_count = 0;
    
    // 遍历所有组装，保留有效的组装
    for (int i = 0; i < assembler->count; i++) {
        // 保留已完成的组装或未超时的组装
        if (assembler->assemblies[i].is_complete ||
            (now - assembler->assemblies[i].last_received_time) < timeout_seconds) {
            if (write_index != i) {
                assembler->assemblies[write_index] = assembler->assemblies[i];
            }
            write_index++;
        } else {
            cleaned_count++;
            MOBILE_DEBUG("Cleaned expired assembly: ref=%d, sender=%s\n",
                        assembler->assemblies[i].reference_number,
                        assembler->assemblies[i].sender_number);
        }
    }
    
    assembler->count = write_index;
    assembler->last_cleanup_time = now;
    
    if (cleaned_count > 0) {
        MOBILE_INFO("Cleaned %d expired SMS assemblies, remaining: %d\n", cleaned_count, assembler->count);
    }
}

/**
 * 打印PDU解码结果
 * 以可读格式输出解码后的所有信息
 * 用于调试和测试目的
 *
 * @param result 解码结果结构体指针
 * @return 无返回值
 */
void mobile_print_decode_result(const pdu_decode_result_t* result) {
    if (!result) return;
    
    MOBILE_DEBUG("=== SMS PDU Decode Result ===\n");
    MOBILE_DEBUG("PDU SMSC: %s\n", result->smsc_number);
    MOBILE_DEBUG("PDU Sender: %s\n", result->sender_number);
    MOBILE_DEBUG("PDU Timestamp: %s\n", result->timestamp);
    
    const char* encoding_str = "Unknown";
    switch (result->encoding) {
        case SMS_ENCODING_7BIT: encoding_str = "7bit"; break;
        case SMS_ENCODING_8BIT: encoding_str = "8bit"; break;
        case SMS_ENCODING_UCS2: encoding_str = "UCS2"; break;
    }
    MOBILE_DEBUG("PDU Encoding: %s\n", encoding_str);
    
    const char* sms_type_str;
    switch (result->message_type) {
        case SMS_TYPE_DELIVER: sms_type_str = "DELIVER"; break;
        case SMS_TYPE_SUBMIT: sms_type_str = "SUBMIT"; break;
        case SMS_TYPE_STATUS_REPORT: sms_type_str = "STATUS_REPORT"; break;
        default: sms_type_str = "UNKNOWN"; break;
    }
    MOBILE_DEBUG("PDU SMS Type: %s\n", sms_type_str);
    
    MOBILE_DEBUG("PDU Protocol ID: 0x%02X\n", result->protocol_id);
    MOBILE_DEBUG("PDU Data Coding Scheme: 0x%02X\n", result->data_coding_scheme);
    MOBILE_DEBUG("PDU More Messages: %s\n", result->has_more_messages ? "Yes" : "No");
    
    if (result->long_sms.is_long_sms) {
        MOBILE_DEBUG("PDU Long SMS: Part %d/%d, Ref: %d\n",
               result->long_sms.part_number,
               result->long_sms.total_parts,
               result->long_sms.reference_number);
    }
    
    MOBILE_DEBUG("PDU Message Length: %d chars\n", result->message_length);
    MOBILE_DEBUG("PDU Message Content: '%s'\n", result->message);
    
    if (result->error_code != 0) {
        MOBILE_DEBUG("PDU Error %d: %s\n", result->error_code, result->error_message);
    }
    MOBILE_DEBUG("PDU =============================\n");
}

/**
 * 验证PDU字符串长度和格式
 * 检查长度、字符有效性等基本格式要求
 *
 * @param pdu_str PDU十六进制字符串
 * @return 成功返回0，失败返回错误码
 */
int mobile_validate_pdu_length(const char* pdu_str) {
    int len = strlen(pdu_str);
    
    if (len < 20) {
        return -1;
    }
    
    if (len > MAX_PDU_HEX_LENGTH) {
        return -2;
    }
    
    if (len % 2 != 0) {
        return -3;
    }
    
    for (int i = 0; i < len; i++) {
        if (!isxdigit(pdu_str[i])) {
            return -4;
        }
    }
    
    return 0;
}


/**
 * 将二进制十六进制数据转换为十六进制字符串
 * 用于将PDU二进制数据转换为可读的十六进制格式
 *
 * @param bin_hex_data 二进制十六进制数据指针
 * @param data_size 数据大小
 * @param hex_str 输出的十六进制字符串缓冲区
 * @param hex_str_size 十六进制字符串缓冲区大小
 * @return 转换后的字符串长度，失败返回-1
 */
int mobile_bin_hex_to_hex_string(const char *bin_hex_data, int data_size, char *hex_str, int hex_str_size) {
    if (!bin_hex_data || !hex_str || data_size < 0) {
        return -1;
    }
    
    int required_size = data_size * 2 + 1;
    if (hex_str_size < required_size) {
        return -1;
    }
    
    static const char hex_chars[] = "0123456789ABCDEF";
    
    for (int i = 0; i < data_size; i++) {
        unsigned char byte = (unsigned char)bin_hex_data[i];
        hex_str[i * 2] = hex_chars[(byte >> 4) & 0x0F];
        hex_str[i * 2 + 1] = hex_chars[byte & 0x0F];
    }
    
    hex_str[data_size * 2] = '\0';
    return data_size * 2;
}

/**
 * 简化的二进制十六进制数据转换函数
 * 使用静态缓冲区，适用于单次转换场景
 *
 * @param bin_hex_data 二进制十六进制数据指针
 * @param data_size 数据大小
 * @return 转换后的十六进制字符串指针，失败返回NULL
 */
const char* mobile_bin_hex_to_hex_string_simple(const char *bin_hex_data, int data_size) {
    static char hex_str[QL_SMS_MAX_SEND_PDU_LENGTH * 2 + 1];
    
    if (!bin_hex_data || data_size <= 0 || data_size > QL_SMS_MAX_SEND_PDU_LENGTH) {
        return NULL;
    }
    
    int result = mobile_bin_hex_to_hex_string(bin_hex_data, data_size, hex_str, sizeof(hex_str));
    if (result > 0) {
        return hex_str;
    }
    
    return NULL;
}

/**
 * 从二进制十六进制数据解码PDU
 * 将二进制格式的PDU数据转换为十六进制字符串后进行解码
 * 适用于从二进制源接收PDU数据的场景
 *
 * @param bin_hex_pdu_data 二进制十六进制PDU数据指针
 * @param data_size 二进制数据大小
 * @param result 解码结果结构体指针
 * @return 成功返回0，失败返回错误码
 */
int mobile_pdu_decode_from_bin_hex(const char *bin_hex_pdu_data, int data_size, pdu_decode_result_t *result) {
    if (!bin_hex_pdu_data || !result) {
        return -1;
    }
    
    char hex_str[QL_SMS_MAX_SEND_PDU_LENGTH * 2 + 1];
    int hex_len = mobile_bin_hex_to_hex_string(bin_hex_pdu_data, data_size, hex_str, sizeof(hex_str));
    
    if (hex_len < 0) {
        snprintf(result->error_message, sizeof(result->error_message),
                "Failed to convert binary hex to hex string");
        result->error_code = -100;
        return -1;
    }
    
    return mobile_pdu_decode(hex_str, result);
}

/**
 * 从二进制十六进制数据解码PDU并组装长短信
 * 将二进制格式的PDU数据转换为十六进制字符串后进行解码和长短信组装
 * 支持多部分短信的自动拼接和超时清理
 *
 * @param bin_hex_pdu_data 二进制十六进制PDU数据指针
 * @param data_size 二进制数据大小
 * @param result 解码结果结构体指针
 * @param assembler 短信组装器指针
 * @return 成功返回0，失败返回错误码
 */
int mobile_pdu_decode_with_assembly_from_bin_hex(const char *bin_hex_pdu_data, int data_size,
                                         pdu_decode_result_t *result, sms_assembler_t *assembler) {
    if (!bin_hex_pdu_data || !result) {
        return -1;
    }
    
    char hex_str[QL_SMS_MAX_SEND_PDU_LENGTH * 2 + 1];
    int hex_len = mobile_bin_hex_to_hex_string(bin_hex_pdu_data, data_size, hex_str, sizeof(hex_str));
    
    if (hex_len < 0) {
        snprintf(result->error_message, sizeof(result->error_message),
                "Failed to convert binary hex to hex string");
        result->error_code = -100;
        return -1;
    }
    
    return mobile_pdu_decode_with_assembly(hex_str, result, assembler);
}
