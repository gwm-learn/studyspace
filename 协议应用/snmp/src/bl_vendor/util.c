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

void *malloc_in_comm_list_tail(void **listhead, int size)
{
    if (listhead && size > sizeof(void *))
    {
        COMM_LIST_HEAD *pCurr = (COMM_LIST_HEAD *)malloc(size);
        if (pCurr)
        {
            memset(pCurr, 0x0, size);
            if (*listhead)
            {
                COMM_LIST_HEAD *pTail = (COMM_LIST_HEAD *)(*listhead);
                while(pTail->next)
                {
                    pTail = pTail->next;
                }
                pTail->next = pCurr;
            }
            else
            {
                *listhead = pCurr;
            }
            pCurr->next = NULL;
        }
        return pCurr;
    }
    return NULL;
}

void free_whole_comm_list(void **listhead)
{
    if (listhead && *listhead)
    {
        COMM_LIST_HEAD *pCurr = ((COMM_LIST_HEAD *)(*listhead));
        COMM_LIST_HEAD *pNext = pCurr->next;
        *listhead = NULL;
        while(pCurr)
        {
            free(pCurr);
            pCurr = pNext;
            if (pNext) { pNext = pNext->next; }
        }
    }
}

void free_in_comm_list(void **listhead, void *listNode)
{
    if (listhead && *listhead && listNode)
    {
        COMM_LIST_HEAD *pPre = ((COMM_LIST_HEAD *)(*listhead));
        COMM_LIST_HEAD *pTmp = pPre->next;

        if (*listhead == listNode)
        {
            *listhead = pTmp;
            free(listNode);
        }
        else
        {
            while(pTmp)
            {
                if (pTmp == listNode)
                {
                    pPre->next = pTmp->next;
                    free(listNode);
                    break;
                }
                pPre = pTmp;
                pTmp = pTmp->next;
            }
        }
    }
}

void del_spec_in_str(char spec, char *str, int str_len)
{
    int i = 0;
    int j = 0;

    if (str == NULL)
        return;

    for (i = 0; str[i] != '\0' && i < str_len; i++)
    {
        if (str[i] == spec)
        {
            for (j = i; str[j] != '\0' && str[j+1] != '\0' && j+1 < str_len; j++)
            {
                str[j] = str[j+1];
            }
            str[j] = '\0';
        }
    }
}