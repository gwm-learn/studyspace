#ifndef __UTIL__
#define __UTIL__

#include <common/rt_type.h>
#include <osal/interface.h>

#define MAC_ARRAY_LEN           6

typedef struct _COMM_LIST_HEAD {
    struct _COMM_LIST_HEAD *next;
    unsigned char data[0];
} COMM_LIST_HEAD;

void mac_array_str(uint8 *mac_in, char *mac_out);
void mac_str_array(char *mac_in, uint8 *mac_out);

void *malloc_in_comm_list_tail(void **listhead, int size);
void free_whole_comm_list(void **listhead);
void free_in_comm_list(void **listhead, void *listNode);

void del_spec_in_str(char spec, char *str, int str_len);

#endif