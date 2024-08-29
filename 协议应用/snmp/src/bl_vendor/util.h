#ifndef __UTIL__
#define __UTIL__

#include <common/rt_type.h>
#include <osal/interface.h>

#define MAC_ARRAY_LEN 6

void mac_array_str(uint8 *mac_in, char *mac_out);
void mac_str_array(char *mac_in, uint8 *mac_out);

#endif