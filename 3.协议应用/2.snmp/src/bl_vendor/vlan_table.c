#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>

#include <string.h>
#include "ipc_client.h"
#include "platform_h/cj_plat.h"
#include "util.h"
#include "vlan_table.h"

vlan_state_table_entry *vlan_state_table_head = NULL;
vlan_config_table_entry *vlan_config_table_head = NULL;

void init_vlan_table(void){}

void init_vlan_table_node(void)
{
    init_vlan_state_table_node();
    init_vlan_config_table_node();
}

void init_vlan_state_table_node(void)
{
    int rc;
    netsnmp_iterator_info *iinfo;
    netsnmp_handler_registration *reg;
    netsnmp_table_registration_info *table_info;

    const oid vlan_state_table_oid[] = { 1,3,6,1,4,1,36000,3,1 };

    snmp_log(LOG_DEBUG, "init_vlan_state_table_node init\n");


    table_info = SNMP_MALLOC_TYPEDEF(netsnmp_table_registration_info); 
    if (!table_info)
    {
        snmp_log(LOG_DEBUG, "malloc table_info fail\n");
        return;
    }
    netsnmp_table_helper_add_indexes(table_info, ASN_INTEGER, 0);
    table_info->min_column = COLUMN_VLAN_STATE_ID;
    table_info->max_column = COLUMN_VLAN_STATE_UNTAG_PORT;

    iinfo = SNMP_MALLOC_TYPEDEF(netsnmp_iterator_info);
    if (!iinfo) {
        snmp_log(LOG_DEBUG, "malloc iinfo fail\n");
        return;
    }
    iinfo->get_first_data_point = vlan_state_table_get_first_data_point;
    iinfo->get_next_data_point = vlan_state_table_get_next_data_point;
    iinfo->table_reginfo = table_info;

    reg = netsnmp_create_handler_registration("vlan_state_table", vlan_state_table_handler, vlan_state_table_oid, OID_LENGTH(vlan_state_table_oid), HANDLER_CAN_RONLY);
    if (!reg) {
        snmp_log(LOG_DEBUG, "create handler registration fail\n");
        return;
    }
    rc = netsnmp_register_table_iterator2(reg, iinfo);
    if (rc != SNMPERR_SUCCESS)
    {
        snmp_log(LOG_DEBUG, "register table iterator fail\n");
        return;
    }
    rc = netsnmp_inject_handler_before(reg, netsnmp_get_cache_handler(VLAN_TABLE_TIMEOUT, vlan_state_table_load, vlan_state_table_free, vlan_state_table_oid, OID_LENGTH(vlan_state_table_oid)), TABLE_ITERATOR_NAME);
    if (rc != SNMPERR_SUCCESS)
    {
        snmp_log(LOG_DEBUG, "inject handler before fail\n");
        return;
    }
}

void init_vlan_config_table_node(void)
{
    int rc;
    netsnmp_iterator_info *iinfo;
    netsnmp_handler_registration *reg;
    netsnmp_table_registration_info *table_info;

    const oid vlan_config_table_oid[] = { 1,3,6,1,4,1,36000,3,2 };

    snmp_log(LOG_DEBUG, "init_vlan_config_table_node init\n");


    table_info = SNMP_MALLOC_TYPEDEF(netsnmp_table_registration_info); 
    if (!table_info)
    {
        snmp_log(LOG_DEBUG, "malloc table_info fail\n");
        return;
    }
    netsnmp_table_helper_add_indexes(table_info, ASN_INTEGER, 0);
    table_info->min_column = COLUMN_VLAN_CONFIG_ID;
    table_info->max_column = COLUMN_VLAN_CONFIG_UNTAG;

    iinfo = SNMP_MALLOC_TYPEDEF(netsnmp_iterator_info);
    if (!iinfo) {
        snmp_log(LOG_DEBUG, "malloc iinfo fail\n");
        return;
    }
    iinfo->get_first_data_point = vlan_config_table_get_first_data_point;
    iinfo->get_next_data_point = vlan_config_table_get_next_data_point;
    iinfo->table_reginfo = table_info;

    reg = netsnmp_create_handler_registration("vlan_config_table", vlan_config_table_handler, vlan_config_table_oid, OID_LENGTH(vlan_config_table_oid), HANDLER_CAN_RONLY);
    if (!reg) {
        snmp_log(LOG_DEBUG, "create handler registration fail\n");
        return;
    }
    rc = netsnmp_register_table_iterator2(reg, iinfo);
    if (rc != SNMPERR_SUCCESS)
    {
        snmp_log(LOG_DEBUG, "register table iterator fail\n");
        return;
    }
    rc = netsnmp_inject_handler_before(reg, netsnmp_get_cache_handler(VLAN_TABLE_TIMEOUT, vlan_config_table_load, vlan_config_table_free, vlan_config_table_oid, OID_LENGTH(vlan_config_table_oid)), TABLE_ITERATOR_NAME);
    if (rc != SNMPERR_SUCCESS)
    {
        snmp_log(LOG_DEBUG, "inject handler before fail\n");
        return;
    }
}

vlan_state_table_entry *vlan_state_table_createEntry(long vlan_id, char *vlan_tag_port, char *vlan_untag_port)
{
    vlan_state_table_entry *entry;

    snmp_log(LOG_DEBUG, "vlan_state_table_createEntry\n");

    entry = malloc_in_comm_list_tail((void **)(&vlan_state_table_head), sizeof(vlan_state_table_entry));
    if (!entry)
    {
        snmp_log(LOG_DEBUG, "vlan_state_table_createEntry malloc entry fail\n");
        return NULL;
    }

    entry->vlan_id = vlan_id;
    if (strlen(vlan_tag_port) > 0)
        sprintf(entry->vlan_tag_port, "%s", vlan_tag_port);
    if (strlen(vlan_untag_port) > 0)
        sprintf(entry->vlan_untag_port, "%s", vlan_untag_port);
    return entry;
}

vlan_config_table_entry *vlan_config_table_createEntry(long vlan_id, long vlan_mode, char *vlan_port, char *vlan_tag, char *vlan_untag)
{
    vlan_config_table_entry *entry;

    snmp_log(LOG_DEBUG, "vlan_config_table_createEntry\n");

    entry = malloc_in_comm_list_tail((void **)(&vlan_config_table_head), sizeof(vlan_config_table_entry));
    if (!entry)
    {
        snmp_log(LOG_DEBUG, "vlan_config_table_createEntry malloc entry fail\n");
        return NULL;
    }

    entry->vlan_id = vlan_id;
    entry->vlan_mode = vlan_mode;
    if (strlen(vlan_port) > 0)
        sprintf(entry->vlan_port, "%s", vlan_port);
    if (strlen(vlan_tag) > 0)
        sprintf(entry->vlan_tag, "%s", vlan_tag);
    if (strlen(vlan_untag) > 0)
        sprintf(entry->vlan_untag, "%s", vlan_untag);
    return entry;
}

void vlan_state_table_removeEntry(vlan_state_table_entry *entry)
{
    snmp_log(LOG_DEBUG, "vlan_state_table_removeEntry\n");

    if (!entry || !vlan_state_table_head)
    {
        snmp_log(LOG_DEBUG, "vlan_state_table_removeEntry entry null\n");
        return;
    }

    free_in_comm_list((void **)(&vlan_state_table_head), entry);
}

void vlan_config_table_removeEntry(vlan_config_table_entry *entry)
{
    snmp_log(LOG_DEBUG, "vlan_config_table_removeEntry\n");

    if (!entry || !vlan_config_table_head)
    {
        snmp_log(LOG_DEBUG, "vlan_config_table_removeEntry entry null\n");
        return;
    }

    free_in_comm_list((void **)(&vlan_config_table_head), entry);
}

int vlan_state_table_load(netsnmp_cache * cache, void *vmagic)
{
    int i = 0;
    int port_num = 0;
    char port[128] = {0};
    char single_port[8] = {0};
    int ret = SNMPERR_SUCCESS;
    vlan_state_table_entry *this = NULL;

    snmp_log(LOG_DEBUG, "vlan_state_table_load\n");

    cj_plat_sem_vlan_begin();
    if (vlan_msg == NULL)
    {
        snmp_log(LOG_DEBUG, "vlan_state_table_load vlan_msg is null\n");
        cj_plat_sem_vlan_end();
        return SNMPERR_GENERR;
    }

    for ( i = 0; i < VLAN_MSG_MAXS; i++)
    {
        if(vlan_msg->entry[i].vlanid == 0)
            continue;
        this = malloc_in_comm_list_tail((void **)(&vlan_state_table_head), sizeof(vlan_state_table_entry));
        if (!this)
        {
            snmp_log(LOG_DEBUG, "vlan_state_table_load malloc this fail\n");
            ret = SNMPERR_GENERR;
            break;
        }
        this->vlan_id = vlan_msg->entry[i].vlanid;

        port_num = 0;
        memset(port, '\0', sizeof(port));
        memset(single_port, '\0', sizeof(single_port));
        CJ_PORT_MASK_SCAN(vlan_msg->entry[i].tag_port_mask, port_num)
        {
            sprintf(single_port, "G%d", port_num+1);
            if (port_num == 0) {
                strcpy(port, single_port);
            } else {
                strcat(port, ",");
                strcat(port, single_port);
            }
        }
        if (strlen(port) > 0)
            sprintf(this->vlan_tag_port, "%s", port);

        port_num = 0;
        memset(port, '\0', sizeof(port));
        memset(single_port, '\0', sizeof(single_port));
        CJ_PORT_MASK_SCAN(vlan_msg->entry[i].untag_port_mask, port_num)
        {
            sprintf(single_port, "G%d", port_num+1);
            if (port_num == 0) {
                strcpy(port, single_port);
            } else {
                strcat(port, ",");
                strcat(port, single_port);
            }
        }
        if (strlen(port) > 0)
            sprintf(this->vlan_untag_port, "%s", port);
    }
    cj_plat_sem_vlan_end();
    return ret;
}

int vlan_config_table_load(netsnmp_cache * cache, void *vmagic)
{
    int i = 0;
    int ret = SNMPERR_SUCCESS;
    vlan_config_table_entry *this = NULL;

    snmp_log(LOG_DEBUG, "vlan_config_table_load\n");

    cj_plat_sem_vlan_begin();
    if (vlan_msg == NULL)
    {
        snmp_log(LOG_DEBUG, "vlan_config_table_load vlan_msg is null\n");
        cj_plat_sem_vlan_end();
        return SNMPERR_GENERR;
    }

    for ( i = 0; i < PORT_NUM; i++)
    {
        this = malloc_in_comm_list_tail((void **)(&vlan_config_table_head), sizeof(vlan_config_table_entry));
        if (!this)
        {
            snmp_log(LOG_DEBUG, "vlan_config_table_load malloc this fail\n");
            ret = SNMPERR_GENERR;
            break;
        }
        this->vlan_id = i;
        this->vlan_mode = vlan_msg->port_vlan[i].mode;
        if (strlen(vlan_msg->port_vlan[i].ifname) > 0)
            sprintf(this->vlan_port, "%s", vlan_msg->port_vlan[i].ifname);
        if (strlen(vlan_msg->port_vlan[i].tag_list) > 0)
            sprintf(this->vlan_tag, "%s", vlan_msg->port_vlan[i].tag_list);
        if (strlen(vlan_msg->port_vlan[i].untag_list) > 0)
            sprintf(this->vlan_untag, "%s", vlan_msg->port_vlan[i].untag_list);
    }
    cj_plat_sem_vlan_end();
    return ret;
}

void vlan_state_table_free(netsnmp_cache * cache, void *vmagic)
{
    vlan_state_table_entry *this;

    snmp_log(LOG_DEBUG, "vlan_state_table_free\n");

    if (!vlan_state_table_head)
    {
        snmp_log(LOG_DEBUG, "vlan_state_table_free vlan_state_table_head is null\n");
        return;
    }

    free_whole_comm_list((void **)(&vlan_state_table_head));

    vlan_state_table_head = NULL;
}

void vlan_config_table_free(netsnmp_cache * cache, void *vmagic)
{
    vlan_config_table_entry *this;

    snmp_log(LOG_DEBUG, "vlan_config_table_free\n");

    if (!vlan_config_table_head)
    {
        snmp_log(LOG_DEBUG, "vlan_config_table_free vlan_config_table_head is null\n");
        return;
    }

    free_whole_comm_list((void **)(&vlan_config_table_head));

    vlan_config_table_head = NULL;
}

netsnmp_variable_list *vlan_state_table_get_first_data_point(void **my_loop_context, void **my_data_context, netsnmp_variable_list * put_index_data, netsnmp_iterator_info *mydata)
{
    *my_loop_context = vlan_state_table_head;
    return vlan_state_table_get_next_data_point(my_loop_context, my_data_context, put_index_data, mydata);
}

netsnmp_variable_list *vlan_config_table_get_first_data_point(void **my_loop_context, void **my_data_context, netsnmp_variable_list * put_index_data, netsnmp_iterator_info *mydata)
{
    *my_loop_context = vlan_config_table_head;
    return vlan_config_table_get_next_data_point(my_loop_context, my_data_context, put_index_data, mydata);
}

netsnmp_variable_list * vlan_state_table_get_next_data_point(void **my_loop_context, void **my_data_context, netsnmp_variable_list * put_index_data, netsnmp_iterator_info *mydata)
{
    vlan_state_table_entry *entry = (vlan_state_table_entry *) *my_loop_context;
    netsnmp_variable_list *idx = put_index_data;

    if (entry) {
        snmp_set_var_typed_integer(idx, ASN_INTEGER, entry->vlan_id);
        idx = idx->next_variable;
        *my_data_context = (void *) entry;
        *my_loop_context = (void *) entry->next;
        return put_index_data;
    } else {
        return NULL;
    }
}

netsnmp_variable_list * vlan_config_table_get_next_data_point(void **my_loop_context, void **my_data_context, netsnmp_variable_list * put_index_data, netsnmp_iterator_info *mydata)
{
    vlan_config_table_entry *entry = (vlan_config_table_entry *) *my_loop_context;
    netsnmp_variable_list *idx = put_index_data;

    if (entry) {
        snmp_set_var_typed_integer(idx, ASN_INTEGER, entry->vlan_id);
        idx = idx->next_variable;
        *my_data_context = (void *) entry;
        *my_loop_context = (void *) entry->next;
        return put_index_data;
    } else {
        return NULL;
    }
}

int vlan_state_table_handler(netsnmp_mib_handler *handler, netsnmp_handler_registration *reginfo, netsnmp_agent_request_info *reqinfo, netsnmp_request_info *requests)
{
    netsnmp_request_info *request;
    netsnmp_table_request_info *table_info;
    vlan_state_table_entry *table_entry;

    switch (reqinfo->mode) {
    case MODE_GET:
        for (request = requests; request; request = request->next) {
            table_entry = (vlan_state_table_entry *) netsnmp_extract_iterator_context(request);
            table_info = netsnmp_extract_table_info(request);

            switch (table_info->colnum) {
                case COLUMN_VLAN_STATE_ID:
                    if (!table_entry) {
                        netsnmp_set_request_error(reqinfo, request, SNMP_NOSUCHINSTANCE);
                        continue;
                    }
                    snmp_set_var_typed_value(request->requestvb, ASN_INTEGER, &(table_entry->vlan_id), sizeof(table_entry->vlan_id));
                    break;
                case COLUMN_VLAN_STATE_TAG_PORT:
                    if (!table_entry) {
                        netsnmp_set_request_error(reqinfo, request, SNMP_NOSUCHINSTANCE);
                        continue;
                    }
                    snmp_set_var_typed_value(request->requestvb, ASN_OCTET_STR, table_entry->vlan_tag_port, strlen(table_entry->vlan_tag_port));
                    break;
                case COLUMN_VLAN_STATE_UNTAG_PORT:
                    if (!table_entry) {
                        netsnmp_set_request_error(reqinfo, request, SNMP_NOSUCHINSTANCE);
                        continue;
                    }
                    snmp_set_var_typed_value(request->requestvb, ASN_OCTET_STR, table_entry->vlan_untag_port, strlen(table_entry->vlan_untag_port));
                    break;
                default:
                    netsnmp_set_request_error(reqinfo, request, SNMP_NOSUCHOBJECT);
                    break;
            }
        }
        break;

    }
    return SNMP_ERR_NOERROR;
}

int vlan_config_table_handler(netsnmp_mib_handler *handler, netsnmp_handler_registration *reginfo, netsnmp_agent_request_info *reqinfo, netsnmp_request_info *requests)
{
    netsnmp_request_info *request;
    netsnmp_table_request_info *table_info;
    vlan_config_table_entry *table_entry;

    switch (reqinfo->mode) {
    case MODE_GET:
        for (request = requests; request; request = request->next) {
            table_entry = (vlan_config_table_entry *) netsnmp_extract_iterator_context(request);
            table_info = netsnmp_extract_table_info(request);

            switch (table_info->colnum) {
                case COLUMN_VLAN_CONFIG_ID:
                    if (!table_entry) {
                        netsnmp_set_request_error(reqinfo, request, SNMP_NOSUCHINSTANCE);
                        continue;
                    }
                    snmp_set_var_typed_value(request->requestvb, ASN_INTEGER, &(table_entry->vlan_id), sizeof(table_entry->vlan_id));
                    break;
                case COLUMN_VLAN_CONFIG_MODE:
                    if (!table_entry) {
                        netsnmp_set_request_error(reqinfo, request, SNMP_NOSUCHINSTANCE);
                        continue;
                    }
                    snmp_set_var_typed_value(request->requestvb, ASN_INTEGER, &(table_entry->vlan_mode), sizeof(table_entry->vlan_mode));
                    break;
                case COLUMN_VLAN_CONFIG_PORT:
                    if (!table_entry) {
                        netsnmp_set_request_error(reqinfo, request, SNMP_NOSUCHINSTANCE);
                        continue;
                    }
                    snmp_set_var_typed_value(request->requestvb, ASN_OCTET_STR, table_entry->vlan_port, strlen(table_entry->vlan_port));
                    break;
                case COLUMN_VLAN_CONFIG_TAG:
                    if (!table_entry) {
                        netsnmp_set_request_error(reqinfo, request, SNMP_NOSUCHINSTANCE);
                        continue;
                    }
                    snmp_set_var_typed_value(request->requestvb, ASN_OCTET_STR, table_entry->vlan_tag, strlen(table_entry->vlan_tag));
                    break;
                case COLUMN_VLAN_CONFIG_UNTAG:
                    if (!table_entry) {
                        netsnmp_set_request_error(reqinfo, request, SNMP_NOSUCHINSTANCE);
                        continue;
                    }
                    snmp_set_var_typed_value(request->requestvb, ASN_OCTET_STR, table_entry->vlan_untag, strlen(table_entry->vlan_untag));
                    break;
                default:
                    netsnmp_set_request_error(reqinfo, request, SNMP_NOSUCHOBJECT);
                    break;
            }
        }
        break;

    }
    return SNMP_ERR_NOERROR;
}