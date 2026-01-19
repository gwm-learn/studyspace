#ifndef MOBILE_SERVICE_SMS_H
#define MOBILE_SERVICE_SMS_H

typedef struct _mobile_codes_t {
    char code1[16];
    char code2[16];
    char code3[16];
    char code4[16];
} mobile_codes_t;

typedef struct _mobile_pin_info_t {
    char pin_code[8];
    int first_time;
} mobile_pin_info_t;

int mobile_init_sms_service(void);
void mobile_deinit_sms_service(void);

void mobile_generate_lock_code_str(const char* serialnum, const char* imei, char* code, int num);
int mobile_init_sms_codes(void);
int mobile_generate_pin(const char *imei, char *output);
int mobile_get_pincode_robust(char* output);
void mobile_init_pin(void);
void mobile_sms_pdu_handler(void *p_pdu);

void mobile_print_pin_stc_info(void);
bool mobile_sub_pin_lock_stc(void);
void mobile_update_pin_lock_stc(void);
#endif