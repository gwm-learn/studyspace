#ifndef _MAC_FRAME_TABLE_H_
#define _MAC_FRAME_TABLE_H_

#define MAC_FRAME_TABLE_TIMEOUT 5

enum column_mac_frame_lenth_table
{
    COLUMN_MAC_FRAME_LENTH_PORT = 1,
    COLUMN_MAC_FRAME_LENTH_UNDERSIZE_FRAME,
    COLUMN_MAC_FRAME_LENTH_BYTE_FRAME_64,
    COLUMN_MAC_FRAME_LENTH_BYTE_FRAME_64_127,
    COLUMN_MAC_FRAME_LENTH_BYTE_FRAME_128_255,
    COLUMN_MAC_FRAME_LENTH_BYTE_FRAME_256_511,
    COLUMN_MAC_FRAME_LENTH_BYTE_FRAME_512_1023,
    COLUMN_MAC_FRAME_LENTH_BYTE_FRAME_1024_1518,
    COLUMN_MAC_FRAME_LENTH_OVERSIZE_FRAME,
    COLUMN_MAC_FRAME_LENTH_JABBER_FRAME,
};

enum column_mac_frame_error_table
{
    COLUMN_MAC_FRAME_ERROR_PORT = 1,
    COLUMN_MAC_FRAME_ERROR_ALIGNMEN,
    COLUMN_MAC_FRAME_ERROR_CRC,
    COLUMN_MAC_FRAME_ERROR_EXCESSIVE_COLLISIONS,
    COLUMN_MAC_FRAME_ERROR_INTERNAL_MAC_TR,
    COLUMN_MAC_FRAME_ERROR_CARRIER_SENSE,
    COLUMN_MAC_FRAME_ERROR_INTERNAL_MAC_RX,
    COLUMN_MAC_FRAME_ERROR_IN_PAUSE,
    COLUMN_MAC_FRAME_ERROR_OUT_PAUSE,
};

typedef struct _mac_frame_lenth_table_entry
{
    struct _mac_frame_lenth_table_entry *next;
    char port[32];
    unsigned long long undersizeFrame;
    unsigned long long BytesFrame64;
    unsigned long long BytesFrame64to127;
    unsigned long long BytesFrame128to255;
    unsigned long long BytesFrame256to511;
    unsigned long long BytesFrame512to1023;
    unsigned long long BytesFrame1024to1518;
    unsigned long long oversizeFrame;
    unsigned long long jabberFrame;
}mac_frame_lenth_table_entry;

typedef struct _mac_frame_error_table_entry
{
    struct _mac_frame_error_table_entry *next;
    char port[32];
    unsigned long long alignmentError;
    unsigned long long crcError;
    unsigned long long excessiveCollisions;
    unsigned long long internalMacTrError;
    unsigned long long carrierSenseError;
    unsigned long long internalMacRxError;
    unsigned long long inPauseFrame;
    unsigned long long outPauseFrame;
}mac_frame_error_table_entry;

void init_mac_frame_table(void);
void init_mac_frame_table_node(void);
void init_mac_frame_lenth_table_node(void);
void init_mac_frame_error_table_node(void);

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
    unsigned long long jabberFrame);

mac_frame_error_table_entry *mac_frame_error_table_createEntry(
    char *port,
    unsigned long long alignmentError,
    unsigned long long crcError,
    unsigned long long excessiveCollisions,
    unsigned long long internalMacTrError,
    unsigned long long carrierSenseError,
    unsigned long long internalMacRxError,
    unsigned long long inPauseFrame,
    unsigned long long outPauseFrame);

void mac_frame_lenth_table_removeEntry(mac_frame_lenth_table_entry *entry);
void mac_frame_error_table_removeEntry(mac_frame_error_table_entry *entry);
int mac_frame_lenth_table_load(netsnmp_cache * cache, void *vmagic);
int mac_frame_error_table_load(netsnmp_cache * cache, void *vmagic);
void mac_frame_lenth_table_free(netsnmp_cache * cache, void *vmagic);
void mac_frame_error_table_free(netsnmp_cache * cache, void *vmagic);

netsnmp_variable_list *mac_frame_lenth_table_get_first_data_point(void **my_loop_context, void **my_data_context, netsnmp_variable_list * put_index_data, netsnmp_iterator_info *mydata);
netsnmp_variable_list *mac_frame_error_table_get_first_data_point(void **my_loop_context, void **my_data_context, netsnmp_variable_list * put_index_data, netsnmp_iterator_info *mydata);
netsnmp_variable_list * mac_frame_lenth_table_get_next_data_point(void **my_loop_context, void **my_data_context, netsnmp_variable_list * put_index_data, netsnmp_iterator_info *mydata);
netsnmp_variable_list * mac_frame_error_table_get_next_data_point(void **my_loop_context, void **my_data_context, netsnmp_variable_list * put_index_data, netsnmp_iterator_info *mydata);
int mac_frame_lenth_table_handler(netsnmp_mib_handler *handler, netsnmp_handler_registration *reginfo, netsnmp_agent_request_info *reqinfo, netsnmp_request_info *requests);
int mac_frame_error_table_handler(netsnmp_mib_handler *handler, netsnmp_handler_registration *reginfo, netsnmp_agent_request_info *reqinfo, netsnmp_request_info *requests);

#endif