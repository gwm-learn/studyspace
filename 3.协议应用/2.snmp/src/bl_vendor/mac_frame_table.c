#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>

#include <string.h>
#include "ipc_client.h"
#include "platform_h/cj_plat.h"
#include "util.h"
#include "mac_frame_table.h"

mac_frame_lenth_table_entry *mac_frame_lenth_table_head = NULL;
mac_frame_error_table_entry *mac_frame_error_table_head = NULL;

void init_mac_frame_table(void)
{

}

void init_mac_frame_table_node(void)
{
    init_mac_frame_lenth_table_node();
    init_mac_frame_error_table_node();
}

void init_mac_frame_lenth_table_node(void)
{
    int rc;
    netsnmp_iterator_info *iinfo;
    netsnmp_handler_registration *reg;
    netsnmp_table_registration_info *table_info;

    const oid mac_frame_lenth_table_oid[] = { 1,3,6,1,4,1,36000,1,3,1 };

    snmp_log(LOG_DEBUG, "init_mac_frame_lenth_table_node init\n");


    table_info = SNMP_MALLOC_TYPEDEF(netsnmp_table_registration_info); 
    if (!table_info)
    {
        snmp_log(LOG_DEBUG, "malloc table_info fail\n");
        return;
    }
    netsnmp_table_helper_add_indexes(table_info, ASN_OCTET_STR, 0);
    table_info->min_column = COLUMN_MAC_FRAME_LENTH_PORT;
    table_info->max_column = COLUMN_MAC_FRAME_LENTH_JABBER_FRAME;

    iinfo = SNMP_MALLOC_TYPEDEF(netsnmp_iterator_info);
    if (!iinfo) {
        snmp_log(LOG_DEBUG, "malloc iinfo fail\n");
        return;
    }
    iinfo->get_first_data_point = mac_frame_lenth_table_get_first_data_point;
    iinfo->get_next_data_point = mac_frame_lenth_table_get_next_data_point;
    iinfo->table_reginfo = table_info;

    reg = netsnmp_create_handler_registration("mac_frame_lenth_table", mac_frame_lenth_table_handler, mac_frame_lenth_table_oid, OID_LENGTH(mac_frame_lenth_table_oid), HANDLER_CAN_RONLY);
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
    rc = netsnmp_inject_handler_before(reg, netsnmp_get_cache_handler(MAC_FRAME_TABLE_TIMEOUT, mac_frame_lenth_table_load, mac_frame_lenth_table_free, mac_frame_lenth_table_oid, OID_LENGTH(mac_frame_lenth_table_oid)), TABLE_ITERATOR_NAME);
    if (rc != SNMPERR_SUCCESS)
    {
        snmp_log(LOG_DEBUG, "inject handler before fail\n");
        return;
    }
}

void init_mac_frame_error_table_node(void)
{
    int rc;
    netsnmp_iterator_info *iinfo;
    netsnmp_handler_registration *reg;
    netsnmp_table_registration_info *table_info;

    const oid mac_frame_error_table_oid[] = { 1,3,6,1,4,1,36000,1,3,2 };

    snmp_log(LOG_DEBUG, "init_mac_frame_error_table_node init\n");


    table_info = SNMP_MALLOC_TYPEDEF(netsnmp_table_registration_info); 
    if (!table_info)
    {
        snmp_log(LOG_DEBUG, "malloc table_info fail\n");
        return;
    }
    netsnmp_table_helper_add_indexes(table_info, ASN_OCTET_STR, 0);
    table_info->min_column = COLUMN_MAC_FRAME_ERROR_PORT;
    table_info->max_column = COLUMN_MAC_FRAME_ERROR_OUT_PAUSE;

    iinfo = SNMP_MALLOC_TYPEDEF(netsnmp_iterator_info);
    if (!iinfo) {
        snmp_log(LOG_DEBUG, "malloc iinfo fail\n");
        return;
    }
    iinfo->get_first_data_point = mac_frame_error_table_get_first_data_point;
    iinfo->get_next_data_point = mac_frame_error_table_get_next_data_point;
    iinfo->table_reginfo = table_info;

    reg = netsnmp_create_handler_registration("mac_frame_error_table", mac_frame_error_table_handler, mac_frame_error_table_oid, OID_LENGTH(mac_frame_error_table_oid), HANDLER_CAN_RONLY);
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
    rc = netsnmp_inject_handler_before(reg, netsnmp_get_cache_handler(MAC_FRAME_TABLE_TIMEOUT, mac_frame_error_table_load, mac_frame_error_table_free, mac_frame_error_table_oid, OID_LENGTH(mac_frame_error_table_oid)), TABLE_ITERATOR_NAME);
    if (rc != SNMPERR_SUCCESS)
    {
        snmp_log(LOG_DEBUG, "inject handler before fail\n");
        return;
    }
}


mac_frame_lenth_table_entry *mac_frame_lenth_table_createEntry(
    char *port,
    unsigned long long undersizeFrame,
    unsigned long long BytesFrame64,
    unsigned long long BytesFrame64to127,
    unsigned long long BytesFrame128to255,
    unsigned long long BytesFrame256to511,
    unsigned long long BytesFrame512to1023,
    unsigned long long BytesFrame1024to1518,
    unsigned long long oversizeFrame,
    unsigned long long jabberFrame)
{
    mac_frame_lenth_table_entry *entry;

    snmp_log(LOG_DEBUG, "mac_frame_lenth_table_createEntry\n");

    entry = malloc_in_comm_list_tail((void **)(&mac_frame_lenth_table_head), sizeof(mac_frame_lenth_table_entry));
    if (!entry)
    {
        snmp_log(LOG_DEBUG, "mac_frame_lenth_table_createEntry malloc entry fail\n");
        return NULL;
    }

    sprintf(entry->port, "%s", port);
    entry->undersizeFrame = undersizeFrame;
    entry->BytesFrame64 = BytesFrame64;
    entry->BytesFrame64to127 = BytesFrame64to127;
    entry->BytesFrame128to255 = BytesFrame128to255;
    entry->BytesFrame256to511 = BytesFrame256to511;
    entry->BytesFrame512to1023 = BytesFrame512to1023;
    entry->BytesFrame1024to1518 = BytesFrame1024to1518;
    entry->oversizeFrame = oversizeFrame;
    entry->jabberFrame = jabberFrame;
    return entry;
}

mac_frame_error_table_entry *mac_frame_error_table_createEntry(
    char *port,
    unsigned long long alignmentError,
    unsigned long long crcError,
    unsigned long long excessiveCollisions,
    unsigned long long internalMacTrError,
    unsigned long long carrierSenseError,
    unsigned long long internalMacRxError,
    unsigned long long inPauseFrame,
    unsigned long long outPauseFrame)
{
    mac_frame_error_table_entry *entry;

    snmp_log(LOG_DEBUG, "mac_frame_error_table_createEntry\n");

    entry = malloc_in_comm_list_tail((void **)(&mac_frame_error_table_head), sizeof(mac_frame_error_table_entry));
    if (!entry)
    {
        snmp_log(LOG_DEBUG, "mac_frame_error_table_createEntry malloc entry fail\n");
        return NULL;
    }

    sprintf(entry->port, "%s", port);
    entry->alignmentError = alignmentError;
    entry->crcError = crcError;
    entry->excessiveCollisions = excessiveCollisions;
    entry->internalMacTrError = internalMacTrError;
    entry->carrierSenseError = carrierSenseError;
    entry->internalMacRxError = internalMacRxError;
    entry->inPauseFrame = inPauseFrame;
    entry->outPauseFrame = outPauseFrame;

    return entry;
}

void mac_frame_lenth_table_removeEntry(mac_frame_lenth_table_entry *entry)
{
    snmp_log(LOG_DEBUG, "mac_frame_lenth_table_removeEntry\n");

    if (!entry || !mac_frame_lenth_table_head)
    {
        snmp_log(LOG_DEBUG, "mac_frame_lenth_table_removeEntry entry null\n");
        return ;
    }

    free_in_comm_list((void **)(&mac_frame_lenth_table_head), entry);
}

void mac_frame_error_table_removeEntry(mac_frame_error_table_entry *entry)
{
    snmp_log(LOG_DEBUG, "mac_frame_error_table_removeEntry\n");

    if (!entry || !mac_frame_error_table_head)
    {
        snmp_log(LOG_DEBUG, "mac_frame_error_table_removeEntry entry null\n");
        return ;
    }

    free_in_comm_list((void **)(&mac_frame_error_table_head), entry);
}

int mac_frame_lenth_table_load(netsnmp_cache * cache, void *vmagic)
{
    int i = 0;
    int ret = SNMPERR_SUCCESS;
    mac_frame_lenth_table_entry *this = NULL;

    snmp_log(LOG_DEBUG, "mac_frame_lenth_table_load\n");

    cj_plat_sem_vlan_begin();
    if (vlan_msg == NULL)
    {
        snmp_log(LOG_DEBUG, "mac_frame_lenth_table_load vlan_msg is null\n");
        cj_plat_sem_vlan_end();
        return SNMPERR_GENERR;
    }

    for(i = 0; i < 26; i++) {
        this = malloc_in_comm_list_tail((void **)(&mac_frame_lenth_table_head), sizeof(mac_frame_lenth_table_entry));
        if (!this)
        {
            snmp_log(LOG_DEBUG, "mac_frame_lenth_table_load malloc this fail\n");
            ret = SNMPERR_GENERR;
            break;
        }

        sprintf(this->port, "%s", vlan_msg->ifp[i].name);
        this->undersizeFrame = vlan_msg->ifp[i].statics.undersizeFrame;
        this->BytesFrame64 = vlan_msg->ifp[i].statics.Bytes64Frame;
        this->BytesFrame64to127 = vlan_msg->ifp[i].statics.Bytes64_127Frame;
        this->BytesFrame128to255 = vlan_msg->ifp[i].statics.Bytes127_255Frame;
        this->BytesFrame256to511 = vlan_msg->ifp[i].statics.Bytes255_511Frame;
        this->BytesFrame512to1023 = vlan_msg->ifp[i].statics.Bytes512_1023Frame;
        this->BytesFrame1024to1518 = vlan_msg->ifp[i].statics.Bytes1024_1518Frame;
        this->oversizeFrame = vlan_msg->ifp[i].statics.oversizeFrame;
        this->jabberFrame = vlan_msg->ifp[i].statics.jabberFrame;
    }

    cj_plat_sem_vlan_end();
    return ret;
}

int mac_frame_error_table_load(netsnmp_cache * cache, void *vmagic)
{
    int i = 0;
    int ret = SNMPERR_SUCCESS;
    mac_frame_error_table_entry *this = NULL;

    snmp_log(LOG_DEBUG, "mac_frame_error_table_load\n");

    cj_plat_sem_vlan_begin();
    if (vlan_msg == NULL)
    {
        snmp_log(LOG_DEBUG, "mac_frame_error_table_load vlan_msg is null\n");
        cj_plat_sem_vlan_end();
        return SNMPERR_GENERR;
    }

    for(i = 0; i < 26; i++) {
        this = malloc_in_comm_list_tail((void **)(&mac_frame_error_table_head), sizeof(mac_frame_error_table_entry));
        if (!this)
        {
            snmp_log(LOG_DEBUG, "mac_frame_error_table_load malloc this fail\n");
            ret = SNMPERR_GENERR;
            break;
        }

        sprintf(this->port, "%s", vlan_msg->ifp[i].name);
        this->alignmentError = vlan_msg->ifp[i].statics.alignmentError;
        this->crcError = vlan_msg->ifp[i].statics.crcError;
        this->excessiveCollisions = vlan_msg->ifp[i].statics.excessiveCollisions;
        this->internalMacTrError = vlan_msg->ifp[i].statics.internalMacTrError;
        this->carrierSenseError = vlan_msg->ifp[i].statics.carrierSenseError;
        this->internalMacRxError = vlan_msg->ifp[i].statics.internalMacRxError;
        this->inPauseFrame = vlan_msg->ifp[i].statics.inPauseFrame;
        this->outPauseFrame = vlan_msg->ifp[i].statics.outPauseFrame;
    }

    cj_plat_sem_vlan_end();
    return ret;
}

void mac_frame_lenth_table_free(netsnmp_cache * cache, void *vmagic)
{
    mac_frame_lenth_table_entry *this;

    snmp_log(LOG_DEBUG, "mac_frame_lenth_table_free\n");

    if (!mac_frame_lenth_table_head)
    {
        snmp_log(LOG_DEBUG, "mac_frame_lenth_table_free mac_frame_lenth_table_head is null\n");
        return ;
    }

    free_whole_comm_list((void **)(&mac_frame_lenth_table_head));

    mac_frame_lenth_table_head = NULL;
}

void mac_frame_error_table_free(netsnmp_cache * cache, void *vmagic)
{
    mac_frame_error_table_entry *this;

    snmp_log(LOG_DEBUG, "mac_frame_error_table_free\n");

    if (!mac_frame_error_table_head)
    {
        snmp_log(LOG_DEBUG, "mac_frame_error_table_free mac_frame_error_table_head is null\n");
        return ;
    }

    free_whole_comm_list((void **)(&mac_frame_error_table_head));

    mac_frame_error_table_head = NULL;
}


netsnmp_variable_list *mac_frame_lenth_table_get_first_data_point(void **my_loop_context, void **my_data_context, netsnmp_variable_list * put_index_data, netsnmp_iterator_info *mydata)
{
    *my_loop_context = mac_frame_lenth_table_head;
    return mac_frame_lenth_table_get_next_data_point(my_loop_context, my_data_context, put_index_data, mydata);
}

netsnmp_variable_list *mac_frame_error_table_get_first_data_point(void **my_loop_context, void **my_data_context, netsnmp_variable_list * put_index_data, netsnmp_iterator_info *mydata)
{
    *my_loop_context = mac_frame_error_table_head;
    return mac_frame_error_table_get_next_data_point(my_loop_context, my_data_context, put_index_data, mydata);
}

netsnmp_variable_list * mac_frame_lenth_table_get_next_data_point(void **my_loop_context, void **my_data_context, netsnmp_variable_list * put_index_data, netsnmp_iterator_info *mydata)
{
    mac_frame_lenth_table_entry *entry = (mac_frame_lenth_table_entry *) *my_loop_context;
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

netsnmp_variable_list * mac_frame_error_table_get_next_data_point(void **my_loop_context, void **my_data_context, netsnmp_variable_list * put_index_data, netsnmp_iterator_info *mydata)
{
    mac_frame_error_table_entry *entry = (mac_frame_error_table_entry *) *my_loop_context;
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

int mac_frame_lenth_table_handler(netsnmp_mib_handler *handler, netsnmp_handler_registration *reginfo, netsnmp_agent_request_info *reqinfo, netsnmp_request_info *requests)
{
    netsnmp_request_info *request;
    netsnmp_table_request_info *table_info;
    mac_frame_lenth_table_entry *table_entry;

    switch (reqinfo->mode) {
    case MODE_GET:
        for (request = requests; request; request = request->next) {
            table_entry = (mac_frame_lenth_table_entry *) netsnmp_extract_iterator_context(request);
            table_info = netsnmp_extract_table_info(request);

            switch (table_info->colnum) {
                case COLUMN_MAC_FRAME_LENTH_PORT:
                    if (!table_entry) {
                        netsnmp_set_request_error(reqinfo, request, SNMP_NOSUCHINSTANCE);
                        continue;
                    }
                    snmp_set_var_typed_value(request->requestvb, ASN_OCTET_STR, table_entry->port, strlen(table_entry->port));
                    break;
                case COLUMN_MAC_FRAME_LENTH_UNDERSIZE_FRAME:
                    if (!table_entry) {
                        netsnmp_set_request_error(reqinfo, request, SNMP_NOSUCHINSTANCE);
                        continue;
                    }
                    snmp_set_var_typed_value(request->requestvb, ASN_COUNTER64, &(table_entry->undersizeFrame), sizeof(table_entry->undersizeFrame));
                    break;
                case COLUMN_MAC_FRAME_LENTH_BYTE_FRAME_64:
                    if (!table_entry) {
                        netsnmp_set_request_error(reqinfo, request, SNMP_NOSUCHINSTANCE);
                        continue;
                    }
                    snmp_set_var_typed_value(request->requestvb, ASN_COUNTER64, &(table_entry->BytesFrame64), sizeof(table_entry->BytesFrame64));
                    break;
                case COLUMN_MAC_FRAME_LENTH_BYTE_FRAME_64_127:
                    if (!table_entry) {
                        netsnmp_set_request_error(reqinfo, request, SNMP_NOSUCHINSTANCE);
                        continue;
                    }
                    snmp_set_var_typed_value(request->requestvb, ASN_COUNTER64, &(table_entry->BytesFrame64to127), sizeof(table_entry->BytesFrame64to127));
                    break;
                case COLUMN_MAC_FRAME_LENTH_BYTE_FRAME_128_255:
                    if (!table_entry) {
                        netsnmp_set_request_error(reqinfo, request, SNMP_NOSUCHINSTANCE);
                        continue;
                    }
                    snmp_set_var_typed_value(request->requestvb, ASN_COUNTER64, &(table_entry->BytesFrame128to255), sizeof(table_entry->BytesFrame128to255));
                    break;
                case COLUMN_MAC_FRAME_LENTH_BYTE_FRAME_256_511:
                    if (!table_entry) {
                        netsnmp_set_request_error(reqinfo, request, SNMP_NOSUCHINSTANCE);
                        continue;
                    }
                    snmp_set_var_typed_value(request->requestvb, ASN_COUNTER64, &(table_entry->BytesFrame256to511), sizeof(table_entry->BytesFrame256to511));
                    break;
                case COLUMN_MAC_FRAME_LENTH_BYTE_FRAME_512_1023:
                    if (!table_entry) {
                        netsnmp_set_request_error(reqinfo, request, SNMP_NOSUCHINSTANCE);
                        continue;
                    }
                    snmp_set_var_typed_value(request->requestvb, ASN_COUNTER64, &(table_entry->BytesFrame512to1023), sizeof(table_entry->BytesFrame512to1023));
                    break;
                case COLUMN_MAC_FRAME_LENTH_BYTE_FRAME_1024_1518:
                    if (!table_entry) {
                        netsnmp_set_request_error(reqinfo, request, SNMP_NOSUCHINSTANCE);
                        continue;
                    }
                    snmp_set_var_typed_value(request->requestvb, ASN_COUNTER64, &(table_entry->BytesFrame1024to1518), sizeof(table_entry->BytesFrame1024to1518));
                    break;
                case COLUMN_MAC_FRAME_LENTH_OVERSIZE_FRAME:
                    if (!table_entry) {
                        netsnmp_set_request_error(reqinfo, request, SNMP_NOSUCHINSTANCE);
                        continue;
                    }
                    snmp_set_var_typed_value(request->requestvb, ASN_COUNTER64, &(table_entry->oversizeFrame), sizeof(table_entry->oversizeFrame));
                    break;
                case COLUMN_MAC_FRAME_LENTH_JABBER_FRAME:
                    if (!table_entry) {
                        netsnmp_set_request_error(reqinfo, request, SNMP_NOSUCHINSTANCE);
                        continue;
                    }
                    snmp_set_var_typed_value(request->requestvb, ASN_COUNTER64, &(table_entry->jabberFrame), sizeof(table_entry->jabberFrame));
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

int mac_frame_error_table_handler(netsnmp_mib_handler *handler, netsnmp_handler_registration *reginfo, netsnmp_agent_request_info *reqinfo, netsnmp_request_info *requests)
{
    netsnmp_request_info *request;
    netsnmp_table_request_info *table_info;
    mac_frame_error_table_entry *table_entry;

    switch (reqinfo->mode) {
    case MODE_GET:
        for (request = requests; request; request = request->next) {
            table_entry = (mac_frame_error_table_entry *) netsnmp_extract_iterator_context(request);
            table_info = netsnmp_extract_table_info(request);

            switch (table_info->colnum) {
                case COLUMN_MAC_FRAME_ERROR_PORT:
                    if (!table_entry) {
                        netsnmp_set_request_error(reqinfo, request, SNMP_NOSUCHINSTANCE);
                        continue;
                    }
                    snmp_set_var_typed_value(request->requestvb, ASN_OCTET_STR, table_entry->port, strlen(table_entry->port));
                    break;
                case COLUMN_MAC_FRAME_ERROR_ALIGNMEN:
                    if (!table_entry) {
                        netsnmp_set_request_error(reqinfo, request, SNMP_NOSUCHINSTANCE);
                        continue;
                    }
                    snmp_set_var_typed_value(request->requestvb, ASN_COUNTER64, &(table_entry->alignmentError), sizeof(table_entry->alignmentError));
                    break;
                case COLUMN_MAC_FRAME_ERROR_CRC:
                    if (!table_entry) {
                        netsnmp_set_request_error(reqinfo, request, SNMP_NOSUCHINSTANCE);
                        continue;
                    }
                    snmp_set_var_typed_value(request->requestvb, ASN_COUNTER64, &(table_entry->crcError), sizeof(table_entry->crcError));
                    break;
                case COLUMN_MAC_FRAME_ERROR_EXCESSIVE_COLLISIONS:
                    if (!table_entry) {
                        netsnmp_set_request_error(reqinfo, request, SNMP_NOSUCHINSTANCE);
                        continue;
                    }
                    snmp_set_var_typed_value(request->requestvb, ASN_COUNTER64, &(table_entry->excessiveCollisions), sizeof(table_entry->excessiveCollisions));
                    break;
                case COLUMN_MAC_FRAME_ERROR_INTERNAL_MAC_TR:
                    if (!table_entry) {
                        netsnmp_set_request_error(reqinfo, request, SNMP_NOSUCHINSTANCE);
                        continue;
                    }
                    snmp_set_var_typed_value(request->requestvb, ASN_COUNTER64, &(table_entry->internalMacTrError), sizeof(table_entry->internalMacTrError));
                    break;
                case COLUMN_MAC_FRAME_ERROR_CARRIER_SENSE:
                    if (!table_entry) {
                        netsnmp_set_request_error(reqinfo, request, SNMP_NOSUCHINSTANCE);
                        continue;
                    }
                    snmp_set_var_typed_value(request->requestvb, ASN_COUNTER64, &(table_entry->carrierSenseError), sizeof(table_entry->carrierSenseError));
                    break;
                case COLUMN_MAC_FRAME_ERROR_INTERNAL_MAC_RX:
                    if (!table_entry) {
                        netsnmp_set_request_error(reqinfo, request, SNMP_NOSUCHINSTANCE);
                        continue;
                    }
                    snmp_set_var_typed_value(request->requestvb, ASN_COUNTER64, &(table_entry->internalMacRxError), sizeof(table_entry->internalMacRxError));
                    break;
                case COLUMN_MAC_FRAME_ERROR_IN_PAUSE:
                    if (!table_entry) {
                        netsnmp_set_request_error(reqinfo, request, SNMP_NOSUCHINSTANCE);
                        continue;
                    }
                    snmp_set_var_typed_value(request->requestvb, ASN_COUNTER64, &(table_entry->inPauseFrame), sizeof(table_entry->inPauseFrame));
                    break;
                case COLUMN_MAC_FRAME_ERROR_OUT_PAUSE:
                    if (!table_entry) {
                        netsnmp_set_request_error(reqinfo, request, SNMP_NOSUCHINSTANCE);
                        continue;
                    }
                    snmp_set_var_typed_value(request->requestvb, ASN_COUNTER64, &(table_entry->outPauseFrame), sizeof(table_entry->outPauseFrame));
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
