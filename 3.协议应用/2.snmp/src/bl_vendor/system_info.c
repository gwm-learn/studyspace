#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>

#include <string.h>
#include <time.h>
#include "ipc_client.h"
#include "util.h"
#include "system_info.h"
#include "packet_table.h"
#include "mac_frame_table.h"

void init_system_info(void){}

void init_system_info_node(void)
{
    init_global_info_node();
    init_memory_info_node();
    init_system_load_node();
    init_packet_table_node();
    init_mac_frame_table_node();
}

void init_global_info_node(void)
{
    const oid macAddress_oid[]      = { 1,3,6,1,4,1,36000,1,1,1,1 };
    const oid hardwareVersion_oid[] = { 1,3,6,1,4,1,36000,1,1,1,2 };
    const oid softwareVersion_oid[] = { 1,3,6,1,4,1,36000,1,1,1,3 };
    const oid sn_oid[]              = { 1,3,6,1,4,1,36000,1,1,1,4 };
    const oid compilingTime_oid[]   = { 1,3,6,1,4,1,36000,1,1,1,5 };
    const oid systemTime_oid[]      = { 1,3,6,1,4,1,36000,1,1,1,6 };
    const oid powerTime_oid[]       = { 1,3,6,1,4,1,36000,1,1,1,7 };

    snmp_log(LOG_DEBUG, "init_global_info_node init\n");

    netsnmp_register_scalar(netsnmp_create_handler_registration("macAddress", handle_macAddress, macAddress_oid, OID_LENGTH(macAddress_oid), HANDLER_CAN_RONLY));
    netsnmp_register_scalar(netsnmp_create_handler_registration("hardwareVersion", handle_hardwareVersion, hardwareVersion_oid, OID_LENGTH(hardwareVersion_oid), HANDLER_CAN_RONLY));
    netsnmp_register_scalar(netsnmp_create_handler_registration("softwareVersion", handle_softwareVersion, softwareVersion_oid, OID_LENGTH(softwareVersion_oid), HANDLER_CAN_RONLY));
    netsnmp_register_scalar(netsnmp_create_handler_registration("sn", handle_sn, sn_oid, OID_LENGTH(sn_oid), HANDLER_CAN_RONLY));
    netsnmp_register_scalar(netsnmp_create_handler_registration("compilingTime", handle_compilingTime, compilingTime_oid, OID_LENGTH(compilingTime_oid), HANDLER_CAN_RONLY));
    netsnmp_register_scalar(netsnmp_create_handler_registration("systemTime", handle_systemTime, systemTime_oid, OID_LENGTH(systemTime_oid), HANDLER_CAN_RONLY));
    netsnmp_register_scalar(netsnmp_create_handler_registration("powerTime", handle_powerTime, powerTime_oid, OID_LENGTH(powerTime_oid), HANDLER_CAN_RONLY));
}

void init_memory_info_node(void)
{
    const oid memory_used_oid[]    = { 1,3,6,1,4,1,36000,1,1,2,1 };
    const oid memory_buffers_oid[] = { 1,3,6,1,4,1,36000,1,1,2,2 };
    const oid memory_cached_oid[]  = { 1,3,6,1,4,1,36000,1,1,2,3 };
    const oid memory_free_oid[]    = { 1,3,6,1,4,1,36000,1,1,2,4 };

    snmp_log(LOG_DEBUG, "init_memory_info_node init\n");

    netsnmp_register_scalar(netsnmp_create_handler_registration("memory_used", handle_memory_used, memory_used_oid, OID_LENGTH(memory_used_oid), HANDLER_CAN_RONLY));
    netsnmp_register_scalar(netsnmp_create_handler_registration("memory_buffers", handle_memory_buffers, memory_buffers_oid, OID_LENGTH(memory_buffers_oid), HANDLER_CAN_RONLY));
    netsnmp_register_scalar(netsnmp_create_handler_registration("memory_cached", handle_memory_cached, memory_cached_oid, OID_LENGTH(memory_cached_oid), HANDLER_CAN_RONLY));
    netsnmp_register_scalar(netsnmp_create_handler_registration("memory_free", handle_memory_free, memory_free_oid, OID_LENGTH(memory_free_oid), HANDLER_CAN_RONLY));
}

void init_system_load_node(void)
{
    const oid system_cpu_oid[]    = { 1,3,6,1,4,1,36000,1,1,3,1 };
    const oid system_memory_oid[] = { 1,3,6,1,4,1,36000,1,1,3,2 };

    snmp_log(LOG_DEBUG, "init_system_load_node init\n");

    netsnmp_register_scalar(netsnmp_create_handler_registration("system_cpu", handle_system_cpu, system_cpu_oid, OID_LENGTH(system_cpu_oid), HANDLER_CAN_RONLY));
    netsnmp_register_scalar(netsnmp_create_handler_registration("system_memory", handle_system_memory, system_memory_oid, OID_LENGTH(system_memory_oid), HANDLER_CAN_RONLY));
}

int handle_macAddress(netsnmp_mib_handler *handler, netsnmp_handler_registration *reginfo, netsnmp_agent_request_info *reqinfo, netsnmp_request_info *requests)
{
    char mac[16] = {0};

    cj_ctl_msg_t msg;
    cj_ctl_msg_t recv_msg;

    switch(reqinfo->mode) {
        case MODE_GET:
            snmp_log(LOG_DEBUG, "handle_macAddress MODE_GET\n");
            msg.msg_type  = CJ_MSG_TYPE_SNMP;
            msg.protorl   = CJ_CLIENT_SNMP;
            msg.sub_type.snmp_type = CJ_MSG_SNMP_SUB_TYPE_GET_MAC;
            if (snmp_get_msg(&msg, sizeof(msg), (char *)&recv_msg, sizeof(cj_ctl_msg_t)) < 0){
                snmp_log(LOG_DEBUG, "handle_macAddress snmp_get_msg fail\n");
                netsnmp_set_request_error(reqinfo, requests, SNMP_ERR_GENERR);
                break;
            }
            mac_array_str(recv_msg.msg.mac, mac);
            snmp_log(LOG_DEBUG, "handle_macAddress mac str:%s hex:%02x%02x%02x%02x%02x%02x\n", mac, \
            recv_msg.msg.mac[0],recv_msg.msg.mac[1],recv_msg.msg.mac[2],recv_msg.msg.mac[3],recv_msg.msg.mac[4],recv_msg.msg.mac[5]);
            snmp_set_var_typed_value( requests->requestvb, ASN_OCTET_STR, mac, strlen(mac));
            break;
        default:
            snmp_log(LOG_ERR, "unknown mode (%d) in handle_macAddress\n", reqinfo->mode );
            return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

int handle_hardwareVersion(netsnmp_mib_handler *handler, netsnmp_handler_registration *reginfo, netsnmp_agent_request_info *reqinfo, netsnmp_request_info *requests)
{
    char hwversion[64] = {0};

    switch(reqinfo->mode) {
        case MODE_GET:
            snmp_log(LOG_DEBUG, "handle_hardwareVersion MODE_GET\n");
            get_customize_info(HWVERSION_STRING, hwversion);
            snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR, hwversion, strlen(hwversion));
            break;
        default:
            snmp_log(LOG_ERR, "unknown mode (%d) in handle_hardwareVersion\n", reqinfo->mode );
            return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

int handle_softwareVersion(netsnmp_mib_handler *handler, netsnmp_handler_registration *reginfo, netsnmp_agent_request_info *reqinfo, netsnmp_request_info *requests)
{
    char softversion[32] = {0};

    switch(reqinfo->mode) {
        case MODE_GET:
            snmp_log(LOG_DEBUG, "handle_softwareVersion MODE_GET\n");
            get_customize_info(SOFTVERSION_STRING, softversion);
            snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR, softversion, strlen(softversion));
            break;
        default:
            snmp_log(LOG_ERR, "unknown mode (%d) in handle_softwareVersion\n", reqinfo->mode );
            return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

int handle_sn(netsnmp_mib_handler *handler, netsnmp_handler_registration *reginfo, netsnmp_agent_request_info *reqinfo, netsnmp_request_info *requests)
{
    char producesn[32]={0};

    switch(reqinfo->mode) {
        case MODE_GET:
            snmp_log(LOG_DEBUG, "handle_sn MODE_GET\n");
            get_customize_info(PRODUCE_SN_STRING, producesn);
            snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR, producesn, strlen(producesn));
            break;
        default:
            snmp_log(LOG_ERR, "unknown mode (%d) in handle_sn\n", reqinfo->mode );
            return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

int handle_compilingTime(netsnmp_mib_handler *handler, netsnmp_handler_registration *reginfo, netsnmp_agent_request_info *reqinfo, netsnmp_request_info *requests)
{
    char compiletime[64]={0};

    switch(reqinfo->mode) {
        case MODE_GET:
            snmp_log(LOG_DEBUG, "handle_compilingTime MODE_GET\n");
            get_compile_time(compiletime);
            snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR, compiletime, strlen(compiletime));
            break;
        default:
            snmp_log(LOG_ERR, "unknown mode (%d) in handle_compilingTime\n", reqinfo->mode );
            return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

int handle_systemTime(netsnmp_mib_handler *handler, netsnmp_handler_registration *reginfo, netsnmp_agent_request_info *reqinfo, netsnmp_request_info *requests)
{
    char systemtime[64]={0};

    switch(reqinfo->mode) {
        case MODE_GET:
            snmp_log(LOG_DEBUG, "handle_systemTime MODE_GET\n");
            get_system_time(systemtime);
            snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR, systemtime, strlen(systemtime));
            break;
        default:
            snmp_log(LOG_ERR, "unknown mode (%d) in handle_systemTime\n", reqinfo->mode );
            return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

int handle_powerTime(netsnmp_mib_handler *handler, netsnmp_handler_registration *reginfo, netsnmp_agent_request_info *reqinfo, netsnmp_request_info *requests)
{
    unsigned int power_time = 0;

    switch(reqinfo->mode) {
        case MODE_GET:
            snmp_log(LOG_DEBUG, "handle_powerTime MODE_GET\n");
            power_time = get_system_uptime();
            snmp_set_var_typed_value(requests->requestvb, ASN_INTEGER, &power_time, sizeof(power_time));
            break;
        default:
            snmp_log(LOG_ERR, "unknown mode (%d) in handle_powerTime\n", reqinfo->mode );
            return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

int handle_memory_used(netsnmp_mib_handler *handler, netsnmp_handler_registration *reginfo, netsnmp_agent_request_info *reqinfo, netsnmp_request_info *requests)
{
    unsigned int value = 0;

    switch(reqinfo->mode) {
        case MODE_GET:
            snmp_log(LOG_DEBUG, "handle_memory_used MODE_GET\n");
            cj_plat_semw_begin();
            value = (msg->mem_used)/(msg->mem_free + msg->mem_used)*100;
            cj_plat_semw_end();
            snmp_set_var_typed_value(requests->requestvb, ASN_INTEGER, &value, sizeof(value));
            break;
        default:
            snmp_log(LOG_ERR, "unknown mode (%d) in handle_memory_used\n", reqinfo->mode );
            return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

int handle_memory_buffers(netsnmp_mib_handler *handler, netsnmp_handler_registration *reginfo, netsnmp_agent_request_info *reqinfo, netsnmp_request_info *requests)
{
    unsigned int value = 0;

    switch(reqinfo->mode) {
        case MODE_GET:
            snmp_log(LOG_DEBUG, "handle_memory_buffers MODE_GET\n");
            cj_plat_semw_begin();
            value = (msg->mem_buffer)/(msg->mem_free + msg->mem_used)*100;
            cj_plat_semw_end();
            snmp_set_var_typed_value(requests->requestvb, ASN_INTEGER, &value, sizeof(value));
            break;
        default:
            snmp_log(LOG_ERR, "unknown mode (%d) in handle_memory_buffers\n", reqinfo->mode );
            return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

int handle_memory_cached(netsnmp_mib_handler *handler, netsnmp_handler_registration *reginfo, netsnmp_agent_request_info *reqinfo, netsnmp_request_info *requests)
{
    unsigned int value = 0;

    switch(reqinfo->mode) {
        case MODE_GET:
            snmp_log(LOG_DEBUG, "handle_memory_cached MODE_GET\n");
            cj_plat_semw_begin();
            value = (msg->mem_cached)/(msg->mem_free + msg->mem_used)*100;
            cj_plat_semw_end();
            snmp_set_var_typed_value(requests->requestvb, ASN_INTEGER, &value, sizeof(value));
            break;
        default:
            snmp_log(LOG_ERR, "unknown mode (%d) in handle_memory_cached\n", reqinfo->mode );
            return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

int handle_memory_free(netsnmp_mib_handler *handler, netsnmp_handler_registration *reginfo, netsnmp_agent_request_info *reqinfo, netsnmp_request_info *requests)
{
    unsigned int value = 0;

    switch(reqinfo->mode) {
        case MODE_GET:
            snmp_log(LOG_DEBUG, "handle_memory_free MODE_GET\n");
            cj_plat_semw_begin();
            value = (msg->mem_free)/(msg->mem_free + msg->mem_used)*100;
            cj_plat_semw_end();
            snmp_set_var_typed_value(requests->requestvb, ASN_INTEGER, &value, sizeof(value));
            break;
        default:
            snmp_log(LOG_ERR, "unknown mode (%d) in handle_memory_free\n", reqinfo->mode );
            return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

int handle_system_cpu(netsnmp_mib_handler *handler, netsnmp_handler_registration *reginfo, netsnmp_agent_request_info *reqinfo, netsnmp_request_info *requests)
{
    unsigned int value = 0;

    switch(reqinfo->mode) {
        case MODE_GET:
            snmp_log(LOG_DEBUG, "handle_system_cpu MODE_GET\n");
            cj_plat_semw_begin();
            value = msg->cpu_user;
            cj_plat_semw_end();
            snmp_set_var_typed_value(requests->requestvb, ASN_INTEGER, &value, sizeof(value));
            break;
        default:
            snmp_log(LOG_ERR, "unknown mode (%d) in handle_system_cpu\n", reqinfo->mode );
            return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

int handle_system_memory(netsnmp_mib_handler *handler, netsnmp_handler_registration *reginfo, netsnmp_agent_request_info *reqinfo, netsnmp_request_info *requests)
{
    unsigned int value = 0;

    switch(reqinfo->mode) {
        case MODE_GET:
            snmp_log(LOG_DEBUG, "handle_system_memory MODE_GET\n");
            cj_plat_semw_begin();
            value = (msg->mem_used)/(msg->mem_free + msg->mem_used)*100;
            cj_plat_semw_end();
            snmp_set_var_typed_value(requests->requestvb, ASN_INTEGER, &value, sizeof(value));
            break;
        default:
            snmp_log(LOG_ERR, "unknown mode (%d) in handle_system_memory\n", reqinfo->mode );
            return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

int get_customize_info(char *in_data, char *out_data)
{
    FILE *fd = NULL;
    char *str = NULL;
    char buffer[256] = {0};

    if (in_data == NULL || out_data == NULL)
        return -1;

    fd = fopen(CUSTOMIZE_INFO_FILE, "r");
    if (!fd)
        return -1;

    while (fgets(buffer, 256, fd))
    {
        str = strstr(buffer, in_data);
        if (str)
        {
            str +=strlen(in_data);
            strcpy(out_data, str);
            del_spec_in_str('\"', out_data, strlen(out_data));
            break;
        }else
        {
            continue;
        }
    }

    fclose(fd);

    return 0;
}

int get_compile_time(char *compiletime)
{
    FILE *fd = NULL;
    char *str = NULL;
    char buffer[256] = {0};

    fd = fopen(BUILDFILE,"r");
    if( !fd )
        return -1;

    while (fgets(buffer, 256, fd))
    {
        str = strstr(buffer, BUILDTIME_STR);
        if (str)
        {
            str +=strlen(BUILDTIME_STR);
            strcpy(compiletime,str);
            break;
        }else
        {
            continue;
        }
    }

    fclose(fd);
    return 0;
}

unsigned int get_system_uptime(char *system_uptime)
{
    struct timespec currTime = {0, 0};

    if (clock_gettime(CLOCK_MONOTONIC, &currTime) != 0) {
        snmp_log(LOG_ERR, "bad news, get time err!\n");
    }

    return (unsigned int)(currTime.tv_sec);
}

void get_system_time(char *system_time)
{
    time_t current_secs;
    struct tm * tm_time;

    time(&current_secs);
    tm_time = localtime(&current_secs);
    sprintf(system_time, "%d/%02d/%02d %02d:%02d:%02d",
            (tm_time->tm_year+ 1900),
            (tm_time->tm_mon+1),
            (tm_time->tm_mday),
            (tm_time->tm_hour),
            (tm_time->tm_min),(tm_time->tm_sec));
}