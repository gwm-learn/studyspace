#ifndef _VLAN_TABLE_H_
#define _VLAN_TABLE_H_

#define VLAN_TABLE_TIMEOUT 5
#define VLAN_MSG_MAXS      4094
#define PORT_NUM           26
#define VLAN_PORT_MAX      28

enum column_vlan_state_table
{
    COLUMN_VLAN_STATE_ID = 1,
    COLUMN_VLAN_STATE_TAG_PORT,
    COLUMN_VLAN_STATE_UNTAG_PORT,
};

enum column_vlan_config_table
{
    COLUMN_VLAN_CONFIG_ID = 1,
    COLUMN_VLAN_CONFIG_MODE,
    COLUMN_VLAN_CONFIG_PORT,
    COLUMN_VLAN_CONFIG_TAG,
    COLUMN_VLAN_CONFIG_UNTAG,
};

typedef struct _vlan_state_table_entry
{
    struct _vlan_state_table_entry *next;
    long vlan_id;
    char vlan_tag_port[128];
    char vlan_untag_port[128];
}vlan_state_table_entry;

typedef struct _vlan_config_table_entry
{
    struct _vlan_config_table_entry *next;
    long vlan_id;
    long vlan_mode;
    char vlan_port[32];
    char vlan_tag[128];
    char vlan_untag[128];
}vlan_config_table_entry;

void init_vlan_table(void);
void init_vlan_table_node(void);
void init_vlan_state_table_node(void);
void init_vlan_config_table_node(void);

vlan_state_table_entry *vlan_state_table_createEntry(long vlan_id, char *vlan_tag_port, char *vlan_untag_port);
vlan_config_table_entry *vlan_config_table_createEntry(long vlan_id, long vlan_mode, char *vlan_port, char *vlan_tag, char *vlan_untag);
void vlan_state_table_removeEntry(vlan_state_table_entry *entry);
void vlan_config_table_removeEntry(vlan_config_table_entry *entry);
int vlan_state_table_load(netsnmp_cache * cache, void *vmagic);
int vlan_config_table_load(netsnmp_cache * cache, void *vmagic);
void vlan_state_table_free(netsnmp_cache * cache, void *vmagic);
void vlan_config_table_free(netsnmp_cache * cache, void *vmagic);

netsnmp_variable_list *vlan_state_table_get_first_data_point(void **my_loop_context, void **my_data_context, netsnmp_variable_list * put_index_data, netsnmp_iterator_info *mydata);
netsnmp_variable_list *vlan_config_table_get_first_data_point(void **my_loop_context, void **my_data_context, netsnmp_variable_list * put_index_data, netsnmp_iterator_info *mydata);
netsnmp_variable_list * vlan_state_table_get_next_data_point(void **my_loop_context, void **my_data_context, netsnmp_variable_list * put_index_data, netsnmp_iterator_info *mydata);
netsnmp_variable_list * vlan_config_table_get_next_data_point(void **my_loop_context, void **my_data_context, netsnmp_variable_list * put_index_data, netsnmp_iterator_info *mydata);
int vlan_state_table_handler(netsnmp_mib_handler *handler, netsnmp_handler_registration *reginfo, netsnmp_agent_request_info *reqinfo, netsnmp_request_info *requests);
int vlan_config_table_handler(netsnmp_mib_handler *handler, netsnmp_handler_registration *reginfo, netsnmp_agent_request_info *reqinfo, netsnmp_request_info *requests);

#endif