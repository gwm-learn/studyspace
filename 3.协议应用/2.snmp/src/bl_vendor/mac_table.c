#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>

#include <string.h>
#include "ipc_client.h"
#include "util.h"
#include "mac_table.h"

mac_table_entry *mac_table_head = NULL;

void init_mac_table(void){}

void init_mac_table_node(void)
{
    int rc;
    netsnmp_iterator_info *iinfo;
    netsnmp_handler_registration *reg;
    netsnmp_table_registration_info *table_info;

    const oid mac_table_oid[] = { 1,3,6,1,4,1,36000,2,1 };

    snmp_log(LOG_DEBUG, "init_mac_table_node init\n");


    table_info = SNMP_MALLOC_TYPEDEF(netsnmp_table_registration_info); 
    if (!table_info)
    {
        snmp_log(LOG_DEBUG, "malloc table_info fail\n");
        return;
    }
    netsnmp_table_helper_add_indexes(table_info, ASN_INTEGER, 0);
    table_info->min_column = COLUMN_MAC_ID;
    table_info->max_column = COLUMN_MAC_ROW_STATUS;

    iinfo = SNMP_MALLOC_TYPEDEF(netsnmp_iterator_info);
    if (!iinfo) {
        snmp_log(LOG_DEBUG, "malloc iinfo fail\n");
        return;
    }
    iinfo->get_first_data_point = mac_table_get_first_data_point;
    iinfo->get_next_data_point = mac_table_get_next_data_point;
    iinfo->table_reginfo = table_info;

    reg = netsnmp_create_handler_registration("mac_table", mac_table_handler, mac_table_oid, OID_LENGTH(mac_table_oid), HANDLER_CAN_RWRITE);
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
    rc = netsnmp_inject_handler_before(reg, netsnmp_get_cache_handler(MAC_TABLE_TIMEOUT, mac_table_load, mac_table_free, mac_table_oid, OID_LENGTH(mac_table_oid)), TABLE_ITERATOR_NAME);
    if (rc != SNMPERR_SUCCESS)
    {
        snmp_log(LOG_DEBUG, "inject handler before fail\n");
        return;
    }
}

mac_table_entry *mac_table_createEntry(long mac_id, long mac_vlan, long mac_type, char *mac_port, char *mac_address, int mac_row_status)
{
    cj_ctl_msg_t msg;
    mac_table_entry *entry;

    snmp_log(LOG_DEBUG, "mac_table_createEntry\n");

    entry = malloc_in_comm_list_tail((void **)(&mac_table_head), sizeof(mac_table_entry));
    if (!entry)
    {
        snmp_log(LOG_DEBUG, "mac_table_createEntry malloc entry fail\n");
        return NULL;
    }

    entry->mac_id = mac_id;
    entry->mac_vlan = mac_vlan;
    entry->mac_type = mac_type;
    if (strlen(mac_port) > 0)
        sprintf(entry->mac_port, "%s", mac_port);
    if (strlen(mac_address) > 0)
        sprintf(entry->mac_address, "%s", mac_address);
    entry->mac_row_status = mac_row_status;

    msg.msg_type  = CJ_MSG_TYPE_SNMP;
    msg.protorl   = CJ_CLIENT_SNMP;
    msg.sub_type.snmp_type = CJ_MSG_SNMP_SUB_TYPE_MAC_ENTRY_ADD;
    msg.msg.index = entry->mac_vlan;
    sprintf(msg.msg.name, "%s", mac_port);
    mac_str_array(mac_address, msg.msg.mac);
    if (snmp_set_msg(&msg, sizeof(msg)) < 0){
        snmp_log(LOG_DEBUG, "mac_table_createEntry snmp_set_msg fail\n");
    }
    return entry;
}

void mac_table_removeEntry(mac_table_entry *entry)
{
    cj_ctl_msg_t msg;

    snmp_log(LOG_DEBUG, "mac_table_removeEntry\n");

    if (!entry || !mac_table_head)
    {
        snmp_log(LOG_DEBUG, "mac_table_removeEntry entry null\n");
        return;
    }

    msg.msg_type  = CJ_MSG_TYPE_SNMP;
    msg.protorl   = CJ_CLIENT_SNMP;
    msg.sub_type.snmp_type = CJ_MSG_SNMP_SUB_TYPE_MAC_ENTRY_DEL;
    msg.msg.index = entry->mac_vlan;
    sprintf(msg.msg.name, "%s", entry->mac_port);
    mac_str_array(entry->mac_address, msg.msg.mac);
    if (snmp_set_msg(&msg, sizeof(msg)) < 0){
        snmp_log(LOG_DEBUG, "mac_table_removeEntry snmp_set_msg fail\n");
    }
    free_in_comm_list((void **)(&mac_table_head), entry);
}

int mac_table_load(netsnmp_cache * cache, void *vmagic)
{
    int i = 0;
    int ret = SNMPERR_SUCCESS;
    char mac_address[16] = {0};
    mac_table_entry *this = NULL;

    snmp_log(LOG_DEBUG, "mac_table_load\n");

    cj_plat_semw_begin();
    if (msg == NULL)
    {
        snmp_log(LOG_DEBUG, "mac_table_load msg is null\n");
        cj_plat_semw_end();
        return SNMPERR_GENERR;
    }

    snmp_log(LOG_DEBUG, "mac_table_load msg entry_count is %d\n", msg->entry_count);
    for ( i = 0; i < msg->entry_count; i++)
    {
        memset(mac_address, '\0', sizeof(mac_address));
        this = malloc_in_comm_list_tail((void **)(&mac_table_head), sizeof(mac_table_entry));
        if (!this)
        {
            snmp_log(LOG_DEBUG, "mac_table_load malloc this fail\n");
            ret = SNMPERR_GENERR;
            break;
        }
        this->mac_id = i;
        this->mac_vlan = msg->web_entry[i].vlan;
        this->mac_type = msg->web_entry[i].type;
        if (strlen(msg->web_entry[i].ifname) > 0)
            sprintf(this->mac_port, "%s", msg->web_entry[i].ifname);
        mac_array_str(msg->web_entry[i].mac, mac_address);
        if (strlen(mac_address) > 0)
            sprintf(this->mac_address, "%s", mac_address);
        this->mac_row_status = RS_ACTIVE;
    }
    cj_plat_semw_end();
    return ret;
}

void mac_table_free(netsnmp_cache * cache, void *vmagic)
{
    mac_table_entry *this;

    snmp_log(LOG_DEBUG, "mac_table_free\n");

    if (!mac_table_head)
    {
        snmp_log(LOG_DEBUG, "mac_table_free mac_table_head is null\n");
        return;
    }

    free_whole_comm_list((void **)(&mac_table_head));

    mac_table_head = NULL;
}

netsnmp_variable_list *mac_table_get_first_data_point(void **my_loop_context, void **my_data_context, netsnmp_variable_list * put_index_data, netsnmp_iterator_info *mydata)
{
    *my_loop_context = mac_table_head;
    return mac_table_get_next_data_point(my_loop_context, my_data_context, put_index_data, mydata);
}

netsnmp_variable_list * mac_table_get_next_data_point(void **my_loop_context, void **my_data_context, netsnmp_variable_list * put_index_data, netsnmp_iterator_info *mydata)
{
    mac_table_entry *entry = (mac_table_entry *) *my_loop_context;
    netsnmp_variable_list *idx = put_index_data;

    if (entry) {
        snmp_set_var_typed_integer(idx, ASN_INTEGER, entry->mac_id);
        idx = idx->next_variable;
        *my_data_context = (void *) entry;
        *my_loop_context = (void *) entry->next;
        return put_index_data;
    } else {
        return NULL;
    }
}

void mac_table_set_vlan(char *mac, int vlan)
{
    cj_ctl_msg_t msg;

    msg.msg_type  = CJ_MSG_TYPE_SNMP;
    msg.protorl   = CJ_CLIENT_SNMP;
    msg.sub_type.snmp_type = CJ_MSG_SNMP_SUB_TYPE_MAC_SET_VLAN;
    msg.msg.index = vlan;
    mac_str_array(mac, msg.msg.mac);
    if (snmp_set_msg(&msg, sizeof(msg)) < 0){
        snmp_log(LOG_DEBUG, "mac_table_createEntry snmp_set_msg fail\n");
    }
}

void mac_table_set_port(char *mac, char *port)
{
    cj_ctl_msg_t msg;

    msg.msg_type  = CJ_MSG_TYPE_SNMP;
    msg.protorl   = CJ_CLIENT_SNMP;
    msg.sub_type.snmp_type = CJ_MSG_SNMP_SUB_TYPE_MAC_SET_PORT;
    sprintf(msg.msg.name, port);
    mac_str_array(mac, msg.msg.mac);
    if (snmp_set_msg(&msg, sizeof(msg)) < 0){
        snmp_log(LOG_DEBUG, "mac_table_createEntry snmp_set_msg fail\n");
    }
}

void mac_table_set_mac(char *mac_old, char *mac_new)
{
    cj_ctl_msg_t msg;

    msg.msg_type  = CJ_MSG_TYPE_SNMP;
    msg.protorl   = CJ_CLIENT_SNMP;
    msg.sub_type.snmp_type = CJ_MSG_SNMP_SUB_TYPE_MAC_SET_MAC;
    mac_str_array(mac_old, msg.msg.mac);
    mac_str_array(mac_new, msg.msg.mac_mask);
    if (snmp_set_msg(&msg, sizeof(msg)) < 0){
        snmp_log(LOG_DEBUG, "mac_table_createEntry snmp_set_msg fail\n");
    }
}

int mac_table_handler(netsnmp_mib_handler *handler, netsnmp_handler_registration *reginfo, netsnmp_agent_request_info *reqinfo, netsnmp_request_info *requests)
{
    int ret;
    netsnmp_request_info *request;
    netsnmp_table_request_info *table_info;
    mac_table_entry *table_entry;
    mac_table_entry *table_row;

    switch (reqinfo->mode) {
        case MODE_GET:
            for (request = requests; request; request = request->next) {
                table_entry = (mac_table_entry *) netsnmp_extract_iterator_context(request);
                table_info = netsnmp_extract_table_info(request);

                switch (table_info->colnum) {
                    case COLUMN_MAC_ID:
                        if (!table_entry) {
                            netsnmp_set_request_error(reqinfo, request, SNMP_NOSUCHINSTANCE);
                            continue;
                        }
                        snmp_set_var_typed_value(request->requestvb, ASN_INTEGER, &(table_entry->mac_id), sizeof(table_entry->mac_id));
                        break;
                    case COLUMN_MAC_VLAN:
                        if (!table_entry) {
                            netsnmp_set_request_error(reqinfo, request, SNMP_NOSUCHINSTANCE);
                            continue;
                        }
                        snmp_set_var_typed_value(request->requestvb, ASN_INTEGER, &(table_entry->mac_vlan), sizeof(table_entry->mac_vlan));
                        break;
                    case COLUMN_MAC_TYPE:
                        if (!table_entry) {
                            netsnmp_set_request_error(reqinfo, request, SNMP_NOSUCHINSTANCE);
                            continue;
                        }
                        snmp_set_var_typed_value(request->requestvb, ASN_INTEGER, &(table_entry->mac_type), sizeof(table_entry->mac_type));
                        break;
                    case COLUMN_MAC_PORT:
                        if (!table_entry) {
                            netsnmp_set_request_error(reqinfo, request, SNMP_NOSUCHINSTANCE);
                            continue;
                        }
                        snmp_set_var_typed_value(request->requestvb, ASN_OCTET_STR, table_entry->mac_port, strlen(table_entry->mac_port));
                        break;
                    case COLUMN_MAC_ADDRTESS:
                        if (!table_entry) {
                            netsnmp_set_request_error(reqinfo, request, SNMP_NOSUCHINSTANCE);
                            continue;
                        }
                        snmp_set_var_typed_value(request->requestvb, ASN_OCTET_STR, table_entry->mac_address, strlen(table_entry->mac_address));
                        break;
                    case COLUMN_MAC_ROW_STATUS:
                        if (!table_entry) {
                            netsnmp_set_request_error(reqinfo, request, SNMP_NOSUCHINSTANCE);
                            continue;
                        }
                        snmp_set_var_typed_value(request->requestvb, ASN_INTEGER, &(table_entry->mac_row_status), sizeof(table_entry->mac_row_status));
                        break;
                    default:
                        netsnmp_set_request_error(reqinfo, request, SNMP_NOSUCHOBJECT);
                        break;
                }
            }
            break;
        case MODE_SET_RESERVE1:
            for (request = requests; request; request = request->next) {
                table_entry = (mac_table_entry *) netsnmp_extract_iterator_context(request);
                table_info = netsnmp_extract_table_info(request);
                switch (table_info->colnum) {
                    case COLUMN_MAC_VLAN:
                        ret = netsnmp_check_vb_type(request->requestvb, ASN_INTEGER);
                        if (ret != SNMP_ERR_NOERROR)
                            netsnmp_set_request_error(reqinfo, request, ret);
                        break;
                    case COLUMN_MAC_PORT:
                        ret = netsnmp_check_vb_type(request->requestvb, ASN_OCTET_STR);
                        if (ret != SNMP_ERR_NOERROR)
                            netsnmp_set_request_error(reqinfo, request, ret);
                        break;
                    case COLUMN_MAC_ADDRTESS:
                        ret = netsnmp_check_vb_type(request->requestvb, ASN_OCTET_STR);
                        if (ret != SNMP_ERR_NOERROR)
                            netsnmp_set_request_error(reqinfo, request, ret);
                        break;
                    case COLUMN_MAC_ROW_STATUS:
                        snmp_log(LOG_DEBUG, "mac_table_handler MODE_SET_RESERVE1 COLUMN_MAC_ROW_STATUS\n");
                        ret = netsnmp_check_vb_rowstatus(request->requestvb, (table_entry ? RS_ACTIVE : RS_NONEXISTENT));
                        if (ret != SNMP_ERR_NOERROR) {
                            netsnmp_set_request_error(reqinfo, request, ret);
                        }
                        break;
                    default:
                        netsnmp_set_request_error(reqinfo, request, SNMP_ERR_NOTWRITABLE);
                        break;
                }
            }
            break;
        case MODE_SET_RESERVE2:
            for (request = requests; request; request = request->next) {
                table_entry = (struct mac_table_entry *) netsnmp_extract_iterator_context(request);
                table_info = netsnmp_extract_table_info(request);

                switch (table_info->colnum) {
                    case COLUMN_MAC_VLAN:
                    case COLUMN_MAC_PORT:
                    case COLUMN_MAC_ADDRTESS:
                        break;
                    case COLUMN_MAC_ROW_STATUS:
                        snmp_log(LOG_DEBUG, "mac_table_handler MODE_SET_RESERVE2 COLUMN_MAC_ROW_STATUS\n");
                        switch (*request->requestvb->val.integer) {
                            case RS_CREATEANDGO:
                            case RS_CREATEANDWAIT:
                                table_row = mac_table_createEntry(msg->entry_count, 1, 1, "G1", "000000000000", *request->requestvb->val.integer);
                                if (table_row) {
                                    netsnmp_insert_iterator_context(request, table_row);
                                } else {
                                    netsnmp_set_request_error(reqinfo, request, SNMP_ERR_RESOURCEUNAVAILABLE);
                                }
                            break;
                            case RS_DESTROY:
                                if (table_entry) {
                                    mac_table_removeEntry(table_entry);
                                } else {
                                    netsnmp_set_request_error(reqinfo, request, SNMP_NOSUCHINSTANCE);
                                }
                            break;
                        }
                        break;
                    default:
                        netsnmp_set_request_error(reqinfo, request, SNMP_ERR_NOACCESS);
                        break;
                    }
            }
        break;
        case MODE_SET_ACTION:
            for (request = requests; request; request = request->next) {
                table_entry = (mac_table_entry *) netsnmp_extract_iterator_context(request);
                table_info = netsnmp_extract_table_info(request);

                switch (table_info->colnum) {
                    case COLUMN_MAC_VLAN:
                        mac_table_set_vlan(table_entry->mac_address, requests->requestvb->val.integer);
                        break;
                    case COLUMN_MAC_PORT:
                        mac_table_set_port(table_entry->mac_address, requests->requestvb->val.string);
                        break;
                    case COLUMN_MAC_ADDRTESS:
                        mac_table_set_mac(table_entry->mac_address, requests->requestvb->val.string);
                        break;
                    case COLUMN_MAC_ROW_STATUS:
                        snmp_log(LOG_DEBUG, "mac_table_handler MODE_SET_ACTION COLUMN_MAC_ROW_STATUS\n");
                        table_entry->mac_row_status = *request->requestvb->val.integer;
                        switch (*request->requestvb->val.integer) {
                            case RS_CREATEANDGO:
                            case RS_CREATEANDWAIT:
                            case RS_DESTROY:
                                break;
                            default:
                                netsnmp_set_request_error(reqinfo, request, SNMP_ERR_NOACCESS);
                                break;
                        }
                        break;
                    default:
                        netsnmp_set_request_error(reqinfo, request, SNMP_ERR_NOTWRITABLE);
                        break;
                }
            }
        break;
        case MODE_SET_COMMIT:
            for (request = requests; request; request = request->next) {
                table_entry = (mac_table_entry *) netsnmp_extract_iterator_context(request);
                table_info = netsnmp_extract_table_info(request);
                switch (table_info->colnum) {
                    case COLUMN_MAC_ROW_STATUS:
                        snmp_log(LOG_DEBUG, "mac_table_handler MODE_SET_COMMIT COLUMN_MAC_ROW_STATUS\n");
                        break;
                    default:
                        break;
                }
            }
        break;
        case MODE_SET_FREE:
        case MODE_SET_UNDO:
            for (request = requests; request; request = request->next) {
                table_entry = (struct mac_table_entry *) netsnmp_extract_iterator_context(request);
                table_info = netsnmp_extract_table_info(request);

                switch (table_info->colnum) {
                    case COLUMN_MAC_ROW_STATUS:
                        snmp_log(LOG_DEBUG, "mac_table_handler MODE_SET_FREE or MODE_SET_UNDO COLUMN_MAC_ROW_STATUS\n");
                        break;
                    }
            }
        break;
        default:
            snmp_log(LOG_ERR, "mac_table_handler unknown mode (%d)\n", reqinfo->mode );
            for (request = requests; request; request = request->next) {
                table_entry = (mac_table_entry *) netsnmp_extract_iterator_context(request);
                table_info = netsnmp_extract_table_info(request);

                switch (table_info->colnum) {
                    case COLUMN_MAC_ROW_STATUS:
                        snmp_log(LOG_ERR, "mac_table_handler unknown mode (%d) unknown clonum (%d) unknown value (%d)\n", reqinfo->mode, table_info->colnum, *request->requestvb->val.integer);
                        break;
                    default:
                        break;
                }
            }
        break;
    }
}
