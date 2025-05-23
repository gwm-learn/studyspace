#ifndef _SYSTEM_INFO_H_
#define _SYSTEM_INFO_H_

#define BUILDFILE               "/etc/ccdver"
#define BUILDTIME_STR           "buildTime:"
#define CUSTOMIZE_INFO_FILE     "/etc/customize.mk"
#define SOFTVERSION_STRING      "CUS_PARAMS_SOFTVERSION="
#define HWVERSION_STRING        "CUS_PARAMS_HARDVERSION="
#define PRODUCE_SN_STRING       "CUS_PARAMS_SN="

void init_system_info(void);
void init_system_info_node(void);
void init_global_info_node(void);
void init_memory_info_node(void);
void init_system_load_node(void);

Netsnmp_Node_Handler handle_macAddress;
Netsnmp_Node_Handler handle_hardwareVersion;
Netsnmp_Node_Handler handle_softwareVersion;
Netsnmp_Node_Handler handle_sn;
Netsnmp_Node_Handler handle_compilingTime;
Netsnmp_Node_Handler handle_systemTime;
Netsnmp_Node_Handler handle_powerTime;

Netsnmp_Node_Handler handle_memory_used;
Netsnmp_Node_Handler handle_memory_buffers;
Netsnmp_Node_Handler handle_memory_cached;
Netsnmp_Node_Handler handle_memory_free;

Netsnmp_Node_Handler handle_system_cpu;
Netsnmp_Node_Handler handle_system_memory;

int get_customize_info(char *in_data, char *out_data);
int get_compile_time(char *compiletime);
unsigned int get_system_uptime();
void get_system_time(char *systemtime);

#endif