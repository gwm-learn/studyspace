/*====================================================

FILE:  QCMAP_WLAN_WKK.cpp

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

  ===========================================================================*/
#include "QCMAP_WLAN_WKK.h"



/*===================================================================
  Class Definitions
  ===================================================================*/

/*===========================================================================
  FUNCTION QCMAP_WLAN_WKK
  ===========================================================================
  @brief
  Initializes the WLAN Client.
  @input
  void
  @return
  void
  @dependencies
  @sideefects
  None
  =========================================================================*/
QCMAP_WLAN_WKK::QCMAP_WLAN_WKK()
{
  strlcpy(mWLANConfigFile, WLAN_CONFIG_FILE, MAX_FILE_PATH_LEN);
  return;
}

QCMAP_WLAN_WKK::~QCMAP_WLAN_WKK()
{
  return;
}

/*===========================================================================
  FUNCTION GetWlanIfaceName
  ===========================================================================*/
/*!
  @brief
  To Get the particular ath interface to toggle the wifi link

  @return
  we have interface name as athX
  returns X as integer - Success
  return -1 - failure

  @parameters
  cahr *interface

  @note

  - Dependencies
  - None

  - Side Effects
  - None
 */
/*=========================================================================*/
int QCMAP_WLAN_WKK::GetWlanIfaceName
(
  char *interface
)
{
  if( interface == NULL)
  {
      LOG_MSG_ERROR("Failed to get the WLAN interface", 0, 0, 0);
      return -1;
  }
  return (interface[3]-'0');
}

boolean QCMAP_WLAN_WKK::IsWifiUp(qmi_error_type_v01 *qmi_err_num)
{
  char cmd[QCMAP_MAX_COMMAND_LEN] = {0};
  char result[QCMAP_MAX_SCAN_SIZE] = {0};
  memset(cmd, 0, QCMAP_MAX_COMMAND_LEN);
  memset(result, 0, QCMAP_MAX_SCAN_SIZE);
  snprintf(cmd, MAX_COMMAND_STR_LEN,"%s %s", mWLANConfigFile, IS_WIFI_UP);
  if (!ExecuteSystemCmd((const char *)cmd, result, sizeof(result)))
  {
    LOG_MSG_ERROR("Failed to execute command : %s", cmd, 0, 0);
    *qmi_err_num = QMI_ERR_INTERNAL_V01;
    return false;
  }
  return atoi(result);
}
/*===========================================================================
  FUNCTION GetActiveWlanIfInfo
  ===========================================================================*/
/*!
  @brief
  Obtains information from active WLAN interfaces.

  @return
  true  - on Success
  false - on Failure

  @note

  - Dependencies
  - WLAN should be enabled

  - Side Effects
  - None

  - Sample output:
 */
/*=========================================================================*/
boolean QCMAP_WLAN_WKK::FillSpecificWlanIfInfo(qcmap_msgr_wlan_if_info_t *wlan_info_cfg, int wlan_mode)
{
  char result[QCMAP_MAX_SCAN_SIZE] = {0};
  char cmd[QCMAP_MAX_COMMAND_LEN] = {0};
  memset(cmd, 0, QCMAP_MAX_COMMAND_LEN);
  memset(result, 0, QCMAP_MAX_SCAN_SIZE);

  if (wlan_mode == QCMAP_MSGR_WLAN_MODE_7AP_V01)
  {
    char *ptr = NULL;
    char *p = NULL;
    char split[]=" ";
    char wlan_device_info[QCMAP_MAX_SCAN_SIZE] = {0};
    int ap_count=0;
    UCI_WLAN_GET_INT_OPTION(ap_count, "wlanconfig",  "totalapcount");
    wlan_info_cfg->wlan_if_info_len = ap_count; // Primary AP + total Geust AP.
    p=NULL;
    int idx=0;

    snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "%s %s",
                              mWLANConfigFile, CMD_GET_ACTIVE_WLAN_INFO_7AP);
    if (!ExecuteSystemCmd((const char *)cmd, result, sizeof(result)))
    {
      LOG_MSG_ERROR("Failed to get active wlan device info", 0, 0, 0);
      return false;
    }
    ptr = strtok_r(result, split, &p);
    while (ptr != NULL)
    {
      wlan_device_info[idx++]=*ptr;
      ptr = strtok_r(NULL, split, &p);
    }
    UCI_WLAN_GET_STR_OPTION(wlan_info_cfg->wlan_if_info[0].if_name, "primaryap", "ifname")
    wlan_info_cfg->wlan_if_info[0].state=wlan_device_info[0]-'0';
    wlan_info_cfg->wlan_if_info[0].wlan_ap_type=QCMAP_MSGR_WLAN_IFACE_PRIMARY_AP_V01;
    for(int i=0; i < (ap_count -1); i++)
    {
      UCI_WLAN_GET_STR_OPTION_WITH_IDX(wlan_info_cfg->wlan_if_info[i+1].if_name, "guestapconfig", "ifname", i)
      wlan_info_cfg->wlan_if_info[i+1].state=wlan_device_info[i+1]-'0';
      wlan_info_cfg->wlan_if_info[i+1].wlan_ap_type=QCMAP_MSGR_WLAN_IFACE_GUEST_AP_ONE_V01;
    }

  }
  else
  {
    if (wlan_mode == QCMAP_MSGR_WLAN_MODE_AP_V01)
    {
        //Get primary ifname
        UCI_WLAN_GET_STR_OPTION(wlan_info_cfg->wlan_if_info[0].if_name, "primaryap", "ifname")
    }
    else if (wlan_mode == QCMAP_MSGR_WLAN_MODE_AP_AP_V01)
    {
        // Get Primary and Guest AP ifname
        UCI_WLAN_GET_STR_OPTION(wlan_info_cfg->wlan_if_info[0].if_name, "primaryap", "ifname")
        UCI_WLAN_GET_STR_OPTION(wlan_info_cfg->wlan_if_info[1].if_name, "guestap",   "ifname")
    }
    else if(wlan_mode == QCMAP_MSGR_WLAN_MODE_AP_STA_V01)
    {
        // Get Primary and STA ifname
        UCI_WLAN_GET_STR_OPTION(wlan_info_cfg->wlan_if_info[0].if_name, "primaryap",     "ifname")
        UCI_WLAN_GET_STR_OPTION(wlan_info_cfg->wlan_if_info[1].if_name, "stamodeconfig", "ifname")
    }
    else if(wlan_mode == QCMAP_MSGR_WLAN_MODE_AP_AP_AP_V01)
    {
        // Get Primary/Guest AP/Guest AP2  ifname
        UCI_WLAN_GET_STR_OPTION(wlan_info_cfg->wlan_if_info[0].if_name, "primaryap", "ifname")
        UCI_WLAN_GET_STR_OPTION(wlan_info_cfg->wlan_if_info[1].if_name, "guestap",    "ifname")
        UCI_WLAN_GET_STR_OPTION(wlan_info_cfg->wlan_if_info[2].if_name, "guestaptwo", "ifname")
    }
    else if(wlan_mode == QCMAP_MSGR_WLAN_MODE_AP_AP_STA_V01)
    {
        // Get Primary/Guest AP/STA ifname
        UCI_WLAN_GET_STR_OPTION(wlan_info_cfg->wlan_if_info[0].if_name, "primaryap", "ifname")
        UCI_WLAN_GET_STR_OPTION(wlan_info_cfg->wlan_if_info[1].if_name, "guestap",   "ifname")
        UCI_WLAN_GET_STR_OPTION(wlan_info_cfg->wlan_if_info[2].if_name, "stamodeconfig", "ifname")
    }
    else if (wlan_mode == QCMAP_MSGR_WLAN_MODE_STA_ONLY_V01)
    {
        // Get STA ifname
        UCI_WLAN_GET_STR_OPTION(wlan_info_cfg->wlan_if_info[0].if_name, "stamodeconfig", "ifname")
    }
    else if (wlan_mode == QCMAP_MSGR_WLAN_MODE_AP_AP_AP_AP_V01)
    {
        // Get Primary/Guest AP/Guest AP2/ Guest AP3 ifname
        UCI_WLAN_GET_STR_OPTION(wlan_info_cfg->wlan_if_info[0].if_name, "primaryap",  "ifname")
        UCI_WLAN_GET_STR_OPTION(wlan_info_cfg->wlan_if_info[1].if_name, "guestap",    "ifname")
        UCI_WLAN_GET_STR_OPTION(wlan_info_cfg->wlan_if_info[2].if_name, "guestaptwo", "ifname")
        UCI_WLAN_GET_STR_OPTION(wlan_info_cfg->wlan_if_info[3].if_name, "guestapthree", "ifname")
    }
  }

    //to add: QCMAP_MSGR_WLAN_DEV_WKK_V01 is not included in qcmap_msgr_wlan_device_type_v01
  for (int j = 0; j < wlan_info_cfg->wlan_if_info_len; j++)
  {
      wlan_info_cfg->wlan_if_info[j].wlan_dev_type = QCMAP_MSGR_WLAN_DEV_WKK_V01;
  }
  return true;
}

boolean QCMAP_WLAN_WKK::GetWLANConfigEx
(
  qcmap_wlan_ex_config *wlan_config,
  qmi_error_type_v01 *qmi_err_num
)
{

  int ap_count2=0, ap_count5=0, ap_count6=0;
  int band=0, accessprofile=0;
  char result[QCMAP_MAX_SCAN_SIZE] = {0};
  memset(result, 0, QCMAP_MAX_SCAN_SIZE);

  if(wlan_config == NULL)
  {
    LOG_MSG_ERROR("NULL pointer passed ",0,0,0);
    *qmi_err_num = QMI_ERR_INVALID_ARG_V01;
    return false;
  }

  UCI_WLAN_GET_INT_OPTION(wlan_config->wlan_mode,       "wlanconfig", "mode")
  UCI_WLAN_GET_INT_OPTION(wlan_config->primary_ap_band, "primaryap",  "band")

  if (wlan_config->wlan_mode != QCMAP_MSGR_WLAN_MODE_7AP_V01)
  {
   QCMAP_WLAN_Common::GetWLANConfigEx(wlan_config, qmi_err_num);
  }
  else
  {
    UCI_WLAN_GET_INT_OPTION(ap_count2, "wlanconfig", "guestapcount2g");
    UCI_WLAN_GET_INT_OPTION(ap_count5, "wlanconfig", "guestapcount5g");
    UCI_WLAN_GET_INT_OPTION(ap_count6, "wlanconfig", "guestapcount6g");
    wlan_config->ap_config_len += ap_count2;
    wlan_config->ap_config_len += ap_count5;
    wlan_config->ap_config_len += ap_count6;

    for (int i = 0; i < wlan_config->ap_config_len; i++)
    {
      UCI_WLAN_GET_INT_OPTION_WITH_IDX(band,          "guestapconfig", "band",          i)
      UCI_WLAN_GET_INT_OPTION_WITH_IDX(accessprofile, "guestapconfig", "accessprofile", i)
      wlan_config->ap_config[i].guest_ap_profile=accessprofile;
      wlan_config->ap_config[i].band=band;
    }
  }
  *qmi_err_num = QMI_ERR_NONE_V01;
  return true;
}

/*=====================================================================
  FUNCTION DisAssociateClient
======================================================================*/
/*!
@brief
  - Disassocaite WiFi client with mac addr
@return
  true - Success
  false - Failure

@note
  - Disassocaite WiFi client with mac addr by hostapd_cli disaasocaite command

- Dependencies
  - None

- Side Effects
  - None
*/
/*=========================================================================*/
boolean QCMAP_WLAN_WKK::DisAssociateClient_Device(const char* device, const char* mac_addr_str)
{
  if (NULL !=mac_addr_str)
  {
     int if_index = GetWlanIfaceName(device);
     if (if_index != -1)
     {
       QCMAP_LAN_CLIENT_RUN_COMMANDS("hostapd_cli -i %s -p /var/run/hostapd-wifi%d/ disassociate %s", device, if_index, mac_addr_str);
       return true;
     }
  }

  return false;

}

boolean QCMAP_WLAN_WKK::RestartTetheredWLANClient()
{
  char result[QCMAP_MAX_SCAN_SIZE] = {0};

  char *token=NULL;
  char *ptr;
  int if_index = 0;
  memset(result, 0, QCMAP_MAX_SCAN_SIZE);

  //get all ath devices
  if ( !ExecuteSystemCmd("brctl show | grep ath | tr '\n' '\t'", result, sizeof(result)) )
  {
    return false;
  }
  token = strtok_r(result, "\t",&ptr);
  while (token != NULL)
  {
    // restart.
    if (!strncmp(token, "ath", strlen("ath")) && ((if_index = GetWlanIfaceName(token)) != -1))
    {
      QCMAP_LAN_CLIENT_RUN_COMMANDS("wifi multi_down wifi%d %s", if_index, token);
      QCMAP_LAN_CLIENT_RUN_COMMANDS("wifi multi_up   wifi%d %s", if_index, token);
    }
    token = strtok_r(NULL, "\t", &ptr);
  }
  QCMAP_LAN_CLIENT_RUN_COMMANDS("%s %s", WLAN_CONFIG_FILE, CMD_START_HOSTAPD_CLI_SERVICE_STA);

  return true;

}

boolean QCMAP_WLAN_WKK::ResetWLANatBootup()
{
  /* wifi reset */
  QCMAP_LAN_CLIENT_RUN_COMMANDS("%s %s", WLAN_CONFIG_FILE, CMD_RESET_WLAN_ATBOOTUP);
  LOG_MSG_INFO1("WLAN reset at bootup successfully", 0, 0, 0);
  return true;
}

