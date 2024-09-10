#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>

#include <string.h>
#include "ipc_client.h"
#include "platform_h/cj_plat.h"
#include "util.h"
#include "packet_table.h"

basic_packet_table_entry *basic_packet_table_head = NULL;
detail_packet_table_entry *detail_packet_table_head = NULL;

void init_packet_table(void)
{

}

void init_packet_table_node(void)
{
    init_basic_packet_table_node();
    init_detail_packet_table_node();
}

void init_basic_packet_table_node(void)
{
    int rc;
    netsnmp_iterator_info *iinfo;
    netsnmp_handler_registration *reg;
    netsnmp_table_registration_info *table_info;

    const oid basic_packet_table_oid[] = { 1,3,6,1,4,1,36000,1,2,1 };

    snmp_log(LOG_DEBUG, "init_basic_packet_table_node init\n");


    table_info = SNMP_MALLOC_TYPEDEF(netsnmp_table_registration_info); 
    if (!table_info)
    {
        snmp_log(LOG_DEBUG, "malloc table_info fail\n");
        return;
    }
    netsnmp_table_helper_add_indexes(table_info, ASN_OCTET_STR, 0);
    table_info->min_column = COLUMN_BASIC_PACKET_PORT;
    table_info->max_column = COLUMN_BASIC_PACKET_TX_ERRORS;

    iinfo = SNMP_MALLOC_TYPEDEF(netsnmp_iterator_info);
    if (!iinfo) {
        snmp_log(LOG_DEBUG, "malloc iinfo fail\n");
        return;
    }
    iinfo->get_first_data_point = basic_packet_table_get_first_data_point;
    iinfo->get_next_data_point = basic_packet_table_get_next_data_point;
    iinfo->table_reginfo = table_info;

    reg = netsnmp_create_handler_registration("basic_packet_table", basic_packet_table_handler, basic_packet_table_oid, OID_LENGTH(basic_packet_table_oid), HANDLER_CAN_RONLY);
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
    rc = netsnmp_inject_handler_before(reg, netsnmp_get_cache_handler(PACKET_TABLE_TIMEOUT, basic_packet_table_load, basic_packet_table_free, basic_packet_table_oid, OID_LENGTH(basic_packet_table_oid)), TABLE_ITERATOR_NAME);
    if (rc != SNMPERR_SUCCESS)
    {
        snmp_log(LOG_DEBUG, "inject handler before fail\n");
        return;
    }
}

void init_detail_packet_table_node(void)
{
    int rc;
    netsnmp_iterator_info *iinfo;
    netsnmp_handler_registration *reg;
    netsnmp_table_registration_info *table_info;

    const oid detail_packet_table_oid[] = { 1,3,6,1,4,1,36000,1,2,2 };

    snmp_log(LOG_DEBUG, "init_detail_packet_table_node init\n");


    table_info = SNMP_MALLOC_TYPEDEF(netsnmp_table_registration_info); 
    if (!table_info)
    {
        snmp_log(LOG_DEBUG, "malloc table_info fail\n");
        return;
    }
    netsnmp_table_helper_add_indexes(table_info, ASN_OCTET_STR, 0);
    table_info->min_column = COLUMN_DETAIL_PACKET_PORT;
    table_info->max_column = COLUMN_DETAIL_PACKET_TP_PORT_DISCARD;

    iinfo = SNMP_MALLOC_TYPEDEF(netsnmp_iterator_info);
    if (!iinfo) {
        snmp_log(LOG_DEBUG, "malloc iinfo fail\n");
        return;
    }
    iinfo->get_first_data_point = detail_packet_table_get_first_data_point;
    iinfo->get_next_data_point = detail_packet_table_get_next_data_point;
    iinfo->table_reginfo = table_info;

    reg = netsnmp_create_handler_registration("detail_packet_table", detail_packet_table_handler, detail_packet_table_oid, OID_LENGTH(detail_packet_table_oid), HANDLER_CAN_RONLY);
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
    rc = netsnmp_inject_handler_before(reg, netsnmp_get_cache_handler(PACKET_TABLE_TIMEOUT, detail_packet_table_load, detail_packet_table_free, detail_packet_table_oid, OID_LENGTH(detail_packet_table_oid)), TABLE_ITERATOR_NAME);
    if (rc != SNMPERR_SUCCESS)
    {
        snmp_log(LOG_DEBUG, "inject handler before fail\n");
        return;
    }
}


basic_packet_table_entry *basic_packet_table_createEntry(
    char *port,
    unsigned long long rx_bytes,
    unsigned long long rx_packets,
    unsigned long long rx_dropped,
    unsigned long long rx_errors,
    unsigned long long tx_bytes,
    unsigned long long tx_packets,
    unsigned long long tx_dropped,
    unsigned long long tx_errors)
{
    basic_packet_table_entry *entry;

    snmp_log(LOG_DEBUG, "basic_packet_table_createEntry\n");

    entry = malloc_in_comm_list_tail((void **)(&basic_packet_table_head), sizeof(basic_packet_table_entry));
    if (!entry)
    {
        snmp_log(LOG_DEBUG, "basic_packet_table_createEntry malloc entry fail\n");
        return NULL;
    }

    sprintf(entry->port, "%s", port);
    entry->rx_bytes = rx_bytes;
    entry->rx_packets = rx_packets;
    entry->rx_dropped = rx_dropped;
    entry->rx_errors = rx_errors;
    entry->tx_bytes = tx_bytes;
    entry->tx_packets = tx_packets;
    entry->tx_dropped = tx_dropped;
    entry->tx_errors = tx_errors;
    return entry;
}

detail_packet_table_entry *detail_packet_table_createEntry(
    char *port,
    unsigned long long rx_bytes,
    unsigned long long rx_ucastpkts,
    unsigned long long rx_multicast,
    unsigned long long rx_broadcast,
    unsigned long long tx_ucastpkts,
    unsigned long long tx_multicast,
    unsigned long long tx_broadcast,
    unsigned long long delat_exceed_discard,
    unsigned long long mtu_exceed_discard,
    unsigned long long tp_port_discard)
{
    detail_packet_table_entry *entry;

    snmp_log(LOG_DEBUG, "detail_packet_table_createEntry\n");

    entry = malloc_in_comm_list_tail((void **)(&detail_packet_table_head), sizeof(detail_packet_table_entry));
    if (!entry)
    {
        snmp_log(LOG_DEBUG, "detail_packet_table_createEntry malloc entry fail\n");
        return NULL;
    }

    sprintf(entry->port, "%s", port);
    entry->rx_bytes = rx_bytes;
    entry->rx_ucastpkts = rx_ucastpkts;
    entry->rx_multicast = rx_multicast;
    entry->rx_broadcast = rx_broadcast;
    entry->tx_ucastpkts = tx_ucastpkts;
    entry->tx_multicast = tx_multicast;
    entry->tx_broadcast = tx_broadcast;
    entry->delat_exceed_discard = delat_exceed_discard;
    entry->mtu_exceed_discard = mtu_exceed_discard;
    entry->tp_port_discard = tp_port_discard;

    return entry;
}

void basic_packet_table_removeEntry(basic_packet_table_entry *entry)
{
    snmp_log(LOG_DEBUG, "basic_packet_table_removeEntry\n");

    if (!entry || !basic_packet_table_head)
    {
        snmp_log(LOG_DEBUG, "basic_packet_table_removeEntry entry null\n");
        return ;
    }

    free_in_comm_list((void **)(&basic_packet_table_head), entry);
}

void detail_packet_table_removeEntry(detail_packet_table_entry *entry)
{
    snmp_log(LOG_DEBUG, "detail_packet_table_removeEntry\n");

    if (!entry || !detail_packet_table_head)
    {
        snmp_log(LOG_DEBUG, "detail_packet_table_removeEntry entry null\n");
        return ;
    }

    free_in_comm_list((void **)(&detail_packet_table_head), entry);
}

int basic_packet_table_load(netsnmp_cache * cache, void *vmagic)
{
    int i = 0;
    int ret = SNMPERR_SUCCESS;
    basic_packet_table_entry *this = NULL;

    snmp_log(LOG_DEBUG, "basic_packet_table_load\n");

    cj_plat_sem_vlan_begin();
    if (vlan_msg == NULL)
    {
        snmp_log(LOG_DEBUG, "basic_packet_table_load vlan_msg is null\n");
        cj_plat_sem_vlan_end();
        return SNMPERR_GENERR;
    }

    for(i = 0; i < 26; i++) {
        this = malloc_in_comm_list_tail((void **)(&basic_packet_table_head), sizeof(basic_packet_table_entry));
        if (!this)
        {
            snmp_log(LOG_DEBUG, "basic_packet_table_load malloc this fail\n");
            ret = SNMPERR_GENERR;
            break;
        }

        sprintf(this->port, "%s", vlan_msg->ifp[i].name);
        this->rx_bytes = vlan_msg->ifp[i].statics.rxBytes;
        this->rx_packets = vlan_msg->ifp[i].statics.rxPackets;
        this->rx_dropped = vlan_msg->ifp[i].statics.rxDropped;
        this->rx_errors = vlan_msg->ifp[i].statics.rxErrors;
        this->tx_bytes = vlan_msg->ifp[i].statics.txBytes;
        this->tx_packets = vlan_msg->ifp[i].statics.txPackets;
        this->tx_dropped = vlan_msg->ifp[i].statics.txDropped;
        this->tx_errors = vlan_msg->ifp[i].statics.txErrors;
    }

    cj_plat_sem_vlan_end();
    return ret;
}

int detail_packet_table_load(netsnmp_cache * cache, void *vmagic)
{
    int i = 0;
    int ret = SNMPERR_SUCCESS;
    detail_packet_table_entry *this = NULL;

    snmp_log(LOG_DEBUG, "detail_packet_table_load\n");

    cj_plat_sem_vlan_begin();
    if (vlan_msg == NULL)
    {
        snmp_log(LOG_DEBUG, "detail_packet_table_load vlan_msg is null\n");
        cj_plat_sem_vlan_end();
        return SNMPERR_GENERR;
    }

    for(i = 0; i < 26; i++) {
        this = malloc_in_comm_list_tail((void **)(&detail_packet_table_head), sizeof(detail_packet_table_entry));
        if (!this)
        {
            snmp_log(LOG_DEBUG, "detail_packet_table_load malloc this fail\n");
            ret = SNMPERR_GENERR;
            break;
        }

        sprintf(this->port, "%s", vlan_msg->ifp[i].name);
        this->rx_bytes = vlan_msg->ifp[i].statics.rxBytes;
        this->rx_ucastpkts = vlan_msg->ifp[i].statics.rxUcastPkts;
        this->rx_multicast = vlan_msg->ifp[i].statics.rxMulticast;
        this->rx_broadcast = vlan_msg->ifp[i].statics.rxBroadcast;
        this->tx_ucastpkts = vlan_msg->ifp[i].statics.txUcastPkts;
        this->tx_multicast = vlan_msg->ifp[i].statics.txMulticast;
        this->tx_broadcast = vlan_msg->ifp[i].statics.txBroadcast;
        this->delat_exceed_discard = vlan_msg->ifp[i].statics.delayExceedDiscard;
        this->mtu_exceed_discard = vlan_msg->ifp[i].statics.mtuExceedDiscard;
        this->tp_port_discard = vlan_msg->ifp[i].statics.tpPortDiscard;
    }

    cj_plat_sem_vlan_end();
    return ret;
}

void basic_packet_table_free(netsnmp_cache * cache, void *vmagic)
{
    basic_packet_table_entry *this;

    snmp_log(LOG_DEBUG, "basic_packet_table_free\n");

    if (!basic_packet_table_head)
    {
        snmp_log(LOG_DEBUG, "basic_packet_table_free basic_packet_table_head is null\n");
        return ;
    }

    free_whole_comm_list((void **)(&basic_packet_table_head));

    basic_packet_table_head = NULL;
}

void detail_packet_table_free(netsnmp_cache * cache, void *vmagic)
{
    detail_packet_table_entry *this;

    snmp_log(LOG_DEBUG, "detail_packet_table_free\n");

    if (!detail_packet_table_head)
    {
        snmp_log(LOG_DEBUG, "detail_packet_table_free detail_packet_table_head is null\n");
        return ;
    }

    free_whole_comm_list((void **)(&detail_packet_table_head));

    detail_packet_table_head = NULL;
}


netsnmp_variable_list *basic_packet_table_get_first_data_point(void **my_loop_context, void **my_data_context, netsnmp_variable_list * put_index_data, netsnmp_iterator_info *mydata)
{
    *my_loop_context = basic_packet_table_head;
    return basic_packet_table_get_next_data_point(my_loop_context, my_data_context, put_index_data, mydata);
}

netsnmp_variable_list *detail_packet_table_get_first_data_point(void **my_loop_context, void **my_data_context, netsnmp_variable_list * put_index_data, netsnmp_iterator_info *mydata)
{
    *my_loop_context = detail_packet_table_head;
    return detail_packet_table_get_next_data_point(my_loop_context, my_data_context, put_index_data, mydata);
}

netsnmp_variable_list * basic_packet_table_get_next_data_point(void **my_loop_context, void **my_data_context, netsnmp_variable_list * put_index_data, netsnmp_iterator_info *mydata)
{
    basic_packet_table_entry *entry = (basic_packet_table_entry *) *my_loop_context;
    netsnmp_variable_list *idx = put_index_data;

    if (entry) {
        snmp_set_var_typed_value(idx, ASN_OCTET_STR, entry->port, strlen(entry->port));
        idx = idx->next_variable;
        *my_data_context = (void *) entry;
        *my_loop_context = (void *) entry->next;
        return put_index_data;
    } else {
        return NULL;
    }
}

netsnmp_variable_list * detail_packet_table_get_next_data_point(void **my_loop_context, void **my_data_context, netsnmp_variable_list * put_index_data, netsnmp_iterator_info *mydata)
{
    detail_packet_table_entry *entry = (detail_packet_table_entry *) *my_loop_context;
    netsnmp_variable_list *idx = put_index_data;

    if (entry) {
        snmp_set_var_typed_value(idx, ASN_OCTET_STR, entry->port, strlen(entry->port));
        idx = idx->next_variable;
        *my_data_context = (void *) entry;
        *my_loop_context = (void *) entry->next;
        return put_index_data;
    } else {
        return NULL;
    }
}

int basic_packet_table_handler(netsnmp_mib_handler *handler, netsnmp_handler_registration *reginfo, netsnmp_agent_request_info *reqinfo, netsnmp_request_info *requests)
{
    netsnmp_request_info *request;
    netsnmp_table_request_info *table_info;
    basic_packet_table_entry *table_entry;

    switch (reqinfo->mode) {
    case MODE_GET:
        for (request = requests; request; request = request->next) {
            table_entry = (basic_packet_table_entry *) netsnmp_extract_iterator_context(request);
            table_info = netsnmp_extract_table_info(request);

            switch (table_info->colnum) {
                case COLUMN_BASIC_PACKET_PORT:
                    if (!table_entry) {
                        netsnmp_set_request_error(reqinfo, request, SNMP_NOSUCHINSTANCE);
                        continue;
                    }
                    snmp_set_var_typed_value(request->requestvb, ASN_OCTET_STR, table_entry->port, strlen(table_entry->port));
                    break;
                case COLUMN_BASIC_PACKET_RX_BYTES:
                    if (!table_entry) {
                        netsnmp_set_request_error(reqinfo, request, SNMP_NOSUCHINSTANCE);
                        continue;
                    }
                    snmp_set_var_typed_value(request->requestvb, ASN_COUNTER64, &(table_entry->rx_bytes), sizeof(table_entry->rx_bytes));
                    break;
                case COLUMN_BASIC_PACKET_RX_PACKETS:
                    if (!table_entry) {
                        netsnmp_set_request_error(reqinfo, request, SNMP_NOSUCHINSTANCE);
                        continue;
                    }
                    snmp_set_var_typed_value(request->requestvb, ASN_COUNTER64, &(table_entry->rx_packets), sizeof(table_entry->rx_packets));
                    break;
                case COLUMN_BASIC_PACKET_RX_DROPPED:
                    if (!table_entry) {
                        netsnmp_set_request_error(reqinfo, request, SNMP_NOSUCHINSTANCE);
                        continue;
                    }
                    snmp_set_var_typed_value(request->requestvb, ASN_COUNTER64, &(table_entry->rx_dropped), sizeof(table_entry->rx_dropped));
                    break;
                case COLUMN_BASIC_PACKET_RX_ERRORS:
                    if (!table_entry) {
                        netsnmp_set_request_error(reqinfo, request, SNMP_NOSUCHINSTANCE);
                        continue;
                    }
                    snmp_set_var_typed_value(request->requestvb, ASN_COUNTER64, &(table_entry->rx_errors), sizeof(table_entry->rx_errors));
                    break;
                case COLUMN_BASIC_PACKET_TX_BYTES:
                    if (!table_entry) {
                        netsnmp_set_request_error(reqinfo, request, SNMP_NOSUCHINSTANCE);
                        continue;
                    }
                    snmp_set_var_typed_value(request->requestvb, ASN_COUNTER64, &(table_entry->tx_bytes), sizeof(table_entry->tx_bytes));
                    break;
                case COLUMN_BASIC_PACKET_TX_PACKETS:
                    if (!table_entry) {
                        netsnmp_set_request_error(reqinfo, request, SNMP_NOSUCHINSTANCE);
                        continue;
                    }
                    snmp_set_var_typed_value(request->requestvb, ASN_COUNTER64, &(table_entry->tx_packets), sizeof(table_entry->tx_packets));
                    break;
                case COLUMN_BASIC_PACKET_TX_DROPPED:
                    if (!table_entry) {
                        netsnmp_set_request_error(reqinfo, request, SNMP_NOSUCHINSTANCE);
                        continue;
                    }
                    snmp_set_var_typed_value(request->requestvb, ASN_COUNTER64, &(table_entry->tx_dropped), sizeof(table_entry->tx_dropped));
                    break;
                case COLUMN_BASIC_PACKET_TX_ERRORS:
                    if (!table_entry) {
                        netsnmp_set_request_error(reqinfo, request, SNMP_NOSUCHINSTANCE);
                        continue;
                    }
                    snmp_set_var_typed_value(request->requestvb, ASN_COUNTER64, &(table_entry->tx_errors), sizeof(table_entry->tx_errors));
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

int detail_packet_table_handler(netsnmp_mib_handler *handler, netsnmp_handler_registration *reginfo, netsnmp_agent_request_info *reqinfo, netsnmp_request_info *requests)
{
    netsnmp_request_info *request;
    netsnmp_table_request_info *table_info;
    detail_packet_table_entry *table_entry;

    switch (reqinfo->mode) {
    case MODE_GET:
        for (request = requests; request; request = request->next) {
            table_entry = (detail_packet_table_entry *) netsnmp_extract_iterator_context(request);
            table_info = netsnmp_extract_table_info(request);

            switch (table_info->colnum) {
                case COLUMN_DETAIL_PACKET_PORT:
                    if (!table_entry) {
                        netsnmp_set_request_error(reqinfo, request, SNMP_NOSUCHINSTANCE);
                        continue;
                    }
                    snmp_set_var_typed_value(request->requestvb, ASN_OCTET_STR, table_entry->port, strlen(table_entry->port));
                    break;
                case COLUMN_DETAIL_PACKET_RX_BYTES:
                    if (!table_entry) {
                        netsnmp_set_request_error(reqinfo, request, SNMP_NOSUCHINSTANCE);
                        continue;
                    }
                    snmp_set_var_typed_value(request->requestvb, ASN_COUNTER64, &(table_entry->rx_bytes), sizeof(table_entry->rx_bytes));
                    break;
                case COLUMN_DETAIL_PACKET_RX_UCASTPKTS:
                    if (!table_entry) {
                        netsnmp_set_request_error(reqinfo, request, SNMP_NOSUCHINSTANCE);
                        continue;
                    }
                    snmp_set_var_typed_value(request->requestvb, ASN_COUNTER64, &(table_entry->rx_ucastpkts), sizeof(table_entry->rx_ucastpkts));
                    break;
                case COLUMN_DETAIL_PACKET_RX_MULTICAST:
                    if (!table_entry) {
                        netsnmp_set_request_error(reqinfo, request, SNMP_NOSUCHINSTANCE);
                        continue;
                    }
                    snmp_set_var_typed_value(request->requestvb, ASN_COUNTER64, &(table_entry->rx_multicast), sizeof(table_entry->rx_multicast));
                    break;
                case COLUMN_DETAIL_PACKET_RX_BROADCAST:
                    if (!table_entry) {
                        netsnmp_set_request_error(reqinfo, request, SNMP_NOSUCHINSTANCE);
                        continue;
                    }
                    snmp_set_var_typed_value(request->requestvb, ASN_COUNTER64, &(table_entry->rx_broadcast), sizeof(table_entry->rx_broadcast));
                    break;
                case COLUMN_DETAIL_PACKET_TX_UCASTPKTS:
                    if (!table_entry) {
                        netsnmp_set_request_error(reqinfo, request, SNMP_NOSUCHINSTANCE);
                        continue;
                    }
                    snmp_set_var_typed_value(request->requestvb, ASN_COUNTER64, &(table_entry->tx_ucastpkts), sizeof(table_entry->tx_ucastpkts));
                    break;
                case COLUMN_DETAIL_PACKET_TX_MULTICAST:
                    if (!table_entry) {
                        netsnmp_set_request_error(reqinfo, request, SNMP_NOSUCHINSTANCE);
                        continue;
                    }
                    snmp_set_var_typed_value(request->requestvb, ASN_COUNTER64, &(table_entry->tx_multicast), sizeof(table_entry->tx_multicast));
                    break;
                case COLUMN_DETAIL_PACKET_TX_BROADCAST:
                    if (!table_entry) {
                        netsnmp_set_request_error(reqinfo, request, SNMP_NOSUCHINSTANCE);
                        continue;
                    }
                    snmp_set_var_typed_value(request->requestvb, ASN_COUNTER64, &(table_entry->tx_broadcast), sizeof(table_entry->tx_broadcast));
                    break;
                case COLUMN_DETAIL_PACKET_DELAY_EXCEED_DISCARD:
                    if (!table_entry) {
                        netsnmp_set_request_error(reqinfo, request, SNMP_NOSUCHINSTANCE);
                        continue;
                    }
                    snmp_set_var_typed_value(request->requestvb, ASN_COUNTER64, &(table_entry->delat_exceed_discard), sizeof(table_entry->delat_exceed_discard));
                    break;
                case COLUMN_DETAIL_PACKET_MTU_EXCEED_DISCARD:
                    if (!table_entry) {
                        netsnmp_set_request_error(reqinfo, request, SNMP_NOSUCHINSTANCE);
                        continue;
                    }
                    snmp_set_var_typed_value(request->requestvb, ASN_COUNTER64, &(table_entry->mtu_exceed_discard), sizeof(table_entry->mtu_exceed_discard));
                    break;
                case COLUMN_DETAIL_PACKET_TP_PORT_DISCARD:
                    if (!table_entry) {
                        netsnmp_set_request_error(reqinfo, request, SNMP_NOSUCHINSTANCE);
                        continue;
                    }
                    snmp_set_var_typed_value(request->requestvb, ASN_COUNTER64, &(table_entry->tp_port_discard), sizeof(table_entry->tp_port_discard));
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
