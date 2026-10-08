/*====================================================

FILE:  QCMAP_LAN_Firewall_Client.cpp

SERVICES:
QCMAP LAN Firewall Client Implementation

=====================================================

  Copyright (c) 2022-2023 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Confidential and Proprietary - Qualcomm Technologies, Inc.

=====================================================*/
/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  when       who        what, where, why
  --------   ---        -------------------------------------------------------
  06/07/23   mk         Splitted LAN_CLIENT module into multiple modules
  ===========================================================================*/

#include "QCMAP_LAN_Client.h"

/*=====================================================================
  FUNCTION checkSNATEntryLimit()
======================================================================*/
/*!
@brief
  - check if the number of snat entries exceeds 50

@return
  true - Success
  false - Failure

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/

boolean QCMAP_LAN_Client::checkSNATEntryLimit
(
  qmi_error_type_v01 *qmi_err_num
)
{
  uint32_t no_of_redirects = 0;
  char result[QCMAP_MAX_SCAN_SIZE] = {0};

  if (UciGetUtility(owrt_filename[static_cast<int>(config_file::QCMAP_LAN)],
      "no_of_configs", 0, "no_of_redirects", result, 0))
  {
    no_of_redirects = atoi(result);
  }
  else
  {
    LOG_MSG_ERROR("UciGetUtility failed ",0,0,0);
    *qmi_err_num = QMI_ERR_INTERNAL_V01;
    return false;
  }

  LOG_MSG_INFO1("SNAT count :%d",no_of_redirects,0,0);
  if( no_of_redirects < QCMAP_MSGR_MAX_SNAT_ENTRIES_V01 )
  {
    return true;
  }
  else
  {
    LOG_MSG_ERROR("Reached to max count",0,0,0);
    return false;
  }
}

 /*=====================================================================
  FUNCTION checkFirewallEntryLimit()
======================================================================*/
/*!
@brief
  - check if the number of entries exceeds 128

@return
  true - Success
  false - Failure

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/

boolean QCMAP_LAN_Client::checkFirewallEntryLimit
(
  qmi_error_type_v01 *qmi_err_num
)
{
  uint32_t no_of_total_firewall_rules = 0;
  uint32_t total_user_add_rules = 0;

  char result[QCMAP_MAX_SCAN_SIZE] = {0};

  memset(result, 0, QCMAP_MAX_SCAN_SIZE);
  if (UciGetUtility(owrt_filename[static_cast<int>(config_file::QCMAP_FIREWALL)],
       "firewall", 0, "no_of_rules", result, 0))
  {
     no_of_total_firewall_rules = atoi(result);
  }
  else
  {
    LOG_MSG_ERROR("UciGetUtility failed ",0,0,0);
    *qmi_err_num = QMI_ERR_INTERNAL_V01;
    return false;
  }

  total_user_add_rules = no_of_total_firewall_rules;
  LOG_MSG_INFO1("Firewall count:%d ",no_of_total_firewall_rules,0,0);
  /* checks to avoid addition of rule it total entry limit exceeds 128*/
  if( total_user_add_rules < MAX_FIREWALL_ENTRY )
  {
    return true;
  }
  else
  {
    LOG_MSG_ERROR("Reached to max count",0,0,0);
    return false;
  }

}

/*======================================================
  FUNCTION:  DeleteConntrackEntryForDropIPv6FirewallEntries
  =====================================================*/
  /*!
      @brief
      Delete the client conntrack when we are adding a firewall entry for
      that IPv6 address and port combination

      @params
         IPv6 address
         Protocol
         UDP Port start number
         UDP Port Range
         TCP Port start number
         TCP Port Range

      @return
      void
  */
/*====================================================*/
void QCMAP_LAN_Client::DeleteConntrackEntryForDropIPv6FirewallEntries
(
  qcmap_msgr_firewall_entry_conf_t *firewall_entry,
  uint8_t protocol_num,
  qmi_error_type_v01 *qmi_err_num
)
{
  uint16_t prefixbitvalue_src_addr = 0;
  uint16_t prefixbitvalue_dest_addr = 0;
  char command[MAX_COMMAND_STR_LEN]={0};
  char tmp[MAX_COMMAND_STR_LEN]={0};
  char v6str[INET6_ADDRSTRLEN]={0};
  uint8_t prefixWords_src_addr = 0;
  uint8_t prefixWords_dest_addr = 0;
  uint8_t prefixBits_src_addr = 0;
  uint8_t prefixBits_dest_addr = 0;
  bool check_sport = false;
  bool check_dport = false;
  uint16_t min_sport;
  uint16_t max_sport;
  uint16_t min_dport;
  uint16_t max_dport;
  uint16_t port = 0;
  FILE *fd = NULL;
  char *protocol=NULL,*saddr=NULL,*daddr=NULL, *sport=NULL, *dport=NULL, *ptr;
  in6_addr tmp_src_ipv6, curr_src_ipv6_addr, tmp_dest_ipv6, curr_dest_ipv6_addr;
  int bufferSize = MAX_COMMAND_STR_LEN;
  char stringline[MAX_COMMAND_STR_LEN] = {0};
  bool processV6srcaddr =  false;
  bool processV6destaddr =  false;
  bool skipPrefixBitProcessForSrcAddr = false;
  bool skipPrefixBitProcessForDestAddr = false;

  LOG_MSG_INFO1("DeleteConntrackEntryForDropIPv6FirewallEntries \n",0,0,0);

  if( firewall_entry == NULL )
  {
    LOG_MSG_ERROR("NULL firewall_entry\n",0,0,0);
    *qmi_err_num = QMI_ERR_INVALID_ARG_V01;
    return;
  }

  if (protocol_num != PS_IPPROTO_TCP && protocol_num != PS_IPPROTO_UDP && protocol_num != PS_IPPROTO_ICMP6)
  {
    if ( protocol_num == PS_IPPROTO_TCP_UDP)
    {
      DeleteConntrackEntryForDropIPv6FirewallEntries(firewall_entry,PS_IPPROTO_TCP, qmi_err_num);
      DeleteConntrackEntryForDropIPv6FirewallEntries(firewall_entry,PS_IPPROTO_UDP, qmi_err_num);
    }
    return;
  }

  /*
   */
  if ( protocol_num == PS_IPPROTO_UDP)
    snprintf(command, MAX_COMMAND_STR_LEN,
             "conntrack -L -f ipv6 | cut -f1,9,10,11,12 -d ' ' | grep -v dport=%d ",
             PS_IPPROTO_DNS);
  else if ( protocol_num == PS_IPPROTO_TCP)
    snprintf(command, MAX_COMMAND_STR_LEN,
             "conntrack -L -f ipv6 | cut -f1,10,11,12,13 -d ' ' | grep -v dport=%d ",
             PS_IPPROTO_DNS);
  else if (protocol_num == PS_IPPROTO_ICMP6)
    snprintf(command, MAX_COMMAND_STR_LEN, "conntrack -L -f ipv6 | cut -f1,6,7 -d ' ' ");

  switch(protocol_num)
  {
    case PS_IPPROTO_TCP:

      strlcat(command,"| grep tcp ",MAX_COMMAND_STR_LEN);

      if ( firewall_entry->filter_spec.next_prot_hdr.tcp.field_mask
           & IPFLTR_MASK_TCP_SRC_PORT )
      {
        if( firewall_entry->filter_spec.next_prot_hdr.tcp.src.range !=0 )
        {
          min_sport = firewall_entry->filter_spec.next_prot_hdr.tcp.src.port;
          max_sport = firewall_entry->filter_spec.next_prot_hdr.tcp.src.port +
                      firewall_entry->filter_spec.next_prot_hdr.tcp.src.range;
          check_sport = true;
        }
        else
        {
          snprintf(tmp,MAX_COMMAND_STR_LEN,"| grep sport=%d ",
                   firewall_entry->filter_spec.next_prot_hdr.tcp.src.port);
        }
        strlcat(command,tmp,MAX_COMMAND_STR_LEN);
        memset(tmp,0,MAX_COMMAND_STR_LEN);
      }

      if ( firewall_entry->filter_spec.next_prot_hdr.tcp.field_mask & IPFLTR_MASK_TCP_DST_PORT )
      {
        if ( firewall_entry->filter_spec.next_prot_hdr.tcp.dst.range !=0 )
        {
          min_dport = firewall_entry->filter_spec.next_prot_hdr.tcp.dst.port;
          max_dport = firewall_entry->filter_spec.next_prot_hdr.tcp.dst.port +
                      firewall_entry->filter_spec.next_prot_hdr.tcp.dst.range;
          check_dport = true;
        }else
        {
          snprintf(tmp,MAX_COMMAND_STR_LEN,"| grep dport=%d ",
                   firewall_entry->filter_spec.next_prot_hdr.tcp.dst.port);
        }
        strlcat(command,tmp,MAX_COMMAND_STR_LEN);
        memset(tmp,0,MAX_COMMAND_STR_LEN);
      }

      break;

    case PS_IPPROTO_UDP:

      strlcat(command,"| grep udp ",MAX_COMMAND_STR_LEN);

      if ( firewall_entry->filter_spec.next_prot_hdr.udp.field_mask &
           IPFLTR_MASK_UDP_SRC_PORT )
      {
        if ( firewall_entry->filter_spec.next_prot_hdr.udp.src.range !=0 )
        {
          min_sport = firewall_entry->filter_spec.next_prot_hdr.udp.src.port;
          max_sport = firewall_entry->filter_spec.next_prot_hdr.udp.src.port +
                      firewall_entry->filter_spec.next_prot_hdr.udp.src.range;
          check_sport = true;
        }else
        {
          snprintf(tmp,MAX_COMMAND_STR_LEN,"| grep sport=%d ",
                   firewall_entry->filter_spec.next_prot_hdr.udp.src.port );
        }
        strlcat(command,tmp,MAX_COMMAND_STR_LEN);
        memset(tmp,0,MAX_COMMAND_STR_LEN);
      }

      if ( firewall_entry->filter_spec.next_prot_hdr.udp.field_mask & IPFLTR_MASK_UDP_DST_PORT )
      {
        if ( firewall_entry->filter_spec.next_prot_hdr.udp.dst.range !=0 )
        {
          min_dport = firewall_entry->filter_spec.next_prot_hdr.udp.dst.port;
          max_dport = firewall_entry->filter_spec.next_prot_hdr.udp.dst.port +
                      firewall_entry->filter_spec.next_prot_hdr.udp.dst.range;
          check_dport = true;
        }else
        {
          snprintf(tmp,MAX_COMMAND_STR_LEN,"| grep dport=%d ",
                   firewall_entry->filter_spec.next_prot_hdr.udp.dst.port );
        }
        strlcat(command,tmp,MAX_COMMAND_STR_LEN);
        memset(tmp,0,MAX_COMMAND_STR_LEN);
      }

      break;

    case PS_IPPROTO_ICMP6:
      /* For icmp protocol, we do not have src or dest ports */
      strlcat(command,"| grep icmpv6 ",MAX_COMMAND_STR_LEN);

      break;
  }

  if ( firewall_entry->filter_spec.ip_hdr.v6.field_mask & IPFLTR_MASK_IP6_SRC_ADDR )
  {
    prefixWords_src_addr = firewall_entry->filter_spec.ip_hdr.v6.src.prefix_len/(sizeof(uint16_t)*8);
    prefixBits_src_addr = firewall_entry->filter_spec.ip_hdr.v6.src.prefix_len%(sizeof(uint16_t)*8);

    memcpy (&curr_src_ipv6_addr.s6_addr ,
            firewall_entry->filter_spec.ip_hdr.v6.src.addr.in6_u.u6_addr16,
            QCMAP_MSGR_IPV6_ADDR_LEN_V01);

    prefixbitvalue_src_addr = curr_src_ipv6_addr.s6_addr[prefixWords_src_addr] >> (16 - prefixBits_src_addr);

    if ( prefixBits_src_addr == 0 && prefixWords_src_addr == 8 )
    {
      processV6srcaddr = false;

      inet_ntop(AF_INET6,
               (void*)&curr_src_ipv6_addr, v6str, sizeof(v6str));
      snprintf(tmp, MAX_COMMAND_STR_LEN,"| grep src=%s", (char*)v6str);
      strlcat(command,tmp,MAX_COMMAND_STR_LEN);
      memset(tmp,0,MAX_COMMAND_STR_LEN);
    }
    else
    {
      processV6srcaddr = true;
    }
  }

  if ( firewall_entry->filter_spec.ip_hdr.v6.field_mask & IPFLTR_MASK_IP6_DST_ADDR )
  {
    prefixWords_dest_addr = firewall_entry->filter_spec.ip_hdr.v6.dst.prefix_len/(sizeof(uint16_t)*8);
    prefixBits_dest_addr = firewall_entry->filter_spec.ip_hdr.v6.dst.prefix_len%(sizeof(uint16_t)*8);

    memcpy (&curr_dest_ipv6_addr.s6_addr ,
            firewall_entry->filter_spec.ip_hdr.v6.dst.addr.in6_u.u6_addr16,
            QCMAP_MSGR_IPV6_ADDR_LEN_V01);

    prefixbitvalue_dest_addr = curr_dest_ipv6_addr.s6_addr[prefixWords_dest_addr] >> (16 - prefixBits_dest_addr);

    if ( prefixBits_dest_addr == 0 && prefixWords_dest_addr == 8 )
    {
      processV6destaddr = false;

      inet_ntop(AF_INET6,
               (void*)&curr_dest_ipv6_addr, v6str, sizeof(v6str));
      snprintf(tmp, MAX_COMMAND_STR_LEN,"| grep dst=%s", (char*)v6str);
      strlcat(command,tmp,MAX_COMMAND_STR_LEN);
      memset(tmp,0,MAX_COMMAND_STR_LEN);
    }
    else
    {
      processV6destaddr = true;
    }
  }


  snprintf(tmp, MAX_COMMAND_STR_LEN, " > %s", IPv6_CONNTRACK_FILTER_PATH);
  strlcat(command, tmp, MAX_COMMAND_STR_LEN);
  memset(tmp,0,MAX_COMMAND_STR_LEN);
  printf("Grep : %s \n", command);
  ds_system_call(command, strlen(command));

  fd = fopen(IPv6_CONNTRACK_FILTER_PATH,"r");


  if(fd == NULL)
  {
    LOG_MSG_ERROR("FetchPortInfoFromV6Conntrack - Error in opening ",0,0,0);
    *qmi_err_num = QMI_ERR_INTERNAL_V01;
    return;
  }

  while(fgets( stringline, bufferSize, fd) != NULL)
  {
    skipPrefixBitProcessForSrcAddr = false;
    skipPrefixBitProcessForDestAddr = false;

    protocol = strtok_r(stringline, " ", &ptr);

    saddr = strtok_r(NULL, " ", &ptr);

    if(saddr)
      *(saddr + 3) = ' ';

      //Copy to temp str then pton and then check prefixword byte value after right shifting

    if (processV6srcaddr)
    {
      if(saddr)
        inet_pton(AF_INET6, saddr + 4, (void *)&tmp_src_ipv6);

      if (memcmp (&curr_src_ipv6_addr.s6_addr16, &tmp_src_ipv6.s6_addr16,
                  (sizeof (uint16_t))*prefixWords_src_addr) != 0)
      {
        skipPrefixBitProcessForSrcAddr = true;
      }

      if ((skipPrefixBitProcessForSrcAddr)||
           ( tmp_src_ipv6.s6_addr16[prefixWords_src_addr] >> ((sizeof(uint16_t)*8)-prefixBits_src_addr) != prefixbitvalue_src_addr))
      {
        continue;
      }
    }

    daddr = strtok_r(NULL, " ", &ptr);
    if(daddr)
      *(daddr + 3) = ' ';

    if (processV6destaddr)
    {
      if(daddr)
        inet_pton(AF_INET6, daddr + 4, (void *)&tmp_dest_ipv6);

      if (memcmp (&curr_dest_ipv6_addr.s6_addr16, &tmp_dest_ipv6.s6_addr16,
                  (sizeof (uint16_t))*prefixWords_dest_addr) != 0)
      {
        skipPrefixBitProcessForDestAddr = true;
      }

      if ((skipPrefixBitProcessForDestAddr)||
           ( tmp_dest_ipv6.s6_addr16[prefixWords_dest_addr] >> ((sizeof(uint16_t)*8)-prefixBits_dest_addr) != prefixbitvalue_dest_addr))
      {
        continue;
      }
    }

    if( protocol_num != PS_IPPROTO_ICMP6 )
    {
      sport = strtok_r(NULL, " ", &ptr);

      if(sport)
      *(sport + 5) = ' ';

      dport = strtok_r(NULL, " ", &ptr);

      if(dport)
        *(dport + 5) = ' ';

      if (check_sport)
      {
        if(sport)
          port = ds_atoi((sport+6));
        if (port < min_sport || port > max_sport)
        continue;
      }

      if (check_dport)
      {
        if(dport)
          port = ds_atoi((dport+6));
        if (port < min_dport || port > max_dport)
          continue;
      }
    }


    //Delete contrack entry
    memset(tmp,0,MAX_COMMAND_STR_LEN);

    snprintf(command, MAX_COMMAND_STR_LEN, "conntrack -D -f ipv6");
    if(protocol)
    {
      snprintf(tmp, MAX_COMMAND_STR_LEN, " -p %s", protocol);
      strlcat(command,tmp,MAX_COMMAND_STR_LEN);
      memset(tmp,0,MAX_COMMAND_STR_LEN);
    }
    if(saddr)
    {
      snprintf(tmp, MAX_COMMAND_STR_LEN, "  --orig-%s", saddr);
      strlcat(command,tmp,MAX_COMMAND_STR_LEN);
      memset(tmp,0,MAX_COMMAND_STR_LEN);
    }
    if(daddr)
    {
      snprintf(tmp, MAX_COMMAND_STR_LEN, "  --orig-%s", daddr);
      strlcat(command,tmp,MAX_COMMAND_STR_LEN);
      memset(tmp,0,MAX_COMMAND_STR_LEN);
    }
    if(sport)
    {
      snprintf(tmp, MAX_COMMAND_STR_LEN, " --%s", sport);
      strlcat(command,tmp,MAX_COMMAND_STR_LEN);
      memset(tmp,0,MAX_COMMAND_STR_LEN);
    }
    if(dport)
    {
      snprintf(tmp, MAX_COMMAND_STR_LEN, " --%s", dport);
      strlcat(command,tmp,MAX_COMMAND_STR_LEN);
      memset(tmp,0,MAX_COMMAND_STR_LEN);
    }
    ds_system_call(command, strlen(command));
    memset(command,0,MAX_COMMAND_STR_LEN);
  }

  fclose(fd);

  snprintf(command,MAX_COMMAND_STR_LEN,"rm -rf %s",IPv6_CONNTRACK_FILTER_PATH);
  ds_system_call(command, strlen(command));
}

/*======================================================
  FUNCTION:  DeleteConntrackEntryForAcceptIPv6FirewallEntries
  =====================================================*/
  /*!
      @brief
      Delete the client conntrack when we are adding a firewall entry for
      that IPv6 address and port combination

      @params
         IPv6 address
         Protocol
         UDP Port start number
         UDP Port Range
         TCP Port start number
         TCP Port Range

      @return
      void
  */
/*====================================================*/
void QCMAP_LAN_Client::DeleteConntrackEntryForAcceptIPv6FirewallEntries
(
  qcmap_msgr_firewall_entry_conf_t *firewall_entry,
  uint8_t protocol_num,
  qmi_error_type_v01 *qmi_err_num
)
{
  uint16_t prefixbitvalue_src_addr = 0;
  uint16_t prefixbitvalue_dest_addr = 0;
  char command[MAX_COMMAND_STR_LEN]={0};
  char tmp[MAX_COMMAND_STR_LEN]={0};
  uint8_t prefixWords_dest_addr = 0;
  uint8_t prefixWords_src_addr = 0;
  uint8_t prefixBits_src_addr = 0;
  uint8_t prefixBits_dest_addr = 0;
  bool check_sport = false;
  bool check_dport = false;
  uint16_t min_sport;
  uint16_t max_sport;
  uint16_t min_dport;
  uint16_t max_dport;
  uint16_t port = 0;
  FILE *fd = NULL;
  char *protocol=NULL,*saddr=NULL, *daddr=NULL, *sport=NULL, *dport=NULL, *ptr;
  in6_addr tmp_src_ipv6, curr_src_ipv6_addr, tmp_dest_ipv6, curr_dest_ipv6_addr;
  int bufferSize = MAX_COMMAND_STR_LEN;
  char stringline[MAX_COMMAND_STR_LEN] = {0};
  bool processV6srcaddr =  false;
  bool processV6destaddr =  false;

  LOG_MSG_INFO1("DeleteConntrackEntryForAcceptIPv6FirewallEntries \n",0,0,0);

  if( firewall_entry == NULL )
  { 
    LOG_MSG_ERROR("NULL firewall_entry\n",0,0,0);
    *qmi_err_num = QMI_ERR_INVALID_ARG_V01;
    return;
  }

  if ( protocol_num == PS_IPPROTO_UDP)
    snprintf(command, MAX_COMMAND_STR_LEN,
             "conntrack -L -f ipv6 | cut -f1,9,10,11,12 -d ' ' | grep -v dport=%d ",
             PS_IPPROTO_DNS);
  else if ( protocol_num == PS_IPPROTO_TCP)
    snprintf(command, MAX_COMMAND_STR_LEN,
             "conntrack -L -f ipv6 | cut -f1,10,11,12,13 -d ' ' | grep -v dport=%d ",
             PS_IPPROTO_DNS);
  else if ( protocol_num == PS_IPPROTO_ICMP6)
    snprintf(command, MAX_COMMAND_STR_LEN,
             "conntrack -L -f ipv6 | cut -f1,6,7 -d ' ' ");

  switch(protocol_num)
  {
    case PS_IPPROTO_TCP:

      strlcat(command,"| grep tcp ",MAX_COMMAND_STR_LEN);

      if ( firewall_entry->filter_spec.next_prot_hdr.tcp.field_mask
           & IPFLTR_MASK_TCP_SRC_PORT )
      {
        min_sport = firewall_entry->filter_spec.next_prot_hdr.tcp.src.port;
        max_sport = firewall_entry->filter_spec.next_prot_hdr.tcp.src.port +
                    firewall_entry->filter_spec.next_prot_hdr.tcp.src.range;
        check_sport = true;
      }

      if ( firewall_entry->filter_spec.next_prot_hdr.tcp.field_mask & IPFLTR_MASK_TCP_DST_PORT )
      {
        min_dport = firewall_entry->filter_spec.next_prot_hdr.tcp.dst.port;
        max_dport = firewall_entry->filter_spec.next_prot_hdr.tcp.dst.port +
                    firewall_entry->filter_spec.next_prot_hdr.tcp.dst.range;
        check_dport = true;
      }

      break;

    case PS_IPPROTO_UDP:

      strlcat(command,"| grep udp ",MAX_COMMAND_STR_LEN);

      if ( firewall_entry->filter_spec.next_prot_hdr.udp.field_mask &
           IPFLTR_MASK_UDP_SRC_PORT )
      {
        min_sport = firewall_entry->filter_spec.next_prot_hdr.udp.src.port;
        max_sport = firewall_entry->filter_spec.next_prot_hdr.udp.src.port +
                    firewall_entry->filter_spec.next_prot_hdr.udp.src.range;
        check_sport = true;
      }

      if ( firewall_entry->filter_spec.next_prot_hdr.udp.field_mask & IPFLTR_MASK_UDP_DST_PORT )
      {
        min_dport = firewall_entry->filter_spec.next_prot_hdr.udp.dst.port;
        max_dport = firewall_entry->filter_spec.next_prot_hdr.udp.dst.port +
                    firewall_entry->filter_spec.next_prot_hdr.udp.dst.range;
        check_dport = true;
      }

      break;

    case PS_IPPROTO_ICMP6:
      /* For icmp protocol, we do not have src or dest ports */
      strlcat(command,"| grep icmpv6 ",MAX_COMMAND_STR_LEN);

      break;
  }

  if ( firewall_entry->filter_spec.ip_hdr.v6.field_mask & IPFLTR_MASK_IP6_SRC_ADDR)
  {
    prefixWords_src_addr = firewall_entry->filter_spec.ip_hdr.v6.src.prefix_len/(sizeof(uint16_t)*8);
    prefixBits_src_addr = firewall_entry->filter_spec.ip_hdr.v6.src.prefix_len%(sizeof(uint16_t)*8);

    memcpy (&curr_src_ipv6_addr.s6_addr ,
            firewall_entry->filter_spec.ip_hdr.v6.src.addr.in6_u.u6_addr16,
            QCMAP_MSGR_IPV6_ADDR_LEN_V01);

    if (prefixWords_src_addr < 8)
    {
      prefixbitvalue_src_addr = curr_src_ipv6_addr.s6_addr[prefixWords_src_addr] >> (16 - prefixBits_src_addr);
    }

    processV6srcaddr = true;
  }

  if ( firewall_entry->filter_spec.ip_hdr.v6.field_mask & IPFLTR_MASK_IP6_DST_ADDR)
  {
    prefixWords_dest_addr = firewall_entry->filter_spec.ip_hdr.v6.dst.prefix_len/(sizeof(uint16_t)*8);
    prefixBits_dest_addr = firewall_entry->filter_spec.ip_hdr.v6.dst.prefix_len%(sizeof(uint16_t)*8);

    memcpy (&curr_dest_ipv6_addr.s6_addr ,
            firewall_entry->filter_spec.ip_hdr.v6.dst.addr.in6_u.u6_addr16,
            QCMAP_MSGR_IPV6_ADDR_LEN_V01);

    if (prefixWords_dest_addr < 8)
    {
      prefixbitvalue_dest_addr = curr_dest_ipv6_addr.s6_addr[prefixWords_dest_addr] >> (16 - prefixBits_dest_addr);
    }

    processV6destaddr = true;
  }


  snprintf(tmp, MAX_COMMAND_STR_LEN, " > %s", IPv6_CONNTRACK_FILTER_PATH);
  strlcat(command, tmp, MAX_COMMAND_STR_LEN);
  memset(tmp,0,MAX_COMMAND_STR_LEN);
  ds_system_call(command, strlen(command));

  fd = fopen(IPv6_CONNTRACK_FILTER_PATH,"r");


  if(fd == NULL)
  {
    LOG_MSG_ERROR("FetchPortInfoFromV6Conntrack - Error in opening ",0,0,0);
    *qmi_err_num = QMI_ERR_INVALID_ARG_V01;
    return;
  }

  while(fgets( stringline, bufferSize, fd) != NULL)
  {

    protocol = strtok_r(stringline, " ", &ptr);

    saddr = strtok_r(NULL, " ", &ptr);
    daddr = strtok_r(NULL, " ", &ptr);

    if(saddr)
      *(saddr + 3) = ' ';

    if(daddr)
      *(daddr + 3) = ' ';

    //Copy to temp str then pton and then check prefixword byte value after right shifting
    if (processV6srcaddr)
    {
      if(saddr)
        inet_pton(AF_INET6, saddr + 4, (void *)&tmp_src_ipv6);
      if (memcmp (&curr_src_ipv6_addr.s6_addr16, &tmp_src_ipv6.s6_addr16,
                  (sizeof (uint16_t))*prefixWords_src_addr) != 0)
      {
        goto delete_entry_v6;
      }

      if ((prefixWords_src_addr < 8) &&
           (tmp_src_ipv6.s6_addr16[prefixWords_src_addr] >> ((sizeof(uint16_t)*8)-prefixBits_src_addr) != prefixbitvalue_src_addr))
      {
        goto delete_entry_v6;
      }
    }

    //Copy to temp str then pton and then check prefixword byte value after right shifting
    if (processV6destaddr)
    {
      if(daddr)
        inet_pton(AF_INET6, daddr + 4, (void *)&tmp_dest_ipv6);
      if (memcmp (&curr_dest_ipv6_addr.s6_addr16, &tmp_dest_ipv6.s6_addr16,
                  (sizeof (uint16_t))*prefixWords_dest_addr) != 0)
      {
        goto delete_entry_v6;
      }

      if ((prefixWords_dest_addr < 8) &&
           (tmp_dest_ipv6.s6_addr16[prefixWords_dest_addr] >> ((sizeof(uint16_t)*8)-prefixBits_dest_addr) != prefixbitvalue_dest_addr))
      {
        goto delete_entry_v6;
      }
    }

    if( protocol_num != PS_IPPROTO_ICMP6 )
    {
      sport = strtok_r(NULL, " ", &ptr);

      if(sport)
        *(sport + 5) = ' ';

      dport = strtok_r(NULL, " ", &ptr);

      if(dport)
        *(dport + 5) = ' ';

      if (check_sport)
      {
        if(sport)
          port = ds_atoi((sport+6));
        if (port < min_sport || port > max_sport)
          goto delete_entry_v6;
      }

      if (check_dport)
      {
        if(dport)
          port = ds_atoi((dport+6));
        if (port < min_dport || port > max_dport)
          goto delete_entry_v6;
      }
    }

    /*check protocol*/
    if (firewall_entry->filter_spec.ip_hdr.v6.next_hdr_prot == PS_IPPROTO_TCP_UDP ||
        firewall_entry->filter_spec.ip_hdr.v6.next_hdr_prot == protocol_num ||
        firewall_entry->filter_spec.ip_hdr.v6.next_hdr_prot == 0)
      continue;

    //Delete contrack entry
    delete_entry_v6:
    memset(tmp,0,MAX_COMMAND_STR_LEN);

    snprintf(command, MAX_COMMAND_STR_LEN, "conntrack -D -f ipv6");
    if(protocol)
    {
      snprintf(tmp, MAX_COMMAND_STR_LEN, " -p %s", protocol);
      strlcat(command,tmp,MAX_COMMAND_STR_LEN);
      memset(tmp,0,MAX_COMMAND_STR_LEN);
    }
    if(saddr)
    {
      snprintf(tmp, MAX_COMMAND_STR_LEN, "  --orig-%s", saddr);
      strlcat(command,tmp,MAX_COMMAND_STR_LEN);
      memset(tmp,0,MAX_COMMAND_STR_LEN);
    }
    if(daddr)
    {
      snprintf(tmp, MAX_COMMAND_STR_LEN, "  --orig-%s", daddr);
      strlcat(command,tmp,MAX_COMMAND_STR_LEN);
      memset(tmp,0,MAX_COMMAND_STR_LEN);
    }
    if(sport)
    {
      snprintf(tmp, MAX_COMMAND_STR_LEN, " --%s", sport);
      strlcat(command,tmp,MAX_COMMAND_STR_LEN);
      memset(tmp,0,MAX_COMMAND_STR_LEN);
    }
    if(dport)
    {
      snprintf(tmp, MAX_COMMAND_STR_LEN, " --%s", dport);
      strlcat(command,tmp,MAX_COMMAND_STR_LEN);
      memset(tmp,0,MAX_COMMAND_STR_LEN);
    }
    ds_system_call(command, strlen(command));
    memset(command,0,MAX_COMMAND_STR_LEN);

  }

  fclose(fd);

  snprintf(command,MAX_COMMAND_STR_LEN,"rm -rf %s",IPv6_CONNTRACK_FILTER_PATH);
  ds_system_call(command, strlen(command));
}
/*===========================================================================
  FUNCTION DeleteConntrackEntryForDropIPv4FirewallEntries()
==========================================================================*/
/*!
@brief
  Delete the client conntrack when we are adding a DROP firewall entry for
  that IPv4 address and port combination

@parameters
  firewall_entry - firewall entry to add
  protocol_num   - protocol number

@return
  bool

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/

bool QCMAP_LAN_Client::DeleteConntrackEntryForDropIPv4FirewallEntries
(
  qcmap_msgr_firewall_entry_conf_t *firewall_entry,
  uint8_t protocol_num,
  qmi_error_type_v01 *qmi_err_num
)
{
  char *protocol=NULL,*saddr=NULL, *sport=NULL, *dport=NULL, *ptr;
  char command[MAX_COMMAND_STR_LEN];
  char tmp[MAX_COMMAND_STR_LEN]={0};
  FILE *fd = NULL;
  char *line = NULL;
  size_t len = 0;
  int read;
  bool check_ip = false;
  bool check_sport = false;
  bool check_dport = false;
  uint16_t min_sport;
  uint16_t max_sport;
  uint16_t min_dport = 0;
  uint16_t max_dport = 0;
  uint16_t port = 0;
  uint32_t subnet_mask, ip_addr;

  if (firewall_entry == NULL)
  {
    LOG_MSG_ERROR("NULL Arguments passed",0,0,0);
    *qmi_err_num = QMI_ERR_INVALID_ARG_V01;
    return false;
  }

  if (protocol_num != PS_IPPROTO_TCP && protocol_num != PS_IPPROTO_UDP && protocol_num != PS_IPPROTO_ICMP)
  {
    if ( protocol_num == PS_IPPROTO_TCP_UDP)
    {
      DeleteConntrackEntryForDropIPv4FirewallEntries(firewall_entry,PS_IPPROTO_TCP, qmi_err_num);
      DeleteConntrackEntryForDropIPv4FirewallEntries(firewall_entry,PS_IPPROTO_UDP, qmi_err_num);
    }
    return true;
  }

  /*The logic is to grep for the conntrack entries using firewall params
  so as to reduce the number of conntrack entries to be checked to a
  minimum value. We write these entries to a txt file and then read the
  text file, check if the entry needs to be deleted and then delete the entry*/

  /*We only need certain columns of conntrack entries for deletion,
  so we cut those particular columns*/

  if ( protocol_num == PS_IPPROTO_UDP)
    snprintf(command, MAX_COMMAND_STR_LEN,"conntrack -L | cut -f1,9,11,12 -d ' ' ");
  else if ( protocol_num == PS_IPPROTO_TCP)
    snprintf(command, MAX_COMMAND_STR_LEN,"conntrack -L | cut -f1,10,12,13 -d ' ' ");
  else if (protocol_num == PS_IPPROTO_ICMP)
    snprintf(command, MAX_COMMAND_STR_LEN, "conntrack -L | cut -f1,8,9 -d ' ' ");

  if ( firewall_entry->filter_spec.ip_hdr.v4.field_mask & IPFLTR_MASK_IP4_SRC_ADDR )
  {
    snprintf(tmp, MAX_COMMAND_STR_LEN,"| grep src=");
    strlcat(command,tmp,MAX_COMMAND_STR_LEN);
    memset(tmp,0,MAX_COMMAND_STR_LEN);

    subnet_mask = ntohl(firewall_entry->filter_spec.ip_hdr.v4.src.subnet_mask.ps_s_addr);

    ip_addr = ntohl(firewall_entry->filter_spec.ip_hdr.v4.src.addr.ps_s_addr);

    if (subnet_mask < 0xff000000)
      check_ip = true;

    /*We need to grep according to the subnet mask. If subnet mask
    is 255.0.0.0 / 255.255.0.0 / 255.255.255.0 / 255.255.255.255 ,
    then we can grep straight-away for the match. Otherwise, we grep for
    the minimum pattern that matches perfectly and check later whether
    we need to delete that conntrack entry or not*/

    else if ((subnet_mask >= 0xff000000) && (subnet_mask < 0xffff0000))
    {
      snprintf(tmp, MAX_COMMAND_STR_LEN,"%d. ",(ip_addr & 0xff000000) >> 24);
      strlcat(command,tmp,MAX_COMMAND_STR_LEN);
      memset(tmp,0,MAX_COMMAND_STR_LEN);

      if (subnet_mask != 0xff000000)
        check_ip = true;
    }

    else if ((subnet_mask >= 0xffff0000) && (subnet_mask < 0xffffff00))
    {
      snprintf(tmp, MAX_COMMAND_STR_LEN,"%d.%d. ",
               (ip_addr & 0xff000000)>>24,
               (ip_addr & 0x00ff0000) >> 16);
      strlcat(command,tmp,MAX_COMMAND_STR_LEN);
      memset(tmp,0,MAX_COMMAND_STR_LEN);

      if (subnet_mask != 0xffff0000)
        check_ip = true;
    }

    else if ((subnet_mask >= 0xffffff00) && (subnet_mask < 0xffffffff))
    {
      snprintf(tmp, MAX_COMMAND_STR_LEN,"%d.%d.%d. ",
               (ip_addr & 0xff000000)>>24,
               (ip_addr & 0x00ff0000) >> 16,
               (ip_addr & 0x0000ff00) >> 8);
      strlcat(command,tmp,MAX_COMMAND_STR_LEN);
      memset(tmp,0,MAX_COMMAND_STR_LEN);

      if (subnet_mask != 0xffffff00)
        check_ip = true;
    }
    else if (subnet_mask == 0xffffffff)
    {
      readable_addr(AF_INET,&(firewall_entry->filter_spec.ip_hdr.v4.src.addr.ps_s_addr),tmp);
      strlcat(tmp," ",MAX_COMMAND_STR_LEN);
      strlcat(command,tmp,MAX_COMMAND_STR_LEN);
      memset(tmp,0,MAX_COMMAND_STR_LEN);

      if (subnet_mask != 0xffffffff)
        check_ip = true;
    }

  }

  switch(protocol_num)
  {
    case PS_IPPROTO_TCP:

      strlcat(command,"| grep tcp ",MAX_COMMAND_STR_LEN);

      /*If sport / dport are defined, we use them directly, or else we set a flag
      to check the sport and dport later and delete conntrack entry only
      if matches the criterion*/

      if ( firewall_entry->filter_spec.next_prot_hdr.tcp.field_mask
           & IPFLTR_MASK_TCP_SRC_PORT )
      {
        if( firewall_entry->filter_spec.next_prot_hdr.tcp.src.range !=0 )
        {
          min_sport = firewall_entry->filter_spec.next_prot_hdr.tcp.src.port;
          max_sport = firewall_entry->filter_spec.next_prot_hdr.tcp.src.port +
                      firewall_entry->filter_spec.next_prot_hdr.tcp.src.range;
          check_sport = true;
        }
        else
        {
          snprintf(tmp,MAX_COMMAND_STR_LEN,"| grep sport=%d ",
                   firewall_entry->filter_spec.next_prot_hdr.tcp.src.port);
        }
        strlcat(command,tmp,MAX_COMMAND_STR_LEN);
        memset(tmp,0,MAX_COMMAND_STR_LEN);
      }

      if ( firewall_entry->filter_spec.next_prot_hdr.tcp.field_mask & IPFLTR_MASK_TCP_DST_PORT )
      {
        if ( firewall_entry->filter_spec.next_prot_hdr.tcp.dst.range !=0 )
        {
          min_dport = firewall_entry->filter_spec.next_prot_hdr.tcp.dst.port;
          max_dport = firewall_entry->filter_spec.next_prot_hdr.tcp.dst.port +
                      firewall_entry->filter_spec.next_prot_hdr.tcp.dst.range;
          check_dport = true;
        }else
        {
          snprintf(tmp,MAX_COMMAND_STR_LEN,"| grep dport=%d ",
                   firewall_entry->filter_spec.next_prot_hdr.tcp.dst.port);
        }
        strlcat(command,tmp,MAX_COMMAND_STR_LEN);
        memset(tmp,0,MAX_COMMAND_STR_LEN);
      }

      break;

    case PS_IPPROTO_UDP:

      strlcat(command,"| grep udp ",MAX_COMMAND_STR_LEN);

      if ( firewall_entry->filter_spec.next_prot_hdr.udp.field_mask &
           IPFLTR_MASK_UDP_SRC_PORT )
      {
        if ( firewall_entry->filter_spec.next_prot_hdr.udp.src.range !=0 )
        {
          min_sport = firewall_entry->filter_spec.next_prot_hdr.udp.src.port;
          max_sport = firewall_entry->filter_spec.next_prot_hdr.udp.src.port +
                      firewall_entry->filter_spec.next_prot_hdr.udp.src.range;
          check_sport = true;
        }else
        {
          snprintf(tmp,MAX_COMMAND_STR_LEN,"| grep sport=%d ",
                   firewall_entry->filter_spec.next_prot_hdr.udp.src.port );
        }
        strlcat(command,tmp,MAX_COMMAND_STR_LEN);
        memset(tmp,0,MAX_COMMAND_STR_LEN);
      }

      if ( firewall_entry->filter_spec.next_prot_hdr.udp.field_mask & IPFLTR_MASK_UDP_DST_PORT )
      {
        if ( firewall_entry->filter_spec.next_prot_hdr.udp.dst.range !=0 )
        {
          min_dport = firewall_entry->filter_spec.next_prot_hdr.udp.dst.port;
          max_dport = firewall_entry->filter_spec.next_prot_hdr.udp.dst.port +
                      firewall_entry->filter_spec.next_prot_hdr.udp.dst.range;
          check_dport = true;
        }else
        {
          snprintf(tmp,MAX_COMMAND_STR_LEN,"| grep dport=%d ",
                   firewall_entry->filter_spec.next_prot_hdr.udp.dst.port );
        }
        strlcat(command,tmp,MAX_COMMAND_STR_LEN);
        memset(tmp,0,MAX_COMMAND_STR_LEN);
      }
      break;

    case PS_IPPROTO_ICMP:
      /* For icmp protocol, we do not have src or dest ports */
      strlcat(command,"| grep icmp ",MAX_COMMAND_STR_LEN);

      break;
  }

  snprintf(tmp, MAX_COMMAND_STR_LEN,"> %s",CONNTRACK_ENTRIES);
  strlcat(command,tmp,MAX_COMMAND_STR_LEN);
  LOG_MSG_INFO1("Conntrack command : %s",command,0,0);
  ds_system_call(command, strlen(command));
  memset(tmp,0,MAX_COMMAND_STR_LEN);
  memset(command,0,MAX_COMMAND_STR_LEN);

  /*Open the conntrack.txt file to fetch protocol,saddr,dport*/
  fd = fopen(CONNTRACK_ENTRIES,"r");
  if(fd == NULL)
  {
    LOG_MSG_ERROR("DeleteConntrackEntries - Error in opening %s",
                  CONNTRACK_ENTRIES,0,0);
    *qmi_err_num = QMI_ERR_INTERNAL_V01;
    return false;
  }

  /*Reset the file ptr to start of file*/
  if (fseek(fd,0,SEEK_SET)!=0)
  {
    fclose (fd);
    LOG_MSG_ERROR("File pointer not reset to beginning of file\n",0,0,0);
    return false;
  }
  /*Read the file line-by-line and delete the entry if required*/
  while (((read = getline(&line, &len, fd)) != -1))
  {
    protocol = strtok_r(line, " ", &ptr);

    saddr = strtok_r(NULL, " ", &ptr);
    if(saddr)
      *(saddr + 3) = ' ';

    /*Check for subnet mask*/
    if (check_ip)
    {
      if(saddr)
        inet_pton(AF_INET, saddr+4, (void *)&ip_addr);
      if ((ip_addr & firewall_entry->filter_spec.ip_hdr.v4.src.subnet_mask.ps_s_addr) !=
          (firewall_entry->filter_spec.ip_hdr.v4.src.addr.ps_s_addr & firewall_entry->filter_spec.ip_hdr.v4.src.subnet_mask.ps_s_addr))
        continue;
    }

    //We dont have sport and dport in ICMP
    if(protocol_num != PS_IPPROTO_ICMP)
    {
      sport = strtok_r(NULL, " ", &ptr);
      if(sport)
        *(sport + 5) = ' ';

      dport = strtok_r(NULL, " ", &ptr);
      if(dport)
        *(dport + 5) = ' ';
      /*Check for source port range*/
      if (check_sport)
      {
        if(sport)
          port = ds_atoi((sport+6));
        if (port < min_sport || port > max_sport)
          continue;
      }

      /*Check for destination port range*/
      if (check_dport)
      {
        if(dport)
          port = ds_atoi((dport+6));
        if (port < min_dport || port > max_dport)
          continue;
      }
    }


    memset(tmp,0,MAX_COMMAND_STR_LEN);

    /*Delete contrack entry*/
    snprintf(command, MAX_COMMAND_STR_LEN, "conntrack -D");
    if(protocol)
    {
      snprintf(tmp, MAX_COMMAND_STR_LEN, " -p %s", protocol);
      strlcat(command,tmp,MAX_COMMAND_STR_LEN);
      memset(tmp,0,MAX_COMMAND_STR_LEN);
    }
    if(saddr)
    {
      snprintf(tmp, MAX_COMMAND_STR_LEN, "  --orig-%s", saddr);
      strlcat(command,tmp,MAX_COMMAND_STR_LEN);
      memset(tmp,0,MAX_COMMAND_STR_LEN);
    }
    if(sport)
    {
      snprintf(tmp, MAX_COMMAND_STR_LEN, " --%s", sport);
      strlcat(command,tmp,MAX_COMMAND_STR_LEN);
      memset(tmp,0,MAX_COMMAND_STR_LEN);
    }
    if(dport)
    {
      snprintf(tmp, MAX_COMMAND_STR_LEN, " --%s", dport);
      strlcat(command,tmp,MAX_COMMAND_STR_LEN);
      memset(tmp,0,MAX_COMMAND_STR_LEN);
    }
    ds_system_call(command, strlen(command));
    memset(command,0,MAX_COMMAND_STR_LEN);

  }

  fclose(fd);

  snprintf(command,MAX_COMMAND_STR_LEN,"rm -rf %s",CONNTRACK_ENTRIES);
  ds_system_call(command, strlen(command));

  return true;
}

/*======================================================
  FUNCTION:  DeleteConntrackEntryForAcceptIPv4FirewallEntries
  =====================================================*/
  /*!
      @brief
      Delete the client conntrack when we are adding a ACCEPT firewall entry for
      that IPv4 address and port combination

      @params
         IPv6 address
         Protocol
         UDP Port start number
         UDP Port Range
         TCP Port start number
         TCP Port Range

      @return
      false - failure
      true - success
  */
/*====================================================*/
bool QCMAP_LAN_Client::DeleteConntrackEntryForAcceptIPv4FirewallEntries
(
  qcmap_msgr_firewall_entry_conf_t *firewall_entry,
  uint8_t protocol_num,
  qmi_error_type_v01 *qmi_err_num
  
)
{
  char command[MAX_COMMAND_STR_LEN]={0};
  char tmp[MAX_COMMAND_STR_LEN]={0};
  char result[QCMAP_MAX_SCAN_SIZE]={0};
  int read;
  bool check_sport = false;
  bool check_dport = false;
  bool check_ip = false;
  uint16_t min_sport = 0;
  uint16_t max_sport = 0;
  uint16_t min_dport = 0;
  uint16_t max_dport = 0;
  uint16_t port = 0;
  uint16_t no_of_vlans;
  uint16_t vlan_id;
  char *line = NULL;
  size_t len = 0;
  FILE *fd = NULL;
  uint32_t ip_addr;
  char *protocol=NULL,*saddr=NULL, *sport=NULL, *dport=NULL, *ptr;
  struct in_addr addr;
  char vlan_ip[QCMAP_LAN_MAX_IPV4_ADDR_SIZE]={0};
  char apps_ip[QCMAP_LAN_MAX_IPV4_ADDR_SIZE]={0};

  LOG_MSG_INFO1("DeleteConntrackEntryForAcceptIPv4FirewallEntries \n",0,0,0);

  if( firewall_entry == NULL )
  {
    LOG_MSG_ERROR("NULL firewall_entry\n",0,0,0);
    return false;
  }

  if ( firewall_entry->filter_spec.ip_hdr.v4.field_mask & IPFLTR_MASK_IP4_SRC_ADDR )
  {
    ip_addr = ntohl(firewall_entry->filter_spec.ip_hdr.v4.src.addr.ps_s_addr);
    check_ip = true;
  }

  switch(protocol_num)
  {
    case PS_IPPROTO_TCP:

      snprintf(tmp, MAX_COMMAND_STR_LEN, "| grep tcp ");

      if ( firewall_entry->filter_spec.next_prot_hdr.tcp.field_mask
           & IPFLTR_MASK_TCP_SRC_PORT )
      {
        min_sport = firewall_entry->filter_spec.next_prot_hdr.tcp.src.port;
        max_sport = firewall_entry->filter_spec.next_prot_hdr.tcp.src.port +
                    firewall_entry->filter_spec.next_prot_hdr.tcp.src.range;
        check_sport = true;
      }

      if ( firewall_entry->filter_spec.next_prot_hdr.tcp.field_mask & IPFLTR_MASK_TCP_DST_PORT )
      {
        min_dport = firewall_entry->filter_spec.next_prot_hdr.tcp.dst.port;
        max_dport = firewall_entry->filter_spec.next_prot_hdr.tcp.dst.port +
                    firewall_entry->filter_spec.next_prot_hdr.tcp.dst.range;
        check_dport = true;
      }

      break;

    case PS_IPPROTO_UDP:

      snprintf(tmp, MAX_COMMAND_STR_LEN, "| grep udp ");

      if ( firewall_entry->filter_spec.next_prot_hdr.udp.field_mask &
           IPFLTR_MASK_UDP_SRC_PORT )
      {
        min_sport = firewall_entry->filter_spec.next_prot_hdr.udp.src.port;
        max_sport = firewall_entry->filter_spec.next_prot_hdr.udp.src.port +
                    firewall_entry->filter_spec.next_prot_hdr.udp.src.range;
        check_sport = true;
      }

      if ( firewall_entry->filter_spec.next_prot_hdr.udp.field_mask & IPFLTR_MASK_UDP_DST_PORT )
      {
        min_dport = firewall_entry->filter_spec.next_prot_hdr.udp.dst.port;
        max_dport = firewall_entry->filter_spec.next_prot_hdr.udp.dst.port +
                    firewall_entry->filter_spec.next_prot_hdr.udp.dst.range;
        check_dport = true;
      }
      break;

    case PS_IPPROTO_ICMP:
      /* For icmp protocol, we do not have src or dest ports */
      strlcat(command,"| grep icmp ",MAX_COMMAND_STR_LEN);
      break;

    default:
      LOG_MSG_ERROR("Protocol is not valid",0,0,0);
      return false;
  }

  if ( firewall_entry->filter_spec.ip_hdr.v4.field_mask & IPFLTR_MASK_IP4_SRC_ADDR )
  {
    check_ip = true;
  }

  if ( QCMAP_LAN_Client::UciGetUtility(owrt_filename[0], "no_of_configs", 0, "no_of_vlans", result, 0) )
  {
    no_of_vlans=atoi(result);
  }
  else
  {
    /* In the event ucigetutility returns false */
    LOG_MSG_ERROR(" Failed to retrieve no_of_vlans value from uci db", 0, 0, 0);
    *qmi_err_num = QMI_ERR_INTERNAL_V01;
    return false;
  }

  for(int i=0; i<=no_of_vlans; i++)
  {
    memset(command,0,MAX_COMMAND_STR_LEN);
    memset(result,0,QCMAP_MAX_SCAN_SIZE);
    memset(vlan_ip,0,QCMAP_LAN_MAX_IPV4_ADDR_SIZE);
    if( i == 0 )
    {
      snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh get network.lan.ipaddr");
      if(!ExecuteSystemCmd((const char *)command,result,sizeof(result)))
      {
        LOG_MSG_ERROR("Failed to execute command : %s",command,0,0);
        return false;
      }
      strlcpy(vlan_ip,result,QCMAP_LAN_MAX_IPV4_ADDR_SIZE);
    }
    else
    {
      if ( QCMAP_LAN_Client::UciGetUtility(owrt_filename[0], "vlan", 0, "vlan_id", result, i-1) )
      {
        vlan_id=atoi(result);
      }
      else
      {
        /* In the event ucigetutility function returns false */
        LOG_MSG_ERROR("Failed to get vlan_id's from uci db", 0, 0, 0);
        *qmi_err_num = QMI_ERR_INTERNAL_V01;
        return false;
      }

      memset(result,0,QCMAP_MAX_SCAN_SIZE);

      snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh get network.lan%d.ipaddr",vlan_id);
      if(!ExecuteSystemCmd((const char *)command,result,sizeof(result)))
      {
        LOG_MSG_ERROR("Failed to execute command : %s",command,0,0);
        return false;
      }
      strlcpy(vlan_ip,result,QCMAP_LAN_MAX_IPV4_ADDR_SIZE);
    }
    //we shouldn't grep the conntracks generated by bridge.
    if(protocol_num == PS_IPPROTO_UDP)
      snprintf(command, MAX_COMMAND_STR_LEN,
               "conntrack -L | cut -f1,9,11,12 -d ' ' | grep -v -w src=%s ", vlan_ip);
    else if(protocol_num == PS_IPPROTO_TCP)
      snprintf(command, MAX_COMMAND_STR_LEN,
               "conntrack -L | cut -f1,10,12,13 -d ' ' | grep -v -w src=%s ", vlan_ip);
    else if (protocol_num == PS_IPPROTO_ICMP)
      snprintf(command, MAX_COMMAND_STR_LEN,
               "conntrack -L | cut -f1,8,9 -d ' ' | grep -v -w src=%s ", vlan_ip);

    strlcat(command, tmp, MAX_COMMAND_STR_LEN);

    memset(tmp, 0, MAX_COMMAND_STR_LEN);
    snprintf(tmp, MAX_COMMAND_STR_LEN, " > %s", CONNTRACK_ENTRIES);
    strlcat(command, tmp, MAX_COMMAND_STR_LEN);
    LOG_MSG_INFO1("Conntrack command : %s", command, 0, 0);
    ds_system_call(command, strlen(command));
  }

  /*Open the conntrack.txt file to fetch protocol,saddr,dport*/
  fd = fopen(CONNTRACK_ENTRIES,"r");
  if(fd == NULL)
  {
    LOG_MSG_ERROR("DeleteConntrackEntries - Error in opening %s",
                  CONNTRACK_ENTRIES,0,0);
    return false;
  }

  /*Reset the file ptr to start of file*/
  if (fseek(fd,0,SEEK_SET)!=0)
  {
    LOG_MSG_ERROR("File pointer not reset to beginning of file\n",0,0,0);
    return false;
  }

  /*Read the file line-by-line and delete the entry if required*/
  while (((read = getline(&line, &len, fd)) != -1))
  {
    protocol = strtok_r(line, " ", &ptr);

    saddr = strtok_r(NULL, " ", &ptr);

    if(saddr)
      *(saddr + 3) = ' ';

    /*Check for subnet mask*/
    if (check_ip)
    {
      if(saddr)
        inet_pton(AF_INET, saddr+4, (void *)&ip_addr);
      if ((ip_addr & firewall_entry->filter_spec.ip_hdr.v4.src.subnet_mask.ps_s_addr) !=
          (firewall_entry->filter_spec.ip_hdr.v4.src.addr.ps_s_addr & firewall_entry->filter_spec.ip_hdr.v4.src.subnet_mask.ps_s_addr))
        goto delete_entry;
    }

    if(protocol_num != PS_IPPROTO_ICMP)
    {
      sport = strtok_r(NULL, " ", &ptr);

      if(sport)
        *(sport + 5) = ' ';

      dport = strtok_r(NULL, " ", &ptr);

      if(dport)
        *(dport + 5) = ' ';

      /*Check for source port range*/
      if (check_sport)
      {
        if(sport)
          port = ds_atoi((sport+6));
        if (port < min_sport || port > max_sport)
          goto delete_entry;
      }

      /*Check for destination port range*/
      if (check_dport)
      {
        if(dport)
          port = ds_atoi((dport+6));
        if (port < min_dport || port > max_dport)
          goto delete_entry;
      }
    }

    /*check protocol*/
    if (firewall_entry->filter_spec.ip_hdr.v4.next_hdr_prot == PS_IPPROTO_TCP_UDP ||
        firewall_entry->filter_spec.ip_hdr.v4.next_hdr_prot == protocol_num ||
        firewall_entry->filter_spec.ip_hdr.v4.next_hdr_prot == 0)
      continue;

    /*Delete contrack entry*/
    delete_entry:
    memset(tmp,0,MAX_COMMAND_STR_LEN);

    snprintf(command, MAX_COMMAND_STR_LEN, "conntrack -D");
    if(protocol)
    {
      snprintf(tmp, MAX_COMMAND_STR_LEN, " -p %s", protocol);
      strlcat(command,tmp,MAX_COMMAND_STR_LEN);
      memset(tmp,0,MAX_COMMAND_STR_LEN);
    }
    if(saddr)
    {
      snprintf(tmp, MAX_COMMAND_STR_LEN, "  --orig-%s", saddr);
      strlcat(command,tmp,MAX_COMMAND_STR_LEN);
      memset(tmp,0,MAX_COMMAND_STR_LEN);
    }
    if(sport)
    {
      snprintf(tmp, MAX_COMMAND_STR_LEN, " --%s", sport);
      strlcat(command,tmp,MAX_COMMAND_STR_LEN);
      memset(tmp,0,MAX_COMMAND_STR_LEN);
    }
    if(dport)
    {
      snprintf(tmp, MAX_COMMAND_STR_LEN, " --%s", dport);
      strlcat(command,tmp,MAX_COMMAND_STR_LEN);
      memset(tmp,0,MAX_COMMAND_STR_LEN);
    }
    ds_system_call(command, strlen(command));
    memset(command,0,MAX_COMMAND_STR_LEN);

  }

  fclose(fd);

  snprintf(command,MAX_COMMAND_STR_LEN,"rm -rf %s",CONNTRACK_ENTRIES);
  ds_system_call(command, strlen(command));

  return true;
}

/*===========================================================================
  FUNCTION SetFirewall
  ===========================================================================*/
/*!
  @brief
  Sets the Firewall Config with all params

  @return
  true  - on Success
  false - on Failure

  - Dependencies
  - None

  - Side Effects
  - None
 */
/*=========================================================================*/
boolean QCMAP_LAN_Client::SetFirewall
(
  boolean             enable_firewall,
  boolean             pkts_allowed,
  uint32_t            wan_profile_handle,
  qmi_error_type_v01 *qmi_err_num
)
{
  if ( wan_profile_handle==0 )
  {
    LOG_MSG_ERROR("Invalid wan profile handle passed ",0,0,0);
    *qmi_err_num = QMI_ERR_INVALID_HANDLE_V01;
    return false;
  }

  IPv4_DISABLE_NAT_EXCLUSIVITY_CHECK(wan_profile_handle, qmi_err_num, false);

  char command[QCMAP_MAX_COMMAND_LEN] = {0};
  snprintf(command, QCMAP_MAX_COMMAND_LEN,"%s %s %d %d %d",FIREWALL_CONFIG_FILE,SET_FIREWALL,enable_firewall,pkts_allowed,wan_profile_handle);
  ds_system_call(command, strlen(command));
  LOG_MSG_INFO1("setting firewall done",0,0,0);
  return true;
}

/*===========================================================================
  FUNCTION GetFirewall
  ===========================================================================*/
/*!
  @brief
  Gets the Firewall Config

  @return
  true  - on Success
  false - on Failure

  @note

  - Dependencies
  - None

  - Side Effects
  - None
 */
/*=========================================================================*/
boolean QCMAP_LAN_Client::GetFirewall
(
  uint32_t               wan_profile_handle,
  boolean                *enable_firewall,
  boolean                *pkts_allowed,
  qmi_error_type_v01 *qmi_err_num
)
{
  char command[QCMAP_MAX_COMMAND_LEN] = {0};
  char result[QCMAP_MAX_SCAN_SIZE] = {0};
  int no_of_profiles;
  uint32_t profile_id;

  if (wan_profile_handle == 0)
  {
    LOG_MSG_ERROR("Invalid wan profile handle passed ",0,0,0);
    *qmi_err_num = QMI_ERR_INVALID_PROFILE_V01;
    return false;
  }

  if(enable_firewall == NULL || pkts_allowed == NULL)
  {
    LOG_MSG_ERROR("Pointer to enable firewall or Default Policy is NULL",0,0,0);
    *qmi_err_num = QMI_ERR_INVALID_ARG_V01;
    return false;
  }

  /* Getting the total number of wan profiles */
  snprintf(command, MAX_COMMAND_STR_LEN,"%s %s", UCI_GET_COMMAND, UCI_QUERY_NO_OF_WWAN_PROFILES);

  if (!ExecuteSystemCmd((const char *)command,result,sizeof(result)))
  {
    LOG_MSG_ERROR("Failed to execute command : %s",command,0,0);
    *qmi_err_num = QMI_ERR_INTERNAL_V01;
    return false;
  }
  no_of_profiles = atoi(result);

  /* Looping through all the profiles and fetching firewall details
     from the current profile */
  for(int i=0; i<no_of_profiles; i++)
  {
    memset(command,0,QCMAP_MAX_COMMAND_LEN);
    memset(result,0,QCMAP_MAX_SCAN_SIZE);
    snprintf(command, QCMAP_MAX_COMMAND_LEN, "/etc/data/uci_ex.sh get qcmap_lan.@profile[%d].profile_id",i);
    if(!ExecuteSystemCmd((const char *)command,result,sizeof(result)))
    {
      LOG_MSG_ERROR("Failed to execute command : %s",command,0,0);
      *qmi_err_num = QMI_ERR_INTERNAL_V01;
      return false;
    }
    profile_id=atoi(result);

    if(profile_id == wan_profile_handle)
    {
      snprintf(command, QCMAP_MAX_COMMAND_LEN, "/etc/data/uci_ex.sh get qcmap_lan.@profile[%d].firewall_enabled",i);
      if (!ExecuteSystemCmd(command, result, sizeof(result)))
      {
        LOG_MSG_ERROR("failed to execute command : %s",command,0,0);
        *qmi_err_num = QMI_ERR_INTERNAL_V01;
        return false;
      }
      if(atoi(result) == 1)
      {
        *enable_firewall = true;
      }
      else
        *enable_firewall = false;

      memset(result,0,QCMAP_MAX_SCAN_SIZE);
      snprintf(command, QCMAP_MAX_COMMAND_LEN, "/etc/data/uci_ex.sh get qcmap_lan.@profile[%d].pkts_allowed",i);
      if (!ExecuteSystemCmd(command, result, sizeof(result)))
      {
        LOG_MSG_ERROR("failed to execute command : %s",command,0,0);
        *qmi_err_num = QMI_ERR_INTERNAL_V01;
        return false;
      }
      if(atoi(result) == 1)
      {
        *pkts_allowed = true;
      }
      else
        *pkts_allowed = false;
      return true;
    }

  }
  return false;
}


/*===========================================================================
  FUNCTION CheckForDuplicateFirewallRule
  ===========================================================================*/
/*!
  @brief
  Check For Duplicate Firewall Rule

  @return
   true  on success.
   false on failure
  @note

  - Dependencies
  - None

  - Side Effects
  - None
 */
/*=========================================================================*/

boolean QCMAP_LAN_Client::CheckForDuplicateFirewallRule
(
  qcmap_msgr_firewall_entry_conf_t *firewall_entry,
  uint32_t wan_profile_handle,
  qmi_error_type_v01 *qmi_err_num
)
{
  qcmap_msgr_firewall_entry_conf_t firewall_config[MAX_FIREWALL_ENTRY];
  int   handle_list_len;
  qmi_error_type_v01 error_num_temp = QMI_ERR_NONE_V01;
  qcmap_msgr_firewall_conf_t extd_firewall_handle_list;

  memset(&firewall_config, 0, MAX_FIREWALL_ENTRY*sizeof(qcmap_msgr_firewall_entry_conf_t));
  if(QCMAP_LAN_Client::GetFireWallConfigList(firewall_entry->filter_spec.ip_vsn , wan_profile_handle, &handle_list_len,
                        &extd_firewall_handle_list, firewall_config, &error_num_temp))
  {
    if( handle_list_len > 0 )
    {
      for(int i=0;i<handle_list_len;i++)
      {
        if ( (memcmp(&(firewall_entry->filter_spec), &(firewall_config[i].filter_spec),  sizeof(ip_filter_type)) == 0) &&
            (firewall_entry->firewall_direction == firewall_config[i].firewall_direction))
        {
          *qmi_err_num = error_num_temp;
          return true;
        }
      }
    }
  }
  return false;
}

/*===========================================================================
  FUNCTION CheckIpptFeatureModeWithFirewallSupport
  ===========================================================================*/
/*!
  @brief
  Function will check whether if IPPt is set w/o NAT mode and firewall support
  flag is set or not. If Both are set, will not allow ipv4 firewall rule addition.

  @return
   true  on success.
   false on failure
  @note

  - Dependencies
  - None

  - Side Effects
  - None
 */
/*=========================================================================*/
boolean QCMAP_LAN_Client::CheckIpptFeatureModeWithFirewallSupport(qmi_error_type_v01 *qmi_err_num)
{
  int feature_mode = 1;
  int firewall_support_flag = 0;
  char command[QCMAP_MAX_COMMAND_LEN] = {0};
  char result[QCMAP_MAX_SCAN_SIZE] = {0};

  memset(command, 0, QCMAP_MAX_COMMAND_LEN);
  memset(result, 0, QCMAP_MAX_SCAN_SIZE);
  snprintf(command, QCMAP_MAX_COMMAND_LEN, "%s %s",
            UCI_GET_COMMAND, QCMAP_IP_PT_FEATURE_MODE);
  if (ExecuteSystemCmd((const char *)command, result,
                  sizeof(result)))
  {
    feature_mode = atoi(result);
  }
  else
  {
    LOG_MSG_ERROR("Unable to query IP Passthrough feature mode", 0, 0, 0);
    *qmi_err_num = QMI_ERR_INTERNAL_V01;
    return false;
  }

  memset(command, 0, QCMAP_MAX_COMMAND_LEN);
  memset(result, 0, QCMAP_MAX_SCAN_SIZE);
  snprintf(command, QCMAP_MAX_COMMAND_LEN, "%s %s",
            UCI_GET_COMMAND, QCMAP_IPPT_WO_NAT_FW_SUPPORT);
  if (ExecuteSystemCmd((const char *)command, result,
                  sizeof(result)))
  {
    firewall_support_flag = atoi(result);
  }
  else
  {
    LOG_MSG_ERROR("Unable to query Firewall feature support flag", 0, 0, 0);
    *qmi_err_num = QMI_ERR_INTERNAL_V01;
    return false;
  }

  if ( ( (qcmap_lan_ip_passthrough_feature_mode_enum)feature_mode == IP_PASSTHROUGH_MODE_WITHOUT_NAT ) && (firewall_support_flag == IPV4_FIREWALL_NOT_SUPPORTED) )
  {
    return true;
  }
  return false;

}
/*===========================================================================
  FUNCTION AddFireWallEntry_Internal
  ===========================================================================*/
/*!
  @brief
  Add a firewall configuration according to user config
  Used by AddFireWallEntry() and AddUPNPPinholeEntry()

  @return
   true  on success.
   false on failure
  @note

  - Dependencies
  - None

  - Side Effects
  - None
 */
/*=========================================================================*/

boolean QCMAP_LAN_Client::AddFireWallEntry_Internal
(
  qcmap_msgr_firewall_conf_t *firewall_conf,
  uint32_t wan_profile_handle,
  boolean  upnp_pinhole,
  qmi_error_type_v01 *qmi_err_num
)
{
  qcmap_msgr_firewall_entry_conf_t *firewall_entry;
  int firewall_pkts_allowed=0;
  int enable_firewall_flag=0;
  char command[QCMAP_MAX_COMMAND_LEN] = {0};
  char result[QCMAP_MAX_SCAN_SIZE] = {0};
  uint8_t next_hdr_prot;
  int previous_v4_fw_rules_count=0;
  int previous_v6_fw_rules_count=0;
  int profile_index;
  ds_assert(firewall_conf != NULL);
  qmi_error_type_v01 error_num_temp = QMI_ERR_NONE_V01;

  LOG_MSG_INFO1("Entering AddFireWallEntry_Internal",0,0,0);
  firewall_entry = &firewall_conf->extd_firewall_entry;
  if ( firewall_entry->filter_spec.ip_vsn == IP_V4 )
  {
    if (QCMAP_LAN_Client::CheckIpptFeatureModeWithFirewallSupport(qmi_err_num))
    {
      LOG_MSG_ERROR("IPV4 firewall rule addition is not supported in IPPT w/o NAT mode",0,0,0);
      return false;
    }
  }
  /* check if firewall entry is not exceeding the max limit 128*/
  if( !checkFirewallEntryLimit( &error_num_temp ) )
  {
    if( error_num_temp == QMI_ERR_INTERNAL_V01 )
    {
      LOG_MSG_ERROR("UciGetUtility Failed to fetch info.", 0,0,0);
    }
    else
    {
      LOG_MSG_ERROR("Failed to AddFireWallEntry, max entry limit 128 exceeds ", 0,0,0);
    }
    return false;
  }

  // Check for duplicacy
  if( CheckForDuplicateFirewallRule(firewall_entry, wan_profile_handle, qmi_err_num) )
  {
    // This means we already added this one.
    LOG_MSG_ERROR("Firewall entry is already present\n",0,0,0);
    return false;
  }

  // Use current time as seed for random generator
  srand(time(0));
  // calling random function once as rand() function will give zero for the first time in Openwrt
  rand();
  firewall_entry->firewall_handle = (rand()%1000000);

  LOG_MSG_INFO1("IP family type %d, Direction %d ",firewall_entry->filter_spec.ip_vsn,
        firewall_entry->firewall_direction, 0);

  /* Get the profile index in qcmap_lan */
  profile_index = GetProfileIndex(wan_profile_handle);
  if (profile_index == QCMAP_LAN_INVALID)
  {
    /* indicates profile_idx not found */
    LOG_MSG_ERROR(" Profile index for profile: %d not found in qcmap_lan. Exiting!",
                  wan_profile_handle, 0, 0);
    *qmi_err_num = QMI_ERR_INTERNAL_V01;
    return false;
  }

  memset(result,0,QCMAP_MAX_SCAN_SIZE);
  snprintf(command, MAX_COMMAND_STR_LEN,"%s %s", UCI_GET_COMMAND, UCI_QUERY_NO_OF_FIREWALL_CONFIGURED);
  if (!ExecuteSystemCmd(command, result, sizeof(result)))
  {
    LOG_MSG_ERROR("failed to execute command : %s",command,0,0);
    *qmi_err_num = QMI_ERR_INTERNAL_V01;
    return false;
  }

  int count = atoi(result);
  snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh add qcmap_firewall firewall_rule");
  ds_system_call(command, strlen(command));
  snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh set qcmap_firewall.@firewall_rule[%d].firewall_handle=%d", count, firewall_entry->firewall_handle);
  ds_system_call(command, strlen(command));

  memset(result,0,QCMAP_MAX_SCAN_SIZE);
  snprintf(command, MAX_COMMAND_STR_LEN, "%s %s", UCI_GET_COMMAND, UCI_QUERY_NO_OF_FIREWALL_ENTRIES);
  if (!ExecuteSystemCmd(command, result, sizeof(result)))
  {
    LOG_MSG_ERROR("failed to execute command : %s",command,0,0);
    *qmi_err_num = QMI_ERR_INTERNAL_V01;
    return false;
  }

  int idx = atoi(result);

  memset(result,0,QCMAP_MAX_SCAN_SIZE);
  snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh get qcmap_lan.@profile[%d].no_of_v4_fw_rules", profile_index);
  if (!ExecuteSystemCmd(command, result, sizeof(result)))
  {
    LOG_MSG_ERROR("failed to execute command : %s",command,0,0);
    *qmi_err_num = QMI_ERR_INTERNAL_V01;
    return false;
  }

  previous_v4_fw_rules_count = atoi(result);

  memset(result,0,QCMAP_MAX_SCAN_SIZE);
  snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh get qcmap_lan.@profile[%d].no_of_v6_fw_rules", profile_index);
  if (!ExecuteSystemCmd(command, result, sizeof(result)))
  {
    LOG_MSG_ERROR("failed to execute command : %s",command,0,0);
    *qmi_err_num = QMI_ERR_INTERNAL_V01;
    return false;
  }

  previous_v6_fw_rules_count = atoi(result);

  int i = 0;
  /*since we need to add UL Firewall rules in two chains i.e OUTPUT and FORWARD, so we are looping two times*/
  if (firewall_entry->firewall_direction == QCMAP_MSGR_UL_FIREWALL)
  {
    while(i < MAX_LOOP_COUNT)
    {
      idx++;
      snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh add firewall rule");
      ds_system_call(command, strlen(command));
      snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh set firewall.@rule[%d]='%s'",idx,"rule");
      ds_system_call(command, strlen(command));
      snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh set qcmap_firewall.@firewall_rule[%d].direction='%s'",count,"UL");
      ds_system_call(command, strlen(command));
      if(i==0)
      {
        if(wan_profile_handle == 1)
        {
          snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh set firewall.@rule[%d].src='%s'",idx,"lan_wan");
          ds_system_call(command, strlen(command));
        }
        else
        {
          snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh set firewall.@rule[%d].src='%s%d'",idx,"lan_wan",wan_profile_handle);
          ds_system_call(command, strlen(command));
        }
        if(wan_profile_handle == 1)
        {
          snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh set firewall.@rule[%d].dest='%s'",idx,"wan5g");
          ds_system_call(command, strlen(command));
        }
        else
        {
          snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh set firewall.@rule[%d].dest='%s%d'",idx,"wan5g",wan_profile_handle);
          ds_system_call(command, strlen(command));
        }
        if((firewall_entry->filter_spec.ip_vsn == IP_V6) && upnp_pinhole)
        {
          QCMAP_LAN_CLIENT_RUN_COMMANDS("/etc/data/uci_ex.sh set firewall.@rule[%d].target='ACCEPT'",idx);
        }
        else
        {
          QCMAP_LAN_CLIENT_RUN_COMMANDS("/etc/data/uci_ex.sh set firewall.@rule[%d].target='%s'",idx,TARGET_CONNMARK);
          QCMAP_LAN_CLIENT_RUN_COMMANDS("/etc/data/uci_ex.sh set firewall.@rule[%d].set_mark='%d'",idx,FIREWALL_MARK);
        }
        snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh set firewall.@rule[%d].name='%d-%d-%d'",idx,firewall_entry->firewall_handle, i, wan_profile_handle);
        ds_system_call(command, strlen(command));
        if (firewall_entry->filter_spec.ip_vsn == IP_V4)
        {
          if(AddFireWallEntryUtilityV4(firewall_entry,idx,count))
          {
            printf("\n V4 Firewall Rule added successfully in Forward chain");
          }
        }
        else if(firewall_entry->filter_spec.ip_vsn == IP_V6)
        {
          if(AddFireWallEntryUtilityV6(firewall_entry,idx,count))
          {
            printf("\n V6 Firewall Rule added successfully in Forward chain");
          }
        }
      }
      else if(i == 1)
      {
        if(wan_profile_handle == 1)
        {
          snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh set firewall.@rule[%d].dest='%s'",idx,"wan5g");
          ds_system_call(command, strlen(command));
        }
        else
        {
          snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh set firewall.@rule[%d].dest='%s%d'",idx,"wan5g",wan_profile_handle);
          ds_system_call(command, strlen(command));
        }
        if((firewall_entry->filter_spec.ip_vsn == IP_V6) && upnp_pinhole)
        {
          QCMAP_LAN_CLIENT_RUN_COMMANDS("/etc/data/uci_ex.sh set firewall.@rule[%d].target='ACCEPT'",idx);
        }
        else
        {
          QCMAP_LAN_CLIENT_RUN_COMMANDS("/etc/data/uci_ex.sh set firewall.@rule[%d].target='%s'",idx,TARGET_CONNMARK);
          QCMAP_LAN_CLIENT_RUN_COMMANDS("/etc/data/uci_ex.sh set firewall.@rule[%d].set_mark='%d'",idx,FIREWALL_MARK);
        }
        snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh set firewall.@rule[%d].name='%d-%d-%d'",idx, firewall_entry->firewall_handle, i, wan_profile_handle);
        ds_system_call(command, strlen(command));
        if (firewall_entry->filter_spec.ip_vsn == IP_V4)
        {
          if(AddFireWallEntryUtilityV4(firewall_entry,idx,count))
          {
            printf("\n V4 Firewall Rule added successfully in Output chain");
          }
        }
        else if(firewall_entry->filter_spec.ip_vsn == IP_V6)
        {
          if(AddFireWallEntryUtilityV6(firewall_entry,idx,count))
          {
            printf("\n V6 Firewall Rule added successfully in Output chain");
          }
        }
      }

    if( GetBackhaulStatus(wan_profile_handle, firewall_entry->filter_spec.ip_vsn, qmi_err_num) )
    {
      memset(result,0,QCMAP_MAX_SCAN_SIZE);
      snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh get qcmap_lan.@profile[%d].firewall_enabled", profile_index);
      if (!ExecuteSystemCmd(command, result, sizeof(result)))
      {
        LOG_MSG_ERROR("failed to execute command : %s",command,0,0);
        return false;
      }
      /* If firewall enabled flag is 1 and Backhaul is UP, then only we need to
         set the enable flag as 1 */
      if(atoi(result) == 1)
      {
        snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh set firewall.@rule[%d].enabled='%d'",idx,1);
        ds_system_call(command, strlen(command));
      }
      else
      {
        snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh set firewall.@rule[%d].enabled='%d'",idx,0);
        ds_system_call(command, strlen(command));
      }
    }
    else
    {
     printf("\nEither Backhaul is not UP or  ip version is difft. with \n");
     snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh set firewall.@rule[%d].enabled='%d'",idx,0);
     ds_system_call(command, strlen(command));
    }
    snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh reorder firewall.@rule[%d]='%d'",idx,0);
    ds_system_call(command, strlen(command));

    if(i == 1)
    {
      break;
    }

    i+=1;
  }
 }

  if (firewall_entry->firewall_direction == QCMAP_MSGR_DL_FIREWALL)
  {
    idx++;
    snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh add firewall rule");
    ds_system_call(command, strlen(command));
    snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh set firewall.@rule[%d]='%s'",idx,"rule");
    ds_system_call(command, strlen(command));
    snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh set qcmap_firewall.@firewall_rule[%d].direction='%s'",count,"DL");
    ds_system_call(command, strlen(command));
    if(wan_profile_handle == 1)
    {
      snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh set firewall.@rule[%d].src='%s'",idx,"wan5g");
      ds_system_call(command, strlen(command));
    }
    else
    {
      snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh set firewall.@rule[%d].src='%s%d'",idx,"wan5g",wan_profile_handle);
      ds_system_call(command, strlen(command));
    }
    snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh set firewall.@rule[%d].dest='%s'",idx,"*");
    ds_system_call(command, strlen(command));

    if((firewall_entry->filter_spec.ip_vsn == IP_V6) && upnp_pinhole)
    {
      QCMAP_LAN_CLIENT_RUN_COMMANDS("/etc/data/uci_ex.sh set firewall.@rule[%d].target='ACCEPT'",idx);
    }
    else
    {
      QCMAP_LAN_CLIENT_RUN_COMMANDS("/etc/data/uci_ex.sh set firewall.@rule[%d].target='%s'",idx,TARGET_CONNMARK);
      QCMAP_LAN_CLIENT_RUN_COMMANDS("/etc/data/uci_ex.sh set firewall.@rule[%d].set_mark='%d'",idx,FIREWALL_MARK);
    }
    snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh set firewall.@rule[%d].name='%d-%d-%d'",idx, firewall_entry->firewall_handle, 0, wan_profile_handle);
    ds_system_call(command, strlen(command));
    if (firewall_entry->filter_spec.ip_vsn == IP_V4)
    {
      if(AddFireWallEntryUtilityV4(firewall_entry,idx,count))
      {
        printf("\n V4 DL Firewall Rule added successfully in Prerouting chain");
      }
    }
    else if(firewall_entry->filter_spec.ip_vsn == IP_V6)
    {
      if(AddFireWallEntryUtilityV6(firewall_entry,idx,count))
      {
         printf("\n V6 DL Firewall Rule added successfully in Prerouting chain");
      }
    }
    if( GetBackhaulStatus(wan_profile_handle, firewall_entry->filter_spec.ip_vsn, qmi_err_num) )
    {
      memset(result,0,QCMAP_MAX_SCAN_SIZE);
      snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh get qcmap_lan.@profile[%d].firewall_enabled", profile_index);
      if (!ExecuteSystemCmd(command, result, sizeof(result)))
      {
        LOG_MSG_ERROR("failed to execute command : %s",command,0,0);
        *qmi_err_num = QMI_ERR_INTERNAL_V01;
        return false;
      }

      enable_firewall_flag = atoi(result);
      /* If firewall enabled flag is 1 and Backhaul is UP, then only we need to
         set the enable flag as 1 */
      if( enable_firewall_flag == 1 )
      {
        snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh set firewall.@rule[%d].enabled='%d'",idx,1);
        ds_system_call(command, strlen(command));
      }
      else
      {
        snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh set firewall.@rule[%d].enabled='%d'",idx,0);
        ds_system_call(command, strlen(command));
      }
    }
    else
    {
     printf("\nEither Backhaul is not UP or  ip version is difft. with Backhaul type \n");
     snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh set firewall.@rule[%d].enabled='%d'",idx,0);
     ds_system_call(command, strlen(command));
    }

     snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh reorder firewall.@rule[%d]='%d'",idx,0);
     ds_system_call(command, strlen(command));
   }

  next_hdr_prot = firewall_entry->filter_spec.ip_hdr.v4.next_hdr_prot;

  memset(result,0,QCMAP_MAX_SCAN_SIZE);
  if ( QCMAP_LAN_Client::UciGetUtility(owrt_filename[0], "profile", 0, "pkts_allowed", result, profile_index) )
  {
    firewall_pkts_allowed=atoi(result);
  }

  if( firewall_entry->filter_spec.ip_vsn == IP_V4 )
  {
    snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh set qcmap_firewall.@firewall_rule[%d].name='FirewallV4-%d'", count, wan_profile_handle);
    ds_system_call(command, strlen(command));
    snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh set qcmap_lan.@profile[%d].no_of_v4_fw_rules='%d'", profile_index, previous_v4_fw_rules_count+1);
    ds_system_call(command, strlen(command));
  }
  else if( firewall_entry->filter_spec.ip_vsn == IP_V6 )
  {
    snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh set qcmap_firewall.@firewall_rule[%d].name='FirewallV6-%d'", count, wan_profile_handle);
    ds_system_call(command, strlen(command));
    snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh set qcmap_lan.@profile[%d].no_of_v6_fw_rules='%d'", profile_index, previous_v6_fw_rules_count+1);
    ds_system_call(command, strlen(command));
  }

  snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh set %s='%d'", UCI_QUERY_NO_OF_FIREWALL_ENTRIES, idx);
  ds_system_call(command, strlen(command));
  count+=1;
  snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh set %s='%d'", UCI_QUERY_NO_OF_FIREWALL_CONFIGURED, count);
  ds_system_call(command, strlen(command));
  ExecuteUCICommit();
  snprintf(command, MAX_COMMAND_STR_LEN, "/etc/init.d/firewall reload");
  ds_system_call(command, strlen(command));

  /* The below API's are need to clear the conntrack entries which would in turn delete
    the SFE entries.
    firewall_pkt_allowed =  false we need to delete the conntrack entries for the firewall IP, Port
    and Protocol combination which is entered
    firewall_pkt_allowed =  true we need to delete all other conntrack entries except for the
    firewall IP, Port and Protocol combination which is entered
    This would be applicable for Only TCP, UDP, ICMP and if no protocol is mentioned (if firewall is added
    only based on sport and dport)
   */
  if ( enable_firewall_flag && GetBackhaulStatus(wan_profile_handle, firewall_entry->filter_spec.ip_vsn, qmi_err_num) )
  {
    if ( next_hdr_prot == PS_IPPROTO_UDP || next_hdr_prot == PS_IPPROTO_TCP ||
         next_hdr_prot == PS_IPPROTO_TCP_UDP || next_hdr_prot == PS_IPPROTO_NO_PROTO ||
         next_hdr_prot == PS_IPPROTO_ICMP || next_hdr_prot == PS_IPPROTO_ICMP6 )
    {
        /* This is to delete external v4 conntrack when firewall rule count is zero either
        in ul or dl direction*/
        snprintf(command, MAX_COMMAND_STR_LEN, "%s %s %d", FIREWALL_CONFIG_FILE, CHECK_EXTERNAL_IPV4_CONNTARCK, wan_profile_handle);
        ds_system_call(command, strlen(command));
        if (firewall_pkts_allowed)
        {
          if (firewall_entry->filter_spec.ip_vsn == IP_V4)
          {
            DeleteConntrackEntryForAcceptIPv4FirewallEntries(firewall_entry, PS_IPPROTO_TCP, qmi_err_num);
            DeleteConntrackEntryForAcceptIPv4FirewallEntries(firewall_entry, PS_IPPROTO_UDP, qmi_err_num);
          }
          else if(firewall_entry->filter_spec.ip_vsn == IP_V6)
          {
            DeleteConntrackEntryForAcceptIPv6FirewallEntries(firewall_entry, PS_IPPROTO_TCP, qmi_err_num);
            DeleteConntrackEntryForAcceptIPv6FirewallEntries(firewall_entry, PS_IPPROTO_UDP, qmi_err_num);
          }
        }
        else
        {
          if (firewall_entry->filter_spec.ip_vsn == IP_V4)
          {
            if (next_hdr_prot == PS_IPPROTO_NO_PROTO)
              DeleteConntrackEntryForDropIPv4FirewallEntries(firewall_entry, PS_IPPROTO_TCP_UDP, qmi_err_num);
            else
              DeleteConntrackEntryForDropIPv4FirewallEntries(firewall_entry, next_hdr_prot, qmi_err_num);
          }
          else if(firewall_entry->filter_spec.ip_vsn == IP_V6)
          {
            if (next_hdr_prot == PS_IPPROTO_NO_PROTO)
              DeleteConntrackEntryForDropIPv6FirewallEntries(firewall_entry, PS_IPPROTO_TCP_UDP, qmi_err_num);
            else
              DeleteConntrackEntryForDropIPv6FirewallEntries(firewall_entry, next_hdr_prot, qmi_err_num);
          }
        }
    }
  }

  return true;
}

/*===========================================================================
  FUNCTION AddFireWallEntry
  ===========================================================================*/
/*!
  @brief
  Add a firewall configuration according to user config

  @return
   true  on success.
   false on failure
  @note

  - Dependencies
  - None

  - Side Effects
  - None
 */
/*=========================================================================*/

boolean QCMAP_LAN_Client::AddFireWallEntry
(
  qcmap_msgr_firewall_conf_t *firewall_conf,
  uint32_t wan_profile_handle,
  qmi_error_type_v01 *qmi_err_num
)
{
  return AddFireWallEntry_Internal(firewall_conf, wan_profile_handle, false, qmi_err_num);
}

/*===========================================================================
  FUNCTION AddUPNPPinholeEntry
  ===========================================================================*/
/*!
  @brief
  Add a firewall configuration for UPnP

  @return
   true  on success.
   false on failure
  @note

  - Dependencies
  - None

  - Side Effects

  - None
 */
/*=========================================================================*/

boolean QCMAP_LAN_Client::AddUPNPPinholeEntry
(
  qcmap_msgr_firewall_conf_t *firewall_conf,
  uint32_t wan_profile_handle,
  boolean  upnp_pinhole,
  qmi_error_type_v01 *qmi_err_num
)
{
  boolean upnp_pinhole_flag=false;
  if(GetUPNPState(&upnp_pinhole_flag, qmi_err_num) == false)
    return false;

  /* CHECK FOR PINHOLE*/
  if(firewall_conf->extd_firewall_entry.filter_spec.ip_vsn == IP_V6)
  {
    if(upnp_pinhole_flag == false )
    {
      LOG_MSG_ERROR("The UPNP Inbound Pinhole flag is not set. Adding Entry not allowed\n",0,0,0);
      *qmi_err_num = QMI_ERR_INTERNAL_V01;
      return false;
    }
  }

  return AddFireWallEntry_Internal(firewall_conf, wan_profile_handle, upnp_pinhole, qmi_err_num);

}

/*===========================================================================
FUNCTION GetFireWallConfigList
===========================================================================
@brief
  Get all the firewall configuration list for Mobile AP

@input
  ip_version - version of IP
  wan_profile_handle - current wan profile handle
  *handle_list_len  - pointer to handle_list_len variable
  extd_firewall_handle_list -  firewall configuration list
  firewall_config  - firewall_config array
  qmi_err_num

@return
  void

@dependencies
  usr to provide input

@sideefects
  None
=========================================================================*/
boolean QCMAP_LAN_Client::GetFireWallConfigList
(
  int ip_version,
  uint32_t wan_profile_handle,
  int* handle_list_len,
  qcmap_msgr_firewall_conf_t *extd_firewall_handle_list,
  qcmap_msgr_firewall_entry_conf_t *firewall_config,
  qmi_error_type_v01 *qmi_err_num
)
{
  char command[MAX_COMMAND_STR_LEN] = {0};
  char result[QCMAP_MAX_SCAN_SIZE] = {0};
  char firewall_entry_name[QCMAP_MAX_SCAN_SIZE] = {0};
  int  profile_index;
  int  total_fw_rules;
  int  index=0;
  int  val;

  /* Get the profile index in qcmap_lan */
  profile_index = GetProfileIndex(wan_profile_handle);
  if (profile_index == QCMAP_LAN_INVALID)
  {
    /* indicates profile_idx not found */
    LOG_MSG_ERROR(" Profile index for profile: %d not found in qcmap_lan. Exiting!",
                  wan_profile_handle, 0, 0);
    *qmi_err_num = QMI_ERR_INTERNAL_V01;
    return false;
  }

  //getting no of v4/v6 firewall rules added in that profile
  memset(result,0,QCMAP_MAX_SCAN_SIZE);
  if( ip_version == IP_V4 )
  {
    snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh get qcmap_lan.@profile[%d].no_of_v4_fw_rules",profile_index);
    snprintf(firewall_entry_name, QCMAP_MAX_SCAN_SIZE, "FirewallV4-%d", wan_profile_handle);
  }
  else
  {
    snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh get qcmap_lan.@profile[%d].no_of_v6_fw_rules",profile_index);
    snprintf(firewall_entry_name, QCMAP_MAX_SCAN_SIZE, "FirewallV6-%d", wan_profile_handle);
  }

  if (!ExecuteSystemCmd(command, result, sizeof(result)))
  {
    LOG_MSG_ERROR("failed to execute command : %s",command,0,0);
    *qmi_err_num = QMI_ERR_INTERNAL_V01;
    return false;
  }

  *handle_list_len = atoi(result);
  if(*handle_list_len == 0)
  {
    return true;
  }

  //getting total no of firewall rules added in that profile
  memset(result,0,QCMAP_MAX_SCAN_SIZE);
  snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh get qcmap_firewall.@firewall[0].no_of_rules");
  if (!ExecuteSystemCmd(command, result, sizeof(result)))
  {
    LOG_MSG_ERROR("failed to execute command : %s",command,0,0);
    *qmi_err_num = QMI_ERR_INTERNAL_V01;
    return false;
  }

  total_fw_rules = atoi(result);
  for(int i=0; i < total_fw_rules; i++)
  {

    memset(command,0,MAX_COMMAND_STR_LEN);
    memset(result,0,QCMAP_MAX_SCAN_SIZE);

    if (UciGetUtility(owrt_filename[static_cast<int>(config_file::QCMAP_FIREWALL)],
          "firewall_rule", 0, "name", result, i))
    {
      LOG_MSG_INFO1("result = %s\n", result, 0, 0);
    }
    else
    {
      LOG_MSG_ERROR("UciGetUtility failed ",0,0,0);
      return false;
    }

    if(strncmp(result, firewall_entry_name, QCMAP_MAX_SCAN_SIZE)==0)
    {
      val = 0;
      memset(result,0,QCMAP_MAX_SCAN_SIZE);

      if (UciGetUtility(owrt_filename[static_cast<int>(config_file::QCMAP_FIREWALL)],
            "firewall_rule", 0, "firewall_handle", result, i))
      {
        firewall_config[index].firewall_handle = atoi(result);
      }
      else
      {
        LOG_MSG_ERROR("UciGetUtility failed ",0,0,0);
        return false;
      }

      memset(result,0,QCMAP_MAX_SCAN_SIZE);

      if (UciGetUtility(owrt_filename[static_cast<int>(config_file::QCMAP_FIREWALL)],
            "firewall_rule", 0, "direction", result, i))
      {

        if(strncmp(result, "UL", strlen("UL"))==0)
        {
          firewall_config[index].firewall_direction = QCMAP_MSGR_UL_FIREWALL;
        }
        else
        {
          firewall_config[index].firewall_direction = QCMAP_MSGR_DL_FIREWALL;
        }
      }
      else
      {
        LOG_MSG_ERROR("UciGetUtility failed ",0,0,0);
        return false;
      }

      memset(result,0,QCMAP_MAX_SCAN_SIZE);
      if (UciGetUtility(owrt_filename[static_cast<int>(config_file::QCMAP_FIREWALL)],
            "firewall_rule", 0, "family", result, i))
      {

        if(strncmp(result, IP_V4_STRING, strlen(IP_V4_STRING))==0)
        {
          firewall_config[index].filter_spec.ip_vsn = IP_V4;
        }
        else
        {
          firewall_config[index].filter_spec.ip_vsn = IP_V6;
        }
      }
      else
      {
        LOG_MSG_ERROR("UciGetUtility failed ",0,0,0);
        return false;
      }

      if(firewall_config[index].filter_spec.ip_vsn == IP_V4)
      {

        memset(result,0,QCMAP_MAX_SCAN_SIZE);
        if (UciGetUtility(owrt_filename[static_cast<int>(config_file::QCMAP_FIREWALL)],
              "firewall_rule", 0, "src_addr", result, i))
        {

          if(strncmp(result, "Any", strlen("Any"))!=0)
          {
            firewall_config[index].filter_spec.ip_hdr.v4.field_mask |= IPFLTR_MASK_IP4_SRC_ADDR;
            if (inet_pton(AF_INET, result, (char *)&firewall_config[index].filter_spec.ip_hdr.v4.src.addr.ps_s_addr) <=0)
            {
              LOG_MSG_ERROR(" Address not in presentation format\n", 0, 0, 0);
              return false;
            }

            memset(result,0,QCMAP_MAX_SCAN_SIZE);
            if (UciGetUtility(owrt_filename[static_cast<int>(config_file::QCMAP_FIREWALL)],
                "firewall_rule", 0, "src_addr_mask", result, i))
            {
              if(strncmp(result, "Any", strlen("Any"))!=0)
              {
                if (inet_pton(AF_INET, result, (uint8* )&firewall_config[index].filter_spec.ip_hdr.v4.src.subnet_mask.ps_s_addr) <=0)
                {
                     LOG_MSG_ERROR(" Address not in presentation format\n", 0, 0, 0);
                     return false;
                }
              }
            }
            else
            {
              LOG_MSG_ERROR("UciGetUtility failed ",0,0,0);
              return false;
            }
          }
        }
        else
        {
          LOG_MSG_ERROR("UciGetUtility failed ",0,0,0);
          return false;
        }/* src_addr if condition ends here */

        memset(result,0,QCMAP_MAX_SCAN_SIZE);
        if (UciGetUtility(owrt_filename[static_cast<int>(config_file::QCMAP_FIREWALL)],
              "firewall_rule", 0, "dest_addr", result, i))
        {

          if(strncmp(result, "Any", strlen("Any"))!=0)
          {
            firewall_config[index].filter_spec.ip_hdr.v4.field_mask |= IPFLTR_MASK_IP4_DST_ADDR;
            if ( inet_pton(AF_INET, result, (char *)&firewall_config[index].filter_spec.ip_hdr.v4.dst.addr.ps_s_addr) <=0 ) {
              LOG_MSG_ERROR(" Address not in presentation format\n", 0, 0, 0);
              return false;
            }

            memset(result,0,QCMAP_MAX_SCAN_SIZE);
            if (UciGetUtility(owrt_filename[static_cast<int>(config_file::QCMAP_FIREWALL)],
                "firewall_rule", 0, "dest_addr_mask", result, i))
            {

              if(strncmp(result, "Any", strlen("Any"))!=0)
              {
                if ( inet_pton(AF_INET, result, (uint8* )&firewall_config[index].filter_spec.ip_hdr.v4.dst.subnet_mask.ps_s_addr) <=0 ) {
                     LOG_MSG_ERROR(" Address not in presentation format\n", 0, 0, 0);
                     return false;
                   }
              }
            }
            else
            {
              LOG_MSG_ERROR("UciGetUtility failed ",0,0,0);
              return false;
            }
          }
        }
        else
        {
          LOG_MSG_ERROR("UciGetUtility failed ",0,0,0);
          return false;
        }/* dest_addr if condition ends here */

        memset(result,0,QCMAP_MAX_SCAN_SIZE);
        if (UciGetUtility(owrt_filename[static_cast<int>(config_file::QCMAP_FIREWALL)],
              "firewall_rule", 0, "tos_value", result, i))
        {
          if(strncmp(result, "Any", strlen("Any"))!=0)
          {
            firewall_config[index].filter_spec.ip_hdr.v4.field_mask |= IPFLTR_MASK_IP4_TOS;
            firewall_config[index].filter_spec.ip_hdr.v4.tos.val = (int)strtol(result, NULL, 16);

          memset(result,0,QCMAP_MAX_SCAN_SIZE);
          if (UciGetUtility(owrt_filename[static_cast<int>(config_file::QCMAP_FIREWALL)],
              "firewall_rule", 0, "tos_mask", result, i))
          {
            if(strncmp(result, "Any", strlen("Any"))!=0)
            {
              firewall_config[index].filter_spec.ip_hdr.v4.tos.mask = (int)strtol(result, NULL, 16);
            }
          }
          else
          {
            LOG_MSG_ERROR("UciGetUtility failed ",0,0,0);
            return false;
          }
         }
        }
        else
        {
          LOG_MSG_ERROR("UciGetUtility failed ",0,0,0);
          return false;
        }/* tos if condition ends here */

        memset(result,0,QCMAP_MAX_SCAN_SIZE);
        if (UciGetUtility(owrt_filename[static_cast<int>(config_file::QCMAP_FIREWALL)],
              "firewall_rule", 0, "proto", result, i))
        {

          if(strncmp(result, "Any", strlen("Any"))!=0)
          {
            firewall_config[index].filter_spec.ip_hdr.v4.field_mask |= IPFLTR_MASK_IP4_NEXT_HDR_PROT;
            if(strncmp(result, TCP_PROTO, strlen(result))==0)
            {
              val=PS_IPPROTO_TCP;
              firewall_config[index].filter_spec.ip_hdr.v4.next_hdr_prot = PS_IPPROTO_TCP;
            }
            else if(strncmp(result, UDP_PROTO, strlen(result))==0)
            {
              val=PS_IPPROTO_UDP;
              firewall_config[index].filter_spec.ip_hdr.v4.next_hdr_prot = PS_IPPROTO_UDP;
            }
            else if(strncmp(result, ICMP_PROTO, strlen(result))==0)
            {
              val=PS_IPPROTO_ICMP;
              firewall_config[index].filter_spec.ip_hdr.v4.next_hdr_prot = PS_IPPROTO_ICMP;
            }
            else if(strncmp(result, TCP_UDP_PROTO, strlen(result))==0)
            {
              val=PS_IPPROTO_TCP_UDP;
              firewall_config[index].filter_spec.ip_hdr.v4.next_hdr_prot = PS_IPPROTO_TCP_UDP;
            }
            else if(strncmp(result, ESP_PROTO, strlen(result))==0)
            {
              val=PS_IPPROTO_ESP;
              firewall_config[index].filter_spec.ip_hdr.v4.next_hdr_prot = PS_IPPROTO_ESP;
            }
          }
        }
        else
        {
          LOG_MSG_ERROR("UciGetUtility failed ",0,0,0);
          return false;
        }/* Proto if condition ends */
      } /* IPV4 if condition ends here */
      else
      {
        //if IP Version is IPV6.
        memset(result,0,QCMAP_MAX_SCAN_SIZE);
        if (UciGetUtility(owrt_filename[static_cast<int>(config_file::QCMAP_FIREWALL)],
              "firewall_rule", 0, "src_addr", result, i))
        {

          if(strncmp(result, "Any", strlen("Any"))!=0)
          {
            firewall_config[index].filter_spec.ip_hdr.v6.field_mask |= IPFLTR_MASK_IP6_SRC_ADDR;
            if ( inet_pton(AF_INET6, result, (uint8 *)&firewall_config[index].filter_spec.ip_hdr.v6.src.addr.in6_u.u6_addr8) <=0 ) {
              LOG_MSG_ERROR(" Address not in presentation format\n", 0, 0, 0);
              return false;
            }

            memset(result,0,QCMAP_MAX_SCAN_SIZE);
            if (UciGetUtility(owrt_filename[static_cast<int>(config_file::QCMAP_FIREWALL)],
                "firewall_rule", 0, "src_prefix_length", result, i))
            {
              if(strncmp(result, "Any", strlen("Any"))!=0)
              {
                firewall_config[index].filter_spec.ip_hdr.v6.src.prefix_len = atoi(result);
              }
            }
            else
            {
              LOG_MSG_ERROR("UciGetUtility failed ",0,0,0);
              return false;
            }
          }
        }
        else
        {
          LOG_MSG_ERROR("UciGetUtility failed ",0,0,0);
          return false;
        }/* src_addr if condition ends here */

        memset(result,0,QCMAP_MAX_SCAN_SIZE);
        if (UciGetUtility(owrt_filename[static_cast<int>(config_file::QCMAP_FIREWALL)],
              "firewall_rule", 0, "dest_addr", result, i))
        {

          if(strncmp(result, "Any", strlen("Any"))!=0)
          {
            firewall_config[index].filter_spec.ip_hdr.v6.field_mask |= IPFLTR_MASK_IP6_DST_ADDR;
            if ( inet_pton(AF_INET6, result, (uint8 *)&firewall_config[index].filter_spec.ip_hdr.v6.dst.addr.in6_u.u6_addr8) <=0 ) {
              LOG_MSG_ERROR(" Address not in presentation format\n", 0, 0, 0);
              return false;
            }

            memset(result,0,QCMAP_MAX_SCAN_SIZE);
            if (UciGetUtility(owrt_filename[static_cast<int>(config_file::QCMAP_FIREWALL)],
                "firewall_rule", 0, "dest_prefix_length", result, i))
            {

              if(strncmp(result, "Any", strlen("Any"))!=0)
              {
                firewall_config[index].filter_spec.ip_hdr.v6.dst.prefix_len = atoi(result);
              }
            }
            else
            {
              LOG_MSG_ERROR("UciGetUtility failed ",0,0,0);
              return false;
            }
          }
        }
        else
        {
          LOG_MSG_ERROR("UciGetUtility failed ",0,0,0);
          return false;
        }/* dest_addr if condition ends here */

        memset(result,0,QCMAP_MAX_SCAN_SIZE);
        if (UciGetUtility(owrt_filename[static_cast<int>(config_file::QCMAP_FIREWALL)],
              "firewall_rule", 0, "tos_value", result, i))
        {
          if(strncmp(result, "Any", strlen("Any"))!=0)
          {
            firewall_config[index].filter_spec.ip_hdr.v6.field_mask |= IPFLTR_MASK_IP6_TRAFFIC_CLASS;
            firewall_config[index].filter_spec.ip_hdr.v6.trf_cls.val = (int)strtol(result, NULL, 16);

          memset(result,0,QCMAP_MAX_SCAN_SIZE);
          if (UciGetUtility(owrt_filename[static_cast<int>(config_file::QCMAP_FIREWALL)],
              "firewall_rule", 0, "tos_mask", result, i))
          {
            if(strncmp(result, "Any", strlen("Any"))!=0)
            {
              firewall_config[index].filter_spec.ip_hdr.v6.trf_cls.mask = (int)strtol(result, NULL, 16);
            }
          }
          else
          {
            LOG_MSG_ERROR("UciGetUtility failed ",0,0,0);
            return false;
          }
         }
        }
        else
        {
          LOG_MSG_ERROR("UciGetUtility failed ",0,0,0);
          return false;
        }/* tos if condition ends here */
        memset(result,0,QCMAP_MAX_SCAN_SIZE);
        if (UciGetUtility(owrt_filename[static_cast<int>(config_file::QCMAP_FIREWALL)],
              "firewall_rule", 0, "proto", result, i))
        {
          if(strncmp(result, "Any", strlen("Any"))!=0)
          {
            firewall_config[index].filter_spec.ip_hdr.v6.field_mask |= IPFLTR_MASK_IP6_NEXT_HDR_PROT;
            if(strncmp(result, TCP_PROTO, strlen(result))==0)
            {
              val=PS_IPPROTO_TCP;
              firewall_config[index].filter_spec.ip_hdr.v6.next_hdr_prot = PS_IPPROTO_TCP;
            }
            else if(strncmp(result, UDP_PROTO, strlen(result))==0)
            {
              val=PS_IPPROTO_UDP;
              firewall_config[index].filter_spec.ip_hdr.v6.next_hdr_prot = PS_IPPROTO_UDP;
            }
            else if(strncmp(result, ICMP_PROTO, strlen(result))==0)
            {
              val=PS_IPPROTO_ICMP6;
              firewall_config[index].filter_spec.ip_hdr.v6.next_hdr_prot = PS_IPPROTO_ICMP6;
            }
            else if(strncmp(result, TCP_UDP_PROTO, strlen(result))==0)
            {
              val=PS_IPPROTO_TCP_UDP;
              firewall_config[index].filter_spec.ip_hdr.v6.next_hdr_prot = PS_IPPROTO_TCP_UDP;
            }
            else if(strncmp(result, ESP_PROTO, strlen(result))==0)
            {
              val=PS_IPPROTO_ESP;
              firewall_config[index].filter_spec.ip_hdr.v6.next_hdr_prot = PS_IPPROTO_ESP;
            }
          }
        }
        else
        {
          LOG_MSG_ERROR("UciGetUtility failed ",0,0,0);
          return false;
        }/* Proto if condition ends */

      } /* IPV6 if condition ends here */

      if(val == PS_IPPROTO_TCP)
      {
        memset(result,0,QCMAP_MAX_SCAN_SIZE);
        if (UciGetUtility(owrt_filename[static_cast<int>(config_file::QCMAP_FIREWALL)],
              "firewall_rule", 0, "src_port", result, i))
        {
          if(strncmp(result, "Any", strlen("Any"))!=0)
          {
            firewall_config[index].filter_spec.next_prot_hdr.tcp.field_mask |= IPFLTR_MASK_TCP_SRC_PORT;
            firewall_config[index].filter_spec.next_prot_hdr.tcp.src.port = atoi(result);

            memset(result,0,QCMAP_MAX_SCAN_SIZE);
            if (UciGetUtility(owrt_filename[static_cast<int>(config_file::QCMAP_FIREWALL)],
                "firewall_rule", 0, "src_port_range", result, i))
            {
              firewall_config[index].filter_spec.next_prot_hdr.tcp.src.range = atoi(result);
            }
            else
            {
              LOG_MSG_ERROR("UciGetUtility failed ",0,0,0);
              return false;
            }
          }
        }
        else
        {
          LOG_MSG_ERROR("UciGetUtility failed ",0,0,0);
          return false;
        }

        memset(result,0,QCMAP_MAX_SCAN_SIZE);
        if (UciGetUtility(owrt_filename[static_cast<int>(config_file::QCMAP_FIREWALL)],
              "firewall_rule", 0, "dest_port", result, i))
        {
          if(strncmp(result, "Any", strlen("Any"))!=0)
          {
            firewall_config[index].filter_spec.next_prot_hdr.tcp.field_mask |= IPFLTR_MASK_TCP_DST_PORT;
            firewall_config[index].filter_spec.next_prot_hdr.tcp.dst.port = atoi(result);

            memset(result,0,QCMAP_MAX_SCAN_SIZE);
            if (UciGetUtility(owrt_filename[static_cast<int>(config_file::QCMAP_FIREWALL)],
                "firewall_rule", 0, "dest_port_range", result, i))
            {
              firewall_config[index].filter_spec.next_prot_hdr.tcp.dst.range = atoi(result);
            }
            else
            {
              LOG_MSG_ERROR("UciGetUtility failed ",0,0,0);
              return false;
            }
          }
        }
        else
        {
          LOG_MSG_ERROR("UciGetUtility failed ",0,0,0);
          return false;
        }
      }
      else if(val == PS_IPPROTO_UDP)
      {
        memset(result,0,QCMAP_MAX_SCAN_SIZE);
        if (UciGetUtility(owrt_filename[static_cast<int>(config_file::QCMAP_FIREWALL)],
              "firewall_rule", 0, "src_port", result, i))
        {
          if(strncmp(result, "Any", strlen("Any"))!=0)
          {
            firewall_config[index].filter_spec.next_prot_hdr.udp.field_mask |= IPFLTR_MASK_UDP_SRC_PORT;
            firewall_config[index].filter_spec.next_prot_hdr.udp.src.port = atoi(result);

            memset(result,0,QCMAP_MAX_SCAN_SIZE);
            if (UciGetUtility(owrt_filename[static_cast<int>(config_file::QCMAP_FIREWALL)],
                "firewall_rule", 0, "src_port_range", result, i))
            {
              firewall_config[index].filter_spec.next_prot_hdr.udp.src.range = atoi(result);
            }
            else
            {
              LOG_MSG_ERROR("UciGetUtility failed ",0,0,0);
              return false;
            }
          }
        }
        else
        {
          LOG_MSG_ERROR("UciGetUtility failed ",0,0,0);
          return false;
        }

        memset(result,0,QCMAP_MAX_SCAN_SIZE);
        if (UciGetUtility(owrt_filename[static_cast<int>(config_file::QCMAP_FIREWALL)],
              "firewall_rule", 0, "dest_port", result, i))
        {
          if(strncmp(result, "Any", strlen("Any"))!=0)
          {
            firewall_config[index].filter_spec.next_prot_hdr.udp.field_mask |= IPFLTR_MASK_UDP_DST_PORT;
            firewall_config[index].filter_spec.next_prot_hdr.udp.dst.port = atoi(result);

            memset(result,0,QCMAP_MAX_SCAN_SIZE);
            if (UciGetUtility(owrt_filename[static_cast<int>(config_file::QCMAP_FIREWALL)],
                "firewall_rule", 0, "dest_port_range", result, i))
            {
              firewall_config[index].filter_spec.next_prot_hdr.udp.dst.range = atoi(result);
            }
            else
            {
              LOG_MSG_ERROR("UciGetUtility failed ",0,0,0);
              return false;
            }
          }
        }
        else
        {
          LOG_MSG_ERROR("UciGetUtility failed ",0,0,0);
          return false;
        }
      }
      else if (val == PS_IPPROTO_TCP_UDP)
      {
        memset(result,0,QCMAP_MAX_SCAN_SIZE);
        if (UciGetUtility(owrt_filename[static_cast<int>(config_file::QCMAP_FIREWALL)],
              "firewall_rule", 0, "src_port", result, i))
        {
          if(strncmp(result, "Any", strlen("Any"))!=0)
          {
            firewall_config[index].filter_spec.next_prot_hdr.tcp_udp_port_range.field_mask |= IPFLTR_MASK_TCP_UDP_SRC_PORT;
            firewall_config[index].filter_spec.next_prot_hdr.tcp_udp_port_range.src.port = atoi(result);

            memset(result,0,QCMAP_MAX_SCAN_SIZE);
            if (UciGetUtility(owrt_filename[static_cast<int>(config_file::QCMAP_FIREWALL)],
                "firewall_rule", 0, "src_port_range", result, i))
            {
              firewall_config[index].filter_spec.next_prot_hdr.tcp_udp_port_range.src.range = atoi(result);
            }
            else
            {
              LOG_MSG_ERROR("UciGetUtility failed ",0,0,0);
              return false;
            }
          }
        }
        else
        {
          LOG_MSG_ERROR("UciGetUtility failed ",0,0,0);
          return false;
        }

        memset(result,0,QCMAP_MAX_SCAN_SIZE);
        if (UciGetUtility(owrt_filename[static_cast<int>(config_file::QCMAP_FIREWALL)],
              "firewall_rule", 0, "dest_port", result, i))
        {

          if(strncmp(result, "Any", strlen("Any"))!=0)
          {
            firewall_config[index].filter_spec.next_prot_hdr.tcp_udp_port_range.field_mask |= IPFLTR_MASK_TCP_UDP_DST_PORT;
            firewall_config[index].filter_spec.next_prot_hdr.tcp_udp_port_range.dst.port = atoi(result);

            memset(result,0,QCMAP_MAX_SCAN_SIZE);
            if (UciGetUtility(owrt_filename[static_cast<int>(config_file::QCMAP_FIREWALL)],
                "firewall_rule", 0, "dest_port_range", result, i))
            {
              firewall_config[index].filter_spec.next_prot_hdr.tcp_udp_port_range.dst.range = atoi(result);
            }
            else
            {
              LOG_MSG_ERROR("UciGetUtility failed ",0,0,0);
              return false;
            }
          }
        }
        else
        {
          LOG_MSG_ERROR("UciGetUtility failed ",0,0,0);
          return false;
        }
      }
      else if ( val == PS_IPPROTO_ICMP)
      {
        memset(result,0,QCMAP_MAX_SCAN_SIZE);
        if (UciGetUtility(owrt_filename[static_cast<int>(config_file::QCMAP_FIREWALL)],
              "firewall_rule", 0, "icmp_type", result, i))
        {
          if(strncmp(result, "Any", strlen("Any"))!=0)
          {
            firewall_config[index].filter_spec.next_prot_hdr.icmp.type = atoi(result);
            firewall_config[index].filter_spec.next_prot_hdr.icmp.field_mask |= IPFLTR_MASK_ICMP_MSG_TYPE;
            memset(result,0,QCMAP_MAX_SCAN_SIZE);
            if (UciGetUtility(owrt_filename[static_cast<int>(config_file::QCMAP_FIREWALL)],
                "firewall_rule", 0, "icmp_code", result, i))
            {
              firewall_config[index].filter_spec.next_prot_hdr.icmp.code = atoi(result);
              firewall_config[index].filter_spec.next_prot_hdr.icmp.field_mask |= IPFLTR_MASK_ICMP_MSG_CODE;
            }
          }
        }
        else
        {
          LOG_MSG_ERROR("UciGetUtility failed ",0,0,0);
          return false;
        }
      }
      else if ( val == PS_IPPROTO_ICMP6 )
      {
        memset(result,0,QCMAP_MAX_SCAN_SIZE);
        if (UciGetUtility(owrt_filename[static_cast<int>(config_file::QCMAP_FIREWALL)],
              "firewall_rule", 0, "icmp6_type", result, i))
        {
          if(strncmp(result, "Any", strlen("Any"))!=0)
          {
            firewall_config[index].filter_spec.next_prot_hdr.icmp.type = atoi(result);
            firewall_config[index].filter_spec.next_prot_hdr.icmp.field_mask |= IPFLTR_MASK_ICMP_MSG_TYPE;
            memset(result,0,QCMAP_MAX_SCAN_SIZE);
            if (UciGetUtility(owrt_filename[static_cast<int>(config_file::QCMAP_FIREWALL)],
                "firewall_rule", 0, "icmp6_code", result, i))
            {
              firewall_config[index].filter_spec.next_prot_hdr.icmp.code = atoi(result);
              firewall_config[index].filter_spec.next_prot_hdr.icmp.field_mask |= IPFLTR_MASK_ICMP_MSG_CODE;
            }
          }
        }
        else
        {
          LOG_MSG_ERROR("UciGetUtility failed ",0,0,0);
          return false;
        }
      }
      else if( val == PS_IPPROTO_ESP)
      {
        if (UciGetUtility(owrt_filename[static_cast<int>(config_file::QCMAP_FIREWALL)],
              "firewall_rule", 0, "esp_spi", result, i))
        {
          if(strncmp(result, "Any", strlen("Any"))!=0)
          {
            firewall_config[index].filter_spec.next_prot_hdr.esp.spi = atoi(result);
            firewall_config[index].filter_spec.next_prot_hdr.esp.field_mask |= IPFLTR_MASK_ESP_SPI;
          }
        }
        else
        {
          LOG_MSG_ERROR("UciGetUtility failed ",0,0,0);
          return false;
        }
      }
      index++;

    }

  }
  return true;
}


/*==========================================================================
FUNCTION DeleteFireWallEntry
===========================================================================*/
/*!
@brief
  delete firewall config for firewall entry identified by the handle
@return
  true  on success.
  false on failure
@note
  - Dependencies
  - None

- Side Effects
  - None
*/
/*=========================================================================*/
boolean QCMAP_LAN_Client::DeleteFireWallEntry
(
  int handle,
  uint32_t wan_profile_handle,
  qmi_error_type_v01 *qmi_err_num
)
{
  if(handle < 0)
  {
    printf("Improper Handle passed");
    *qmi_err_num = QMI_ERR_INVALID_HANDLE_V01;
    return false;
  }

  char command[QCMAP_MAX_COMMAND_LEN] = {0};
  snprintf(command, QCMAP_MAX_COMMAND_LEN,"%s %s %d %d",FIREWALL_CONFIG_FILE,DELETE_FIREWALL, handle, wan_profile_handle);
  ds_system_call(command, strlen(command));
  return true;
}

/*===========================================================================
  FUNCTION EnableFirewall
  ===========================================================================
  @brief
    Enable the firewall configuration rules when backhaul is Up

  @input
    wan_profile_handle

  @return
    boolean

  @dependencies
    usr to provide input

  @sideefects
    None
  =========================================================================*/

boolean QCMAP_LAN_Client::EnableFirewall
(
  uint32_t wan_profile_handle,
  qcmap_backhaul_type bh_type,
  qmi_error_type_v01 *qmi_err_num
)
{
  char command[QCMAP_MAX_COMMAND_LEN] = {0};

  if ( wan_profile_handle == 0 )
  {
    LOG_MSG_ERROR("Invalid wan profile handle passed ",0,0,0);
    *qmi_err_num = QMI_ERR_INVALID_HANDLE_V01;
    return false;
  }
  snprintf(command, QCMAP_MAX_COMMAND_LEN,"%s %s %d %d", FIREWALL_CONFIG_FILE, ENABLE_FIREWALL, wan_profile_handle, bh_type);
  ds_system_call(command, strlen(command));
  printf("Backhaul Up - Enable firewall done\n");
  return true;
}

/*===========================================================================
  FUNCTION DisableFirewall
  ===========================================================================
  @brief
    Disable the firewall configuration rules when backhaul down

  @input
    void

  @return
   boolean

  @dependencies
   usr to provide input

  @sideefects
   None
  =========================================================================*/

boolean QCMAP_LAN_Client::DisableFirewall
(
  uint32_t wan_profile_handle,
  qcmap_backhaul_type bh_type,
  qmi_error_type_v01 * qmi_err_num
)
{
  char command[QCMAP_MAX_COMMAND_LEN] = {0};

  if ( wan_profile_handle == 0 )
  {
    LOG_MSG_ERROR("Invalid wan profile handle passed ",0,0,0);
    * qmi_err_num = QMI_ERR_INVALID_PROFILE_V01;
    return false;
  }

  snprintf(command, QCMAP_MAX_COMMAND_LEN,"%s %s %d %d",FIREWALL_CONFIG_FILE, DISABLE_FIREWALL, wan_profile_handle, bh_type);
  ds_system_call(command, strlen(command));
  printf("Backhaul Down - Disable firewall done\n");
  return true;
}

/*===========================================================================
FUNCTION SetIPSecVpnPassthrough()
===========================================================================*/
/** @ingroup qcmap_set_ipsec_vpn_passthrough_config

  sets ipsec vpn passthrough

  @datatypes
  qcmap_lan_ipsec_vpn_passthrough_mode_enum

  @param[in]      enable_state              IPsec VPN Passthrough Enable state \n
  @param[in]      default_handle            Default Profile handle \n
  @param[in]      profile_handle            Current Profile Handle

  @return
  TRUE -- Success \n
  FALSE -- Failure \n
*/
/*=========================================================================*/
boolean QCMAP_LAN_Client::SetIPSECVpnPassthrough
(
  qcmap_lan_ipsec_vpn_passthrough_mode_enum enable_state,
  const uint32_t default_handle,
  const uint32_t profile_handle,
  qmi_error_type_v01 * qmi_err_num
)
{
  char cmd[QCMAP_MAX_COMMAND_LEN] = {0};
  char result[QCMAP_MAX_SCAN_SIZE] = {0};
  int ipsecpt_enable = -1, profile_idx = -1;

  /* Calling Utility function to check if wan rules are present in uci database */
  if (!CheckIfNetworkRulesExist(NETWORK_WAN, profile_handle))
  {
    LOG_MSG_ERROR("Failed to validate network wan rules presence in uci database"
                  "for profile: %d", profile_handle, 0, 0);
    return false;
  }

  /* Check enable_state */
  if (enable_state == QCMAP_LAN_IPSEC_VPN_PASSTHROUGH_MODE_UP)
  {
    /* Get the profile index in qcmap_lan */
    profile_idx = GetProfileIndex(profile_handle);
    if (profile_idx == QCMAP_LAN_INVALID)
    {
      /* indicates profile_idx not found */
      LOG_MSG_ERROR(" Profile index for profile: %d not found in qcmap_lan. Exiting!",
                    profile_handle, 0, 0);
      return false;
    }

    /* Check if ipsecpt_enable is already set for wan<Current_Profile> */
    ipsecpt_enable = GetActiveIpsecVpnPt(profile_idx);
    if (ipsecpt_enable != QCMAP_LAN_INVALID)
    {
      if (ipsecpt_enable == 1)
      {
        LOG_MSG_ERROR("IPSec VPN Passthrough already active on the PDN: %d.",
                       profile_handle, 0, 0);
        *qmi_err_num = QMI_ERR_NO_EFFECT_V01;
        return false;
      }
    }

    /* Enable IPSec VPN Passthrough */
    memset(cmd, 0, QCMAP_MAX_COMMAND_LEN);
    snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "%s set_vpnpt %d %d %d %s", NAT_ALG_VPN_CONFIG_FILE,
             profile_handle, profile_idx, enable_state, IPSEC_VPN_PASSTHROUGH_V4);
    ds_system_call(cmd, strlen(cmd));
  } /* End enable */
  else if (enable_state == QCMAP_LAN_IPSEC_VPN_PASSTHROUGH_MODE_DOWN)
  {
    /* Get the profile index in qcmap_lan */
    profile_idx = GetProfileIndex(profile_handle);
    if (profile_idx == QCMAP_LAN_INVALID)
    {
      /* indicates profile_idx not found */
      LOG_MSG_ERROR(" Profile index for profile: %d not found in qcmap_lan. Exiting!",
                    profile_handle, 0, 0);
      return false;
    }

    /* Check if ipsecpt_enable is already set for wan<Current_Profile> */
    ipsecpt_enable = GetActiveIpsecVpnPt(profile_idx);
    if (ipsecpt_enable == QCMAP_LAN_INVALID)
    {
      LOG_MSG_ERROR("Error in retrieving ipsecpt_enable value for profile handle: %d",
                    profile_handle, 0, 0);
      return false;
    }

    if (ipsecpt_enable == 0)
    {
      LOG_MSG_ERROR("IPSec VPN Passthrough already disabled on the PDN: %d", profile_handle, 0, 0);
      *qmi_err_num = QMI_ERR_NO_EFFECT_V01;
      return false;
    }

    /* Disable IPSec VPN Passthrough */
    memset(cmd, 0, QCMAP_MAX_COMMAND_LEN);
    snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "%s set_vpnpt %d %d %d %s", NAT_ALG_VPN_CONFIG_FILE,
             profile_handle, profile_idx, enable_state, IPSEC_VPN_PASSTHROUGH_V4);
    ds_system_call(cmd, strlen(cmd));
  } /* End disable */
  else
  {
    LOG_MSG_ERROR("Unknown state value passed. Exiting!", 0, 0, 0);
    *qmi_err_num = QMI_ERR_INTERNAL_V01;
    return false;
  }
  return true;
} /* End SetIPSECVpnPassthrough */

/*===========================================================================
FUNCTION GetIPSECVpnPassthrough()
===========================================================================*/
/** @ingroup qcmap_get_ipsec_vpn_passthrough_config

  gets IPSec VPN Passthrough enable flag

  @datatypes
  qcmap_lan_ipsec_vpn_passthrough_mode_enum

  @param[in]      enable_state              IPsec VPN Passthrough Enable state \n
  @param[in]      profile_handle            Current Profile Handle

  @return
  TRUE -- Success \n
  FALSE -- Failure \n
*/
/*=========================================================================*/
boolean QCMAP_LAN_Client::GetIPSECVpnPassthrough
(
  qcmap_lan_ipsec_vpn_passthrough_mode_enum *enable_state,
  const uint32_t profile_handle,
  qmi_error_type_v01 *qmi_err_num
)
{
  char cmd[QCMAP_MAX_COMMAND_LEN] = {0};
  char result[QCMAP_MAX_SCAN_SIZE] = {0};
  int profile_idx = -1;
  int err_num = 0;

  LOG_MSG_INFO1("\nGetIPSECVpnPassthrough in API",0,0,0);

  /* Calling Utility function to check if wan rules are present in uci database */
  if (!CheckIfNetworkRulesExist(NETWORK_WAN, profile_handle))
  {
    LOG_MSG_ERROR("Failed to validate network wan rules presence in uci database"
                  "for profile: %d", profile_handle, 0, 0);
    return false;
  }

  /* Get the profile index in qcmap_lan */
  profile_idx = GetProfileIndex(profile_handle);
  if (profile_idx == QCMAP_LAN_INVALID)
  {
    /* indicates profile_idx not found */
    LOG_MSG_ERROR(" Profile index for profile: %d not found in qcmap_lan. Exiting!",
                  profile_handle, 0, 0);
    return false;
  }

  /* Get ipsecpt_enable state */
  memset(cmd, 0, QCMAP_MAX_COMMAND_LEN);
  memset(result, 0, QCMAP_MAX_SCAN_SIZE);
  snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "%s qcmap_lan.@profile[%d].ipsecpt_enable",
           UCI_GET_COMMAND, profile_idx);
  if (!ExecuteSystemCmd((const char *)cmd, result,
                       sizeof(result),&err_num))
  {
    /* Indicates ipsecpt_enable not present */
    if (err_num == EINVAL)
    {
      *qmi_err_num = QMI_ERR_NONE_V01;
      LOG_MSG_ERROR("IPSec VPN Passthrough is not configured errno=%d config not found", err_num, 0, 0);
    }
    else
      LOG_MSG_ERROR("IPSec VPN Passthrough enable state for PDN: %d not found!", profile_handle, 0, 0);
    return false;
  }
  *enable_state = atoi(result);

  return true;
} /* End GetIPSECVpnPassthrough */

/*=====================================================================
  FUNCTION GetActiveIpsecVpnPt
======================================================================*/
/*!
@brief
  - Get ipsecpt_enable value of passed profile number in qcmap_lan database

@return
  ipsecpt_enable

@note
  - returns ipsecpt_enable value of passed profile number in qcmap_lan
   database

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
int GetActiveIpsecVpnPt
(
  const uint32_t profile_idx
)
{
  char cmd[QCMAP_MAX_COMMAND_LEN] = {0};
  char result[QCMAP_MAX_SCAN_SIZE] = {0};
  int ipsecpt_enable = -1;

  snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "%s qcmap_lan.@profile[%d].ipsecpt_enable",
             UCI_GET_COMMAND, profile_idx);
  if (!ExecuteSystemCmd((const char *)cmd, result,
                      sizeof(result)))
  {
    LOG_MSG_ERROR("Failed to get ipsecpt_enable value from qcmap_lan database for"
                  " profile idx:%d", profile_idx, 0, 0);
    return QCMAP_LAN_INVALID;
  }
  ipsecpt_enable = atoi(result);
  return ipsecpt_enable;
} /* End GetActiveIpsecVpnPt */

/*===========================================================================
FUNCTION SetPPTPVpnPassthrough()
===========================================================================*/
/** @ingroup qcmap_set_pptp_vpn_passthrough_config

  sets PPTP VPN Passthrough

  @datatypes
  qcmap_lan_pptp_vpn_passthrough_mode_enum

  @param[in]      enable_state              PPTP VPN Passthrough Enable state \n
  @param[in]      default_handle            Default Profile handle \n
  @param[in]      profile_handle            Current Profile Handle

  @return
  TRUE -- Success \n
  FALSE -- Failure \n
*/
/*=========================================================================*/
boolean QCMAP_LAN_Client::SetPPTPVpnPassthrough
(
  qcmap_lan_pptp_vpn_passthrough_mode_enum enable_state,
  const uint32_t default_handle,
  const uint32_t profile_handle,
  qmi_error_type_v01 *qmi_err_num
)
{
  char cmd[QCMAP_MAX_COMMAND_LEN] = {0};
  char result[QCMAP_MAX_SCAN_SIZE] = {0};
  int pptppt_enable = -1, profile_idx = -1;

  /* Calling Utility function to check if wan rules are present in uci database */
  if (!CheckIfNetworkRulesExist(NETWORK_WAN, profile_handle))
  {
    LOG_MSG_ERROR("Failed to validate network wan rules presence in uci database"
                  "for profile: %d", profile_handle, 0, 0);
    return false;
  }

  /* Check enable_state */
  if (enable_state == QCMAP_LAN_PPTP_VPN_PASSTHROUGH_MODE_UP)
  {
    /* Get the profile index in qcmap_lan */
    profile_idx = GetProfileIndex(profile_handle);
    if (profile_idx == QCMAP_LAN_INVALID)
    {
      /* indicates profile_idx not found */
      LOG_MSG_ERROR(" Profile index for profile: %d not found in qcmap_lan. Exiting!",
                    profile_handle, 0, 0);
      return false;
    }

    /* Check if pptppt_enable is already set for wan<Current_Profile> */
    pptppt_enable = GetActivePptpVpnPt(profile_idx);
    if (pptppt_enable != QCMAP_LAN_INVALID)
    {
      if (pptppt_enable == 1)
      {
        LOG_MSG_ERROR("PPTP VPN Passthrough already active on the PDN: %d.",
                       profile_handle, 0, 0);
        *qmi_err_num = QMI_ERR_NO_EFFECT_V01;
        return false;
      }
    }

    /* Enable PPTP VPN Passthrough */
    memset(cmd, 0, QCMAP_MAX_COMMAND_LEN);
    snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "%s set_vpnpt %d %d %d %s", NAT_ALG_VPN_CONFIG_FILE,
             profile_handle, profile_idx, enable_state, PPTP_VPN_PASSTHROUGH_V4);
    ds_system_call(cmd, strlen(cmd));
  } /* End enable */
  else if (enable_state == QCMAP_LAN_PPTP_VPN_PASSTHROUGH_MODE_DOWN)
  {
    /* Get the profile index in qcmap_lan */
    profile_idx = GetProfileIndex(profile_handle);
    if (profile_idx == QCMAP_LAN_INVALID)
    {
      /* indicates profile_idx not found */
      LOG_MSG_ERROR(" Profile index for profile: %d not found in qcmap_lan. Exiting!",
                    profile_handle, 0, 0);
      return false;
    }

    /* Check if pptppt_enable is already set for wan<Current_Profile> */
    pptppt_enable = GetActivePptpVpnPt(profile_idx);
    if (pptppt_enable == QCMAP_LAN_INVALID)
    {
      LOG_MSG_ERROR("Error in retrieving pptppt_enable value for profile handle: %d",
                    profile_handle, 0, 0);
      return false;
    }

    if (pptppt_enable == 0)
    {
      LOG_MSG_ERROR("PPTP VPN Passthrough already disabled on the PDN: %d", profile_handle, 0, 0);
      *qmi_err_num = QMI_ERR_NO_EFFECT_V01;
      return false;
    }

    /* Disable PPTP VPN Passthrough */
    memset(cmd, 0, QCMAP_MAX_COMMAND_LEN);
    snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "%s set_vpnpt %d %d %d %s", NAT_ALG_VPN_CONFIG_FILE,
             profile_handle, profile_idx, enable_state, PPTP_VPN_PASSTHROUGH_V4);
    ds_system_call(cmd, strlen(cmd));
  } /* End disable */
  else
  {
    LOG_MSG_ERROR("Unknown state value passed. Exiting!", 0, 0, 0);
    return false;
  }
  return true;
} /* End SetPPTPVpnPassthrough */

/*===========================================================================
FUNCTION GetPPTPVpnPassthrough()
===========================================================================*/
/** @ingroup qcmap_get_pptp_vpn_passthrough_config

  gets PPTP VPN Passthrough enable flag

  @datatypes
  qcmap_lan_pptp_vpn_passthrough_mode_enum

  @param[in]      enable_state              PPTP VPN Passthrough Enable state \n
  @param[in]      profile_handle            Current Profile Handle

  @return
  TRUE -- Success \n
  FALSE -- Failure \n
*/
/*=========================================================================*/
boolean QCMAP_LAN_Client::GetPPTPVpnPassthrough
(
  qcmap_lan_pptp_vpn_passthrough_mode_enum *enable_state,
  const uint32_t profile_handle,
  qmi_error_type_v01 *qmi_err_num
)
{
  char cmd[QCMAP_MAX_COMMAND_LEN] = {0};
  char result[QCMAP_MAX_SCAN_SIZE] = {0};
  int profile_idx = -1;
  int err_num = 0;

  LOG_MSG_INFO1("\nGetPPTPVpnPassthrough in API",0,0,0);

  /* Calling Utility function to check if wan rules are present in uci database */
  if (!CheckIfNetworkRulesExist(NETWORK_WAN, profile_handle))
  {
    LOG_MSG_ERROR("Failed to validate network wan rules presence in uci database"
                  "for profile: %d", profile_handle, 0, 0);
    return false;
  }

  /* Get the profile index in qcmap_lan */
  profile_idx = GetProfileIndex(profile_handle);
  if (profile_idx == QCMAP_LAN_INVALID)
  {
    /* indicates profile_idx not found */
    LOG_MSG_ERROR(" Profile index for profile: %d not found in qcmap_lan. Exiting!",
                  profile_handle, 0, 0);
    return false;
  }

  /* Get pptppt_enable state */
  memset(cmd, 0, QCMAP_MAX_COMMAND_LEN);
  memset(result, 0, QCMAP_MAX_SCAN_SIZE);
  snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "%s qcmap_lan.@profile[%d].pptppt_enable",
           UCI_GET_COMMAND, profile_idx);
  if (!ExecuteSystemCmd((const char *)cmd, result,
                       sizeof(result),&err_num))
  {
    /* Indicates pptppt_enable not present */
    if (err_num == EINVAL)
    {
      *qmi_err_num = QMI_ERR_NONE_V01;
       LOG_MSG_ERROR("PPTP VPN Passthrough is not configured errno=%d config not found", err_num, 0, 0);
    }
    else
      LOG_MSG_ERROR("PPTP VPN Passthrough enable state for PDN: %d not found!", profile_handle, 0, 0);
    return false;
  }
  *enable_state = atoi(result);

  return true;
} /* End GetPPTPVpnPassthrough */

/*=====================================================================
  FUNCTION GetActivePptpVpnPt
======================================================================*/
/*!
@brief
  - Get pptppt_enable value of passed profile number in qcmap_lan database

@return
  pptppt_enable

@note
  - returns pptppt_enable value of passed profile number in qcmap_lan
   database

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
int GetActivePptpVpnPt
(
  const uint32_t profile_idx
)
{
  char cmd[QCMAP_MAX_COMMAND_LEN] = {0};
  char result[QCMAP_MAX_SCAN_SIZE] = {0};
  int pptppt_enable = -1;

  snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "%s qcmap_lan.@profile[%d].pptppt_enable",
             UCI_GET_COMMAND, profile_idx);
  if (!ExecuteSystemCmd((const char *)cmd, result,
                      sizeof(result)))
  {
    LOG_MSG_ERROR("Failed to get pptppt_enable value from qcmap_lan database for"
                  " profile idx:%d", profile_idx, 0, 0);
    return QCMAP_LAN_INVALID;
  }
  pptppt_enable = atoi(result);
  return pptppt_enable;
} /* End GetActivePptpVpnPt */

/*===========================================================================
FUNCTION SetL2TPIPSECVpnPassthrough()
===========================================================================*/
/** @ingroup qcmap_set_l2tp_ipsec_vpn_passthrough_config

  sets L2TP/IPSec VPN Passthrough

  @datatypes
  qcmap_lan_l2tp_ipsec_vpn_passthrough_mode_enum

  @param[in]      enable_state              L2TP/IPSec VPN Passthrough Enable state \n
  @param[in]      default_handle            Default Profile handle \n
  @param[in]      profile_handle            Current Profile Handle

  @return
  TRUE -- Success \n
  FALSE -- Failure \n
*/
/*=========================================================================*/
boolean QCMAP_LAN_Client::SetL2TPIPSECVpnPassthrough
(
  qcmap_lan_l2tp_ipsec_vpn_passthrough_mode_enum enable_state,
  const uint32_t default_handle,
  const uint32_t profile_handle,
  qmi_error_type_v01 *qmi_err_num
)
{
  char cmd[QCMAP_MAX_COMMAND_LEN] = {0};
  char result[QCMAP_MAX_SCAN_SIZE] = {0};
  int ipsecpt_enable = -1, profile_idx = -1;

  /* Calling Utility function to check if wan rules are present in uci database */
  if (!CheckIfNetworkRulesExist(NETWORK_WAN, profile_handle))
  {
    LOG_MSG_ERROR("Failed to validate network wan rules presence in uci database"
                  "for profile: %d", profile_handle, 0, 0);
    return false;
  }

  /* Check enable_state */
  if (enable_state == QCMAP_LAN_L2TP_IPSEC_VPN_PASSTHROUGH_MODE_UP)
  {
    /* Get the profile index in qcmap_lan */
    profile_idx = GetProfileIndex(profile_handle);
    if (profile_idx == QCMAP_LAN_INVALID)
    {
      /* indicates profile_idx not found */
      LOG_MSG_ERROR(" Profile index for profile: %d not found in qcmap_lan. Exiting!",
                    profile_handle, 0, 0);
      return false;
    }

    /* Check if ipsecpt_enable is already set for wan<Current_Profile> */
    ipsecpt_enable = GetActiveIpsecVpnPt(profile_idx);
    if (ipsecpt_enable != QCMAP_LAN_INVALID)
    {
      if (ipsecpt_enable == 1)
      {
        LOG_MSG_ERROR("L2TP/IPSec VPN Passthrough already active on the PDN: %d.",
                       profile_handle, 0, 0);
        return false;
      }
    }

    /* Enable L2TP/IPSec VPN Passthrough */
    memset(cmd, 0, QCMAP_MAX_COMMAND_LEN);
    snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "%s set_vpnpt %d %d %d %s", NAT_ALG_VPN_CONFIG_FILE,
             profile_handle, profile_idx, enable_state, L2TPIPSEC_VPN_PASSTHROUGH_V4);
    ds_system_call(cmd, strlen(cmd));
  } /* End enable */
  else if (enable_state == QCMAP_LAN_PPTP_VPN_PASSTHROUGH_MODE_DOWN)
  {
    /* Get the profile index in qcmap_lan */
    profile_idx = GetProfileIndex(profile_handle);
    if (profile_idx == QCMAP_LAN_INVALID)
    {
      /* indicates profile_idx not found */
      LOG_MSG_ERROR(" Profile index for profile: %d not found in qcmap_lan. Exiting!",
                    profile_handle, 0, 0);
      return false;
    }

    /* Check if ipsecpt_enable is already set for wan<Current_Profile> */
    ipsecpt_enable = GetActiveIpsecVpnPt(profile_idx);
    if (ipsecpt_enable == QCMAP_LAN_INVALID)
    {
      LOG_MSG_ERROR("Error in retrieving ipsecpt_enable value for profile handle: %d",
                    profile_handle, 0, 0);
      return false;
    }

    if (ipsecpt_enable == 0)
    {
      LOG_MSG_ERROR("L2TP/IPSec VPN Passthrough already disabled on the PDN: %d", profile_handle, 0, 0);
      return false;
    }

    /* Disable L2TP/IPSec VPN Passthrough */
    memset(cmd, 0, QCMAP_MAX_COMMAND_LEN);
    snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "%s set_vpnpt %d %d %d %s", NAT_ALG_VPN_CONFIG_FILE,
             profile_handle, profile_idx, enable_state, L2TPIPSEC_VPN_PASSTHROUGH_V4);
    ds_system_call(cmd, strlen(cmd));
  } /* End disable */
  else
  {
    LOG_MSG_ERROR("Unknown state value passed. Exiting!", 0, 0, 0);
    return false;
  }
  return true;
} /* End SetL2TPIPSECVpnPassthrough */

/*===========================================================================
FUNCTION GetL2TPIPSECVpnPassthrough()
===========================================================================*/
/** @ingroup qcmap_get_l2tp_ipsec_vpn_passthrough_config

  gets L2TP/IPSec VPN Passthrough enable flag

  @datatypes
  qcmap_lan_l2tp_ipsec_vpn_passthrough_mode_enum

  @param[in]      enable_state              L2TP/IPSec VPN Passthrough Enable state \n
  @param[in]      profile_handle            Current Profile Handle

  @return
  TRUE -- Success \n
  FALSE -- Failure \n
*/
/*=========================================================================*/
boolean QCMAP_LAN_Client::GetL2TPIPSECVpnPassthrough
(
  qcmap_lan_l2tp_ipsec_vpn_passthrough_mode_enum *enable_state,
  const uint32_t profile_handle,
  qmi_error_type_v01 *qmi_err_num
)
{
  char cmd[QCMAP_MAX_COMMAND_LEN] = {0};
  char result[QCMAP_MAX_SCAN_SIZE] = {0};
  int profile_idx = -1;
  int err_num = 0;

  LOG_MSG_INFO1("\nGetL2TPIPSECVpnPassthrough in API",0,0,0);

  /* Calling Utility function to check if wan rules are present in uci database */
  if (!CheckIfNetworkRulesExist(NETWORK_WAN, profile_handle))
  {
    LOG_MSG_ERROR("Failed to validate network wan rules presence in uci database"
                  "for profile: %d", profile_handle, 0, 0);
    return false;
  }

  /* Get the profile index in qcmap_lan */
  profile_idx = GetProfileIndex(profile_handle);
  if (profile_idx == QCMAP_LAN_INVALID)
  {
    /* indicates profile_idx not found */
    LOG_MSG_ERROR(" Profile index for profile: %d not found in qcmap_lan. Exiting!",
                  profile_handle, 0, 0);
    return false;
  }

  /* Get ipsecpt_enable state */
  memset(cmd, 0, QCMAP_MAX_COMMAND_LEN);
  memset(result, 0, QCMAP_MAX_SCAN_SIZE);
  snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "%s qcmap_lan.@profile[%d].ipsecpt_enable",
           UCI_GET_COMMAND, profile_idx);
  if (!ExecuteSystemCmd((const char *)cmd, result,
                       sizeof(result),&err_num))
  {
    /* Indicates ipsecpt_enable not present */
    if (err_num == EINVAL)
    {
      *qmi_err_num = QMI_ERR_NONE_V01;
      LOG_MSG_ERROR("L2TP/IPSec VPN Passthrough is not configured errno=%d config not found", err_num, 0, 0);
    }
    else
      LOG_MSG_ERROR("L2TP/IPSec VPN Passthrough enable state for PDN: %d not found!", profile_handle, 0, 0);
    return false;
  }
  *enable_state = atoi(result);

  return true;
} /* End GetL2TPIPSECVpnPassthrough */

/*===========================================================================
FUNCTION SetIPSECVpnPassthroughIpv6()
===========================================================================*/
/** @ingroup qcmap_set_ipsec_vpn_passthrough_v6_config

  sets IPV6 IPSec VPN Passthrough

  @datatypes
  qcmap_lan_ipsec_vpn_passthrough_v6_mode_enum

  @param[in]      enable_state              IPV6 IPsec VPN Passthrough Enable state \n
  @param[in]      default_handle            Default Profile handle \n
  @param[in]      profile_handle            Current Profile Handle

  @return
  TRUE -- Success \n
  FALSE -- Failure \n
*/
/*=========================================================================*/
boolean QCMAP_LAN_Client::SetIPSECVpnPassthroughIpv6
(
  qcmap_lan_ipsec_vpn_passthrough_v6_mode_enum enable_state,
  const uint32_t default_handle,
  const uint32_t profile_handle,
  qmi_error_type_v01 * qmi_err_num
)
{
  char cmd[QCMAP_MAX_COMMAND_LEN] = {0};
  char result[QCMAP_MAX_SCAN_SIZE] = {0};
  int ipsecptv6_enable = -1, profile_idx = -1;

  /* Calling Utility function to check if wan rules are present in uci database */
  if (!CheckIfNetworkRulesExist(NETWORK_WAN, profile_handle))
  {
    LOG_MSG_ERROR("Failed to validate network wan rules presence in uci database"
                  "for profile: %d", profile_handle, 0, 0);
    *qmi_err_num = QMI_ERR_INVALID_ARG_V01;
    return false;
  }

  /* Check enable_state */
  if (enable_state == QCMAP_LAN_IPSEC_VPN_PASSTHROUGH_V6_MODE_UP)
  {
    /* Get the profile index in qcmap_lan */
    profile_idx = GetProfileIndex(profile_handle);
    if (profile_idx == QCMAP_LAN_INVALID)
    {
      /* indicates profile_idx not found */
      LOG_MSG_ERROR(" Profile index for profile: %d not found in qcmap_lan. Exiting!",
                    profile_handle, 0, 0);
      *qmi_err_num = QMI_ERR_INVALID_ARG_V01;
      return false;
    }

    /* Check if ipsecptv6_enable is already set for wan<Current_Profile> */
    ipsecptv6_enable = GetActiveIpsecVpnPtIpv6(profile_idx);
    if (ipsecptv6_enable != QCMAP_LAN_INVALID)
    {
      if (ipsecptv6_enable == 1)
      {
        LOG_MSG_ERROR("IPV6 IPSec VPN Passthrough already active on the PDN: %d.",
                       profile_handle, 0, 0);
        *qmi_err_num = QMI_ERR_NO_EFFECT_V01;
        return false;
      }
    }

    /* Enable IPV6 IPSec VPN Passthrough */
    memset(cmd, 0, QCMAP_MAX_COMMAND_LEN);
    snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "%s set_vpnpt %d %d %d %s", NAT_ALG_VPN_CONFIG_FILE,
             profile_handle, profile_idx, enable_state, IPSEC_VPN_PASSTHROUGH_V6);
    ds_system_call(cmd, strlen(cmd));
  } /* End enable */
  else if (enable_state == QCMAP_LAN_IPSEC_VPN_PASSTHROUGH_V6_MODE_DOWN)
  {
    /* Get the profile index in qcmap_lan */
    profile_idx = GetProfileIndex(profile_handle);
    if (profile_idx == QCMAP_LAN_INVALID)
    {
      /* indicates profile_idx not found */
      LOG_MSG_ERROR(" Profile index for profile: %d not found in qcmap_lan. Exiting!",
                    profile_handle, 0, 0);
      *qmi_err_num = QMI_ERR_INTERNAL_V01;
      return false;
    }

    /* Check if ipsecptv6_enable is already set for wan<Current_Profile> */
    ipsecptv6_enable = GetActiveIpsecVpnPtIpv6(profile_idx);
    if (ipsecptv6_enable == QCMAP_LAN_INVALID)
    {
      LOG_MSG_ERROR("Error in retrieving ipsecptv6_enable value for profile handle: %d",
                    profile_handle, 0, 0);
      return false;
    }

    if (ipsecptv6_enable == 0)
    {
      LOG_MSG_ERROR("IPV6 IPSec VPN Passthrough already disabled on the PDN: %d", profile_handle, 0, 0);
      *qmi_err_num = QMI_ERR_NO_EFFECT_V01;
      return false;
    }

    /* Disable IPV6 IPSec VPN Passthrough */
    memset(cmd, 0, QCMAP_MAX_COMMAND_LEN);
    snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "%s set_vpnpt %d %d %d %s", NAT_ALG_VPN_CONFIG_FILE,
             profile_handle, profile_idx, enable_state, IPSEC_VPN_PASSTHROUGH_V6);
    ds_system_call(cmd, strlen(cmd));
  } /* End disbale */
  else
  {
    LOG_MSG_ERROR("Unknown state value passed. Exiting!", 0, 0, 0);
    return false;
  }
  return true;
} /* End SetIPSECVpnPassthroughIpv6 */

/*===========================================================================
FUNCTION GetIPSECVpnPassthroughIpv6()
===========================================================================*/
/** @ingroup qcmap_get_ipsec_vpn_passthrough_v6_config

  gets IPV6 IPSec VPN Passthrough enable flag

  @datatypes
  qcmap_lan_ipsec_vpn_passthrough_v6_mode_enum

  @param[in]      enable_state              IPV6 IPsec VPN Passthrough Enable state \n
  @param[in]      profile_handle            Current Profile Handle

  @return
  TRUE -- Success \n
  FALSE -- Failure \n
*/
/*=========================================================================*/
boolean QCMAP_LAN_Client::GetIPSECVpnPassthroughIpv6
(
  qcmap_lan_ipsec_vpn_passthrough_v6_mode_enum *enable_state,
  const uint32_t profile_handle,
  qmi_error_type_v01 *qmi_err_num
)
{
  char cmd[QCMAP_MAX_COMMAND_LEN] = {0};
  char result[QCMAP_MAX_SCAN_SIZE] = {0};
  int profile_idx = -1;
  int err_num = 0;

  LOG_MSG_INFO1("\nGetIPSECVpnPassthroughIpv6 in API",0,0,0);

  /* Calling Utility function to check if wan rules are present in uci database */
  if (!CheckIfNetworkRulesExist(NETWORK_WAN, profile_handle))
  {
    LOG_MSG_ERROR("Failed to validate network wan rules presence in uci database"
                  "for profile: %d", profile_handle, 0, 0);
    return false;
  }

  /* Get the profile index in qcmap_lan */
  profile_idx = GetProfileIndex(profile_handle);
  if (profile_idx == QCMAP_LAN_INVALID)
  {
    /* indicates profile_idx not found */
    LOG_MSG_ERROR(" Profile index for profile: %d not found in qcmap_lan. Exiting!",
                  profile_handle, 0, 0);
    return false;
  }

  /* Get ipsecptv6_enable state */
  memset(cmd, 0, QCMAP_MAX_COMMAND_LEN);
  memset(result, 0, QCMAP_MAX_SCAN_SIZE);
  snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "%s qcmap_lan.@profile[%d].ipsecptv6_enable",
           UCI_GET_COMMAND, profile_idx);
  if (!ExecuteSystemCmd((const char *)cmd, result,
                       sizeof(result),&err_num))
  {
    /* Indicates ipsecptv6_enable not present */
    if (err_num == EINVAL)
    {
      *qmi_err_num = QMI_ERR_NONE_V01;
      LOG_MSG_ERROR("IPPT is not configured errno=%d config not found", err_num, 0, 0);
    }
    else
      LOG_MSG_ERROR("IPV6 IPSec VPN Passthrough enable state for PDN: %d not found!", profile_handle, 0, 0);
    return false;
  }
  *enable_state = atoi(result);

  return true;
} /* End GetIPSECVpnPassthroughIpv6 */

/*=====================================================================
  FUNCTION GetActiveIpsecVpnPtIpv6
======================================================================*/
/*!
@brief
  - Get ipsecptv6_enable value of passed profile number in qcmap_lan database

@return
  ipsecptv6_enable

@note
  - returns ipsecptv6_enable value of passed profile number in qcmap_lan
   database

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
int GetActiveIpsecVpnPtIpv6
(
  const uint32_t profile_idx
)
{
  char cmd[QCMAP_MAX_COMMAND_LEN] = {0};
  char result[QCMAP_MAX_SCAN_SIZE] = {0};
  int ipsecptv6_enable = -1;

  snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "%s qcmap_lan.@profile[%d].ipsecptv6_enable",
             UCI_GET_COMMAND, profile_idx);
  if (!ExecuteSystemCmd((const char *)cmd, result,
                      sizeof(result)))
  {
    LOG_MSG_ERROR("Failed to get ipsecptv6_enable value from qcmap_lan database for"
                  " profile idx:%d", profile_idx, 0, 0);
    return QCMAP_LAN_INVALID;
  }
  ipsecptv6_enable = atoi(result);
  return ipsecptv6_enable;
} /* End GetActiveIpsecVpnPtIpv6 */

/*===========================================================================
FUNCTION SetPPTPVpnPassthroughIpv6()
===========================================================================*/
/** @ingroup qcmap_set_pptp_vpn_passthrough_v6_config

  sets IPV6 PPTP VPN Passthrough

  @datatypes
  qcmap_lan_pptp_vpn_passthrough_v6_mode_enum

  @param[in]      enable_state              IPV6 PPTP VPN Passthrough Enable state \n
  @param[in]      default_handle            Default Profile handle \n
  @param[in]      profile_handle            Current Profile Handle

  @return
  TRUE -- Success \n
  FALSE -- Failure \n
*/
/*=========================================================================*/
boolean QCMAP_LAN_Client::SetPPTPVpnPassthroughIpv6
(
  qcmap_lan_pptp_vpn_passthrough_v6_mode_enum enable_state,
  const uint32_t default_handle,
  const uint32_t profile_handle,
  qmi_error_type_v01 *qmi_err_num
)
{
  char cmd[QCMAP_MAX_COMMAND_LEN] = {0};
  char result[QCMAP_MAX_SCAN_SIZE] = {0};
  int pptpptv6_enable = -1, profile_idx = -1;

  /* Calling Utility function to check if wan rules are present in uci database */
  if (!CheckIfNetworkRulesExist(NETWORK_WAN, profile_handle))
  {
    LOG_MSG_ERROR("Failed to validate network wan rules presence in uci database"
                  "for profile: %d", profile_handle, 0, 0);
    return false;
  }

  /* Check enable_state */
  if (enable_state == QCMAP_LAN_PPTP_VPN_PASSTHROUGH_V6_MODE_UP)
  {
    /* Get the profile index in qcmap_lan */
    profile_idx = GetProfileIndex(profile_handle);
    if (profile_idx == QCMAP_LAN_INVALID)
    {
      /* indicates profile_idx not found */
      LOG_MSG_ERROR(" Profile index for profile: %d not found in qcmap_lan. Exiting!",
                    profile_handle, 0, 0);
      return false;
    }

    /* Check if pptpptv6_enable is already set for wan<Current_Profile> */
    pptpptv6_enable = GetActivePptpVpnPtIpv6(profile_idx);
    if (pptpptv6_enable != QCMAP_LAN_INVALID)
    {
      if (pptpptv6_enable == 1)
      {
        LOG_MSG_ERROR("IPV6 PPTP VPN Passthrough already active on the PDN: %d.",
                       profile_handle, 0, 0);
        *qmi_err_num = QMI_ERR_NO_EFFECT_V01;
        return false;
      }
    }

    /* Enable IPV6 PPTP VPN Passthrough */
    memset(cmd, 0, QCMAP_MAX_COMMAND_LEN);
    snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "%s set_vpnpt %d %d %d %s", NAT_ALG_VPN_CONFIG_FILE,
             profile_handle, profile_idx, enable_state, PPTP_VPN_PASSTHROUGH_V6);
    ds_system_call(cmd, strlen(cmd));
  } /* End enable */
  else if (enable_state == QCMAP_LAN_PPTP_VPN_PASSTHROUGH_V6_MODE_DOWN)
  {
    /* Get the profile index in qcmap_lan */
    profile_idx = GetProfileIndex(profile_handle);
    if (profile_idx == QCMAP_LAN_INVALID)
    {
      /* indicates profile_idx not found */
      LOG_MSG_ERROR(" Profile index for profile: %d not found in qcmap_lan. Exiting!",
                    profile_handle, 0, 0);
      return false;
    }

    /* Check if pptpptv6_enable is already set for wan<Current_Profile> */
    pptpptv6_enable = GetActivePptpVpnPtIpv6(profile_idx);
    if (pptpptv6_enable == QCMAP_LAN_INVALID)
    {
      LOG_MSG_ERROR("Error in retrieving pptpptv6_enable value for profile handle: %d",
                    profile_handle, 0, 0);
      return false;
    }

    if (pptpptv6_enable == 0)
    {
      LOG_MSG_ERROR("IPV6 PPTP VPN Passthrough already disabled on the PDN: %d", profile_handle, 0, 0);
      *qmi_err_num = QMI_ERR_NO_EFFECT_V01;
      return false;
    }

    /* Disable IPV6 PPTP VPN Passthrough */
    memset(cmd, 0, QCMAP_MAX_COMMAND_LEN);
    snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "%s set_vpnpt %d %d %d %s", NAT_ALG_VPN_CONFIG_FILE,
             profile_handle, profile_idx, enable_state, PPTP_VPN_PASSTHROUGH_V6);
    ds_system_call(cmd, strlen(cmd));
  } /* End disable */
  else
  {
    LOG_MSG_ERROR("Unknown state value passed. Exiting!", 0, 0, 0);
    return false;
  }
  return true;
} /* End SetPPTPVpnPassthroughIpv6 */

/*===========================================================================
FUNCTION GetPPTPVpnPassthroughIpv6()
===========================================================================*/
/** @ingroup qcmap_get_pptp_vpn_passthrough_v6_config

  gets IPV6 PPTP VPN Passthrough enable flag

  @datatypes
  qcmap_lan_pptp_vpn_passthrough_v6_mode_enum

  @param[in]      enable_state              IPV6 PPTP VPN Passthrough Enable state \n
  @param[in]      profile_handle            Current Profile Handle

  @return
  TRUE -- Success \n
  FALSE -- Failure \n
*/
/*=========================================================================*/
boolean QCMAP_LAN_Client::GetPPTPVpnPassthroughIpv6
(
  qcmap_lan_pptp_vpn_passthrough_v6_mode_enum *enable_state,
  const uint32_t profile_handle,
  qmi_error_type_v01 *qmi_err_num
)
{
  char cmd[QCMAP_MAX_COMMAND_LEN] = {0};
  char result[QCMAP_MAX_SCAN_SIZE] = {0};
  int profile_idx = -1;
  int err_num = 0;

  LOG_MSG_INFO1("\nGetPPTPVpnPassthroughIpv6 in API",0,0,0);

  /* Calling Utility function to check if wan rules are present in uci database */
  if (!CheckIfNetworkRulesExist(NETWORK_WAN, profile_handle))
  {
    LOG_MSG_ERROR("Failed to validate network wan rules presence in uci database"
                  "for profile: %d", profile_handle, 0, 0);
    return false;
  }

  /* Get the profile index in qcmap_lan */
  profile_idx = GetProfileIndex(profile_handle);
  if (profile_idx == QCMAP_LAN_INVALID)
  {
    /* indicates profile_idx not found */
    LOG_MSG_ERROR(" Profile index for profile: %d not found in qcmap_lan. Exiting!",
                  profile_handle, 0, 0);
    return false;
  }

  /* Get pptpptv6_enable state */
  memset(cmd, 0, QCMAP_MAX_COMMAND_LEN);
  memset(result, 0, QCMAP_MAX_SCAN_SIZE);
  snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "%s qcmap_lan.@profile[%d].pptpptv6_enable",
           UCI_GET_COMMAND, profile_idx);
  if (!ExecuteSystemCmd((const char *)cmd, result,
                       sizeof(result),&err_num))
  {
    /* Indicates pptpptv6_enable not present */
    if (err_num == EINVAL)
    {
      *qmi_err_num = QMI_ERR_NONE_V01;
      LOG_MSG_ERROR("IPPT is not configured errno=%d config not found", err_num, 0, 0);
    }
    else
      LOG_MSG_ERROR("IPV6 PPTP VPN Passthrough enable state for PDN: %d not found!", profile_handle, 0, 0);
    return false;
  }
  *enable_state = atoi(result);

  return true;
} /* End GetPPTPVpnPassthroughIpv6 */

/*=====================================================================
  FUNCTION GetActivePptpVpnPtIpv6
======================================================================*/
/*!
@brief
  - Get pptpptv6_enable value of passed profile number in qcmap_lan database

@return
  pptpptv6_enable

@note
  - returns pptpptv6_enable value of passed profile number in qcmap_lan
   database

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
int GetActivePptpVpnPtIpv6
(
  const uint32_t profile_idx
)
{
  char cmd[QCMAP_MAX_COMMAND_LEN] = {0};
  char result[QCMAP_MAX_SCAN_SIZE] = {0};
  int pptpptv6_enable = -1;

  snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "%s qcmap_lan.@profile[%d].pptpptv6_enable",
             UCI_GET_COMMAND, profile_idx);
  if (!ExecuteSystemCmd((const char *)cmd, result,
                      sizeof(result)))
  {
    LOG_MSG_ERROR("Failed to get pptpptv6_enable value from qcmap_lan database for"
                  " profile idx:%d", profile_idx, 0, 0);
    return QCMAP_LAN_INVALID;
  }
  pptpptv6_enable = atoi(result);
  return pptpptv6_enable;
} /* End GetActivePptpVpnPtIpv6 */

/*===========================================================================
FUNCTION SetL2TPIPSECVpnPassthroughIpv6()
===========================================================================*/
/** @ingroup qcmap_set_l2tp_ipsec_vpn_passthrough_v6_config

  sets IPV6 L2TP/IPSec VPN Passthrough

  @datatypes
  qcmap_lan_l2tp_ipsec_vpn_passthrough_v6_mode_enum

  @param[in]      enable_state              IPV6 L2TP/IPSec VPN Passthrough Enable state \n
  @param[in]      default_handle            Default Profile handle \n
  @param[in]      profile_handle            Current Profile Handle

  @return
  TRUE -- Success \n
  FALSE -- Failure \n
*/
/*=========================================================================*/
boolean QCMAP_LAN_Client::SetL2TPIPSECVpnPassthroughIpv6
(
  qcmap_lan_l2tp_ipsec_vpn_passthrough_v6_mode_enum enable_state,
  const uint32_t default_handle,
  const uint32_t profile_handle,
  qmi_error_type_v01 *qmi_err_num
)
{
  char cmd[QCMAP_MAX_COMMAND_LEN] = {0};
  char result[QCMAP_MAX_SCAN_SIZE] = {0};
  int ipsecptv6_enable = -1, profile_idx = -1;

  /* Calling Utility function to check if wan rules are present in uci database */
  if (!CheckIfNetworkRulesExist(NETWORK_WAN, profile_handle))
  {
    LOG_MSG_ERROR("Failed to validate network wan rules presence in uci database"
                  "for profile: %d", profile_handle, 0, 0);
    return false;
  }

  /* Check enable_state */
  if (enable_state == QCMAP_LAN_L2TP_IPSEC_VPN_PASSTHROUGH_V6_MODE_UP)
  {
    /* Get the profile index in qcmap_lan */
    profile_idx = GetProfileIndex(profile_handle);
    if (profile_idx == QCMAP_LAN_INVALID)
    {
      /* indicates profile_idx not found */
      LOG_MSG_ERROR(" Profile index for profile: %d not found in qcmap_lan. Exiting!",
                    profile_handle, 0, 0);
      return false;
    }

    /* Check if ipsecptv6_enable is already set for wan<Current_Profile> */
    ipsecptv6_enable = GetActiveIpsecVpnPtIpv6(profile_idx);
    if (ipsecptv6_enable != QCMAP_LAN_INVALID)
    {
      if (ipsecptv6_enable == 1)
      {
        LOG_MSG_ERROR("IPV6 L2TP/IPSec VPN Passthrough already active on the PDN: %d.",
                       profile_handle, 0, 0);
        *qmi_err_num = QMI_ERR_NO_EFFECT_V01;
        return false;
      }
    }

    /* Enable IPV6 L2TP/IPSec VPN Passthrough */
    memset(cmd, 0, QCMAP_MAX_COMMAND_LEN);
    snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "%s set_vpnpt %d %d %d %s", NAT_ALG_VPN_CONFIG_FILE,
             profile_handle, profile_idx, enable_state, L2TPIPSEC_VPN_PASSTHROUGH_V6);
    ds_system_call(cmd, strlen(cmd));
  } /* End enable */
  else if (enable_state == QCMAP_LAN_L2TP_IPSEC_VPN_PASSTHROUGH_V6_MODE_DOWN)
  {
    /* Get the profile index in qcmap_lan */
    profile_idx = GetProfileIndex(profile_handle);
    if (profile_idx == QCMAP_LAN_INVALID)
    {
      /* indicates profile_idx not found */
      LOG_MSG_ERROR(" Profile index for profile: %d not found in qcmap_lan. Exiting!",
                    profile_handle, 0, 0);
      return false;
    }

    /* Check if ipsecptv6_enable is already set for wan<Current_Profile> */
    ipsecptv6_enable = GetActiveIpsecVpnPtIpv6(profile_idx);
    if (ipsecptv6_enable == QCMAP_LAN_INVALID)
    {
      LOG_MSG_ERROR("Error in retrieving ipsecptv6_enable value for profile handle: %d",
                    profile_handle, 0, 0);
      return false;
    }

    if (ipsecptv6_enable == 0)
    {
      LOG_MSG_ERROR("IPV6 L2TP/IPSec VPN Passthrough already disabled on the PDN: %d", profile_handle, 0, 0);
      *qmi_err_num = QMI_ERR_NO_EFFECT_V01;
      return false;
    }

    /* Disable IPV6 L2TP/IPSec VPN Passthrough */
    memset(cmd, 0, QCMAP_MAX_COMMAND_LEN);
    snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "%s set_vpnpt %d %d %d %s", NAT_ALG_VPN_CONFIG_FILE,
             profile_handle, profile_idx, enable_state, L2TPIPSEC_VPN_PASSTHROUGH_V6);
    ds_system_call(cmd, strlen(cmd));
  } /* End disable */
  else
  {
    LOG_MSG_ERROR("Unknown state value passed. Exiting!", 0, 0, 0);
    return false;
  }
  return true;
} /* End SetL2TPIPSECVpnPassthroughIpv6 */

/*===========================================================================
FUNCTION GetL2TPIPSECVpnPassthroughIpv6()
===========================================================================*/
/** @ingroup qcmap_get_l2tp_ipsec_vpn_passthrough_v6_config

  gets IPV6 L2TP/IPSec VPN Passthrough enable flag

  @datatypes
  qcmap_lan_l2tp_ipsec_vpn_passthrough_v6_mode_enum

  @param[in]      enable_state              IPV6 L2TP/IPSec VPN Passthrough Enable state \n
  @param[in]      profile_handle            Current Profile Handle

  @return
  TRUE -- Success \n
  FALSE -- Failure \n
*/
/*=========================================================================*/
boolean QCMAP_LAN_Client::GetL2TPIPSECVpnPassthroughIpv6
(
  qcmap_lan_l2tp_ipsec_vpn_passthrough_v6_mode_enum *enable_state,
  const uint32_t profile_handle,
  qmi_error_type_v01 *qmi_err_num
)
{
  char cmd[QCMAP_MAX_COMMAND_LEN] = {0};
  char result[QCMAP_MAX_SCAN_SIZE] = {0};
  int profile_idx = -1;
  int err_num = 0;

  LOG_MSG_INFO1("\nGetL2TPIPSECVpnPassthroughIpv6 in API",0,0,0);

  /* Calling Utility function to check if wan rules are present in uci database */
  if (!CheckIfNetworkRulesExist(NETWORK_WAN, profile_handle))
  {
    LOG_MSG_ERROR("Failed to validate network wan rules presence in uci database"
                  "for profile: %d", profile_handle, 0, 0);
    return false;
  }

  /* Get the profile index in qcmap_lan */
  profile_idx = GetProfileIndex(profile_handle);
  if (profile_idx == QCMAP_LAN_INVALID)
  {
    /* indicates profile_idx not found */
    LOG_MSG_ERROR(" Profile index for profile: %d not found in qcmap_lan. Exiting!",
                  profile_handle, 0, 0);
    return false;
  }

  /* Get ipsecptv6_enable state */
  memset(cmd, 0, QCMAP_MAX_COMMAND_LEN);
  memset(result, 0, QCMAP_MAX_SCAN_SIZE);
  snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "%s qcmap_lan.@profile[%d].ipsecptv6_enable",
           UCI_GET_COMMAND, profile_idx);
  if (!ExecuteSystemCmd((const char *)cmd, result,
                       sizeof(result),&err_num))
  {
    /* Indicates ipsecptv6_enable not present */
    if (err_num == EINVAL)
    {
      *qmi_err_num = QMI_ERR_NONE_V01;
      LOG_MSG_ERROR("IPPT is not configured errno=%d config not found", err_num, 0, 0);
    }
    else
      LOG_MSG_ERROR("IPV6 L2TP/IPSec VPN Passthrough enable state for PDN: %d not found!", profile_handle, 0, 0);
    return false;
  }
  *enable_state = atoi(result);

  return true;
} /* End GetL2TPIPSECVpnPassthroughIpv6 */

/*===========================================================================
FUNCTION GetFireWallHandlesList
===========================================================================
@brief
  Get all the firewall Handle list for Mobile AP

@input
  wan_profile_handle - current wan profile handle
  *handlelist  - pointer to qcmap_msgr_get_firewall_handle_list_conf_t
  qmi_err_num

@return
  void

@dependencies
  usr to provide input

@sideefects
  None
=========================================================================*/
boolean QCMAP_LAN_Client::GetFireWallHandlesList
(
  uint32_t wan_profile_handle,
  qcmap_msgr_get_firewall_handle_list_conf_t *handlelist,
  qmi_error_type_v01 *qmi_err_num
)
{
  char command[MAX_COMMAND_STR_LEN] = {0};
  char result[QCMAP_MAX_SCAN_SIZE] = {0};
  char firewall_entry_name[QCMAP_MAX_SCAN_SIZE] = {0};
  int  profile_index;
  int  total_fw_rules;
  int  index=0;
  int  val;

  LOG_MSG_INFO1("Entering GetFireWallHandlesList function",0,0,0);
  /* Get the profile index in qcmap_lan */
  profile_index = GetProfileIndex(wan_profile_handle);
  if (profile_index == QCMAP_LAN_INVALID)
  {
    /* indicates profile_idx not found */
    LOG_MSG_ERROR(" Profile index for profile: %d not found in qcmap_lan. Exiting!",
                  wan_profile_handle, 0, 0);
    *qmi_err_num = QMI_ERR_INTERNAL_V01;
    return false;
  }

  //getting no of v4/v6 firewall rules added in that profile
  memset(result,0,QCMAP_MAX_SCAN_SIZE);
  if( handlelist->ip_family == IP_V4 )
  {
    snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh get qcmap_lan.@profile[%d].no_of_v4_fw_rules",profile_index);
    snprintf(firewall_entry_name, QCMAP_MAX_SCAN_SIZE, "FirewallV4-%d", wan_profile_handle);
  }
  else if( handlelist->ip_family == IP_V6 )
  {
    snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh get qcmap_lan.@profile[%d].no_of_v6_fw_rules",profile_index);
    snprintf(firewall_entry_name, QCMAP_MAX_SCAN_SIZE, "FirewallV6-%d", wan_profile_handle);
  }

  if (!ExecuteSystemCmd(command, result, sizeof(result)))
  {
    LOG_MSG_ERROR("failed to execute command : %s",command,0,0);
    *qmi_err_num = QMI_ERR_INTERNAL_V01;
    return false;
  }

  handlelist->num_of_entries = atoi(result);
  if(handlelist->num_of_entries == 0)
  {
    return true;
  }

  //getting total no of firewall rules added in that profile
  memset(result,0,QCMAP_MAX_SCAN_SIZE);
  snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh get qcmap_firewall.@firewall[0].no_of_rules");
  if (!ExecuteSystemCmd(command, result, sizeof(result)))
  {
    LOG_MSG_ERROR("failed to execute command : %s",command,0,0);
    *qmi_err_num = QMI_ERR_INTERNAL_V01;
    return false;
  }

  total_fw_rules = atoi(result);
  for(int i=0; i < total_fw_rules; i++)
  {

    memset(command,0,MAX_COMMAND_STR_LEN);
    memset(result,0,QCMAP_MAX_SCAN_SIZE);

    if (UciGetUtility(owrt_filename[static_cast<int>(config_file::QCMAP_FIREWALL)],
          "firewall_rule", 0, "name", result, i))
    {
      LOG_MSG_INFO1("Firewall rule name result = %s\n", result, 0, 0);
    }
    else
    {
      LOG_MSG_ERROR("UciGetUtility failed ",0,0,0);
      return false;
    }

    if(strncmp(result, firewall_entry_name, QCMAP_MAX_SCAN_SIZE)==0)
    {
      memset(result,0,QCMAP_MAX_SCAN_SIZE);

      if (UciGetUtility(owrt_filename[static_cast<int>(config_file::QCMAP_FIREWALL)],
            "firewall_rule", 0, "firewall_handle", result, i))
      {
        handlelist->handle_list[index++] = atoi(result);
      }
      else
      {
        LOG_MSG_ERROR("UciGetUtility failed ",0,0,0);
        return false;
      }

    }
  }
  return true;

}


/*===========================================================================
FUNCTION Get_Index_by_handle
===========================================================================
@brief
   get the index of a firewall entry using handle
@input
  wan_profile_handle
  qmi_err_num


@return
  boolean

@dependencies
  usr to provide input

@sideefects
  None
=========================================================================*/
int QCMAP_LAN_Client::Get_Index_by_handle
(
  uint32 handle,
  qmi_error_type_v01                *qmi_err_num
)
{
    char command[MAX_COMMAND_STR_LEN] = {0};
    char result[QCMAP_MAX_SCAN_SIZE] = {0};
    int  total_fw_rules;
    uint32 firewall_handle;

    LOG_MSG_INFO1("Entering Get_Index_by_handle function",0,0,0);
    //getting total no of firewall rules added in that profile
    memset(result,0,QCMAP_MAX_SCAN_SIZE);
    snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh get qcmap_firewall.@firewall[0].no_of_rules");

    if (!ExecuteSystemCmd(command, result, sizeof(result)))
    {
      LOG_MSG_ERROR("failed to execute command : %s",command,0,0);
      *qmi_err_num = QMI_ERR_INTERNAL_V01;
      return false;
    }

    total_fw_rules = atoi(result);
    for(int i = 0; i < total_fw_rules; i++)
    {
        memset(result,0,QCMAP_MAX_SCAN_SIZE);
        if (UciGetUtility(owrt_filename[static_cast<int>(config_file::QCMAP_FIREWALL)],
              "firewall_rule", 0, "firewall_handle", result, i))
        {
          firewall_handle = atoi(result);
          LOG_MSG_INFO1("firewall_handle from uci get = %d",firewall_handle,0,0);
          if(firewall_handle != handle)
          {
            continue;
          }
          else
          {
             return i;
          }
        }
    }
  /* if handle does'nt match with any entry */
    return -1;
}


/*===========================================================================
FUNCTION GetFireWallEntry_by_handle
===========================================================================
@brief
  Get all the firewall entry using the handle

@input
  ip_version
  wan_profile_handle
  firewall_entry
  qmi_err_num


@return
  boolean

@dependencies
  usr to provide input

@sideefects
  None
=========================================================================*/

boolean QCMAP_LAN_Client::GetFireWallEntry_by_handle
(
  qcmap_msgr_firewall_entry_conf_t  *firewall_entry,
  qmi_error_type_v01                *qmi_err_num
)
{
  char command[MAX_COMMAND_STR_LEN] = {0};
  char result[QCMAP_MAX_SCAN_SIZE] = {0};

  int  total_fw_rules;
  int val;

  LOG_MSG_INFO1("Entering GetFireWallEntry_by_handle Function",0,0,0);
  uint32 handle = firewall_entry->firewall_handle;

  int i = Get_Index_by_handle(handle, qmi_err_num);

  if(i == -1)
  {
     LOG_MSG_ERROR("failed to get the index",0,0,0);
     return false;
  }

  //getting total no of firewall rules added in that profile
  memset(result,0,QCMAP_MAX_SCAN_SIZE);
  snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh get qcmap_firewall.@firewall[0].no_of_rules");
  if (!ExecuteSystemCmd(command, result, sizeof(result)))
  {  LOG_MSG_ERROR("failed to execute command : %s",command,0,0);
     *qmi_err_num = QMI_ERR_INTERNAL_V01;
     return false;
  }

  total_fw_rules = atoi(result);

  memset(command,0,MAX_COMMAND_STR_LEN);
  memset(result,0,QCMAP_MAX_SCAN_SIZE);

  if (UciGetUtility(owrt_filename[static_cast<int>(config_file::QCMAP_FIREWALL)],
        "firewall_rule", 0, "firewall_handle", result, i))
  {
    firewall_entry->firewall_handle = atoi(result);
  }
  else
  {
    LOG_MSG_ERROR("UciGetUtility failed ",0,0,0);
    return false;
  }

  memset(result,0,QCMAP_MAX_SCAN_SIZE);

  if (UciGetUtility(owrt_filename[static_cast<int>(config_file::QCMAP_FIREWALL)],
        "firewall_rule", 0, "direction", result, i))
  {

    if(strncmp(result, "UL", strlen("UL"))==0)
    {
      firewall_entry->firewall_direction = QCMAP_MSGR_UL_FIREWALL;
    }
    else
    {
      firewall_entry->firewall_direction = QCMAP_MSGR_DL_FIREWALL;
    }
  }
  else
  {
    LOG_MSG_ERROR("UciGetUtility failed ",0,0,0);
    return false;
  }

  memset(result,0,QCMAP_MAX_SCAN_SIZE);
  if (UciGetUtility(owrt_filename[static_cast<int>(config_file::QCMAP_FIREWALL)],
        "firewall_rule", 0, "family", result, i))
  {

    if(strncmp(result, IP_V4_STRING, strlen(IP_V4_STRING))==0)
    {
      firewall_entry->filter_spec.ip_vsn = IP_V4;
    }
    else
    {
      firewall_entry->filter_spec.ip_vsn = IP_V6;
    }
  }
  else
  {
    LOG_MSG_ERROR("UciGetUtility failed ",0,0,0);
    return false;
  }

  if(firewall_entry->filter_spec.ip_vsn == IP_V4)
  {

    memset(result,0,QCMAP_MAX_SCAN_SIZE);
    if (UciGetUtility(owrt_filename[static_cast<int>(config_file::QCMAP_FIREWALL)],
          "firewall_rule", 0, "src_addr", result, i))
    {

      if(strncmp(result, "Any", strlen("Any"))!=0)
      {
        firewall_entry->filter_spec.ip_hdr.v4.field_mask |= IPFLTR_MASK_IP4_SRC_ADDR;
        if (inet_pton(AF_INET, result, (char *)&firewall_entry->filter_spec.ip_hdr.v4.src.addr.ps_s_addr) <=0)
        {
          LOG_MSG_ERROR(" Address not in presentation format\n", 0, 0, 0);
          return false;
        }

        memset(result,0,QCMAP_MAX_SCAN_SIZE);
        if (UciGetUtility(owrt_filename[static_cast<int>(config_file::QCMAP_FIREWALL)],
            "firewall_rule", 0, "src_addr_mask", result, i))
        {
          if(strncmp(result, "Any", strlen("Any"))!=0)
          {
            if (inet_pton(AF_INET, result, (uint8* )&firewall_entry->filter_spec.ip_hdr.v4.src.subnet_mask.ps_s_addr) <=0)
            {
                 LOG_MSG_ERROR(" Address not in presentation format\n", 0, 0, 0);
                 return false;
            }
          }
        }
        else
        {
          LOG_MSG_ERROR("UciGetUtility failed ",0,0,0);
          return false;
        }
      }
    }
    else
    {
      LOG_MSG_ERROR("UciGetUtility failed ",0,0,0);
      return false;
    }/* src_addr if condition ends here */

    memset(result,0,QCMAP_MAX_SCAN_SIZE);
    if (UciGetUtility(owrt_filename[static_cast<int>(config_file::QCMAP_FIREWALL)],
          "firewall_rule", 0, "dest_addr", result, i))
    {

      if(strncmp(result, "Any", strlen("Any"))!=0)
      {
        firewall_entry->filter_spec.ip_hdr.v4.field_mask |= IPFLTR_MASK_IP4_DST_ADDR;
        if ( inet_pton(AF_INET, result, (char *)&firewall_entry->filter_spec.ip_hdr.v4.dst.addr.ps_s_addr) <=0 ) {
          LOG_MSG_ERROR(" Address not in presentation format\n", 0, 0, 0);
          return false;
        }

        memset(result,0,QCMAP_MAX_SCAN_SIZE);
        if (UciGetUtility(owrt_filename[static_cast<int>(config_file::QCMAP_FIREWALL)],
            "firewall_rule", 0, "dest_addr_mask", result, i))
        {

          if(strncmp(result, "Any", strlen("Any"))!=0)
          {
            if ( inet_pton(AF_INET, result, (uint8* )&firewall_entry->filter_spec.ip_hdr.v4.dst.subnet_mask.ps_s_addr) <=0 ) {
                 LOG_MSG_ERROR(" Address not in presentation format\n", 0, 0, 0);
                 return false;
               }
          }
        }
        else
        {
          LOG_MSG_ERROR("UciGetUtility failed ",0,0,0);
          return false;
        }
      }
    }
    else
    {
      LOG_MSG_ERROR("UciGetUtility failed ",0,0,0);
      return false;
    }/* dest_addr if condition ends here */

    memset(result,0,QCMAP_MAX_SCAN_SIZE);
    if (UciGetUtility(owrt_filename[static_cast<int>(config_file::QCMAP_FIREWALL)],
          "firewall_rule", 0, "tos_value", result, i))
    {
      if(strncmp(result, "Any", strlen("Any"))!=0)
      {
        firewall_entry->filter_spec.ip_hdr.v4.field_mask |= IPFLTR_MASK_IP4_TOS;
        firewall_entry->filter_spec.ip_hdr.v4.tos.val = (int)strtol(result, NULL, 16);

      memset(result,0,QCMAP_MAX_SCAN_SIZE);
      if (UciGetUtility(owrt_filename[static_cast<int>(config_file::QCMAP_FIREWALL)],
          "firewall_rule", 0, "tos_mask", result, i))
      {
        if(strncmp(result, "Any", strlen("Any"))!=0)
        {
          firewall_entry->filter_spec.ip_hdr.v4.tos.mask = (int)strtol(result, NULL, 16);
        }
      }
      else
      {
        LOG_MSG_ERROR("UciGetUtility failed ",0,0,0);
        return false;
      }
     }
    }
    else
    {
      LOG_MSG_ERROR("UciGetUtility failed ",0,0,0);
      return false;
    }/* tos if condition ends here */

    memset(result,0,QCMAP_MAX_SCAN_SIZE);
    if (UciGetUtility(owrt_filename[static_cast<int>(config_file::QCMAP_FIREWALL)],
          "firewall_rule", 0, "proto", result, i))
    {

      if(strncmp(result, "Any", strlen("Any"))!=0)
      {
        firewall_entry->filter_spec.ip_hdr.v4.field_mask |= IPFLTR_MASK_IP4_NEXT_HDR_PROT;
        if(strncmp(result, TCP_PROTO, strlen(result))==0)
        {
          val=PS_IPPROTO_TCP;
          firewall_entry->filter_spec.ip_hdr.v4.next_hdr_prot = PS_IPPROTO_TCP;
        }
        else if(strncmp(result, UDP_PROTO, strlen(result))==0)
        {
          val=PS_IPPROTO_UDP;
          firewall_entry->filter_spec.ip_hdr.v4.next_hdr_prot = PS_IPPROTO_UDP;
        }
        else if(strncmp(result, ICMP_PROTO, strlen(result))==0)
        {
          val=PS_IPPROTO_ICMP;
          firewall_entry->filter_spec.ip_hdr.v4.next_hdr_prot = PS_IPPROTO_ICMP;
        }
        else if(strncmp(result, TCP_UDP_PROTO, strlen(result))==0)
        {
          val=PS_IPPROTO_TCP_UDP;
          firewall_entry->filter_spec.ip_hdr.v4.next_hdr_prot = PS_IPPROTO_TCP_UDP;
        }
        else if(strncmp(result, ESP_PROTO, strlen(result))==0)
        {
          val=PS_IPPROTO_ESP;
          firewall_entry->filter_spec.ip_hdr.v4.next_hdr_prot = PS_IPPROTO_ESP;
        }
      }
    }
    else
    {
      LOG_MSG_ERROR("UciGetUtility failed ",0,0,0);
      return false;
    }/* Proto if condition ends */
  } /* IPV4 if condition ends here */
  else
  {
    //if IP Version is IPV6.
    memset(result,0,QCMAP_MAX_SCAN_SIZE);
    if (UciGetUtility(owrt_filename[static_cast<int>(config_file::QCMAP_FIREWALL)],
          "firewall_rule", 0, "src_addr", result, i))
    {

      if(strncmp(result, "Any", strlen("Any"))!=0)
      {
        firewall_entry->filter_spec.ip_hdr.v6.field_mask |= IPFLTR_MASK_IP6_SRC_ADDR;
        if ( inet_pton(AF_INET6, result, (uint8 *)&firewall_entry->filter_spec.ip_hdr.v6.src.addr.in6_u.u6_addr8) <=0 ) {
          LOG_MSG_ERROR(" Address not in presentation format\n", 0, 0, 0);
          return false;
        }

        memset(result,0,QCMAP_MAX_SCAN_SIZE);
        if (UciGetUtility(owrt_filename[static_cast<int>(config_file::QCMAP_FIREWALL)],
            "firewall_rule", 0, "src_prefix_length", result, i))
        {
          if(strncmp(result, "Any", strlen("Any"))!=0)
          {
            firewall_entry->filter_spec.ip_hdr.v6.src.prefix_len = atoi(result);
          }
        }
        else
        {
          LOG_MSG_ERROR("UciGetUtility failed ",0,0,0);
          return false;
        }
      }
    }
    else
    {
      LOG_MSG_ERROR("UciGetUtility failed ",0,0,0);
      return false;
    }/* src_addr if condition ends here */

    memset(result,0,QCMAP_MAX_SCAN_SIZE);
    if (UciGetUtility(owrt_filename[static_cast<int>(config_file::QCMAP_FIREWALL)],
          "firewall_rule", 0, "dest_addr", result, i))
    {

      if(strncmp(result, "Any", strlen("Any"))!=0)
      {
        firewall_entry->filter_spec.ip_hdr.v6.field_mask |= IPFLTR_MASK_IP6_DST_ADDR;
        if ( inet_pton(AF_INET6, result, (uint8 *)&firewall_entry->filter_spec.ip_hdr.v6.dst.addr.in6_u.u6_addr8) <=0 ) {
          LOG_MSG_ERROR(" Address not in presentation format\n", 0, 0, 0);
          return false;
        }

        memset(result,0,QCMAP_MAX_SCAN_SIZE);
        if (UciGetUtility(owrt_filename[static_cast<int>(config_file::QCMAP_FIREWALL)],
            "firewall_rule", 0, "dest_prefix_length", result, i))
        {

          if(strncmp(result, "Any", strlen("Any"))!=0)
          {
            firewall_entry->filter_spec.ip_hdr.v6.dst.prefix_len = atoi(result);
          }
        }
        else
        {
          LOG_MSG_ERROR("UciGetUtility failed ",0,0,0);
          return false;
        }
      }
    }
    else
    {
      LOG_MSG_ERROR("UciGetUtility failed ",0,0,0);
      return false;
    }/* dest_addr if condition ends here */

    memset(result,0,QCMAP_MAX_SCAN_SIZE);
    if (UciGetUtility(owrt_filename[static_cast<int>(config_file::QCMAP_FIREWALL)],
          "firewall_rule", 0, "tos_value", result, i))
    {
      if(strncmp(result, "Any", strlen("Any"))!=0)
      {
        firewall_entry->filter_spec.ip_hdr.v6.field_mask |= IPFLTR_MASK_IP6_TRAFFIC_CLASS;
        firewall_entry->filter_spec.ip_hdr.v6.trf_cls.val = (int)strtol(result, NULL, 16);

      memset(result,0,QCMAP_MAX_SCAN_SIZE);
      if (UciGetUtility(owrt_filename[static_cast<int>(config_file::QCMAP_FIREWALL)],
          "firewall_rule", 0, "tos_mask", result, i))
      {
        if(strncmp(result, "Any", strlen("Any"))!=0)
        {
          firewall_entry->filter_spec.ip_hdr.v6.trf_cls.mask = (int)strtol(result, NULL, 16);
        }
      }
      else
      {
        LOG_MSG_ERROR("UciGetUtility failed ",0,0,0);
        return false;
      }
     }
    }
    else
    {
      LOG_MSG_ERROR("UciGetUtility failed ",0,0,0);
      return false;
    }/* tos if condition ends here */
    memset(result,0,QCMAP_MAX_SCAN_SIZE);
    if (UciGetUtility(owrt_filename[static_cast<int>(config_file::QCMAP_FIREWALL)],
          "firewall_rule", 0, "proto", result, i))
    {
      if(strncmp(result, "Any", strlen("Any"))!=0)
      {
        firewall_entry->filter_spec.ip_hdr.v6.field_mask |= IPFLTR_MASK_IP6_NEXT_HDR_PROT;
        if(strncmp(result, TCP_PROTO, strlen(result))==0)
        {
          val=PS_IPPROTO_TCP;
          firewall_entry->filter_spec.ip_hdr.v6.next_hdr_prot = PS_IPPROTO_TCP;
        }
        else if(strncmp(result, UDP_PROTO, strlen(result))==0)
        {
          val=PS_IPPROTO_UDP;
          firewall_entry->filter_spec.ip_hdr.v6.next_hdr_prot = PS_IPPROTO_UDP;
        }
        else if(strncmp(result, ICMP_PROTO, strlen(result))==0)
        {
          val=PS_IPPROTO_ICMP6;
          firewall_entry->filter_spec.ip_hdr.v6.next_hdr_prot = PS_IPPROTO_ICMP6;
        }
        else if(strncmp(result, TCP_UDP_PROTO, strlen(result))==0)
        {
          val=PS_IPPROTO_TCP_UDP;
          firewall_entry->filter_spec.ip_hdr.v6.next_hdr_prot = PS_IPPROTO_TCP_UDP;
        }
        else if(strncmp(result, ESP_PROTO, strlen(result))==0)
        {
          val=PS_IPPROTO_ESP;
          firewall_entry->filter_spec.ip_hdr.v6.next_hdr_prot = PS_IPPROTO_ESP;
        }
      }
    }
    else
    {
      LOG_MSG_ERROR("UciGetUtility failed ",0,0,0);
      return false;
    }/* Proto if condition ends */

  } /* IPV6 if condition ends here */

  if(val == PS_IPPROTO_TCP)
  {
    memset(result,0,QCMAP_MAX_SCAN_SIZE);
    if (UciGetUtility(owrt_filename[static_cast<int>(config_file::QCMAP_FIREWALL)],
          "firewall_rule", 0, "src_port", result, i))
    {
      if(strncmp(result, "Any", strlen("Any"))!=0)
      {
        firewall_entry->filter_spec.next_prot_hdr.tcp.field_mask |= IPFLTR_MASK_TCP_SRC_PORT;
        firewall_entry->filter_spec.next_prot_hdr.tcp.src.port = atoi(result);

        memset(result,0,QCMAP_MAX_SCAN_SIZE);
        if (UciGetUtility(owrt_filename[static_cast<int>(config_file::QCMAP_FIREWALL)],
            "firewall_rule", 0, "src_port_range", result, i))
        {
          firewall_entry->filter_spec.next_prot_hdr.tcp.src.range = atoi(result);
        }
        else
        {
          LOG_MSG_ERROR("UciGetUtility failed ",0,0,0);
          return false;
        }
      }
    }
    else
    {
      LOG_MSG_ERROR("UciGetUtility failed ",0,0,0);
      return false;
    }

    memset(result,0,QCMAP_MAX_SCAN_SIZE);
    if (UciGetUtility(owrt_filename[static_cast<int>(config_file::QCMAP_FIREWALL)],
          "firewall_rule", 0, "dest_port", result, i))
    {
      if(strncmp(result, "Any", strlen("Any"))!=0)
      {
        firewall_entry->filter_spec.next_prot_hdr.tcp.field_mask |= IPFLTR_MASK_TCP_DST_PORT;
        firewall_entry->filter_spec.next_prot_hdr.tcp.dst.port = atoi(result);

        memset(result,0,QCMAP_MAX_SCAN_SIZE);
        if (UciGetUtility(owrt_filename[static_cast<int>(config_file::QCMAP_FIREWALL)],
            "firewall_rule", 0, "dest_port_range", result, i))
        {
          firewall_entry->filter_spec.next_prot_hdr.tcp.dst.range = atoi(result);
        }
        else
        {
          LOG_MSG_ERROR("UciGetUtility failed ",0,0,0);
          return false;
        }
      }
    }
    else
    {
      LOG_MSG_ERROR("UciGetUtility failed ",0,0,0);
      return false;
    }
  }
  else if(val == PS_IPPROTO_UDP)
  {
    memset(result,0,QCMAP_MAX_SCAN_SIZE);
    if (UciGetUtility(owrt_filename[static_cast<int>(config_file::QCMAP_FIREWALL)],
          "firewall_rule", 0, "src_port", result, i))
    {
      if(strncmp(result, "Any", strlen("Any"))!=0)
      {
        firewall_entry->filter_spec.next_prot_hdr.udp.field_mask |= IPFLTR_MASK_UDP_SRC_PORT;
        firewall_entry->filter_spec.next_prot_hdr.udp.src.port = atoi(result);

        memset(result,0,QCMAP_MAX_SCAN_SIZE);
        if (UciGetUtility(owrt_filename[static_cast<int>(config_file::QCMAP_FIREWALL)],
            "firewall_rule", 0, "src_port_range", result, i))
        {
          firewall_entry->filter_spec.next_prot_hdr.udp.src.range = atoi(result);
        }
        else
        {
          LOG_MSG_ERROR("UciGetUtility failed ",0,0,0);
          return false;
        }
      }
    }
    else
    {
      LOG_MSG_ERROR("UciGetUtility failed ",0,0,0);
      return false;
    }

    memset(result,0,QCMAP_MAX_SCAN_SIZE);
    if (UciGetUtility(owrt_filename[static_cast<int>(config_file::QCMAP_FIREWALL)],
          "firewall_rule", 0, "dest_port", result, i))
    {
      if(strncmp(result, "Any", strlen("Any"))!=0)
      {
        firewall_entry->filter_spec.next_prot_hdr.udp.field_mask |= IPFLTR_MASK_UDP_DST_PORT;
        firewall_entry->filter_spec.next_prot_hdr.udp.dst.port = atoi(result);

        memset(result,0,QCMAP_MAX_SCAN_SIZE);
        if (UciGetUtility(owrt_filename[static_cast<int>(config_file::QCMAP_FIREWALL)],
            "firewall_rule", 0, "dest_port_range", result, i))
        {
          firewall_entry->filter_spec.next_prot_hdr.udp.dst.range = atoi(result);
        }
        else
        {
          LOG_MSG_ERROR("UciGetUtility failed ",0,0,0);
          return false;
        }
      }
    }
    else
    {
      LOG_MSG_ERROR("UciGetUtility failed ",0,0,0);
      return false;
    }
  }
  else if (val == PS_IPPROTO_TCP_UDP)
  {
    memset(result,0,QCMAP_MAX_SCAN_SIZE);
    if (UciGetUtility(owrt_filename[static_cast<int>(config_file::QCMAP_FIREWALL)],
          "firewall_rule", 0, "src_port", result, i))
    {
      if(strncmp(result, "Any", strlen("Any"))!=0)
      {
        firewall_entry->filter_spec.next_prot_hdr.tcp_udp_port_range.field_mask |= IPFLTR_MASK_TCP_UDP_SRC_PORT;
        firewall_entry->filter_spec.next_prot_hdr.tcp_udp_port_range.src.port = atoi(result);

        memset(result,0,QCMAP_MAX_SCAN_SIZE);
        if (UciGetUtility(owrt_filename[static_cast<int>(config_file::QCMAP_FIREWALL)],
            "firewall_rule", 0, "src_port_range", result, i))
        {
          firewall_entry->filter_spec.next_prot_hdr.tcp_udp_port_range.src.range = atoi(result);
        }
        else
        {
          LOG_MSG_ERROR("UciGetUtility failed ",0,0,0);
          return false;
        }
      }
    }
    else
    {
      LOG_MSG_ERROR("UciGetUtility failed ",0,0,0);
      return false;
    }

    memset(result,0,QCMAP_MAX_SCAN_SIZE);
    if (UciGetUtility(owrt_filename[static_cast<int>(config_file::QCMAP_FIREWALL)],
          "firewall_rule", 0, "dest_port", result, i))
    {

      if(strncmp(result, "Any", strlen("Any"))!=0)
      {
        firewall_entry->filter_spec.next_prot_hdr.tcp_udp_port_range.field_mask |= IPFLTR_MASK_TCP_UDP_DST_PORT;
        firewall_entry->filter_spec.next_prot_hdr.tcp_udp_port_range.dst.port = atoi(result);

        memset(result,0,QCMAP_MAX_SCAN_SIZE);
        if (UciGetUtility(owrt_filename[static_cast<int>(config_file::QCMAP_FIREWALL)],
            "firewall_rule", 0, "dest_port_range", result, i))
        {
          firewall_entry->filter_spec.next_prot_hdr.tcp_udp_port_range.dst.range = atoi(result);
        }
        else
        {
          LOG_MSG_ERROR("UciGetUtility failed ",0,0,0);
          return false;
        }
      }
    }
    else
    {
      LOG_MSG_ERROR("UciGetUtility failed ",0,0,0);
      return false;
    }
  }
  else if ( val == PS_IPPROTO_ICMP)
  {
    memset(result,0,QCMAP_MAX_SCAN_SIZE);
    if (UciGetUtility(owrt_filename[static_cast<int>(config_file::QCMAP_FIREWALL)],
          "firewall_rule", 0, "icmp_type", result, i))
    {
      if(strncmp(result, "Any", strlen("Any"))!=0)
      {
        firewall_entry->filter_spec.next_prot_hdr.icmp.type = atoi(result);
        firewall_entry->filter_spec.next_prot_hdr.icmp.field_mask |= IPFLTR_MASK_ICMP_MSG_TYPE;
        memset(result,0,QCMAP_MAX_SCAN_SIZE);
        if (UciGetUtility(owrt_filename[static_cast<int>(config_file::QCMAP_FIREWALL)],
            "firewall_rule", 0, "icmp_code", result, i))
        {
          firewall_entry->filter_spec.next_prot_hdr.icmp.code = atoi(result);
          firewall_entry->filter_spec.next_prot_hdr.icmp.field_mask |= IPFLTR_MASK_ICMP_MSG_CODE;
        }
      }
    }
    else
    {
      LOG_MSG_ERROR("UciGetUtility failed ",0,0,0);
      return false;
    }
  }
  else if ( val == PS_IPPROTO_ICMP6 )
  {
    memset(result,0,QCMAP_MAX_SCAN_SIZE);
    if (UciGetUtility(owrt_filename[static_cast<int>(config_file::QCMAP_FIREWALL)],
          "firewall_rule", 0, "icmp6_type", result, i))
    {
      if(strncmp(result, "Any", strlen("Any"))!=0)
      {
        firewall_entry->filter_spec.next_prot_hdr.icmp.type = atoi(result);
        firewall_entry->filter_spec.next_prot_hdr.icmp.field_mask |= IPFLTR_MASK_ICMP_MSG_TYPE;
        memset(result,0,QCMAP_MAX_SCAN_SIZE);
        if (UciGetUtility(owrt_filename[static_cast<int>(config_file::QCMAP_FIREWALL)],
            "firewall_rule", 0, "icmp6_code", result, i))
        {
          firewall_entry->filter_spec.next_prot_hdr.icmp.code = atoi(result);
          firewall_entry->filter_spec.next_prot_hdr.icmp.field_mask |= IPFLTR_MASK_ICMP_MSG_CODE;
        }
      }
    }
    else
    {
      LOG_MSG_ERROR("UciGetUtility failed ",0,0,0);
      return false;
    }
  }
  else if( val == PS_IPPROTO_ESP)
  {
    if (UciGetUtility(owrt_filename[static_cast<int>(config_file::QCMAP_FIREWALL)],
          "firewall_rule", 0, "esp_spi", result, i))
    {
      if(strncmp(result, "Any", strlen("Any"))!=0)
      {
        firewall_entry->filter_spec.next_prot_hdr.esp.spi = atoi(result);
        firewall_entry->filter_spec.next_prot_hdr.esp.field_mask |= IPFLTR_MASK_ESP_SPI;
      }
    }
    else
    {
      LOG_MSG_ERROR("UciGetUtility failed ",0,0,0);
      return false;
    }
  }
  return true;
}

