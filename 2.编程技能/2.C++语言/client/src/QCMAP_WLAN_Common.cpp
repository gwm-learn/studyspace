/*====================================================

FILE:  QCMAP_WLAN_Common.cpp

SERVICES:
QCMAP WLAN Implementation

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
#include "QCMAP_WLAN_Common.h"

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef __cplusplus
}
#endif

/*===========================================================================
  FUNCTION QCMAP_WLAN_Common
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
QCMAP_WLAN_Common::QCMAP_WLAN_Common()
{
  IsWlanEnabled = false;
  return;
}

QCMAP_WLAN_Common::~QCMAP_WLAN_Common()
{
  return;
}

boolean QCMAP_WLAN_Common::FillSpecificWlanIfInfo(qcmap_msgr_wlan_if_info_t * wlan_info_cfg, int wlan_mode)
{
  return false;
}

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
boolean QCMAP_WLAN_Common::EnableWLAN(qmi_error_type_v01 *qmi_err_num)
{
  char cmd[QCMAP_MAX_COMMAND_LEN] = {0};
  char result[QCMAP_MAX_SCAN_SIZE] = {0};
  qcmap_msgr_wlan_mode_enum_v01 wifi_mode = QCMAP_MSGR_WLAN_MODE_ENUM_MIN_ENUM_VAL_V01;

  //Get SAP status before enable WLAN.
  snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "%s %s",
                            mWLANConfigFile,
                            CMD_GET_SAP_STATUS);
  if (ExecuteSystemCmd((const char *)cmd, result, sizeof(result)))
  {
    if (strstr(result, RESULT_SAP_STATE_ENABLED) ||
        strstr(result, RESULT_SAP_STATE_DFS) ||
        strstr(result, RESULT_STA_ONLY_MODE))
    {
      LOG_MSG_INFO1(" WLAN is already enabled, first disable then enable.", 0, 0, 0);
      QCMAP_LAN_CLIENT_RUN_COMMANDS("echo QCMAP:WLAN is already enabled > /dev/kmsg");
      printf("WLAN is already enabled\n");
      *qmi_err_num = QMI_ERR_NO_EFFECT_V01;
      return false;
    }
  }

  memset(cmd, 0, QCMAP_MAX_COMMAND_LEN);
  memset(result, 0, QCMAP_MAX_SCAN_SIZE);
  QCMAP_LAN_CLIENT_RUN_COMMANDS("%s %s", mWLANConfigFile, CMD_ENABLE_WLAN);

  /* Get AP status, check hostapd is running or not. */
  memset(cmd, 0, QCMAP_MAX_COMMAND_LEN);
  snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "%s %s",
                            mWLANConfigFile,
                            CMD_GET_SAP_STATUS);
  if (ExecuteSystemCmd((const char *)cmd, result, sizeof(result)))
  {
    if (!strstr(result, RESULT_SAP_STATE_ENABLED) &&
        !strstr(result, RESULT_SAP_STATE_DFS) &&
        !strstr(result, RESULT_STA_ONLY_MODE))
    {
      LOG_MSG_ERROR("Failed to start hostapd", 0, 0, 0);
      QCMAP_LAN_CLIENT_RUN_COMMANDS("echo QCMAP:Start Hostapd failed > /dev/kmsg");

      /* Disable WLAN */
      LOG_MSG_INFO1("Disable WLAN",0,0,0);
      if (!QCMAP_WLAN_Common::DisableWLAN(qmi_err_num))
      {
        LOG_MSG_ERROR("Failed to disable WLAN",0,0,0);
        *qmi_err_num = QMI_ERR_INTERNAL_V01;
        return false;
      }
      *qmi_err_num = QMI_ERR_INTERNAL_V01;
      return false;
    }
  }
  else
  {
    LOG_MSG_ERROR("Unable to get sap status", 0, 0, 0);
    QCMAP_LAN_CLIENT_RUN_COMMANDS("echo Unable to get sap status > /dev/kmsg");
    *qmi_err_num = QMI_ERR_INTERNAL_V01;
    return false;
  }

  /* Get wlan config mode */
  memset(cmd, 0, QCMAP_MAX_COMMAND_LEN);
  memset(result, 0, QCMAP_MAX_SCAN_SIZE);
  snprintf(cmd, MAX_COMMAND_STR_LEN,"%s %s", UCI_GET_COMMAND, UCI_WLAN_CONFIG_MODE);
  if (!ExecuteSystemCmd((const char *)cmd, result, sizeof(result)))
  {
    LOG_MSG_ERROR("Failed to execute command : %s", cmd, 0, 0);
    *qmi_err_num = QMI_ERR_INTERNAL_V01;
    return false;
  }
  wifi_mode = atoi(result);

  /* for STA/AP+STA/AP+AP+STA mode, check wpa_supplicant is running or not. */
  if (wifi_mode  == QCMAP_MSGR_WLAN_MODE_AP_STA_V01 ||
      wifi_mode  == QCMAP_MSGR_WLAN_MODE_AP_AP_STA_V01 ||
      wifi_mode  == QCMAP_MSGR_WLAN_MODE_STA_ONLY_V01)
  {
    memset(cmd, 0, QCMAP_MAX_COMMAND_LEN);
    memset(result, 0, QCMAP_MAX_SCAN_SIZE);
    snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "%s %s",
                              mWLANConfigFile,
                              CMD_GET_SUPPLICANT_STATUS);
    if (ExecuteSystemCmd((const char *)cmd, result, sizeof(result)))
    {
      if (!strstr(result, RESULT_SUPPLICANT_RUNNING_NORMAL))
      {
        LOG_MSG_ERROR("Failed to run supplicant", 0, 0, 0);
        QCMAP_LAN_CLIENT_RUN_COMMANDS("echo QCMAP:Start Supplicant failed > /dev/kmsg");
        *qmi_err_num = QMI_ERR_INTERNAL_V01;
        return false;
      }
    }
    else
    {
      LOG_MSG_ERROR("Unable to get supplicant status", 0, 0, 0);
      QCMAP_LAN_CLIENT_RUN_COMMANDS("echo Unable to get supplicant status > /dev/kmsg");
      *qmi_err_num = QMI_ERR_INTERNAL_V01;
      return false;
    }
  }

  LOG_MSG_INFO1("Enable WLAN",0,0,0);
  QCMAP_LAN_CLIENT_RUN_COMMANDS("echo QCMAP:WLAN Enabled > /dev/kmsg");

  /* Start Hostapd and Wap cli service. */
  QCMAP_LAN_CLIENT_RUN_COMMANDS("%s %s", mWLANConfigFile, CMD_START_HOSTAPD_CLI_SERVICE_STA);
  QCMAP_LAN_CLIENT_RUN_COMMANDS("%s %s", mWLANConfigFile, CMD_START_WPA_CLI_SERVICE);

  /* Install access restrictions for the Guest SSID if the profile is INTERNETONLY. */
  QCMAP_LAN_CLIENT_RUN_COMMANDS("%s %s", mWLANConfigFile, CMD_INSTALL_GUEST_AP_ACCESS_RULES);

  IsWlanEnabled = true;
  *qmi_err_num = QMI_ERR_NONE_V01;
  return true;
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
boolean QCMAP_WLAN_Common::DisableWLAN(qmi_error_type_v01 *qmi_err_num)
{
  /* Flush Guest AP access rules*/
  QCMAP_LAN_CLIENT_RUN_COMMANDS("%s %s", mWLANConfigFile, CMD_DELETE_GUEST_AP_ACCESS_RULES);
  QCMAP_LAN_CLIENT_RUN_COMMANDS("%s %s", mWLANConfigFile, CMD_DISABLE_WLAN);

  LOG_MSG_INFO1("Disable WLAN",0,0,0);
  IsWlanEnabled = false;
  *qmi_err_num = QMI_ERR_NONE_V01;
  return true;
}

/*===========================================================================
FUNCTION SetCoexConfig()
===========================================================================*/
/*
  Enable/Disable the CoEX feature to reduce co-channel interference
  between WLAN <-> WWAN channels.

  @param[in]  coex_state   Enable/Disable CoEX.

  @return
  TRUE -- Success.
  FALSE -- Failure.

  @dependencies
  None
*/
boolean QCMAP_WLAN_Common::SetCoexConfig
(
  int coex_state
)
{
   QCMAP_LAN_CLIENT_RUN_COMMANDS("%s %s %d", mWLANConfigFile, SET_COEX_CONFIG, coex_state);
   LOG_MSG_INFO1("Coex Configuration is set",0,0,0);
   return true;
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
boolean QCMAP_WLAN_Common::ActivateWLAN(qmi_error_type_v01 *qmi_err_num)
{
  char cmd[QCMAP_MAX_COMMAND_LEN] = {0};
  /* Disable WLAN */
  if (!QCMAP_WLAN_Common::DisableWLAN(qmi_err_num))
  {
    LOG_MSG_ERROR("Failed to disable WLAN",0,0,0);
    *qmi_err_num = QMI_ERR_INTERNAL_V01;
    return false;
  }
  /* Enable WLAN */
  if (!QCMAP_WLAN_Common::EnableWLAN(qmi_err_num))
  {
    LOG_MSG_ERROR("Failed to enable WLAN",0,0,0);
    *qmi_err_num = QMI_ERR_INTERNAL_V01;
    return false;
  }
  LOG_MSG_INFO1("Activate WLAN",0,0,0);
  *qmi_err_num = QMI_ERR_NONE_V01;
  IsWlanEnabled = true;
  return true;
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
  qcmap_bootup_enable_config bootup_config

@note
  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
boolean QCMAP_WLAN_Common::SetWLANBootupConfigEx
(
  qcmap_msgr_bootup_flag_v01 wlan_enable,
  qmi_error_type_v01 *qmi_err_num
)
{
  QCMAP_LAN_CLIENT_RUN_COMMANDS("%s %s %d",
    mWLANConfigFile, CMD_WLAN_SET_BOOUP_ENABLE, wlan_enable);
  *qmi_err_num = QMI_ERR_NONE_V01;
  return true;
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
  qcmap_bootup_enable_config bootup_config

@note
  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
boolean QCMAP_WLAN_Common::GetWLANBootupConfigEx
(
  qcmap_bootup_enable_config *bootup_config,
  qmi_error_type_v01 *qmi_err_num
)
{
  char command[QCMAP_MAX_COMMAND_LEN] = {0};
  char result[QCMAP_MAX_SCAN_SIZE] = {0};

  if(bootup_config == NULL)
  {
    LOG_MSG_ERROR("NULL pointer passed ",0,0,0);
    *qmi_err_num = QMI_ERR_INVALID_ARG_V01;
    return false;
  }

  snprintf(command, QCMAP_MAX_COMMAND_LEN, "%s %s", UCI_GET_COMMAND, UCI_WLAN_BOOTUP_SETTING);

  if (!ExecuteSystemCmd((const char*)command, result, sizeof(result)))
  {
    LOG_MSG_ERROR("Failed to execute command : %s",command,0,0);
    *qmi_err_num = QMI_ERR_INTERNAL_V01;
    return false;
  }
  bootup_config->wlan_enable = atoi(result);
  *qmi_err_num = QMI_ERR_NONE_V01;
  return true;
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
boolean QCMAP_WLAN_Common::SetWLANConfigEx
(
  qcmap_wlan_ex2_config & wlan_config,
  qmi_error_type_v01     *qmi_err_num
)
{
/* validate the primary ap band */
  if (wlan_config.primary_ap_band != 0 & !QCMAP_VALIDATE_BAND_INFO(wlan_config.primary_ap_band) )
  {
    LOG_MSG_ERROR("Invalid primary ap band ",0,0,0);
    *qmi_err_num = QMI_ERR_INVALID_ARG_V01;
    return false;
  }

/* validate the gust 1/2/3 ap band */
  int temp_gust_ap = 0;
  while (temp_gust_ap < QCMAP_MSGR_MAX_GUEST_AP_COUNT)
  {
    if (wlan_config.ap_config[temp_gust_ap].band != 0 & !QCMAP_VALIDATE_BAND_INFO(wlan_config.ap_config[temp_gust_ap].band))
    {
        LOG_MSG_ERROR("Invalid guest AP %d band ", temp_gust_ap,0,0);
        *qmi_err_num = QMI_ERR_INVALID_ARG_V01;
        return false;
    }
    temp_gust_ap++;
  }

/* validate the station band */
  if (wlan_config.station_band != 0 & !QCMAP_VALIDATE_BAND_INFO(wlan_config.station_band) )
  {
    LOG_MSG_ERROR("Invalid station_band ",0,0,0);
    *qmi_err_num = QMI_ERR_INVALID_ARG_V01;
    return false;
  }

  /* Set WLAN config */
  char cmd[QCMAP_MAX_COMMAND_LEN] = {0};
  char strIPv4Addr   [INET_ADDRSTRLEN];
  char strIPv4GwIP   [INET_ADDRSTRLEN];
  char strIPv4Netmask[INET_ADDRSTRLEN];
  char strIPv4DNS    [INET_ADDRSTRLEN];

  /* Set WLAN mode */
  QCMAP_LAN_CLIENT_RUN_COMMANDS("%s %s %d", mWLANConfigFile, CMD_WLAN_SET_MODE,
                                            wlan_config.wlan_mode);

  /* Set primary AP config */
  QCMAP_LAN_CLIENT_RUN_COMMANDS("%s %s %d", mWLANConfigFile, CMD_SET_PRIMARY_AP_CONFIG,
                                            wlan_config.primary_ap_band);

  /* Set guest AP config */
  memset(cmd, 0, QCMAP_MAX_COMMAND_LEN);
  for(int i = wlan_config.ap_config_len; i < QCMAP_MSGR_MAX_GUEST_AP_COUNT; i++)
  {
    //ensure unused ap_config is zero
    memset(&(wlan_config.ap_config[i]), 0 , sizeof(qcmap_msgr_wlan_ap_band_config_v01));
  }

  if (wlan_config.wlan_mode != QCMAP_MSGR_WLAN_MODE_7AP_V01)
  {
    QCMAP_LAN_CLIENT_RUN_COMMANDS("%s %s %d %d %d %d %d %d",
                            mWLANConfigFile,
                            CMD_SET_GUEST_AP_CONFIG,
                            wlan_config.ap_config[0].band,
                            wlan_config.ap_config[0].guest_ap_profile,
                            wlan_config.ap_config[1].band,
                            wlan_config.ap_config[1].guest_ap_profile,
                            wlan_config.ap_config[2].band,
                            wlan_config.ap_config[2].guest_ap_profile);
  /* Set station config */
  memset(strIPv4Addr,    0, INET_ADDRSTRLEN);
  memset(strIPv4GwIP,    0, INET_ADDRSTRLEN);
  memset(strIPv4Netmask, 0, INET_ADDRSTRLEN);
  memset(strIPv4DNS,     0, INET_ADDRSTRLEN);

  /* don't need care return value, in case addr not set, the string will be empty */
  readable_addr(AF_INET, &wlan_config.station_config.static_ip_config.ip_addr,  strIPv4Addr);
  readable_addr(AF_INET, &wlan_config.station_config.static_ip_config.gw_ip,    strIPv4GwIP);
  readable_addr(AF_INET, &wlan_config.station_config.static_ip_config.netmask,  strIPv4Netmask);
  readable_addr(AF_INET, &wlan_config.station_config.static_ip_config.dns_addr, strIPv4DNS);

  QCMAP_LAN_CLIENT_RUN_COMMANDS("%s %s %d %d %d %s %s %s %s",
                            mWLANConfigFile,
                            CMD_SET_STA_CONFIG,
                            wlan_config.station_band,
                            wlan_config.station_config.ap_sta_bridge_mode,
                            wlan_config.station_config.conn_type,
                            strIPv4Addr, strIPv4GwIP, strIPv4Netmask, strIPv4DNS);

  }
  else
  {
    for (int i = 0; i < wlan_config.ap_config_len; i++)
    {
       memset(cmd, 0, QCMAP_MAX_COMMAND_LEN);
       snprintf(cmd, QCMAP_MAX_COMMAND_LEN,"%s %s %d %d %d",
                                mWLANConfigFile,
                                CMD_SET_GUEST_7AP_CONFIG,
                                wlan_config.ap_config[i].band,
                                wlan_config.ap_config[i].guest_ap_profile,
                                i);
       ds_system_call(cmd, strlen(cmd));
    }
    /* Update guest ap count in 2G/5G/6G radio*/
     QCMAP_LAN_CLIENT_RUN_COMMANDS("/etc/data/uci_ex.sh set qcmap_wlan_current.@wlanconfig[0].guestapcount2g=%d",wlan_config.guestap_count_2g);
     QCMAP_LAN_CLIENT_RUN_COMMANDS("/etc/data/uci_ex.sh set qcmap_wlan_current.@wlanconfig[0].guestapcount5g=%d",wlan_config.guestap_count_5g);
     QCMAP_LAN_CLIENT_RUN_COMMANDS("/etc/data/uci_ex.sh set qcmap_wlan_current.@wlanconfig[0].guestapcount6g=%d",wlan_config.guestap_count_6g);
     QCMAP_LAN_CLIENT_RUN_COMMANDS("/etc/data/uci_ex.sh set qcmap_wlan_current.@wlanconfig[0].totalapcount=%d",  wlan_config.ap_config_len);
  }

  /* update the network/wireless configuration files */
  QCMAP_LAN_CLIENT_RUN_COMMANDS("%s %s", mWLANConfigFile, CMD_UPDATE_WIRELESS_NETWORK);
  *qmi_err_num = QMI_ERR_NONE_V01;
  return true;
}
/*=====================================================================
  FUNCTION GetWLANConfigEx3
======================================================================*/
/*!
@brief
  - GetWLANConfigEx3 WLAN from qcmap client

@return
  true - Success
  false - Failure

@param[in]
  qcmap_wlan_ex3_config_t* wlan_config

@note
  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
boolean QCMAP_WLAN_Common::GetWLANConfigEx3
(
  qcmap_wlan_ex3_config_t* wlan_config,
  qmi_error_type_v01     *qmi_err_num
)
{
  char result[QCMAP_MAX_SCAN_SIZE] = {0};
  in_addr addr;

  if(wlan_config == NULL || qmi_err_num == NULL)
  {
    LOG_MSG_ERROR("Invalid argument ",0,0,0);
    return false;
  }

  UCI_WLAN_GET_INT_OPTION(wlan_config->wlan_mode,         "wlanconfig",      "mode")
  UCI_WLAN_GET_INT_OPTION(wlan_config->ap_config_len,     "wlanconfig",      "totalapcount")
  UCI_WLAN_GET_INT_OPTION(wlan_config->mld_wlan_mode,     "mldwlanconfig",   "mode")
  UCI_WLAN_GET_INT_OPTION(wlan_config->mld_ap_config_len, "mldwlanconfig",  "totalmldapcount")

  for(int i=0; i<(wlan_config->ap_config_len); i++)
  {
     UCI_WLAN_GET_INT_OPTION_WITH_IDX(wlan_config->ap_config[i].band,
                                              "apconfig","band",i)
     UCI_WLAN_GET_INT_OPTION_WITH_IDX(wlan_config->ap_config[i].accessprofile,
                                              "apconfig","accessprofile",i)
  }

  if(wlan_config->wlan_mode == QCMAP_MSGR_WLAN_MODE_AP_STA_V01 ||
     wlan_config->wlan_mode == QCMAP_MSGR_WLAN_MODE_STA_ONLY_V01)
  {
    wlan_config->is_sta_configured = true;
    UCI_WLAN_GET_INT_OPTION(wlan_config->station_config.band,
                                       "stamodeconfig", "band")
    UCI_WLAN_GET_INT_OPTION(wlan_config->station_config.ap_sta_bridge_mode,
                                        "stamodeconfig",  "bridge_mode")
    UCI_WLAN_GET_INT_OPTION(wlan_config->station_config.conn_type,
                                        "stamodeconfig",  "sta_mode_conn_type")
    if(wlan_config->station_config.conn_type == QCMAP_MSGR_STA_CONNECTION_STATIC_V01)
    {
      // Get STATIC Config info.
      UCI_WLAN_GET_ADDR_OPTION(wlan_config->station_config.static_ip_config.ip_addr,
                                       "stamodeconfig", "ipaddr")
      UCI_WLAN_GET_ADDR_OPTION(wlan_config->station_config.static_ip_config.gw_ip,
                                           "stamodeconfig", "gateway")
      UCI_WLAN_GET_ADDR_OPTION(wlan_config->station_config.static_ip_config.netmask,
                                           "stamodeconfig", "netmask")
      UCI_WLAN_GET_ADDR_OPTION(wlan_config->station_config.static_ip_config.dns_addr,
                                           "stamodeconfig", "dnsserver")
    }
  }


  for(int i=0; i<(wlan_config->mld_ap_config_len); i++)
  {
     UCI_WLAN_GET_INT_OPTION_WITH_IDX(wlan_config->mld_ap_config[i].no_of_mld_link,
                                              "mldapconfig","no_of_mld_link",i)
     UCI_WLAN_GET_INT_OPTION_WITH_IDX(wlan_config->mld_ap_config[i].accessprofile,
                                              "mldapconfig","accessprofile",i)
  }

  if(wlan_config->mld_wlan_mode == QCMAP_MSGR_MLD_WLAN_MODE_AP_STA ||
     wlan_config->mld_wlan_mode == QCMAP_MSGR_MLD_WLAN_MODE_STA)
  {
    wlan_config->is_mld_sta_configured = true;
    UCI_WLAN_GET_INT_OPTION(wlan_config->mld_sta_config.no_of_mld_link,
                                       "mldstaconfig", "no_of_mld_link")
    UCI_WLAN_GET_INT_OPTION(wlan_config->mld_sta_config.ap_sta_bridge_mode,
                                        "mldstaconfig",  "bridge_mode")
    UCI_WLAN_GET_INT_OPTION(wlan_config->mld_sta_config.conn_type,
                                        "mldstaconfig",  "sta_mode_conn_type")
    if(wlan_config->mld_sta_config.conn_type == QCMAP_MSGR_STA_CONNECTION_STATIC_V01)
    {
      // Get STATIC Config info.
      UCI_WLAN_GET_ADDR_OPTION(wlan_config->mld_sta_config.static_ip_config.ip_addr,
                                       "mldstaconfig", "ipaddr")
      UCI_WLAN_GET_ADDR_OPTION(wlan_config->mld_sta_config.static_ip_config.gw_ip,
                                           "mldstaconfig", "gateway")
      UCI_WLAN_GET_ADDR_OPTION(wlan_config->mld_sta_config.static_ip_config.netmask,
                                           "mldstaconfig", "netmask")
      UCI_WLAN_GET_ADDR_OPTION(wlan_config->mld_sta_config.static_ip_config.dns_addr,
                                           "mldstaconfig", "dnsserver")
    }
  }

  *qmi_err_num = QMI_ERR_NONE_V01;
  return true;
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

@note
  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
boolean QCMAP_WLAN_Common::SetWLANConfigEx3
(
  qcmap_wlan_ex3_config_t& wlan_config,
  qmi_error_type_v01     *qmi_err_num
)
{

  int total_mld_links = 0;
  int total_non_mld_link = 0;

  /* Set WLAN config */
  char cmd[QCMAP_MAX_COMMAND_LEN] = {0};
  char strIPv4Addr   [INET_ADDRSTRLEN];
  char strIPv4GwIP   [INET_ADDRSTRLEN];
  char strIPv4Netmask[INET_ADDRSTRLEN];
  char strIPv4DNS    [INET_ADDRSTRLEN];
  /* Validate number of AP configudred */
  if((wlan_config.ap_config_len > QCMAP_MSGR_MAX_AP_COUNT) ||
     (wlan_config.mld_ap_config_len > QCMAP_MSGR_MAX_MLD_AP_COUNT))
  {
    LOG_MSG_ERROR("Invalid number of AP passed AP len:%d MLD AP Link: %d",
                                      wlan_config.ap_config_len,
                                      wlan_config.mld_ap_config_len, 0);
    *qmi_err_num = QMI_INTERNAL_ERR;
    return false;
  }

  total_non_mld_link = wlan_config.ap_config_len;
  for(int i=0; i< wlan_config.mld_ap_config_len; i++)
  {
    total_mld_links += wlan_config.mld_ap_config[i].no_of_mld_link;
  }
  if(wlan_config.is_mld_sta_configured)
  {
    total_mld_links += wlan_config.mld_sta_config.no_of_mld_link;
  }

  if(wlan_config.is_sta_configured)
  {
    total_non_mld_link++;
  }

  /*MLD Link and Non MLD link should not exceed 21*/
  if(total_mld_links + total_non_mld_link > QCMAP_MSGR_MAX_MLD_LINK)
  {
    LOG_MSG_ERROR("Invalid number of AP is configured MLD link: %d non-MLD link:%d",
                                      total_mld_links,
                                      total_non_mld_link, 0);
    *qmi_err_num = QMI_INTERNAL_ERR;
    return false;
  }

  if(wlan_config.is_sta_configured && wlan_config.is_mld_sta_configured)
  {
     LOG_MSG_ERROR("Invalid config, STA is configured in both MLD and non-MLD mode",0,0,0);
    *qmi_err_num = QMI_INTERNAL_ERR;
    return false;
  }

  /*Validate Band */
  if(wlan_config.ap_config_len > 0)
  {
    for(int i=0; i<wlan_config.ap_config_len; i++)
    {
      if(!QCMAP_VALIDATE_BAND_INFO(wlan_config.ap_config[i].band))
      {
        LOG_MSG_ERROR("Invalid band passed for AP:%d band:%d,",i+1,wlan_config.ap_config[i].band,0);
        *qmi_err_num = QMI_INTERNAL_ERR;
        return false;
      }
      if(wlan_config.ap_config[i].band == 2)
      {
        wlan_config.ap_count_2g++;
      }
      else if(wlan_config.ap_config[i].band == 5)
      {
        wlan_config.ap_count_5g++;
      }
      else
      {
        wlan_config.ap_count_6g++;
      }
    }
  }

  /*Validate MLD AP Band */
  if(wlan_config.mld_ap_config_len > 0)
  {
    for(int i=0; i<wlan_config.mld_ap_config_len; i++)
    {
      for(int j=0; j<wlan_config.mld_ap_config[i].no_of_mld_link; j++)
      {
        if(!QCMAP_VALIDATE_BAND_INFO(wlan_config.mld_ap_config[i].band[j]))
        {
          LOG_MSG_ERROR("Invalid band passed for MLD AP:%d band:%d valid_band:%d",
                            i+1,
                            wlan_config.mld_ap_config[i].band[j],
                            QCMAP_VALIDATE_BAND_INFO(wlan_config.mld_ap_config[i].band[j]));
          *qmi_err_num = QMI_INTERNAL_ERR;
          return false;
        }
      }
    }
  }

  /*Validate STA config*/
  if(wlan_config.is_sta_configured)
  {
    if(!QCMAP_VALIDATE_BAND_INFO(wlan_config.station_config.band))
    {
      LOG_MSG_ERROR("Invalid band passed for STA band:%d,",wlan_config.station_config.band,0,0);
      *qmi_err_num = QMI_INTERNAL_ERR;
      return false;
    }
  }

  /*Validate MLD-STA config*/
  if(wlan_config.is_mld_sta_configured)
  {
      for(int i=0; i<wlan_config.mld_sta_config.no_of_mld_link; i++)
      {
        if(!QCMAP_VALIDATE_BAND_INFO(wlan_config.mld_sta_config.band[i]))
        {
          LOG_MSG_ERROR("Invalid band passed for MLD-STA Link:%d band:%d,",i+1,wlan_config.mld_sta_config.band[i],0);
          *qmi_err_num = QMI_INTERNAL_ERR;
          return false;
        }
      }
  }

  if(wlan_config.ap_config_len > 0 && wlan_config.is_sta_configured)
  {
    wlan_config.wlan_mode = QCMAP_MSGR_WLAN_MODE_AP_STA_V01;
        }
  else if(wlan_config.ap_config_len > 0)
  {
    wlan_config.wlan_mode = QCMAP_MSGR_WLAN_MODE_AP_V01;
  }
  else if(wlan_config.is_sta_configured)
  {
    wlan_config.wlan_mode = QCMAP_MSGR_WLAN_MODE_STA_ONLY_V01;
  }

  if(wlan_config.mld_ap_config_len > 0 && wlan_config.is_mld_sta_configured)
  {
    wlan_config.mld_wlan_mode = QCMAP_MSGR_MLD_WLAN_MODE_AP_STA;
  }
  else if(wlan_config.mld_ap_config_len > 0)
  {
    wlan_config.mld_wlan_mode = QCMAP_MSGR_MLD_WLAN_MODE_AP;
  }
  else if(wlan_config.is_mld_sta_configured)
  {
    wlan_config.mld_wlan_mode = QCMAP_MSGR_MLD_WLAN_MODE_STA;
  }

  /* Set WLAN mode */
  QCMAP_LAN_CLIENT_RUN_COMMANDS("%s %s %d", mWLANConfigFile, CMD_WLAN_SET_MODE,
                                            wlan_config.wlan_mode);

  /* Set MLD WLAN mode */
  QCMAP_LAN_CLIENT_RUN_COMMANDS("%s %s %d", mWLANConfigFile, CMD_MLD_WLAN_SET_MODE,
                                            wlan_config.mld_wlan_mode);

  /*Set AP config */
  if(wlan_config.ap_config_len > 0)
  {
    for(int ap_count=0; ap_count<wlan_config.ap_config_len; ap_count++)
    {
      QCMAP_LAN_CLIENT_RUN_COMMANDS("%s %s %d %d %d",
                            mWLANConfigFile,
                            CMD_SET_AP_CONFIG,
                            ap_count,
                            wlan_config.ap_config[ap_count].band,
                            wlan_config.ap_config[ap_count].accessprofile);
    }
  }

  /*Set MLD AP config */
  if(wlan_config.mld_ap_config_len > 0)
  {
    for(int mld_ap_count=0; mld_ap_count<wlan_config.mld_ap_config_len; mld_ap_count++)
    {
      QCMAP_LAN_CLIENT_RUN_COMMANDS("%s %s %d %d %d",
                            mWLANConfigFile,
                            CMD_SET_MLD_AP_CONFIG,
                            mld_ap_count,
                            wlan_config.mld_ap_config[mld_ap_count].no_of_mld_link,
                            wlan_config.mld_ap_config[mld_ap_count].accessprofile);
      /*Update MLD link band */
      for(int j=0; j<wlan_config.mld_ap_config[mld_ap_count].no_of_mld_link; j++)
      {
        QCMAP_LAN_CLIENT_RUN_COMMANDS("/etc/data/uci_ex.sh set qcmap_wlan_current.@mldapconfig[%d].link_band%d=%d",
                                               mld_ap_count,j,
                                               wlan_config.mld_ap_config[mld_ap_count].band[j]);
      }
    }
  }

  if(wlan_config.is_sta_configured)
  {
    /* don't need care return value, in case addr not set, the string will be empty */
    readable_addr(AF_INET, &wlan_config.station_config.static_ip_config.ip_addr,  strIPv4Addr);
    readable_addr(AF_INET, &wlan_config.station_config.static_ip_config.gw_ip,    strIPv4GwIP);
    readable_addr(AF_INET, &wlan_config.station_config.static_ip_config.netmask,  strIPv4Netmask);
    readable_addr(AF_INET, &wlan_config.station_config.static_ip_config.dns_addr, strIPv4DNS);

    QCMAP_LAN_CLIENT_RUN_COMMANDS("%s %s %d %d %d %s %s %s %s",
                              mWLANConfigFile,
                              CMD_SET_STA_CONFIG,
                              wlan_config.station_config.band,
                              wlan_config.station_config.ap_sta_bridge_mode,
                              wlan_config.station_config.conn_type,
                              strIPv4Addr, strIPv4GwIP, strIPv4Netmask, strIPv4DNS);
  }

  if(wlan_config.is_mld_sta_configured)
  {
    /* don't need care return value, in case addr not set, the string will be empty */
    readable_addr(AF_INET, &wlan_config.mld_sta_config.static_ip_config.ip_addr,  strIPv4Addr);
    readable_addr(AF_INET, &wlan_config.mld_sta_config.static_ip_config.gw_ip,    strIPv4GwIP);
    readable_addr(AF_INET, &wlan_config.mld_sta_config.static_ip_config.netmask,  strIPv4Netmask);
    readable_addr(AF_INET, &wlan_config.mld_sta_config.static_ip_config.dns_addr, strIPv4DNS);

    QCMAP_LAN_CLIENT_RUN_COMMANDS("%s %s %d %d %d %s %s %s %s",
                              mWLANConfigFile,
                              CMD_SET_MLD_STA_CONFIG,
                              wlan_config.mld_sta_config.no_of_mld_link,
                              wlan_config.mld_sta_config.ap_sta_bridge_mode,
                              wlan_config.mld_sta_config.conn_type,
                              strIPv4Addr, strIPv4GwIP, strIPv4Netmask, strIPv4DNS);
   /*Update MLD link band */
    for(int j=0; j<wlan_config.mld_sta_config.no_of_mld_link; j++)
    {
      QCMAP_LAN_CLIENT_RUN_COMMANDS("/etc/data/uci_ex.sh set qcmap_wlan_current.@mldstaconfig[0].link_band%d=%d",
                                             j,
                                             wlan_config.mld_sta_config.band[j]);
    }
  }

  // Update AP + MLD AP count
  QCMAP_LAN_CLIENT_RUN_COMMANDS("%s %s %d %d %d %d %d",
                            mWLANConfigFile,
                            CMD_SET_MLD_AP_CONFIG_LEN,
                            wlan_config.ap_count_2g,
                            wlan_config.ap_count_5g,
                            wlan_config.ap_count_6g,
                            wlan_config.ap_config_len,
                            wlan_config.mld_ap_config_len);

  /* update the network/wireless configuration files */
  QCMAP_LAN_CLIENT_RUN_COMMANDS("%s %s", mWLANConfigFile, CMD_UPDATE_WIRELESS_NETWORK_EX);
  *qmi_err_num = QMI_ERR_NONE_V01;
  return true;
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
boolean QCMAP_WLAN_Common::GetWLANConfigEx
(
  qcmap_wlan_ex_config *wlan_config,
  qmi_error_type_v01 *qmi_err_num
)
{
  char result[QCMAP_MAX_SCAN_SIZE] = {0};
  in_addr addr;

  if(wlan_config == NULL)
  {
    LOG_MSG_ERROR("NULL pointer passed ",0,0,0);
    *qmi_err_num = QMI_ERR_INVALID_ARG_V01;
    return false;
  }

  UCI_CUR_WLAN_GET_INT_OPTION(wlan_config->wlan_mode,         "wlanconfig",  "mode")
  UCI_CUR_WLAN_GET_INT_OPTION(wlan_config->primary_ap_band,   "primaryap",   "band")
  UCI_CUR_WLAN_GET_INT_OPTION(wlan_config->ap_config[0].band, "guestap",     "band")
  UCI_CUR_WLAN_GET_INT_OPTION(wlan_config->ap_config[1].band, "guestaptwo",  "band")
  UCI_CUR_WLAN_GET_INT_OPTION(wlan_config->ap_config[2].band, "guestapthree","band")
  UCI_CUR_WLAN_GET_INT_OPTION(wlan_config->ap_config[0].guest_ap_profile,
                                       "guestap",     "accessprofile")
  UCI_CUR_WLAN_GET_INT_OPTION(wlan_config->ap_config[1].guest_ap_profile,
                                       "guestaptwo",  "accessprofile")
  UCI_CUR_WLAN_GET_INT_OPTION(wlan_config->ap_config[2].guest_ap_profile,
                                       "guestapthree","accessprofile")

  UCI_CUR_WLAN_GET_INT_OPTION(wlan_config->station_band,
                                       "stamodeconfig", "band")
  UCI_CUR_WLAN_GET_INT_OPTION(wlan_config->station_config.ap_sta_bridge_mode,
                                       "stamodeconfig", "bridge_mode")
  UCI_CUR_WLAN_GET_INT_OPTION(wlan_config->station_config.conn_type ,
                                       "stamodeconfig", "sta_mode_conn_type")
  UCI_CUR_WLAN_GET_ADDR_OPTION(wlan_config->station_config.static_ip_config.ip_addr,
                                       "stamodeconfig", "ipaddr")
  UCI_CUR_WLAN_GET_ADDR_OPTION(wlan_config->station_config.static_ip_config.gw_ip,
                                       "stamodeconfig", "gateway")
  UCI_CUR_WLAN_GET_ADDR_OPTION(wlan_config->station_config.static_ip_config.netmask,
                                       "stamodeconfig", "netmask")
  UCI_CUR_WLAN_GET_ADDR_OPTION(wlan_config->station_config.static_ip_config.dns_addr,
                                       "stamodeconfig", "dnsserver")

  wlan_config->guest_profile.guest_ap_profile[0] = wlan_config->ap_config[0].guest_ap_profile;
  wlan_config->guest_profile.guest_ap_profile[1] = wlan_config->ap_config[1].guest_ap_profile;
  wlan_config->guest_profile.guest_ap_profile[2] = wlan_config->ap_config[2].guest_ap_profile;

  *qmi_err_num = QMI_ERR_NONE_V01;
  return true;
}

/*=====================================================================
  FUNCTION GetActiveWLANConfigEx
======================================================================*/
/*!
@brief
  - GetActiveWLANConfigEx WLAN from qcmap client

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
boolean QCMAP_WLAN_Common::GetActiveWLANConfigEx
(
  qcmap_wlan_ex_config *wlan_config,
  qmi_error_type_v01 *qmi_err_num
)
{
  char result[QCMAP_MAX_SCAN_SIZE] = {0};
  in_addr addr;

  if(wlan_config == NULL)
  {
    LOG_MSG_ERROR("NULL pointer passed ",0,0,0);
    *qmi_err_num = QMI_ERR_INVALID_ARG_V01;
    return false;
  }

  UCI_WLAN_GET_INT_OPTION(wlan_config->wlan_mode,         "wlanconfig",  "mode")
  UCI_WLAN_GET_INT_OPTION(wlan_config->primary_ap_band,   "primaryap",   "band")

  UCI_WLAN_GET_INT_OPTION(wlan_config->ap_config[0].band, "guestap",     "band")
  UCI_WLAN_GET_INT_OPTION(wlan_config->ap_config[1].band, "guestaptwo",  "band")
  UCI_WLAN_GET_INT_OPTION(wlan_config->ap_config[2].band, "guestapthree","band")
  UCI_WLAN_GET_INT_OPTION(wlan_config->ap_config[0].guest_ap_profile,
                                       "guestap",     "accessprofile")
  UCI_WLAN_GET_INT_OPTION(wlan_config->ap_config[1].guest_ap_profile,
                                       "guestaptwo",  "accessprofile")
  UCI_WLAN_GET_INT_OPTION(wlan_config->ap_config[2].guest_ap_profile,
                                       "guestapthree","accessprofile")

  UCI_WLAN_GET_INT_OPTION(wlan_config->station_band,
                                       "stamodeconfig", "band")
  UCI_WLAN_GET_INT_OPTION(wlan_config->station_config.ap_sta_bridge_mode,
                                       "stamodeconfig", "bridge_mode")
  UCI_WLAN_GET_INT_OPTION(wlan_config->station_config.conn_type ,
                                       "stamodeconfig", "sta_mode_conn_type")
  UCI_WLAN_GET_ADDR_OPTION(wlan_config->station_config.static_ip_config.ip_addr,
                                       "stamodeconfig", "ipaddr")
  UCI_WLAN_GET_ADDR_OPTION(wlan_config->station_config.static_ip_config.gw_ip,
                                       "stamodeconfig", "gateway")
  UCI_WLAN_GET_ADDR_OPTION(wlan_config->station_config.static_ip_config.netmask,
                                       "stamodeconfig", "netmask")
  UCI_WLAN_GET_ADDR_OPTION(wlan_config->station_config.static_ip_config.dns_addr,
                                       "stamodeconfig", "dnsserver")

  wlan_config->guest_profile.guest_ap_profile[0] = wlan_config->ap_config[0].guest_ap_profile;
  wlan_config->guest_profile.guest_ap_profile[1] = wlan_config->ap_config[1].guest_ap_profile;
  wlan_config->guest_profile.guest_ap_profile[2] = wlan_config->ap_config[2].guest_ap_profile;

  *qmi_err_num = QMI_ERR_NONE_V01;
  return true;
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
  qcmap_msgr_wlan_mode_enum_v01 wlan_mode

@note
  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
 bool QCMAP_WLAN_Common::GetWLANStatus
(
  qcmap_msgr_wlan_mode_enum_v01 *wlan_mode,
  qmi_error_type_v01 *qmi_err_num
)
{
  char result[QCMAP_MAX_SCAN_SIZE] = {0};

  UCI_WLAN_GET_INT_OPTION((*wlan_mode), "wlanconfig", "mode");
  *qmi_err_num = QMI_ERR_NONE_V01;
  return true;
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
  qcmap_msgr_station_mode_status_enum_v01& sta_status

@note
  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
bool QCMAP_WLAN_Common::GetStationModeStatus
(
  qcmap_msgr_station_mode_status_enum_v01 *sta_status,
  qmi_error_type_v01 *qmi_err_num
)
{
  char result[QCMAP_MAX_SCAN_SIZE] = {0};

  QCMAP_LAN_CLIENT_RUN_COMMANDS("%s %s", mWLANConfigFile, CMD_GET_STA_STATUS);

  UCI_WLAN_GET_INT_OPTION((*sta_status), "stamodeconfig",  "status")
  *qmi_err_num = QMI_ERR_NONE_V01;
  return true;
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
    |        |         |             |           |
    | IF Name| AP type |  Card Type  |   State   |
    |        |         |             |           |
    +---------+---------+-------------+-----------+
    |    ath0|  Primary|       WAKIKI|    Enabled|
    +---------+---------+-------------+-----------+
    |   ath01|    Guest|       WAKIKI|    Enabled|
    +---------+---------+-------------+-----------+

 */
/*=========================================================================*/
boolean QCMAP_WLAN_Common::GetActiveWlanIfInfo
(
  qcmap_msgr_wlan_if_info_t *wlan_info_cfg,
  qmi_error_type_v01 *qmi_err_num
)
{
  char result[QCMAP_MAX_SCAN_SIZE] = {0};
  char cmd[QCMAP_MAX_COMMAND_LEN] = {0};
  char split[]=" ";
  char wlan_device_info[QCMAP_MAX_SCAN_SIZE] = {0};
  int wlan_mode = 0, mld_config = 0;
  int i = 0;
  qcmap_msgr_wlan_iface_active_state_enum_v01 primary_ap_enabled = 0;
  qcmap_msgr_wlan_iface_active_state_enum_v01 guest_ap_enabled = 0;
  qcmap_msgr_wlan_iface_active_state_enum_v01 guest_ap2_enabled = 0;
  qcmap_msgr_wlan_iface_active_state_enum_v01 guest_ap3_enabled = 0;
  qcmap_msgr_wlan_iface_active_state_enum_v01 sta_enabled = 0;
  int ap_count=0, mld_ap_count=0;
  char *ptr = NULL;
  char *p = NULL;

  if (!wlan_info_cfg)
  {
    LOG_MSG_ERROR("Invalid parameter passed ",0,0,0);
    *qmi_err_num = QMI_ERR_INVALID_ARG_V01;
    return false;
  }

  UCI_WLAN_GET_INT_OPTION(mld_config, "wlanconfig",  "wlanconfigex");
  if(mld_config == 0)
  {
  snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "%s %s",
                            mWLANConfigFile, CMD_GET_ACTIVE_WLAN_INFO);
  if (!ExecuteSystemCmd((const char *)cmd, result, sizeof(result)))
  {
    LOG_MSG_ERROR("Failed to get active wlan device info", 0, 0, 0);
    *qmi_err_num = QMI_ERR_INTERNAL_V01;
    return false;
  }

  ptr = strtok_r(result, split, &p);
  while (ptr != NULL)
  {
    wlan_device_info[i++]=*ptr;
    ptr = strtok_r(NULL, split, &p);
  }

  #define GET_IFACE_STAE(a) (a == 1) ? QCMAP_MSGR_WLAN_IFACE_ACTIVE_V01 : QCMAP_MSGR_WLAN_IFACE_INACTIVE_V01;

  primary_ap_enabled = GET_IFACE_STAE(wlan_device_info[1]-'0');
  guest_ap_enabled   = GET_IFACE_STAE(wlan_device_info[2]-'0');
  guest_ap2_enabled  = GET_IFACE_STAE(wlan_device_info[3]-'0');
  guest_ap3_enabled  = GET_IFACE_STAE(wlan_device_info[4]-'0');
  sta_enabled        = GET_IFACE_STAE(wlan_device_info[5]-'0');

  UCI_WLAN_GET_INT_OPTION(wlan_mode, "wlanconfig",  "mode");
  if (wlan_mode == QCMAP_MSGR_WLAN_MODE_7AP_V01)
  {
      UCI_WLAN_GET_INT_OPTION(ap_count, "wlanconfig",  "totalapcount");
      wlan_info_cfg->wlan_if_info_len = ap_count; // Primary AP + total Geust AP.
      p=NULL;
      int idx=0;
      memset(cmd, 0, QCMAP_MAX_COMMAND_LEN);
      memset(result, 0, QCMAP_MAX_SCAN_SIZE);
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
        wlan_info_cfg->wlan_if_info[i+1].wlan_ap_type=QCMAP_MSGR_WLAN_IFACE_GUEST_AP_ONE_V01 ;
      }
  }
  else
  {
    switch(wlan_mode)
    {
      case QCMAP_MSGR_WLAN_MODE_STA_ONLY_V01:
      case QCMAP_MSGR_WLAN_MODE_AP_V01:
        wlan_info_cfg->wlan_if_info_len = 1;
        break;
      case QCMAP_MSGR_WLAN_MODE_AP_AP_V01:
      case QCMAP_MSGR_WLAN_MODE_AP_STA_V01:
        wlan_info_cfg->wlan_if_info_len = 2;
        break;
      case QCMAP_MSGR_WLAN_MODE_AP_AP_AP_V01:
      case QCMAP_MSGR_WLAN_MODE_AP_AP_STA_V01:
        wlan_info_cfg->wlan_if_info_len = 3;
        break;
      case QCMAP_MSGR_WLAN_MODE_AP_AP_AP_AP_V01:
        wlan_info_cfg->wlan_if_info_len = 4;
        break;
      default:
        LOG_MSG_ERROR("Unknown WLAN mode %d", wlan_mode, 0, 0);
        *qmi_err_num = QMI_ERR_INVALID_ARG_V01;
        return false;
    }

    LOG_MSG_INFO1("GetActiveWlanIfInfo: wlan_if_info_len = %d", wlan_info_cfg->wlan_if_info_len, 0, 0);

    if ((wlan_info_cfg->wlan_if_info_len > QCMAP_MSGR_MAX_WLAN_IFACE_V01) ||
        (wlan_info_cfg->wlan_if_info_len == 0))
    {
      LOG_MSG_INFO1("Invalid entries %d", wlan_info_cfg->wlan_if_info_len,0,0);
      *qmi_err_num = QMI_ERR_INVALID_ARG_V01;
      return false;
    }


    FillSpecificWlanIfInfo(wlan_info_cfg, wlan_mode);

    if (wlan_mode == QCMAP_MSGR_WLAN_MODE_AP_V01)
    {
      wlan_info_cfg->wlan_if_info[0].wlan_ap_type = QCMAP_MSGR_WLAN_IFACE_PRIMARY_AP_V01;
      wlan_info_cfg->wlan_if_info[0].state = primary_ap_enabled;
    }
    else if (wlan_mode == QCMAP_MSGR_WLAN_MODE_AP_AP_V01)
    {
      wlan_info_cfg->wlan_if_info[0].wlan_ap_type = QCMAP_MSGR_WLAN_IFACE_PRIMARY_AP_V01;
      wlan_info_cfg->wlan_if_info[1].wlan_ap_type = QCMAP_MSGR_WLAN_IFACE_GUEST_AP_ONE_V01;

      wlan_info_cfg->wlan_if_info[0].state = primary_ap_enabled;
      wlan_info_cfg->wlan_if_info[1].state = guest_ap_enabled;
    }
    else if(wlan_mode == QCMAP_MSGR_WLAN_MODE_AP_STA_V01)
    {
      wlan_info_cfg->wlan_if_info[0].wlan_ap_type = QCMAP_MSGR_WLAN_IFACE_PRIMARY_AP_V01;
      wlan_info_cfg->wlan_if_info[1].wlan_ap_type = QCMAP_MSGR_WLAN_IFACE_STATION_V01;

      wlan_info_cfg->wlan_if_info[0].state = primary_ap_enabled;
      wlan_info_cfg->wlan_if_info[1].state = sta_enabled;
    }
    else if(wlan_mode == QCMAP_MSGR_WLAN_MODE_AP_AP_AP_V01)
    {
      wlan_info_cfg->wlan_if_info[0].wlan_ap_type = QCMAP_MSGR_WLAN_IFACE_PRIMARY_AP_V01;
      wlan_info_cfg->wlan_if_info[1].wlan_ap_type = QCMAP_MSGR_WLAN_IFACE_GUEST_AP_ONE_V01;
      wlan_info_cfg->wlan_if_info[2].wlan_ap_type = QCMAP_MSGR_WLAN_IFACE_GUEST_AP_ONE_V01;

      wlan_info_cfg->wlan_if_info[0].state = primary_ap_enabled;
      wlan_info_cfg->wlan_if_info[1].state = guest_ap_enabled;
      wlan_info_cfg->wlan_if_info[2].state = guest_ap2_enabled;
    }
    else if(wlan_mode == QCMAP_MSGR_WLAN_MODE_AP_AP_STA_V01)
    {
      wlan_info_cfg->wlan_if_info[0].wlan_ap_type = QCMAP_MSGR_WLAN_IFACE_PRIMARY_AP_V01;
      wlan_info_cfg->wlan_if_info[1].wlan_ap_type = QCMAP_MSGR_WLAN_IFACE_GUEST_AP_ONE_V01;
      wlan_info_cfg->wlan_if_info[2].wlan_ap_type = QCMAP_MSGR_WLAN_IFACE_STATION_V01;

      wlan_info_cfg->wlan_if_info[0].state = primary_ap_enabled;
      wlan_info_cfg->wlan_if_info[1].state = guest_ap_enabled;
      wlan_info_cfg->wlan_if_info[2].state = sta_enabled;
    }
    else if (wlan_mode == QCMAP_MSGR_WLAN_MODE_STA_ONLY_V01)
    {
      wlan_info_cfg->wlan_if_info[0].wlan_ap_type = QCMAP_MSGR_WLAN_IFACE_STATION_V01;
      wlan_info_cfg->wlan_if_info[0].state = sta_enabled;
    }
    else if (wlan_mode == QCMAP_MSGR_WLAN_MODE_AP_AP_AP_AP_V01)
    {
      wlan_info_cfg->wlan_if_info[0].wlan_ap_type = QCMAP_MSGR_WLAN_IFACE_PRIMARY_AP_V01;
      wlan_info_cfg->wlan_if_info[1].wlan_ap_type = QCMAP_MSGR_WLAN_IFACE_GUEST_AP_ONE_V01;
      wlan_info_cfg->wlan_if_info[2].wlan_ap_type = QCMAP_MSGR_WLAN_IFACE_GUEST_AP_ONE_V01;
      wlan_info_cfg->wlan_if_info[3].wlan_ap_type = QCMAP_MSGR_WLAN_IFACE_GUEST_AP_ONE_V01;

      wlan_info_cfg->wlan_if_info[0].state = primary_ap_enabled;
      wlan_info_cfg->wlan_if_info[1].state = guest_ap_enabled;
      wlan_info_cfg->wlan_if_info[2].state = guest_ap2_enabled;
      wlan_info_cfg->wlan_if_info[3].state = guest_ap3_enabled;
    }
  }
  }
  else
  {
    //Get WLAN IF info for MLD config.
    UCI_WLAN_GET_INT_OPTION(ap_count, "wlanconfig",  "totalapcount");
    UCI_WLAN_GET_INT_OPTION(mld_ap_count, "mldwlanconfig",  "totalmldapcount");
    wlan_info_cfg->wlan_if_info_len = (ap_count + mld_ap_count); // MLD + NON-MLD AP
    p=NULL;
    int idx=0;
    memset(cmd, 0, QCMAP_MAX_COMMAND_LEN);
    memset(result, 0, QCMAP_MAX_SCAN_SIZE);
    snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "%s %s",
                              mWLANConfigFile, CMD_GET_ACTIVE_MLD_WLAN_INFO);
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

    for(i=0; i < ap_count; i++)
    {
      UCI_WLAN_GET_STR_OPTION_WITH_IDX(wlan_info_cfg->wlan_if_info[i].if_name, "apconfig", "ifname", i)
      wlan_info_cfg->wlan_if_info[i].state=wlan_device_info[i]-'0';
      wlan_info_cfg->wlan_if_info[i].wlan_ap_type = QCMAP_MSGR_WLAN_IFACE_GUEST_AP_ONE_V01;
      wlan_info_cfg->wlan_if_info[i].wlan_dev_type = QCMAP_MSGR_WLAN_DEV_WKK_V01;
    }

    for(int j=0; j<mld_ap_count; j++, i++)
    {
      UCI_WLAN_GET_STR_OPTION_WITH_IDX(wlan_info_cfg->wlan_if_info[i].if_name, "mldapconfig", "ifname", j)
      wlan_info_cfg->wlan_if_info[i].state=wlan_device_info[i]-'0';
      wlan_info_cfg->wlan_if_info[i].wlan_ap_type = QCMAP_MSGR_WLAN_IFACE_MLD_AP;
      wlan_info_cfg->wlan_if_info[i].wlan_dev_type = QCMAP_MSGR_WLAN_DEV_WKK_V01;
    }
  }
  *qmi_err_num = QMI_ERR_NONE_V01;
  return true;
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
boolean QCMAP_WLAN_Common::ActivateHostapdConfig
(
  qcmap_msgr_activate_hostapd_ap_enum_v01 ap_type,
  qcmap_msgr_activate_hostapd_action_enum_v01 action_type,
  qmi_error_type_v01 *qmi_err_num
)
{
  char cmd[QCMAP_MAX_COMMAND_LEN] = {0};
  char result[QCMAP_MAX_SCAN_SIZE] = {0};

  snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "%s %s %d %d",
                            mWLANConfigFile,
                            CMD_ACTIVATE_HOSTAPD_CONFIG,
                            ap_type,
                            action_type);

  if (ExecuteSystemCmd((const char *)cmd, result, sizeof(result)))
  {
    if (!strstr(result, RESULT_ACTIVATE_HOSTAPD_SUCCESS))
    {
      LOG_MSG_ERROR("Failed to activate hostapd", 0, 0, 0);
      *qmi_err_num = QMI_ERR_INTERNAL_V01;
      return false;
    }
  }
  else
  {
    *qmi_err_num = QMI_ERR_INTERNAL_V01;
    return false;
  }
  *qmi_err_num = QMI_ERR_NONE_V01;
  return true;
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
boolean QCMAP_WLAN_Common::ActivateSupplicantConfig
(
  qmi_error_type_v01 *qmi_err_num
)
{
  char cmd[QCMAP_MAX_COMMAND_LEN] = {0};
  char result[QCMAP_MAX_SCAN_SIZE] = {0};

  snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "%s %s",
                            mWLANConfigFile,
                            CMD_ACTIVATE_SUPPLICANT_CONFIG);

  if (ExecuteSystemCmd((const char *)cmd, result, sizeof(result)))
  {
    if (!strstr(result, RESULT_ACTIVATE_SUPPLICANT_SUCCESS))
    {
      LOG_MSG_ERROR("Failed to activate supplicant", 0, 0, 0);
      *qmi_err_num = QMI_ERR_INTERNAL_V01;
      return false;
    }
  }
  else
  {
    *qmi_err_num = QMI_ERR_INTERNAL_V01;
    return false;
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
boolean QCMAP_WLAN_Common:: DisAssociateClient(const char* mac_addr_str)
{
  char cmd[QCMAP_MAX_COMMAND_LEN] = {0};
  char result[QCMAP_MAX_SCAN_SIZE] = {0};

  if (NULL !=mac_addr_str)
  {
    snprintf(cmd, QCMAP_MAX_COMMAND_LEN,"bridge fdb show | grep %s | awk '{print $3}' | tr -d ' \n' ", mac_addr_str);
    if (ExecuteSystemCmd((const char *)cmd, result, sizeof(result)))
    {
        return DisAssociateClient_Device(result, mac_addr_str);
    }
    else
    {
      LOG_MSG_ERROR("Not found neighbor with the mac-addr !", 0, 0, 0);
    }
  }
  return false;
}

/*=====================================================================
  FUNCTION RestartTetheredWLANClient
======================================================================*/
/*!
@brief
  - Restart tethered WLAN Client

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
boolean QCMAP_WLAN_Common::RestartTetheredWLANClient()
{
  //restart hostapd and start hostapd_cli process
  QCMAP_LAN_CLIENT_RUN_COMMANDS("%s %s", mWLANConfigFile, CMD_RESTART_TETHERED_WLAN_CLIENT);
  return true;
}

boolean QCMAP_WLAN_Common::ProcessStaIndicate
(
  qcmap_msgr_sta_connect_status_enum_v01 sta_connection_state
)
{
  if (sta_connection_state == QCMAP_MSGR_EVENT_STA_CONNECTED_V01)
  {
    QCMAP_LAN_CLIENT_RUN_COMMANDS("%s %s %d", mWLANConfigFile, CMD_AP_STA_MODE_CONNECT, 1);
  }
  else
  {
    QCMAP_LAN_CLIENT_RUN_COMMANDS("%s %s %d", mWLANConfigFile, CMD_AP_STA_MODE_DISCONNECT, 1);
  }

  return true;
}

boolean QCMAP_WLAN_Common::IsWiFiDevcies(  const char *phy_iface)
{
  if ((strncmp(phy_iface, HMT_WLAN_DEVICE, strlen(HMT_WLAN_DEVICE)) == 0) ||
       (strncmp(phy_iface, WKK_WLAN_DEVICE, strlen(WKK_WLAN_DEVICE)) == 0))
       return true;

  return false;
}

boolean QCMAP_WLAN_Common::IsStationStatic()
{
  char result[QCMAP_MAX_SCAN_SIZE] = {0};

  int conn_type = QCMAP_STA_CONNECTION_DYNAMIC;
  UCI_WLAN_GET_INT_OPTION(conn_type, "stamodeconfig", "sta_mode_conn_type");
  return conn_type == QCMAP_STA_CONNECTION_STATIC;
}

boolean QCMAP_WLAN_Common::IsBridgeMode()
{
  char result[QCMAP_MAX_SCAN_SIZE] = {0};

  int bridge_mode = 0;
  UCI_WLAN_GET_INT_OPTION(bridge_mode, "stamodeconfig", "bridge_mode");
  return bridge_mode == 1;
}


boolean QCMAP_WLAN_Common::SwitchWlanEnableBand
(
  qcmap_msgr_sap_band_status_enum_v01 sap_5g_enable_state,
  qcmap_msgr_sta_band_status_enum_v01 sta_5g_enable_state
)
{
  QCMAP_LAN_CLIENT_RUN_COMMANDS("%s %s %d %d",
                                 mWLANConfigFile, CMD_SWITCH_WLAN_ENABLE_BAND,
                                 sap_5g_enable_state, sta_5g_enable_state);
  return true;
}


boolean QCMAP_WLAN_Common::InstallGuestAPRules()
{
  char cmd[QCMAP_MAX_COMMAND_LEN] = {0};
  char result[QCMAP_MAX_SCAN_SIZE] = {0};

  //Get SAP status before Reinstall Guest AP acess rules.
  snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "%s %s", WLAN_CONFIG_FILE, CMD_GET_SAP_STATUS);
  if (ExecuteSystemCmd((const char *)cmd, result, sizeof(result)))
  {
    if (strstr(result, RESULT_SAP_STATE_ENABLED) ||
        strstr(result, RESULT_SAP_STATE_DFS) ||
        strstr(result, RESULT_STA_ONLY_MODE))
    {
      QCMAP_LAN_CLIENT_RUN_COMMANDS("%s %s", WLAN_CONFIG_FILE, CMD_INSTALL_GUEST_AP_ACCESS_RULES);
      LOG_MSG_INFO1("Reinstall Guest AP acess rules success", 0, 0, 0);
      return true;
    }
    else
    {
      LOG_MSG_INFO1("Wlan is disabled ", 0, 0, 0);
      return false;
    }
  }
  else
  {
   LOG_MSG_INFO1("Wlan status check failed", 0, 0, 0);
   return false;
  }
}
boolean QCMAP_WLAN_Common::UnInstallGuestAPRules()
{
  char cmd[QCMAP_MAX_COMMAND_LEN] = {0};
  char result[QCMAP_MAX_SCAN_SIZE] = {0};

  //Get SAP status before Reinstall Guest AP acess rules.
  snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "%s %s", WLAN_CONFIG_FILE, CMD_GET_SAP_STATUS);
  if (ExecuteSystemCmd((const char *)cmd, result, sizeof(result)))
  {
    if (strstr(result, RESULT_SAP_STATE_ENABLED) ||
        strstr(result, RESULT_SAP_STATE_DFS) ||
        strstr(result, RESULT_STA_ONLY_MODE))
    {
      QCMAP_LAN_CLIENT_RUN_COMMANDS("%s %s", WLAN_CONFIG_FILE, CMD_DELETE_GUEST_AP_ACCESS_RULES);
      LOG_MSG_INFO1("UnInstall Guest AP acess rules success", 0, 0, 0);
      return true;
    }
    else
    {
      LOG_MSG_INFO1("Wlan is disabled ", 0, 0, 0);
      return false;
    }
  }
  else
  {
   LOG_MSG_INFO1("Wlan status check failed", 0, 0, 0);
   return false;
  }
}



