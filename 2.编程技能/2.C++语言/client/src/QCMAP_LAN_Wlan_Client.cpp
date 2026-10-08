/*====================================================

FILE:  QCMAP_LAN_Wlan_Client.cpp

SERVICES:
QCMAP LAN GSB Wlan Implementation

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
#include "QCMAP_WLAN_Common.h"



/*=====================================================================
  FUNCTION EnableWLAN
======================================================================*/
/*!
@brief
  - Enable WLAN from qcmap client

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
boolean QCMAP_LAN_Client::EnableWLAN(qmi_error_type_v01       *qmi_err_num)
{
  char cmd[QCMAP_MAX_COMMAND_LEN] = {0};
  char result[QCMAP_MAX_SCAN_SIZE] = {0};
  qcmap_msgr_wlan_mode_enum_v01 wifi_mode = QCMAP_MSGR_WLAN_MODE_ENUM_MIN_ENUM_VAL_V01;
  uint8_t ezmesh_enable = 0;

  UCI_EZMESH_GET_INT_OPTION(ezmesh_enable, "ezmesh", "enable_state");
  if(ezmesh_enable)
  {
    LOG_MSG_ERROR("Cannot Enable WLAN since EZMesh is already Enabled",0,0,0);
    printf("Enable WLAN is not possible since EZMesh is enabled\n");
    return false;
  }

  if(m_pQCMapWlanObj == NULL) return false;

  IsWlanEnabled = m_pQCMapWlanObj->EnableWLAN(qmi_err_num);
  return IsWlanEnabled;
}

boolean QCMAP_LAN_Client::EnableWLAN()
{
  qmi_error_type_v01   qmi_err_num;
  return EnableWLAN(&qmi_err_num);
}

boolean QCMAP_LAN_Client::IsWifiUp(qmi_error_type_v01       *qmi_err_num)
{
   return m_pQCMapWlanObj->IsWifiUp(qmi_err_num);
}

/*=====================================================================
  FUNCTION DisableWLAN
======================================================================*/
/*!
@brief
  - Disable WLAN from qcmap client

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
boolean QCMAP_LAN_Client::DisableWLAN(qmi_error_type_v01      *qmi_err_num)
{
  boolean ret = false;
  uint8_t ap_sta_bridge_mode = 0;
  char result[QCMAP_MAX_SCAN_SIZE] = {0};

  if(m_pQCMapWlanObj == NULL) return false;

  UCI_WLAN_GET_INT_OPTION(ap_sta_bridge_mode, "stamodeconfig", "bridge_mode");

  IsWlanEnabled = false;
  ret = m_pQCMapWlanObj->DisableWLAN(qmi_err_num);
  if(ap_sta_bridge_mode == 1)
  {
    RestartTetheredClient(QCMAP_LAN_DEVICE_TYPE_USB);
    RestartTetheredClient(QCMAP_LAN_DEVICE_TYPE_ETHERNET);
    RestartTetheredClient(QCMAP_LAN_DEVICE_TYPE_ETHERNET_NIC2);
  }
  return ret;
}

boolean QCMAP_LAN_Client::IsWLANEnable()
{
  return IsWlanEnabled;
}


/*=====================================================================
  FUNCTION ActivateWLAN
======================================================================*/
/*!
@brief
  - ActivateWLAN WLAN from qcmap client

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
boolean QCMAP_LAN_Client::ActivateWLAN(qmi_error_type_v01       *qmi_err_num)
{
  if(m_pQCMapWlanObj == NULL) return false;

  IsWlanEnabled = m_pQCMapWlanObj->ActivateWLAN(qmi_err_num);
  return IsWlanEnabled ;
}

/*=====================================================================
  FUNCTION SetWLANBootupConfigEx
======================================================================*/
/*!
@brief
  - SetWLANBootupConfigEx WLAN from qcmap client

@return
  true - Success
  false - Failure

@param[in]
  qcmap_wlan_enable_bootup_conf wlan_bootup_enable_config

@note
  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
boolean QCMAP_LAN_Client::SetWLANBootupConfigEx
(
  qcmap_msgr_bootup_flag_v01 wlan_enable,
  qmi_error_type_v01       *qmi_err_num
)
{
  if(m_pQCMapWlanObj == NULL) return false;

  return m_pQCMapWlanObj->SetWLANBootupConfigEx(wlan_enable, qmi_err_num);
}

/*=====================================================================
  FUNCTION GetWLANBootupConfigEx
======================================================================*/
/*!
@brief
  - GetWLANBootupConfigEx WLAN from qcmap client

@return
  true - Success
  false - Failure

@param[in]
  qcmap_wlan_enable_bootup_conf wlan_bootup_enable_config

@note
  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
boolean QCMAP_LAN_Client::GetWLANBootupConfigEx
(
  qcmap_bootup_enable_config *wlan_bootup_enable_config,
  qmi_error_type_v01 *qmi_err_num
)

{
  if(m_pQCMapWlanObj == NULL) return false;

  return m_pQCMapWlanObj->GetWLANBootupConfigEx(wlan_bootup_enable_config, qmi_err_num);
}

/*=====================================================================
  FUNCTION SetWLANConfigEx
======================================================================*/
/*!
@brief
  - SetWLANConfigEx WLAN from qcmap client

@return
  true - Success
  false - Failure

@param[in]
  qcmap_wlan_ex_config wlan_config

@note
  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
boolean QCMAP_LAN_Client::SetWLANConfigEx
(
  qcmap_wlan_ex2_config& wlan_config,
  qmi_error_type_v01 *qmi_err_num
)
{
  uint8_t ezmesh_enable = 0;
  char result[QCMAP_MAX_SCAN_SIZE] = {'\0'};

  UCI_EZMESH_GET_INT_OPTION(ezmesh_enable, "ezmesh", "enable_state");
  if(ezmesh_enable)
  {
    LOG_MSG_ERROR("Cannot Set WLAN Config since EZMesh is already Enabled",0,0,0);
    printf("Set WLAN config is not possible since EZMesh is enabled\n");
    *qmi_err_num = QMI_ERR_INTERNAL_V01;
    return false;
  }

  if(m_pQCMapWlanObj == NULL) return false;

  return m_pQCMapWlanObj->SetWLANConfigEx(wlan_config, qmi_err_num);
}

/*=====================================================================
  FUNCTION SetWLANConfigEx3
======================================================================*/
/*!
@brief
  - SetWLANConfigEx3 WLAN from qcmap client

@return
  true - Success
  false - Failure

@param[in]
  qcmap_wlan_ex3_config_t wlan_config
  qmi_error_type_v01  *qmi_err_num

@note
  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
boolean QCMAP_LAN_Client::SetWLANConfigEx3
(
  qcmap_wlan_ex3_config_t& wlan_config,
  qmi_error_type_v01 *qmi_err_num
)
{
  uint8_t ezmesh_enable = 0;
  char result[QCMAP_MAX_SCAN_SIZE] = {'\0'};

  UCI_EZMESH_GET_INT_OPTION(ezmesh_enable, "ezmesh", "enable_state");
  if(ezmesh_enable == QCMAP_MSGR_EZMESH_ENABLED_V01)
  {
    LOG_MSG_ERROR("Cannot Set WLAN Config since EZMesh is already Enabled",0,0,0);
    printf("Set WLAN config is not possible since EZMesh is enabled\n");
    *qmi_err_num = QMI_ERR_INTERNAL_V01;
    return false;
  }

  if(m_pQCMapWlanObj == NULL)
  {
    LOG_MSG_ERROR("m_pQCMapWlanObj is NULL",0,0,0);
    *qmi_err_num = QMI_ERR_INTERNAL_V01;
    return false;
  }

  return m_pQCMapWlanObj->SetWLANConfigEx3(wlan_config, qmi_err_num);
}


/*=====================================================================
  FUNCTION GetWLANConfigEx3
======================================================================*/
/*!
@brief
  - SetWLANConfigEx3 WLAN from qcmap client

@return
  true - Success
  false - Failure

@param[in]
  qcmap_wlan_ex3_config_t* wlan_config
  qmi_error_type_v01  *qmi_err_num

@note
  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
boolean QCMAP_LAN_Client::GetWLANConfigEx3
(
  qcmap_wlan_ex3_config_t* wlan_config,
  qmi_error_type_v01 *qmi_err_num
)
{

  if(m_pQCMapWlanObj == NULL)
  {
    LOG_MSG_ERROR("m_pQCMapWlanObj is NULL",0,0,0);
    *qmi_err_num = QMI_ERR_INTERNAL_V01;
    return false;
  }

  return m_pQCMapWlanObj->GetWLANConfigEx3(wlan_config, qmi_err_num);
}


/*=====================================================================
  FUNCTION GetWLANConfigEx
======================================================================*/
/*!
@brief
  - GetWLANConfigEx WLAN from qcmap client

@return
  true - Success
  false - Failure

@param[in/out]
  qcmap_wlan_ex_config *wlan_config

@note
  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
boolean QCMAP_LAN_Client::GetWLANConfigEx
(
  qcmap_wlan_ex_config *wlan_config,
  qmi_error_type_v01 *qmi_err_num
)
{
  if(m_pQCMapWlanObj == NULL) return false;

  return m_pQCMapWlanObj->GetWLANConfigEx(wlan_config, qmi_err_num);
}

/*=====================================================================
  FUNCTION GetWLANStatus
======================================================================*/
/*!
@brief
  - GetWLANStatus from qcmap client

@return
  true - Success
  false - Failure


@param[in/out]
  qcmap_wlan_mode_enum& wlan_mode

@note
  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
 bool QCMAP_LAN_Client::GetWLANStatus
 (
 qcmap_msgr_wlan_mode_enum_v01* wlan_mode,
 qmi_error_type_v01 *qmi_err_num
 )
{
  if(m_pQCMapWlanObj == NULL) return false;

  return m_pQCMapWlanObj->GetWLANStatus(wlan_mode, qmi_err_num);
}

/*=====================================================================
  FUNCTION GetStationModeStatus
======================================================================*/
/*!
@brief
  - GetStationModeStatus from qcmap client

@return
  true - Success
  false - Failure


@param[in/out]
  qcmap_sta_status_enum& sta_status

@note
  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
 bool QCMAP_LAN_Client::GetStationModeStatus
(
 qcmap_msgr_station_mode_status_enum_v01* sta_status,
 qmi_error_type_v01 *qmi_err_num
)
{
  if(m_pQCMapWlanObj == NULL) return false;

  return m_pQCMapWlanObj->GetStationModeStatus(sta_status, qmi_err_num);
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
    +---------+---------+-------------+-----------+
    |        |         |             |           |
    | IF Name | AP type |  Card Type |   State   |
    |        |         |             |           |
    +---------+---------+-------------+-----------+
    |    ath0|  Primary|       WAKIKI|    Enabled|
    +---------+---------+-------------+-----------+
    |   ath01|    Guest|       WAKIKI|    Enabled|
    +---------+---------+-------------+-----------+

 */
/*=========================================================================*/
boolean QCMAP_LAN_Client::GetActiveWlanIfInfo
(
qcmap_msgr_wlan_if_info_t *wlan_info_cfg,
qmi_error_type_v01 *qmi_err_num
)
{
  if(m_pQCMapWlanObj == NULL) return false;

  return m_pQCMapWlanObj->GetActiveWlanIfInfo(wlan_info_cfg, qmi_err_num);
}

/*=====================================================================
  FUNCTION ActivateHostapdConfig
======================================================================*/
/*!
@brief
  - Activates the hostapd configuration from qcmap client

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
boolean QCMAP_LAN_Client::ActivateHostapdConfig
(
  qcmap_msgr_activate_hostapd_ap_enum_v01 ap_type,
  qcmap_msgr_activate_hostapd_action_enum_v01 action_type,
  qmi_error_type_v01 *qmi_err_num
)
{
  if(m_pQCMapWlanObj == NULL) return false;

  return m_pQCMapWlanObj->ActivateHostapdConfig(ap_type, action_type, qmi_err_num);
}

/*=====================================================================
  FUNCTION ActivateSupplicantConfig
======================================================================*/
/*!
@brief
  - Activates the WPA supplicant configuration from qcmap client

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
boolean QCMAP_LAN_Client::ActivateSupplicantConfig(qmi_error_type_v01 *qmi_err_num)
{
  if(m_pQCMapWlanObj == NULL) return false;

  return m_pQCMapWlanObj->ActivateSupplicantConfig(qmi_err_num);
}

boolean QCMAP_LAN_Client::DisAssociateClient(const char* mac_addr_str)
{
  if(m_pQCMapWlanObj == NULL) return false;

  return m_pQCMapWlanObj->DisAssociateClient(mac_addr_str);
}

/*===========================================================================
  FUNCTION SwitchWlanEnableBand
===========================================================================*/
/*!
@brief
  This function is to switch sap/sta between 5GHz and 2.5GHz

@parameters
- sap_5g_enable_state
- sta_5g_enable_state

@return
  true
  false

@note
- Dependencies
- None

- Side Effects
- None
*/
/*=========================================================================*/
boolean QCMAP_LAN_Client::SwitchWlanEnableBand
(
  qcmap_msgr_sap_band_status_enum_v01 sap_5g_enable_state,
  qcmap_msgr_sta_band_status_enum_v01 sta_5g_enable_state
)
{
  if(m_pQCMapWlanObj == NULL) return false;

  return m_pQCMapWlanObj->SwitchWlanEnableBand(sap_5g_enable_state, sta_5g_enable_state);
}

/*===========================================================================
  FUNCTION ProcessStaStatusInd
===========================================================================*/
/*!
@brief
  This function is to process station associate/disassociate

@parameters
- sta_connection_state

@return
  true
  false

@note
- Dependencies
- None

- Side Effects
- None
*/
/*=========================================================================*/
boolean QCMAP_LAN_Client::ProcessStaStatusInd
(
  qcmap_msgr_sta_connect_status_enum_v01 sta_connection_state
)
{
  boolean ret = false;
  if(m_pQCMapWlanObj == NULL) return false;

  ret = m_pQCMapWlanObj->ProcessStaIndicate(sta_connection_state);
  if(m_pQCMapWlanObj->IsBridgeMode())
  {
    //in case station bridge connect/disconnect, need restart tether client to update addr,
    if(ret)
    {
      RestartTetheredClient(QCMAP_LAN_DEVICE_TYPE_USB);
      RestartTetheredClient(QCMAP_LAN_DEVICE_TYPE_ETHERNET);
      RestartTetheredClient(QCMAP_LAN_DEVICE_TYPE_ETHERNET_NIC2);
      RestartTetheredClient(QCMAP_LAN_DEVICE_TYPE_ALL_AP);
    }
  }
  return ret;
}

boolean QCMAP_LAN_Client::ResetWLANatBootup()
{
  if(m_pQCMapWlanObj == NULL) return false;

  return m_pQCMapWlanObj->ResetWLANatBootup();
}

boolean QCMAP_LAN_Client::IsWiFiDevcies
(
  const char *phy_iface
)
{
  if(m_pQCMapWlanObj == NULL) return false;

  return m_pQCMapWlanObj->IsWiFiDevcies(phy_iface);
}

boolean QCMAP_LAN_Client::InstallGuestAPRules()
{
  if(m_pQCMapWlanObj == NULL) return false;

  return m_pQCMapWlanObj->InstallGuestAPRules();
}
boolean QCMAP_LAN_Client::UnInstallGuestAPRules()
{
  if(m_pQCMapWlanObj == NULL) return false;

  return m_pQCMapWlanObj->UnInstallGuestAPRules();
}


