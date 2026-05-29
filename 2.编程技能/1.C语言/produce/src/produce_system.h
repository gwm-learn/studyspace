#ifndef __PRODUCE_SYSTEM_H__
#define __PRODUCE_SYSTEM_H__

void produce_restore_defaults(int param_flag, char* param);
void produce_config_check(int param_flag, char* param);
void produce_reset_flag(int param_flag, char* param);
void produce_config_write(int param_flag, char* param);
void produce_sn(int param_flag, char* param);
void produce_fota_url(int param_flag, char* param);
void produce_admin_passwd(int param_flag, char* param);
void produce_product_class(int param_flag, char* param);
void produce_software_version(int param_flag, char* param);
void produce_hardware_version(int param_flag, char* param);
void produce_factory_mode(int param_flag, char* param);
void produce_encrypt_pass(int param_flag, char* param);
void produce_show_all_factory_params(int param_flag, char* param);

#endif
