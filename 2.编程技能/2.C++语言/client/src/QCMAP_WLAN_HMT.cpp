/*====================================================

FILE:  QCMAP_WLAN_HMT.cpp

SERVICES:
QCMAP WLAN Client Implementation

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
#include "QCMAP_WLAN_HMT.h"


/*===================================================================
  Class Definitions
  ===================================================================*/

/*===========================================================================
  FUNCTION QCMAP_WLAN_HMT
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
QCMAP_WLAN_HMT::QCMAP_WLAN_HMT()
{
  strlcpy(mWLANConfigFile, WLAN_CONFIG_FILE, MAX_FILE_PATH_LEN);
  LOG_MSG_ERROR("WLAN HMT object created",0,0,0);
  return;
}

QCMAP_WLAN_HMT::~QCMAP_WLAN_HMT()
{
  return;
}

/*===========================================================================
  FUNCTION FillActiveWlanIfInfo
  ===========================================================================*/
/*!
  @brief
  Fill iface name info in wlanIfInfo.

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
boolean QCMAP_WLAN_HMT::FillSpecificWlanIfInfo(qcmap_msgr_wlan_if_info_t *wlan_info_cfg, int wlan_mode)
{
  if (!wlan_info_cfg)
  {
    LOG_MSG_ERROR("Invalid parameter passed ",0,0,0);
    return false;
  }
  if (wlan_mode == QCMAP_MSGR_WLAN_MODE_AP_V01)
  {
      strlcpy(wlan_info_cfg->wlan_if_info[0].if_name, WLAN_DEVICE0, QCMAP_MAX_IFACE_NAME_SIZE_V01);
  }
  else if (wlan_mode == QCMAP_MSGR_WLAN_MODE_AP_AP_V01)
  {
      strlcpy(wlan_info_cfg->wlan_if_info[0].if_name, WLAN_DEVICE0, QCMAP_MAX_IFACE_NAME_SIZE_V01);
      strlcpy(wlan_info_cfg->wlan_if_info[1].if_name, WLAN_DEVICE1, QCMAP_MAX_IFACE_NAME_SIZE_V01);
  }
  else if(wlan_mode == QCMAP_MSGR_WLAN_MODE_AP_STA_V01)
  {
      strlcpy(wlan_info_cfg->wlan_if_info[0].if_name, WLAN_DEVICE1, QCMAP_MAX_IFACE_NAME_SIZE_V01);
      strlcpy(wlan_info_cfg->wlan_if_info[1].if_name, WLAN_DEVICE0, QCMAP_MAX_IFACE_NAME_SIZE_V01);
  }
  else if(wlan_mode == QCMAP_MSGR_WLAN_MODE_AP_AP_AP_V01)
  {
      strlcpy(wlan_info_cfg->wlan_if_info[0].if_name, WLAN_DEVICE0, QCMAP_MAX_IFACE_NAME_SIZE_V01);
      strlcpy(wlan_info_cfg->wlan_if_info[1].if_name, WLAN_DEVICE1, QCMAP_MAX_IFACE_NAME_SIZE_V01);
      strlcpy(wlan_info_cfg->wlan_if_info[2].if_name, WLAN_DEVICE2, QCMAP_MAX_IFACE_NAME_SIZE_V01);
  }
  else if(wlan_mode == QCMAP_MSGR_WLAN_MODE_AP_AP_STA_V01)
  {
      strlcpy(wlan_info_cfg->wlan_if_info[0].if_name, WLAN_DEVICE1, QCMAP_MAX_IFACE_NAME_SIZE_V01);
      strlcpy(wlan_info_cfg->wlan_if_info[1].if_name, WLAN_DEVICE2, QCMAP_MAX_IFACE_NAME_SIZE_V01);
      strlcpy(wlan_info_cfg->wlan_if_info[2].if_name, WLAN_DEVICE0, QCMAP_MAX_IFACE_NAME_SIZE_V01);
  }
  else if (wlan_mode == QCMAP_MSGR_WLAN_MODE_STA_ONLY_V01)
  {
      strlcpy(wlan_info_cfg->wlan_if_info[0].if_name, WLAN_DEVICE0, QCMAP_MAX_IFACE_NAME_SIZE_V01);
  }
  else if (wlan_mode == QCMAP_MSGR_WLAN_MODE_AP_AP_AP_AP_V01)
  {
      strlcpy(wlan_info_cfg->wlan_if_info[0].if_name, WLAN_DEVICE0, QCMAP_MAX_IFACE_NAME_SIZE_V01);
      strlcpy(wlan_info_cfg->wlan_if_info[1].if_name, WLAN_DEVICE1, QCMAP_MAX_IFACE_NAME_SIZE_V01);
      strlcpy(wlan_info_cfg->wlan_if_info[2].if_name, WLAN_DEVICE2, QCMAP_MAX_IFACE_NAME_SIZE_V01);
      strlcpy(wlan_info_cfg->wlan_if_info[3].if_name, WLAN_DEVICE3, QCMAP_MAX_IFACE_NAME_SIZE_V01);
  }

  //to add: QCMAP_MSGR_WLAN_DEV_HMT_V01 is not included in qcmap_msgr_wlan_device_type_v01
  for (int j = 0; j < wlan_info_cfg->wlan_if_info_len; j++)
  {
      wlan_info_cfg->wlan_if_info[j].wlan_dev_type = QCMAP_MSGR_WLAN_DEV_HMT_V01;
  }
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
boolean QCMAP_WLAN_HMT::DisAssociateClient_Device(const char* device, const char* mac_addr_str)
{
  if (NULL !=mac_addr_str )
  {
     QCMAP_LAN_CLIENT_RUN_COMMANDS("hostapd_cli -i %s disassociate %s", device, mac_addr_str);
  }
  return true;
}

boolean QCMAP_WLAN_HMT::RestartTetheredWLANClient()
{
  //restart hostapd and start hostapd_cli process
  QCMAP_LAN_CLIENT_RUN_COMMANDS("%s %s", WLAN_CONFIG_FILE, CMD_RESTART_TETHERED_WLAN_CLIENT);
  return true;
}

boolean QCMAP_WLAN_HMT::ResetWLANatBootup()
{
  /* wifi reset */
  QCMAP_LAN_CLIENT_RUN_COMMANDS("%s %s", WLAN_CONFIG_FILE, CMD_RESET_WLAN_ATBOOTUP);
  LOG_MSG_INFO1("WLAN reset at bootup successfully", 0, 0, 0);
  return true;
}

boolean QCMAP_WLAN_HMT::IsWifiUp(qmi_error_type_v01 *qmi_err_num)
{
       //Do nothing
       return true;
}
