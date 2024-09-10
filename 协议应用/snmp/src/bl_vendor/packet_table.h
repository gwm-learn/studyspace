#ifndef _PACKET_TABLE_H_
#define _PACKET_TABLE_H_

#define PACKET_TABLE_TIMEOUT 5

enum column_basic_packet_table
{
    COLUMN_BASIC_PACKET_PORT = 1,
    COLUMN_BASIC_PACKET_RX_BYTES,
    COLUMN_BASIC_PACKET_RX_PACKETS,
    COLUMN_BASIC_PACKET_RX_DROPPED,
    COLUMN_BASIC_PACKET_RX_ERRORS,
    COLUMN_BASIC_PACKET_TX_BYTES,
    COLUMN_BASIC_PACKET_TX_PACKETS,
    COLUMN_BASIC_PACKET_TX_DROPPED,
    COLUMN_BASIC_PACKET_TX_ERRORS,
};

enum column_detail_packet_table
{
    COLUMN_DETAIL_PACKET_PORT = 1,
    COLUMN_DETAIL_PACKET_RX_BYTES,
    COLUMN_DETAIL_PACKET_RX_UCASTPKTS,
    COLUMN_DETAIL_PACKET_RX_MULTICAST,
    COLUMN_DETAIL_PACKET_RX_BROADCAST,
    COLUMN_DETAIL_PACKET_TX_UCASTPKTS,
    COLUMN_DETAIL_PACKET_TX_MULTICAST,
    COLUMN_DETAIL_PACKET_TX_BROADCAST,
    COLUMN_DETAIL_PACKET_DELAY_EXCEED_DISCARD,
    COLUMN_DETAIL_PACKET_MTU_EXCEED_DISCARD,
    COLUMN_DETAIL_PACKET_TP_PORT_DISCARD,
};

typedef struct _basic_packet_table_entry
{
    struct _basic_packet_table_entry *next;
    char port[32];
    unsigned long long rx_bytes;
    unsigned long long rx_packets;
    unsigned long long rx_dropped;
    unsigned long long rx_errors;
    unsigned long long tx_bytes;
    unsigned long long tx_packets;
    unsigned long long tx_dropped;
    unsigned long long tx_errors;
}basic_packet_table_entry;

typedef struct _detail_packet_table_entry
{
    struct _detail_packet_table_entry *next;
    char port[32];
    unsigned long long rx_bytes;
    unsigned long long rx_ucastpkts;
    unsigned long long rx_multicast;
    unsigned long long rx_broadcast;
    unsigned long long tx_ucastpkts;
    unsigned long long tx_multicast;
    unsigned long long tx_broadcast;
    unsigned long long delat_exceed_discard;
    unsigned long long mtu_exceed_discard;
    unsigned long long tp_port_discard;
}detail_packet_table_entry;

void init_packet_table(void);
void init_packet_table_node(void);
void init_basic_packet_table_node(void);
void init_detail_packet_table_node(void);

basic_packet_table_entry *basic_packet_table_createEntry(
    char *port,
    unsigned long long rx_bytes,
    unsigned long long rx_packets,
    unsigned long long rx_dropped,
    unsigned long long rx_errors,
    unsigned long long tx_bytes,
    unsigned long long tx_packets,
    unsigned long long tx_dropped,
    unsigned long long tx_errors);

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
    unsigned long long tp_port_discard);

void basic_packet_table_removeEntry(basic_packet_table_entry *entry);
void detail_packet_table_removeEntry(detail_packet_table_entry *entry);
int basic_packet_table_load(netsnmp_cache * cache, void *vmagic);
int detail_packet_table_load(netsnmp_cache * cache, void *vmagic);
void basic_packet_table_free(netsnmp_cache * cache, void *vmagic);
void detail_packet_table_free(netsnmp_cache * cache, void *vmagic);

netsnmp_variable_list *basic_packet_table_get_first_data_point(void **my_loop_context, void **my_data_context, netsnmp_variable_list * put_index_data, netsnmp_iterator_info *mydata);
netsnmp_variable_list *detail_packet_table_get_first_data_point(void **my_loop_context, void **my_data_context, netsnmp_variable_list * put_index_data, netsnmp_iterator_info *mydata);
netsnmp_variable_list * basic_packet_table_get_next_data_point(void **my_loop_context, void **my_data_context, netsnmp_variable_list * put_index_data, netsnmp_iterator_info *mydata);
netsnmp_variable_list * detail_packet_table_get_next_data_point(void **my_loop_context, void **my_data_context, netsnmp_variable_list * put_index_data, netsnmp_iterator_info *mydata);
int basic_packet_table_handler(netsnmp_mib_handler *handler, netsnmp_handler_registration *reginfo, netsnmp_agent_request_info *reqinfo, netsnmp_request_info *requests);
int detail_packet_table_handler(netsnmp_mib_handler *handler, netsnmp_handler_registration *reginfo, netsnmp_agent_request_info *reqinfo, netsnmp_request_info *requests);

#endif