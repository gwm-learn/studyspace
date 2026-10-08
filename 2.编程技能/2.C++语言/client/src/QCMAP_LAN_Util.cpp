/*====================================================

FILE:  QCMAP_LAN_Util.cpp

SERVICES:
QCMAP LAN Util Client Implementation

=====================================================

  Copyright (c) 2022-2023 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Confidential and Proprietary - Qualcomm Technologies, Inc.

  Copyright (c) 2004 by Internet Systems Consortium, Inc. ("ISC")
  Copyright (c) 1996,1999 by Internet Software Consortium.

  Permission to use, copy, modify, and distribute this software for any
  purpose with or without fee is hereby granted, provided that the above
  copyright notice and this permission notice appear in all copies.

  THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES
  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
  MERCHANTABILITY AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR
  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
  OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

=====================================================*/
/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  when       who        what, where, why
  --------   ---        -------------------------------------------------------
  06/07/23   mk         Splitted LAN_CLIENT module into multiple modules
  ===========================================================================*/

#include <fstream>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <linux/if.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <assert.h>
#include <sys/time.h>
#include <sys/select.h>
#include <pthread.h>
#include <sys/stat.h>
#include <time.h>
#include <algorithm>
#include "ds_util.h"
#include "ds_string.h"
#include "QCMAP_LAN_Client.h"
#include <iostream>
#include <fstream>
#include <string>
#include <vector>


/*MAX FIREWALL ENTRY*/
#define MAX_FIREWALL_ENTRY                   128

#define QCMAP_MAX_COMMAND_LEN                100   /* Max Command length */
#define INET6_ADDRSTRLEN                     46    /* INET6 address string*/

/*QCMAP_VALIDATE_BAND_INFO*/
#define QCMAP_VALIDATE_BAND_INFO(band)(band == 2 || band == 5 || band == 6 ? true : false)

/*ETH Mode*/
#define QCMAP_ETH_MODE_LAN 0
#define QCMAP_ETH_MODE_WAN 1

/*---------------------------------------------------------------------------
  Return values indicating error status
---------------------------------------------------------------------------*/
#define QCMAP_CM_SUCCESS               0         /* Successful operation   */
#define QCMAP_CM_ERROR                -1         /* Unsuccessful operation */
#define TRUE                           1

/*---------------------------------------------------------------------------
  Logging definitions
---------------------------------------------------------------------------*/
#define QCMAP_LOG(...)                         \
 LOG_MSG_INFO1( "%s %d:", __FILE__, __LINE__,0); \
 LOG_MSG_INFO1( __VA_ARGS__ ,0,0);

#define QCMAP_LOG_FUNC_ENTRY()  \
 QCMAP_LOG                   \
(                              \
       "Entering function %s\n",  \
       __FUNCTION__               \
)

#define QCMAP_LOG_FUNC_EXIT()   \
 QCMAP_LOG                   \
(                              \
       "Exiting function %s\n",   \
       __FUNCTION__ \
)

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef __cplusplus
}
#endif


#include "QCMAP_LAN_Client.h"

 /*===========================================================================
 FUNCTION ReverseByteOrder()
 ===========================================================================*/
 /** @ingroup qcmap_reverse_byte_order

   Reverses the byte order.

   @param[in]      addr      IPv6 quadlet  \n

   @return
   uint32_t -- Reversed byte order
 */
 /*=========================================================================*/
uint32_t QCMAP_LAN_Client::ReverseByteOrder
(
  uint32_t addr
)
{
  uint16_t doublet_low, doublet_high = 0;
  doublet_low = addr & MASK16 ;
  doublet_high = (addr >> 16) & MASK16;
  doublet_low = ((((doublet_low << 8) >> 8 ) & MASK8) << 8)|(doublet_low >> 8);
  doublet_high = ((((doublet_high << 8) >> 8 ) & MASK8) << 8)|(doublet_high >> 8);
  return ((doublet_low << 16)| doublet_high);
}

 /*===========================================================================
 FUNCTION GetIPv6Prefix()
 ===========================================================================*/
 /** @ingroup qcmap_get_ipv6_prefix

   Calculates IPv6 prefix from IPv6 address and prefix length.

   @param[in]      v6_addr_str      IPv6 Address string \n
   @param[in,out]  v6_prefix_str    IPv6 Prefix string \n
   @param[in]      v6_prefix_len    IPv6 Prefix length

   @return
   TRUE -- Success \n
   FALSE -- Failure \n
 */
 /*=========================================================================*/
boolean QCMAP_LAN_Client::GetIPv6Prefix
(
  const char *v6_addr_str,
  unsigned char *v6_prefix_str,
  uint8_t prefix_len
)
{
  qcmap_ipv6_addr ipv6_addr;
  unsigned char net_addr[sizeof(struct in6_addr)];
  uint32_t temp = 0;
  uint32_t result = 0;
  uint32_t prefixmask = 0;

  if (inet_pton(AF_INET6, v6_addr_str, net_addr) < 0)
  {
    LOG_MSG_ERROR("Error in retrieving Network address", 0, 0, 0);
    return false;
  }
  memcpy(&ipv6_addr, net_addr, sizeof(qcmap_ipv6_addr));
  if (prefix_len <= 32)
  {
    temp = QCMAP_LAN_Client::ReverseByteOrder(ipv6_addr.quadlet1);
    prefixmask = (MASK32 << (32 - prefix_len));
    result = QCMAP_LAN_Client::ReverseByteOrder(temp & prefixmask);
    memcpy(v6_prefix_str, &result, sizeof(uint32_t));
  }
  else if(prefix_len >32 && prefix_len <= 64)
  {
    temp = QCMAP_LAN_Client::ReverseByteOrder(ipv6_addr.quadlet2);
    prefixmask = (MASK32 << (64 - prefix_len));
    result = QCMAP_LAN_Client::ReverseByteOrder(temp & prefixmask);
    memcpy(v6_prefix_str, &ipv6_addr.quadlet1, sizeof(uint32_t));
    memcpy(v6_prefix_str+sizeof(uint32_t), &result, sizeof(uint32_t));
  }
  else if (prefix_len >64 && prefix_len <= 96)
  {
    temp = QCMAP_LAN_Client::ReverseByteOrder(ipv6_addr.quadlet3);
    prefixmask = (MASK32 << (96 - prefix_len));
    result = QCMAP_LAN_Client::ReverseByteOrder(temp & prefixmask);
    memcpy(v6_prefix_str, &ipv6_addr.quadlet1, sizeof(uint32_t));
    memcpy(v6_prefix_str+sizeof(uint32_t), &ipv6_addr.quadlet2, sizeof(uint32_t));
    memcpy(v6_prefix_str+sizeof(uint32_t)+sizeof(uint32_t), &result, sizeof(uint32_t));
  }
  else
  {
    temp = QCMAP_LAN_Client::ReverseByteOrder(ipv6_addr.quadlet4);
    prefixmask = (MASK32 << (128 - prefix_len));
    result = QCMAP_LAN_Client::ReverseByteOrder(temp & prefixmask);
    memcpy(v6_prefix_str, &ipv6_addr.quadlet1, sizeof(uint32_t));
    memcpy(v6_prefix_str+sizeof(uint32_t), &ipv6_addr.quadlet2, sizeof(uint32_t));
    memcpy(v6_prefix_str+sizeof(uint32_t)+sizeof(uint32_t), &ipv6_addr.quadlet3, sizeof(uint32_t));
    memcpy(v6_prefix_str+sizeof(uint32_t)+sizeof(uint32_t)+sizeof(uint32_t), &result, sizeof(uint32_t));
  }
  return true;
}


/*===========================================================================
  FUNCTION GetTetheredIfaceNameFromUCI
==========================================================================*/
/*!
@brief
  Gets iface name for a given tethered iface type from UCI.


@return
  true - if operation is successful
  false - if operation fails.

@note
- Dependencies
- None

- Side Effects
- None
*/
/*=========================================================================*/

boolean QCMAP_LAN_Client::GetTetheredIfaceNameFromUCI(char *iface_str, char *iface_name)
{
  char command[MAX_COMMAND_STR_LEN] = {0};
  char result[QCMAP_MAX_SCAN_SIZE] = {0};

  if (iface_str == NULL || iface_name == NULL)
  {
    LOG_MSG_ERROR("null params passed", 0, 0, 0);
    return false;
  }
  if (UciGetUtility(owrt_filename[static_cast<int>(config_file::QCMAP_LAN)],
      "phy_iface_names", 0, iface_str, result, 0))
  {
    snprintf(iface_name, QCMAP_MAX_IFACE_NAME_SIZE_V01, "%s", result);
    return true;
  }
  else
  {
    LOG_MSG_ERROR("failed to get iface_name for iface_str:%s", iface_str, 0, 0);
    return false;
  }
}


qcmap_cdt_enum QCMAP_LAN_Client::utilGetCDTValue()
{
   char result[20]={0};
   int result_int=0;

   if( !ExecuteSystemCmd("cat /sys/devices/soc0/platform_subtype_id", result, sizeof(result)))
   {
     LOG_MSG_ERROR("Get CDT Value failure",0,0,0);
     return QCMAP_CDT_UNKNOWN;
   }

   result_int = atoi(result);
   switch (result_int)
   {
     /* CDT zero represents RCM device with HMT wifi card, QCMAP treats same as HMT */
     case 0:
        return QCMAP_CDT_HMT;
     case 3:
        return QCMAP_CDT_WKK;
     case 2:
        return QCMAP_CDT_HMT;
     default:
        return QCMAP_CDT_UNKNOWN;
   }
}

qcmap_cpe_wkk_enum QCMAP_LAN_Client::UtilGetCpeWkkType()
{
  char result[QCMAP_MAX_SCAN_SIZE]={0};

  if(!ExecuteSystemCmd("cat /tmp/sysinfo/model", result, sizeof(result)))
  {
    LOG_MSG_ERROR("Get CPE WKK Type failure",0,0,0);
    return QCMAP_CPE_WKK_UNKNOWN;
  }

  if(strncmp(result, CPE_WKK_V1_STR, strlen(CPE_WKK_V1_STR)) == 0)
  {
    return QCMAP_CPE_WKK_V1;
  }
  else if(strncmp(result, CPE_WKK_V2_STR, strlen(CPE_WKK_V2_STR)) == 0)
  {
    return QCMAP_CPE_WKK_V2;
  }

  return QCMAP_CPE_WKK_UNKNOWN;
}

/*=====================================================================
  FUNCTION UciSetUtility
======================================================================*/
/*!
@brief
  - execute uci set command depending upon the inputs

@input
  param[in]          filename                   name of the config file.
                                                A macro "owrt_filename" is defined for this.
                                                owrt_filename[0] --> qcmap_lan
                                                owrt_filename[1] --> qcmap_firewall
                                                owrt_filename[2] --> firewall
                                                owrt_filename[3] --> network
                                                owrt_filename[4] --> dhcp
  param[in]          config                     name of the uci config
  param[in]          IsConfigNameProvided       if name of the config section is provided i.e
                                                if the value is 1, then we dont need the index
                                                to set or get the option using uci.
                                                example :
                                                network.lan=interface
                                                network.lan.device='br-lan'
                                                network.lan.proto='static'
                                                network.lan.netmask='255.255.255.0'
                                                network.lan.ip6assign='60'
                                                network.lan.ipaddr='192.168.2.1'

                                                config interface 'lan'  <--Here we have name of
                                                                           config, so we can access
                                                                           option by config name,
                                                                           we dont need index here
                                                                           and here config will
                                                                           be "lan"
                                                  option device 'br-lan'
                                                  option proto 'static'
                                                  option netmask '255.255.255.0'
                                                  option ip6assign '60'
                                                  option ipaddr '192.168.2.1'

                                                network.@device[0]=device
                                                network.@device[0].name='br-lan'
                                                network.@device[0].type='bridge'
                                                network.@device[0].ports='eth0' 'eth0' 'eth1' 'rndis0' 'ecm0'

                                                In this case,we just have
                                                config device         <-- Here we dont have a name,
                                                                          so we have to use index to
                                                                          get or set option values.
                                                                          Hence,Here config will
                                                                          be "device"
                                                  option name 'br-lan'
                                                  option type 'bridge'
                                                  list ports 'eth0'
                                                  list ports 'eth0'
                                                  list ports 'eth1'
                                                  list ports 'rndis0'
                                                  list ports 'ecm0'
  param[in]          option                     uci option name
  param[in]          value                      value we want to set(string)
  param[in]          index                      index

@return
  true - Success
  false - Failure

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
boolean QCMAP_LAN_Client::UciSetUtility
(
  const char* filename,
  const char* config,
  boolean     IsConfigNameProvided,
  const char* option,
  char*       value,
  int         index = 0
)
{
  char cmd[QCMAP_MAX_COMMAND_LEN]={0};

  if (filename == NULL || config == NULL || option == NULL || value==NULL)
  {
    LOG_MSG_ERROR("Arguments passed are NULL",0,0,0);
    return false;
  }
  if(IsConfigNameProvided)
  {
    /* then we can set option value by config name */
    snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "%s %s.%s.%s='%s' ",
             UCI_SET_COMMAND, filename, config, option, value);
  }
  else
  {
    snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "%s %s.@%s[%d].%s='%s' ",
             UCI_SET_COMMAND, filename, config, index, option, value);
  }
  LOG_MSG_INFO1("set cmd: %s", cmd,0,0);
  ds_system_call(cmd, strlen(cmd));
  ExecuteUCICommit();
  return true;
}

/*=====================================================================
  FUNCTION UciSetUtility
======================================================================*/
/*!
@brief
  - execute uci set command depending upon the inputs
  - Its a overload function of UciSetUtility having set value as integer

@input
  param[in]          filename                   name of the config file.
                                                A macro "owrt_filename" is defined for this.
                                                owrt_filename[0] --> qcmap_lan
                                                owrt_filename[1] --> qcmap_firewall
                                                owrt_filename[2] --> firewall
                                                owrt_filename[3] --> network
                                                owrt_filename[4] --> dhcp
  param[in]          config                     name of the uci config
  param[in]          IsConfigNameProvided       if name of the config section is provided i.e
                                                if the value is 1, then we dont need the index
                                                to set or get the option using uci.
                                                example :
                                                network.lan=interface
                                                network.lan.device='br-lan'
                                                network.lan.proto='static'
                                                network.lan.netmask='255.255.255.0'
                                                network.lan.ip6assign='60'
                                                network.lan.ipaddr='192.168.2.1'

                                                config interface 'lan'  <--Here we have name of
                                                                           config, so we can access
                                                                           option by config name,
                                                                           we dont need index here
                                                                           and here config will
                                                                           be "lan"
                                                  option device 'br-lan'
                                                  option proto 'static'
                                                  option netmask '255.255.255.0'
                                                  option ip6assign '60'
                                                  option ipaddr '192.168.2.1'

                                                network.@device[0]=device
                                                network.@device[0].name='br-lan'
                                                network.@device[0].type='bridge'
                                                network.@device[0].ports='eth0' 'eth0' 'eth1' 'rndis0' 'ecm0'

                                                In this case,we just have
                                                config device         <-- Here we dont have a name,
                                                                          so we have to use index to
                                                                          get or set option values.
                                                                          Hence,Here config will
                                                                          be "device"
                                                  option name 'br-lan'
                                                  option type 'bridge'
                                                  list ports 'eth0'
                                                  list ports 'eth0'
                                                  list ports 'eth1'
                                                  list ports 'rndis0'
                                                  list ports 'ecm0'
  param[in]          option                     uci option name
  param[in]          value                      value we want to set(integer)
  param[in]          index                      index

@return
  true - Success
  false - Failure

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
boolean QCMAP_LAN_Client::UciSetUtility
(
  const char* filename,
  const char* config,
  boolean     IsConfigNameProvided,
  const char* option,
  int         value,
  int         index = 0
)
{
  char cmd[QCMAP_MAX_COMMAND_LEN]={0};

  if (filename == NULL || config == NULL || option == NULL )
  {
    LOG_MSG_ERROR("Arguments passed are NULL",0,0,0);
    return false;
  }
  if(IsConfigNameProvided)
  {
    /* then we can set option value by config name */
    snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "%s %s.%s.%s='%d' ",
             UCI_SET_COMMAND, filename, config, option, value);
  }
  else
  {
    snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "%s %s.@%s[%d].%s='%d' ",
             UCI_SET_COMMAND, filename, config, index, option, value);
  }
  LOG_MSG_INFO1("set cmd: %s", cmd,0,0);
  ds_system_call(cmd, strlen(cmd));
  ExecuteUCICommit();
  return true;
}

/*=====================================================================
  FUNCTION UciGetUtility
======================================================================*/
/*!
@brief
  - execute uci get command depending upon the inputs

@input
  param[in]          filename                   name of the config  file.
                                                A macro "owrt_filename" is defined for this.
                                                owrt_filename[0] --> qcmap_lan
                                                owrt_filename[1] --> qcmap_firewall
                                                owrt_filename[2] --> firewall
                                                owrt_filename[3] --> network
                                                owrt_filename[4] --> dhcp
  param[in]          config                     name of the uci config
  param[in]          IsConfigNameProvided       if name of the config section is provided i.e
                                                if the value is 1, then we dont need the index
                                                to set or get the option using uci.
                                                example :
                                                network.lan=interface
                                                network.lan.device='br-lan'
                                                network.lan.proto='static'
                                                network.lan.netmask='255.255.255.0'
                                                network.lan.ip6assign='60'
                                                network.lan.ipaddr='192.168.2.1'

                                                config interface 'lan'  <--Here we have name of
                                                                           config, so we can access
                                                                           option by config name,
                                                                           we dont need index here
                                                                           and here config will
                                                                           be "lan"
                                                  option device 'br-lan'
                                                  option proto 'static'
                                                  option netmask '255.255.255.0'
                                                  option ip6assign '60'
                                                  option ipaddr '192.168.2.1'

                                                network.@device[0]=device
                                                network.@device[0].name='br-lan'
                                                network.@device[0].type='bridge'
                                                network.@device[0].ports='eth0' 'eth0' 'eth1' 'rndis0' 'ecm0'

                                                In this case,we just have
                                                config device         <-- Here we dont have a name,
                                                                          so we have to use index to
                                                                          get or set option values.
                                                                          Hence,Here config will
                                                                          be "device"
                                                  option name 'br-lan'
                                                  option type 'bridge'
                                                  list ports 'eth0'
                                                  list ports 'eth0'
                                                  list ports 'eth1'
                                                  list ports 'rndis0'
                                                  list ports 'ecm0'
  param[in]          option                     uci option name
  param[in]          result                     to store the output of uci get command
  param[in]          index                      index

@return
  true - Success
  false - Failure

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
boolean QCMAP_LAN_Client::UciGetUtility
(
  const char* filename,
  const char* config,
  boolean     IsConfigNameProvided,
  const char* option,
  char*       result,
  int         index = 0,
  int *err_num = 0
)
{
  char cmd[QCMAP_MAX_COMMAND_LEN]={0};
  char res[QCMAP_MAX_SCAN_SIZE]={0};

  if (filename == NULL || config == NULL || option == NULL || result==NULL)
  {
    LOG_MSG_ERROR("Arguments passed are NULL",0,0,0);
    return false;
  }
  if(IsConfigNameProvided)
  {
    /* then we can get option value by config name */
    snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "%s %s.%s.%s",
             UCI_GET_COMMAND, filename, config, option);
  }
  else
  {
    snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "%s %s.@%s[%d].%s",
             UCI_GET_COMMAND, filename, config, index, option);
  }
  if(!ExecuteSystemCmd((const char *)cmd,res,sizeof(res),err_num))
  {
    LOG_MSG_ERROR("Failed to execute command : %s, err: %d ",cmd,err_num,0);
    return false;
  }
  strlcpy(result, res, strlen(res));
  return true;
}


/*===========================================================================
FUNCTION CheckIfFileExists()
===========================================================================*/
/** @ingroup qcmap_check_if_file_exists

  Checks if file exists.

  @param[in]      flename           file name

  @return
  TRUE -- Success \n
  FALSE -- Failure \n
*/
/*=========================================================================*/
boolean QCMAP_LAN_Client::CheckIfFileExists
(
  char *filename
)
{
  struct stat buffer;
  bzero(&buffer, sizeof(buffer));
  if ((stat(filename, &buffer) == 0) && (S_ISREG(buffer.st_mode))) {
    LOG_MSG_INFO1("File %s exists", filename, 0, 0);
    return true;
  }
  else {
    LOG_MSG_INFO1("File %s not exists", filename, 0, 0);
    return false;
  }
} /* End CheckIfFileExists */


/*===========================================================================
  FUNCTION AddFireWallEntryUtilityV6
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

boolean QCMAP_LAN_Client::AddFireWallEntryUtilityV6
(
  qcmap_msgr_firewall_entry_conf_t *firewall_entry,
  int idx,
  int count
)
{
  char command[MAX_COMMAND_STR_LEN] = {0};
  char extra_command[MAX_COMMAND_STR_LEN] = {0};
  char tmp[MAX_COMMAND_STR_LEN]={0};
  /* Initialize next_hdr_prot to 0 (NO_PROTO) */
  uint8_t next_hdr_prot = 0;

  if( firewall_entry == NULL )
  {
    LOG_MSG_ERROR("Pointer to firewall_entry is NULL",0,0,0);
    return false;
  }
  switch( firewall_entry->filter_spec.ip_vsn )
  {
    case IP_V6:
    {
      char str[INET6_ADDRSTRLEN]={0};
      snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh set qcmap_firewall.@firewall_rule[%d].family='%s'",count,IP_V6_STRING);
      ds_system_call(command, strlen(command));
      snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh set firewall.@rule[%d].family='%s'",idx,IP_V6_STRING);
      ds_system_call(command, strlen(command));
      /* If user inputs IPV6 src address and IPV6 Prefix Length */
      if ( firewall_entry->filter_spec.ip_hdr.v6.field_mask & IPFLTR_MASK_IP6_SRC_ADDR )
      {
        readable_addr(AF_INET6,(uint32_t *)&firewall_entry->filter_spec.ip_hdr.v6.src.addr.in6_u.u6_addr32,(char *)&str);
        snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh set qcmap_firewall.@firewall_rule[%d].src_addr='%s'",count,str);
        ds_system_call(command, strlen(command));
        snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh set qcmap_firewall.@firewall_rule[%d].src_prefix_length='%d'",
          count,firewall_entry->filter_spec.ip_hdr.v6.src.prefix_len);
        ds_system_call(command, strlen(command));
        snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh set firewall.@rule[%d].src_ip='%s/%d'",
          idx,str,firewall_entry->filter_spec.ip_hdr.v6.src.prefix_len);
        ds_system_call(command, strlen(command));
      }
      else
      {
        snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh set qcmap_firewall.@firewall_rule[%d].src_addr='%s'",count,"Any");
        ds_system_call(command, strlen(command));
      }

      memset(str,0,INET6_ADDRSTRLEN);

      /* If user inputs IPV6 destination address and IPV6 Prefix Length */
      if ( firewall_entry->filter_spec.ip_hdr.v6.field_mask & IPFLTR_MASK_IP6_DST_ADDR )
      {
        readable_addr(AF_INET6,(uint32_t *)&firewall_entry->filter_spec.ip_hdr.v6.dst.addr.in6_u.u6_addr32,(char *)&str);
        snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh set qcmap_firewall.@firewall_rule[%d].dest_addr='%s'",count,str);
        ds_system_call(command, strlen(command));
        snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh set qcmap_firewall.@firewall_rule[%d].dest_prefix_length='%d'",
          count,firewall_entry->filter_spec.ip_hdr.v6.dst.prefix_len);
        ds_system_call(command, strlen(command));
        snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh set firewall.@rule[%d].dest_ip='%s/%d'",
          idx,str,firewall_entry->filter_spec.ip_hdr.v6.dst.prefix_len);
        ds_system_call(command, strlen(command));
      }
      else
      {
        snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh set qcmap_firewall.@firewall_rule[%d].dest_addr='%s'",count,"Any");
        ds_system_call(command, strlen(command));
      }

      memset(str,0,INET6_ADDRSTRLEN);

      /* If user inputs IPV6 traffic class */
      if(firewall_entry->filter_spec.ip_hdr.v4.field_mask & IPFLTR_MASK_IP6_TRAFFIC_CLASS)
      {
        snprintf(tmp, MAX_COMMAND_STR_LEN," -m tos --tos 0x%x/0x%x ",
              firewall_entry->filter_spec.ip_hdr.v6.trf_cls.val,
              firewall_entry->filter_spec.ip_hdr.v6.trf_cls.mask);
        strlcat(extra_command, tmp, MAX_COMMAND_STR_LEN);
        memset(tmp,0,MAX_COMMAND_STR_LEN);
        snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh set qcmap_firewall.@firewall_rule[%d].tos_value='%x'",
          count,firewall_entry->filter_spec.ip_hdr.v6.trf_cls.val);
        ds_system_call(command, strlen(command));
        snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh set qcmap_firewall.@firewall_rule[%d].tos_mask='%x'",
          count,firewall_entry->filter_spec.ip_hdr.v6.trf_cls.mask);
        ds_system_call(command, strlen(command));
      }
      else
      {
        snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh set qcmap_firewall.@firewall_rule[%d].tos_value='%s'",count,"Any");
        ds_system_call(command, strlen(command));
      }

      /* If user inputs IPV6 protocol */
      if( firewall_entry->filter_spec.ip_hdr.v6.field_mask & IPFLTR_MASK_IP6_NEXT_HDR_PROT )
      {
        next_hdr_prot = firewall_entry->filter_spec.ip_hdr.v6.next_hdr_prot;
      }
      else
      {
        snprintf(command, MAX_COMMAND_STR_LEN,"/etc/data/uci_ex.sh set firewall.@rule[%d].proto='0'",idx);
        ds_system_call(command, strlen(command));
        snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh set qcmap_firewall.@firewall_rule[%d].proto='%s'",count,"Any");
        ds_system_call(command, strlen(command));
      }
      break;
    }
    default:
      LOG_MSG_ERROR("Unsupported IP Version", 0, 0, 0);
      return false;

  }
  LOG_MSG_INFO1( "Next header protocol is %d ", next_hdr_prot, 0, 0 );

  /* On the basis of protocol we are installing rules */
  switch(next_hdr_prot)
  {
    case PS_IPPROTO_TCP:
      snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh set qcmap_firewall.@firewall_rule[%d].proto='%s'",count,TCP_PROTO);
      ds_system_call(command, strlen(command));
      snprintf(command, MAX_COMMAND_STR_LEN,"/etc/data/uci_ex.sh set firewall.@rule[%d].proto='%s'",idx,TCP_PROTO);
      ds_system_call(command, strlen(command));
      if(firewall_entry->filter_spec.next_prot_hdr.tcp.field_mask & IPFLTR_MASK_TCP_SRC_PORT )
      {
        if( firewall_entry->filter_spec.next_prot_hdr.tcp.src.range !=0 )
        {
          snprintf(command,MAX_COMMAND_STR_LEN,"/etc/data/uci_ex.sh set firewall.@rule[%d].src_port='%d:%d'",idx,
                   firewall_entry->filter_spec.next_prot_hdr.tcp.src.port,
                  ( firewall_entry->filter_spec.next_prot_hdr.tcp.src.port +
                    firewall_entry->filter_spec.next_prot_hdr.tcp.src.range));
          ds_system_call(command, strlen(command));
          snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh set qcmap_firewall.@firewall_rule[%d].src_port='%d'",
            count,firewall_entry->filter_spec.next_prot_hdr.tcp.src.port);
          ds_system_call(command, strlen(command));
          snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh set qcmap_firewall.@firewall_rule[%d].src_port_range='%d'",
            count,firewall_entry->filter_spec.next_prot_hdr.tcp.src.range);
          ds_system_call(command, strlen(command));
        }
        else
        {
          snprintf(command,MAX_COMMAND_STR_LEN,"/etc/data/uci_ex.sh set firewall.@rule[%d].src_port='%d' ",idx,
                   firewall_entry->filter_spec.next_prot_hdr.tcp.src.port);
          ds_system_call(command, strlen(command));
          snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh set qcmap_firewall.@firewall_rule[%d].src_port='%d'",
            count,firewall_entry->filter_spec.next_prot_hdr.tcp.src.port);
          ds_system_call(command, strlen(command));
        }
      }
      else
      {
        snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh set qcmap_firewall.@firewall_rule[%d].src_port='%s'",
          count,"Any");
        ds_system_call(command, strlen(command));
      }

      if(firewall_entry->filter_spec.next_prot_hdr.tcp.field_mask & IPFLTR_MASK_TCP_DST_PORT )
      {
        if ( firewall_entry->filter_spec.next_prot_hdr.tcp.dst.range !=0 )
        {
          snprintf(command,MAX_COMMAND_STR_LEN,"/etc/data/uci_ex.sh set firewall.@rule[%d].dest_port='%d:%d'",idx,
                   firewall_entry->filter_spec.next_prot_hdr.tcp.dst.port,
                  ( firewall_entry->filter_spec.next_prot_hdr.tcp.dst.port +
                    firewall_entry->filter_spec.next_prot_hdr.tcp.dst.range));
          ds_system_call(command, strlen(command));
          snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh set qcmap_firewall.@firewall_rule[%d].dest_port='%d'",
            count,firewall_entry->filter_spec.next_prot_hdr.tcp.dst.port);
          ds_system_call(command, strlen(command));
          snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh set qcmap_firewall.@firewall_rule[%d].dest_port_range='%d'",
            count,firewall_entry->filter_spec.next_prot_hdr.tcp.dst.range);
          ds_system_call(command, strlen(command));
        }
        else
        {
          snprintf(command,MAX_COMMAND_STR_LEN,"/etc/data/uci_ex.sh set firewall.@rule[%d].dest_port='%d' ",idx,
                   firewall_entry->filter_spec.next_prot_hdr.tcp.dst.port);
           ds_system_call(command, strlen(command));
           snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh set qcmap_firewall.@firewall_rule[%d].dest_port='%d'",
            count,firewall_entry->filter_spec.next_prot_hdr.tcp.dst.port);
           ds_system_call(command, strlen(command));
        }
      }
      else
      {
        snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh set qcmap_firewall.@firewall_rule[%d].dest_port='%s'",
          count,"Any");
        ds_system_call(command, strlen(command));
      }
      break;

    case PS_IPPROTO_UDP:
      snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh set qcmap_firewall.@firewall_rule[%d].proto='%s'",count,UDP_PROTO);
      ds_system_call(command, strlen(command));
      snprintf(command, MAX_COMMAND_STR_LEN,"/etc/data/uci_ex.sh set firewall.@rule[%d].proto='%s'",idx,UDP_PROTO);
      ds_system_call(command, strlen(command));
      if(firewall_entry->filter_spec.next_prot_hdr.udp.field_mask & IPFLTR_MASK_UDP_SRC_PORT )
      {
        if( firewall_entry->filter_spec.next_prot_hdr.udp.src.range !=0 )
        {
          snprintf(command,MAX_COMMAND_STR_LEN,"/etc/data/uci_ex.sh set firewall.@rule[%d].src_port='%d:%d'",idx,
                   firewall_entry->filter_spec.next_prot_hdr.udp.src.port,
                  ( firewall_entry->filter_spec.next_prot_hdr.udp.src.port +
                    firewall_entry->filter_spec.next_prot_hdr.udp.src.range));
          ds_system_call(command, strlen(command));
          snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh set qcmap_firewall.@firewall_rule[%d].src_port='%d'",
            count,firewall_entry->filter_spec.next_prot_hdr.udp.src.port);
          ds_system_call(command, strlen(command));
          snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh set qcmap_firewall.@firewall_rule[%d].src_port_range='%d'",
            count,firewall_entry->filter_spec.next_prot_hdr.udp.src.range);
          ds_system_call(command, strlen(command));
        }
        else
        {
          snprintf(command,MAX_COMMAND_STR_LEN,"/etc/data/uci_ex.sh set firewall.@rule[%d].src_port='%d' ",idx,
                   firewall_entry->filter_spec.next_prot_hdr.udp.src.port);
          ds_system_call(command, strlen(command));
          snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh set qcmap_firewall.@firewall_rule[%d].src_port='%d'",
            count,firewall_entry->filter_spec.next_prot_hdr.udp.src.port);
          ds_system_call(command, strlen(command));
        }
      }
      else
      {
        snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh set qcmap_firewall.@firewall_rule[%d].src_port='%s'",
          count,"Any");
        ds_system_call(command, strlen(command));
      }
      if(firewall_entry->filter_spec.next_prot_hdr.udp.field_mask & IPFLTR_MASK_UDP_DST_PORT )
      {
        if ( firewall_entry->filter_spec.next_prot_hdr.udp.dst.range !=0 )
        {
          snprintf(command,MAX_COMMAND_STR_LEN,"/etc/data/uci_ex.sh set firewall.@rule[%d].dest_port='%d:%d'",idx,
                   firewall_entry->filter_spec.next_prot_hdr.udp.dst.port,
                  ( firewall_entry->filter_spec.next_prot_hdr.udp.dst.port +
                    firewall_entry->filter_spec.next_prot_hdr.udp.dst.range));
          ds_system_call(command, strlen(command));
          snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh set qcmap_firewall.@firewall_rule[%d].dest_port='%d'",
            count,firewall_entry->filter_spec.next_prot_hdr.udp.dst.port);
          ds_system_call(command, strlen(command));
          snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh set qcmap_firewall.@firewall_rule[%d].dest_port_range='%d'",
            count,firewall_entry->filter_spec.next_prot_hdr.udp.dst.range);
          ds_system_call(command, strlen(command));
       }
       else
       {
         snprintf(command,MAX_COMMAND_STR_LEN,"/etc/data/uci_ex.sh set firewall.@rule[%d].dest_port='%d' ",idx,
                   firewall_entry->filter_spec.next_prot_hdr.udp.dst.port);
         ds_system_call(command, strlen(command));
         snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh set qcmap_firewall.@firewall_rule[%d].dest_port='%d'",
          count,firewall_entry->filter_spec.next_prot_hdr.udp.dst.port);
         ds_system_call(command, strlen(command));
       }
      }
      else
      {
        snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh set qcmap_firewall.@firewall_rule[%d].dest_port='%s'",
          count,"Any");
        ds_system_call(command, strlen(command));
      }
      break;

    case PS_IPPROTO_TCP_UDP:

     snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh set qcmap_firewall.@firewall_rule[%d].proto='%s'",count,TCP_UDP_PROTO);
     ds_system_call(command, strlen(command));
     snprintf(command, MAX_COMMAND_STR_LEN,"/etc/data/uci_ex.sh set firewall.@rule[%d].proto='%s'",idx,TCP_UDP_PROTO);
     ds_system_call(command, strlen(command));
     if( firewall_entry->filter_spec.next_prot_hdr.tcp_udp_port_range.field_mask &
         IPFLTR_MASK_TCP_UDP_SRC_PORT )
     {
       snprintf(command,MAX_COMMAND_STR_LEN,"/etc/data/uci_ex.sh set firewall.@rule[%d].src_port='%d:%d'",idx,
                   firewall_entry->filter_spec.next_prot_hdr.tcp.src.port,
                  ( firewall_entry->filter_spec.next_prot_hdr.tcp.src.port +
                    firewall_entry->filter_spec.next_prot_hdr.tcp.src.range));
       ds_system_call(command, strlen(command));
       snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh set qcmap_firewall.@firewall_rule[%d].src_port='%d'",
        count,firewall_entry->filter_spec.next_prot_hdr.tcp.src.port);
       ds_system_call(command, strlen(command));
       snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh set qcmap_firewall.@firewall_rule[%d].src_port_range='%d'",
        count,firewall_entry->filter_spec.next_prot_hdr.tcp.src.range);
       ds_system_call(command, strlen(command));
     }
     else
     {
        snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh set qcmap_firewall.@firewall_rule[%d].src_port='%s'",
                 count,"Any");
        ds_system_call(command, strlen(command));
     }

     if ( firewall_entry->filter_spec.next_prot_hdr.tcp_udp_port_range.field_mask &
          IPFLTR_MASK_TCP_UDP_DST_PORT )
     {
       snprintf(command,MAX_COMMAND_STR_LEN,"/etc/data/uci_ex.sh set firewall.@rule[%d].dest_port='%d:%d'",idx,
                   firewall_entry->filter_spec.next_prot_hdr.tcp.dst.port,
                  ( firewall_entry->filter_spec.next_prot_hdr.tcp.dst.port +
                    firewall_entry->filter_spec.next_prot_hdr.tcp.dst.range));
       ds_system_call(command, strlen(command));
       snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh set qcmap_firewall.@firewall_rule[%d].dest_port='%d'",
        count,firewall_entry->filter_spec.next_prot_hdr.tcp.dst.port);
       ds_system_call(command, strlen(command));
       snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh set qcmap_firewall.@firewall_rule[%d].dest_port_range='%d'",
        count,firewall_entry->filter_spec.next_prot_hdr.tcp.dst.range);
       ds_system_call(command, strlen(command));
     }
     else
     {
        snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh set qcmap_firewall.@firewall_rule[%d].dest_port='%s'",
          count,"Any");
        ds_system_call(command, strlen(command));
     }
     break;

   case PS_IPPROTO_ICMP6:
     snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh set qcmap_firewall.@firewall_rule[%d].proto='%s'",count,ICMP_PROTO);
     ds_system_call(command, strlen(command));
     snprintf(command, MAX_COMMAND_STR_LEN,"/etc/data/uci_ex.sh set firewall.@rule[%d].proto='%s'",idx,ICMP_PROTO);
     ds_system_call(command, strlen(command));

     if (( firewall_entry->filter_spec.next_prot_hdr.icmp.field_mask &
          IPFLTR_MASK_ICMP_MSG_TYPE ) && ( firewall_entry->filter_spec.next_prot_hdr.icmp.field_mask &
          IPFLTR_MASK_ICMP_MSG_CODE ))
     {
       snprintf(command,MAX_COMMAND_STR_LEN,"/etc/data/uci_ex.sh set firewall.@rule[%d].icmp_type='%d/%d' ",idx,
                firewall_entry->filter_spec.next_prot_hdr.icmp.type,
                firewall_entry->filter_spec.next_prot_hdr.icmp.code);
       ds_system_call(command, strlen(command));
       snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh set qcmap_firewall.@firewall_rule[%d].icmp6_type='%d'",
        count,firewall_entry->filter_spec.next_prot_hdr.icmp.type);
       ds_system_call(command, strlen(command));
       snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh set qcmap_firewall.@firewall_rule[%d].icmp6_code='%d'",
        count,firewall_entry->filter_spec.next_prot_hdr.icmp.code);
       ds_system_call(command, strlen(command));
     }
     else if ( firewall_entry->filter_spec.next_prot_hdr.icmp.field_mask &
          IPFLTR_MASK_ICMP_MSG_TYPE )
     {
       snprintf(command,MAX_COMMAND_STR_LEN,"/etc/data/uci_ex.sh set firewall.@rule[%d].icmp_type='%d'",
        idx,firewall_entry->filter_spec.next_prot_hdr.icmp.type);
       ds_system_call(command, strlen(command));
       snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh set qcmap_firewall.@firewall_rule[%d].icmp6_type='%d'",
        count,firewall_entry->filter_spec.next_prot_hdr.icmp.type);
       ds_system_call(command, strlen(command));
       snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh set qcmap_firewall.@firewall_rule[%d].icmp6_code='%s'",
        count,"Any");
       ds_system_call(command, strlen(command));
     }
     else
     {
       snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh set qcmap_firewall.@firewall_rule[%d].icmp6_type='%s'",
                count,"Any");
       ds_system_call(command, strlen(command));
       snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh set qcmap_firewall.@firewall_rule[%d].icmp6_code='%s'",
                count,"Any");
       ds_system_call(command, strlen(command));
     }
     break;

    case PS_IPPROTO_ESP:
      snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh set qcmap_firewall.@firewall_rule[%d].proto='%s'",count,ESP_PROTO);
      ds_system_call(command, strlen(command));
      snprintf(command, MAX_COMMAND_STR_LEN,"/etc/data/uci_ex.sh set firewall.@rule[%d].proto='%s'",idx,ESP_PROTO);
      ds_system_call(command, strlen(command));
      if(firewall_entry->filter_spec.next_prot_hdr.esp.field_mask & IPFLTR_MASK_ESP_SPI )
      {
        snprintf(tmp,MAX_COMMAND_STR_LEN," --espspi %d ",
                   firewall_entry->filter_spec.next_prot_hdr.esp.spi);
        strlcat(extra_command,tmp,MAX_COMMAND_STR_LEN);
        snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh set qcmap_firewall.@firewall_rule[%d].esp_spi='%x'",count,firewall_entry->filter_spec.next_prot_hdr.esp.spi);
        ds_system_call(command, strlen(command));
      }
      else
      {
        snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh set qcmap_firewall.@firewall_rule[%d].esp_spi='%s'",count,"Any");
      }

      break;

    default:
      LOG_MSG_ERROR("Unsupported protocol %d ",next_hdr_prot, 0, 0);
      return false;
  }
  snprintf(command,MAX_COMMAND_STR_LEN,"/etc/data/uci_ex.sh set firewall.@rule[%d].extra='%s'",idx,extra_command);
  ds_system_call(command, strlen(command));
  LOG_MSG_INFO1("Added FIREWALL Entry Utility Successful...",0,0,0);
  return true;
}

/*===========================================================================
  FUNCTION AddFireWallEntryUtilityV4
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

boolean QCMAP_LAN_Client::AddFireWallEntryUtilityV4
(
  qcmap_msgr_firewall_entry_conf_t *firewall_entry,
  int idx,
  int count
)
{
  char command[MAX_COMMAND_STR_LEN] = {0};
  char extra_command[MAX_COMMAND_STR_LEN] = {0};
  char tmp[MAX_COMMAND_STR_LEN]={0};
  /* Initialize next_hdr_prot to 0 (NO_PROTO) */
  uint8_t next_hdr_prot = 0;

  if( firewall_entry == NULL )
  {
    LOG_MSG_ERROR("Pointer to firewall_entry is NULL",0,0,0);
    return false;
  }
  switch( firewall_entry->filter_spec.ip_vsn )
  {
    case IP_V4:
      char str1[INET6_ADDRSTRLEN];
      char str2[INET6_ADDRSTRLEN];
      snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh set qcmap_firewall.@firewall_rule[%d].family='%s'",count,IP_V4_STRING);
      ds_system_call(command, strlen(command));
      snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh set firewall.@rule[%d].family='%s'",idx,IP_V4_STRING);
      ds_system_call(command, strlen(command));

      /* If user inputs IPV4 destination address and IPV4 Destination addr subnet mask */
      if ( firewall_entry->filter_spec.ip_hdr.v4.field_mask & IPFLTR_MASK_IP4_SRC_ADDR )
      {
        readable_addr(AF_INET,&(firewall_entry->filter_spec.ip_hdr.v4.src.addr.ps_s_addr),(char *)&str1);
        snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh set qcmap_firewall.@firewall_rule[%d].src_addr='%s'",count,str1);
        ds_system_call(command, strlen(command));
        readable_addr(AF_INET,&(firewall_entry->filter_spec.ip_hdr.v4.src.subnet_mask.ps_s_addr),(char *)&str2);
        snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh set qcmap_firewall.@firewall_rule[%d].src_addr_mask='%s'",count,str2);
        ds_system_call(command, strlen(command));
        snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh set firewall.@rule[%d].src_ip='%s/%s'",idx,str1,str2);
        ds_system_call(command, strlen(command));
      }
      else
      {
        snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh set qcmap_firewall.@firewall_rule[%d].src_addr='%s'",count,"Any");
        ds_system_call(command, strlen(command));
      }

      memset(str1,0,INET6_ADDRSTRLEN);
      memset(str2,0,INET6_ADDRSTRLEN);

      /* If user inputs IPV4 destination address and IPV4 Destination addr subnet mask */
      if ( firewall_entry->filter_spec.ip_hdr.v4.field_mask & IPFLTR_MASK_IP4_DST_ADDR )
      {
        readable_addr(AF_INET,&(firewall_entry->filter_spec.ip_hdr.v4.dst.addr.ps_s_addr),(char *)&str1);
        snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh set qcmap_firewall.@firewall_rule[%d].dest_addr='%s'",count,str1);
        ds_system_call(command, strlen(command));
        readable_addr(AF_INET,&(firewall_entry->filter_spec.ip_hdr.v4.dst.subnet_mask.ps_s_addr),(char *)&str2);
        snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh set qcmap_firewall.@firewall_rule[%d].dest_addr_mask='%s'",count,str2);
        ds_system_call(command, strlen(command));
        snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh set firewall.@rule[%d].dest_ip='%s/%s'",idx,str1,str2);
        ds_system_call(command, strlen(command));
      }
      else
      {
        snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh set qcmap_firewall.@firewall_rule[%d].dest_addr='%s'",count,"Any");
        ds_system_call(command, strlen(command));
      }

      memset(str1,0,INET6_ADDRSTRLEN);
      memset(str2,0,INET6_ADDRSTRLEN);

      /* If user inputs IPV4 tos */
      if ( firewall_entry->filter_spec.ip_hdr.v4.field_mask & IPFLTR_MASK_IP4_TOS )
      {
        snprintf( tmp,MAX_COMMAND_STR_LEN," -m tos --tos 0x%x/0x%x ",
                  firewall_entry->filter_spec.ip_hdr.v4.tos.val,
                  firewall_entry->filter_spec.ip_hdr.v4.tos.mask );

        strlcat(extra_command,tmp,MAX_COMMAND_STR_LEN);
        memset(tmp,0,MAX_COMMAND_STR_LEN);
        snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh set qcmap_firewall.@firewall_rule[%d].tos_value='%x'",
          count,firewall_entry->filter_spec.ip_hdr.v4.tos.val);
        ds_system_call(command, strlen(command));
        snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh set qcmap_firewall.@firewall_rule[%d].tos_mask='%x'",
          count,firewall_entry->filter_spec.ip_hdr.v4.tos.mask);
        ds_system_call(command, strlen(command));
      }
      else
      {
        snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh set qcmap_firewall.@firewall_rule[%d].tos_value='%s'",count,"Any");
        ds_system_call(command, strlen(command));
      }

      /* If user inputs IPV4 protocol */
      if( firewall_entry->filter_spec.ip_hdr.v4.field_mask & IPFLTR_MASK_IP4_NEXT_HDR_PROT )
      {
        next_hdr_prot = firewall_entry->filter_spec.ip_hdr.v4.next_hdr_prot;
      }
      else
      {
        snprintf(command, MAX_COMMAND_STR_LEN,"/etc/data/uci_ex.sh set firewall.@rule[%d].proto='0'",idx);
        ds_system_call(command, strlen(command));
        snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh set qcmap_firewall.@firewall_rule[%d].proto='%s'",count,"Any");
        ds_system_call(command, strlen(command));
      }
      break;
    default:
      LOG_MSG_ERROR("Unsupported IP Version ", 0, 0, 0);
      return false;
  }
  LOG_MSG_INFO1( "Next header protocol is %d ", next_hdr_prot, 0, 0 );

  /* According to the protocol, we are installing the firewall rules */
  switch(next_hdr_prot)
  {
    case PS_IPPROTO_TCP:
      snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh set qcmap_firewall.@firewall_rule[%d].proto='%s'",count,TCP_PROTO);
      ds_system_call(command, strlen(command));
      snprintf(command, MAX_COMMAND_STR_LEN,"/etc/data/uci_ex.sh set firewall.@rule[%d].proto='%s'",idx,TCP_PROTO);
      ds_system_call(command, strlen(command));
      if(firewall_entry->filter_spec.next_prot_hdr.tcp.field_mask & IPFLTR_MASK_TCP_SRC_PORT )
      {
        if( firewall_entry->filter_spec.next_prot_hdr.tcp.src.range !=0 )
        {
          snprintf(command,MAX_COMMAND_STR_LEN,"/etc/data/uci_ex.sh set firewall.@rule[%d].src_port='%d:%d'",idx,
                   firewall_entry->filter_spec.next_prot_hdr.tcp.src.port,
                  ( firewall_entry->filter_spec.next_prot_hdr.tcp.src.port +
                    firewall_entry->filter_spec.next_prot_hdr.tcp.src.range));
          ds_system_call(command, strlen(command));
          snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh set qcmap_firewall.@firewall_rule[%d].src_port='%d'",
            count,firewall_entry->filter_spec.next_prot_hdr.tcp.src.port);
          ds_system_call(command, strlen(command));
          snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh set qcmap_firewall.@firewall_rule[%d].src_port_range='%d'",
            count,firewall_entry->filter_spec.next_prot_hdr.tcp.src.range);
          ds_system_call(command, strlen(command));
        }
        else
        {
          snprintf(command,MAX_COMMAND_STR_LEN,"/etc/data/uci_ex.sh set firewall.@rule[%d].src_port='%d' ",idx,
                   firewall_entry->filter_spec.next_prot_hdr.tcp.src.port);
          ds_system_call(command, strlen(command));
          snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh set qcmap_firewall.@firewall_rule[%d].src_port='%d'",
            count,firewall_entry->filter_spec.next_prot_hdr.tcp.src.port);
          ds_system_call(command, strlen(command));
        }
      }
      else
      {
        snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh set qcmap_firewall.@firewall_rule[%d].src_port='%s'",
                 count,"Any");
        ds_system_call(command, strlen(command));
      }
      if(firewall_entry->filter_spec.next_prot_hdr.tcp.field_mask & IPFLTR_MASK_TCP_DST_PORT )
      {
        if ( firewall_entry->filter_spec.next_prot_hdr.tcp.dst.range !=0 )
        {
          snprintf(command,MAX_COMMAND_STR_LEN,"/etc/data/uci_ex.sh set firewall.@rule[%d].dest_port='%d:%d'",idx,
                   firewall_entry->filter_spec.next_prot_hdr.tcp.dst.port,
                  ( firewall_entry->filter_spec.next_prot_hdr.tcp.dst.port +
                    firewall_entry->filter_spec.next_prot_hdr.tcp.dst.range));
          ds_system_call(command, strlen(command));
          snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh set qcmap_firewall.@firewall_rule[%d].dest_port='%d'",
            count,firewall_entry->filter_spec.next_prot_hdr.tcp.dst.port);
          ds_system_call(command, strlen(command));
          snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh set qcmap_firewall.@firewall_rule[%d].dest_port_range='%d'",
            count,firewall_entry->filter_spec.next_prot_hdr.tcp.dst.range);
          ds_system_call(command, strlen(command));
        }
        else
        {
          snprintf(command,MAX_COMMAND_STR_LEN,"/etc/data/uci_ex.sh set firewall.@rule[%d].dest_port='%d' ",idx,
                   firewall_entry->filter_spec.next_prot_hdr.tcp.dst.port);
           ds_system_call(command, strlen(command));
           snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh set qcmap_firewall.@firewall_rule[%d].dest_port='%d'",
            count,firewall_entry->filter_spec.next_prot_hdr.tcp.dst.port);
           ds_system_call(command, strlen(command));
        }
      }
      else
      {
        snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh set qcmap_firewall.@firewall_rule[%d].dest_port='%s'",
          count,"Any");
        ds_system_call(command, strlen(command));
      }
      break;

    case PS_IPPROTO_UDP:
      snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh set qcmap_firewall.@firewall_rule[%d].proto='%s'",count,UDP_PROTO);
      ds_system_call(command, strlen(command));
      snprintf(command, MAX_COMMAND_STR_LEN,"/etc/data/uci_ex.sh set firewall.@rule[%d].proto='%s'",idx,UDP_PROTO);
      ds_system_call(command, strlen(command));
      if(firewall_entry->filter_spec.next_prot_hdr.udp.field_mask & IPFLTR_MASK_UDP_SRC_PORT )
      {
        if( firewall_entry->filter_spec.next_prot_hdr.udp.src.range !=0 )
        {
          snprintf(command,MAX_COMMAND_STR_LEN,"/etc/data/uci_ex.sh set firewall.@rule[%d].src_port='%d:%d'",idx,
                   firewall_entry->filter_spec.next_prot_hdr.udp.src.port,
                  ( firewall_entry->filter_spec.next_prot_hdr.udp.src.port +
                    firewall_entry->filter_spec.next_prot_hdr.udp.src.range));
          ds_system_call(command, strlen(command));
          snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh set qcmap_firewall.@firewall_rule[%d].src_port='%d'",
            count,firewall_entry->filter_spec.next_prot_hdr.udp.src.port);
          ds_system_call(command, strlen(command));
          snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh set qcmap_firewall.@firewall_rule[%d].src_port_range='%d'",
            count,firewall_entry->filter_spec.next_prot_hdr.udp.src.range);
          ds_system_call(command, strlen(command));
        }
        else
        {
          snprintf(command,MAX_COMMAND_STR_LEN,"/etc/data/uci_ex.sh set firewall.@rule[%d].src_port='%d' ",idx,
                   firewall_entry->filter_spec.next_prot_hdr.udp.src.port);
          ds_system_call(command, strlen(command));
          snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh set qcmap_firewall.@firewall_rule[%d].src_port='%d'",
            count,firewall_entry->filter_spec.next_prot_hdr.udp.src.port);
          ds_system_call(command, strlen(command));
        }
      }
      else
      {
        snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh set qcmap_firewall.@firewall_rule[%d].src_port='%s'",
                 count,"Any");
        ds_system_call(command, strlen(command));
      }

      if(firewall_entry->filter_spec.next_prot_hdr.udp.field_mask & IPFLTR_MASK_UDP_DST_PORT )
      {
        if ( firewall_entry->filter_spec.next_prot_hdr.udp.dst.range !=0 )
        {
          snprintf(command,MAX_COMMAND_STR_LEN,"/etc/data/uci_ex.sh set firewall.@rule[%d].dest_port='%d:%d'",idx,
                   firewall_entry->filter_spec.next_prot_hdr.udp.dst.port,
                  ( firewall_entry->filter_spec.next_prot_hdr.udp.dst.port +
                    firewall_entry->filter_spec.next_prot_hdr.udp.dst.range));
          ds_system_call(command, strlen(command));
          snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh set qcmap_firewall.@firewall_rule[%d].dest_port='%d'",
            count,firewall_entry->filter_spec.next_prot_hdr.udp.dst.port);
          ds_system_call(command, strlen(command));
          snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh set qcmap_firewall.@firewall_rule[%d].dest_port_range='%d'",
            count,firewall_entry->filter_spec.next_prot_hdr.udp.dst.range);
          ds_system_call(command, strlen(command));
       }
       else
       {
         snprintf(command,MAX_COMMAND_STR_LEN,"/etc/data/uci_ex.sh set firewall.@rule[%d].dest_port='%d' ",idx,
                   firewall_entry->filter_spec.next_prot_hdr.udp.dst.port);
         ds_system_call(command, strlen(command));
         snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh set qcmap_firewall.@firewall_rule[%d].dest_port='%d'",
          count,firewall_entry->filter_spec.next_prot_hdr.udp.dst.port);
         ds_system_call(command, strlen(command));
       }
      }
      else
      {
        snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh set qcmap_firewall.@firewall_rule[%d].dest_port='%s'",
          count,"Any");
        ds_system_call(command, strlen(command));
      }
      break;

    case PS_IPPROTO_TCP_UDP:

     snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh set qcmap_firewall.@firewall_rule[%d].proto='%s'",count,TCP_UDP_PROTO);
     ds_system_call(command, strlen(command));
     snprintf(command, MAX_COMMAND_STR_LEN,"/etc/data/uci_ex.sh set firewall.@rule[%d].proto='%s'",idx,TCP_UDP_PROTO);
     ds_system_call(command, strlen(command));
     if( firewall_entry->filter_spec.next_prot_hdr.tcp_udp_port_range.field_mask &
         IPFLTR_MASK_TCP_UDP_SRC_PORT )
     {
       snprintf(command,MAX_COMMAND_STR_LEN,"/etc/data/uci_ex.sh set firewall.@rule[%d].src_port='%d:%d'",idx,
                   firewall_entry->filter_spec.next_prot_hdr.tcp.src.port,
                  ( firewall_entry->filter_spec.next_prot_hdr.tcp.src.port +
                    firewall_entry->filter_spec.next_prot_hdr.tcp.src.range));
       ds_system_call(command, strlen(command));
       snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh set qcmap_firewall.@firewall_rule[%d].src_port='%d'",
        count,firewall_entry->filter_spec.next_prot_hdr.tcp.src.port);
       ds_system_call(command, strlen(command));
       snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh set qcmap_firewall.@firewall_rule[%d].src_port_range='%d'",
        count,firewall_entry->filter_spec.next_prot_hdr.tcp.src.range);
       ds_system_call(command, strlen(command));
     }
     else
     {
        snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh set qcmap_firewall.@firewall_rule[%d].src_port='%s'",
          count,"Any");
        ds_system_call(command, strlen(command));
     }

     if ( firewall_entry->filter_spec.next_prot_hdr.tcp_udp_port_range.field_mask &
          IPFLTR_MASK_TCP_UDP_DST_PORT )
     {
       snprintf(command,MAX_COMMAND_STR_LEN,"/etc/data/uci_ex.sh set firewall.@rule[%d].dest_port='%d:%d'",idx,
                   firewall_entry->filter_spec.next_prot_hdr.tcp.dst.port,
                  ( firewall_entry->filter_spec.next_prot_hdr.tcp.dst.port +
                    firewall_entry->filter_spec.next_prot_hdr.tcp.dst.range));
       ds_system_call(command, strlen(command));
       snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh set qcmap_firewall.@firewall_rule[%d].dest_port='%d'",
        count,firewall_entry->filter_spec.next_prot_hdr.tcp.dst.port);
       ds_system_call(command, strlen(command));
       snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh set qcmap_firewall.@firewall_rule[%d].dest_port_range='%d'",
        count,firewall_entry->filter_spec.next_prot_hdr.tcp.dst.range);
       ds_system_call(command, strlen(command));
     }
     else
     {
        snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh set qcmap_firewall.@firewall_rule[%d].dest_port='%s'",
          count,"Any");
        ds_system_call(command, strlen(command));
     }
     break;

   case PS_IPPROTO_ICMP:
     snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh set qcmap_firewall.@firewall_rule[%d].proto='%s'",count,ICMP_PROTO);
     ds_system_call(command, strlen(command));
     snprintf(command, MAX_COMMAND_STR_LEN,"/etc/data/uci_ex.sh set firewall.@rule[%d].proto='%s'",idx,ICMP_PROTO);
     ds_system_call(command, strlen(command));

     if (( firewall_entry->filter_spec.next_prot_hdr.icmp.field_mask &
          IPFLTR_MASK_ICMP_MSG_TYPE ) && ( firewall_entry->filter_spec.next_prot_hdr.icmp.field_mask &
          IPFLTR_MASK_ICMP_MSG_CODE ))
     {
       snprintf(command,MAX_COMMAND_STR_LEN,"/etc/data/uci_ex.sh set firewall.@rule[%d].icmp_type='%d/%d' ",idx,
                firewall_entry->filter_spec.next_prot_hdr.icmp.type,
                firewall_entry->filter_spec.next_prot_hdr.icmp.code);
       ds_system_call(command, strlen(command));
       snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh set qcmap_firewall.@firewall_rule[%d].icmp_type='%d'",
        count,firewall_entry->filter_spec.next_prot_hdr.icmp.type);
       ds_system_call(command, strlen(command));
       snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh set qcmap_firewall.@firewall_rule[%d].icmp_code='%d'",
        count,firewall_entry->filter_spec.next_prot_hdr.icmp.code);
       ds_system_call(command, strlen(command));
     }
     else if ( firewall_entry->filter_spec.next_prot_hdr.icmp.field_mask &
          IPFLTR_MASK_ICMP_MSG_TYPE )
     {
       snprintf(command,MAX_COMMAND_STR_LEN,"/etc/data/uci_ex.sh set firewall.@rule[%d].icmp_type='%d'",
        idx,firewall_entry->filter_spec.next_prot_hdr.icmp.type);
       ds_system_call(command, strlen(command));
       snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh set qcmap_firewall.@firewall_rule[%d].icmp_type='%d'",
        count,firewall_entry->filter_spec.next_prot_hdr.icmp.type);
       ds_system_call(command, strlen(command));
       snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh set qcmap_firewall.@firewall_rule[%d].icmp_code='%s'",
        count,"Any");
       ds_system_call(command, strlen(command));
     }
     else
     {
       snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh set qcmap_firewall.@firewall_rule[%d].icmp_type='%s'",
        count,"Any");
       ds_system_call(command, strlen(command));
       snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh set qcmap_firewall.@firewall_rule[%d].icmp_code='%s'",
        count,"Any");
       ds_system_call(command, strlen(command));
     }
     break;

    case PS_IPPROTO_ESP:
      snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh set qcmap_firewall.@firewall_rule[%d].proto='%s'",count,ESP_PROTO);
      ds_system_call(command, strlen(command));
      snprintf(command, MAX_COMMAND_STR_LEN,"/etc/data/uci_ex.sh set firewall.@rule[%d].proto='%s'",idx,ESP_PROTO);
      ds_system_call(command, strlen(command));
      if(firewall_entry->filter_spec.next_prot_hdr.esp.field_mask & IPFLTR_MASK_ESP_SPI )
      {
        snprintf(tmp,MAX_COMMAND_STR_LEN," --espspi %d ",
                   firewall_entry->filter_spec.next_prot_hdr.esp.spi);
        strlcat(extra_command,tmp,MAX_COMMAND_STR_LEN);
        snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh set qcmap_firewall.@firewall_rule[%d].esp_spi='%x'",
          count,firewall_entry->filter_spec.next_prot_hdr.esp.spi);
        ds_system_call(command, strlen(command));
      }
      else
      {
        snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh set qcmap_firewall.@firewall_rule[%d].esp_spi='%s'",count,"Any");
      }
      break;

    default:
      LOG_MSG_ERROR("Unsupported protocol %d ",next_hdr_prot, 0, 0);
      return false;

  }
  snprintf(command,MAX_COMMAND_STR_LEN,"/etc/data/uci_ex.sh set firewall.@rule[%d].extra='%s'",idx,extra_command);
  ds_system_call(command, strlen(command));
  LOG_MSG_INFO1("Added FIREWALL Entry Utility Successful...",0,0,0);
  return true;
}
/*=====================================================================
  FUNCTION check_non_empty_mac_addr
======================================================================*/
/*!
@brief
  Check for empty mac address

@return
  bool

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
boolean check_non_empty_mac_addr
(
  uint8_t *mac,
  char mac_addr_string[]
)
{
  memset(mac_addr_string,0,QCMAP_LAN_MAC_ADDR_NUM_CHARS);
  snprintf( mac_addr_string,QCMAP_LAN_MAC_ADDR_NUM_CHARS,
            "%02x:%02x:%02x:%02x:%02x:%02x",
           mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
  if ( strncmp(mac_addr_string,MAC_NULL_STR,QCMAP_LAN_MAC_ADDR_NUM_CHARS)
       == 0 )
     return false;
  else
    return true;
} /* End check_non_empty_mac_addr */

/*=====================================================================
  FUNCTION CheckIfNetworkRulesExist
======================================================================*/
/*!
@brief
  - Checks if lan or wan network rules exist in uci database

@return
  true - Success
  false - Failure

@note
  - System call to check if provided lan or wan network rules exist
  in uci database

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
boolean CheckIfNetworkRulesExist
(
  const char *network_interface,
  const uint32_t profile_handle
)
{
  char cmd[QCMAP_MAX_COMMAND_LEN] = {0};
  char result[QCMAP_MAX_SCAN_SIZE] = {0};

  if (NULL == network_interface)
  {
    LOG_MSG_ERROR("Invalid network interface received", 0, 0, 0);
    return false;
  }

  /* Check if required wan uci rules are present */
  if (profile_handle == 1)
    snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "%s %s",
                                    UCI_GET_COMMAND, network_interface);
  else
    snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "%s %s%d",
                UCI_GET_COMMAND, network_interface, profile_handle);

  if (!ExecuteSystemCmd((const char *)cmd, result,
                    sizeof(result)))
  {
    /* Indicates uci rules are missing */
    LOG_MSG_ERROR("Missing network uci rules for PDN: %d", profile_handle, 0, 0);
    return false;
  }
  return true;
} /* End CheckIfNetworkRulesExist */

/*=====================================================================
  FUNCTION GetProfileIndex
======================================================================*/
/*!
@brief
  - Get profile index of passed profile number in qcmap_lan database

@return
  profile_idx

@note
  - returns the profile index of passed profile number in qcmap_lan database

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
int GetProfileIndex
(
  const uint32_t profile_handle
)
{
  char cmd[QCMAP_MAX_COMMAND_LEN] = {0};
  char result[QCMAP_MAX_SCAN_SIZE] = {0};
  int no_of_profiles = -1, profile_idx = -1;

/* Get the profile index in qcmap_lan */
  snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "%s %s",
           UCI_GET_COMMAND, UCI_QUERY_NO_OF_PROFILES);
  if (ExecuteSystemCmd((const char *)cmd, result,
                      sizeof(result)))
  {
    no_of_profiles = atoi(result);
    /* iterate through the profile and find out index of wan<Prof_ID> */
    for (int i = 0; i < no_of_profiles; i++)
    {
      /* get the profile_id */
      memset(cmd, 0, QCMAP_MAX_COMMAND_LEN);
      memset(result, 0, QCMAP_MAX_SCAN_SIZE);
      snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "%s qcmap_lan.@profile[%d].profile_id",
               UCI_GET_COMMAND, i);
      if (ExecuteSystemCmd((const char *)cmd, result,
                      sizeof(result)))
      {
        /* Check if retrieved profile_id is same as profile_handle */
        if (profile_handle == atoi(result))
        {
          profile_idx = i;
          break;
        }
      }
    }
  }
  return profile_idx;
} /* End GetProfileIndex */


/*=====================================================================
  FUNCTION UpdateRmnetFile
======================================================================*/
/*!
@brief
  - Performs network up or down based on an event

@return
  true - Success
  false - Failure

@note
  - System call to check if provided lan or wan network rules exist
  in uci database

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
boolean UpdateRmnetFile
(
  const char *evt,
  const uint32_t profile_handle,
  qcmap_backhaul_type bh_type
)
{
  char cmd[QCMAP_MAX_COMMAND_LEN] = {0};
  bool post_process = false;

  if (NULL == evt)
  {
    LOG_MSG_ERROR("event parameter null passed", 0, 0, 0);
    return false;
  }

  if (evt == RMNET_BRING_UP_CMD && bh_type == BACKHAUL_V4)
  {
    /* Get enable IPPT value from qcmap_lan database */
    /** NOTE: As we are calling "up" on "v4" during reconfiguring of PDN,
     *  i.e., when user disables IPPT on an active call, we have to
     *  clean up IPPT configuration and reconfigure backhaul in Non-IPPT mode.
     *  In this scenario, we have to start mwan3 */
    qcmap_lan_ip_passthrough_mode_enum enable_state;
    enable_state=(qcmap_lan_ip_passthrough_mode_enum)CheckEnableIPPT(profile_handle);
    if (enable_state == QCMAP_LAN_IP_PASSTHROUGH_MODE_DOWN)
    {
      /** Indicates the scenario as described above in note section */
      post_process = CheckMWANHandling(profile_handle, evt);
    }
    else if (enable_state == QCMAP_LAN_IP_PASSTHROUGH_MODE_UP)
    {
      /* Handle MWAN for IPPT calls (On default PDN for now) */
      /** NOTE: in this case, we may have to stop mwan as the call is being brought up.
       *  IPPT related additional checks have been implemented in the below
       *  function */
      PerformStartStopMWAN3(profile_handle, MWAN3_STOP);
    }
  }
  else if (evt == RMNET_TEAR_DOWN_CMD && bh_type == BACKHAUL_V4)
  {
    /** NOTE: Pre-processing and caching of "post_process" flag is
     *  required for following reasons
     *  1. To avoid starting mwan for all the v4 teardown events
     *  2. To start mwan when backhaul is being brought down which was
     *     configured in IPPT mode
     *  3. To start mwan when backhaul is being reconfigured from IPPT
     *     to non-IPPT mode(explained in note section on first if condition) */
    post_process = CheckMWANHandling(profile_handle, evt);
  }

  snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "%s %s %d %d", RMNET_UPDATE_FILE,
              evt, profile_handle, bh_type);
  ds_system_call(cmd, strlen(cmd));

  if (post_process)
  {
    /** NOTE: this indicates we have to start mwan3 process */
    PerformStartStopMWAN3(profile_handle, MWAN3_START);
  }

  return true;
} /* End UpdateRmnetFile */

/*=====================================================================
  FUNCTION UpdateRmnetEthFile
======================================================================*/
/*!
@brief
  - Performs network up or down based on an event for eth pdu call

@return
  true - Success
  false - Failure

@note
  - System call to check if provided lan or wan network rules exist
  in uci database

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
boolean UpdateRmnetEthFile
(
  const char *evt,
  const uint32_t profile_handle
)
{
  char cmd[QCMAP_MAX_COMMAND_LEN] = {0};

  if (NULL == evt)
  {
    LOG_MSG_ERROR("event parameter null passed", 0, 0, 0);
    return false;
  }

  snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "%s %s %d", RMNET_ETH_UPDATE_FILE,
              evt, profile_handle);
  ds_system_call(cmd, strlen(cmd));

  return true;
} /* End UpdateRmnetFile */

/*=====================================================================
  FUNCTION GetActiveIPPT
======================================================================*/
/*!
@brief
  - Get active IPPT value of passed profile number in qcmap_lan database

@return
  active_ippt

@note
  - returns the active IPPT value of passed profile number in qcmap_lan
   database

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
int GetActiveIPPT
(
  const uint32_t profile_idx
)
{
  char cmd[QCMAP_MAX_COMMAND_LEN] = {0};
  char result[QCMAP_MAX_SCAN_SIZE] = {0};
  int active_ippt = -1;

  snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "%s qcmap_lan.@profile[%d].active_ippt",
             UCI_GET_COMMAND, profile_idx);
  if (!ExecuteSystemCmd((const char *)cmd, result,
                      sizeof(result)))
  {
    LOG_MSG_ERROR("Failed to get active_ippt value from qcmap_lan database for"
                  " profile idx:%d", profile_idx, 0, 0);
    return QCMAP_LAN_INVALID;
  }
  active_ippt = atoi(result);
  return active_ippt;
} /* End GetActiveIPPT */

/*=====================================================================
  FUNCTION ExecuteUCICommit
======================================================================*/
/*!
@brief
  - Executes uci commit command

@return
  none

@note
  - Executes uci commit command using dsi_system_call

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
void ExecuteUCICommit()
{
  char cmd[QCMAP_MAX_COMMAND_LEN] = {0};

  snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "%s", UCI_COMMIT);
  ds_system_call(cmd, strlen(cmd));

  return;
} /* End ExecuteUCICommit */

/*=====================================================
  FUNCTION CompareKernelVer
======================================================*/
/*!
@brief
  Compares the kernel version with given kernel version.

@parameters
  char     *compare_kernel_ver

@return
  true  - if actual kernel version higher than or equal to
          given kernel version
  false - if actual kernel version less than given kernel version
  -1    - on failure

@note

- Dependencies
- None

- Side Effects
- None
*/
/*=====================================================*/
int QCMAP_LAN_Client::CompareKernelVer
(
  const char *compare_kernel_ver
)
{
  char *ver=NULL, *input_ver=NULL,*ptr1,*ptr2;
  char buff[KERNEL_VERSION_LENGTH];
  char Kernel_ver[KERNEL_VERSION_LENGTH];

  memset(buff, 0, KERNEL_VERSION_LENGTH);
  memset(Kernel_ver, 0, KERNEL_VERSION_LENGTH);

  if (QCMAP_LAN_Client::GetKernelVer(Kernel_ver) == false)
  {
    LOG_MSG_ERROR("Unable to get the kernel version info", 0, 0, 0);
    return QCMAP_CM_ERROR;
  }
  ver = strtok_r(Kernel_ver,".", &ptr1);
  snprintf(buff,KERNEL_VERSION_LENGTH,"%s",compare_kernel_ver);
  input_ver = strtok_r(buff,".", &ptr2);

  /* Here we are comparing the current kernel version
     with kernel version 4.9 */
  while ((ver != NULL) && (input_ver != NULL))
  {
    if (atoi(ver) > atoi(input_ver))
      return true;
    else if (atoi(ver) < atoi(input_ver))
      return false;

    ver = strtok_r(NULL,".",&ptr1);
    input_ver = strtok_r(NULL,".",&ptr2);
  }

  if (input_ver == NULL)
    return true;
  else
    return false;

return true;
}

/*=====================================================================
  FUNCTION isInterfaceUP
======================================================================*/
/*!
@brief
  - Get the If status

@return
  -IF_STATUS_UP
  -IF_STATUS_DOWN

@param[in]
  char* if_name

@note
  - Dependencies
  - None

  - Side Effects
  - None
*/
/*=========================================================================*/
int isInterfaceUP(char* if_name)
{
  char buf[MAX_COMMAND_STR_LEN] = {0};
  char command[MAX_COMMAND_STR_LEN]  = {0};
  char tmp[MAX_COMMAND_STR_LEN]={0};
  FILE *stream = NULL;


  snprintf( command, MAX_COMMAND_STR_LEN,NET_DEV_FILE_ROOT_PATH);
  snprintf(tmp, MAX_COMMAND_STR_LEN, "/%s/operstate", if_name);
  strlcat(command, tmp, MAX_COMMAND_STR_LEN);

  LOG_MSG_INFO1("file path %s", command,0,0);

  stream = fopen(command, "r");
  if(NULL == stream)
  {
    LOG_MSG_ERROR("Failed to open if file",0,0,0);
    return QCMAP_CM_ERROR;
  }

  fread(buf, sizeof(char), MAX_COMMAND_STR_LEN, stream);
  fclose(stream);

  if ((strncmp(buf, IF_STATE_UP, strlen(IF_STATE_UP)) == 0))
  {
    LOG_MSG_INFO1("if %s state is up, buf in %s state",if_name,buf,0);
    return IF_STATUS_UP;
  }
  else
  {
    LOG_MSG_INFO1("if %s state is down, buf in %s state",if_name,buf,0);
    return IF_STATUS_DOWN;
  }
}

/*=====================================================================
  FUNCTION ChangeIFState
======================================================================*/
/*!
@brief
  - Change If state

@return
  QCMAP_CM_SUCCESS - Success
  QCMAP_CM_ERROR - Failure

@param[in]
  char* if_name
  char* state

@note
  - Dependencies
  - None

  - Side Effects
  - None
*/
/*=========================================================================*/
void ChangeIFState(char* if_name, char* state)
{
  char command[MAX_COMMAND_STR_LEN];
  snprintf( command, MAX_COMMAND_STR_LEN,"ifconfig %s %s", if_name, state);
  ds_system_call( command, strlen(command));
}

/*===========================================================================
  FUNCTION SendHWFilteringInfoToIPA
==========================================================================*/
/*!
@brief
  Sends Hardware Filtering Information to IPA through ioctl.

@parameters
  qcmap_msgr_config_state_enum_v01 state
  qcmap_hw_mac_filter_config mac_filter_config

@return
  FALSE - on failure
  TRUE -  on success

@note
- Dependencies
- None

- Side Effects
- None
*/
/*=========================================================================*/

bool QCMAP_LAN_Client::SendHWFilteringInfoToIPA
(
  qcmap_hdw_filter_config              *hw_filter_config
)
{
  char command[MAX_COMMAND_STR_LEN]={0};
  char mac_addr_string[QCMAP_LAN_MAC_ADDR_NUM_CHARS];
  char result[QCMAP_MAX_SCAN_SIZE] = {0};
  char tmp_ipa_cmd[MAX_COMMAND_STR_LEN]={0};
  in_addr start_ip, end_ip;
  char segment_start_buff[QCMAP_MAX_SCAN_SIZE]={0}, segment_end_buff[QCMAP_MAX_SCAN_SIZE]={0};
  int count=0;
  uint num_of_mac=0, error_num;

  if (hw_filter_config->mac_flt_state == QCMAP_CONFIG_DISABLE)
  {
    snprintf( command, MAX_COMMAND_STR_LEN, "ipa swflt --mac-disable");
    ds_system_call(command, strlen(command));
    count++;
  }
  if (hw_filter_config->ip_segment_filter_state == QCMAP_CONFIG_DISABLE)
  {
    snprintf( command, MAX_COMMAND_STR_LEN, "ipa swflt --ip-disable");
    ds_system_call(command, strlen(command));
    count++;
  }
  if (hw_filter_config->iface_filter_state == QCMAP_CONFIG_DISABLE)
  {
    snprintf( command, MAX_COMMAND_STR_LEN, "ipa swflt --port-disable");
    ds_system_call(command, strlen(command));
    count++;
  }
  if(count == 3)
  {
    LOG_MSG_ERROR("Invalid Configuration in Enable State",0,0,0);
    return false;
  }

  num_of_mac = hw_filter_config->num_of_clients;
  snprintf(tmp_ipa_cmd, MAX_COMMAND_STR_LEN, "ipa swflt ");

  if (hw_filter_config->mac_flt_state == QCMAP_CONFIG_ENABLE)
  {
    memset(command, 0 , sizeof(command));
    snprintf(command, MAX_COMMAND_STR_LEN, "--mac-enable --add-mac ");
    strlcat(tmp_ipa_cmd, command, MAX_COMMAND_STR_LEN);
    for(unsigned int j = 0 ; j < num_of_mac ; j++)
    {
      memset(command, 0 , sizeof(command));
      ds_mac_addr_ntop(hw_filter_config->client_list[j].hw_filtering_mac_addr, mac_addr_string);
      snprintf(command, MAX_COMMAND_STR_LEN, "%s ", mac_addr_string);
      strlcat(tmp_ipa_cmd, command, MAX_COMMAND_STR_LEN);
    }
  }

  if (hw_filter_config->ip_segment_filter_state == QCMAP_CONFIG_ENABLE)
  {
    memset(command, 0 , sizeof(command));
    snprintf(command, MAX_COMMAND_STR_LEN, "--ip-enable --add-ip-seg ");
    strlcat(tmp_ipa_cmd, command, MAX_COMMAND_STR_LEN);
    for(int j=0; j<hw_filter_config->num_of_ip_segments; j++)
    {
      start_ip.s_addr = htonl(hw_filter_config->ip_segment_filter_list[j].ip_segment_start);
      end_ip.s_addr = htonl(hw_filter_config->ip_segment_filter_list[j].ip_segment_end);
      memcpy(segment_start_buff, inet_ntoa(start_ip), strlen(inet_ntoa(start_ip)));
      memcpy(segment_end_buff, inet_ntoa(end_ip), strlen(inet_ntoa(end_ip)));
      /* Null terminating the buffers */
      segment_start_buff[QCMAP_MAX_SCAN_SIZE-1]='\0';
      segment_end_buff[QCMAP_MAX_SCAN_SIZE-1]='\0';
      memset(command, 0 , sizeof(command));
      snprintf(command, MAX_COMMAND_STR_LEN, "%s %s ", segment_start_buff, segment_end_buff);
      strlcat(tmp_ipa_cmd, command, MAX_COMMAND_STR_LEN);
    }
  }

  if (hw_filter_config->iface_filter_state == QCMAP_CONFIG_ENABLE)
  {
    memset(command, 0 , sizeof(command));
    snprintf(command, MAX_COMMAND_STR_LEN, "--port-enable --add-port ");
    strlcat(tmp_ipa_cmd, command, MAX_COMMAND_STR_LEN);
    for(int j=0; j<hw_filter_config->num_of_iface; j++)
    {
      memset(command, 0 , sizeof(command));
      snprintf(command, MAX_COMMAND_STR_LEN, "%s ", hw_filter_config->if_name_filter_list[j].if_name);
      strlcat(tmp_ipa_cmd, command, MAX_COMMAND_STR_LEN);
    }
  }

  ds_system_call(tmp_ipa_cmd, strlen(tmp_ipa_cmd));

  return true;
}

/*===========================================================================
  FUNCTION GetSetHWFilteringStateFromUciConfig
==========================================================================*/
/*!
@brief
  action : GET - Gets HW Control Filtering Info from Uci Config.
  action : SET - Sets HW Control Filtering Info to Uci Config.

@parameters
  qcmap_action_type action
  qcmap_config_state *state
  qcmap_hdw_filter_config *hw_filter_config

@return
  FALSE - on failure
  TRUE -  on success

@note
- Dependencies
- None

- Side Effects
- None
*/
/*=========================================================================*/
bool QCMAP_LAN_Client::GetSetHWFilteringStateFromUciConfig
(
#ifndef FEATURE_QCMAP_OFFTARGET
  qcmap_action_type                   action,
#endif
  qcmap_config_state                  *state,
  qcmap_hdw_filter_config              *hw_filter_config
)
{
  bool ret = false;
  unsigned int num_of_mac = 0;
  char mac_addr_string[QCMAP_LAN_MAC_ADDR_NUM_CHARS];
  char client_tag[QCMAP_MAX_SCAN_SIZE] = {0}, ip_seg_start[QCMAP_MAX_SCAN_SIZE] = {0}, ip_seg_end[QCMAP_MAX_SCAN_SIZE] = {0}, iface_tag[QCMAP_MAX_SCAN_SIZE]={0};
  in_addr start_ip, end_ip, ap_ip_addr, subnet_ip, gateway_ip, netmask_ip;
  qcmap_lan_config lan_config;
  char result[QCMAP_MAX_SCAN_SIZE];
  char content_buf[QCMAP_MAX_SCAN_SIZE], segment_start_buff[QCMAP_MAX_SCAN_SIZE]={0}, segment_end_buff[QCMAP_MAX_SCAN_SIZE]={0};
  qmi_error_type_v01 qmi_err_num;


  if(!(state && hw_filter_config))
  {
    LOG_MSG_ERROR("Invalid parameter passed ",0,0,0);
    return ret;
  }

#ifndef FEATURE_QCMAP_OFFTARGET
  if(action == SET_VALUE)
#endif
  {

    if (!QCMAP_LAN_Client::UciSetUtility(owrt_filename[1], "firewall", 0,
        "hw_filtering_state",*state) )
    {
      return false;
    }

    if (hw_filter_config->num_of_clients < 0 ||
        hw_filter_config->num_of_clients > QCMAP_MAX_HW_MAC_FILTER_CLIENTS ||
        hw_filter_config->num_of_ip_segments < 0 ||
        hw_filter_config->num_of_ip_segments > QCMAP_MAX_IPV4_SEGMENT_FILTERING ||
        hw_filter_config->num_of_iface < 0 ||
        hw_filter_config->num_of_iface > std::min(QCMAP_MAX_IFACE_FILTERING,IPA_MAX_IFACE_FILTERING))
    {
      LOG_MSG_ERROR("Incorrect Configuration for num_of_macs %d, num_of_ip_segs %d, num_of_iface %d",
                     hw_filter_config->num_of_clients, hw_filter_config->num_of_ip_segments,
                     hw_filter_config->num_of_iface);
      return ret;
    }
    if (!QCMAP_LAN_Client::UciSetUtility(owrt_filename[1], "mac_filtering", 0,
        "mac_filter_state",hw_filter_config->mac_flt_state) )
    {
      return false;
    }

    if (!QCMAP_LAN_Client::UciSetUtility(owrt_filename[1], "mac_filtering", 0,
        "mac_filter_num_of_clients",hw_filter_config->num_of_clients) )
    {
      return false;
    }

    if (!QCMAP_LAN_Client::UciSetUtility(owrt_filename[1], "ip_segment_filtering", 0,
        "ip_segment_filter_state",hw_filter_config->ip_segment_filter_state) )
    {
      return false;
    }

    if (!QCMAP_LAN_Client::UciSetUtility(owrt_filename[1], "ip_segment_filtering", 0,
        "num_of_ip_segment",hw_filter_config->num_of_ip_segments) )
    {
      return false;
    }

    if (!QCMAP_LAN_Client::UciSetUtility(owrt_filename[1], "if_name_filtering", 0,
        "iface_filter_state",hw_filter_config->iface_filter_state) )
    {
      return false;
    }

    if (!QCMAP_LAN_Client::UciSetUtility(owrt_filename[1], "if_name_filtering", 0,
        "num_of_iface",hw_filter_config->num_of_iface) )
    {
      return false;
    }

    if (QCMAP_LAN_Client::GetLANConfig(&lan_config, &qmi_err_num))
    {
      gateway_ip.s_addr = htonl(lan_config.gw_ip);
      netmask_ip.s_addr = htonl(lan_config.netmask);
    }
    else
    {
      printf("error in getting gwip or netmask");
      return false;
    }

    num_of_mac = hw_filter_config->num_of_clients;

    if (hw_filter_config->mac_flt_state == QCMAP_CONFIG_ENABLE)
    {
      for(unsigned int j = 0 ; j < num_of_mac ; j++)
      {
        memset(client_tag, 0, sizeof(client_tag));
        snprintf(client_tag, QCMAP_MAX_SCAN_SIZE, "HWMACFilterClient%d", j+1);
        ds_mac_addr_ntop(hw_filter_config->client_list[j].hw_filtering_mac_addr, mac_addr_string);
        if (!QCMAP_LAN_Client::UciSetUtility(owrt_filename[1], "mac_filtering", 0,
            client_tag, mac_addr_string) )
        {
          return false;
        }
      }
    }

    if(hw_filter_config->ip_segment_filter_state == QCMAP_CONFIG_ENABLE)
    {
      for(int j=0; j<hw_filter_config->num_of_ip_segments; j++)
      {
        memset(ip_seg_start, 0, sizeof(ip_seg_start));
        memset(ip_seg_end, 0, sizeof(ip_seg_end));
        snprintf(ip_seg_start, QCMAP_MAX_SCAN_SIZE, "IPSegmentStart%d", j+1);
        snprintf(ip_seg_end, QCMAP_MAX_SCAN_SIZE, "IPSegmentEnd%d", j+1);
        start_ip.s_addr = htonl(hw_filter_config->ip_segment_filter_list[j].ip_segment_start);

        if((start_ip.s_addr & netmask_ip.s_addr) != (gateway_ip.s_addr & netmask_ip.s_addr))
        {
          printf("Segment %d Start IP configured by user is not in subnet\n",j+1);
          LOG_MSG_ERROR("Segment %d Start IP configured by user is not in subnet",j+1,0,0);
          return false;
        }
        end_ip.s_addr = htonl(hw_filter_config->ip_segment_filter_list[j].ip_segment_end);
        if((end_ip.s_addr & netmask_ip.s_addr) != (gateway_ip.s_addr & netmask_ip.s_addr))
        {
          LOG_MSG_ERROR("Segment %d End IP configured by user is not in subnet",j+1,0,0);
          return false;
        }
        memcpy(segment_start_buff, inet_ntoa(start_ip), strlen(inet_ntoa(start_ip)));
        memcpy(segment_end_buff, inet_ntoa(end_ip), strlen(inet_ntoa(end_ip)));
        /* Null terminating the buffers */
        segment_start_buff[QCMAP_MAX_SCAN_SIZE-1]='\0';
        segment_end_buff[QCMAP_MAX_SCAN_SIZE-1]='\0';

        if (!QCMAP_LAN_Client::UciSetUtility(owrt_filename[1], "ip_segment_filtering", 0,
            ip_seg_start ,segment_start_buff) )
        {
          return false;
        }

        if (!QCMAP_LAN_Client::UciSetUtility(owrt_filename[1], "ip_segment_filtering", 0,
             ip_seg_end ,segment_end_buff) )
        {
          return false;
        }

      }
    }

    if(hw_filter_config->iface_filter_state == QCMAP_CONFIG_ENABLE)
    {
      for(int j=0; j<hw_filter_config->num_of_iface; j++)
      {
        memset(iface_tag,0,sizeof(iface_tag));
        snprintf(iface_tag, QCMAP_MAX_SCAN_SIZE, "InterfaceName%d", j+1);
        if (!QCMAP_LAN_Client::UciSetUtility(owrt_filename[1], "if_name_filtering", 0,
            iface_tag,hw_filter_config->if_name_filter_list[j].if_name) )
        {
          return false;
        }
      }
    }
    return true;
  }
#ifndef FEATURE_QCMAP_OFFTARGET
  else if(action == GET_VALUE)
#endif
  {
    //memset(command,0,QCMAP_MAX_COMMAND_LEN);
    memset(result,0,QCMAP_MAX_SCAN_SIZE);

    if ( QCMAP_LAN_Client::UciGetUtility(owrt_filename[1], "firewall", 0,
         "hw_filtering_state", result) )
    {
      *state = (qcmap_config_state)atoi(result);
    }

    memset(result,0,QCMAP_MAX_SCAN_SIZE);

    if (QCMAP_LAN_Client::UciGetUtility(owrt_filename[1], "mac_filtering", 0,
        "mac_filter_state", result) )
    {
      hw_filter_config->mac_flt_state = (qcmap_config_state)atoi(result);
    }

    memset(result,0,QCMAP_MAX_SCAN_SIZE);

    if (QCMAP_LAN_Client::UciGetUtility(owrt_filename[1], "mac_filtering", 0,
        "mac_filter_num_of_clients",result) )
    {
      hw_filter_config->num_of_clients = atoi(result);
    }

    memset(result,0,QCMAP_MAX_SCAN_SIZE);

    if (QCMAP_LAN_Client::UciGetUtility(owrt_filename[1], "ip_segment_filtering", 0,
        "ip_segment_filter_state",result) )
    {
      hw_filter_config->ip_segment_filter_state = (qcmap_config_state)atoi(result);
    }

    memset(result,0,QCMAP_MAX_SCAN_SIZE);

    if (QCMAP_LAN_Client::UciGetUtility(owrt_filename[1], "ip_segment_filtering", 0,
        "num_of_ip_segment",result) )
    {
      hw_filter_config->num_of_ip_segments =atoi(result);
    }

    memset(result,0,QCMAP_MAX_SCAN_SIZE);

    if (QCMAP_LAN_Client::UciGetUtility(owrt_filename[1], "if_name_filtering", 0,
        "iface_filter_state",result) )
    {
      hw_filter_config->iface_filter_state = (qcmap_config_state)atoi(result);
    }

    memset(result,0,QCMAP_MAX_SCAN_SIZE);

    if (QCMAP_LAN_Client::UciGetUtility(owrt_filename[1], "if_name_filtering", 0,
        "num_of_iface",result) )
    {
      hw_filter_config->num_of_iface = atoi(result);
    }


    for(int i = 0 ; i < hw_filter_config->num_of_clients ; i++)
    {
      memset(result,0,QCMAP_MAX_SCAN_SIZE);
      memset(content_buf, 0 , QCMAP_MAX_SCAN_SIZE);
      snprintf(client_tag, QCMAP_MAX_SCAN_SIZE, "HWMACFilterClient%d", i+1);
      if (QCMAP_LAN_Client::UciGetUtility(owrt_filename[1], "mac_filtering", 0,
           client_tag, result) )
      {
        strlcpy(content_buf, result, strlen(result)+1);
      }
      for (int j = 0; j < QCMAP_MAC_ADDR_LEN; j++)
      {
        hw_filter_config->client_list[i].hw_filtering_mac_addr[j] =
         (ds_hex_to_dec(content_buf[j * 3]) << 4) |ds_hex_to_dec(content_buf[j * 3 + 1]);
      }
    }

    for(int i=0; i<hw_filter_config->num_of_ip_segments; i++)
    {
      snprintf(ip_seg_start, QCMAP_MAX_SCAN_SIZE, "IPSegmentStart%d", i+1);
      snprintf(ip_seg_end, QCMAP_MAX_SCAN_SIZE, "IPSegmentEnd%d", i+1);
      memset(result,0,QCMAP_MAX_SCAN_SIZE);
      memset(content_buf, 0 , QCMAP_MAX_SCAN_SIZE);
      if (!QCMAP_LAN_Client::UciGetUtility(owrt_filename[1], "ip_segment_filtering", 0,
          ip_seg_start,result) )
      {
        return false;
      }
      strlcpy(content_buf, result, strlen(result)+1);
      if(inet_aton(content_buf, &start_ip))
      {
        hw_filter_config->ip_segment_filter_list[i].ip_segment_start = ntohl(start_ip.s_addr);
      }
      else
      {
        LOG_MSG_ERROR("Error in Reading Start IP Addr of IP Segment %d from uci", i+1,0,0);
        return false;
      }

      memset(result, 0, QCMAP_MAX_SCAN_SIZE);
      memset(content_buf, 0 , QCMAP_MAX_SCAN_SIZE);

      if (!QCMAP_LAN_Client::UciGetUtility(owrt_filename[1], "ip_segment_filtering", 0,
          ip_seg_end,result) )
      {
        return false;
      }
      strlcpy(content_buf, result, strlen(result)+1);
      if(inet_aton(content_buf, &end_ip))
      {
        hw_filter_config->ip_segment_filter_list[i].ip_segment_end = ntohl(end_ip.s_addr);
      }
      else
      {
        LOG_MSG_ERROR("Error in Reading End IP Addr of IP Segment %d from xml", i+1,0,0);
        return false;
      }
    }

    for(int i=0; i<hw_filter_config->num_of_iface; i++)
    {
      memset(iface_tag,0,sizeof(iface_tag));
      snprintf(iface_tag, QCMAP_MAX_SCAN_SIZE, "InterfaceName%d", i+1);
      hw_filter_config->if_name_filter_list[i].if_name_len = QCMAP_MAX_IFACE_NAME_SIZE;
      memset(result, 0, QCMAP_MAX_SCAN_SIZE);
      if (!QCMAP_LAN_Client::UciGetUtility(owrt_filename[1], "if_name_filtering", 0,
          iface_tag,result) )
      {
        return false;
      }
      strlcpy(hw_filter_config->if_name_filter_list[i].if_name, result, QCMAP_MAX_IFACE_NAME_SIZE_V01);
    }
   return true;
  }
  return ret;
}
/*===========================================================================
FUNCTION dns_list_to_string
===========================================================================*/
/*!
  @brief
  Converts array of dns name strings to one string,
  where each name is seprated by a ' ' character

  @parameters
    char                           *dns_string
    uint32_t                        buf_size
    dns_seach_list_info            *dns_list
    uint32_t                        list_len

  @return
  None

  @note
  - Dependencies
  - None

  - Side Effects
  - None
  */
/*=========================================================================*/
 void QCMAP_LAN_Client::dns_list_to_string
(
  char                           *dns_string,
  uint32_t                        buf_size,
  dns_seach_list_info            *dns_list,
  uint32_t                        list_len
)
{
  if(dns_list!=NULL)
  {
    dns_string[0]='\0';
    for(int i=0; i<list_len; i++)
    {
        strlcat(dns_string, dns_list[i].dns_search_name, buf_size);
        if(i != list_len-1)
        {
            strlcat(dns_string, " ", buf_size);
        }
    }
  }
} /* End DNSListToString */

/*==========================================================================
FUNCTION util_get_eth_nic_number()
===========================================================================*/
/** @ingroup qcmap_get_eth_nic_config

  Get eth nic number from eth_iface_name.

  @param[in]        char*

  @return
  1 -- Nic 1 \n
  2 -- Nic 2 \n
*/
/*=========================================================================*/

int QCMAP_LAN_Client::util_get_eth_nic_number
(
  char *eth_iface_name
)
{
  qcmap_interface_type_enum eth_nic_type;
  int nic_number = QCMAP_LAN_INVALID; 

  eth_nic_type = GetEthNiCTypeFromName(eth_iface_name);
  if(eth_nic_type == QCMAP_INTERFACE_TYPE_ETH)
  {
    nic_number = 1;
  }
  else if(eth_nic_type == QCMAP_INTERFACE_TYPE_ETH_NIC2)
  {
    nic_number = 2;
  }
  return nic_number;
}

/*=====================================================================
  FUNCTION CheckEnableIPPT
======================================================================*/
/*!
@brief
  - Checks if ippt is enabled on the Profile

@return
  - enable_state

@param[in]
  - profile handle

@note
  - Dependencies
  - None

  - Side Effects
  - None
*/
/*=========================================================================*/
int CheckEnableIPPT(const uint32_t profile_handle)
{
  char cmd[QCMAP_MAX_COMMAND_LEN] = {0};
  char result[QCMAP_MAX_SCAN_SIZE] = {0};
  int profile_idx = -1, enable_state = -1;

  /* Get profile_idx from qcmap_lan */
  profile_idx = GetProfileIndex(profile_handle);
  if (profile_idx == QCMAP_LAN_INVALID)
  {
    /* indicates profile_idx not found */
    LOG_MSG_ERROR(" Profile index for profile: %d not found in qcmap_lan. Exiting!",
                  profile_handle, 0, 0);
    return QCMAP_LAN_INVALID;
  }

  /* Get ippt_enable state */
  snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "%s qcmap_lan.@profile[%d].ippt_enable",
           UCI_GET_COMMAND, profile_idx);
  if (!ExecuteSystemCmd((const char *)cmd, result,
                       sizeof(result)))
  {
    /* Indicates ippt_enable is not present */
    LOG_MSG_ERROR("IPPT enable state for PDN: %d not found!", profile_handle, 0, 0);
    return QCMAP_LAN_INVALID;
  }
  enable_state = atoi(result);

  return enable_state;
} /* End CheckEnableIPPT() */


/*=====================================================================
  FUNCTION GetIPPTPDNCountfromUCI
======================================================================*/
/*!
@brief
  - Gets the IPPT PDN count information from qcmap_lan db

@return
  - int IPPT PDN count

@param[in]
  - None

@note
  - Dependencies
  - None

  - Side Effects
  - None
*/
/*=========================================================================*/
int GetIPPTPDNCountfromUCI()
{
  char cmd[QCMAP_MAX_COMMAND_LEN] = {0};
  char result[QCMAP_MAX_SCAN_SIZE] = {0};
  int ippt_pdn_count;

  snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "%s %s",
             UCI_GET_COMMAND, QCMAP_GET_IPPT_PDN_COUNT);
  if (!ExecuteSystemCmd((const char *)cmd, result,
                      sizeof(result)))
  {
    LOG_MSG_ERROR("Failed to get default_pdn value from qcmap_lan database", 0, 0, 0);
    return QCMAP_LAN_INVALID;
  }

  ippt_pdn_count = atoi(result);
  if (ippt_pdn_count >= NUMERIC_ZERO)
    return ippt_pdn_count;
  else
    return QCMAP_LAN_INVALID;
} /* End GetIPPTPDNCountfromUCI() */

/*===========================================================================
 FUNCTION readable_addr
  ===========================================================================
  @brief
  converts the a numeric address into a text string suitable
  for presentation
  @input
  domain - identifies ipv4 or ipv6 domain
  addr   - contains the numeric address
  str    - this is an ouput value contains address in text string
  @return
  0  - success
  -1 - failure
  @dependencies
  It depends on inet_ntop()
  @sideefects
  None
  =========================================================================*/

int readable_addr(int domain, const uint32 *addr, char *str)
{
  if((addr!=NULL) && (str!=NULL))
  {
    if (inet_ntop(domain, (const char *)addr, str, INET6_ADDRSTRLEN) == NULL)
    {
      printf("\n Not in presentation format \n");
      return -1;
    }
  }
  return 0;
}


/*=====================================================================
  FUNCTION GetVlanIndex
======================================================================*/
/*!
@brief
  - Get VLAN index of passed vlan_id in qcmap_lan database

@return
  vlan_id

@note
  - returns the VLAN index of passed vlan_id in qcmap_lan database

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
int GetVlanIndex
(
  const uint32_t vlan_id
)
{
  char cmd[QCMAP_MAX_COMMAND_LEN] = {0};
  char result[QCMAP_MAX_SCAN_SIZE] = {0};
  int no_of_vlans = -1, vlan_idx = -1;

/* Get the profile index in qcmap_lan */
  snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "%s %s",
           UCI_GET_COMMAND, UCI_QUERY_NO_OF_VLANS);
  if (ExecuteSystemCmd((const char *)cmd, result,
                      sizeof(result)))
  {
    no_of_vlans = atoi(result);
    /* iterate through the profile and find out index of wan<Prof_ID> */
    for (int i = 0; i < no_of_vlans; i++)
    {
      /* get the profile_id */
      memset(cmd, 0, QCMAP_MAX_COMMAND_LEN);
      memset(result, 0, QCMAP_MAX_SCAN_SIZE);
      snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "%s qcmap_lan.@vlan[%d].vlan_id",
               UCI_GET_COMMAND, i);
      if (ExecuteSystemCmd((const char *)cmd, result,
                      sizeof(result)))
      {
        /* Check if retrieved profile_id is same as profile_handle */
        if (vlan_id == atoi(result))
        {
          vlan_idx = i;
          break;
        }
      }
    }
  }
  return vlan_idx;
} /* End GetVlanIndex */


/*=====================================================================
  FUNCTION ExecuteSystemCmd
======================================================================*/
/*!
@brief
  - executes the system cmd and returns the output of the command

@return
  true - Success
  false - Failure

@note
  - fgets() is used to read the output of the command so the returned output
    will be either of size (result_len -1) characters, or till the newline char
    or end-of-file character whichever comes first

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
boolean ExecuteSystemCmd
(
  const char *cmd,
  char *result,
  uint8_t result_len,
  int *error_num = 0
)
{
  if ((NULL == cmd) || (NULL == result) || (0 == result_len))
  {
    LOG_MSG_ERROR("Invalid arguments passed", 0, 0, 0);
    if (error_num != NULL)
    {
      *error_num = 0;
    }
    return false;
  }

  FILE * fp = popen(cmd, "r");
  if (fp)
  {
    if (fgets(result, result_len, fp) != NULL)
    {
      pclose(fp);
      LOG_MSG_INFO1("qcmap lan system call: %s :successfull.", cmd, 0, 0);
      LOG_MSG_INFO1("result: %s.", result, 0, 0);
      return true;
    }
    else
    {
      pclose(fp);
      LOG_MSG_ERROR("Fail to read output:%s", cmd, 0, 0);
      if (error_num != NULL)
      {
        *error_num = errno;
      }
      return false;
    }
  }
  else
  {
    LOG_MSG_ERROR("Error in executing %s", cmd, 0, 0);
    if (error_num != NULL)
    {
      *error_num = errno;
    }
    return false;
  }
  return false;
} /*End ExecuteSystemCmd() */

/*===========================================================================
FUNCTION  GetWLANEXIfaceIndex
==========================================================================*/
/*!
@brief
This function is get the WLAN Iface Index given the ap_type and iface_type

@parameters
  qcmap_msgr_activate_hostapd_ap_enum_v01 ap_type

@return
qcmap_msgr_wlan_iface_index_enum_v01

@note

- Dependencies
-None

- Side Effects
- None
*/
/*=========================================================================*/
qcmap_msgr_wlan_iface_index_enum_v01 GetWLANEXIfaceIndex
(
  uint16 ap_type
)
{
    switch(ap_type)
    {
        case QCMAP_MSGR_PRIMARY_AP_V01:
            return  QCMAP_MSGR_WLAN_IFACE_PRIMARY_AP_V01;
        case QCMAP_MSGR_GUEST_AP_V01:
            return  QCMAP_MSGR_WLAN_IFACE_GUEST_AP_ONE_V01;
        case QCMAP_MSGR_GUEST_AP_2_V01:
            return  QCMAP_MSGR_WLAN_IFACE_GUEST_AP_TWO_V01;
        case QCMAP_MSGR_GUEST_AP_3_V01:
            return  QCMAP_MSGR_WLAN_IFACE_GUEST_AP_THREE_V01;
        case QCMAP_MSGR_GUEST_AP_4_V01:
            return  QCMAP_MSGR_WLAN_IFACE_GUEST_AP_FOUR_V01;
        case QCMAP_MSGR_GUEST_AP_5_V01:
            return  QCMAP_MSGR_WLAN_IFACE_GUEST_AP_FIVE_V01;
        case QCMAP_MSGR_GUEST_AP_6_V01:
            return  QCMAP_MSGR_WLAN_IFACE_GUEST_AP_SIX_V01;
        case QCMAP_MSGR_GUEST_AP_7_V01:
            return  QCMAP_MSGR_WLAN_IFACE_GUEST_AP_SEVEN_V01;
        case QCMAP_MSGR_GUEST_AP_8_V01:
            return  QCMAP_MSGR_WLAN_IFACE_GUEST_AP_EIGHT_V01;
        case QCMAP_MSGR_GUEST_AP_9_V01:
            return  QCMAP_MSGR_WLAN_IFACE_GUEST_AP_NINE_V01;
        case QCMAP_MSGR_GUEST_AP_10_V01:
            return  QCMAP_MSGR_WLAN_IFACE_GUEST_AP_TEN_V01;
        case QCMAP_MSGR_GUEST_AP_11_V01:
            return  QCMAP_MSGR_WLAN_IFACE_GUEST_AP_ELEVEN_V01;
        case QCMAP_MSGR_GUEST_AP_12_V01:
            return  QCMAP_MSGR_WLAN_IFACE_GUEST_AP_TWELVE_V01;
        case QCMAP_MSGR_GUEST_AP_13_V01:
            return  QCMAP_MSGR_WLAN_IFACE_GUEST_AP_THIRTEEN_V01;
        default:
            return QCMAP_MSGR_WLAN_IFACE_INDEX_ENUM_MIN_ENUM_VAL_V01;
    }
}

/*=====================================================================
  FUNCTION GetCurrentBackhaul
======================================================================*/
/*!
@brief
  - Gets the bh_presentv4/v6 information from qcmap_lan db

@return
  - true - Success
    false - Failure

@note
  - Dependencies
  - None

  - Side Effects
  - None
*/
/*=========================================================================*/
boolean GetCurrentBackhaul
(
  char *bh_present_v4,
  char *bh_present_v6,
  int profile_idx
)
{
  char cmd[QCMAP_MAX_COMMAND_LEN] = {0};
  char result[QCMAP_MAX_SCAN_SIZE] = {0};

  if (NULL == bh_present_v4 || NULL == bh_present_v6 || profile_idx == QCMAP_LAN_INVALID)
  {
    LOG_MSG_ERROR("NULL parameters received", 0, 0, 0);
    return false;
  }

  /* Get bh_present parameter */
  snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "%s qcmap_lan.@profile[%d].bh_present",
           UCI_GET_COMMAND, profile_idx);
  if (ExecuteSystemCmd((const char *)cmd, result,
                      sizeof(result)))
  {
    result[strlen(result)-1] = '\0';
    strlcpy(bh_present_v4, result, QCMAP_MAX_IFACE_NAME_SIZE);
  }
  
  /* Get bh_present_v6 parameter */
  snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "%s qcmap_lan.@profile[%d].bh_present_v6",
           UCI_GET_COMMAND, profile_idx);
  if (ExecuteSystemCmd((const char *)cmd, result,
                      sizeof(result)))
  {
    result[strlen(result)-1] = '\0';
    strlcpy(bh_present_v6, result, QCMAP_MAX_IFACE_NAME_SIZE);
  }

  return true;
} /* End: GetCurrentBackhaul() */

/*=====================================================================
  FUNCTION inet_net_pton_ipv4
======================================================================*/
 /*!
@brief
  - convert IPv4 network number from presentation to network format.
      accepts hex octets, hex strings, decimal octets, and /CIDR.
      "size" is in bytes and describes "dst"

@return
  - number of bits, either imputed classfully or specified with /CIDR,
       or -1 if some failure occurred (check errno)

@note
  - Dependencies
  - None

  - Side Effects
  - None

 */
/*=========================================================================*/
static int
inet_net_pton_ipv4
(
  const char *src,
  unsigned char *dst,
  size_t size
)
{
  static const char xdigits[] = "0123456789abcdef";
  static const char digits[] = "0123456789";
  int n, ch, tmp = 0, dirty, bits;
  const unsigned char *odst = dst;

  ch = *src++;
  if (ch == '0' && (src[0] == 'x' || src[0] == 'X')
      && isascii(src[1])
      && isxdigit(src[1])) {
    /* Hexadecimal: Eat nybble string. */
    if (!size)
      goto emsgsize;
    dirty = 0;
    src++;  /* skip x or X. */
    while ((ch = *src++) != '\0' && isascii(ch) && isxdigit(ch)) {
      if (isupper(ch))
        ch = tolower(ch);
      n = (int)(strchr(xdigits, ch) - xdigits);
      if (dirty == 0)
        tmp = n;
      else
        tmp = (tmp << 4) | n;
      if (++dirty == 2) {
        if (!size--)
          goto emsgsize;
        *dst++ = (unsigned char) tmp;
        dirty = 0;
      }
    }
    if (dirty) {  /* Odd trailing nybble? */
      if (!size--)
        goto emsgsize;
      *dst++ = (unsigned char) (tmp << 4);
    }
  } else if (isascii(ch) && isdigit(ch)) {
    /* Decimal: eat dotted digit string. */
    for (;;) {
      tmp = 0;
      do {
        n = (int)(strchr(digits, ch) - digits);
        tmp *= 10;
        tmp += n;
        if (tmp > 255)
          goto enoent;
      } while ((ch = *src++) != '\0' &&
               isascii(ch) && isdigit(ch));
      if (!size--)
        goto emsgsize;
      *dst++ = (unsigned char) tmp;
      if (ch == '\0' || ch == '/')
        break;
      if (ch != '.')
        goto enoent;
      ch = *src++;
      if (!isascii(ch) || !isdigit(ch))
        goto enoent;
    }
  } else
    goto enoent;

  bits = -1;
  if (ch == '/' && isascii(src[0]) &&
      isdigit(src[0]) && dst > odst) {
    /* CIDR width specifier.  Nothing can follow it. */
    ch = *src++;    /* Skip over the /. */
    bits = 0;
    do {
      n = (int)(strchr(digits, ch) - digits);
      bits *= 10;
      bits += n;
      if (bits > 32)
        goto enoent;
    } while ((ch = *src++) != '\0' && isascii(ch) && isdigit(ch));
    if (ch != '\0')
      goto enoent;
  }

  /* Firey death and destruction unless we prefetched EOS. */
  if (ch != '\0')
    goto enoent;

  /* If nothing was written to the destination, we found no address. */
  if (dst == odst)
    goto enoent;  /* LCOV_EXCL_LINE: all valid paths above increment dst */
  /* If no CIDR spec was given, infer width from net class. */
  if (bits == -1) {
    if (*odst >= 240)       /* Class E */
      bits = 32;
    else if (*odst >= 224)  /* Class D */
      bits = 8;
    else if (*odst >= 192)  /* Class C */
      bits = 24;
    else if (*odst >= 128)  /* Class B */
      bits = 16;
    else                    /* Class A */
      bits = 8;
    /* If imputed mask is narrower than specified octets, widen. */
    if (bits < ((dst - odst) * 8))
      bits = (int)(dst - odst) * 8;
    /*
     * If there are no additional bits specified for a class D
     * address adjust bits to 4.
     */
    if (bits == 8 && *odst == 224)
      bits = 4;
  }
  /* Extend network to cover the actual mask. */
  while (bits > ((dst - odst) * 8)) {
    if (!size--)
      goto emsgsize;
    *dst++ = '\0';
  }
  return (bits);

  enoent:
  return (-1);

  emsgsize:
  return (-1);
}/* End: inet_net_pton_ipv4() */

static int
getbits
(
  const char *src,
  int *bitsp
)
{
  static const char digits[] = "0123456789";
  int n;
  int val;
  char ch;

  val = 0;
  n = 0;
  while ((ch = *src++) != '\0') {
    const char *pch;

    pch = strchr(digits, ch);
    if (pch != NULL) {
      if (n++ != 0 && val == 0)       /* no leading zeros */
        return (0);
      val *= 10;
      val += (int)(pch - digits);
      if (val > 128)                  /* range */
        return (0);
      continue;
    }
    return (0);
  }
  if (n == 0)
    return (0);
  *bitsp = val;
  return (1);
}

static int
getv4
(
  const char *src,
  unsigned char *dst,
  int *bitsp
)
{
  static const char digits[] = "0123456789";
  unsigned char *odst = dst;
  int n;
  unsigned int val;
  char ch;

  val = 0;
  n = 0;
  while ((ch = *src++) != '\0') {
    const char *pch;

    pch = strchr(digits, ch);
    if (pch != NULL) {
      if (n++ != 0 && val == 0)       /* no leading zeros */
        return (0);
      val *= 10;
      val += (int)(pch - digits);
      if (val > 255)                  /* range */
        return (0);
      continue;
    }
    if (ch == '.' || ch == '/') {
      if (dst - odst > 3)             /* too many octets? */
        return (0);
      *dst++ = (unsigned char)val;
      if (ch == '/')
        return (getbits(src, bitsp));
      val = 0;
      n = 0;
      continue;
    }
    return (0);
  }
  if (n == 0)
    return (0);
  if (dst - odst > 3)             /* too many octets? */
    return (0);
  *dst = (unsigned char)val;
  return 1;
}

/*=====================================================================
  FUNCTION inet_net_pton_ipv6
======================================================================*/
 /*!
@brief
  - convert IPv6 network number from presentation to network format.
      accepts hex octets, hex strings, decimal octets, and /CIDR.
      "size" is in bytes and describes "dst"

@return
  - number of bits, either imputed classfully or specified with /CIDR,
       or -1 if some failure occurred (check errno)

@note
  - Dependencies
  - None

  - Side Effects
  - None

 */
/*=========================================================================*/
static int
inet_net_pton_ipv6
(
  const char *src,
  unsigned char *dst,
  size_t size
)
{
  static const char xdigits_l[] = "0123456789abcdef",
    xdigits_u[] = "0123456789ABCDEF";
  unsigned char tmp[NS_IN6ADDRSZ], *tp, *endp, *colonp;
  const char *xdigits, *curtok;
  int ch, saw_xdigit;
  unsigned int val;
  int digits;
  int bits;
  size_t bytes;
  int words;
  int ipv4;

  memset((tp = tmp), '\0', NS_IN6ADDRSZ);
  endp = tp + NS_IN6ADDRSZ;
  colonp = NULL;
  /* Leading :: requires some special handling. */
  if (*src == ':')
    if (*++src != ':')
      goto enoent;
  curtok = src;
  saw_xdigit = 0;
  val = 0;
  digits = 0;
  bits = -1;
  ipv4 = 0;
  while ((ch = *src++) != '\0') {
    const char *pch;

    if ((pch = strchr((xdigits = xdigits_l), ch)) == NULL)
      pch = strchr((xdigits = xdigits_u), ch);
    if (pch != NULL) {
      val <<= 4;
      val |= (int)(pch - xdigits);
      if (++digits > 4)
        goto enoent;
      saw_xdigit = 1;
      continue;
    }
    if (ch == ':') {
      curtok = src;
      if (!saw_xdigit) {
        if (colonp)
          goto enoent;
        colonp = tp;
        continue;
      } else if (*src == '\0')
        goto enoent;
      if (tp + NS_INT16SZ > endp)
        return (0);
      *tp++ = (unsigned char)((val >> 8) & 0xff);
      *tp++ = (unsigned char)(val & 0xff);
      saw_xdigit = 0;
      digits = 0;
      val = 0;
      continue;
    }
    if (ch == '.' && ((tp + NS_INADDRSZ) <= endp) &&
        getv4(curtok, tp, &bits) > 0) {
      tp += NS_INADDRSZ;
      saw_xdigit = 0;
      ipv4 = 1;
      break;  /* '\0' was seen by inet_pton4(). */
    }
    if (ch == '/' && getbits(src, &bits) > 0)
      break;
    goto enoent;
  }
  if (saw_xdigit) {
    if (tp + NS_INT16SZ > endp)
      goto enoent;
    *tp++ = (unsigned char)((val >> 8) & 0xff);
    *tp++ = (unsigned char)(val & 0xff);
  }
  if (bits == -1)
    bits = 128;

  words = (bits + 15) / 16;
  if (words < 2)
    words = 2;
  if (ipv4)
    words = 8;
  endp =  tmp + 2 * words;

  if (colonp != NULL) {
    /*
     * Since some memmove()'s erroneously fail to handle
     * overlapping regions, we'll do the shift by hand.
     */
    const int n = tp - colonp;
    int i;

    if (tp == endp)
      goto enoent;
    for (i = 1; i <= n; i++) {
      *(endp - i) = *(colonp + n - i);
      *(colonp + n - i) = 0;
    }
    tp = endp;
  }
  if (tp != endp)
    goto enoent;

  bytes = (bits + 7) / 8;
  if (bytes > size)
    goto emsgsize;
  memcpy(dst, tmp, bytes);
  return (bits);

  enoent:
  return (-1);

  emsgsize:
  return (-1);
}/* End: inet_net_pton_ipv6() */

/*=====================================================================
  FUNCTION ares_inet_net_pton
======================================================================*/
 /*!
@brief
 - convert network number from presentation to network format.
      accepts hex octets, hex strings, decimal octets, and /CIDR.
      "size" is in bytes and describes "dst"


@return
 - number of bits, either imputed classfully or specified with /CIDR,
       or -1 if some failure occurred (check errno)

@note
  - Dependencies
  - None

  - Side Effects
  - None

 */
/*=========================================================================*/
int
ares_inet_net_pton
(
  int af,
  const char *src,
  void *dst,
  size_t size
)
{
  switch (af)
  {
    case AF_INET:
      return (inet_net_pton_ipv4(src, dst, size));
    case AF_INET6:
      return (inet_net_pton_ipv6(src, dst, size));
    default:
      return 0;
  }
}/* End: ares_inet_net_pton() */

/*=====================================================================
  FUNCTION GetMappedPDNforVlan
======================================================================*/
/*!
@brief
  - Get mapped PDN for a given VLAN index

@return
  profile_handle/PDN

@note
  - returns the mapped PDN to passed vlan_id in qcmap_lan database

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
int GetMappedPDNforVlan
(
  const uint32_t vlan_idx,
  qmi_error_type_v01 *qmi_err_num
)
{
  char cmd[QCMAP_MAX_COMMAND_LEN] = {0};
  char result[QCMAP_MAX_SCAN_SIZE] = {0};
  int profile_handle = QCMAP_LAN_INVALID;
  int error_num;

  /* Get mappped PDN based on vlan index from qcmap_lan */
  if (QCMAP_LAN_Client::UciGetUtility(owrt_filename[0], "vlan", 0,
       "wwan_profile_num",result, vlan_idx, &error_num) )
  {
    profile_handle=atoi(result);
  }
  else if (error_num == EINVAL)
  {
    LOG_MSG_ERROR("Vlan is not mapped to PDN",0,0,0);
    *qmi_err_num = QMI_ERR_NONE_V01;
  }
  else
  {
    LOG_MSG_ERROR("Failed to get mapped pdn for a vlan",0,0,0);
    *qmi_err_num = QMI_ERR_INTERNAL_V01;
  }
  return profile_handle;
} /* End GetVlanIndex */
