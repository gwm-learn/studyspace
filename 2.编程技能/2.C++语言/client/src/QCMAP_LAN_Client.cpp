/*====================================================

FILE:  QCMAP_LAN_Client.cpp

SERVICES:
QCMAP LAN Client Implementation

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
  02/14/22   ak         Support for HW Filtering
  02/13/22   ak         Support for Parental Control
  02/03/22   ak         conntrack deleteion support for firewall
  12/28/22   dk         Added support for Delete WWAN policy
  12/12/22   dk         Added WLAN API Support in Openwrt
  11/28/22   ak         Added DHCP Reservation Support in Openwrt
  11/24/22   dk         Added ETH backhaul support
  10/05/22   sp         Introduce IP Passthrough for OpenWRT
  09/30/22   ak         Added Firewall Support in OpenWRT
  08/18/22   dk         Added SNAT support for openWRT
  07/26/22   sp         Enable MPDN-VLAN support for OpenWRT
  07/13/22   ak         Enable NAT for OpenWRT
  07/05/22   dk         Added VLAN support on openWRT
  06/08/22   sp         Created LAN client library
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
#include "QCMAP_WLAN_Common.h"
#include "QCMAP_WLAN_HMT.h"
#include "QCMAP_WLAN_WKK.h"

#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#define QCMAP_MAX_COMMAND_LEN                600   /*  Max Command length */

const char* wlan_mode_str[] = {
   "Invalid" ,
   "AP"      ,
   "AP-AP"   ,
   "AP-STA"  ,
   "AP-AP-AP",
   "AP-AP-STA",
   "STA",
   "AP-AP-AP-AP",
   "AP-STA-BRIDGE",
   "AP-AP-STA-BRIDGE",
   "STA-ONLY-BRIDGE",
   "Invalid", /*0xB is not supported*/
   "Invalid", /*0xC is not supported*/
   "7-AP"
};

/*===================================================================
  Class Definitions
  ===================================================================*/

/*===========================================================================
  FUNCTION QCMAP_LAN_Client
  ===========================================================================
  @brief
  Initializes the LAN Client.
  @input
  void
  @return
  void
  @dependencies
  @sideefects
  None
  =========================================================================*/
QCMAP_LAN_Client::QCMAP_LAN_Client()
{
  QCMAP_LOG_FUNC_ENTRY();
  IsWlanEnabled = false;

  Init();
  return;
}  /* end QCMAP_LAN_Client() */

/*===========================================================================
  FUNCTION Init
  ===========================================================================*/
/*!
  @brief
  Initialization function

  @return
  void

  @note

  Dependencies
  None

  Side Effects
  None
 */
/*=========================================================================*/
void QCMAP_LAN_Client::Init()
{
#ifdef FEATURE_DATA_LOG_QXDM
  /* Initializing Diag for QXDM logs*/
  if (TRUE != Diag_LSM_Init(NULL))
  {
     printf("Diag_LSM_Init failed !!");
  }
#endif

  qcmap_cdt_enum cdtValue = utilGetCDTValue();
  if(cdtValue == QCMAP_CDT_WKK)
    m_pQCMapWlanObj = new QCMAP_WLAN_WKK();
  else if (cdtValue == QCMAP_CDT_HMT)
    m_pQCMapWlanObj = new QCMAP_WLAN_HMT();
  else
    LOG_MSG_ERROR("It's neither WKK nor HMT!",0,0,0);

  dhcp_reservations_updated = false;
  return;
}  /* end Init() */

 /*===========================================================================
 FUNCTION EnableNetIFd()
 ===========================================================================*/
 /** @ingroup qcmap_enable_netifd

  Enables Netifd. It writes wwan Backhaul parameters to a temp file
  and triggers rmnet_update shell script which is an indication to
  NetIFd to read the temp file and set up LAN routes.

  @datatypes
  qcmap_wwan_backhaul_info \n
  qcmap_backhaul_type

  @param[in]  wwan_info  WWAN Backhaul information \n
  @param[in]  bh_type    WWAN Backhaul type(v4/v6)

  @return
  TRUE -- Success \n
  FALSE -- Failure \n
*/
/*=========================================================================*/
boolean QCMAP_LAN_Client::EnableNetIFd
(
  qcmap_wwan_backhaul_info *wwan_info,
  qcmap_backhaul_type bh_type,
  qcmap_backhaul_enable_type enable_type = SETUP
)
{
  FILE *config_file = NULL;
  FILE *resolv_file = NULL;
  char file_name[QCMAP_MAX_COMMAND_LEN] = {0};
  char cmd[QCMAP_MAX_COMMAND_LEN] = {0};
  char addr_str[INET6_ADDRSTRLEN] = {0};
  char ipv4config[QCMAP_MAX_STRING_LEN] = {0};
  char ipv6config[QCMAP_MAX_STRING_LEN] = {0};
  qcmap_wwan_backhaul_info *bh_info;
  boolean retval = false;
  qmi_error_type_v01 error_num_temp = QMI_ERR_NONE_V01;
  int profile_idx = -1, active_ippt = -1;
  uint32_t MAX_DNS_STRING_LEN = QCMAP_DOMAIN_NAME_MAX_V01*QCMAP_MAX_NUM_DNS_SEARCH_LIST;
  char dns_str[MAX_DNS_STRING_LEN] ={0};
  boolean resolv_file_valid = true;

  //eth pdu
  int active_call_number = 0;
  int vlan_number_of_this_call = 0;
  uint16 vlan_id = 0;
  int loop = 0;
  int total_vlan_number=0;
  bool no_vlan_pdn = false;
  char result[QCMAP_MAX_SCAN_SIZE] = {0};
  memset(result, 0, QCMAP_MAX_SCAN_SIZE);

  memset(addr_str, 0, INET6_ADDRSTRLEN);
  memset(cmd, 0, QCMAP_MAX_COMMAND_LEN);
  memset(file_name, 0, QCMAP_MAX_COMMAND_LEN);

  if (wwan_info ==  NULL)
  {
    LOG_MSG_ERROR("wwan_info can't be NULL ",0,0,0);
    return false;
  }

  bh_info = wwan_info;
  if (BACKHAUL_V4 == bh_type)
  {
    /*Check file is already exist*/
    snprintf(ipv4config, QCMAP_MAX_STRING_LEN, "%s%s%d", CONFIG_FILE_PATH,
                 IPV4_CONFIG_FILE_NAME, wwan_info->profile_handle);

    if ( RECONFIG != enable_type)
    {
      if(CheckIfFileExists(ipv4config))
      {
        LOG_MSG_INFO1("V4 File already exist",0,0,0);
        printf("LAN side call is already up");
        return true;
      }
    }

    /* Create config file */
    snprintf(file_name, QCMAP_MAX_COMMAND_LEN, "%s%s%d", CONFIG_FILE_PATH,
              IPV4_CONFIG_FILE_NAME, bh_info->profile_handle);
    config_file = fopen(file_name, "w");
    if (config_file == NULL)
    {
      LOG_MSG_ERROR("Error opening file: %s", file_name, 0, 0);
      return retval;
    }
    resolv_file = fopen(RESOLV_PATH, "a");
    if (resolv_file == NULL)
    {
      LOG_MSG_ERROR("Error opening file: %s, errno: %d", RESOLV_PATH, errno, 0);
      resolv_file_valid=false;
    }
    /* Add interface name */
    fprintf(config_file, "export IFNAME=\"%s\"\n", bh_info->iface_name);
    /* Add Public V4 address */
    IPV4_CONVERT_NETWORK_ADDRESS_TO_STRING(bh_info->v4_addr, addr_str);
    fprintf(config_file, "export PUBLIC_IP=\"%s\"\n", addr_str);
    /* Add V4 subnet mask */
    memset(addr_str, 0, INET6_ADDRSTRLEN);
    IPV4_CONVERT_NETWORK_ADDRESS_TO_STRING(bh_info->v4_addr_subnet_mask,
                                           addr_str);
    fprintf(config_file, "export NETMASK=\"%s\"\n", addr_str);
    /* Add v4 primary and secondary DNS address */
    if ((bh_info->v4_pri_dns_addr != 0) && (bh_info->v4_sec_dns_addr != 0))
    {
      char pri_dns[INET6_ADDRSTRLEN] = {0};
      char sec_dns[INET6_ADDRSTRLEN] = {0};

      memset(pri_dns, 0, INET6_ADDRSTRLEN);
      memset(sec_dns, 0, INET6_ADDRSTRLEN);
      IPV4_CONVERT_NETWORK_ADDRESS_TO_STRING(bh_info->v4_pri_dns_addr,
                                           pri_dns);
      IPV4_CONVERT_NETWORK_ADDRESS_TO_STRING(bh_info->v4_sec_dns_addr,
                                           sec_dns);
      fprintf(config_file, "export DNSSERVERS=\"%s %s\"\n", pri_dns, sec_dns);
      if(resolv_file_valid)
      {
        fprintf(resolv_file, "nameserver %s\n", pri_dns);
        fprintf(resolv_file, "nameserver %s\n", sec_dns);
      }
    }
    else if (bh_info->v4_pri_dns_addr != 0)
    {
      memset(addr_str, 0, INET6_ADDRSTRLEN);
      IPV4_CONVERT_NETWORK_ADDRESS_TO_STRING(bh_info->v4_pri_dns_addr,
                                           addr_str);
      fprintf(config_file, "export DNSSERVERS=\"%s\"\n", addr_str);
      if(resolv_file_valid)
      {
        fprintf(resolv_file, "nameserver %s\n", addr_str);
      }
    }
    else if (bh_info->v4_sec_dns_addr != 0)
    {
      memset(addr_str, 0, INET6_ADDRSTRLEN);
      IPV4_CONVERT_NETWORK_ADDRESS_TO_STRING(bh_info->v4_sec_dns_addr,
                                           addr_str);
      fprintf(config_file, "export DNSSERVERS=\"%s\"\n", addr_str);
      if(resolv_file_valid)
      {
        fprintf(resolv_file, "nameserver %s\n", addr_str);
      }
    }
    else
    {
      fprintf(config_file, "export DNSSERVERS=\"\"\n");
    }
    /* Add v4 gateway address */
    memset(addr_str, 0, INET6_ADDRSTRLEN);
    IPV4_CONVERT_NETWORK_ADDRESS_TO_STRING(bh_info->v4_gw_addr,
                                           addr_str);
    fprintf(config_file, "export GATEWAY=\"%s\"\n", addr_str);

    if(bh_info->v4_mtu > 0)
    {
      /* Add MTU info */
      fprintf(config_file, "export IPV4MTU=\"%d\"\n",  bh_info->v4_mtu);
    }

    /* flush command */
    if (config_file)
    {
      fflush(config_file);
    }
    /* flush command */
    if (resolv_file)
    {
      fflush(resolv_file);
      fclose(resolv_file);
      resolv_file = NULL;
    }

    /* Send all information to script */
    snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "%s %s %u", IP_COLLISION_FILE,
             QCMAP_CHECK_ADDRESS_CONFLICT, (const uint32_t)bh_info->profile_handle);
    ds_system_call(cmd, strlen(cmd));

    /* Perform rmnet update */
    if (!UpdateRmnetFile(RMNET_BRING_UP_CMD, (const uint32_t)bh_info->profile_handle, bh_type))
    {
      LOG_MSG_ERROR("Failed to update NetIfD!", 0, 0, 0);
      return false;
    }
  }
  else if (BACKHAUL_V6 == bh_type)
  {
    /*Check file is already exist*/
    snprintf(ipv6config, QCMAP_MAX_STRING_LEN, "%s%s%d", CONFIG_FILE_PATH,
                 IPV6_CONFIG_FILE_NAME, wwan_info->profile_handle);
    if ( RECONFIG != enable_type )
    {
      if(CheckIfFileExists(ipv6config))
      {
        LOG_MSG_INFO1("V6 File already exist",0,0,0);
        printf("LAN side call is already up");
        return true;
      }
    }
    /* Create config file */
    snprintf(file_name, QCMAP_MAX_COMMAND_LEN, "%s%s%d", CONFIG_FILE_PATH,
              IPV6_CONFIG_FILE_NAME, bh_info->profile_handle);
    config_file = fopen(file_name, "w");
    if (config_file == NULL)
    {
      LOG_MSG_ERROR("Error opening file: %s", file_name, 0, 0);
      return retval;
    }
    resolv_file = fopen(RESOLV_PATH, "a");
    if (resolv_file == NULL)
    {
      LOG_MSG_ERROR("Error opening file: %s, errno: %d", RESOLV_PATH, errno, 0);
      resolv_file_valid=false;
    }
    /* Add interface name */
    fprintf(config_file, "export IFNAME=\"%s\"\n", bh_info->iface_name);
    /* Add Public V6 address */
    IPV6_CONVERT_NETWORK_ADDRESS_TO_STRING(bh_info->v6_addr, addr_str);
    fprintf(config_file, "export PUBLIC_IP6=\"%s\"\n", addr_str);
    /* Add V6 prefix length */
    fprintf(config_file, "export NETMASK6=\"%u\"\n", bh_info->v6_addr_prefix_len);
    /* Add V6 Prefix */
    unsigned char v6_prefix[sizeof(struct in6_addr)] = {0};
    memset(v6_prefix, 0, sizeof(struct in6_addr));
    bool retval = QCMAP_LAN_Client::GetIPv6Prefix((const char *)addr_str,
                                                   v6_prefix, bh_info->v6_addr_prefix_len);
    if (retval)
    {
      memset(addr_str, 0, INET6_ADDRSTRLEN);
      if (inet_ntop(AF_INET6, v6_prefix, addr_str, INET6_ADDRSTRLEN) == NULL)
      {
        LOG_MSG_ERROR("Prefix not obtained", 0, 0, 0);
        return retval;
      }
      fprintf(config_file, "export PREFIX6=\"%s/%u\"\n", addr_str,bh_info->v6_addr_prefix_len);
    }
    else
    {
      LOG_MSG_ERROR("Error in retrieving IPV6 Prefix", 0, 0, 0);
    }
    /* Add V6 Gateway Address */
    memset(addr_str, 0, INET6_ADDRSTRLEN);
    IPV6_CONVERT_NETWORK_ADDRESS_TO_STRING(bh_info->v6_gw_addr, addr_str);
    fprintf(config_file, "export GATEWAY6=\"%s\"\n", addr_str);

    if(bh_info->v6_mtu > 0)
    {
      /* Add MTU info */
      fprintf(config_file, "export IPV6MTU=\"%d\"\n",  bh_info->v6_mtu);
    }

    /* Add DNS server Address */
    if (!(IN6_IS_ADDR_UNSPECIFIED_32(bh_info->v6_pri_dns_addr)) &&
        !(IN6_IS_ADDR_UNSPECIFIED_32(bh_info->v6_sec_dns_addr)))
    {
      char pri_dns[INET6_ADDRSTRLEN] = {0};
      char sec_dns[INET6_ADDRSTRLEN] = {0};

      memset(pri_dns, 0, INET6_ADDRSTRLEN);
      memset(sec_dns, 0, INET6_ADDRSTRLEN);
      IPV6_CONVERT_NETWORK_ADDRESS_TO_STRING(bh_info->v6_pri_dns_addr,
                                           pri_dns);
      IPV6_CONVERT_NETWORK_ADDRESS_TO_STRING(bh_info->v6_sec_dns_addr,
                                           sec_dns);
      fprintf(config_file, "export DNSSERVERS6=\"%s %s\"\n", pri_dns, sec_dns);
      if(resolv_file_valid)
      {
        fprintf(resolv_file, "nameserver %s\n", pri_dns);
        fprintf(resolv_file, "nameserver %s\n", sec_dns);
      }
    }
    else if (!(IN6_IS_ADDR_UNSPECIFIED_32(bh_info->v6_pri_dns_addr)))
    {
      memset(addr_str, 0, INET6_ADDRSTRLEN);
      IPV6_CONVERT_NETWORK_ADDRESS_TO_STRING(bh_info->v6_pri_dns_addr,
                                           addr_str);
      fprintf(config_file, "export DNSSERVERS6=\"%s\"\n", addr_str);
      if(resolv_file_valid)
      {
        fprintf(resolv_file, "nameserver %s\n", addr_str);
      }
    }
    else if (!(IN6_IS_ADDR_UNSPECIFIED_32(bh_info->v6_sec_dns_addr)))
    {
      memset(addr_str, 0, INET6_ADDRSTRLEN);
      IPV6_CONVERT_NETWORK_ADDRESS_TO_STRING(bh_info->v6_sec_dns_addr,
                                           addr_str);
      fprintf(config_file, "export DNSSERVERS6=\"%s\"\n", addr_str);
      if(resolv_file_valid)
      {
        fprintf(resolv_file, "nameserver %s\n", addr_str);
      }
    }
    else
    {
      fprintf(config_file, "export DNSSERVERS6=\"\"\n");
    }
    /* Add DNS Search List */
    if (bh_info->dns_search_list_len != 0)
    {
        dns_list_to_string(dns_str, MAX_DNS_STRING_LEN, bh_info->dns_search_list, bh_info->dns_search_list_len);
        fprintf(config_file, "export DNSSEARCH=\"%s\"\n",dns_str);
        if(resolv_file_valid)
        {
          fprintf(resolv_file, "search %s\n", dns_str);
        }
    }

    /* Get profile_idx from qcmap_lan */
    profile_idx = GetProfileIndex(bh_info->profile_handle);
    if (profile_idx != QCMAP_LAN_INVALID)
    {
      /* Check if active_ippt is enabled */
      active_ippt = GetActiveIPPT(profile_idx);
      if (active_ippt == 1)
        sleep(1);
    }
    /* Perform rmnet update */
    if (!UpdateRmnetFile(RMNET_BRING_UP_CMD, (const uint32_t)bh_info->profile_handle, bh_type))
    {
      LOG_MSG_ERROR("Failed to update NetIfD!", 0, 0, 0);
      return false;
    }
  }
  else if (BACKHAUL_ETHPDU == bh_type)
  {

    /* Create config file */
    snprintf(file_name, QCMAP_MAX_COMMAND_LEN, "%s%s%d", CONFIG_FILE_PATH,
              ETHPDU_CONFIG_FILE_NAME, bh_info->profile_handle);
    config_file = fopen(file_name, "w");
    if (config_file == NULL)
    {
      LOG_MSG_ERROR("Error opening file: %s", file_name, 0, 0);
      return retval;
    }
    /* Add interface name */
    fprintf(config_file, "export IFNAME=\"%s\"\n", bh_info->iface_name);
    /* Add VLAN start/end */
    fprintf(config_file, "export VLAN_START=\"%u\"\n", bh_info->vlan_start);
    fprintf(config_file, "export VLAN_END=\"%u\"\n", bh_info->vlan_end);

    /*Need to fflush file here as it will be used in update script*/
    fflush(config_file);
    fclose(config_file);
    config_file = NULL;

    /* Perform rmnet Eth update */
    if (!UpdateRmnetEthFile(RMNET_BRING_UP_CMD, (const uint32_t)bh_info->profile_handle))
    {
      LOG_MSG_ERROR("Failed to update NetIfD!", 0, 0, 0);
      return false;
    }
  }
  else
  {
    LOG_MSG_ERROR("Unrecognized PDN type passed", 0, 0, 0);
    return retval;
  }

  /* flush command */
  if (config_file)
  {
    fflush(config_file);
    fclose(config_file);
    config_file = NULL;
  }

  /* flush command */
  if (resolv_file)
  {
    fflush(resolv_file);
    fclose(resolv_file);
    resolv_file = NULL;
  }
  retval = true;
  if(BACKHAUL_ETHPDU != bh_type)
  {
    QCMAP_LAN_Client::AddWanIfaceOnWanAllList(bh_info->profile_handle);

    if (BACKHAUL_V4 == bh_type)
    {
      /* Perform dnsmasq restart and link toggle based on uci option queries */
      QCMAP_LAN_Client::CheckIPPTMode((const uint32_t)bh_info->profile_handle,BH_EVENT);
    }
  }

  return retval;
}  /*End EnableNetIFd() */

/*===========================================================================
FUNCTION DelConfigFile()
===========================================================================*/
/** @ingroup qcmap_delete_config_file

  Deletes config file in /tmp directory during BH tear down event.

  @datatypes
  qcmap_backhaul_type

  @param[in]      profile_num      BH profile number \n
  @param[in]      bh_type          WWAN Backhaul type(v4/v6)

  @return
  TRUE -- Success \n
  FALSE -- Failure \n
*/
/*=========================================================================*/
boolean QCMAP_LAN_Client::DelConfigFile
(
  uint32_t profile_num,
  qcmap_backhaul_type bh_type
)
{
  FILE *config_file = NULL;
  char file_name[QCMAP_MAX_COMMAND_LEN] = {0};
  char cmd[QCMAP_MAX_COMMAND_LEN] = {0};
  char addr_str[INET6_ADDRSTRLEN] = {0};
  boolean retval = false;
  qmi_error_type_v01 error_num_temp = QMI_ERR_NONE_V01;
  uint32_t MAX_DNS_STRING_LEN = QCMAP_DOMAIN_NAME_MAX_V01*QCMAP_MAX_NUM_DNS_SEARCH_LIST;
  int ns_buffer_len = 2*INET6_ADDRSTRLEN + strlen("export DNSSERVERS6") + 4;
  char nameserver_buffer[ns_buffer_len];
  std::string filtered_str, temp;
  char pri_addr_str[INET6_ADDRSTRLEN] = {0};
  char sec_addr_str[INET6_ADDRSTRLEN] = {0};
  char dns_str[MAX_DNS_STRING_LEN] = {0};
  boolean pri_valid = false;
  boolean sec_valid = false;
  boolean dns_list_valid = false;
  qmi_error_type_v01 qmi_err_num;

  //eth pdu
  uint32_t profile_handle = 0;
  int active_call_number = 0;
  int total_vlan_number=0;
  int vlan_number_temp=0;
  int vlanId = 0;
  int loop = 0;
  int vlanStart = 0;
  bool no_vlan_pdn_flag = false;
  int vlanEnd = 0;
  int vlan_number_of_this_call = 0;
  char result[QCMAP_MAX_SCAN_SIZE] = {0};
  char sectionName[QCMAP_MAX_SCAN_SIZE] = {0};
  char iface_name[QCMAP_MAX_IFACE_NAME_SIZE];
  memset(result, 0, QCMAP_MAX_SCAN_SIZE);

  memset(cmd, 0, QCMAP_MAX_COMMAND_LEN);
  memset(file_name, 0, QCMAP_MAX_COMMAND_LEN);

  /* Get dns search list from uci to avoid concurrency with teardown script */
  if (BACKHAUL_V6 == bh_type)
  {
    if ( QCMAP_LAN_Client::UciGetUtility(owrt_filename[4], "lan", 1, "domain", dns_str, 0) )
    {
      dns_list_valid = true;
    }
  }

  /* Perform rmnet ETH update */
  if (BACKHAUL_ETHPDU == bh_type)
  {
    if (!UpdateRmnetEthFile(RMNET_TEAR_DOWN_CMD, (const uint32_t)profile_num))
    {
      LOG_MSG_ERROR("Failed to update NetIfD!", 0, 0, 0);
      return false;
    }
  }

  if((BACKHAUL_V6 == bh_type) || (BACKHAUL_V4 == bh_type))
  {
    if (!UpdateRmnetFile(RMNET_TEAR_DOWN_CMD, (const uint32_t)profile_num,bh_type))
    {
      LOG_MSG_ERROR("Failed to update NetIfD!", 0, 0, 0);
      return false;
    }
  }

  /* Delete config file */
  if (BACKHAUL_V6 == bh_type)
  {
    snprintf(file_name, QCMAP_MAX_COMMAND_LEN, "%s%s%d", CONFIG_FILE_PATH,
              IPV6_CONFIG_FILE_NAME, profile_num);
    config_file = fopen(file_name, "a+");
    if (config_file == NULL)
    {
      LOG_MSG_ERROR("Error opening file: %s", file_name, 0, 0);
      return retval;
    }

    /*
      Get nameservers from config file
        1. Loop through each line of config file
        2. Check to see if export DNSSERVERS is in line
        3. If so:
            4. Strip quotation marks
            5. Get value substring of nameserver post '='
        6. Check if there is ' ' in value substr
            7. If yes there are pri and sec dns nameserver
            8. If no only one dns nameserver
    */
    filtered_str.clear();
    memset(nameserver_buffer, 0, sizeof(nameserver_buffer));
    while(fgets(nameserver_buffer, ns_buffer_len, config_file) != NULL)
    {
      temp.clear();
      temp = nameserver_buffer;

      if((std::string::npos != temp.find("export DNSSERVERS6")))
      {
        //save this line
        filtered_str.append(temp);
        filtered_str.erase(std::remove( filtered_str.begin(), filtered_str.end(), '\"' ),
            filtered_str.end());
        filtered_str = filtered_str.substr(filtered_str.find('=')+1, filtered_str.size());
        if(std::string::npos != filtered_str.find(' '))
        {
          strlcpy(pri_addr_str, filtered_str.substr(0,
            filtered_str.find(' ')).c_str(), INET6_ADDRSTRLEN);
          strlcpy(sec_addr_str, filtered_str.substr(filtered_str.find(' ')+1,
            filtered_str.size()).c_str(), INET6_ADDRSTRLEN);
          pri_valid = true;
          sec_valid = true;
        }
        else
        {
          strlcpy(pri_addr_str, filtered_str.c_str(), INET6_ADDRSTRLEN);
          pri_valid = true;
        }
        break;
      }
      memset(nameserver_buffer, 0, sizeof(nameserver_buffer));
    }
    /* end of while */
    DeleteDNSFromResolv(pri_addr_str, sec_addr_str, dns_str, pri_valid,
      sec_valid, dns_list_valid, MAX_DNS_STRING_LEN);

    LOG_MSG_INFO1("Removing config file: %s", file_name, 0, 0);
    memset(cmd, 0, QCMAP_MAX_COMMAND_LEN);
    snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "rm -rf %s", file_name);
    ds_system_call(cmd, strlen(cmd));
  }
  else if (BACKHAUL_V4 == bh_type)
  {
    pri_valid = false;
    sec_valid = false;
    dns_list_valid = false;
    snprintf(file_name, QCMAP_MAX_COMMAND_LEN, "%s%s%d", CONFIG_FILE_PATH,
              IPV4_CONFIG_FILE_NAME, profile_num);
    config_file = fopen(file_name, "a+");
    if (config_file == NULL)
    {
      LOG_MSG_ERROR("Error opening file: %s", file_name, 0, 0);
      return retval;
    }

    /*
      Get nameservers from config file
        1. Loop through each line of config file
        2. Check to see if export DNSSERVERS is in line
        3. If so:
            4. Strip quotation marks
            5. Get value substring of nameserver post '='
        6. Check if there is ' ' in value substr
            7. If yes there are pri and sec dns nameserver
            8. If no only one dns nameserver
    */
    filtered_str.clear();
    memset(nameserver_buffer, 0, sizeof(nameserver_buffer));
    while(fgets(nameserver_buffer, ns_buffer_len, config_file) != NULL)
    {
      temp.clear();
      temp = nameserver_buffer;

      if((std::string::npos != temp.find("export DNSSERVERS")))
      {
        //save this line
        filtered_str.append(temp);
        filtered_str.erase(std::remove( filtered_str.begin(), filtered_str.end(), '\"' ),
            filtered_str.end());
        filtered_str = filtered_str.substr(filtered_str.find('=')+1, filtered_str.size());
        if(std::string::npos != filtered_str.find(' '))
        {
          strlcpy(pri_addr_str, filtered_str.substr(0,
            filtered_str.find(' ')).c_str(), INET6_ADDRSTRLEN);
          strlcpy(sec_addr_str, filtered_str.substr(filtered_str.find(' ')+1,
            filtered_str.size()).c_str(), INET6_ADDRSTRLEN);
          pri_valid = true;
          sec_valid = true;
        }
        else
        {
          strlcpy(pri_addr_str, filtered_str.c_str(), INET6_ADDRSTRLEN);
          pri_valid = true;
        }
        break;
      }
      memset(nameserver_buffer, 0, sizeof(nameserver_buffer));
    }
    /* end of while */
    DeleteDNSFromResolv(pri_addr_str, sec_addr_str, dns_str, pri_valid,
      sec_valid, dns_list_valid, MAX_DNS_STRING_LEN);

    LOG_MSG_INFO1("Removing config file: %s", file_name, 0, 0);
    memset(cmd, 0, QCMAP_MAX_COMMAND_LEN);
    snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "rm -rf %s", file_name);
    ds_system_call(cmd, strlen(cmd));
  }
  else if (BACKHAUL_ETHPDU == bh_type)
  {
    //sleep 1 to wait for script finish deleting vlan info and remove the interface from bridge
    sleep(1);
    snprintf(file_name, QCMAP_MAX_COMMAND_LEN, "%s%s%d", CONFIG_FILE_PATH,
          ETHPDU_CONFIG_FILE_NAME, profile_num);
    config_file = fopen(file_name, "w");
    if (config_file == NULL)
    {
      LOG_MSG_ERROR("Error opening file: %s", file_name, 0, 0);
      return retval;
    }
    LOG_MSG_INFO1("Removing eth pdu config file: %s", file_name, 0, 0);
    memset(cmd, 0, QCMAP_MAX_COMMAND_LEN);
    snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "rm -rf %s", file_name);
    ds_system_call(cmd, strlen(cmd));
  }
  else
  {
    LOG_MSG_ERROR("Unrecognized PDN type passed", 0, 0, 0);
    return retval;
  }
  /* flush command */
  if (config_file)
  {
    fflush(config_file);
    fclose(config_file);
    config_file = NULL;
  }
  retval = true;

  if (BACKHAUL_ETHPDU != bh_type)
  {
    QCMAP_LAN_Client::DelWanIfaceFromWanAllList(profile_num);

    if (BACKHAUL_V4 == bh_type)
    {
      /* NOTE: Reason for adding here is to ensure this function gets called only for v4
      Backhaul disconnect event */
      /* Perform dnsmasq restart and link toggle based on uci option queries */
      QCMAP_LAN_Client::CheckIPPTMode((const uint32_t)profile_num, BH_EVENT);
    }
  }

  return retval;
} /*End DelConfigFile() */

/*===========================================================================
  FUNCTION GetIfaceNameFromEnum
==========================================================================*/
/*!
@brief
  Gets iface name for a given iface_enum.


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
bool QCMAP_LAN_Client::GetIfaceNameFromEnum
(
  qcmap_interface_type_enum iface_type,
  char                      *iface_name
)
{
  bool ret_val = true;
  char command[MAX_COMMAND_STR_LEN] = {0};
  char temp_iface_name[QCMAP_MAX_IFACE_NAME_SIZE_V01] = {};

  if (iface_name == NULL)
  {
    ret_val = false;
    LOG_MSG_ERROR("INVALID_ARG!!! iface_name is NULL",0,0,0);
    return ret_val;
  }

  switch (iface_type)
  {
    case QCMAP_INTERFACE_TYPE_ETH:
      ret_val = GetTetheredIfaceNameFromUCI("eth", temp_iface_name);
      break;

    case QCMAP_INTERFACE_TYPE_ETH_NIC2:
      ret_val = GetTetheredIfaceNameFromUCI("eth_nic2", temp_iface_name);
      break;

    case QCMAP_INTERFACE_TYPE_ECM:
      ret_val = GetTetheredIfaceNameFromUCI("ecm", temp_iface_name);
      break;

    case QCMAP_INTERFACE_TYPE_RNDIS:
      ret_val = GetTetheredIfaceNameFromUCI("rndis", temp_iface_name);
      break;

    default:
      LOG_MSG_ERROR("Invalid iface_type=%d", iface_type, 0,0);
      ret_val = false;
      return ret_val;
  }
  if (ret_val)
  {
    snprintf(iface_name, QCMAP_MAX_IFACE_NAME_SIZE_V01, "%s", temp_iface_name);
  }
  else
  {
    LOG_MSG_ERROR("failed to get iface_name for iface_type:%d", iface_type, 0, 0);
  }
  LOG_MSG_INFO1("iface_name:%s for iface_type:%d", iface_name, iface_type, 0);
  return ret_val;
}

/*===========================================================================
  FUNCTION GetIfaceEnumFromName
==========================================================================*/
/*!
@brief
  Gets iface name for a given iface_enum.


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
bool QCMAP_LAN_Client::GetIfaceEnumFromName
(
  const char                 *iface_name,
  qcmap_interface_type_enum  *iface_type
)
{
  bool ret_val = true;
  char eth_iface[QCMAP_MAX_IFACE_NAME_SIZE_V01] = {0};
  char eth_nic2_iface[QCMAP_MAX_IFACE_NAME_SIZE_V01] = {0};
  char ecm_iface[QCMAP_MAX_IFACE_NAME_SIZE_V01] = {0};
  char rndis_iface[QCMAP_MAX_IFACE_NAME_SIZE_V01] = {0};

  if (iface_name == NULL || iface_type == NULL)
  {
    LOG_MSG_ERROR("INVALID_ARG!!! iface_name or iface_type is NULL",0,0,0);
    ret_val = false;
    return ret_val;
  }
  if (!GetTetheredIfaceNameFromUCI("rndis", rndis_iface))
  {
    LOG_MSG_ERROR("failed to get RNDIS interface from UCI", 0, 0, 0);
    return false;
  }
  if (!GetTetheredIfaceNameFromUCI("ecm", ecm_iface))
  {
    LOG_MSG_ERROR("failed to get ECM interface from UCI", 0, 0, 0);
    return false;
  }
  if (!GetTetheredIfaceNameFromUCI("eth", eth_iface))
  {
    LOG_MSG_ERROR("failed to get ETH interface from UCI", 0, 0, 0);
    return false;
  }
  if (!GetTetheredIfaceNameFromUCI("eth_nic2", eth_nic2_iface))
  {
    LOG_MSG_ERROR("failed to get ETH_NIC2 interface from UCI", 0, 0, 0);
    return false;
  }

  if (!strncmp(iface_name, ecm_iface, strlen(ecm_iface)))
  {
    *iface_type = QCMAP_INTERFACE_TYPE_ECM;
  }
  else if (!strncmp(iface_name, rndis_iface, strlen(rndis_iface)))
  {
    *iface_type = QCMAP_INTERFACE_TYPE_RNDIS;
  }
  else if (!strncmp(iface_name, eth_iface, strlen(eth_iface)))
  {
    *iface_type = QCMAP_INTERFACE_TYPE_ETH;
  }
  else if (!strncmp(iface_name, eth_nic2_iface, strlen(eth_nic2_iface)))
  {
    *iface_type = QCMAP_INTERFACE_TYPE_ETH_NIC2;
  }
  else
  {
    LOG_MSG_ERROR("Invalid iface_name=%d", iface_name, 0,0);
    ret_val     = false;
    *iface_type = QCMAP_INTERFACE_TYPE_ENUM_MAX_ENUM_VAL;
  }
  return ret_val;
}

/*===========================================================================
FUNCTION CreateWWANPolicy()
===========================================================================*/
/** @ingroup section_CreateWWANPolicy

  Creates a WWAN profile in UCI data base whenever a new profile is created.

  @datatypes
  uint32_t \n

  @param[in] profile_handle  profile_handle created .

  @return
  void

  @dependencies
  QCMobileAP must be enabled. @newpage
*/
/*=========================================================================*/
void QCMAP_LAN_Client::CreateWWANPolicy
(
  uint32_t wan_profile_handle,
  qmi_error_type_v01 *qmi_err_num
)
{
  if ( wan_profile_handle == 0 )
  {
    LOG_MSG_ERROR("Invalid wan profile handle passed ",0,0,0);
    *qmi_err_num = QMI_ERR_INVALID_ARG_V01;
    return;
  }

  char command[QCMAP_MAX_COMMAND_LEN] = {0};
  snprintf(command, QCMAP_MAX_COMMAND_LEN,"%s %s %d", BACKHAUL_WWAN_CONFIG_FILE, QCMAP_CREATE_WWAN_PROFILE, wan_profile_handle);
  ds_system_call(command, strlen(command));
  return;
}

/*===========================================================================
FUNCTION CreateWWANPolicy()
===========================================================================*/
/** @ingroup section_CreateWWANPolicy

  Creates a WWAN profile in UCI data base whenever a new profile is created.

  @datatypes
  uint32_t \n

  @param[in] profile_handle  profile_handle created .

  @return
  void

  @dependencies
  QCMobileAP must be enabled. @newpage
*/
/*=========================================================================*/
void QCMAP_LAN_Client::CreateWWANPolicyEx
(
  uint32_t wan_profile_handle,
  qcmap_msgr_ip_family_enum_v01      ip_family,
  qmi_error_type_v01 *qmi_err_num
)
{
  if ( wan_profile_handle == 0 )
  {
    LOG_MSG_ERROR("Invalid wan profile handle passed ",0,0,0);
    *qmi_err_num = QMI_ERR_INVALID_ARG_V01;
    return;
  }

  if (ip_family != QCMAP_MSGR_IP_FAMILY_V4_V01 &&
      ip_family != QCMAP_MSGR_IP_FAMILY_V6_V01 &&
      ip_family != QCMAP_MSGR_IP_FAMILY_V4V6_V01 &&
      ip_family != QCMAP_MSGR_IP_FAMILY_ETH_V01)
  {
    LOG_MSG_ERROR("Invalid IP family passed",0,0,0);
    *qmi_err_num = QMI_ERR_INVALID_ARG_V01;
    return;
  }

  char command[QCMAP_MAX_COMMAND_LEN] = {0};
  snprintf(command, QCMAP_MAX_COMMAND_LEN,"%s %s %d %d", BACKHAUL_WWAN_CONFIG_FILE, QCMAP_CREATE_WWAN_PROFILE, wan_profile_handle,ip_family);
  ds_system_call(command, strlen(command));
  return;
}

/*===========================================================================
FUNCTION AddWanIfaceOnWanAllList()
===========================================================================*/
/** @ingroup section_AddWanIfaceOnWanAllList

  adds a wan iface corresponding to profile id as a list option in wan_all zone.

  @datatypes
  uint32_t \n

  @param[in] profile_handle         profile_handle of the wan profile.

  @return
  void

  @dependencies
  QCMobileAP must be enabled. @newpage
*/
/*=========================================================================*/
void QCMAP_LAN_Client::AddWanIfaceOnWanAllList
(
  uint32_t wan_profile_handle
)
{
  if ( wan_profile_handle == 0 )
  {
    LOG_MSG_ERROR("Invalid wan profile handle passed ",0,0,0);
    return;
  }

  char command[QCMAP_MAX_COMMAND_LEN] = {0};
  snprintf(command, QCMAP_MAX_COMMAND_LEN,"%s %s %d", BACKHAUL_WWAN_CONFIG_FILE,
    QCMAP_ADD_WAN_IFACE_ON_WAN_ALL_LIST, wan_profile_handle);
  ds_system_call(command, strlen(command));
  return;
}

/*===========================================================================
FUNCTION DelWanIfaceOnWanAllList()
===========================================================================*/
/** @ingroup section_DelWanIfaceOnWanAllList

  Deletes a wan iface corresponding to profile id as a list option in wan_all zone.

  @datatypes
  uint32_t \n

  @param[in] profile_handle         profile_handle of the wan profile.

  @return
  void

  @dependencies
  QCMobileAP must be enabled. @newpage
*/
/*=========================================================================*/
void QCMAP_LAN_Client::DelWanIfaceFromWanAllList
(
  uint32_t wan_profile_handle
)
{
  if ( wan_profile_handle == 0 )
  {
    LOG_MSG_ERROR("Invalid wan profile handle passed ",0,0,0);
    return;
  }

  char command[QCMAP_MAX_COMMAND_LEN] = {0};
  snprintf(command, QCMAP_MAX_COMMAND_LEN,"%s %s %d",BACKHAUL_WWAN_CONFIG_FILE,
    QCMAP_DEL_WAN_IFACE_FROM_WAN_ALL_LIST, wan_profile_handle);
  ds_system_call(command, strlen(command));
  return;
}

/*===========================================================================
  FUNCTION GetEthPDUStatus
  ===========================================================================*/
/*!
  @brief
  Get current eth pdu enable status

  @return
  true  - on eth pdu enabled
  false - on eth pdu disabled

  @note

  - Dependencies
  - None

  - Side Effects
  - None
 */
/*=========================================================================*/
boolean QCMAP_LAN_Client::GetEthPDUStatus( )
{
  char cmd[QCMAP_MAX_COMMAND_LEN] = {0};
  char result[QCMAP_MAX_SCAN_SIZE] = {0};

/* For ETH PDU feature mode */
  memset(cmd, 0, QCMAP_MAX_COMMAND_LEN);
  memset(result, 0, QCMAP_MAX_SCAN_SIZE);
  snprintf(cmd, QCMAP_MAX_COMMAND_LEN, " %s %s",
           UCI_GET_COMMAND, QCMAP_ETH_PDU_FEATURE_MODE);
  if (ExecuteSystemCmd((const char *)cmd, result, sizeof(result)))
  {
    if (atoi(result))
      return true;
    else
      return false;
  }
  return false;
}

/*===========================================================================
  FUNCTION GetCurrentActiveV4Backhaul
  ===========================================================================*/
/*!
  @brief
  Get current active v4 Backahul on current profile handle

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
boolean QCMAP_LAN_Client::GetCurrentActiveV4Backhaul
(
 qcmap_backhaul_status_info_ex_type *backhaul_status_info,
 char *bh_present_v4,
 qmi_error_type_v01 *qmi_err_num
)
{
  if((strncmp(bh_present_v4, BACKHAUL_WWAN_NAME, strlen(BACKHAUL_WWAN_NAME)) == 0) &&
     (strlen(bh_present_v4) == strlen(BACKHAUL_WWAN_NAME)))
  {
    backhaul_status_info->backhaul_type = QCMAP_MSGR_WWAN_BACKHAUL_V01;
    backhaul_status_info->backhaul_v4_available = true;
#ifdef FEATURE_DATA_ETH_PDU
    if ( GetEthPDUStatus())
      backhaul_status_info->backhaul_eth_available = true;
#endif
  }
  else if ((strncmp(bh_present_v4, BACKHAUL_WLAN_NAME, strlen(BACKHAUL_WLAN_NAME)) == 0) &&
           (strlen(bh_present_v4) == strlen(BACKHAUL_WLAN_NAME)))
  {
    backhaul_status_info->backhaul_type = QCMAP_MSGR_WLAN_BACKHAUL_V01;
    backhaul_status_info->backhaul_v4_available = true;
  }
  else if ((strncmp(bh_present_v4, BACKHAUL_ETH_NAME, strlen(BACKHAUL_ETH_NAME)) == 0 &&
          (strlen(bh_present_v4) == strlen(BACKHAUL_ETH_NAME) )))
  {
    backhaul_status_info->backhaul_type = QCMAP_MSGR_ETHERNET_BACKHAUL_V01;
    backhaul_status_info->backhaul_v4_available = true;
  }
  else if ((strncmp(bh_present_v4, BACKHAUL_USB_NAME, strlen(BACKHAUL_USB_NAME)) == 0) &&
          (strlen(bh_present_v4) == strlen(BACKHAUL_USB_NAME)))
  {
    backhaul_status_info->backhaul_type = QCMAP_MSGR_USB_CRADLE_BACKHAUL_V01;
    backhaul_status_info->backhaul_v4_available = true;
  }
  else if ((strncmp(bh_present_v4, BACKHAUL_BT_NAME, strlen(BACKHAUL_BT_NAME)) == 0) &&
          (strlen(bh_present_v4) == strlen(BACKHAUL_BT_NAME)))
  {
    backhaul_status_info->backhaul_type = QCMAP_MSGR_BT_BACKHAUL_V01;
    backhaul_status_info->backhaul_v4_available = true;
  }
  else if( strlen(bh_present_v4) == 0 )
  {
    LOG_MSG_INFO1("No V4 Backhaul active", 0, 0, 0);
    backhaul_status_info->backhaul_v4_available = false;
  }
  return true;
}

/*===========================================================================
  FUNCTION GetCurrentActiveV6Backhaul
  ===========================================================================*/
/*!
  @brief
  Get current active v6 Backahul on current profile handle

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
boolean QCMAP_LAN_Client::GetCurrentActiveV6Backhaul
(
 qcmap_backhaul_status_info_ex_type *backhaul_status_info,
 char *bh_present_v6,
 qmi_error_type_v01 *qmi_err_num
)
{
  if ((strncmp(bh_present_v6, BACKHAUL_WWAN_NAME, strlen(BACKHAUL_WWAN_NAME)) == 0) &&
     (strlen(bh_present_v6) == strlen(BACKHAUL_WWAN_NAME)))
  {
    backhaul_status_info->backhaul_type = QCMAP_MSGR_WWAN_BACKHAUL_V01;
    backhaul_status_info->backhaul_v6_available = true;
#ifdef FEATURE_DATA_ETH_PDU
    if ( GetEthPDUStatus())
      backhaul_status_info->backhaul_eth_available = true;
#endif
  }
  else if ((strncmp(bh_present_v6, BACKHAUL_WLAN_NAME, strlen(BACKHAUL_WLAN_NAME)) == 0) &&
           (strlen(bh_present_v6) == strlen(BACKHAUL_WLAN_NAME)))
  {
   backhaul_status_info->backhaul_type = QCMAP_MSGR_WLAN_BACKHAUL_V01;
   backhaul_status_info->backhaul_v6_available = true;
  }
  else if ((strncmp(bh_present_v6, BACKHAUL_ETH_NAME, strlen(BACKHAUL_ETH_NAME)) == 0 &&
          (strlen(bh_present_v6) == strlen(BACKHAUL_ETH_NAME) )))
  {
    backhaul_status_info->backhaul_type = QCMAP_MSGR_ETHERNET_BACKHAUL_V01;
    backhaul_status_info->backhaul_v6_available = true;
  }
  else if ((strncmp(bh_present_v6, BACKHAUL_USB_NAME, strlen(BACKHAUL_USB_NAME)) == 0) &&
          (strlen(bh_present_v6) == strlen(BACKHAUL_USB_NAME)))
  {
    backhaul_status_info->backhaul_type = QCMAP_MSGR_USB_CRADLE_BACKHAUL_V01;
    backhaul_status_info->backhaul_v6_available = true;
  }
  else if ((strncmp(bh_present_v6, BACKHAUL_BT_NAME, strlen(BACKHAUL_BT_NAME)) == 0) &&
          (strlen(bh_present_v6) == strlen(BACKHAUL_BT_NAME)))
  {
    backhaul_status_info->backhaul_type = QCMAP_MSGR_BT_BACKHAUL_V01;
    backhaul_status_info->backhaul_v6_available = true;
  }
  else if( strlen(bh_present_v6) == 0 )
  {
    LOG_MSG_INFO1("No V6 Backhaul active", 0, 0, 0);
    backhaul_status_info->backhaul_v6_available = false;
  }
  return true;
}

/*===========================================================================
  FUNCTION GetCurrentActiveBackhaul
  ===========================================================================*/
/*!
  @brief
  Get current active Backahul on current profile handle

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
boolean QCMAP_LAN_Client::GetCurrentActiveBackhaul
(
 uint32_t wan_profile_handle,
 qcmap_backhaul_status_info_ex_type *backhaul_status_info,
 qmi_error_type_v01 *qmi_err_num
)
{
  char bh_present_v4[QCMAP_MAX_IFACE_NAME_SIZE] = {0};
  char bh_present_v6[QCMAP_MAX_IFACE_NAME_SIZE] = {0};
  int profile_idx = -1;

  profile_idx = GetProfileIndex(wan_profile_handle);

  if (!GetCurrentBackhaul(bh_present_v4, bh_present_v6, profile_idx))
  {
    LOG_MSG_ERROR("Error getting current backhaul.\n ", 0,0,0);
    return false;
  }

  /*Intialize backhaul available params to false*/
    backhaul_status_info->backhaul_v4_available = false;
    backhaul_status_info->backhaul_v6_available = false;
#ifdef FEATURE_DATA_ETH_PDU
    backhaul_status_info->backhaul_eth_available = false;
#endif

  if (GetBackhaulStatus(wan_profile_handle, IP_V6, qmi_err_num)
      && GetCurrentActiveV6Backhaul(backhaul_status_info, bh_present_v6, qmi_err_num))
  {
    LOG_MSG_INFO1("Successfully get current v6 bh", 0, 0, 0);
  }
  else
    LOG_MSG_ERROR("Error getting current active v6 backhaul.\n ", 0,0,0);

  if (GetBackhaulStatus(wan_profile_handle, IP_V4, qmi_err_num)
      && GetCurrentActiveV4Backhaul(backhaul_status_info, bh_present_v4, qmi_err_num))
  {
    LOG_MSG_INFO1("Successfully get current v6 bh", 0, 0, 0);
  }
  else
    LOG_MSG_ERROR("Error getting current active v6 backhaul.\n ", 0,0,0);

  return true;
}


/*===========================================================================
  FUNCTION GetBackhaulStatus
  ===========================================================================*/
/*!
  @brief
  Gets Backahul Status on current profile handle

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

boolean QCMAP_LAN_Client::GetBackhaulStatus
(
 uint32_t wan_profile_handle,
 ip_version_enum_type ip_version,
 qmi_error_type_v01 *qmi_err_num
)
{
  boolean is_pdn_active = false;
  char result[QCMAP_MAX_SCAN_SIZE] = {0};
  char cmd[QCMAP_MAX_COMMAND_LEN] = {0};
  char ipv4config[QCMAP_MAX_STRING_LEN] = {0};
  char ipv6config[QCMAP_MAX_STRING_LEN] = {0};
  char if_name[QCMAP_MAX_IFACE_NAME_SIZE] = {0};
  int profile_idx = -1;

  profile_idx = GetProfileIndex(wan_profile_handle);
  if (ip_version == IP_V4)
    snprintf(cmd, MAX_COMMAND_STR_LEN,
             "/etc/data/uci_ex.sh get qcmap_lan.@profile[%d].bh_present | tr -d ' \n'", profile_idx);
  else if (ip_version == IP_V6)
    snprintf(cmd, MAX_COMMAND_STR_LEN,
             "/etc/data/uci_ex.sh get qcmap_lan.@profile[%d].bh_present_v6 | tr -d ' \n'", profile_idx);
  else
  {
    *qmi_err_num = QMI_ERR_INTERNAL_V01;
    return false;
  }

  if (!ExecuteSystemCmd(cmd, result, sizeof(result)))
  {
    LOG_MSG_ERROR("Error getting present V%d BH.\n ", ip_version,0,0);
    return false;
  }

  if((strncmp(result, BACKHAUL_WWAN_NAME, strlen(BACKHAUL_WWAN_NAME)) == 0) &&
     (strlen(result) == strlen(BACKHAUL_WWAN_NAME) ))
  {
    snprintf(ipv4config, QCMAP_MAX_STRING_LEN, "/tmp/ipv4config%d", wan_profile_handle);
    snprintf(ipv6config, QCMAP_MAX_STRING_LEN, "/tmp/ipv6config%d", wan_profile_handle);
  }
  else if (((strncmp(result, BACKHAUL_WLAN_NAME, strlen(BACKHAUL_WLAN_NAME)) == 0) &&
            (strlen(result) == strlen(BACKHAUL_WLAN_NAME) )) ||
          ((strncmp(result, BACKHAUL_ETH_NAME, strlen(BACKHAUL_ETH_NAME)) == 0) &&
            (strlen(result) == strlen(BACKHAUL_ETH_NAME) )) ||
          ((strncmp(result, BACKHAUL_USB_NAME, strlen(BACKHAUL_USB_NAME)) == 0) &&
            (strlen(result) == strlen(BACKHAUL_USB_NAME) )) ||
          ((strncmp(result, BACKHAUL_BT_NAME, strlen(BACKHAUL_BT_NAME)) == 0) &&
            (strlen(result) == strlen(BACKHAUL_BT_NAME) )))
  {
    strlcpy(if_name, result, QCMAP_MAX_IFACE_NAME_SIZE);
    snprintf(ipv4config, QCMAP_MAX_STRING_LEN, "/tmp/ipv4config.%s", if_name);
    snprintf(ipv6config, QCMAP_MAX_STRING_LEN, "/tmp/ipv6config.%s", if_name);
  }
  else
  {
    LOG_MSG_ERROR("Invalid Backhaul name found", 0, 0, 0);
    *qmi_err_num = QMI_ERR_INTERNAL_V01;
    return false;
  }

  /* Check if ipv4config/ipv6config file is present */
  if (ip_version == IP_V4)  return QCMAP_LAN_Client::CheckIfFileExists(ipv4config);
  if( ip_version == IP_V6)  return QCMAP_LAN_Client::CheckIfFileExists(ipv6config);

  *qmi_err_num = QMI_ERR_INTERNAL_V01;
  return false;
}

/*=====================================================================
  FUNCTION SetResetDHCPIgnoreOption
======================================================================*/
/*!
@brief
  - Sets or Resets DHCP ignore option on each of the bridge's

@return
  true - Success
  false - Failure

@note
  - Setting dhcp ignore to 1 to stop DHCP server functioning
  on bridge's.

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
boolean SetResetDHCPIgnoreOption
(
  boolean reset,
  const std::nullptr_t bridge_id = NULL
)
{
  char cmd[QCMAP_MAX_COMMAND_LEN] = {0};
  char result[QCMAP_MAX_SCAN_SIZE] = {0};
  char vid[QCMAP_MAX_SCAN_SIZE] = {0};
  char profile_num[QCMAP_MAX_SCAN_SIZE] = {0};


  if (bridge_id == NULL)
  {
    /* Get Number of VLAN's */
    snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "%s %s",
              UCI_GET_COMMAND, UCI_QUERY_NO_OF_VLAN_COMMAND);
    if (ExecuteSystemCmd((const char *)cmd, result,
                                           sizeof(result)))
    {
      /* Iterate through the num of vlan's */
      int num = atoi(result);

      /* Check if number of VLAN's is greater than zero */
      if (num <= 0)
      {
        /* Indicates no VLAN's configured */
        LOG_MSG_INFO1("No VLAN's configured", 0, 0, 0);
        return true;
      }
      /* Iterate through the VLAN's and check for PDN mappings */
      for (int i = 0; i < num; i++)
      {
        /* Get VLAN-ID */
        memset(vid, 0, QCMAP_MAX_SCAN_SIZE);
        memset(cmd, 0, QCMAP_MAX_COMMAND_LEN);
        snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "%s qcmap_lan.@vlan[%d].vlan_id",
              UCI_GET_COMMAND, i);
        if (ExecuteSystemCmd((const char *)cmd, vid,
                                           sizeof(vid)))
        {
          /* Two scenario's.
             1. If it returns a valid PDN number, indicates on-demand PDN is mapped.
             2. If it returns false, indicates no mapping.
             In either case, we need to set DHCP ignore option based on reset value
             If reset is true -> Set DHCP option
             If reset is false -> Reset DHCP option */
          if (reset)
          {
            LOG_MSG_ERROR("Setting DHCP ignore option to bridge: lan%d", atoi(vid), 0, 0);
            memset(cmd, 0, QCMAP_MAX_COMMAND_LEN);
            snprintf(cmd, QCMAP_MAX_COMMAND_LEN, " %s dhcp.lan%d.ignore=1",
                     UCI_SET_COMMAND, atoi(vid));
            ds_system_call(cmd, strlen(cmd));
          }
          else
          {
            LOG_MSG_ERROR("Resetting DHCP ignore option to bridge: lan%d", atoi(vid), 0, 0);
            memset(cmd, 0, QCMAP_MAX_COMMAND_LEN);
            snprintf(cmd, QCMAP_MAX_COMMAND_LEN, " %s dhcp.lan%d.ignore=0",
                     UCI_SET_COMMAND, atoi(vid));
            ds_system_call(cmd, strlen(cmd));
          }
        }
      }
      /* Commit uci */
      ExecuteUCICommit();

      /* Perform dnsmasq reload */
      memset(cmd, 0, QCMAP_MAX_COMMAND_LEN);
      snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "%s", DNSMASQ_RELOAD_COMMAND);
      ds_system_call(cmd, strlen(cmd));
    }
    else
    {
      LOG_MSG_ERROR("failed to retrieve no_of_vlan's", 0, 0, 0);
      return false;
    }
  }
  /*NOTE: Can implement setresetdhcpignoreoption based on a specific bridge_id in future
  using bridge_id optional parameter */
  return true;
} /* End SetResetDHCPIgnoreOption */

/*==========================================================================
FUNCTION GetEthNiCNameFromType()
===========================================================================*/
/** @ingroup GetEthNiCNameFromType

  Get ETH NIC config.

  @param[in]        qcmap_interface_type_enum

  @return
  TRUE -- Success \n
  FALSE -- Failure \n
*/
/*=========================================================================*/
boolean QCMAP_LAN_Client::GetEthNiCNameFromType
(
  qcmap_interface_type_enum  eth_nic_type,
  char* const eth_nic_name,
  uint32_t length
)
{
  char result[QCMAP_MAX_SCAN_SIZE] = {0};
  int32 index=INT_MIN;

  if ( QCMAP_INTERFACE_TYPE_ETH == eth_nic_type )
  {
    index = 0;
  }
  else if ( QCMAP_INTERFACE_TYPE_ETH_NIC2 == eth_nic_type )
  {
    index = 1;
  }

  if (INT_MIN != index)
  {
    // Get the ETH mode
    if ( QCMAP_LAN_Client::UciGetUtility(owrt_filename[0], "eth_nic_config", 0, "name", result, index) )
    {
      if (strlen(result) > length)
      {
        LOG_MSG_ERROR("Source string(%d) is longer than destination string(%d)", strlen(result), length, 0);
        return FALSE;
      }
      else
      {
        strlcpy(eth_nic_name, result, strlen(result)+1);
        LOG_MSG_INFO1("ethNiCName: %s for eth_nic_type %d result %s", eth_nic_name, eth_nic_type, result);
        return TRUE;
      }

    }
    else
    {
      LOG_MSG_ERROR("UciGetUtility failed. Check eth_nic_config from qcmap_lan",0,0,0);
      return FALSE;
    }
  }
  else
  {
    LOG_MSG_ERROR("Invalid eth_nic_type %d passed", eth_nic_type,0, 0);
    return FALSE;
  }
} /* GetEthNiCNameFromType */


/*==========================================================================
FUNCTION GetEthNiCTypeFromName()
===========================================================================*/
/** @ingroup qcmap_get_eth_nic_config

  Get ETH NIC config.

  @param[in]        char*

  @return
  TRUE -- Success \n
  FALSE -- Failure \n
*/
/*=========================================================================*/
qcmap_interface_type_enum  QCMAP_LAN_Client::GetEthNiCTypeFromName
(
  const char *eth_nic_name
)
{
  char result[QCMAP_MAX_SCAN_SIZE] = {0};
  uint32_t no_of_nics=0;
  uint32_t nic_index=0;
  qcmap_interface_type_enum ret_val;

  if(UciGetUtility(owrt_filename[0],"eth_config",0,"no_of_nics",result,0))
  {
    no_of_nics = atoi(result);
  }
  else
  {
    LOG_MSG_ERROR("UciGetUtility failed ",0,0,0);
    return QCMAP_INTERFACE_TYPE_ENUM_MAX_ENUM_VAL;
  }

  for(nic_index=0; nic_index<no_of_nics; nic_index++)
  {
    memset(result, 0, QCMAP_MAX_SCAN_SIZE);
    if(UciGetUtility(owrt_filename[0],"eth_nic_config",0,"name",result,nic_index))
    {
      if ( strncmp(result, eth_nic_name, strlen(result)) == 0 )
      {
        break;
      }
    }
    else
    {
      LOG_MSG_ERROR("UciGetUtility failed ",0,0,0);
      return QCMAP_INTERFACE_TYPE_ENUM_MAX_ENUM_VAL;
    }
  }

  switch (nic_index)
  {
    case 0:
      ret_val = QCMAP_INTERFACE_TYPE_ETH;
      break;

    case 1:
      ret_val = QCMAP_INTERFACE_TYPE_ETH_NIC2;
      break;

    default:
      LOG_MSG_ERROR("Invalid iface_type=%d", nic_index, 0,0);
      ret_val = QCMAP_INTERFACE_TYPE_ENUM_MAX_ENUM_VAL;
  }

  return ret_val;

}/* GetEthNiCTypeFromName */

/*==========================================================================
FUNCTION GetEthernetNicConfig()
===========================================================================*/
/** @ingroup qcmap_get_eth_nic_config

  Get ETH NIC config.

  @param[in]      qcmap_eth_config_t* eth_config.

  @return
  TRUE -- Success \n
  FALSE -- Failure \n
*/
/*=========================================================================*/

boolean QCMAP_LAN_Client::GetEthernetNicConfig
(
  qcmap_eth_config_t *eth_config,
  qmi_error_type_v01 *qmi_err_num
)
{
  char result[QCMAP_MAX_SCAN_SIZE] = {0};
  int type,mode;
  int num_of_eth_nics = 0;
  int num_of_macsec_nics = 0;

  if(eth_config == NULL)
  {
    LOG_MSG_ERROR("NULL pointer passed",0,0,0);
    *qmi_err_num = QMI_ERR_INVALID_ARG_V01;
    return false;
  }

  eth_config->is_eth_nics_config_valid = true;

  if(UciGetUtility(owrt_filename[0],"eth_config",0,"mode",result,0))
  {
    mode = atoi(result);
    switch(mode)
    {
      case 0:
         eth_config->mode = QCMAP_ETHERNET_LAN_ROUTER;
         LOG_MSG_INFO1("ETH NIC mode is set to : %d",eth_config->mode,0,0);
         break;
      case 1:
          eth_config->mode = QCMAP_ETHERNET_WAN_ROUTER;
          LOG_MSG_INFO1("ETH NIC mode is set to : %d",eth_config->mode,0,0);
          break;
      case 2:
          eth_config->mode = QCMAP_ETHERNET_WAN_LAN_ROUTER;
          LOG_MSG_INFO1("ETH NIC mode is set to : %d",eth_config->mode,0,0);
          break;
      default:
          LOG_MSG_ERROR("Invalid mode type received: %d",eth_config->mode,0,0);
          *qmi_err_num = QMI_ERR_INVALID_ARG_V01;
          return false;
    }

  }
  else
  {
    LOG_MSG_ERROR("UciGetUtility failed ",0,0,0);
    *qmi_err_num = QMI_ERR_INTERNAL_V01;
    return false;
  }

  memset(result, 0, QCMAP_MAX_SCAN_SIZE);
  if(UciGetUtility(owrt_filename[0],"eth_config",0,"no_of_nics",result,0))
  {
    eth_config->no_of_nics = atoi(result);
  }
  else
  {
    LOG_MSG_ERROR("UciGetUtility failed ",0,0,0);
    *qmi_err_num = QMI_ERR_INTERNAL_V01;
    return false;
  }

  for(uint32_t i=0; i<QCMAP_MAX_ETH_NICS; i++)
  {
    memset(result, 0, QCMAP_MAX_SCAN_SIZE);
    if(UciGetUtility(owrt_filename[0],"eth_nic_config",0,"enabled",result,i))
    {
      if(atoi(result) == FALSE)
      {
        continue;
      }
    }
    else
    {
      LOG_MSG_ERROR("UciGetUtility failed ",0,0,0);
      continue;
    }

    memset(result, 0, QCMAP_MAX_SCAN_SIZE);
    if(UciGetUtility(owrt_filename[0],"eth_nic_config",0,"name",result,i))
    {
      eth_config->eth_nic_config[num_of_eth_nics].eth_nic_type = QCMAP_LAN_Client::GetEthNiCTypeFromName(result);
      strlcpy(eth_config->eth_nic_config[num_of_eth_nics].eth_iface_name, result, QCMAP_MAX_IFACE_NAME_SIZE);
    }
    else
    {
      LOG_MSG_ERROR("UciGetUtility failed ",0,0,0);
      *qmi_err_num = QMI_ERR_INTERNAL_V01;
      return false;
    }

    memset(result, 0, QCMAP_MAX_SCAN_SIZE);
    if(UciGetUtility(owrt_filename[0],"eth_nic_config",0,"type",result,i))
    {
      type = atoi(result);
      switch(type)
      {
        case 0:
          eth_config->eth_nic_config[num_of_eth_nics].eth_nic_nw_type = QCMAP_ETHERNET_LAN_TYPE;
          break;
        case 1:
          eth_config->eth_nic_config[num_of_eth_nics].eth_nic_nw_type = QCMAP_ETHERNET_WAN_TYPE;
          break;
        default:
            LOG_MSG_ERROR("Invalid eth_nic type",0,0,0);
            *qmi_err_num = QMI_ERR_INVALID_ARG_V01;
            return false;
      }
    }
    else
    {
      LOG_MSG_ERROR("UciGetUtility failed ",0,0,0);
      *qmi_err_num = QMI_ERR_INTERNAL_V01;
      return false;
    }

    memset(result, 0, QCMAP_MAX_SCAN_SIZE);
    if(UciGetUtility(owrt_filename[0],"eth_nic_config",0,"macsec_enabled",result,i))
    {
      switch(atoi(result))
      {
        case QCMAP_MSGR_CONFIG_DISABLE:
            eth_config->macsec_nic_config[num_of_macsec_nics].state = QCMAP_MSGR_CONFIG_DISABLE;
            break;
        case QCMAP_MSGR_CONFIG_ENABLE:
            eth_config->macsec_nic_config[num_of_macsec_nics].state = QCMAP_MSGR_CONFIG_ENABLE;
            break;
        case QCMAP_MSGR_CONFIG_RESTART:
            eth_config->macsec_nic_config[num_of_macsec_nics].state = QCMAP_MSGR_CONFIG_RESTART;
            break;
        default:
            break;
      }

      if(eth_config->macsec_nic_config[num_of_macsec_nics].state == QCMAP_MSGR_CONFIG_ENABLE)
      {
        memset(result, 0, QCMAP_MAX_SCAN_SIZE);
        if(UciGetUtility(owrt_filename[0],"eth_nic_config",0,"macsec_name",result,i))
        {
          strlcpy(eth_config->macsec_nic_config[num_of_macsec_nics].macsec_iface_name, result, QCMAP_MAX_IFACE_NAME_SIZE);
          strlcpy(eth_config->macsec_nic_config[num_of_macsec_nics].eth_nic_iface_name,
              eth_config->eth_nic_config[i].eth_iface_name, QCMAP_MAX_IFACE_NAME_SIZE);
        }

        memset(result, 0, QCMAP_MAX_SCAN_SIZE);
        if(UciGetUtility(owrt_filename[0],"eth_nic_config",0,"macsec_type",result,i))
        {
          if(atoi(result) == QCMAP_MSGR_MACSEC_MODE_SUPPLICANT)
          {
            eth_config->macsec_nic_config[num_of_macsec_nics].macsec_mode = QCMAP_MSGR_MACSEC_MODE_SUPPLICANT;
          }
          else
          {
            eth_config->macsec_nic_config[num_of_macsec_nics].macsec_mode = QCMAP_MSGR_MACSEC_MODE_AUTHENTICATOR;
          }
        }
        else
        {
          LOG_MSG_ERROR("UciGetUtility failed ",0,0,0);
          *qmi_err_num = QMI_ERR_INTERNAL_V01;
          return false;
        }
        num_of_macsec_nics++;
      }
    }
    num_of_eth_nics++;
    if(num_of_eth_nics >= QCMAP_MAX_ETH_NICS)
    {
      break;
    }
  }
  eth_config->no_of_macsec_nics = num_of_macsec_nics;
  if(num_of_macsec_nics > 0)
  {
    eth_config->is_macsec_nic_config_valid = TRUE;
  }

  return true;
}

/*==========================================================================
FUNCTION SetEthernetNicConfig()
===========================================================================*/
/** @ingroup qcmap_get_eth_nic_config

  Set Ethernet NIC config.

  @param[in]      qcmap_eth_config_t eth_config.

  @return
  TRUE -- Success \n
  FALSE -- Failure \n
*/
/*=========================================================================*/

boolean QCMAP_LAN_Client::SetEthernetNicConfig
(
  const qcmap_eth_config_t eth_config,
  qmi_error_type_v01 *qmi_err_num
)
{
  char cmd[QCMAP_MAX_COMMAND_LEN] = {0};
  char eth_nic_name[QCMAP_MAX_SCAN_SIZE] = {0};
  int nic_number = 0;

  switch(eth_config.mode)
  {
    case QCMAP_ETHERNET_LAN_ROUTER:
      LOG_MSG_INFO1("ETH NIC mode: %d",QCMAP_ETHERNET_LAN_ROUTER,0,0);
      break;

    case QCMAP_ETHERNET_WAN_ROUTER:
      LOG_MSG_INFO1("ETH NIC mode: %d",QCMAP_ETHERNET_WAN_ROUTER,0,0);
      break;

    case QCMAP_ETHERNET_WAN_LAN_ROUTER:
      LOG_MSG_INFO1("ETH NIC mode: %d",QCMAP_ETHERNET_WAN_LAN_ROUTER,0,0);
      break;

    default:
      LOG_MSG_ERROR("ETH NIC mode invalid : %d",eth_config.mode,0,0);
      *qmi_err_num = QMI_ERR_INVALID_ARG_V01;
      return false;
  }
  ds_system_call(cmd, strlen(cmd));
  for(uint32_t i=0; i<eth_config.no_of_nics; i++)
  {
    nic_number = util_get_eth_nic_number(eth_config.eth_nic_config[i].eth_iface_name);
    /*handle invalid nic_number*/
    if (nic_number == QCMAP_LAN_INVALID) 
    {
      LOG_MSG_ERROR("nic number is invalid",0,0,0);
      *qmi_err_num = QMI_ERR_INTERNAL_V01;
      return false;
    }

    memset(cmd, 0, QCMAP_MAX_COMMAND_LEN);
    if(i == 0)
    {
      QCMAP_LAN_CLIENT_RUN_COMMANDS("%s %s %d %d %d",
                      TETHERING_CONFIG_FILE,
                      SET_ETH_CONFIG,
                      eth_config.mode,
                      eth_config.no_of_nics,
                      (nic_number));
    }
    QCMAP_LAN_CLIENT_RUN_COMMANDS("%s %s %s %d %d",
                      TETHERING_CONFIG_FILE,
                      SET_ETH_TYPE,
                      eth_config.eth_nic_config[i].eth_iface_name,
                      eth_config.eth_nic_config[i].eth_nic_nw_type,
                      (nic_number));
  }
  for(uint32_t i=0; i<eth_config.no_of_macsec_nics; i++)
  {
    if(eth_config.mode != QCMAP_ETHERNET_LAN_ROUTER){
       LOG_MSG_ERROR("Macsec can only be enabled when eth is in LAN mode",0,0,0);
       break;
    }
    nic_number = util_get_eth_nic_number(eth_config.macsec_nic_config[i].eth_nic_iface_name);
    memset(cmd, 0, QCMAP_MAX_COMMAND_LEN);
    switch(eth_config.macsec_nic_config[i].state)
    {
      case QCMAP_CONFIG_DISABLE:
        QCMAP_LAN_CLIENT_RUN_COMMANDS("%s %s %d",
                    TETHERING_CONFIG_FILE,
                    DISABLE_MACSEC,
                    nic_number);
        break;
      case QCMAP_CONFIG_ENABLE:
          QCMAP_LAN_CLIENT_RUN_COMMANDS("%s %s %s %d %d",
                    TETHERING_CONFIG_FILE,
                    ENABLE_MACSEC,
                    eth_config.macsec_nic_config[i].macsec_iface_name,
                    eth_config.macsec_nic_config[i].macsec_mode,
                    nic_number);
        break;
      case QCMAP_CONFIG_RESTART:
          QCMAP_LAN_CLIENT_RUN_COMMANDS("%s %s %d",
                    TETHERING_CONFIG_FILE,
                    STOP_MACSEC,
                    nic_number);
          QCMAP_LAN_CLIENT_RUN_COMMANDS("%s %s %d",
                    TETHERING_CONFIG_FILE,
                    START_MACSEC,
                    nic_number);
        break;
    }
  }
  return true;
}/*End of SetEthernetNicConfig */

/*===========================================================================
FUNCTION CountNumOfSetBits()
===========================================================================*/
/** @ingroup section_CountNumOfSetBits

  Count Number Of Set Bits in a Number

  @param[in] uint32_t  number

  @return
  int -> num of set bits

  @dependencies
  QCMobileAP must be enabled. @newpage
*/
/*=========================================================================*/
int CountNumOfSetBits(uint32_t num)
{
  int count=0;
  while(num > 0)
  {
    num = (num & (num-1));
    count++;
  }
  return count;
}

/*===========================================================================
FUNCTION SetLANConfig()
===========================================================================*/
/** @ingroup section_SetLANConfig

  Sets LAN config

  @param[in] qcmap_lan_config   *lan_config

  @return
  TRUE -- Success \n
  FALSE -- Failure \n

  @dependencies
  QCMobileAP must be enabled. @newpage
*/
/*=========================================================================*/
boolean QCMAP_LAN_Client::SetLANConfig
(
  qcmap_lan_config *lan_config,
  qmi_error_type_v01 *qmi_err_num
)
{
  char command[QCMAP_MAX_COMMAND_LEN] = {0};
  bool set_default_config=false;
  char result[QCMAP_MAX_SCAN_SIZE] = {0};
  int16_t bridge_id;
  uint32_t wan_profile_id;
  uint32_t gw_ip_third_octet;
  uint32_t netmask_third_octet;
  bool enable_status;
  int num_of_set_bits_in_subnet = 0;
  int num_of_host_bits = 0;
  int max_number_of_clients = 0;

  QCMAP_LOG_FUNC_ENTRY();

  if ( lan_config == NULL || lan_config->gw_ip == 0 )
  {
    LOG_MSG_ERROR("Invalid lan_config/gw_ip passed ",0,0,0);
    *qmi_err_num = QMI_ERR_INVALID_ARG_V01;
    return false;
  }

  /* Check if requested subnet is to support <255 clients. For example 255.255.255.0
     can serve any ip range.If not, then subnet mask will span more than
     last octect of the gw ip, so need to be even number on 3rd octect to
     ensure Gateway IP is out of the dhcp range */
  gw_ip_third_octet   = (lan_config->gw_ip >> 8)%256 ;
  netmask_third_octet = (lan_config->netmask >> 8)%256 ;

  /* Check if third octet in ip address is odd */
  if ( (netmask_third_octet != 255) && ((gw_ip_third_octet%2) != 0) )
  {
    /* To support 280 clients we are changing netmask to 255.255.254.0 from
       255.255.255.0. Dnsmasq calcualtes the minimum offset ip that can be
       leased to client, when gw_ip is odd it is resulting in gw_ip to fall in
       within range of dhcp_start & dhcp_end.
       Example : gw_ip = 192.168.3.1, dhcp_start =100, dhcp_limit=280
       Calculated dhcp_ranges = 192.168.2.100 - 192.168.3.123.
       To avoid this we need to make sure third octet in gw_ip is even */
    LOG_MSG_ERROR("To support more than 255 clients we cannot have gw_ip with odd number as third octet",0,0,0);
    *qmi_err_num = QMI_ERR_INTERNAL_V01;
    return false;
  }

  num_of_set_bits_in_subnet = CountNumOfSetBits(lan_config->netmask);
  num_of_host_bits = ( 32 - num_of_set_bits_in_subnet );
  max_number_of_clients = (1 << num_of_host_bits);

  LOG_MSG_INFO1("num_of_host_bits = %d , max_clients = %d ", num_of_host_bits, max_number_of_clients, 0);

  if ( lan_config->enable_dhcp == 1 )
  {
    /* Check if gw_ip & dhcp_start_ip, gw_ip & dhcp_end_ip are in same subnet &&
       Check if gw_ip is not in range of dhcp_start_ip and dhcp_end_ip*/
    if ( ((lan_config->gw_ip & lan_config->netmask) !=
          (lan_config->dhcp_config.dhcp_start_ip & lan_config->netmask))||
          ((lan_config->gw_ip & lan_config->netmask) !=
          (lan_config->dhcp_config.dhcp_end_ip & lan_config->netmask))||
          (lan_config->dhcp_config.dhcp_start_ip <= lan_config->gw_ip &&
           lan_config->gw_ip <= lan_config->dhcp_config.dhcp_end_ip) )
    {
      LOG_MSG_INFO1("DHCP Start/End addr provided is invalid. Setting it to default", 0, 0, 0);
      lan_config->dhcp_config.dhcp_start_ip = DEFAULT_DHCP_START_VALUE;
      lan_config->dhcp_config.dhcp_end_ip = MAX_CLIENT;
      lan_config->netmask = DEFAULT_SUBNET_INT;
    }
    else
    {
      lan_config->dhcp_config.dhcp_end_ip = lan_config->dhcp_config.dhcp_end_ip -
                                            lan_config->dhcp_config.dhcp_start_ip+1;
      lan_config->dhcp_config.dhcp_start_ip = lan_config->dhcp_config.dhcp_start_ip%max_number_of_clients;
    }
  }

  if (!QCMAP_LAN_Client::GetBridgeVLANContext(&bridge_id))
  {
    LOG_MSG_ERROR("Failed to get the current bridge context", 0, 0, 0);
    *qmi_err_num = QMI_ERR_INTERNAL_V01;
    return false;
  }

  /*  Check if IPPT is enabled on this bridge_context.In case of IPPT with NAT-FCD.
   IPPT without NAT - Both specific and FCD. In above cases
   we will not allow SetLANConfig as only public range exist */
  if(QCMAP_LAN_Client::GetIPPTStatus(&enable_status, bridge_id, qmi_err_num))
  {
    if(enable_status)
    {
      LOG_MSG_INFO1("Rejecting SetLANConfig as IPPT is active",0,0,0);
      return false;
    }
  }
  else
  {
    LOG_MSG_ERROR("Failed to get IPPT Status", 0, 0, 0);
    return false;
  }

  snprintf(command, QCMAP_MAX_COMMAND_LEN,"%s %s %u %u %u %u %d %u %u", VLAN_CONFIG, SET_LAN_CONFIG,
           lan_config->gw_ip, lan_config->netmask, lan_config->enable_dhcp,
           lan_config->dhcp_config.dhcp_start_ip, lan_config->dhcp_config.dhcp_end_ip,
           lan_config->dhcp_config.lease_time,bridge_id);
  ds_system_call(command, strlen(command));

  return true;
}

/*===========================================================================
FUNCTION GetActiveLANConfig()
===========================================================================*/
/** @ingroup section_GetActiveLANConfig

  Get LAN config

  @param[in] qcmap_lan_config   *lan_config
  @param[in].qmi_error_type_v01    *qmi_err_num

  @return
  TRUE -- Success \n
  FALSE -- Failure \n

  @dependencies
  QCMobileAP must be enabled. @newpage
  Current bridge context is stored in the qcmap_lan.
  Current bridge context will be used for this API.
*/
/*=========================================================================*/
boolean QCMAP_LAN_Client::GetActiveLANConfig
(
  qcmap_lan_config *lan_config,
  qmi_error_type_v01 *qmi_err_num
)
{
  char result[QCMAP_MAX_SCAN_SIZE] = {0};
  char command[QCMAP_MAX_COMMAND_LEN] = {0};
  int dhcp_limit;
  in_addr addr;
  int16_t bridge_id;
  char lan_interface[QCMAP_MAX_COMMAND_LEN] = {0};
  char dev_type[QCMAP_MAX_SCAN_SIZE] = {0};
  char file_path[QCMAP_MAX_SCAN_SIZE] = {0};
  bool is_ippt_enabled_on_bridge = false;
  int profile_index = -1, profile_num = -1, active_ippt = -1;
  qcmap_lan_client_feature_mode_config feature_mode_config;
  memset(&feature_mode_config, 0, sizeof(qcmap_lan_client_feature_mode_config));
  uint64_t features = 0;

  if ( lan_config == NULL )
  {
    LOG_MSG_ERROR("Invalid lan_config passed ",0,0,0);
    *qmi_err_num = QMI_ERR_INVALID_ARG_V01;
    return false;
  }

  if (!QCMAP_LAN_Client::GetBridgeVLANContext(&bridge_id))
  {
    LOG_MSG_ERROR("Failed to get the current bridge context", 0, 0, 0);
    *qmi_err_num = QMI_ERR_INTERNAL_V01;
    return false;
  }

  /* Send all information to script */
  if (bridge_id != 0)
  {
    snprintf(lan_interface, QCMAP_MAX_COMMAND_LEN, "%s%u","lan",bridge_id);
  }
  else
  {
    snprintf(lan_interface, QCMAP_MAX_COMMAND_LEN, "%s","lan");
  }

  /* Check if IPPT is enabled on this bridge_context.In case of IPPT with NAT-FCD.
     IPPT without NAT - Both specific and FCD. We will update is_ippt_enabled_on_bridge value to 1.
     In above cases fetching lan_config has to be done differently from regular fetch */
  if (QCMAP_LAN_Client::CheckIfVLANMappedToIPPTPDN(bridge_id, &profile_index, &profile_num, qmi_err_num))
  {
    LOG_MSG_INFO1("IPPT is configured on bridge(=%d) and pdn (=%d)", bridge_id, profile_num, 0);
    active_ippt = GetActiveIPPT(profile_index);
    if ( active_ippt == 1 )
    {
      LOG_MSG_INFO1("IPPT is active on bridge(=%d) and pdn (=%d)", bridge_id, profile_num, 0);
      if ( QCMAP_LAN_Client::GetFeatureMode(&features, &feature_mode_config, qmi_err_num) )
      {
        /* Get Device type */
        memset(command, 0, QCMAP_MAX_COMMAND_LEN);
        memset(result, 0, QCMAP_MAX_SCAN_SIZE);
        snprintf(command, QCMAP_MAX_COMMAND_LEN, "%s qcmap_lan.@profile[%d].ippt_device_type",
                 UCI_GET_COMMAND, profile_index);
        if ( ExecuteSystemCmd((const char *)command, result,
                         sizeof(result)) )
        {
           strlcpy(dev_type, result, strlen(result));
           LOG_MSG_INFO1("Device_type: %s!",dev_type, 0, 0);
           if ( (feature_mode_config.ip_passthrough_feature_mode ==
                  IP_PASSTHROUGH_MODE_WITH_NAT  &&
                  (strncmp(dev_type,"Any", QCMAP_MAX_STRING_LEN) == 0))
                  ||(feature_mode_config.ip_passthrough_feature_mode ==
                     IP_PASSTHROUGH_MODE_WITHOUT_NAT) )
           {
             LOG_MSG_INFO1("IPPT is active with Feature_mode (=%d) and dev_type(=%s)",
                            feature_mode_config.ip_passthrough_feature_mode, dev_type, 0);
             is_ippt_enabled_on_bridge = true;
           }
         }
      }
    }
  }
  if ( is_ippt_enabled_on_bridge )
  {
    snprintf(file_path, QCMAP_MAX_COMMAND_LEN, "/tmp/ipv4config%d", profile_num);
    if ( !CheckIfFileExists(file_path) )
    {
      LOG_MSG_ERROR("File doesn't exist : %s",file_path,0,0);
      *qmi_err_num = QMI_ERR_INTERNAL_V01;
      return false;
    }
    memset(command, 0, QCMAP_MAX_COMMAND_LEN);
    memset(result, 0, QCMAP_MAX_SCAN_SIZE);
    snprintf(command, QCMAP_MAX_COMMAND_LEN,
             "cat %s | grep -w GATEWAY | awk -F '\"' '{print $2}'", file_path);
    if(!ExecuteSystemCmd((const char *)command,result,sizeof(result)))
    {
      LOG_MSG_ERROR("Failed to execute command : %s",command,0,0);
      *qmi_err_num = QMI_ERR_INTERNAL_V01;
      return false;
    }
    if (strlen(result) > 0) 
    {
      result[strlen(result) - 1] = '\0'; 
    }
    if ( !(inet_aton(result, &addr) <= 0) )
    {
      lan_config->gw_ip = ntohl(addr.s_addr);
    }
    else
    {
      LOG_MSG_ERROR("Invalid gw_ip address obtained",0,0,0);
      *qmi_err_num = QMI_ERR_INTERNAL_V01;
      return false;
    }
  }
  /* Get gw_ip from network config */
  else if ( QCMAP_LAN_Client::UciGetUtility(owrt_filename[3], lan_interface, 1, LAN_IP, result) )
  {
    if ( !(inet_aton(result, &addr) <= 0) )
    {
      lan_config->gw_ip = ntohl(addr.s_addr);
    }
    else
    {
      LOG_MSG_ERROR("Invalid gw_ip address obtained",0,0,0);
      *qmi_err_num = QMI_ERR_INTERNAL_V01;
      return false;
    }
  }
  else
  {
    LOG_MSG_ERROR("Failed to get gw_ip",0,0,0);
    *qmi_err_num = QMI_ERR_INTERNAL_V01;
    return false;
  }
  /* Get netmask from network config */
  if ( is_ippt_enabled_on_bridge )
  {
    if ( QCMAP_LAN_Client::UciGetUtility(owrt_filename[4], lan_interface, 1, LAN_NETMASK, result) )
    {
      if ( !(inet_aton(result, &addr) <= 0) )
      {
        lan_config->netmask = ntohl(addr.s_addr);
      }
      else
      {
        LOG_MSG_ERROR("Invalid netmask address obtained",0,0,0);
        *qmi_err_num = QMI_ERR_INTERNAL_V01;
        return false;
      }
    }
  }
  else if ( QCMAP_LAN_Client::UciGetUtility(owrt_filename[3], lan_interface, 1, LAN_NETMASK, result) )
  {
    if ( !(inet_aton(result, &addr) <= 0) )
    {
      lan_config->netmask = ntohl(addr.s_addr);
    }
    else
    {
      LOG_MSG_ERROR("Invalid netmask address obtained",0,0,0);
      *qmi_err_num = QMI_ERR_INTERNAL_V01;
      return false;
    }
  }
  else
  {
    LOG_MSG_ERROR("Failed to get netmask",0,0,0);
    *qmi_err_num = QMI_ERR_INTERNAL_V01;
    return false;
  }
  /* Get dhcp_enable */
  if ( QCMAP_LAN_Client::UciGetUtility(owrt_filename[4], lan_interface, 1, DHCP_IGNORE, result) )
  {
    /* If dhcp is enabled get dhcp_start and dhcp_limit values */
    int dhcp_ignore=atoi(result);
    if ( dhcp_ignore == 0 )
    {
      lan_config->enable_dhcp = 1;
      if ( QCMAP_LAN_Client::UciGetUtility(owrt_filename[4], lan_interface, 1, DHCP_START, result) )
      {
        if ( !(inet_aton(result, &addr) <= 0) )
        {
          lan_config->dhcp_config.dhcp_start_ip = ntohl(addr.s_addr);
        }
        else
        {
          LOG_MSG_ERROR("Invalid dhcp_start address obtained",0,0,0);
          *qmi_err_num = QMI_ERR_INTERNAL_V01;
          return false;
        }
        /* Update dhcp_ip by appending dhcp_start value to subnet of gw_ip
           When is_ippt_enabled_on_bridge is true dhcp start is already updated above*/
        if ( !is_ippt_enabled_on_bridge )
        {
          lan_config->dhcp_config.dhcp_start_ip = ((lan_config->gw_ip &
                                                  lan_config->netmask) |
                                                  lan_config->dhcp_config.dhcp_start_ip);
        }
      }
      else
      {
        LOG_MSG_ERROR("Failed to get dhcp_start value",0,0,0);
        *qmi_err_num = QMI_ERR_INTERNAL_V01;
        return false;
      }
      /* Get dhcp_limit and calculate dhcp_end_ip */
      if ( QCMAP_LAN_Client::UciGetUtility(owrt_filename[4], lan_interface, 1, DHCP_LIMIT, result, 0) )
      {
        dhcp_limit=atoi(result);
        /* Update dhcp_end value */
        if ( !is_ippt_enabled_on_bridge )
        {
          lan_config->dhcp_config.dhcp_end_ip = lan_config->dhcp_config.dhcp_start_ip +
                                              dhcp_limit -1;
        }
        else
        {
          lan_config->dhcp_config.dhcp_end_ip = lan_config->dhcp_config.dhcp_start_ip;
        }
      }
      else
      {
        LOG_MSG_ERROR("Failed to get dhcp_limit value",0,0,0);
        *qmi_err_num = QMI_ERR_INTERNAL_V01;
        return false;
      }
      if ( QCMAP_LAN_Client::UciGetUtility(owrt_filename[4], lan_interface, 1, DHCP_LEASETIME, result, 0) )
      {
        if(strlen(result) > 0)
        {  char lease_time_unit = result[strlen(result)-1];
           char* rest = result;
           char *lease_time;

            if (lease_time_unit == 'h')
            {
                // Lease time is in hours. Convert to seconds
                lease_time = strtok_r(result, "h",&rest);
                if(lease_time != NULL)
                {
                  lan_config->dhcp_config.lease_time = atoi(lease_time)*60*60;
                }
            }
            else
            {
              lan_config->dhcp_config.lease_time = atoi(result);
            }
       }
      }
      else
      {
        LOG_MSG_ERROR("Failed to get dhcp_lease_time value",0,0,0);
        *qmi_err_num = QMI_ERR_INTERNAL_V01;
        return false;
      }
    }
  }
  else
  {
    LOG_MSG_ERROR("Failed to get dhcp_ignore_value",0,0,0);
    *qmi_err_num = QMI_ERR_INTERNAL_V01;
    return false;
  }
  return true;
}

/*===========================================================================
FUNCTION CheckAndActivateIPCollisionFromBridgeId()
===========================================================================*/
/** @ingroup section_CheckAndActivateIPCollision

  Checks for the ip collision and activates the configuration from bridge id

  @return
  TRUE -- Success \n
  FALSE -- Failure \n

  @dependencies
  QCMobileAP must be enabled. @newpage
*/
/*=========================================================================*/
boolean QCMAP_LAN_Client::CheckAndActivateIPCollisionFromBridgeId()
{
  char        result[QCMAP_MAX_SCAN_SIZE] = {0};
  int32_t     wan_profile_id              = INVALID_PROFILE_HANDLE;
  int16_t     bridge_id                   = INVALID_BRIDGE_ID;

  if (!QCMAP_LAN_Client::GetBridgeVLANContext(&bridge_id))
  {
    LOG_MSG_ERROR("Failed to get the current bridge context", 0, 0, 0);
    return false;
  }

  if (bridge_id == INVALID_BRIDGE_ID)
  {
    LOG_MSG_ERROR("Invalid bridge id", 0, 0, 0);
    return false;
  }

  // for default bridge vlan id is 0 and wan_profile_id is 1
  memset(result,0,QCMAP_MAX_SCAN_SIZE);

  if (bridge_id >= 1)
  {
    if ( QCMAP_LAN_Client::UciGetUtility(owrt_filename[0],
                                         "vlan",
                                         0,
                                         "wwan_profile_num",
                                         result,
                                         GetVlanIndex(bridge_id)) )
    {
      wan_profile_id = atoi(result);
    }
  }
  // Default bridge is always mapped to wan profile
  else if(0 == bridge_id)
  {
    wan_profile_id = GetDefaultProfilefromUCI();
  }

  // This condition ensures to not check for the address conflict in case
  // VLAN is not yet mapped to any PDN
  if (INVALID_PROFILE_HANDLE != wan_profile_id)
  {
    // Check for the address conflict
    QCMAP_LAN_CLIENT_RUN_COMMANDS("%s %s %u", IP_COLLISION_FILE,
             QCMAP_CHECK_ADDRESS_CONFLICT_FROM_BRIDGE_ID, bridge_id);

    return QCMAP_LAN_Client::CheckAndActivateIPCollision(wan_profile_id);
  }

  return true;
} /* CheckAndActivateIPCollisionFromBridgeId() */


/*===========================================================================
FUNCTION ActivateIPCollision()
===========================================================================*/
/** @ingroup section_ActivateIPCollision

  Checks for the ip collision and activates the configuration

  @return
  TRUE -- Success \n
  FALSE -- Failure \n

  @dependencies
  QCMobileAP must be enabled. @newpage
*/
/*=========================================================================*/
boolean QCMAP_LAN_Client::CheckAndActivateIPCollision(
  uint32_t    wan_profile_id
)
{
  char        result[QCMAP_MAX_SCAN_SIZE] = {0};
  int16_t     profile_idx                 = QCMAP_LAN_INVALID;
  int16_t     ip_collision_active         = QCMAP_LAN_INVALID;

  /* Get profile_idx from qcmap_lan */
  profile_idx = GetProfileIndex(wan_profile_id);

  if (profile_idx != QCMAP_LAN_INVALID)
  {
    if ( QCMAP_LAN_Client::UciGetUtility( owrt_filename[0], "profile", 0,
                                          "ip_collision_active", result, profile_idx) )
    {
      ip_collision_active = atoi(result);
    }
    else
    {
      // if ip_collision_active is not set then uci option is not present
      LOG_MSG_INFO1("ip_collision status %d [1:Active, 0:Inactive, -1: option not present/Unable to retreive]",ip_collision_active, 0, 0);
      return true;
    }
  }

  LOG_MSG_INFO1("ip_collision_active [1:Active, 0:Inactive] %d", ip_collision_active, 0, 0);

  if (1 == ip_collision_active)
  {
    // This flag is reset after the call setup from ip_collision.sh
    if (!QCMAP_LAN_Client::UciSetUtility(owrt_filename[0], "profile", 0,
        "ip_collision_enable_in_progress", 1,profile_idx) )
    {
      return false;
    }

  }

    /* Perform rmnet update */
    if (!UpdateRmnetFile(RMNET_BRING_UP_CMD, wan_profile_id, BACKHAUL_V4))
    {
      LOG_MSG_ERROR("Failed to update v4 RMNET!", 0, 0, 0);
      return false;
    }
  return true;
} /* ActivateIPCollision() */


/*===========================================================================
FUNCTION ActivateLAN()
===========================================================================*/
/** @ingroup section_ActivateLAN

  Activates LAN

  @return
  TRUE -- Success \n
  FALSE -- Failure \n

  @dependencies
  QCMobileAP must be enabled. @newpage
*/
/*=========================================================================*/
boolean QCMAP_LAN_Client::ActivateLAN
(
  const uint32_t profile_handle,
  qmi_error_type_v01 *qmi_err_num
)
{
  char command[QCMAP_MAX_COMMAND_LEN] = {0};
  int16_t bridge_id;
  bool enable_status = false;

  LOG_MSG_INFO1("Entering ActivateLAN ",0,0,0);

  if (!QCMAP_LAN_Client::GetBridgeVLANContext(&bridge_id))
  {
    LOG_MSG_ERROR("Failed to get the current bridge context", 0, 0, 0);
    *qmi_err_num = QMI_ERR_INTERNAL_V01;
    return false;
  }

  /* Check if IPPT is enabled on this bridge_context. In case of IPPT with NAT-FCD.
   IPPT without NAT - Both specific and FCD. In above cases
   we will not allow activate lan as only public range exists and setlanconfig is not supported */
  if(QCMAP_LAN_Client::GetIPPTStatus(&enable_status, bridge_id, qmi_err_num))
  {
    if(enable_status)
    {
      LOG_MSG_INFO1("Rejecting ActivateLAN as IPPT is active",0,0,0);
      *qmi_err_num = QMI_ERR_INCOMPATIBLE_STATE_V01;
      return false;
    }
  }
  else
  {
    LOG_MSG_ERROR("Failed to get IPPT Status", 0, 0, 0);
    *qmi_err_num = QMI_ERR_INTERNAL_V01;
    return false;
  }

  /* Check whether the user provided current lan config is same as running/active lan config
   * on the device and proceed to activate lan only when both are different */
  if(!QCMAP_LAN_Client::IsLanCfgUpdated())
  {
    LOG_MSG_INFO1("There is no change in the LAN Config",0,0,0);
    return true;
  }
  LOG_MSG_INFO1("Active LAN config and Current LAN config are mismatched",0,0,0);

/*
 * Delete lease for RNDIS client in case pool might have changed.
 */

  QCMAP_LAN_CLIENT_RUN_COMMANDS("%s %s", VLAN_CONFIG, DELETE_DHCP_LEASE_ENTRY);

  if( !(DeleteOldDHCPReservRecord(qmi_err_num)) )
  {
    LOG_MSG_ERROR("Failed to delete OLD DHCP reservations",0,0,0);
    *qmi_err_num = QMI_ERR_INTERNAL_V01;
    return false;
  }

  /* Add dhcp reservation recored stored in qcmap_lan to dhcp config file*/
  snprintf(command, QCMAP_MAX_COMMAND_LEN,"%s %s", VLAN_CONFIG,
    ACTIVATE_DHCP_RECORDS_ON_ACTIVATE_LAN);
  ds_system_call(command, strlen(command));

  snprintf(command, QCMAP_MAX_COMMAND_LEN,"%s %s %d", VLAN_CONFIG,
                       SET_LAN_CONFIG_ON_ACTIVATE_LAN, bridge_id);
  ds_system_call(command, strlen(command));
  this->dhcp_reservations_updated = false;

  if ( !QCMAP_LAN_Client::CheckAndActivateIPCollisionFromBridgeId() )
  {
    LOG_MSG_ERROR("CheckAndActivateIPCollision Failed",0,0,0);
    *qmi_err_num = QMI_ERR_INTERNAL_V01;
    return false;
  }

  /* Restart tethered links */
  if ( !(QCMAP_LAN_Client::RestartTetheredClient(QCMAP_LAN_DEVICE_TYPE_ANY)) )
  {
    LOG_MSG_ERROR("RestartTetheredClient Failed",0,0,0);
    *qmi_err_num = QMI_ERR_INTERNAL_V01;
    return false;
  }

  return true;
}

/*===========================================================================
  FUNCTION IsLegacyModeEnabled
  ===========================================================================*/
/*!
  @brief
  get legacy enabled or not

  @return
  true	- on Success
  false - on Failure

  @note

  - Dependencies
  - None

  - Side Effects
  - None
 */
/*=========================================================================*/
boolean QCMAP_LAN_Client::IsLegacyModeEnabled()
{
  char command[MAX_COMMAND_STR_LEN] = {0};
  char result[MAX_COMMAND_STR_LEN] = {0};
  int pd_activated = 0;
  int ext_router_mode_enabled = 0;

  snprintf(command, MAX_COMMAND_STR_LEN, QCMAP_GET_PD_ACTIVATED_CONFIG);
  if (!ExecuteSystemCmd((const char *)command, result, sizeof(result)))
  {
    LOG_MSG_ERROR("Failed to get pd_activated config", 0, 0, 0);
  }
  else
  {
    pd_activated = atoi(result);
  }

  memset(command, 0, MAX_COMMAND_STR_LEN);
  memset(result, 0, MAX_COMMAND_STR_LEN);
  snprintf(command, MAX_COMMAND_STR_LEN, QCMAP_GET_PD_EXT_ROUTER_MODE_ENABLED_CONFIG);
  if (!ExecuteSystemCmd((const char *)command, result, sizeof(result)))
  {
    LOG_MSG_ERROR("Failed to get ext_router_mode_enabled config", 0, 0, 0);
  }
  else
  {
    ext_router_mode_enabled = atoi(result);
  }

  if (pd_activated && !ext_router_mode_enabled)
  {
    return true;
  }

  return false;
} /* End: IsLegacyModeEnabled() */

/*===========================================================================
FUNCTION RestartTetheredClient()
===========================================================================*/
/** @ingroup section_RestartTetheredClient

  Toggles the tethered link

  @param[in] qcmap_lan_device_type_enum dev_type

  @return
  TRUE -- Success \n
  FALSE -- Failure \n

  @dependencies
  None
*/
/*=========================================================================*/
boolean QCMAP_LAN_Client::RestartTetheredClient
(
   qcmap_lan_device_type_enum dev_type
)
{
  char command[QCMAP_MAX_COMMAND_LEN] = {0};
  char result[QCMAP_MAX_SCAN_SIZE] = {0};
  qmi_error_type_v01 qmi_err_num;

  LOG_MSG_INFO1("Device type:%d", dev_type, 0, 0);

    if((dev_type == QCMAP_LAN_DEVICE_TYPE_ETHERNET) || (dev_type == QCMAP_LAN_DEVICE_TYPE_ANY))
    {
      LOG_MSG_INFO1("RestartTetheredClient: Ethernet",0,0,0);
      if ( QCMAP_LAN_Client::UciGetUtility(owrt_filename[0], PHY_IFACE_NAMES, 0,
           ETH_INTERFACE, result) )
      {
        if ( QCMAP_LAN_Client::IsTetheredLinkUp(result) && QCMAP_LAN_Client::IsLinkDetected(result) )
        {
          snprintf(command, MAX_COMMAND_STR_LEN, "%s %s %d %d", LAN_UTIL_SCRIPT,
                   INC_DEC_RESTART_LINK_COUNT, INCREMENT, QCMAP_LAN_DEVICE_TYPE_ETHERNET);
          ds_system_call(command, strlen(command));
          QCMAP_LAN_Client::ToggleEth(result);
        }
      }
      else
      {
        LOG_MSG_ERROR("IfaceName not found",0,0,0);
      }
    }

    if((dev_type == QCMAP_LAN_DEVICE_TYPE_ETHERNET_NIC2) || (dev_type == QCMAP_LAN_DEVICE_TYPE_ANY))
    {
      LOG_MSG_INFO1("RestartTetheredClient: Ethernet2",0,0,0);
      if ( QCMAP_LAN_Client::UciGetUtility(owrt_filename[0], PHY_IFACE_NAMES, 0,
           ETHNIC2_INTERFACE, result) )
      {
        if ( QCMAP_LAN_Client::IsTetheredLinkUp(result) && QCMAP_LAN_Client::IsLinkDetected(result) )
        {
          snprintf(command, MAX_COMMAND_STR_LEN, "%s %s %d %d", LAN_UTIL_SCRIPT,
                   INC_DEC_RESTART_LINK_COUNT, INCREMENT, QCMAP_LAN_DEVICE_TYPE_ETHERNET_NIC2);
          ds_system_call(command, strlen(command));
          QCMAP_LAN_Client::ToggleEth(result);
        }
      }
      else
      {
        LOG_MSG_ERROR("IfaceName not found",0,0,0);
      }
    }

    if((dev_type == QCMAP_LAN_DEVICE_TYPE_USB) || (dev_type == QCMAP_LAN_DEVICE_TYPE_ANY))
    {
      LOG_MSG_INFO1("RestartTetheredClient: USB",0,0,0);
      if ( (QCMAP_LAN_Client::UciGetUtility(owrt_filename[0], PHY_IFACE_NAMES, 0,
            RNDIS_INTERFACE, result)) ||
           (QCMAP_LAN_Client::UciGetUtility(owrt_filename[0], PHY_IFACE_NAMES, 0,
            ECM_INTERFACE, result)) )
      {
        if ( QCMAP_LAN_Client::IsTetheredLinkUp(result) )
        {
          QCMAP_LAN_Client::ToggleUSB();
        }
      }
      else
      {
        LOG_MSG_ERROR("IfaceName not found",0,0,0);
      }
    }

    if((dev_type == QCMAP_LAN_DEVICE_TYPE_ANY_AP) || (dev_type == QCMAP_LAN_DEVICE_TYPE_ALL_AP) || (dev_type == QCMAP_LAN_DEVICE_TYPE_ANY))
    {
      m_pQCMapWlanObj->RestartTetheredWLANClient();
    }
  return true;
}

/*===========================================================================
FUNCTION IsTetheredLinkUp()
===========================================================================*/
/** @ingroup section_IsTetheredLinkUp

  Checks if link is up

  @param[in] char *ifname

  @return
  TRUE -- Success \n
  FALSE -- Failure \n

  @dependencies
  None
*/
/*=========================================================================*/
boolean QCMAP_LAN_Client::IsTetheredLinkUp
(
   char *ifname
)
{
  char command[QCMAP_MAX_COMMAND_LEN] = {0};
  char result[QCMAP_MAX_SCAN_SIZE] = {0};
  int output;

  if ( ifname == NULL )
  {
    LOG_MSG_ERROR("Invalid ifname passed", 0, 0, 0);
    return false;
  }
  snprintf(command, QCMAP_MAX_COMMAND_LEN, "%s %s %s",
           BRCTL_SHOW, QUERY_GREP_COUNT, ifname);
  if ( !ExecuteSystemCmd((const char *)command, result, sizeof(result)) )
  {
    LOG_MSG_ERROR("Failed to execute command : %s",command,0,0);
    return false;
  }
  output = atoi(result);
  if ( output == 0 )
  {
    return false;
  }
  return true;
}

/*===========================================================================
FUNCTION ToggleEth()
===========================================================================*/
/** @ingroup section_ToggleEth

  Toggles the ethernet link

  @param[in] char *ifname

  @return
  None

  @dependencies
  None
*/
/*=========================================================================*/
void QCMAP_LAN_Client::ToggleEth
(
   char *ifname
)
{
  char command[QCMAP_MAX_COMMAND_LEN] = {0};

  if ( ifname == NULL )
  {
    LOG_MSG_ERROR("Invalid ifname passed", 0, 0, 0);
    return;
  }
  snprintf(command, QCMAP_MAX_COMMAND_LEN,"%s %s", ETH_RESTART, ifname);
  ds_system_call(command, strlen(command));
}

/*===========================================================================
FUNCTION ToggleUSB()
===========================================================================*/
/** @ingroup section_ToggleUSB

  Toggles the USB link

  @return
  None

  @dependencies
  None
*/
/*=========================================================================*/
void QCMAP_LAN_Client::ToggleUSB()
{
  char command[QCMAP_MAX_COMMAND_LEN] = {0};
  QCMAP_LAN_CLIENT_RUN_COMMANDS("%s", RESTART_USB);
}

/*===========================================================================
  FUNCTION AddDHCPReservRecord
  ===========================================================================*/
/*!
  @brief
  Add a DHCP Reservation Record

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
boolean QCMAP_LAN_Client::AddDHCPReservRecord
(
  qcmap_dhcp_reservation  *dhcp_reserv_record,
  qmi_error_type_v01 *qmi_err_num
)
{
  char command[QCMAP_MAX_COMMAND_LEN] = {0};
  char result[QCMAP_MAX_SCAN_SIZE] = {0};
  char mac_addr_str[QCMAP_LAN_MAC_ADDR_NUM_CHARS] = {0};
  in_addr tmpIP;
  uint32_t dhcp_start_address;
  uint32_t dhcp_end_address;
  qcmap_lan_config lan_config;
  int num_of_entries;
  int16_t current_bridge_context = -1;

  if ( !dhcp_reserv_record )
  {
    LOG_MSG_ERROR("Add DHCP Recrod Failed:",0,0,0);
    return false;
  }

  if (!QCMAP_LAN_Client::GetBridgeVLANContext(&current_bridge_context))
  {
    LOG_MSG_ERROR("Failed to get the current bridge context", 0, 0, 0);
    *qmi_err_num = QMI_ERR_INTERNAL_V01;
    return false;
  }

  if (current_bridge_context != DEFAULT_BRIDGE_ID)
  {
    LOG_MSG_INFO1("\nThis feature is supported only for Default bridge(br-lan), Current LAN Bridge is for VLAN ID %d \n", current_bridge_context,0,0);
    *qmi_err_num = QMI_ERR_INTERNAL_V01;
    return false;
  }

  if ( QCMAP_LAN_Client::UciGetUtility(owrt_filename[0], "no_of_configs", 0,
       "no_of_dhcp_reservation_records", result) )
  {
    num_of_entries=atoi(result);
  }
  else
  {
    LOG_MSG_ERROR("UciGetUtility failed ",0,0,0);
    *qmi_err_num = QMI_ERR_INTERNAL_V01;
    return false;
  }

  if( num_of_entries == QCMAP_MAX_DHCP_RESERVATION_ENTRIES )
  {
    printf(" Reached Max limit on DHCP Reservation Record Entries!!");
    LOG_MSG_ERROR(" Reached Max limit on DHCP Reservation Record Entries!!",0,0,0);
    *qmi_err_num = QMI_ERR_INVALID_ARG_V01;
    return false;
  }

  if( QCMAP_LAN_Client::GetActiveLANConfig(&lan_config, qmi_err_num) == false )
  {
    LOG_MSG_ERROR("GetActiveLANConfig Failed",0,0,0);
    *qmi_err_num = QMI_ERR_INTERNAL_V01;
    return false;
  }

  dhcp_start_address=lan_config.dhcp_config.dhcp_start_ip;
  dhcp_end_address=lan_config.dhcp_config.dhcp_end_ip;

  if ( ( dhcp_start_address > dhcp_reserv_record->client_reserved_ip) ||
        ( (dhcp_end_address - 1) < dhcp_reserv_record->client_reserved_ip) )
  {
      if( dhcp_reserv_record->client_reserved_ip == dhcp_end_address )
      {
          printf("Can not reserve last IP in the Pool\n");
          *qmi_err_num = QMI_ERR_INVALID_ARG_V01;
          return false;
      }
      LOG_MSG_ERROR("IP address is outside the dhcp range!!",0,0,0);
      printf("\nIP address is outside the dhcp range!");
      *qmi_err_num = QMI_ERR_INVALID_ARG_V01;
      return false;
  }


  bool mac_addr_non_empty = check_non_empty_mac_addr(dhcp_reserv_record->client_mac_addr,
                                                mac_addr_str);
  if ( dhcp_reserv_record->enable_reservation )
  {

     /* DHCP Reservation allows MAC address for ETH/WLAN clients and device name for
        USB clients */
     if ((dhcp_reserv_record->client_device_name[0] != '\0') && (mac_addr_non_empty))
     {
       LOG_MSG_ERROR("DHCP Reservation does not allow both MAC address and Hostname",0,0,0);
       *qmi_err_num = QMI_ERR_INVALID_OPERATION_V01;
       return false;
     }
   }

  tmpIP.s_addr =ntohl(dhcp_reserv_record->client_reserved_ip);
  memset(result, 0, QCMAP_MAX_SCAN_SIZE);
  if (mac_addr_non_empty)
  {
    ds_mac_addr_ntop(dhcp_reserv_record->client_mac_addr,mac_addr_str);
    if ( strncmp(mac_addr_str,MAC_NULL_STR,QCMAP_MSGR_MAC_ADDR_NUM_CHARS_V01) == 0 )
    {
        LOG_MSG_ERROR("MAC address of the client is : NULL",0,0,0);
        *qmi_err_num = QMI_ERR_INVALID_ARG_V01;
        return false;
    }

    int device_type = 1;
    snprintf(command, QCMAP_MAX_COMMAND_LEN,"%s %s %d %s %s %d",VLAN_CONFIG,
             ADD_DHCP_RESERVATION, device_type, mac_addr_str, inet_ntoa(tmpIP),
             dhcp_reserv_record->enable_reservation);

    if (!ExecuteSystemCmd(command, result, sizeof(result)))
    {
      LOG_MSG_ERROR("failed to execute command : %s",command,0,0);
      *qmi_err_num = QMI_ERR_INTERNAL_V01;
      return false;
    }
  }
  else if(dhcp_reserv_record->client_device_name[0] != '\0')
  {
    int device_type = 0;
    snprintf(command, QCMAP_MAX_COMMAND_LEN,"%s %s %d %s %s %d",VLAN_CONFIG,
             ADD_DHCP_RESERVATION, device_type, dhcp_reserv_record->client_device_name,
             inet_ntoa(tmpIP), dhcp_reserv_record->enable_reservation);

    if (!ExecuteSystemCmd(command, result, sizeof(result)))
    {
      LOG_MSG_ERROR("failed to execute command : %s",command,0,0);
      *qmi_err_num = QMI_ERR_INTERNAL_V01;
      return false;
    }
  }
  if (strlen(result) > 0) 
  {
    result[strlen(result)-1] = '\0';
  } 
  if(strncmp(result, ADD_DHCP_FAILED, strlen(ADD_DHCP_FAILED)) == 0)
  {
    *qmi_err_num = QMI_ERR_INVALID_ARG_V01;
    return false;
  }

  this->dhcp_reservations_updated = true;
  return true;
}

/*===========================================================================
  FUNCTION GetDHCPReservRecords
  ===========================================================================*/
/*!
  @brief
  Display DHCP Reservation Records

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
boolean QCMAP_LAN_Client::GetDHCPReservRecords
(
  qcmap_dhcp_reservation  *dhcp_reserv_records,
  uint32_t                        *num_entries,
  qmi_error_type_v01 *qmi_err_num

)
{
  char command[QCMAP_MAX_COMMAND_LEN] = {0};
  char result[QCMAP_MAX_SCAN_SIZE] = {0};

  /* Getting number of DHCP reservation records */

  if ( QCMAP_LAN_Client::UciGetUtility(owrt_filename[0], "no_of_configs", 0,
       "no_of_dhcp_reservation_records", result) )
  {
    *num_entries=atoi(result);
  }
  if ( *num_entries == 0 )
  {
    LOG_MSG_INFO1("No DHCP Reservation Records",0,0,0);
    return true;
  }

  for(int i=0 ; i< *num_entries && dhcp_reserv_records ; i++)
  {
    /* Getting DHCP reservation IP */
    memset(command,0,QCMAP_MAX_COMMAND_LEN);
    memset(result,0,QCMAP_MAX_SCAN_SIZE);

    if ( QCMAP_LAN_Client::UciGetUtility(owrt_filename[0],
         "dhcp_reservation", 0, "ip", result,i) )
    {
      strlcpy(dhcp_reserv_records->reserved_ip_string, result, strlen(result)+1);
    }

    /* Getting DHCP reservation Device Type */
    memset(command,0,QCMAP_MAX_COMMAND_LEN);
    memset(result,0,QCMAP_MAX_SCAN_SIZE);
    if (!QCMAP_LAN_Client::UciGetUtility(owrt_filename[0],
         "dhcp_reservation", 0, "device_type", result,i) )
    {
      *qmi_err_num = QMI_ERR_INTERNAL_V01;
      return false;
    }

    if(strncmp(result, "usb", strlen("usb")) == 0)
    {
      memset(command,0,QCMAP_MAX_COMMAND_LEN);
      memset(result,0,QCMAP_MAX_SCAN_SIZE);
      if ( QCMAP_LAN_Client::UciGetUtility(owrt_filename[0],

            "dhcp_reservation", 0, "name", result,i) )
      {
        strlcpy(dhcp_reserv_records->client_device_name, result, strlen(result)+1);
      }
    }

    memset(command,0,QCMAP_MAX_COMMAND_LEN);
    memset(result,0,QCMAP_MAX_SCAN_SIZE);
    if (!QCMAP_LAN_Client::UciGetUtility(owrt_filename[0],
        "dhcp_reservation", 0, "device_type", result,i) )
    {
      *qmi_err_num = QMI_ERR_INTERNAL_V01;
      return false;
    }

    if(strncmp(result, "non_usb", strlen("non_usb")) == 0)
    {
      memset(command,0,QCMAP_MAX_COMMAND_LEN);
      memset(result,0,QCMAP_MAX_SCAN_SIZE);
      if ( QCMAP_LAN_Client::UciGetUtility(owrt_filename[0],
           "dhcp_reservation", 0, "mac", result,i) )
      {
        strlcpy(dhcp_reserv_records->mac_addr_string, result, QCMAP_LAN_MAC_ADDR_NUM_CHARS);
      }
    }

    memset(command,0,QCMAP_MAX_COMMAND_LEN);
    memset(result,0,QCMAP_MAX_SCAN_SIZE);
    if ( QCMAP_LAN_Client::UciGetUtility(owrt_filename[0],
         "dhcp_reservation", 0, "enable", result,i) )
    {
      dhcp_reserv_records->enable_reservation=atoi(result);
    }

    dhcp_reserv_records++;
  }
  return true;
}

/*===========================================================================
  FUNCTION EditDHCPReservRecord
  ===========================================================================*/
/*!
  @brief
  Edit a DHCP record based on MAC or IP address as index

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
boolean QCMAP_LAN_Client::EditDHCPReservRecord
(
   uint32_t                         *client_addr,
   qcmap_dhcp_reservation           *dhcp_reserv_record,
   qmi_error_type_v01 *qmi_err_num
)
{
  char command[QCMAP_MAX_COMMAND_LEN] = {0};
  char result[QCMAP_MAX_SCAN_SIZE] = {0};
  char mac_addr_str[QCMAP_LAN_MAC_ADDR_NUM_CHARS]={0};
  char old_reserv_ip[QCMAP_MAX_SCAN_SIZE];
  char dnsmasq_file_name[QCMAP_MAX_SCAN_SIZE]={0};
  int old_enable_flag =0;
  int num_entries=0;
  uint32_t dhcp_start_address;
  uint32_t dhcp_end_address;
  in_addr tmpIP,clientIP;
  qcmap_lan_config lan_config;
  bool new_reserv_ip_provided = false;
  int16_t current_bridge_context = -1;

  if (!QCMAP_LAN_Client::GetBridgeVLANContext(&current_bridge_context))
  {
    LOG_MSG_ERROR("Failed to get the current bridge context", 0, 0, 0);
    *qmi_err_num = QMI_ERR_INTERNAL_V01;
    return false;
  }

  if (current_bridge_context != DEFAULT_BRIDGE_ID)
  {
    LOG_MSG_INFO1("\nThis feature is supported only for Default bridge(br-lan), Current LAN Bridge is for VLAN ID %d \n", current_bridge_context,0,0);
    *qmi_err_num = QMI_ERR_INTERNAL_V01;
    return false;
  }

  if (QCMAP_LAN_Client::UciGetUtility(owrt_filename[0], "no_of_configs", 0,
      "no_of_dhcp_reservation_records", result) )
  {
     num_entries=atoi(result);
  }
  if ( num_entries == 0 )
  {
    printf("No DHCP Reservation Record to Edit\n");
    return false;
  }

  /*Check for valid pointers*/
  if ( !dhcp_reserv_record || !client_addr)
  {
    LOG_MSG_ERROR("Edit DHCP Records Failed ",0 ,0,0);
    return false;
  }

  clientIP.s_addr=ntohl(dhcp_reserv_record->client_reserved_ip);
  snprintf(dhcp_reserv_record->reserved_ip_string,QCMAP_MAX_SCAN_SIZE,"%s",inet_ntoa(clientIP));

  if ( (dhcp_reserv_record->reserved_ip_string[0] != '\0')  )
  {
    if( QCMAP_LAN_Client::GetActiveLANConfig(&lan_config, qmi_err_num) == false )
    {
      printf("GetLANConfig Failed");
      *qmi_err_num = QMI_ERR_INTERNAL_V01;
      return false;
    }

    dhcp_start_address=lan_config.dhcp_config.dhcp_start_ip;
    dhcp_end_address=lan_config.dhcp_config.dhcp_end_ip;

    /* to check whether new reserv ip provided is in the range or not. */
    if ( ( dhcp_start_address >dhcp_reserv_record->client_reserved_ip) ||
          ( (dhcp_end_address - 1) < dhcp_reserv_record->client_reserved_ip) )
    {
      LOG_MSG_ERROR("New Reserv IP address is outside the dhcp range!!",0,0,0);
      printf("\nNew Reserv IP address is outside the dhcp range!");
      *qmi_err_num = QMI_ERR_INVALID_ARG_V01;
      return false;
    }
  }

  /* Here we Getting index of partular DHCP reservation from qcmap_lan file
     on the basis of reserved ip address provided by user */
  tmpIP.s_addr =ntohl(*client_addr);
  snprintf(command, QCMAP_MAX_COMMAND_LEN,
           "/etc/data/uci_ex.sh show qcmap_lan | grep -i dhcp_reservation | grep -w %s | awk -F'[][]' '{print $2}' ",
           inet_ntoa(tmpIP));

  if(!ExecuteSystemCmd((const char *)command,result,sizeof(result)))
  {
    LOG_MSG_ERROR("Failed to execute command : %s",command,0,0);
    printf("Incorrect Ip Address\n");
    *qmi_err_num = QMI_ERR_INTERNAL_V01;
    return false;
  }
  int index = atoi(result);



  memset(command,0,QCMAP_MAX_COMMAND_LEN);
  memset(result,0,QCMAP_MAX_SCAN_SIZE);
  if ( QCMAP_LAN_Client::UciGetUtility(owrt_filename[0],
      "dhcp_reservation", 0, "enable", result, index) )
  {
    old_enable_flag=atoi(result);
  }

  /* Fectching device type and on the basis of device type, we are editing */
  memset(command,0,QCMAP_MAX_COMMAND_LEN);
  memset(result,0,QCMAP_MAX_SCAN_SIZE);
  if (!QCMAP_LAN_Client::UciGetUtility(owrt_filename[0],
      "dhcp_reservation", 0, "device_type", result, index) )
  {
    return false;
  }

  /* If the device_type/result is usb */
  if(strncmp(result, "usb", strlen("usb")) == 0)
  {
    if ( (dhcp_reserv_record->mac_addr_string[0] != '\0')  )
    {
     printf("\nDHCP Reservation does not allow both MAC address and Hostname");
     return false;
    }

    /* if client_device_name is empty, we are assigning a "NULL" String to it*/
    if ( (dhcp_reserv_record->client_device_name[0] == '\0') )
    {
      strlcpy(dhcp_reserv_record->client_device_name, "NULL", sizeof("NULL"));
    }

    /* if reserved_ip_string is empty, we are assigning a "NULL" String to it*/
    if ( (dhcp_reserv_record->reserved_ip_string[0] == '\0') )
    {
      strlcpy(dhcp_reserv_record->reserved_ip_string, "NULL", sizeof("NULL"));
    }

    snprintf(command, QCMAP_MAX_COMMAND_LEN,"%s %s %s %s %d %d %s %s",VLAN_CONFIG,
             EDIT_DHCP_RESERVATION, inet_ntoa(tmpIP),
             dhcp_reserv_record->reserved_ip_string,old_enable_flag,
             dhcp_reserv_record->enable_reservation, "usb",
             dhcp_reserv_record->client_device_name);
    ds_system_call(command, strlen(command));

  }
  else if(strncmp(result, "non_usb", strlen("non_usb")) == 0)
  {
    if(strncmp(dhcp_reserv_record->client_device_name, "\0", sizeof("\0")) != '\0')
    {
     printf("\nDHCP Reservation does not allow both MAC address and Hostname");
     return false;
    }
    if ( strncmp(dhcp_reserv_record->reserved_ip_string, "\0", sizeof("\0")) == '\0'  )
    {
      strlcpy(dhcp_reserv_record->reserved_ip_string, "NULL", sizeof("NULL"));
    }
    if(strncmp(dhcp_reserv_record->mac_addr_string, "\0", sizeof("\0")) == '\0')
    {
      strlcpy(dhcp_reserv_record->mac_addr_string, "NULL", sizeof("NULL"));
    }

    snprintf(command, QCMAP_MAX_COMMAND_LEN,"%s %s %s %s %d %d %s %s",VLAN_CONFIG,
             EDIT_DHCP_RESERVATION, inet_ntoa(tmpIP),
             dhcp_reserv_record->reserved_ip_string,old_enable_flag,
             dhcp_reserv_record->enable_reservation, "non_usb",
             dhcp_reserv_record->mac_addr_string);
    ds_system_call(command, strlen(command));

  }
  ExecuteUCICommit();
  this->dhcp_reservations_updated = true;

  return true;
}

/*===========================================================================
  FUNCTION DeleteDHCPReservRecord
  ===========================================================================*/
/*!
  @brief
  delete a DHCP record based on MAC or IP address as index

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
boolean QCMAP_LAN_Client::DeleteDHCPReservRecord
(
  uint32_t             *addr,
  qmi_error_type_v01 *qmi_err_num

)
{
  char command[QCMAP_MAX_COMMAND_LEN] = {0};
  char result[QCMAP_MAX_SCAN_SIZE]={0};
  int num_entries=0;
  in_addr tmpIP;
  int16_t current_bridge_context = -1;

  if ( !addr )
  {
    LOG_MSG_ERROR("Delete DHCP Records Failed", 0, 0, 0);
    *qmi_err_num = QMI_ERR_INVALID_ARG_V01;
    return false;
  }

  if (!QCMAP_LAN_Client::GetBridgeVLANContext(&current_bridge_context))
  {
    LOG_MSG_ERROR("Failed to get the current bridge context", 0, 0, 0);
    *qmi_err_num = QMI_ERR_INTERNAL_V01;
    return false;
  }

  if (current_bridge_context != DEFAULT_BRIDGE_ID)
  {
    LOG_MSG_INFO1("\nThis feature is supported only for Default bridge(br-lan), Current LAN Bridge is for VLAN ID %d \n", current_bridge_context,0,0);
    *qmi_err_num = QMI_ERR_INTERNAL_V01;
    return false;
  }

  if ( QCMAP_LAN_Client::UciGetUtility(owrt_filename[0], "no_of_configs", 0,
       "no_of_dhcp_reservation_records", result) )
  {
     num_entries=atoi(result);
  }
  if ( num_entries == 0 )
  {
    LOG_MSG_ERROR("No DHCP Reservation Record to Delete", 0, 0, 0);
    *qmi_err_num = QMI_ERR_NO_ENTRY_V01;
    return false;
  }

  tmpIP.s_addr =ntohl(*addr);
  snprintf(command, QCMAP_MAX_COMMAND_LEN,"%s %s %s", VLAN_CONFIG,
           DELETE_DHCP_RESERVATION, inet_ntoa(tmpIP) );
  ds_system_call(command, strlen(command));

  this->dhcp_reservations_updated = true;

  return true;
}

/*===========================================================================
  FUNCTION DeleteOldDHCPReservRecord
  ===========================================================================*/
/*!
  @brief
  delete old DHCP record which are outside pool based on IP address as index

  @return
  true  - on Success
  false - on Failure

  @parameters
  qmi_error_type_v01 *qmi_err_num

  @note

  - Dependencies
  - None

  - Side Effects
  - None
 */
/*=========================================================================*/
boolean QCMAP_LAN_Client::DeleteOldDHCPReservRecord
(
  qmi_error_type_v01 *qmi_err_num
)
{

  uint32_t dhcp_start_address;
  uint32_t dhcp_end_address;
  uint32_t num_entries =0, i=0;
  uint32_t tmpIP;
  int16_t current_bridge_context = -1;
  in_addr addr;
  addr.s_addr =0;
  qcmap_dhcp_reservation dhcp_reserv_record[QCMAP_MAX_DHCP_RESERVATION_ENTRIES];
  memset(dhcp_reserv_record,0,QCMAP_MAX_DHCP_RESERVATION_ENTRIES*sizeof(qcmap_dhcp_reservation));

  qcmap_lan_config lan_config;
  memset(&lan_config,0,sizeof(qcmap_lan_config));
  if(GetLANConfig(&lan_config, qmi_err_num) == false )
  {
    LOG_MSG_ERROR("GetLANConfig Failed to delete old DHCP records",0,0,0);
    return false;
  }

  dhcp_start_address=lan_config.dhcp_config.dhcp_start_ip;
  dhcp_end_address=lan_config.dhcp_config.dhcp_end_ip;

  if(GetDHCPReservRecords(dhcp_reserv_record, &num_entries, qmi_err_num) )
  {
    if ( num_entries == 0 )
    {
      LOG_MSG_INFO1("No DHCP Reservation Records",0,0,0);
      return true;
    }
    else
    {
      /* Get global bridge context */
      if (!QCMAP_LAN_Client::GetBridgeVLANContext(&current_bridge_context))
      {
        LOG_MSG_ERROR("Failed to get the current bridge context", 0, 0, 0);
        return false;
      }
      if (current_bridge_context != DEFAULT_BRIDGE_ID )
      {
        LOG_MSG_INFO1("\nThis feature is supported only for Default bridge(br-lan), Current LAN Bridge is for VLAN ID %d \n", current_bridge_context,0,0);
        return true;
      }
      for ( i = 0;i < num_entries; i++)
      {
        if ( (inet_aton(dhcp_reserv_record[i].reserved_ip_string, &addr) ) &&
             ( addr.s_addr != 0 ) && (addr.s_addr != 0xffffffff ) );
          tmpIP = ntohl(addr.s_addr);
        if( (tmpIP < dhcp_start_address) ||
            ( tmpIP > (dhcp_end_address - 1)))
        {
          if(!(DeleteDHCPReservRecord(&tmpIP, qmi_err_num)) )
          {
            LOG_MSG_ERROR("Failed to delete old DHCP Reservation record , Error: 0x%x", qmi_err_num,0,0);
            return false;
          }
        }
      }
      return true;
    }
  }
  return true;
}

/*=====================================================
  FUNCTION GetKernelVer
======================================================*/
/*!
@brief
  Queries the kernel version.

@parameters
  char *version

@return
  true  - on success
  false - on failure

@note

- Dependencies
- None

- Side Effects
- None
*/
/*=====================================================*/
boolean QCMAP_LAN_Client::GetKernelVer
(
  char *version
)
{
  char command[QCMAP_MAX_COMMAND_LEN] = {0};
  char result[QCMAP_MAX_SCAN_SIZE] = {0};

  if (version == NULL)
  {
    LOG_MSG_ERROR("Pointer to version is NULL.", 0,0,0);
    return false;
  }

  snprintf(command, QCMAP_MAX_COMMAND_LEN,
           "uname -r | awk '{print $1}' | cut -d '-' -f 1 ");
  if(!ExecuteSystemCmd((const char *)command,result,sizeof(result)))
  {
    LOG_MSG_ERROR("Failed to execute command : %s",command,0,0);
    return false;
  }
  strlcpy(version, result, strlen(result));

  return true;
}

/*==========================================================================
FUNCTION DeleteWWANPolicy()
===========================================================================*/
/** @ingroup qcmap_delete_wwan_policy

  Delete WWAN policy.

  @param[in] uint32_t profile_handle.
  @return
  TRUE -- Success \n
  FALSE -- Failure \n
*/
/*=========================================================================*/

boolean QCMAP_LAN_Client::DeleteWWANPolicy
(
  uint32_t profile_handle,
  qmi_error_type_v01 *qmi_err_num
)
{
  if ( profile_handle == 0 )
  {
    LOG_MSG_ERROR("Invalid wan profile handle passed ",0,0,0);
    *qmi_err_num = QMI_ERR_INVALID_HANDLE_V01;
    return false;
  }

  char command[QCMAP_MAX_COMMAND_LEN] = {0};
  snprintf(command, QCMAP_MAX_COMMAND_LEN,"%s %s %d",
                                          BACKHAUL_WWAN_CONFIG_FILE,
                                          QCMAP_DELETE_WWAN_PROFILE,
                                          profile_handle);
  ds_system_call(command, strlen(command));
  LOG_MSG_INFO1("WWAN policy deleted",0,0,0);
  return true;
}/*DeleteWWANPolicy()*/

/*==========================================================================
FUNCTION GetBackhaulType()
===========================================================================*/
/** @ingroup qcmap_get_backhaul_type

  Get Backhaul prefrences.

  @param[in]      char*       pointer to backhaul name.

  @return
  qcmap_backhaul_type_enum type
*/
/*=========================================================================*/
qcmap_backhaul_type_enum QCMAP_LAN_Client::GetBackhaulType
(
  char*wan
)
{
  qcmap_backhaul_type_enum bh_type = QCMAP_BACKHAUL_TYPE_ENUM_MIN_ENUM_VAL;
  if(wan == NULL)
  {
    LOG_MSG_ERROR("NULL pointer passed", 0,0,0);
  }

  if((wan != NULL) && (strncmp(wan, "m_waneth", strlen("m_waneth")) == 0))
  {
    bh_type = QCMAP_ETHERNET_BACKHAUL;
  }
  else if((wan != NULL) && (strncmp(wan, "m_wanwlan", strlen("m_wanwlan")) == 0))
  {
    bh_type = QCMAP_WLAN_BACKHAUL;
  }
  else if((wan != NULL) && (strncmp(wan, "m_wanbt", strlen("m_wanbt")) == 0))
  {
    bh_type = QCMAP_BT_BACKHAUL;
  }
  else if((wan != NULL) && (strncmp(wan, "m_wanusb", strlen("m_wanusb")) == 0))
  {
     bh_type = QCMAP_USB_CRADLE_BACKHAUL;
  }
  else if((wan != NULL) && (strncmp(wan, "m_wan", strlen("m_wan")) == 0))
  {
    bh_type = QCMAP_WWAN_BACKHAUL;
  }

  return bh_type;
}

/*===========================================================================
FUNCTION GetWANType()
===========================================================================*/
/** @ingroup qcmap_get_wan_type

  Get wan type.

  @param[in]      qcmap_backhaul_type_enum wan_type.

  @return
  pointer to wan type name
*/
/*=========================================================================*/
const char* QCMAP_LAN_Client::GetWANType
(
  qcmap_backhaul_type_enum type
)
{
  const char *ptr = NULL;
  switch (type)
    {
      case QCMAP_WWAN_BACKHAUL:
        ptr = "WWAN";
        break;

        case QCMAP_WLAN_BACKHAUL:
        ptr = "WLAN";
        break;

        case QCMAP_USB_CRADLE_BACKHAUL:
        ptr = "USB Cradle";
        break;

        case QCMAP_ETHERNET_BACKHAUL:
        ptr = "ETHERNET";
        break;

        case QCMAP_BT_BACKHAUL:
        ptr = "BT WAN";
        break;

        default:
            LOG_MSG_ERROR("Invalid wan type",0,0,0);
    }

  return ptr;
}

/*===========================================================================
FUNCTION SetActiveBackhaulPref()
===========================================================================*/
/** @ingroup qcmap_set_active_backhaul_pref

  Set Backhaul prefrences.

  @param[in]      qcmap_backhaul_pref*       pointer to backhaul pref request.

  @return
  TRUE -- Success \n
  FALSE -- Failure \n
*/
/*=========================================================================*/
boolean QCMAP_LAN_Client::SetActiveBackhaulPref
(
  qcmap_backhaul_pref_t *qcmap_backhaul_pref,
  qmi_error_type_v01 *qmi_err_num
)
{
  char result[QCMAP_MAX_SCAN_SIZE] = {0};
  char cmd[QCMAP_MAX_COMMAND_LEN] = {0};

 if(qcmap_backhaul_pref == NULL)
  {
    LOG_MSG_ERROR("NULL pointer passed", 0, 0, 0);
    *qmi_err_num = QMI_ERR_INVALID_ARG_V01;
    return false;
  }

  snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "%s %s %d %d %d %d %d",
                BACKHAUL_COMMMON_CONFIG_FILE,
                SET_BACKHAUL_PREF,
                qcmap_backhaul_pref->first,
                qcmap_backhaul_pref->second,
                qcmap_backhaul_pref->third,
                qcmap_backhaul_pref->fourth,
                qcmap_backhaul_pref->fifth);
  if (ExecuteSystemCmd((const char *)cmd, result,
                                     sizeof(result)) == true)
  {
    LOG_MSG_INFO1("Backhaul pref set properly", 0, 0, 0);
    return true;
  }
  else
  {
    LOG_MSG_ERROR("Fail to set Backhaul pref",0, 0, 0);
    *qmi_err_num = QMI_ERR_INTERNAL_V01;
    return false;
  }
}/* End SetActiveBackhaulPref */


/*===========================================================================
FUNCTION GetBackhaulPref()
===========================================================================*/
/** @ingroup qcmap_get_active_backhaul_pref

  Get Backhaul prefrences.

  @param[in]      qcmap_backhaul_pref*       pointer to backhaul pref response.

  @return
  TRUE -- Success \n
  FALSE -- Failure \n
*/
/*=========================================================================*/

boolean QCMAP_LAN_Client::GetBackhaulPref
(
  qcmap_backhaul_pref_t *qcmap_backhaul_pref_resp,
  qmi_error_type_v01 *qmi_err_num
)
{
  char result[QCMAP_MAX_SCAN_SIZE] = {0};
  char cmd[QCMAP_MAX_COMMAND_LEN] = {0};
  char *token = NULL;
  char *ptr = NULL;
  qcmap_backhaul_type_enum bh_type;

  if(qcmap_backhaul_pref_resp == NULL)
  {
    LOG_MSG_ERROR("NULL pointer passed",0,0,0);
    *qmi_err_num = QMI_ERR_INVALID_ARG_V01;
    return false;
  }

  snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "%s",UCI_GET_BH_PREF);
  if (ExecuteSystemCmd((const char *)cmd, result,
                                     sizeof(result)) == true)
  {
    token = strtok_r(result, " ", &ptr);
    if(token != NULL)
    {
      bh_type = QCMAP_LAN_Client :: GetBackhaulType(token);
      qcmap_backhaul_pref_resp->first = bh_type;
    }

    token = strtok_r(NULL, " ", &ptr);
    if(token != NULL)
    {
      bh_type = QCMAP_LAN_Client :: GetBackhaulType(token);
      qcmap_backhaul_pref_resp->second = bh_type;
    }

    token = strtok_r(NULL, " ", &ptr);
    if(token != NULL)
    {
      bh_type = QCMAP_LAN_Client :: GetBackhaulType(token);
      qcmap_backhaul_pref_resp->third = bh_type;
    }

    token = strtok_r(NULL, " ", &ptr);
    if(token != NULL)
    {
      bh_type = QCMAP_LAN_Client :: GetBackhaulType(token);
      qcmap_backhaul_pref_resp->fourth = bh_type;
    }

    token = strtok_r(NULL, " ", &ptr);
    if(token != NULL)
    {
      bh_type = QCMAP_LAN_Client :: GetBackhaulType(token);
      qcmap_backhaul_pref_resp->fifth = bh_type;
    }

    return true;
  }
  else
  {
    LOG_MSG_ERROR("Fail to get Backhaul pref",0, 0, 0);
    *qmi_err_num = QMI_ERR_INTERNAL_V01;
    return false;
  }
}

/*===========================================================================
  FUNCTION BringupBTTethering
  ===========================================================================*/
/*!
  @brief
  Brings up the BT Tethering

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
boolean QCMAP_LAN_Client::BringupBTTethering
(
  qcmap_bt_mode   bt_tethering_mode
)
{

  if (bt_tethering_mode == QCMAP_BT_LAN_ROUTER ||
      bt_tethering_mode == QCMAP_BT_WAN_ROUTER)
  {
    QCMAP_LAN_CLIENT_RUN_COMMANDS("%s %s %d",
                                          BT_CONFIG_FILE,
                                          BT_BRING_UP,
                                          bt_tethering_mode);
    LOG_MSG_INFO1("Add bt-pan configuration in network",0,0,0);

    return true;
  }
  return false;
}

/*===========================================================================
  FUNCTION BringdownBTTethering
  ===========================================================================*/
/*!
  @brief
  Brings down the BT Tethering

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
boolean QCMAP_LAN_Client::BringdownBTTethering
(
  qcmap_bt_mode   bt_tethering_mode
)
{
  if (bt_tethering_mode == QCMAP_BT_LAN_ROUTER ||
      bt_tethering_mode == QCMAP_BT_WAN_ROUTER)
  {
    QCMAP_LAN_CLIENT_RUN_COMMANDS("%s %s %d",
                                          BT_CONFIG_FILE,
                                          BT_BRING_DOWN,
                                          bt_tethering_mode);
    LOG_MSG_INFO1("Delete bt-pan configuration in network",0,0,0);

    return true;
  }
  return false;
}

boolean QCMAP_LAN_Client::GetBTTetheringStatus
(
  qcmap_bt_tethering_status    *bt_teth_status,
  qcmap_bt_mode                *bt_teth_mode
)
{
  qmi_error_type_v01 qmi_err_num;

  return GetBTTetheringStatus(bt_teth_status, &qmi_err_num, bt_teth_mode);
}

/*===========================================================================
  FUNCTION GetBTTetheringStatus
  ===========================================================================*/
/*!
  @brief
  Displays the BT Tethering current status & mode

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
boolean QCMAP_LAN_Client::GetBTTetheringStatus
(
  qcmap_bt_tethering_status    *bt_teth_status,
  qmi_error_type_v01           *qmi_err_num,
  qcmap_bt_mode                *bt_teth_mode
)
{
  char result[QCMAP_MAX_SCAN_SIZE] = {0};

  if ( !bt_teth_status || !bt_teth_mode)
  {
    LOG_MSG_ERROR("GetBTTetheringStatus Failed ",0,0,0);
    *qmi_err_num = QMI_ERR_INTERNAL_V01;
    return false;
  }

  if (UciGetUtility(owrt_filename[static_cast<int>(config_file::NETWORK)],
      "bt", 1, "device", result, 0))
  {
    if ( strncmp(result, BT_INTERFACE, strlen(BT_INTERFACE)) == 0 )
    {
      memset(result,0,QCMAP_MAX_SCAN_SIZE);

      if (UciGetUtility(owrt_filename[static_cast<int>(config_file::NETWORK)],
                        "bt", 1, "proto", result, 0))
      {
        if ( strncmp(result, STATIC_PROTO, strlen(STATIC_PROTO)) == 0 )
        {
          *bt_teth_status = QCMAP_BT_TETHERING_MODE_UP;
          *bt_teth_mode = QCMAP_BT_LAN_ROUTER;
        }
        else if ( strncmp(result, DHCP_PROTO, strlen(DHCP_PROTO)) == 0 )
        {
          *bt_teth_status = QCMAP_BT_TETHERING_MODE_UP;
          *bt_teth_mode = QCMAP_BT_WAN_ROUTER;
        }
      }
      else
      {
        LOG_MSG_ERROR("UciGetUtility failed ",0,0,0);
    *qmi_err_num = QMI_ERR_INTERNAL_V01;
        return false;
      }
    }
    else
    {
      *bt_teth_status = QCMAP_BT_TETHERING_MODE_DOWN;
    }
  }
  else
  {
    LOG_MSG_ERROR("UciGetUtility failed ",0,0,0);
    *qmi_err_num = QMI_ERR_INTERNAL_V01;
    return false;
  }
  return true;
}

/*===========================================================================
FUNCTION FactoryReset()
===========================================================================*/
/*!
  @brief
  Perform factory reset.

  @return
  TRUE -- Success \n
  FALSE -- Failure \n
*/
/*=========================================================================*/
boolean QCMAP_LAN_Client::RestoreFactoryConfig
(
  qmi_error_type_v01 *qmi_err_num
)
{
  LOG_MSG_INFO1("Factory Reset Start",0,0,0);
  char command[QCMAP_MAX_COMMAND_LEN] = {0};
  snprintf(command, QCMAP_MAX_COMMAND_LEN,"%s",FACTORY_RESET_CONFIG_FILE);
  ds_system_call(command, strlen(command));

  return true;
}/*FactoryReset()*/

/*=====================================================================
  FUNCTION PerformDnsmasqRestart
======================================================================*/
/*!
@brief
  - Executes dnsmasq restart command

@return
  none

@note
  - Executes dnsmasq restart command using dsi_system_call

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
void PerformDnsmasqRestart()
{
  char cmd[QCMAP_MAX_COMMAND_LEN] = {0};
  LOG_MSG_INFO1("Executing dnsmasq restart command: %s", DNSMASQ_RESTART_COMMAND, 0, 0);
  snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "%s", DNSMASQ_RESTART_COMMAND);
  ds_system_call(cmd, strlen(cmd));

  return;
} /* End PerformDnsmasqRestart */

/*=====================================================================
  FUNCTION PerformDnsmasqReload
======================================================================*/
/*!
@brief
  - Executes dnsmasq reload command

@return
  none

@note
  - Executes dnsmasq reload command using dsi_system_call

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
void QCMAP_LAN_Client::PerformDnsmasqReload()
{
  char cmd[QCMAP_MAX_COMMAND_LEN] = {0};
  LOG_MSG_INFO1("Executing dnsmasq reload command: %s", DNSMASQ_RELOAD_COMMAND, 0, 0);
  snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "%s", DNSMASQ_RELOAD_COMMAND);
  ds_system_call(cmd, strlen(cmd));

  return;
} /* End PerformDnsmasqReload */

/*=====================================================================
  FUNCTION GetSSID
======================================================================*/
/*!
@brief
  returns the currently configured ssid from wireless file
  this function is called from the SetResetDCHPVendorInfo

@return
  true if ssid is populated in the input string else false

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
bool QCMAP_LAN_Client::GetSSID(char * ssid)
{
  char command[MAX_COMMAND_STR_LEN] = {0}, result[MAX_COMMAND_STR_LEN] = {0}, temp[MAX_COMMAND_STR_LEN] = {0};

  snprintf(command, MAX_COMMAND_STR_LEN, " %s %s", UCI_GET_COMMAND, "wireless.@wifi-iface[0].mode");
  if (ExecuteSystemCmd((const char *)command, result, sizeof(result)))
  {
    if (strncmp(result, AP_MODE, strlen(AP_MODE)) == 0 )
    {
      memset(command, 0, MAX_COMMAND_STR_LEN);
      snprintf(command, MAX_COMMAND_STR_LEN, " %s %s", UCI_GET_COMMAND, "wireless.@wifi-iface[0].ssid");
      if (ExecuteSystemCmd((const char *)command, temp, sizeof(temp)))
      {
        strlcpy(ssid, temp, strlen(temp));
        LOG_MSG_INFO1("Wi-Fi SSID is: %s", ssid,0,0);
        return true;
      }
      else
      {
        LOG_MSG_ERROR("Failed to execute commad : %s", command, 0, 0);
        return false;
      }
    }
    else
    {
      strlcpy(ssid, DEFAULT_SSID, strlen(DEFAULT_SSID));
      LOG_MSG_INFO1("Wi-Fi SSID is: %s", ssid,0,0);
      return true;
    }
  }
  else
  {
    LOG_MSG_ERROR("Failed to execute commad : %s", command, 0, 0);
    return false;
  }
}

/*=====================================================================
  FUNCTION SetResetDCHPVendorInfo
======================================================================*/
/*!
@brief
  To set/reset dhcp vendor information in lan client side

@return
  true if set/reset dhcp vendor info successfully

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
bool QCMAP_LAN_Client::SetResetDCHPVendorInfo(bool          is_reset)
{
  char command[MAX_COMMAND_STR_LEN] = {0};
  char temp[MAX_COMMAND_STR_LEN] = {0}, ssid[MAX_COMMAND_STR_LEN] = {0};
  int current_state;

  if (is_reset)
  {
    /* Clean up DHCP Option 43 with old ssid */
    QCMAP_LAN_CLIENT_RUN_COMMANDS("%s %d 1", DHCP_OPT_FILE, FEATURE_DHCP_VENDOR_INFO);
    LOG_MSG_INFO1("Clean up DHCP Option 43 with old ssid done", 0, 0, 0);
    return true;
  }
  else
  {
    snprintf(command, MAX_COMMAND_STR_LEN, "%s %s",UCI_GET_COMMAND, "qcmap_lan.@dhcp_option[0].dhcp_vendor_info_enable");
    if (ExecuteSystemCmd((const char *)command, temp, sizeof(temp)))
    {
      current_state = atoi(temp);
      if (current_state)
      {
        if (!GetSSID(ssid))
        {
          LOG_MSG_ERROR("Error in getting the SSID", 0, 0, 0);
          return false;
        }

        memset(command, 0, MAX_COMMAND_STR_LEN);
        memset(temp, 0, MAX_COMMAND_STR_LEN);
        snprintf(command, MAX_COMMAND_STR_LEN, "grep dhcp-option-force %s | grep %s", DNSMASQ_CONFIG_FILE, ssid);

        if (ExecuteSystemCmd(command, temp, sizeof(temp)))
        {
          /* Vendor information populated matches with the current SSID */
          LOG_MSG_INFO1("DHCP Vendor Information is already set for the same SSID", 0, 0, 0);
          return false;
        }
        else
        {
          /* ssid must be changed before the reboot then delete the vendor information string and repopulate */
          QCMAP_LAN_CLIENT_RUN_COMMANDS("%s %d 1", DHCP_OPT_FILE, FEATURE_DHCP_VENDOR_INFO);
          LOG_MSG_INFO1("Successfully cleaned up the previously configured dhcp vendor information from dnsmasq config", 0, 0, 0);
        }
      }
    }
    return true;
  }
}


/*===========================================================================
  FUNCTION SetHWMACFilteringState()
  ===========================================================================*/
  /** @ingroup qcmap_set_hw_mac_filter_state

  Configure the HW Filtering State.

  @datatypes
  qcmap_config_state \n
  qcmap_hdw_filter_config \n
  @param[in] state                       Sets HW Filtering current state
  @param[in] hw_filter_config            HW Filter Configuration

  @return
  TRUE upon success. \n
  FALSE upon failure.

  @dependencies
  None
  */

  /*=========================================================================*/
bool QCMAP_LAN_Client::SetHWMACFilteringState
(
  qcmap_config_state                  state,
  qcmap_hdw_filter_config              hw_filter_config,
  qmi_error_type_v01                  *qmi_err_num
)
{
  bool ret = true;
  qcmap_config_state                   current_state;
  qcmap_hdw_filter_config              current_hw_filter_config;
  char result[QCMAP_MAX_SCAN_SIZE]={0};

  ZERO_INIT_ARG(current_state);
  ZERO_INIT_ARG(current_hw_filter_config);

  memset(result,0,QCMAP_MAX_SCAN_SIZE);

  if ( QCMAP_LAN_Client::UciGetUtility(owrt_filename[1], "firewall", 0,
       "hw_filtering_state", result) )
  {
    current_state = (qcmap_config_state)atoi(result);
  }

  /* Check if current state is already Disable and user is trying to Disable again
     then return QMI_ERR_NO_EFFECT */
  if(current_state == QCMAP_CONFIG_DISABLE && state == QCMAP_CONFIG_DISABLE)
  {
    LOG_MSG_ERROR("SetHWFilteringState(): HW Filtering State Already Disabled", 0, 0, 0);
    *qmi_err_num = QMI_ERR_INTERNAL_V01;
    return false;
  }


  if(current_state == QCMAP_CONFIG_ENABLE && state == QCMAP_CONFIG_DISABLE)
  {
    LOG_MSG_INFO1("SetHWFilteringState(): HW Filtering State Previously Enabled, Disableing the same", 0, 0, 0);
    if(!QCMAP_LAN_Client::ResetHWFilteringStateOnDisable(state))
    {
        *qmi_err_num = QMI_ERR_INTERNAL_V01;
      return false;
    }
    return true;
  }

  if(!QCMAP_LAN_Client::SendHWFilteringInfoToIPA(&hw_filter_config))
  {
    LOG_MSG_ERROR("Failed to Send HW Filtering State to IPA ", 0, 0, 0);
    *qmi_err_num = QMI_ERR_INTERNAL_V01;
    return false;
  }

#ifndef FEATURE_QCMAP_OFFTARGET
  if(!QCMAP_LAN_Client::GetSetHWFilteringStateFromUciConfig(SET_VALUE, &state, &hw_filter_config))
#else
  if(!QCMAP_LAN_Client::GetSetHWFilteringStateFromUciConfig(&state, &hw_filter_config))
#endif
  {
    LOG_MSG_ERROR("Failed to write HW Filtering State to xml", 0, 0, 0);
    *qmi_err_num = QMI_ERR_INTERNAL_V01;
    ret = false;
  }
  return ret;

}

/*===========================================================================
  FUNCTION GetHWFilteringState
==========================================================================*/
/*!
@brief
  Gets the current HW Filtering State.

@parameters
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

bool QCMAP_LAN_Client::GetHWFilteringState
(
  qcmap_config_state                  *status,
  qcmap_hdw_filter_config              *hw_filter_config,
  qmi_error_type_v01                  *qmi_err_num
)
{
  bool ret = true;

  if(hw_filter_config == NULL  ||  status == NULL)
  {
    LOG_MSG_ERROR("Null Parameters Passed ", 0, 0, 0);
    *qmi_err_num = QMI_ERR_INVALID_ARG_V01;
    return false;
  }

#ifndef FEATURE_QCMAP_OFFTARGET
  if(!QCMAP_LAN_Client::GetSetHWFilteringStateFromUciConfig(GET_VALUE, status, hw_filter_config))
#else
  if(!QCMAP_LAN_Client::GetSetHWFilteringStateFromUciConfig(status, hw_filter_config))
#endif
  {
    LOG_MSG_ERROR("Failed to Get HW Filtering Info From Uci Config ", 0, 0, 0);
    *qmi_err_num = QMI_ERR_INTERNAL_V01;
    ret = false;
  }

  LOG_MSG_INFO1("GetHWFilteringState(): status=%d",*status, 0,0);

  return ret;
}

/*===========================================================================
  FUNCTION ResetHWFilteringStateOnDisable
==========================================================================*/
/*!
@brief
  Reset Uci Config and IPA config when HW Filtering is disabled

@parameters
  qcmap_config_state state

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
bool QCMAP_LAN_Client::ResetHWFilteringStateOnDisable
(
  qcmap_config_state  state
)
{
  char command[MAX_COMMAND_STR_LEN]={0};
  char result[QCMAP_MAX_SCAN_SIZE]={0};

  if (!QCMAP_LAN_Client::UciSetUtility(owrt_filename[1], "firewall", 0,
      "hw_filtering_state", 0) )
  {
    return false;
  }

  snprintf( command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh delete qcmap_firewall.@if_name_filtering[0]");
  ds_system_call(command, strlen(command));

  snprintf( command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh delete qcmap_firewall.@ip_segment_filtering[0]");
  ds_system_call(command, strlen(command));

  snprintf( command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh delete qcmap_firewall.@mac_filtering[0]");
  ds_system_call(command, strlen(command));

  snprintf( command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh add qcmap_firewall mac_filtering");
  ds_system_call(command, strlen(command));


  if (!QCMAP_LAN_Client::UciSetUtility(owrt_filename[1], "mac_filtering", 0,
      "mac_filter_state", 0) )
  {
    return false;
  }

  if (!QCMAP_LAN_Client::UciSetUtility(owrt_filename[1], "mac_filtering", 0,
      "mac_filter_num_of_clients", 0) )
  {
    return false;
  }


  snprintf( command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh add qcmap_firewall ip_segment_filtering");
  ds_system_call(command, strlen(command));

  if (!QCMAP_LAN_Client::UciSetUtility(owrt_filename[1], "ip_segment_filtering", 0,
      "ip_segment_filter_state", 0) )
  {
    return false;
  }

  if (!QCMAP_LAN_Client::UciSetUtility(owrt_filename[1], "ip_segment_filtering", 0,
      "num_of_ip_segment", 0) )
  {
    return false;
  }

  snprintf( command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh add qcmap_firewall if_name_filtering");
  ds_system_call(command, strlen(command));


  if (!QCMAP_LAN_Client::UciSetUtility(owrt_filename[1], "if_name_filtering", 0,
      "iface_filter_state", 0) )
  {
    return false;
  }

  if (!QCMAP_LAN_Client::UciSetUtility(owrt_filename[1], "if_name_filtering", 0,
      "num_of_iface", 0) )
  {
    return false;
  }

  snprintf( command, MAX_COMMAND_STR_LEN, "ipa swflt --ip-disable");
  ds_system_call(command, strlen(command));

  snprintf( command, MAX_COMMAND_STR_LEN, "ipa swflt --mac-disable");
  ds_system_call(command, strlen(command));

  snprintf( command, MAX_COMMAND_STR_LEN, "ipa swflt --port-disable");
  ds_system_call(command, strlen(command));

  return true;
}

/*===========================================================================
FUNCTION ValidateBridgeContext()
===========================================================================*/
/** @ingroup qcmap_validate_bridge_context

  Checks if global bridge context is in saved vlan-id list of profile handle

  @param[in]      vlan_ids           list of vlan-id's mapped to the profile
  @param[in]      bridge_context     current global bridge context
  @param[in]      profile_handle     Current WWAN profile number

  @return
  TRUE -- Success \n
  FALSE -- Failure \n
*/
/*=========================================================================*/
boolean ValidateBridgeContext
(
  char *bridge_vlan_ids,
  const int16_t bridge_context,
  const uint32_t profile_handle
)
{
  char *ptr2 = NULL;
  char *vid = strtok_r(bridge_vlan_ids, " ", &ptr2);
  int16_t vlan_id = -1;

  while (vid != NULL)
  {
    /* Convert token to int */
    vlan_id = atoi(vid);
    /* Check if current global bridge context matches the mapped bridge ID */
    if (bridge_context == vlan_id)
    {
      LOG_MSG_INFO1(" Current global bridge context: %d is mapped to Profile number: %d",
                    bridge_context,
                    profile_handle,
                    0);
      return true;
    }
    vid = strtok_r(NULL, " ", &ptr2);
  }
  return false;
} /* End ValidateBridgeContext() */

/*===========================================================================
FUNCTION GetNumberOfProfiles()
===========================================================================*/
/** @ingroup qcmap_get_number_of_profiles

  gets number of profiles from qcmap_lan uci database

  @param[in]      no_of_profiles                 number of profiles

  @return
  TRUE -- Success \n
  FALSE -- Failure
*/
/*=========================================================================*/
boolean QCMAP_LAN_Client::GetNumberOfProfiles
(
  int *no_of_profiles
)
{
  char result[QCMAP_MAX_SCAN_SIZE] = {0};

  if (NULL == no_of_profiles)
  {
    LOG_MSG_ERROR("Null parameter of no_of_profiles passed", 0, 0, 0);
    return false;
  }

  /* get number of profiles */
  if (QCMAP_LAN_Client::UciGetUtility(owrt_filename[0], "no_of_configs", 0,
       "no_of_profiles", result))
  {
    *no_of_profiles = atoi(result);
    return true;
  }
  return false;
} /* End GetNumberOfProfiles() */

/*===========================================================================
FUNCTION GetPDNProfileNumber()
===========================================================================*/
/** @ingroup qcmap_get_pdn_profile_number

  gets the profile_id value saved per pdn from qcmap_lan uci database

  @param[in]      profile_idx                 profile index
  @param[in]      profile_id                  profile handle number

  @return
  TRUE -- Success \n
  FALSE -- Failure
*/
/*=========================================================================*/
boolean QCMAP_LAN_Client::GetPDNProfileNumber
(
  const int profile_idx,
  int *profile_id
)
{
  char result[QCMAP_MAX_SCAN_SIZE] = {0};

  if (NULL == profile_id)
  {
    LOG_MSG_ERROR("Null parameter of profile_id passed", 0, 0, 0);
    return false;
  }

  /* Get profile_id */
  if (QCMAP_LAN_Client::UciGetUtility(owrt_filename[0], "profile", 0,
       "profile_id", result, profile_idx))
  {
    *profile_id = atoi(result);
    return true;
  }
  return false;
} /* End GetPDNProfileNumber() */

/*===========================================================================
  FUNCTION DeleteDNSFromResolv
==========================================================================*/
/*!
@brief
  Deletes the DNS informatoin from resolv.conf file

@parameters
- char* primary dns addr
- char* secondary dns addr
- char* dns_search_str
- int max_buffer_len

@return
- None

@note

- Dependencies
- None

- Side Effects
- None
*/
/*=========================================================================*/
void QCMAP_LAN_Client::DeleteDNSFromResolv
(
  char                           *pri_dns_addr,
  char                           *sec_dns_addr,
  char                           *dns_search_str,
  boolean                         pri_dns_valid,
  boolean                         sec_dns_valid,
  boolean                         dns_search_valid,
  int                             max_buffer_len
)
{
  FILE* fp = NULL;
  char buffer[max_buffer_len] = {0};
  std::string temp, filtered_str;
  boolean str_found = false;

  /* Target independent */

  if(NULL == pri_dns_addr)
  {
    LOG_MSG_INFO1("primary dns addr given NULL!", 0, 0, 0);
  } else if(NULL == sec_dns_addr) {
    LOG_MSG_INFO1("secondary dns addr given NULL!", 0, 0, 0);
  } else if(NULL == dns_search_str) {
    LOG_MSG_INFO1("dns search string given NULL!", 0, 0, 0);
  }

 #ifndef FEATURE_QTIMAP_OFFTARGET
    //first filter out the IPv6 DNS addrs into filtered_str string
    if((fp = fopen(RESOLV_PATH, "r")) == NULL)
    {
      LOG_MSG_INFO1("error opening %s: %s", RESOLV_PATH, strerror(errno), 0);
      return;
    }

    filtered_str.clear();
    memset(buffer, 0, sizeof(buffer));
    while(fgets(buffer, max_buffer_len, fp) != NULL)
    {
      temp.clear();
      temp = buffer;
      str_found = false;

      if(pri_dns_valid && (NULL != pri_dns_addr))
      {
        if(strlen(pri_dns_addr) &&
            (std::string::npos != temp.find(pri_dns_addr)))
        {
          str_found = true;
        }
      }
      if(sec_dns_valid && (NULL != sec_dns_addr))
      {
        if(strlen(sec_dns_addr) &&
            (std::string::npos != temp.find(sec_dns_addr)))
        {
          str_found = true;
        }
      }
      if(dns_search_valid && (NULL != dns_search_str))
      {
        if(strlen(dns_search_str) &&
            (std::string::npos != temp.find(dns_search_str)))
        {
          str_found = true;
        }
      }
      if(!str_found)
      {
        filtered_str.append(temp);
      }
      memset(buffer, 0, sizeof(buffer));
    }

    if(fclose(fp))
    {
      LOG_MSG_INFO1("error closing %s: %s", RESOLV_PATH, strerror(errno), 0);
    }
    fp = NULL;

    //write the filtered lines
    if((fp = fopen(RESOLV_PATH, "w")) == NULL)
    {
      LOG_MSG_INFO1("error opening %s: %s", RESOLV_PATH, strerror(errno), 0);
      return;
    }

    if(fputs(filtered_str.c_str(), fp) < 0)
    {
      LOG_MSG_INFO1("error fputs %s: %s", RESOLV_PATH, strerror(errno), 0);
      goto end;
    }

    end:
    if(fclose(fp))
    {
      LOG_MSG_INFO1("error closing %s: %s", RESOLV_PATH, strerror(errno), 0);
    }
 #endif

  return;
}

/*=====================================================================
  FUNCTION GetV4PublicIP
======================================================================*/
/*!
@brief
  - Fetching the public IP from the /tmp/ipv4config File

@return
  true - Success
  false - Failure

@param
   char * ipv4_addr
   uint32_t profile_handle

@note
  - Dependencies
  - None

  - Side Effects
  - None
*/
/*=========================================================================*/
boolean QCMAP_LAN_Client::GetV4PublicIP(char * ipv4_addr,uint32_t profile_handle)
{
  char file_path[MAX_COMMAND_STR_LEN] = {0};
  char delimeter[INET_ADDRSTRLEN] = {0};
  char action[INET_ADDRSTRLEN] = {0};
  char command[MAX_COMMAND_STR_LEN] = {0};
  char result[INET_ADDRSTRLEN] = {0};
  int profile_idx = -1;

  LOG_MSG_ERROR("fetching profile index",0, 0, 0);
  /* Get profile_idx from qcmap_lan */
  profile_idx = GetProfileIndex(profile_handle);
  if (profile_idx != QCMAP_LAN_INVALID)
  {
    snprintf(file_path, MAX_COMMAND_STR_LEN, "/tmp/ipv4config%d", profile_handle);
    snprintf(delimeter, INET_ADDRSTRLEN, "\'\"\'");
    snprintf(action,    INET_ADDRSTRLEN, "\'{print $2}\'");

    if (CheckIfFileExists(file_path))
    {
      memset(result, 0, INET_ADDRSTRLEN);
      /*Fetching the public ip info from the ipv4config file*/
      snprintf(command, MAX_COMMAND_STR_LEN, "cat %s | grep PUBLIC_IP | awk -F %s %s",
        file_path, delimeter, action);

     if (!ExecuteSystemCmd((const char*)command, result, sizeof(result)))
     {
       LOG_MSG_ERROR("Failed to execute command : %s",command,0,0);
       return false;
     }
     LOG_MSG_INFO1("assigning public ip fetched from tmp/ipv4config file ",0, 0, 0);
     if (strlen(result) > 0 ) 
     {
       result[strlen(result)-1] = '\0';
     } 
     strlcpy(ipv4_addr, result, INET_ADDRSTRLEN);
     return true;
    }
    else
    {
      LOG_MSG_ERROR("file does'nt exist : %s",file_path,0,0);
      return false;
    }
  }
  return false;
}

/*===========================================================================
FUNCTION updateDNSSL
===========================================================================*/
/*!
  @brief
    Updates DNS Search List info
    Should only be called for ipv6 call and for default profile
  @parameters
    qcmap_wwan_backhaul_info *bn_info

  @return
  None

  @note
  - Dependencies
  - None

  - Side Effects
  - None
  */
/*=========================================================================*/
void QCMAP_LAN_Client::updateDNSSL

(
  qcmap_wwan_backhaul_info *bh_info
)
{
  FILE *resolv_file = NULL;
  FILE *config_file = NULL;
  uint32_t MAX_DNS_STRING_LEN = QCMAP_DOMAIN_NAME_MAX_V01*QCMAP_MAX_NUM_DNS_SEARCH_LIST;
  char dns_str[MAX_DNS_STRING_LEN] ={0};
  char cmd[QCMAP_MAX_COMMAND_LEN + MAX_DNS_STRING_LEN] = {0};
  char file_name[QCMAP_MAX_COMMAND_LEN] = {0};
  char buffer[MAX_DNS_STRING_LEN] = {0};
  std::string temp, filtered_str;

  /* Get /tmp/ipv6config file name */
  snprintf(file_name, QCMAP_MAX_COMMAND_LEN, "%s%s%d", CONFIG_FILE_PATH,
            IPV6_CONFIG_FILE_NAME, bh_info->profile_handle);

  /* delete old dns search list from /tmp/resolv.conf and /tmp/ipv6config */
  if ( QCMAP_LAN_Client::UciGetUtility(owrt_filename[4], "lan", 1, "domain", dns_str, 0) )
  {
    DeleteDNSFromResolv(NULL, NULL, dns_str, false,
      false, true, MAX_DNS_STRING_LEN);

    if((config_file = fopen(file_name, "r")) == NULL)
    {
      LOG_MSG_INFO1("error opening %s: %s", file_name, strerror(errno), 0);
      return;
    }
    filtered_str.clear();
    memset(buffer, 0, sizeof(buffer));
    while(fgets(buffer, MAX_DNS_STRING_LEN, config_file) != NULL)
    {
      temp.clear();
      temp = buffer;
      if(!(strlen(dns_str) &&
          (std::string::npos != temp.find(dns_str))))
      {
        filtered_str.append(temp);
      }
      memset(buffer, 0, sizeof(buffer));
    }
    if(fclose(config_file))
    {
      LOG_MSG_INFO1("error closing %s: %s", file_name, strerror(errno), 0);
    }
    config_file = NULL;

    //write the filtered lines
    if((config_file = fopen(file_name, "w")) == NULL)
    {
      LOG_MSG_INFO1("error opening %s: %s", file_name, strerror(errno), 0);
      return;
    }

    if(fputs(filtered_str.c_str(), config_file) < 0)
    {
      LOG_MSG_INFO1("error fputs %s: %s", file_name, strerror(errno), 0);
    }

    if(fclose(config_file))
    {
      LOG_MSG_INFO1("error closing %s: %s", file_name, strerror(errno), 0);
    }
    config_file = NULL;
  }

  /* Add DNS Search List */
  if (bh_info->dns_search_list_len != 0)
  {
    memset(dns_str, 0, sizeof(dns_str));
    dns_list_to_string(dns_str, MAX_DNS_STRING_LEN, bh_info->dns_search_list, bh_info->dns_search_list_len);
    snprintf(cmd, QCMAP_MAX_COMMAND_LEN + MAX_DNS_STRING_LEN, "%s %s \"%s\"", LAN_UTIL_SCRIPT, UPDATE_DNSSL, dns_str);
    ds_system_call(cmd, strlen(cmd));

    resolv_file = fopen(RESOLV_PATH, "a");
    if (resolv_file == NULL)
    {
      LOG_MSG_ERROR("Error opening file: %s, errno: %d", RESOLV_PATH, errno, 0);
    }
    else
    {
      fprintf(resolv_file, "search %s\n", dns_str);
    }

    config_file = fopen(file_name, "a");
    if (config_file == NULL)
    {
      LOG_MSG_ERROR("Error opening file: %s", file_name, 0, 0);
      return;
    }
    else
    {
      fprintf(config_file, "export DNSSEARCH=\"%s\"\n",dns_str);
    }

    /* flush command */
    if (config_file)
    {
      fflush(config_file);
      fclose(config_file);
      config_file = NULL;
    }
    if (resolv_file)
    {
      fflush(resolv_file);
      fclose(resolv_file);
      resolv_file = NULL;
    }
  }
}

/*=====================================================================
  FUNCTION ReplaceConfigItem
======================================================================*/
/*!
@brief
  - Replaces given line with new line in a file

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
bool  QCMAP_LAN_Client::ReplaceConfigItem
(
  string filename,
  string lineToDelete,
  string newLine
)
{
  bool found = false;
  vector <string> lines;
  string line;
  // Open file for read
  std::fstream r_file;
  r_file.open(filename.c_str(), std::fstream::in );
  if (r_file.is_open() == false)
  {
    LOG_MSG_ERROR("Failed to open file %s errno:%d", filename.c_str(), errno, 0);
    return false;
  }

  // Read file line by line
  while (getline(r_file, line))
  {
    // Check if the line matches the string to delete
    if (line.find(lineToDelete) != string::npos)
    {
      found = true;
      lines.push_back(newLine);
    }
    else
    {
       lines.push_back(line);
    }
  }
  // Close file
  r_file.close();

  if (!found)
  {
    LOG_MSG_ERROR("Failed to find line %s", lineToDelete.c_str(), 0, 0);
    return false;
  }

  /*write new content to file*/
  std::ofstream w_file;
  w_file.open(filename.c_str(), std::fstream::out);
  if (w_file.is_open() == false)
  {
    LOG_MSG_ERROR("Failed to open file %s to write errno:%d", filename.c_str(), errno, 0);
    return false;
  }

  for (int i=0; i<lines.size(); i++)
  {
    w_file << lines[i] << std::endl;
  }
  w_file.flush();
  w_file.close();

  return true;
}

/*=====================================================================
  FUNCTION updateWWANMTU
======================================================================*/
/*!
@brief
  - Updates WWAN MTU towards LAN side

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
  /*
     PerIP MTU Feature
    1. Update MTUoptions in UCI via .sh file
        v4 - list dhcp_option_force '26,v4mtu'
        v6 - option ra_mtu 'v6mtu'
    3. MSS Clamping => "mtu_fix 1" for WAN zone in firewall (taken care)
    4. Route MTU  => TBD
    ------ ASYNC IND ----
      1. Del old config
      2. update MTU Options in /tmp/ipconf file
      3. Add new config
     --- Add/delete lan mapping to PDN--- TODO
  */
bool QCMAP_LAN_Client::updateWWANMTU
(
  uint32_t       profile_idx,
  uint8_t        ip_type,
  uint16_t       mtu_info
)
{
  FILE *config_file = NULL;
  char replace_str[QCMAP_MAX_COMMAND_LEN] = {0};
  char file_name[QCMAP_MAX_COMMAND_LEN] = {0};
  int retval = -1;
  char command[QCMAP_MAX_COMMAND_LEN] = {0};

  if(ip_type == QCMAP_IP_FAMILY_V4 && mtu_info > 0)
  {
    /*delete current MTU options*/
    snprintf(command, QCMAP_MAX_COMMAND_LEN, "%s %s %u %u", LAN_UTIL_SCRIPT,
             QCMAP_WWAN_DELETE_MTU_OPTIONS, profile_idx, ip_type);
    ds_system_call(command, strlen(command));

    /* if not already, Create config file */
    snprintf(file_name, QCMAP_MAX_COMMAND_LEN, "%s%s%d", CONFIG_FILE_PATH,
              IPV4_CONFIG_FILE_NAME, profile_idx);
    string filename =  file_name;

    string find_line = "IPV4MTU" ;

    snprintf(replace_str, QCMAP_MAX_COMMAND_LEN, "export IPV4MTU=\"%d\"", mtu_info);
    string new_line= replace_str;

    ReplaceConfigItem(filename, find_line, new_line);

    snprintf(command, QCMAP_MAX_COMMAND_LEN, "%s %s %u %u", LAN_UTIL_SCRIPT,
             QCMAP_WWAN_ADD_MTU_OPTIONS, profile_idx, ip_type);
    ds_system_call(command, strlen(command));

    /*update MTU options*/
    snprintf(command, QCMAP_MAX_COMMAND_LEN, "%s %s %u %u", LAN_UTIL_SCRIPT,
             QCMAP_WWAN_UPDATE_MTU, profile_idx, ip_type);
    ds_system_call(command, strlen(command));

   /* Perform dnsmasq reload */
    LOG_MSG_INFO1("Perform dnsmasq reload", 0, 0, 0);
    PerformDnsmasqReload();
  }

  if(ip_type == QCMAP_IP_FAMILY_V6 && mtu_info > 0)
  {
    /*delete current MTU options*/
    snprintf(command, QCMAP_MAX_COMMAND_LEN, "%s %s %u %u", LAN_UTIL_SCRIPT,
             QCMAP_WWAN_DELETE_MTU_OPTIONS, profile_idx, ip_type);
    ds_system_call(command, strlen(command));

   /* if not already, Create config file */
    snprintf(file_name, QCMAP_MAX_COMMAND_LEN, "%s%s%d", CONFIG_FILE_PATH,
              IPV6_CONFIG_FILE_NAME, profile_idx);

    string filename =  file_name;

    string find_line = "IPV6MTU" ;

    snprintf(replace_str,QCMAP_MAX_COMMAND_LEN, "export IPV6MTU=\"%d\"", mtu_info);
    string new_line = replace_str;

    ReplaceConfigItem(filename, find_line, new_line);

    snprintf(command, QCMAP_MAX_COMMAND_LEN, "%s %s %u %u", LAN_UTIL_SCRIPT,
             QCMAP_WWAN_ADD_MTU_OPTIONS, profile_idx, ip_type);
    ds_system_call(command, strlen(command));

    /*update MTU options*/
    snprintf(command, QCMAP_MAX_COMMAND_LEN, "%s %s %u %u", LAN_UTIL_SCRIPT,
             QCMAP_WWAN_UPDATE_MTU, profile_idx, ip_type);
    ds_system_call(command, strlen(command));

    /* Perform odhcpd reload */
    LOG_MSG_INFO1("Perform odhcpd reload", 0, 0, 0);
    memset(command, 0, QCMAP_MAX_COMMAND_LEN);
    snprintf(command, QCMAP_MAX_COMMAND_LEN, "%s", ODHCPD_RELOAD_COMMAND);
    ds_system_call(command, strlen(command));
  }
  return true;
} /* End updateWWANMTU() */

/*=====================================================================
  FUNCTION GetDefaultProfilefromUCI
======================================================================*/
/*!
@brief
  - Gets the default profile handle information from qcmap_lan db

@return
  - int default_profile

@param[in]
  - None

@note
  - Dependencies
  - None

  - Side Effects
  - None
*/
/*=========================================================================*/
int GetDefaultProfilefromUCI()
{
  char cmd[QCMAP_MAX_COMMAND_LEN] = {0};
  char result[QCMAP_MAX_SCAN_SIZE] = {0};
  int default_profile;

  snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "%s %s",
             UCI_GET_COMMAND, QCMAP_GET_DEFUALT_PDN);
  if (!ExecuteSystemCmd((const char *)cmd, result,
                      sizeof(result)))
  {
    LOG_MSG_ERROR("Failed to get default_pdn value from qcmap_lan database", 0, 0, 0);
    return QCMAP_LAN_INVALID;
  }

  default_profile = atoi(result);
  if (default_profile >= NUMERIC_ZERO)
    return default_profile;
  else
    return QCMAP_LAN_INVALID;
} /* End GetDefaultProfilefromUCI() */

/*=====================================================================
  FUNCTION StopMWAN3
======================================================================*/
/*!
@brief
  - Issues command to stop mwan3 process

@return
  - void

@param[in]
  - None

@note
  - Dependencies
  - None

  - Side Effects
  - None
*/
/*=========================================================================*/
void StopMWAN3()
{
  char cmd[QCMAP_MAX_COMMAND_LEN] = {0};

  snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "%s",
             MWAN3_STOP);
  ds_system_call(cmd, strlen(cmd));
} /* End StopMWAN3() */

/*=====================================================================
  FUNCTION PerformStartStopMWAN3
======================================================================*/
/*!
@brief
  - Checks if ippt is enabled on Profile
  - Gets default profile number from qcmap_lan
  - Checks if profile handle is default profile
  - performs mwan3 stop

@return
  - void

@param[in]
  - profile handle
  - event (Bringup/teardown)

@note
  - Dependencies
  - None

  - Side Effects
  - None
*/
/*=========================================================================*/
void PerformStartStopMWAN3(const uint32_t profile_handle, const char *evt)
{
  qcmap_lan_ip_passthrough_mode_enum enable_state;
  int ippt_pdn_count, default_profile;
  int profile_idx = -1, active_ippt = -1;

  if (evt == MWAN3_STOP)
  {
    /* Check if ippt is enabled on profile handle */
    enable_state=(qcmap_lan_ip_passthrough_mode_enum)CheckEnableIPPT(profile_handle);

    if (enable_state == QCMAP_LAN_IP_PASSTHROUGH_MODE_UP)
    {
      /** Note: Indicates call needs to be brought up in ippt mode */
      /* Get ippt_pdn_count from qcmap_lan */
      /** NOTE: as of now, we are using this parameter for mwan handling only on default PDN.
       *  Can be scaled to on-demand PDN's if required */
      ippt_pdn_count = GetIPPTPDNCountfromUCI();

      /** Note: Since there might be possibility that an on-demand backhaul is already up
       * on IPPT mode, and ippt_pdn_count might be greater than 0, adding a temporary
       * check to query active_ippt parameter associated with curent profile_handle.
       * Condition: is ippt_pdn_count is 0 OR active_ippt parameter is 0.
       * Later inside the if condition we proceed only if it is default_pdn.
       * In this way, it is contained to only default_pdn */

      /* Get profile_idx from qcmap_lan */
      profile_idx = GetProfileIndex(profile_handle);
      if (profile_idx != QCMAP_LAN_INVALID)
      {
         /* Check if active_ippt is enabled */
          active_ippt = GetActiveIPPT(profile_idx);
      }

      if ((ippt_pdn_count == NUMERIC_ZERO) || (active_ippt == IPPT_NOT_ACTIVE))
      {
        LOG_MSG_INFO1(" IPPT PDN Count: %d", 0, 0, 0);
        /* Get default profile number from qcmap_lan */
        default_profile = GetDefaultProfilefromUCI();

        if (default_profile == QCMAP_LAN_INVALID)
        {
          LOG_MSG_ERROR(" Invalid default profile number received from qcmap_lan db", 0, 0, 0);
        }
        else
        {
          /* Check if profile handle is default handle */
          if (profile_handle == (uint32_t)default_profile)
          {
            /**Note: Indicates profile handle is default profile */
            /* Stop mwan3 */
            StopMWAN3();
          }
        }
      }
    }
  }
  else if (evt == MWAN3_START)
  {
    /** NOTE: All checks would've been completed at the pre-processing stage.
     *  We can directly start mwan3 at this stage
     *  Assumption: The decrementing of "ippt_pdn_count" will be taken care from
     *  teardown_interface() of rmnet.script file */
    /* Start mwan3 */
    StartMWAN3();
  }
  return;
} /* End PerformStartStopMWAN3() */

/*=====================================================================
  FUNCTION CheckMWANHandling
======================================================================*/
/*!
@brief
  - Checks if ippt is enabled on Profile
  - Gets default profile number from qcmap_lan
  - Checks if profile handle is default profile
  - Returns a boolean value if post-processing is required

@return
    true - Success
    false - Failure

@param[in]
  - profile handle

@note
  - Dependencies
  - None

  - Side Effects
  - None
*/
/*=========================================================================*/
boolean CheckMWANHandling(const uint32_t profile_handle, const char *evt)
{
  int default_profile;
  int profile_idx = -1, active_ippt = -1;
  qcmap_lan_ip_passthrough_mode_enum enable_state;

  /* Check if profile handle is same as default handle */
  default_profile = GetDefaultProfilefromUCI();

  if (profile_handle == (uint32_t)default_profile)
  {
    /* Check if EnableIPPT and Active IPPT is set */
    enable_state = (qcmap_lan_ip_passthrough_mode_enum)CheckEnableIPPT(profile_handle);

    if ((evt == RMNET_BRING_UP_CMD && enable_state == QCMAP_LAN_IP_PASSTHROUGH_MODE_DOWN) ||
        (evt == RMNET_TEAR_DOWN_CMD && enable_state == QCMAP_LAN_IP_PASSTHROUGH_MODE_UP))
    {
      /* Get profile_idx from qcmap_lan */
      profile_idx = GetProfileIndex(profile_handle);
      if (profile_idx != QCMAP_LAN_INVALID)
      {
        /* Check if active_ippt is enabled */
        active_ippt = GetActiveIPPT(profile_idx);
        if (active_ippt == 1)
        {
          /* If yes, indicates we need to start mwan as part of
             post processing */
          return true;
        }
      }
    }
  }
  return false;
} /* End CheckMWANHandling() */

/*=====================================================================
  FUNCTION StartMWAN3
======================================================================*/
/*!
@brief
  - Issues command to start mwan3 process

@return
  - void

@param[in]
  - None

@note
  - Dependencies
  - None

  - Side Effects
  - None
*/
/*=========================================================================*/
void StartMWAN3()
{
  char cmd[QCMAP_MAX_COMMAND_LEN] = {0};

  snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "%s",
           MWAN3_START);
  ds_system_call(cmd, strlen(cmd));
} /* End StartMWAN3() */

/*=====================================================================
  FUNCTION UpdateWWANPolicy
======================================================================*/
/*!
@brief
  - Updates WWAN Policy on lan lib

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
bool QCMAP_LAN_Client::UpdateWWANPolicy
(
  uint32_t  current_profile_handle,
  qmi_error_type_v01 *qmi_err_num
)
{
  uint32_t  default_profile=0;
  char command[MAX_COMMAND_STR_LEN]={0};
  char result[MAX_COMMAND_STR_LEN]={0};
  snprintf(command, MAX_COMMAND_STR_LEN, QCMAP_GET_DEFAULT_PROFILE_ID);
  if (!ExecuteSystemCmd((const char *)command, result, sizeof(result)))
  {
    LOG_MSG_ERROR("Failed to get default pdn profile, update fails", 0, 0, 0);
    *qmi_err_num = QMI_ERR_INTERNAL_V01;
    return false;
  }
  default_profile = atoi(result);
  if(current_profile_handle != default_profile)
  {
      memset(command, 0, MAX_COMMAND_STR_LEN);
      memset(result, 0, MAX_COMMAND_STR_LEN);
      snprintf(command, MAX_COMMAND_STR_LEN,
                        "%s %s %d %d",
                        BACKHAUL_WWAN_CONFIG_FILE,
                        UPDATE_DEFAULT_PROFILE,
                        current_profile_handle,
                        default_profile);
      if (!ExecuteSystemCmd((const char *)command, result, sizeof(result)))
      {
        LOG_MSG_ERROR("Fail to execute command :%s",command,0,0);
        *qmi_err_num = QMI_ERR_INTERNAL_V01;
        return false;
      }
  }
  return true;
}

/*=====================================================================
  FUNCTION SetDhcpv6DNSConfig
======================================================================*/
/*!
@brief
  - Triggers add/del_dns_options when Proxy DNS v6 is enabled

@return
  - True if success else false

@param[in]
  -   qcmap_config_state     dhcpv6_dns_state
      qmi_error_type_v01    *qmi_err_num

@note
  - Dependencies
  - None

  - Side Effects
  - None
*/
/*=========================================================================*/
boolean QCMAP_LAN_Client::SetDhcpv6DNSConfig
(
  qcmap_config_state     dhcpv6_dns_state,
  qmi_error_type_v01    *qmi_err_num
)
{
  char command[QCMAP_MAX_COMMAND_LEN] = {0};
  qcmap_msgr_config_state_enum_v01 current_dhcpv6_dns_state;

  if (!qmi_err_num)
    return false;
  *qmi_err_num = QMI_ERR_NONE_V01;

  LOG_MSG_INFO1("SetDhcpv6DNSConfig %d", dhcpv6_dns_state, 0, 0);

  if(dhcpv6_dns_state != QCMAP_CONFIG_DISABLE && dhcpv6_dns_state != QCMAP_CONFIG_ENABLE)
  {
    LOG_MSG_ERROR("Invalid dhcpv6_dns_state :%d",dhcpv6_dns_state,0,0);
    *qmi_err_num = QMI_ERR_INVALID_ARG_V01;
    return false;
  }

  /*get current value*/
  if(!QCMAP_LAN_Client::GetDhcpv6DNSConfig(&current_dhcpv6_dns_state, qmi_err_num))
  {
    LOG_MSG_ERROR("Fail to get current state :%d",*qmi_err_num,0,0);
    *qmi_err_num = QMI_ERR_INTERNAL_V01;
     return false;
  }

  if(dhcpv6_dns_state == (qcmap_config_state)current_dhcpv6_dns_state)
  {
    LOG_MSG_ERROR("Duplicate request to set dhcpv6_dns_state :%d",dhcpv6_dns_state,0,0);
    *qmi_err_num = QMI_ERR_NO_EFFECT_V01;
    return false;
  }

  if (dhcpv6_dns_state == QCMAP_CONFIG_DISABLE)
  {
    /*As proxy is enabled, delete current DNS options if any in UCI*/
    if (!QCMAP_LAN_Client::UciSetUtility(owrt_filename[0], "global", 0,
         "proxydnsv6", (int)dhcpv6_dns_state))
    {
      *qmi_err_num = QMI_ERR_INTERNAL_V01;
      return false;
    }

    /*Add current DNS options*/
    snprintf(command, QCMAP_MAX_COMMAND_LEN, "%s %s", LAN_UTIL_SCRIPT,
             QCMAP_LAN_ADD_DNSV6_OPTIONS);
    ds_system_call(command, strlen(command));
  }
  else if (dhcpv6_dns_state == QCMAP_CONFIG_ENABLE)
  {
    /*As proxy is Disabled, add DNS options to UCI, for sending nw dns to clients*/
    snprintf(command, QCMAP_MAX_COMMAND_LEN, "%s %s", LAN_UTIL_SCRIPT,
           QCMAP_LAN_DEL_DNSV6_OPTIONS);
    ds_system_call(command, strlen(command));

    if (!QCMAP_LAN_Client::UciSetUtility(owrt_filename[0], "global", 0,
         "proxydnsv6", (int)dhcpv6_dns_state))
    {
      *qmi_err_num = QMI_ERR_INTERNAL_V01;
      return false;
    }
  }

  /* Perform odhcpd reload */
  LOG_MSG_INFO1("Perform odhcpd reload", 0, 0, 0);
  memset(command, 0, QCMAP_MAX_COMMAND_LEN);
  snprintf(command, QCMAP_MAX_COMMAND_LEN, "%s", ODHCPD_RELOAD_COMMAND);
  ds_system_call(command, strlen(command));

  return true;
}


/*=====================================================================
  FUNCTION GetDhcpv6DNSConfig
======================================================================*/
/*!
@brief
  - Get current Proxy DNS v6 value

@return
  - True if success else false

@param[in]
  qcmap_msgr_config_state_enum_v01 *dhcpv6_dns_state
  qmi_error_type_v01               *qmi_err_num

@note
  - Dependencies
  - None

  - Side Effects
  - None
*/
/*=========================================================================*/
boolean QCMAP_LAN_Client::GetDhcpv6DNSConfig
(
  qcmap_msgr_config_state_enum_v01 *dhcpv6_dns_state,
  qmi_error_type_v01               *qmi_err_num
)
{
  char result[QCMAP_MAX_SCAN_SIZE]={0};

  if (!qmi_err_num)
    return false;
  *qmi_err_num = QMI_ERR_NONE_V01;

  if (!QCMAP_LAN_Client::UciGetUtility(owrt_filename[0], "global", 0, "proxydnsv6", result, 0) )
  {
    *qmi_err_num = QMI_ERR_INTERNAL_V01;
    return false;
  }

  *dhcpv6_dns_state=(qcmap_msgr_config_state_enum_v01)atoi(result);
  return true;
}

/*===========================================================================
  FUNCTION SetUPNPState
  ===========================================================================*/
/*!
  @brief
  Sets the UPNP State

  @return
  true  - on Success
  false - on Failure

  - Dependencies
  - None

  - Side Effects
  - None
 */
/*=========================================================================*/
boolean QCMAP_LAN_Client::SetUPNPState
(
  boolean              upnp_pinhole_flag
)
{
 QCMAP_LAN_CLIENT_RUN_COMMANDS("/etc/data/uci_ex.sh set qcmap_lan.@no_of_configs[0].upnp_pinhole_flag='%d'", upnp_pinhole_flag);
 return true;
}

/*===========================================================================
  FUNCTION GetUPNPStatus
  ===========================================================================*/
/*!
  @brief
  Gets the UPNP state

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
boolean QCMAP_LAN_Client::GetUPNPState
(
  boolean *upnp_pinhole_flag,
  qmi_error_type_v01   *qmi_err_num
)
{
 char command[QCMAP_MAX_COMMAND_LEN] = {0};
 char result[QCMAP_MAX_SCAN_SIZE] = {0};
 snprintf(command, MAX_COMMAND_STR_LEN,"/etc/data/uci_ex.sh get qcmap_lan.@no_of_configs[0].upnp_pinhole_flag");
 if (!ExecuteSystemCmd((const char*)command,result,sizeof(result)))
 {
   LOG_MSG_ERROR("Failed to execute command : %s",command,0,0);
   *qmi_err_num = QMI_ERR_INTERNAL_V01;
   return false;
 }
 (*upnp_pinhole_flag) = atoi(result);
 return true;
}

/*===========================================================================
  FUNCTION ConfigureNetworkOnEthPduModeChange
==========================================================================*/
/*!
@brief
  Configur Network On Eth Pdu Mode Change

@parameters
  eth pdu mode
  device index

@return
  None

@note

- Dependencies
- None

- Side Effects
- None
*/
/*=========================================================================*/
void QCMAP_LAN_Client::ConfigureNetworkOnEthPduModeChange(qcmap_lan_eth_pdu_feature_mode_enum EthPduMode,uint8_t index)
{

  LOG_MSG_INFO1("ETH PDU: Configure eth pdu mode to %d,eth device index:%d.",EthPduMode, index, 0);
  char cmd[QCMAP_MAX_COMMAND_LEN] = {0};

  /* Perform eth pdu mode change */
  if(ETH_PDU_MODE_ENABLE == EthPduMode)
  {
    snprintf(cmd, MAX_COMMAND_STR_LEN,"%s %s %d","/etc/data/backhaulEthPduConfig.sh",ETHPDU_MODE_ENABLE_CMD,index);
    ds_system_call(cmd, strlen(cmd));
  }
  else if(ETH_PDU_MODE_DISABLE == EthPduMode)
  {
    snprintf(cmd, MAX_COMMAND_STR_LEN,"%s %s %d","/etc/data/backhaulEthPduConfig.sh",ETHPDU_MODE_DISABLE_CMD,index);
    ds_system_call(cmd, strlen(cmd));
  }
  else
  {
    LOG_MSG_ERROR("Invalid eth pdu mode!", 0, 0, 0);
  }
}

/*===========================================================================
FUNCTION SetCoexConfig()
===========================================================================*/
/*
  Enable/Disable the CoEX channel avoidance to reduce co-channel interference
  between WLAN <-> WWAN channels.

  @param[in]  coex_state   Enable/Disable CoEX channel avoidance.

  @return
  TRUE -- Success.
  FALSE -- Failure.

  @dependencies
  None
*/
boolean QCMAP_LAN_Client::SetCoexConfig
(
  int coex_state
)
{

  if(coex_state == 2) /* To Enable Coex = 1   To Disable Coex = 2 */
    coex_state = 0;
  m_pQCMapWlanObj->SetCoexConfig(coex_state);
  return true;
}
/*===========================================================================
FUNCTION  WhitelistWLANChannels
==========================================================================*/
/*!
@brief
Whitelist all the blacklisted WLAN channels and trigger ACS.

@return
QCMAP_CM_SUCCESS
QCMAP_CM_ERROR

@note

- Dependencies
- None

- Side Effects
- None
*/
/*=========================================================================*/
int32_t QCMAP_LAN_Client::WhitelistWLANChannels()
{
  char cmd[QCMAP_MAX_SCAN_SIZE] = {0};
  UCI_WLAN_GET_STR_OPTION(cmd, "primaryap", "ifname")
  char wlan_iface[QCMAP_MAX_IFACE_NAME_SIZE] = {0};

  strlcpy(wlan_iface, cmd, QCMAP_MAX_IFACE_NAME_SIZE);

  /* Whitelist all the blacklisted channels on a specific radio */
  QCMAP_LAN_CLIENT_RUN_COMMANDS("wifitool %s block_acs_channel 0",wlan_iface);

  /* Trigger ACS after whitelisting the channels on a specific radio */
  QCMAP_LAN_CLIENT_RUN_COMMANDS("cfg80211tool %s channel 0",wlan_iface);

  LOG_MSG_INFO1(" While listing WLAN Channels is done.", 0, 0, 0);
  return QCMAP_CM_SUCCESS;
}

/*===========================================================================
FUNCTION GetLANConfig()
===========================================================================*/
/** @ingroup section_GetLANConfig

  Get LAN config

  @param[in] qcmap_lan_config   *lan_config
  @param[in].qmi_error_type_v01    *qmi_err_num

  @return
  TRUE -- Success \n
  FALSE -- Failure \n

  @dependencies
  QCMobileAP must be enabled. @newpage
  Current bridge context is stored in the qcmap_lan.
  Current bridge context will be used for this API.
*/
/*=========================================================================*/
boolean QCMAP_LAN_Client::GetLANConfig
(
  qcmap_lan_config *lan_config,
  qmi_error_type_v01 *qmi_err_num
)
{
  char result[QCMAP_MAX_SCAN_SIZE] = {0};
  char command[QCMAP_MAX_COMMAND_LEN] = {0};
  int dhcp_limit;
  in_addr addr;
  int16_t bridge_id;
  char lan_interface[QCMAP_MAX_COMMAND_LEN] = {0};

  QCMAP_LOG_FUNC_ENTRY();

  if ( lan_config == NULL )
  {
    LOG_MSG_ERROR("Invalid lan_config passed ",0,0,0);
    *qmi_err_num = QMI_ERR_INVALID_ARG_V01;
    return false;
  }

  if (!QCMAP_LAN_Client::GetBridgeVLANContext(&bridge_id))
  {
    LOG_MSG_ERROR("Failed to get the current bridge context", 0, 0, 0);
    *qmi_err_num = QMI_ERR_INTERNAL_V01;
    return false;
  }

  /* Send all information to script */
  if (bridge_id != 0)
  {
    snprintf(lan_interface, QCMAP_MAX_COMMAND_LEN, "%s%u","lan",bridge_id);
  }
  else
  {
    snprintf(lan_interface, QCMAP_MAX_COMMAND_LEN, "%s","lan");
  }

  /* Get gw_ip from network config */
  if ( QCMAP_LAN_Client::UciGetUtility(owrt_filename[8], lan_interface,
                                                   1, LAN_IP, result) )
  {
    if ( !(inet_aton(result, &addr) <= 0) )
    {
      lan_config->gw_ip = ntohl(addr.s_addr);
    }
    else
    {
      LOG_MSG_ERROR("Invalid gw_ip address obtained",0,0,0);
      *qmi_err_num = QMI_ERR_INTERNAL_V01;
      return false;
    }
  }
  else
  {
    LOG_MSG_ERROR("Failed to get gw_ip",0,0,0);
    *qmi_err_num = QMI_ERR_INTERNAL_V01;
    return false;
  }
  /* Get netmask from network config */
  if ( QCMAP_LAN_Client::UciGetUtility(owrt_filename[8], lan_interface,
                                              1, LAN_NETMASK, result) )
  {
    if ( !(inet_aton(result, &addr) <= 0) )
    {
      lan_config->netmask = ntohl(addr.s_addr);
    }
    else
    {
      LOG_MSG_ERROR("Invalid netmask address obtained",0,0,0);
      *qmi_err_num = QMI_ERR_INTERNAL_V01;
      return false;
    }
  }
  else
  {
    LOG_MSG_ERROR("Failed to get netmask",0,0,0);
    *qmi_err_num = QMI_ERR_INTERNAL_V01;
    return false;
  }
  /* Get dhcp_enable */
  if ( QCMAP_LAN_Client::UciGetUtility(owrt_filename[8], lan_interface,
                                              1, DHCP_IGNORE, result) )
  {
    /* If dhcp is enabled get dhcp_start and dhcp_limit values */
    int dhcp_ignore=atoi(result);
    if ( dhcp_ignore == 0 )
    {
      lan_config->enable_dhcp = 1;
      if ( QCMAP_LAN_Client::UciGetUtility(owrt_filename[8], lan_interface,
                                                   1, DHCP_START, result) )
      {
        if ( !(inet_aton(result, &addr) <= 0) )
        {
          lan_config->dhcp_config.dhcp_start_ip = ntohl(addr.s_addr);
        }
        else
        {
          LOG_MSG_ERROR("Invalid dhcp_start address obtained",0,0,0);
          *qmi_err_num = QMI_ERR_INTERNAL_V01;
          return false;
        }
        /* Update dhcp_ip by appending dhcp_start value to subnet of gw_ip */
        lan_config->dhcp_config.dhcp_start_ip = ((lan_config->gw_ip &
                                                  lan_config->netmask) |
                                                  lan_config->dhcp_config.dhcp_start_ip);
      }
      else
      {
        LOG_MSG_ERROR("Failed to get dhcp_start value",0,0,0);
        *qmi_err_num = QMI_ERR_INTERNAL_V01;
        return false;
      }
      /* Get dhcp_limit and calculate dhcp_end_ip */
      if ( QCMAP_LAN_Client::UciGetUtility(owrt_filename[8], lan_interface,
                                                   1, DHCP_LIMIT, result) )
      {
        dhcp_limit=atoi(result);
        /* Update dhcp_end value */
        lan_config->dhcp_config.dhcp_end_ip = lan_config->dhcp_config.dhcp_start_ip +
                                              dhcp_limit -1;
      }
      else
      {
        LOG_MSG_ERROR("Failed to get dhcp_limit value",0,0,0);
        *qmi_err_num = QMI_ERR_INTERNAL_V01;
        return false;
      }
      if ( QCMAP_LAN_Client::UciGetUtility(owrt_filename[8], lan_interface,
                                               1, DHCP_LEASETIME, result) )
      {
        if(strlen(result) > 0)
        {
          char lease_time_unit = result[strlen(result)-1];
          char* rest = result;
          char *lease_time;

          if (lease_time_unit == 'h')
          {
            // Lease time is in hours. Convert to seconds
           lease_time = strtok_r(result, "h",&rest);
            if(lease_time != NULL)
            {
              lan_config->dhcp_config.lease_time = atoi(lease_time)*60*60;
            }
          }
          else
          {
            lan_config->dhcp_config.lease_time = atoi(result);
          }
        }
      }
      else
      {
        LOG_MSG_ERROR("Failed to get dhcp_lease_time value",0,0,0);
        *qmi_err_num = QMI_ERR_INTERNAL_V01;
        return false;
      }
    }
  }
  else
  {
    LOG_MSG_ERROR("Failed to get dhcp_ignore_value",0,0,0);
    *qmi_err_num = QMI_ERR_INTERNAL_V01;
    return false;
  }
  return true;
}

/*=====================================================================
  FUNCTION IsLanCfgUpdated
======================================================================*/
/*!
@brief
  Check is current applied LAN config and the prev LAN config are the same

@parameters
   none

@return
   returns succesful if prev/running and current lan config are different

@note
- Dependencies
- None

- Side Effects
- None
*/
/*====================================================================*/
boolean QCMAP_LAN_Client::IsLanCfgUpdated(void)
{
  qcmap_lan_config lan_config_current, lan_config_active;
  qmi_error_type_v01 qmi_err_num;
  int16_t current_bridge_context = -1;

  // Get the user provided current lan config
  if(!QCMAP_LAN_Client::GetLANConfig(&lan_config_current, &qmi_err_num))
  {
    LOG_MSG_ERROR("Failed to Get the current lan config, Error:0x%x", qmi_err_num,0,0);
    return false;
  }

  // Get the present running/active lan config
  if(!QCMAP_LAN_Client::GetActiveLANConfig(&lan_config_active, &qmi_err_num))
  {
    LOG_MSG_ERROR("Failed to Get the active running lan config",0,0,0);
    return false;
  }

  if((lan_config_active.gw_ip != lan_config_current.gw_ip) ||
     (lan_config_active.netmask != lan_config_current.netmask) ||
     (lan_config_active.enable_dhcp != lan_config_current.enable_dhcp))
  {
    return true;
  }

  if(lan_config_current.enable_dhcp)
  {
    if((lan_config_active.dhcp_config.dhcp_start_ip != lan_config_current.dhcp_config.dhcp_start_ip) ||
       (lan_config_active.dhcp_config.dhcp_end_ip != lan_config_current.dhcp_config.dhcp_end_ip) ||
       (lan_config_active.dhcp_config.lease_time != lan_config_current.dhcp_config.lease_time))
    {
      return true;
    }
  }

  if (!QCMAP_LAN_Client::GetBridgeVLANContext(&current_bridge_context))
  {
    LOG_MSG_ERROR("Failed to get the current bridge context", 0, 0, 0);
    return false;
  }

  if ((current_bridge_context == DEFAULT_BRIDGE_ID) &&
      (this->dhcp_reservations_updated == true))
  {
    LOG_MSG_INFO1("DHCP reservation record got updated",0,0,0);
    return true;
  }

  return false;
}

/*===========================================================================
FUNCTION ResetFeatureMode()
===========================================================================*/
/** @ingroup qcmap_reset_feature_mode

  Resets feature mode provided by the user

  @datatypes
  qcmap_lan_client_feature_mode_config

  @param[in]      feature_mode_config   feature mode struct

  @return
  TRUE -- Success \n
  FALSE -- Failure \n
*/
/*=========================================================================*/
boolean QCMAP_LAN_Client::ResetFeatureMode
(
  qcmap_lan_client_feature_mode_config *feature_mode_config,
  qmi_error_type_v01                  *qmi_err_num
)
{
  char cmd[QCMAP_MAX_COMMAND_LEN] = {0};
  char result[QCMAP_MAX_SCAN_SIZE] = {0};

  if (NULL == feature_mode_config)
  {
    LOG_MSG_ERROR("Invalid arguments passed", 0, 0, 0);
    *qmi_err_num = QMI_ERR_INVALID_ARG_V01;
    return false;
  }

  /*Check if its ipsec feature*/
  if (feature_mode_config->ipsec_feature_valid)
  {
    return ResetIPsecFeatureMode(feature_mode_config,qmi_err_num);
  }

  /* Check if feature is IPPT */
  if (feature_mode_config->ip_passthrough_feature_valid)
  {
    return ResetIPPTFeatureMode(feature_mode_config,qmi_err_num);
  }
  else if (feature_mode_config->eth_pdu_feature_valid)
  {
    /* Check if saved feature_mode is same as user defined feature_mode */
    memset(cmd, 0, QCMAP_MAX_COMMAND_LEN);
    memset(result, 0, QCMAP_MAX_SCAN_SIZE);
    snprintf(cmd, QCMAP_MAX_COMMAND_LEN, " %s %s",
              UCI_GET_COMMAND, QCMAP_ETH_PDU_FEATURE_MODE);
    if (ExecuteSystemCmd((const char *)cmd, result,
                    sizeof(result)))
    {
      int feature_mode = atoi(result);
      if (feature_mode_config->eth_pdu_feature_mode == (qcmap_lan_eth_pdu_feature_mode_enum)feature_mode)
      {
        LOG_MSG_INFO1("User defined eth pdu feature mode already set", 0, 0, 0);
        return true;
      }

      /*Set to default value*/
      memset(cmd, 0, QCMAP_MAX_COMMAND_LEN);
      snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "%s %s=%d",
               UCI_SET_COMMAND, QCMAP_ETH_PDU_FEATURE_MODE,ETH_PDU_MODE_DISABLE);
      ds_system_call(cmd, strlen(cmd));

      /* Commit uci */
      ExecuteUCICommit();

      ConfigureNetworkOnEthPduModeChange(ETH_PDU_MODE_DISABLE,ETH_PDU_INVALID_DEVICE_INDEX);
      return true;
    }
    else
    {
      LOG_MSG_ERROR(" Unable to query ETH PDU feature mode", 0, 0, 0);
      return false;
    }
  }
  else
  {
    LOG_MSG_ERROR("Not an IP Passthrough feature!", 0, 0, 0);
    *qmi_err_num = QMI_ERR_INTERNAL_V01;
    //return false;
  }

  if (feature_mode_config->dhcp_lan_options_feature_valid)
  {
    if (feature_mode_config->dhcp_lan_options_feature_modes == QCMAP_MSGR_MASK_DHCP_VENDOR_INFORMATION_V01)
    {
      memset(cmd, 0, QCMAP_MAX_COMMAND_LEN);
      snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "%s %s=%d",
               UCI_SET_COMMAND, "qcmap_lan.@dhcp_option[0].dhcp_vendor_info_enable", 0);
      ds_system_call(cmd, strlen(cmd));
    }
    else if(feature_mode_config->dhcp_lan_options_feature_modes == QCMAP_MSGR_MASK_DHCP_TIMEZONE_INFORMATION_V01)
    {
      memset(cmd, 0, QCMAP_MAX_COMMAND_LEN);
      snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "%s %s=%d",
               UCI_SET_COMMAND, "qcmap_lan.@dhcp_option[0].dhcp_tz_enable", 0);
      ds_system_call(cmd, strlen(cmd));
    }

    /* Commit uci */
    ExecuteUCICommit();
    return true;
  }
  else
  {
    LOG_MSG_ERROR("Not a DHCP options feature!", 0, 0, 0);
    return false;
  }

  return true;
} /* End ResetFeatureMode */

/*===========================================================================
FUNCTION GetFeatureMode()
===========================================================================*/
/** @ingroup qcmap_get_feature_mode

  gets feature mode requested by the user

  @datatypes
  qcmap_lan_client_feature_mode_config

  @param[in]      feature_mode_config   feature mode struct

  @return
  TRUE -- Success \n
  FALSE -- Failure \n
*/
/*=========================================================================*/
boolean QCMAP_LAN_Client::GetFeatureMode
(
  uint64_t                             *enabled_features,
  qcmap_lan_client_feature_mode_config *feature_mode_config,
  qmi_error_type_v01                   *qmi_err_num
)
{
  char cmd[QCMAP_MAX_COMMAND_LEN] = {0};
  char result[QCMAP_MAX_SCAN_SIZE] = {0};

  if (NULL == feature_mode_config)
  {
    LOG_MSG_ERROR("Invalid arguments passed", 0, 0, 0);
    *qmi_err_num = QMI_ERR_INVALID_ARG_V01;
    return false;
  }

  /*For IPsec*/
  GetIPsecFeatureMode(enabled_features,feature_mode_config, qmi_err_num);
  

  /* For IP Passthrough */
  GetIPPTFeatureMode(enabled_features,feature_mode_config, qmi_err_num);

  /* For ETH PDU feature mode */
  memset(cmd, 0, QCMAP_MAX_COMMAND_LEN);
  memset(result, 0, QCMAP_MAX_SCAN_SIZE);
  snprintf(cmd, QCMAP_MAX_COMMAND_LEN, " %s %s",
            UCI_GET_COMMAND, QCMAP_ETH_PDU_FEATURE_MODE);
  if (ExecuteSystemCmd((const char *)cmd, result,
                        sizeof(result)))
  {
    int feature_mode = atoi(result);
    if ((qcmap_lan_eth_pdu_feature_mode_enum)feature_mode == ETH_PDU_MODE_DISABLE)
    {
      LOG_MSG_INFO1("Eth pdu feature disable.",0, 0, 0);
    }
    else if ((qcmap_lan_eth_pdu_feature_mode_enum)feature_mode == ETH_PDU_MODE_ENABLE)
    {
      /* Populate feature mode config */
      *enabled_features |= QCMAP_MSGR_MASK_ENABLE_FEATURE_ETH_PDU_V01;
      LOG_MSG_INFO1("Eth pdu feature enable.",0, 0, 0);
    }
    else
    {
      LOG_MSG_ERROR("Wrong feature mode:%d!",feature_mode, 0, 0);
    }
  }
  else
  {
    LOG_MSG_ERROR("Get Eth Pdu feature mode failed", 0, 0, 0);
    return false;
  }

  /* For DHCP options feature mode */
  GetDHCPLANOptionsFeatureModes(enabled_features,feature_mode_config, qmi_err_num);

  return true;
} /* End GetFeatureMode */

/*===========================================================================
FUNCTION SetFeatureMode()
===========================================================================*/
/** @ingroup qcmap_set_feature_mode

  Sets feature mode provided by the user

  @datatypes
  qcmap_lan_client_feature_mode_config

  @param[in]      feature_mode_config   feature mode struct

  @return
  TRUE -- Success \n
  FALSE -- Failure \n
*/
/*=========================================================================*/
boolean QCMAP_LAN_Client::SetFeatureMode
(
  qcmap_lan_client_feature_mode_config *feature_mode_config,
  qmi_error_type_v01 * qmi_err_num
)
{
  char cmd[QCMAP_MAX_COMMAND_LEN] = {0};
  char result[QCMAP_MAX_SCAN_SIZE] = {0};

  if (NULL == feature_mode_config)
  {
    LOG_MSG_ERROR("Invalid arguments passed", 0, 0, 0);
    *qmi_err_num = QMI_ERR_INVALID_ARG_V01;
    return false;
  }

  /*If feature mode is ipsec*/
  if (feature_mode_config->ipsec_feature_valid)
  {
    return SetIPsecFeatureMode(feature_mode_config,qmi_err_num);
  }

  /* Check if feature mode is IP Passthrough */
  if (feature_mode_config->ip_passthrough_feature_valid)
  {
    return SetIPPTFeatureMode(feature_mode_config,qmi_err_num);
  }
  else if (feature_mode_config->eth_pdu_feature_valid)
  {
    /* Check if saved feature_mode is same as user defined feature_mode */
    memset(cmd, 0, QCMAP_MAX_COMMAND_LEN);
    memset(result, 0, QCMAP_MAX_SCAN_SIZE);
    snprintf(cmd, QCMAP_MAX_COMMAND_LEN, " %s %s",
              UCI_GET_COMMAND, QCMAP_ETH_PDU_FEATURE_MODE);

    if (ExecuteSystemCmd((const char *)cmd, result,
                    sizeof(result)))
    {
      int feature_mode = atoi(result);
      if (feature_mode_config->eth_pdu_feature_mode == (qcmap_lan_eth_pdu_feature_mode_enum)feature_mode)
      {
        LOG_MSG_INFO1("User defined eth pdu feature mode already set", 0, 0, 0);
        return true;
      }
      memset(cmd, 0, QCMAP_MAX_COMMAND_LEN);
      snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "%s %s=%d",
               UCI_SET_COMMAND, QCMAP_ETH_PDU_FEATURE_MODE, feature_mode_config->eth_pdu_feature_mode);
      ds_system_call(cmd, strlen(cmd));

      /* Commit uci */
      ExecuteUCICommit();

      /*Change network config file*/
      ConfigureNetworkOnEthPduModeChange(feature_mode_config->eth_pdu_feature_mode,
                                         feature_mode_config->eth_device_index);
      return true;
    }
    else
    {
      LOG_MSG_ERROR(" Unable to query ETH PDU feature mode", 0, 0, 0);
      return false;
    }
  }
  else
  {
    LOG_MSG_ERROR("Not an IP Passthrough feature!", 0, 0, 0);
    *qmi_err_num = QMI_ERR_INTERNAL_V01;
    //return false;
  }

  /* Check if feature mode is DHCP option */
  if (feature_mode_config->dhcp_lan_options_feature_valid)
  {
    memset(cmd, 0, QCMAP_MAX_COMMAND_LEN);
    /* Enable the feature mode */
    if (feature_mode_config->dhcp_lan_options_feature_modes == QCMAP_MSGR_MASK_DHCP_VENDOR_INFORMATION_V01)
    {
      memset(cmd, 0, QCMAP_MAX_COMMAND_LEN);
      snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "%s %s=%d",
                UCI_SET_COMMAND, "qcmap_lan.@dhcp_option[0].dhcp_vendor_info_enable", feature_mode_config->dhcp_lan_options_feature_valid);
      ds_system_call(cmd, strlen(cmd));
    }
    else if(feature_mode_config->dhcp_lan_options_feature_modes == QCMAP_MSGR_MASK_DHCP_TIMEZONE_INFORMATION_V01)
    {
      memset(cmd, 0, QCMAP_MAX_COMMAND_LEN);
      snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "%s %s=%d",
                UCI_SET_COMMAND, "qcmap_lan.@dhcp_option[0].dhcp_tz_enable", feature_mode_config->dhcp_lan_options_feature_valid);
      ds_system_call(cmd, strlen(cmd));
    }

    /* Commit uci */
    ExecuteUCICommit();
    return true;
  }
  else
  {
    LOG_MSG_ERROR("Not a DHCP options feature!", 0, 0, 0);
    return false;
  }

} /* End SetFeatureMode */

/*=====================================================================
  FUNCTION GetDHCPLANOptionsFeatureModes
======================================================================*/
/*!
@brief
  - Get the currently configured DHCP LAN Options feature modes

@return
  - None

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
bool QCMAP_LAN_Client::GetDHCPLANOptionsFeatureModes
(
  uint64_t                             *enabled_features,
  qcmap_lan_client_feature_mode_config *feature_mode_config,
  qmi_error_type_v01                   *qmi_err_num
)
{
  char cmd[QCMAP_MAX_COMMAND_LEN] = {0};
  char result[QCMAP_MAX_SCAN_SIZE] = {0};
  int tz_feature_valid, vendorinfo_feature_valid=0;

  /* For DHCP options feature mode */
  memset(cmd, 0, QCMAP_MAX_COMMAND_LEN);
  memset(result, 0, QCMAP_MAX_SCAN_SIZE);
  snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "%s %s",
            UCI_GET_COMMAND, "qcmap_lan.@dhcp_option[0].dhcp_vendor_info_enable");
  if (ExecuteSystemCmd((const char *)cmd, result, sizeof(result)))
  {
    vendorinfo_feature_valid = atoi(result);
    if (vendorinfo_feature_valid == 1)
    {

      feature_mode_config->dhcp_lan_options_feature_valid = true;
      feature_mode_config->dhcp_lan_options_feature_modes |= QCMAP_MSGR_MASK_DHCP_VENDOR_INFORMATION_V01;
      LOG_MSG_INFO1("DHCP options vendor info feature enabled.",0, 0, 0);
    }
    else
    {
      LOG_MSG_INFO1("DHCP options vendor info feature not enabled.",0, 0, 0);
    }
  }
  else
  {
    LOG_MSG_ERROR("Get DHCP options vendor info feature failed", 0, 0, 0);
    *qmi_err_num = QMI_ERR_INTERNAL_V01;
    return false;
  }

  memset(cmd, 0, QCMAP_MAX_COMMAND_LEN);
  memset(result, 0, QCMAP_MAX_SCAN_SIZE);
  snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "%s %s",
            UCI_GET_COMMAND, "qcmap_lan.@dhcp_option[0].dhcp_tz_enable");
  if (ExecuteSystemCmd((const char *)cmd, result, sizeof(result)))
  {
    tz_feature_valid = atoi(result);
    if (tz_feature_valid == 1)
    {
      feature_mode_config->dhcp_lan_options_feature_valid = true;
      feature_mode_config->dhcp_lan_options_feature_modes |= QCMAP_MSGR_MASK_DHCP_TIMEZONE_INFORMATION_V01;
      LOG_MSG_INFO1("DHCP options tz feature enabled.",0, 0, 0);
    }
    else
    {
      LOG_MSG_INFO1("DHCP options tz feature not enabled.",0, 0, 0);
    }
  }
  else
  {
    LOG_MSG_ERROR("Get DHCP options tz feature failed", 0, 0, 0);
    *qmi_err_num = QMI_ERR_INTERNAL_V01;
    return false;
  }

  if (tz_feature_valid || vendorinfo_feature_valid)
    *enabled_features |= QCMAP_MSGR_MASK_ENABLE_FEATURE_DHCP_LAN_OPTIONS_V01;

  *qmi_err_num = QMI_ERR_NONE_V01;
  return true;
}

/*=====================================================================
  FUNCTION GetNetworkConfig
======================================================================*/
/*!
@brief
  - Fetching the config details from the config File

@return
  true - Success
  false - Failure

@param
   uint32_t profile_handle

@note
  - Dependencies
  - None

  - Side Effects
  - None
*/
/*=========================================================================*/
boolean QCMAP_LAN_Client::GetNetworkConfig(qcmap_nw_params_t *qcmap_nw_params,
                                        uint32_t profile_handle,
                                        qcmap_ip_family_enum ip_type,
                                        qmi_error_type_v01 *qmi_err_num)
{
  char file_path[MAX_COMMAND_STR_LEN] = {0};
  char result[QCMAP_MAX_COMMAND_LEN] = {0};
  char command[QCMAP_MAX_COMMAND_LEN] = {0};
  char bh_present_v4[QCMAP_MAX_IFACE_NAME_SIZE] = {0};
  char bh_present_v6[QCMAP_MAX_IFACE_NAME_SIZE] = {0};
  int profile_idx = QCMAP_LAN_INVALID;
  char *p = NULL, *pri_dns = NULL, *sec_dns = NULL;
  char split[]=" ";
  in_addr addr;
  struct sockaddr_in6 addr6;
  int default_profile = QCMAP_LAN_INVALID;

  if ( qmi_err_num == NULL || qcmap_nw_params == NULL )
  {
    LOG_MSG_ERROR(" Null argument passed ",0,0,0);
    return false;
  }

  /* Get default profile ID */
  default_profile = GetDefaultProfilefromUCI();
  if (default_profile == QCMAP_LAN_INVALID)
  {
    LOG_MSG_ERROR("Invalid default profile received", 0, 0, 0);
    *qmi_err_num = QMI_ERR_INTERNAL_V01;
    return false;
  }
  /* Check if current profile handle is same as default handle */
  if ((uint32_t)default_profile == profile_handle)
  {
    /* Indicates default profile. we need to get bh_present and bh_present_v6
       parameters from uci */
    /* Get profile Index from uci */
    profile_idx = GetProfileIndex(profile_handle);
    if (profile_idx == QCMAP_LAN_INVALID)
    {
      LOG_MSG_ERROR("Invalid profile index received", 0, 0, 0);
      *qmi_err_num = QMI_ERR_INTERNAL_V01;
      return false;
    }

    /* Get bh_present and bh_present_v6 parameters */
    if (!GetCurrentBackhaul(bh_present_v4, bh_present_v6, profile_idx))
    {
      LOG_MSG_ERROR(" Failed to retrieve bh_present values", 0, 0, 0);
      *qmi_err_num = QMI_ERR_INTERNAL_V01;
      return false;
    }
  }

  if (ip_type == QCMAP_IP_FAMILY_V4)
  {
    if (!GetV4NetworkConfig(qcmap_nw_params, profile_handle, bh_present_v4, qmi_err_num))
    {
      LOG_MSG_ERROR("Failed to get v4 Network configurations", 0, 0, 0);
      return false;
    }
  } /* End: ip_type V4 */
  else if (ip_type == QCMAP_IP_FAMILY_V6)
  {
    if (!GetV6NetworkConfig(qcmap_nw_params, profile_handle, bh_present_v6, qmi_err_num))
    {
      LOG_MSG_ERROR("Failed to get v6 Network configurations", 0, 0, 0);
      return false;
    }
  } /* End: ip type v6 */
#ifdef FEATURE_DATA_ETH_PDU
  else if (ip_type == QCMAP_IP_FAMILY_ETH)
  {
    if (!GetETHPDUNetworkConfig(qcmap_nw_params, profile_handle, qmi_err_num))
    {
      LOG_MSG_ERROR("Failed to get ETH PDU Network configurations", 0, 0, 0);
      return false;
    }
  }
#endif
  else
  {
    LOG_MSG_ERROR("Unrecognized IP Family passed", 0, 0, 0);
    *qmi_err_num = QMI_ERR_INVALID_ARG_V01;
    return false;
  }
  return true;
} /* End GetNetworkConfig() */

/*=====================================================================
  FUNCTION GetV4NetworkConfig
======================================================================*/
/*!
@brief
  - Fetching the config details of IPV4 from the config File

@return
  true - Success
  false - Failure

@param
   uint32_t profile_handle

@note
  - Dependencies
  - None

  - Side Effects
  - None
*/
/*=========================================================================*/
boolean QCMAP_LAN_Client::GetV4NetworkConfig(qcmap_nw_params_t *qcmap_nw_params,
                                             uint32_t profile_handle,
                                             char *bh_present_v4,
                                             qmi_error_type_v01 *qmi_err_num)
{
  char file_path[MAX_COMMAND_STR_LEN] = {0};
  char result[QCMAP_MAX_COMMAND_LEN] = {0};
  char command[MAX_COMMAND_STR_LEN] = {0};
  int default_profile = QCMAP_LAN_INVALID;
  char *p = NULL, *pri_dns = NULL, *sec_dns = NULL;
  char split[]=" ";
  in_addr addr;

  if ( qmi_err_num == NULL || qcmap_nw_params == NULL ||
        profile_handle == QCMAP_LAN_INVALID)
  {
    LOG_MSG_ERROR(" Null argument passed ",0,0,0);
    return false;
  }

  /* Get default profile ID */
  default_profile = GetDefaultProfilefromUCI();
  if (default_profile == QCMAP_LAN_INVALID)
  {
    LOG_MSG_ERROR("Invalid default profile received", 0, 0, 0);
    *qmi_err_num = QMI_ERR_INTERNAL_V01;
    return false;
  }

  /* Get the correct backhaul config file */
  /* NOTE: condition to check if profile handle is default profile is required
     as secondary profiles can be active only on default profile */
  if ((uint32_t)default_profile != profile_handle)
  {
    /* On-demand PDN */
    snprintf(file_path, MAX_COMMAND_STR_LEN, "/tmp/ipv4config%d", profile_handle);
  }
  else
  {
    /* Check for bh_present_v4 parameter only for default pdn */
    if (bh_present_v4 == NULL || (bh_present_v4 && !bh_present_v4[0]))
    {
      LOG_MSG_ERROR("bh_present_v4 is NULL or invalid", 0, 0, 0);
      return false;
    }
    snprintf(command, MAX_COMMAND_STR_LEN, "%s %s %s %s",
             BACKHAUL_COMMMON_CONFIG_FILE,
             GET_BACKHAUL_FILE,
             IP_V4_STRING,
             bh_present_v4);
    memset(result, 0, QCMAP_MAX_COMMAND_LEN);
    if (!ExecuteSystemCmd((const char*)command, result, sizeof(result)))
    {
     LOG_MSG_ERROR("Failed to execute command : %s",command,0,0);
     *qmi_err_num = QMI_ERR_INTERNAL_V01;
     return false;
    }
    if (strlen(result) > 0)
    {
      result[strlen(result)-1] = '\0';
    }
    strlcpy(file_path, result, QCMAP_MAX_FILE_PATH_LEN);
  }

  if (CheckIfFileExists(file_path))
  {
    /* Fetching the public ip info from the ipv4config file */
    memset(result, 0, QCMAP_MAX_COMMAND_LEN);
    snprintf(command, MAX_COMMAND_STR_LEN,
             "cat %s | grep PUBLIC_IP | awk -F '\"' '{print $2}'",
             file_path);
    if (!ExecuteSystemCmd((const char*)command, result, sizeof(result)))
    {
     LOG_MSG_ERROR("Failed to execute command : %s",command,0,0);
     *qmi_err_num = QMI_ERR_INTERNAL_V01;
     return false;
    }

    memset(&addr, 0, sizeof(in_addr));
    if (strlen(result) > 0)
    {
      result[strlen(result)-1] = '\0';
    }
    if ( !(inet_aton(result, &addr) <= 0) )
    {
      qcmap_nw_params->v4_conf.public_ip.s_addr = ntohl(addr.s_addr);
    }
    else
    {
      LOG_MSG_ERROR("Invalid public_ip address obtained",0,0,0);
      *qmi_err_num = QMI_ERR_INTERNAL_V01;
      return false;
    }

    /* Fetching the dnsserver info from the ipv4config file */
    memset(result, 0, QCMAP_MAX_COMMAND_LEN);
    snprintf(command, MAX_COMMAND_STR_LEN,
             "cat %s | grep DNSSERVERS | awk -F '\"' '{print $2}'",
             file_path);
    if (!ExecuteSystemCmd((const char*)command, result, sizeof(result)))
    {
     LOG_MSG_ERROR("Failed to execute command : %s",command,0,0);
     *qmi_err_num = QMI_ERR_INTERNAL_V01;
     return false;
    }

    if (strlen(result) > 0)
    {
      result[strlen(result)-1] = '\0';
    }
    /* NOTE: since there might be multiple dnsservers in a single
     character array, we tokenize the string */
    pri_dns = strtok_r(result, split, &p);
    if (pri_dns)
    {
      memset(&addr, 0, sizeof(in_addr));
      /* convert the string token into network order */
      if (!(inet_aton(pri_dns, &addr) <= 0))
        qcmap_nw_params->v4_conf.primary_dns.s_addr = ntohl(addr.s_addr);
    }

    sec_dns = strtok_r(NULL, split, &p);
    if (sec_dns)
    {
      memset(&addr, 0, sizeof(in_addr));
      /* convert the string token into network order */
      if (!(inet_aton(sec_dns, &addr) <= 0))
        qcmap_nw_params->v4_conf.secondary_dns.s_addr = ntohl(addr.s_addr);
    }
  }
  else
  {
    LOG_MSG_ERROR("Failed to find file:%s",file_path,0,0);
     *qmi_err_num = QMI_ERR_INTERNAL_V01;
     return false;
  }
  return true;
} /* End GetV4NetworkConfig() */

/*=====================================================================
  FUNCTION GetV6NetworkConfig
======================================================================*/
/*!
@brief
  - Fetching the config details of IPV6 from the config File

@return
  true - Success
  false - Failure

@param
   uint32_t profile_handle

@note
  - Dependencies
  - None

  - Side Effects
  - None
*/
/*=========================================================================*/
boolean QCMAP_LAN_Client::GetV6NetworkConfig(qcmap_nw_params_t *qcmap_nw_params,
                                             uint32_t profile_handle,
                                             char *bh_present_v6,
                                             qmi_error_type_v01 *qmi_err_num)
{
  char file_path[MAX_COMMAND_STR_LEN] = {0};
  char result[QCMAP_MAX_COMMAND_LEN] = {0};
  char command[MAX_COMMAND_STR_LEN] = {0};
  int default_profile = QCMAP_LAN_INVALID;
  char *p = NULL, *pri_dns = NULL, *sec_dns = NULL;
  char split[]=" ";
  struct sockaddr_in6 addr6;

  if ( qmi_err_num == NULL || qcmap_nw_params == NULL ||
        profile_handle == QCMAP_LAN_INVALID)
  {
    LOG_MSG_ERROR(" Null argument passed ",0,0,0);
    return false;
  }

  /* Get default profile ID */
  default_profile = GetDefaultProfilefromUCI();
  if (default_profile == QCMAP_LAN_INVALID)
  {
    LOG_MSG_ERROR("Invalid default profile received", 0, 0, 0);
    *qmi_err_num = QMI_ERR_INTERNAL_V01;
    return false;
  }

  /* Get the correct backhaul config file */
  /* NOTE: condition to check if profile handle is default profile is required
     as secondary profiles can be active only on default profile */
  if ((uint32_t)default_profile != profile_handle)
  {
    /* On-demand PDN */
    snprintf(file_path, MAX_COMMAND_STR_LEN, "/tmp/ipv6config%d", profile_handle);
  }
  else
  {
    /* Check for bh_present_v6 parameter only for default pdn */
    if (bh_present_v6 == NULL || (bh_present_v6 && !bh_present_v6[0]))
    {
      LOG_MSG_ERROR("bh_present_v6 is NULL or invalid", 0, 0, 0);
      return false;
    }
    snprintf(command, MAX_COMMAND_STR_LEN, "%s %s %s %s",
             BACKHAUL_COMMMON_CONFIG_FILE,
             GET_BACKHAUL_FILE,
             IP_V6_STRING,
             bh_present_v6);
    memset(result, 0, QCMAP_MAX_COMMAND_LEN);
    if (!ExecuteSystemCmd((const char*)command, result, sizeof(result)))
    {
     LOG_MSG_ERROR("Failed to execute command : %s",command,0,0);
     *qmi_err_num = QMI_ERR_INTERNAL_V01;
     return false;
    }
    if (strlen(result) > 0)
    {
      result[strlen(result)-1] = '\0';
    }
    strlcpy(file_path, result, QCMAP_MAX_FILE_PATH_LEN);
  }

  if (CheckIfFileExists(file_path))
  {
    /* Fetching the public ip6 info from the ipv4config file */
    memset(result, 0, QCMAP_MAX_COMMAND_LEN);
    snprintf(command, MAX_COMMAND_STR_LEN,
             "cat %s | grep PUBLIC_IP6 | awk -F '\"' '{print $2}'",
             file_path);
    if (!ExecuteSystemCmd((const char*)command, result, sizeof(result)))
    {
     LOG_MSG_ERROR("Failed to execute command : %s",command,0,0);
     *qmi_err_num = QMI_ERR_INTERNAL_V01;
     return false;
    }

    memset(&addr6, 0, sizeof(sockaddr_in6));
    if (strlen(result) > 0)
    {
      result[strlen(result)-1] = '\0';
    }
    if ( !(inet_pton(AF_INET6, result, &(addr6.sin6_addr)) <= 0) )
    {
      qcmap_nw_params->v6_conf.public_ip_v6 = addr6.sin6_addr;
    }
    else
    {
      LOG_MSG_ERROR("Invalid public_ip6 address obtained",0,0,0);
      *qmi_err_num = QMI_ERR_INTERNAL_V01;
      return false;
    }

    /* Fetching the dnsserver info from the ipv4config file */
    memset(result, 0, QCMAP_MAX_COMMAND_LEN);
    snprintf(command, MAX_COMMAND_STR_LEN,
             "cat %s | grep DNSSERVERS6 | awk -F '\"' '{print $2}'",
             file_path);
    if (!ExecuteSystemCmd((const char*)command, result, sizeof(result)))
    {
     LOG_MSG_ERROR("Failed to execute command : %s",command,0,0);
     *qmi_err_num = QMI_ERR_INTERNAL_V01;
     return false;
    }

    if (strlen(result) > 0)
    {
      result[strlen(result)-1] = '\0';
    }
    /* NOTE: since there might be multiple dnsservers in a single
     character array, we tokenize the string */
    pri_dns = strtok_r(result, split, &p);
    if (pri_dns)
    {
      memset(&addr6, 0, sizeof(sockaddr_in6));
      /* convert the string token into network order */
      if (!(inet_pton(AF_INET6, pri_dns, &(addr6.sin6_addr)) <= 0))
        qcmap_nw_params->v6_conf.primary_dns_v6 = addr6.sin6_addr;
    }

    sec_dns = strtok_r(NULL, split, &p);
    if (sec_dns)
    {
      memset(&addr6, 0, sizeof(sockaddr_in6));
      /* convert the string token into network order */
      if (!(inet_pton(AF_INET6, sec_dns, &(addr6.sin6_addr)) <= 0))
        qcmap_nw_params->v6_conf.secondary_dns_v6 = addr6.sin6_addr;
    }
  }
  else
  {
    LOG_MSG_ERROR("Failed to find file:%s",file_path,0,0);
     *qmi_err_num = QMI_ERR_INTERNAL_V01;
     return false;
  }
  return true;
} /* End: GetV6NetworkConfig() */

/*===========================================================================
  FUNCTION SetIPv6PDManager
  ===========================================================================*/
/*!
  @brief
  Set Ipv6 pd_manager to qcmap_lan.

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
boolean QCMAP_LAN_Client::SetIPv6PDManager
(
  bool enable
)
{
  QCMAP_LAN_CLIENT_RUN_COMMANDS("%s %s %d", DHCP_RELOAD_PD_OPTION_FILE, QCMAP_LAN_ENABLE_PD_MANAGER, enable);

  if (QCMAP_LAN_Client::IsLegacyModeEnabled())
  {
    LOG_MSG_INFO1("not need restart clients when ipv6 pd legacy mode enabled", 0, 0, 0);
    return true;
  }

  /* After update external router mode config complete, need restart tethered clients to update clients's prefix */
  QCMAP_LAN_Client::RestartTetheredClient(QCMAP_LAN_DEVICE_TYPE_ETHERNET);
  QCMAP_LAN_Client::RestartTetheredClient(QCMAP_LAN_DEVICE_TYPE_ETHERNET_NIC2);
  QCMAP_LAN_Client::RestartTetheredClient(QCMAP_LAN_DEVICE_TYPE_ALL_AP);

  return true;
} /* End: SetIPv6PDManager() */

/*===========================================================================
  FUNCTION SetIPv6PDActivatedConfig
  ===========================================================================*/
/*!
  @brief
  Set/delete Ipv6 Prefix delegation activated config.
  IDU mode, Disable the PD first, will set this config to 0

  @return
  true	- on Success
  false - on Failure

  @note

  - Dependencies
  - None

  - Side Effects
  - None
 */
/*=========================================================================*/
boolean QCMAP_LAN_Client::SetIPv6PDActivatedConfig
(
  int value
)
{
  QCMAP_LAN_CLIENT_RUN_COMMANDS("%s %s %d", DHCP_RELOAD_PD_OPTION_FILE, QCMAP_LAN_ENABLE_PD_ACTIVATED, value);

  return true;
} /* End: SetIPv6PDActivatedConfig() */

/*===========================================================================
  FUNCTION RecyclePrefixForModeChange
  ===========================================================================*/
/*!
  @brief
  When change mode, Legacy->IDU or IDU->Legacy, need restart tethered clients to recycle prefix first

  @return
  true	- on Success
  false - on Failure

  @note

  - Dependencies
  - None

  - Side Effects
  - None
 */
/*=========================================================================*/
boolean QCMAP_LAN_Client::RecyclePrefixForModeChange
(
  bool enable
)
{
  char command[MAX_COMMAND_STR_LEN] = {0};
  char result[MAX_COMMAND_STR_LEN] = {0};
  int pd_activated = 0;
  int ext_router_mode_enabled = 0;
  int pd_prefix_available = 0;

  snprintf(command, MAX_COMMAND_STR_LEN, QCMAP_GET_PD_ACTIVATED_CONFIG);
  if (!ExecuteSystemCmd((const char *)command, result, sizeof(result)))
  {
    LOG_MSG_ERROR("Failed to get pd_activated config", 0, 0, 0);
  }
  else
  {
    pd_activated = atoi(result);
  }

  memset(command, 0, MAX_COMMAND_STR_LEN);
  memset(result, 0, MAX_COMMAND_STR_LEN);
  snprintf(command, MAX_COMMAND_STR_LEN, QCMAP_GET_PD_EXT_ROUTER_MODE_ENABLED_CONFIG);
  if (!ExecuteSystemCmd((const char *)command, result, sizeof(result)))
  {
    LOG_MSG_ERROR("Failed to get ext_router_mode_enabled config", 0, 0, 0);
  }
  else
  {
    ext_router_mode_enabled = atoi(result);
  }

  memset(command, 0, MAX_COMMAND_STR_LEN);
  memset(result, 0, MAX_COMMAND_STR_LEN);
  snprintf(command, MAX_COMMAND_STR_LEN, QCMAP_GET_PD_AVAILABLE_CONFIG);
  if (!ExecuteSystemCmd((const char *)command, result, sizeof(result)))
  {
    LOG_MSG_ERROR("Failed to get pd_prefix_available config", 0, 0, 0);
  }
  else
  {
    pd_prefix_available = atoi(result);
  }

  if ((enable && pd_activated && !ext_router_mode_enabled && pd_prefix_available) ||
      (!enable && pd_activated && ext_router_mode_enabled && pd_prefix_available))
  {
    QCMAP_LAN_Client::RestartTetheredClient(QCMAP_LAN_DEVICE_TYPE_ETHERNET);
    QCMAP_LAN_Client::RestartTetheredClient(QCMAP_LAN_DEVICE_TYPE_ETHERNET_NIC2);
    QCMAP_LAN_Client::RestartTetheredClient(QCMAP_LAN_DEVICE_TYPE_ALL_AP);
  }

  return true;
} /* End: RecyclePrefixForModeChange() */

/*===========================================================================
  FUNCTION SetExtRouterModeEnabled
  ===========================================================================*/
/*!
  @brief
  set/delete external router mode enabled config to qcmap_lan

  @return
  true	- on Success
  false - on Failure

  @note

  - Dependencies
  - None

  - Side Effects
  - None
 */
/*=========================================================================*/
boolean QCMAP_LAN_Client::SetExtRouterModeEnabled
(
  int enable
)
{
  QCMAP_LAN_CLIENT_RUN_COMMANDS("%s %s %d", DHCP_RELOAD_PD_OPTION_FILE, QCMAP_LAN_ENABLE_EXT_ROUTER_MODE, enable);

  return true;
} /* End: SetExtRouterModeEnabled() */

/*=====================================================================
  FUNCTION GetETHPDUNetworkConfig
======================================================================*/
/*!
@brief
  - Fetching the config details of ETH PDU from the config File

@return
  true - Success
  false - Failure

@param
   uint32_t profile_handle

@note
  - Dependencies
  - None

  - Side Effects
  - None
*/
/*=========================================================================*/
boolean QCMAP_LAN_Client::GetETHPDUNetworkConfig(qcmap_nw_params_t *qcmap_nw_params,
                                                 uint32_t profile_handle,
                                                 qmi_error_type_v01 *qmi_err_num)
{
  char file_path[MAX_COMMAND_STR_LEN] = {0};
  char result[QCMAP_MAX_COMMAND_LEN] = {0};
  char command[MAX_COMMAND_STR_LEN] = {0};

  if ( qmi_err_num == NULL || qcmap_nw_params == NULL ||
        profile_handle == QCMAP_LAN_INVALID)
  {
    LOG_MSG_ERROR(" Null argument passed ",0,0,0);
    return false;
  }

  /* construct file path string */
  snprintf(file_path, MAX_COMMAND_STR_LEN, "/tmp/ethpduconfig%d", profile_handle);

  if (CheckIfFileExists(file_path))
  {
    /* Fetching the vlan_start info from the ethpduconfig file */
    memset(result, 0, QCMAP_MAX_COMMAND_LEN);
    snprintf(command, MAX_COMMAND_STR_LEN,
             "cat %s | grep VLAN_START | awk -F '\"' '{print $2}'",
             file_path);
    if (!ExecuteSystemCmd((const char*)command, result, sizeof(result)))
    {
     LOG_MSG_ERROR("Failed to execute command : %s",command,0,0);
     *qmi_err_num = QMI_ERR_INTERNAL_V01;
     return false;
    }

    if (strlen(result) > 0){
      result[strlen(result)-1] = '\0';
      qcmap_nw_params->eth_conf.vlan_start = (uint16_t)atoi(result);
    }

    /* Fetching the vlan_end info from the ethpduconfig file */
    memset(result, 0, QCMAP_MAX_COMMAND_LEN);
    snprintf(command, MAX_COMMAND_STR_LEN,
             "cat %s | grep VLAN_END | awk -F '\"' '{print $2}'",
             file_path);
    if (!ExecuteSystemCmd((const char*)command, result, sizeof(result)))
    {
     LOG_MSG_ERROR("Failed to execute command : %s",command,0,0);
     *qmi_err_num = QMI_ERR_INTERNAL_V01;
     return false;
    }

    if (strlen(result) > 0){
      result[strlen(result)-1] = '\0';
      qcmap_nw_params->eth_conf.vlan_end = (uint16_t)atoi(result);
    }
  }
  else
  {
    LOG_MSG_ERROR("Failed to find file:%s",file_path,0,0);
     *qmi_err_num = QMI_ERR_INTERNAL_V01;
     return false;
  }
  return true;
} /* End: GetETHPDUNetworkConfig() */


boolean QCMAP_LAN_Client::DisableIPV4
(
   qmi_error_type_v01 *qmi_err_num
)
{
  QCMAP_LAN_CLIENT_RUN_COMMANDS("%s %s", BACKHAUL_COMMMON_CONFIG_FILE, "disable_ipv4");
  LOG_MSG_INFO1("Disable IPV4 success", 0, 0, 0);
  return true;
}

boolean QCMAP_LAN_Client::DisableIPV6
(
   qmi_error_type_v01 *qmi_err_num
)
{
  QCMAP_LAN_CLIENT_RUN_COMMANDS("%s %s", BACKHAUL_COMMMON_CONFIG_FILE, "disable_ipv6");
  LOG_MSG_INFO1("Disable IPV6 success", 0, 0, 0);
  return true;
}

boolean QCMAP_LAN_Client::EnableIPV4
(
   qmi_error_type_v01 *qmi_err_num
)
{
  QCMAP_LAN_CLIENT_RUN_COMMANDS("%s %s", BACKHAUL_COMMMON_CONFIG_FILE, "enable_ipv4");
  LOG_MSG_INFO1("Enable IPV4 success", 0, 0, 0);
  return true;
}

boolean QCMAP_LAN_Client::EnableIPV6
(
   qmi_error_type_v01 *qmi_err_num
)
{
  QCMAP_LAN_CLIENT_RUN_COMMANDS("%s %s", BACKHAUL_COMMMON_CONFIG_FILE, "enable_ipv6");
  LOG_MSG_INFO1("Enable IPV6 success", 0, 0, 0);
  return true;
}

/*===========================================================================
FUNCTION IsLinkDetected()
===========================================================================*/
/** @ingroup section_IsTetheredLinkUp

  Checks if link is up

  @param[in] char *ifname

  @return
  TRUE -- Success \n
  FALSE -- Failure \n

  @dependencies
  None
*/
/*=========================================================================*/
boolean QCMAP_LAN_Client::IsLinkDetected
(
   char *ifname
)
{
  char command[QCMAP_MAX_COMMAND_LEN] = {0};
  char result[QCMAP_MAX_SCAN_SIZE] = {0};
  int itr = 1;

  if (ifname == NULL)
  {
    LOG_MSG_ERROR("Invalid ifname passed", 0, 0, 0);
    return false;
  }
  snprintf(command, QCMAP_MAX_COMMAND_LEN, ETH_LINK_DETECTION, ifname);
  while (itr<=5)
  {
    if (!ExecuteSystemCmd((const char *)command, result, sizeof(result)))
    {
      LOG_MSG_ERROR("Failed to execute command : %s",command,0,0);
      return false;
    }
    if ((strncmp(result, LINK_DETECTED, strlen(LINK_DETECTED)) == 0))
    {
      return true;
    }
    itr++;
    sleep(1);
  }
  return false;
}

