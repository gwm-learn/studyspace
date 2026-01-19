#ifndef MOBILE_MODULE_SMS_H
#define MOBILE_MODULE_SMS_H

// 常量定义
#define MAX_PDU_HEX_LENGTH 2048        // PDU十六进制字符串最大长度
#define MAX_PDU_BINARY_LENGTH 1024     // PDU二进制数据最大长度
#define MAX_PHONE_LENGTH 32
#define MAX_TIMESTAMP_LENGTH 24
#define MAX_SINGLE_MESSAGE 160         // 单条短信最大字符数
#define MAX_LONG_MESSAGE 32768         // 长短信最大字符数（32KB）
#define MAX_SMS_PARTS 255              // 最大短信部分数
#define MAX_CONCURRENT_ASSEMBLIES 50   // 最大并发组装数
#define ASSEMBLY_TIMEOUT 300           // 组装超时时间（秒）
#define QL_SMS_MAX_SEND_PDU_LENGTH 255 // 最大PDU长度

struct gsm_7bit_info {
    unsigned char gsm_7bit_ch;
    unsigned char iso_8859_ch;
};

// 编码类型
typedef enum {
    SMS_ENCODING_7BIT = 0,
    SMS_ENCODING_8BIT = 1,
    SMS_ENCODING_UCS2 = 2,
    SMS_ENCODING_UNKNOWN = 3
} sms_encoding_type_t;

// 短信类型
typedef enum {
    SMS_TYPE_DELIVER = 0,      // 接收短信
    SMS_TYPE_SUBMIT = 1,       // 发送短信  
    SMS_TYPE_STATUS_REPORT = 2 // 状态报告
} sms_message_type_t;

// 地址类型
typedef enum {
    ADDRESS_TYPE_UNKNOWN = 0,
    ADDRESS_TYPE_INTERNATIONAL = 1,
    ADDRESS_TYPE_NATIONAL = 2,
    ADDRESS_TYPE_ALPHANUMERIC = 3
} address_type_t;

// 长短信信息
typedef struct {
    uint8_t message_id[4];
    uint8_t is_long_sms;
    uint8_t reference_number;
    uint8_t total_parts;
    uint8_t part_number;
    uint8_t udh_length;
} long_sms_info_t;

// 单条短信部分
typedef struct {
    char message[MAX_SINGLE_MESSAGE + 1];
    uint8_t part_number;
    time_t received_time;
} sms_part_t;

// 长短信组装记录
typedef struct {
    char complete_message[MAX_LONG_MESSAGE + 1];
    sms_part_t parts[MAX_SMS_PARTS];
    char sender_number[MAX_PHONE_LENGTH];
    uint8_t reference_number;
    uint8_t total_parts;
    uint8_t received_parts;
    time_t first_received_time;
    time_t last_received_time;
    uint8_t is_complete;
    
} long_sms_assembly_t;

// 全局长短信组装器
typedef struct {
    long_sms_assembly_t assemblies[MAX_CONCURRENT_ASSEMBLIES];
    int count;
    time_t last_cleanup_time;
} sms_assembler_t;

// 解码结果
typedef struct {
    char assembled_message[MAX_LONG_MESSAGE + 1];
    char message[MAX_SINGLE_MESSAGE + 1];
    char smsc_number[MAX_PHONE_LENGTH];
    char sender_number[MAX_PHONE_LENGTH];
    char timestamp[MAX_TIMESTAMP_LENGTH];
    char error_message[128];

    sms_encoding_type_t encoding;
    sms_message_type_t message_type;
    address_type_t sender_type;
    long_sms_info_t long_sms;
    uint8_t is_part_of_long_sms;
    uint8_t part_index;
    uint8_t total_parts;
    uint8_t assembly_complete;

    int error_code;
    int message_length;
    int has_more_messages;
    int validity_period;
    int protocol_id;
    int data_coding_scheme;
} pdu_decode_result_t;

// 公共函数声明
int mobile_pdu_decode(const char* pdu_str, pdu_decode_result_t* result);
int mobile_pdu_decode_with_assembly(const char* pdu_str, pdu_decode_result_t* result, sms_assembler_t* assembler);
void mobile_init_sms_service_assembler(sms_assembler_t* assembler);
void mobile_cleanup_expired_assemblies(sms_assembler_t* assembler, int timeout_seconds);
void mobile_print_decode_result(const pdu_decode_result_t* result);
int mobile_validate_pdu_length(const char* pdu_str);

// 二进制十六进制数据转换函数
int mobile_bin_hex_to_hex_string(const char *bin_hex_data, int data_size, char *hex_str, int hex_str_size);
const char* mobile_bin_hex_to_hex_string_simple(const char *bin_hex_data, int data_size);

// 二进制十六进制PDU解码函数
int mobile_pdu_decode_from_bin_hex(const char *bin_hex_pdu_data, int data_size, pdu_decode_result_t *result);
int mobile_pdu_decode_with_assembly_from_bin_hex(const char *bin_hex_pdu_data, int data_size,
                                         pdu_decode_result_t *result, sms_assembler_t *assembler);

#endif // MOBILE_MODULE_SMS_H