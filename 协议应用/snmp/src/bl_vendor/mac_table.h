#ifndef _MAC_TABLE_H_
#define _MAC_TABLE_H_

#define MAC_TABLE_TIMEOUT 5

enum column_mac_table
{
    COLUMN_MAC_ID = 1,
    COLUMN_MAC_VLAN,
    COLUMN_MAC_TYPE,
    COLUMN_MAC_PORT,
    COLUMN_MAC_ADDRTESS,
    COLUMN_MAC_ROW_STATUS,
};

typedef struct _mac_table_entry
{
    struct _mac_table_entry *next;
    long mac_id;
    long mac_vlan;
    long mac_type; /*0-dynamic 1-static"*/
    char mac_port[16];
    char mac_address[16];
    long mac_row_status;
}mac_table_entry;

void init_mac_table(void);
void init_mac_table_node(void);

mac_table_entry *mac_table_createEntry(long mac_id, long mac_vlan, long mac_type, char *mac_port, char *mac_address, int mac_row_status);
void mac_table_removeEntry(mac_table_entry *entry);
int mac_table_load(netsnmp_cache * cache, void *vmagic);
void mac_table_free(netsnmp_cache * cache, void *vmagic);

netsnmp_variable_list *mac_table_get_first_data_point(void **my_loop_context, void **my_data_context, netsnmp_variable_list * put_index_data, netsnmp_iterator_info *mydata);
netsnmp_variable_list * mac_table_get_next_data_point(void **my_loop_context, void **my_data_context, netsnmp_variable_list * put_index_data, netsnmp_iterator_info *mydata);

void mac_table_set_vlan(char *mac, int vlan);
void mac_table_set_port(char *mac, char *port);
void mac_table_set_mac(char *mac_old, char *mac_new);

int mac_table_handler(netsnmp_mib_handler *handler, netsnmp_handler_registration *reginfo, netsnmp_agent_request_info *reqinfo, netsnmp_request_info *requests);

#endif