#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>
#include "util.h"

void mac_array_str(uint8 *mac_in, char *mac_out)
{
    int i;
    char pbuf[4] = {0};

    for (i = 0; i < MAC_ARRAY_LEN; i++) {
        sprintf(pbuf, "%02x", mac_in[i]);
        strncat(mac_out, pbuf, 2);
    }
}

void mac_str_array(char *mac_in, uint8 *mac_out)
{
    int i = 0;
    char *mac_p;
    char *endptr;
    char mac_tmp[4] = {0};

    mac_p = mac_in;
    for (i = 0; i < MAC_ARRAY_LEN; i++) {
        strncpy(mac_tmp, mac_p, 2);
        mac_out[i] = strtol(mac_tmp, &endptr, 16);
        memset(mac_tmp, 0, sizeof(mac_tmp));
        mac_p+=2;
    }
}

int mac_str_check(char *mac_in)
{
    int i;
    char *p = mac_in;

    for (i = 0; i < MAC_ARRAY_LEN * 2; i++) {
        if ((*p >= '0' && *p <= '9') || (*p >= 'a' && *p <= 'f') || (*p >= 'A' && *p <= 'F')) {
            p++;
            continue;
        } else {
            return 0;
        }
    }
    return 1;
}