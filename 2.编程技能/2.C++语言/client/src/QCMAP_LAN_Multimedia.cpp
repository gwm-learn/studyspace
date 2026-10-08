/*====================================================

FILE:  QCMAP_LAN_Multimedia.cpp

SERVICES:
QCMAP LAN Multimedia Implementation

=====================================================

  Copyright (c) 2023 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Confidential and Proprietary - Qualcomm Technologies, Inc.

=====================================================*/
/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  when       who        what, where, why
  --------   ---        -------------------------------------------------------
  06/06/23   yunhcao    Added MINIUPNPD API Support in Openwrt
  06/10/23   yunhcao    Added MNDS API Support in Openwrt
  ===========================================================================*/
#include "QCMAP_LAN_Multimedia.h"
#include "QCMAP_LAN_Client.h"

#define QCMAP_MAX_COMMAND_LEN                100   /* Max Command length */
#define MAX_SCAN_SIZE 100
#define MIN_NOTIFY_INTERVAL 30
#define MAX_NOTIFY_INTERVAL 60000


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


/*===================================================================
  Class Definitions
  ===================================================================*/
/*===========================================================================
  FUNCTION QCMAP_LAN_Multimedia
  ===========================================================================
  @brief
  Initializes the LAN Multimedia.
  @input
  void
  @return
  void
  @dependencies
  @sideefects
  None
  =========================================================================*/
QCMAP_LAN_Multimedia::QCMAP_LAN_Multimedia()
{
  QCMAP_LOG_FUNC_ENTRY();
  return;
}  /* end QCMAP_LAN_Multimedia() */

/*===========================================================================
  FUNCTION EnableUPNP
  ===========================================================================*/
/*!
  @brief
  Starts the UPNP daemon

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
boolean QCMAP_LAN_Multimedia::EnableUPNP(qmi_error_type_v01 *qmi_err_num)
{
  qcmap_msgr_upnp_mode_enum_v01 enable_UPNP;
  LOG_MSG_INFO1("Check the current status first!", 0, 0, 0);
  if(!GetUPNPStatus(&enable_UPNP, qmi_err_num))
  {
    LOG_MSG_INFO1("Current status checking fail", 0, 0, 0);
    *qmi_err_num = QMI_ERR_INTERNAL_V01;
    return false;
  }
  if(enable_UPNP == QCMAP_MSGR_UPNP_MODE_UP_V01)
  {
    *qmi_err_num = QMI_ERR_NO_EFFECT_V01;
    LOG_MSG_ERROR("UPnP Aready Enabled", 0, 0, 0);
    return false;
  }
  else
  {
    QCMAP_LAN_CLIENT_RUN_COMMANDS("%s %s", MINIUPNPD_CONFIG_FILE, ENABLE_MINIPUNPD);
    LOG_MSG_INFO1("Double check the setting result ", 0, 0, 0);
    if(!GetUPNPStatus(&enable_UPNP, qmi_err_num))
    {
      LOG_MSG_INFO1("Double checking fail", 0, 0, 0);
      *qmi_err_num = QMI_ERR_INTERNAL_V01;
      return false;
    }
    if(enable_UPNP == QCMAP_MSGR_UPNP_MODE_UP_V01)
    {
      LOG_MSG_INFO1("Enable miniupnpd done", 0, 0, 0);
      return true;
    }
    else
    {
      LOG_MSG_INFO1("Enable miniupnpd fail", 0, 0, 0);
      *qmi_err_num = QMI_ERR_INTERNAL_V01;
      return false;
    }
  }
}


/*===========================================================================
  FUNCTION DisableUPNP
  ===========================================================================*/
/*!
  @brief
  Stops the UPNP daemon

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
boolean QCMAP_LAN_Multimedia::DisableUPNP(qmi_error_type_v01 *qmi_err_num)
{
  qcmap_msgr_upnp_mode_enum_v01 enable_UPNP;
  LOG_MSG_INFO1("Check the current status first!", 0, 0, 0);
  if(!GetUPNPStatus(&enable_UPNP, qmi_err_num))
  {
    LOG_MSG_INFO1("Current status checking fail", 0, 0, 0);
    *qmi_err_num = QMI_ERR_INTERNAL_V01;
    return false;
  }
  if(enable_UPNP == QCMAP_MSGR_UPNP_MODE_DOWN_V01)
  {
    *qmi_err_num = QMI_ERR_NO_EFFECT_V01;
    LOG_MSG_ERROR("UPnP Aready Disabled", 0, 0, 0);
    return false;
  }
  else
  {
    QCMAP_LAN_CLIENT_RUN_COMMANDS("%s %s", MINIUPNPD_CONFIG_FILE, DISABLE_MINIPUNPD);
    LOG_MSG_INFO1("Double check the setting result ", 0, 0, 0);
    if(!GetUPNPStatus(&enable_UPNP, qmi_err_num))
    {
      LOG_MSG_INFO1("Double checking fail", 0, 0, 0);
      return false;
    }
    if(enable_UPNP == QCMAP_MSGR_UPNP_MODE_DOWN_V01)
    {
      LOG_MSG_INFO1("Disable miniupnpd done", 0, 0, 0);
      return true;
    }
    else
    {
      LOG_MSG_INFO1("Disable miniupnpd fail", 0, 0, 0);
      *qmi_err_num = QMI_ERR_INTERNAL_V01;
      return false;
    }
  }
}


/*===========================================================================
  FUNCTION GetUPNPStatus
  ===========================================================================*/
/*!
  @brief
  Returns the status of UPNP

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
boolean QCMAP_LAN_Multimedia::GetUPNPStatus
(
  qcmap_msgr_upnp_mode_enum_v01 *enable_UPNP,
  qmi_error_type_v01 *qmi_err_num
)
{
  char result[QCMAP_MAX_SCAN_SIZE]={0};

  LOG_MSG_INFO1("Execute command uci get upnpd.config.enabled", 0, 0, 0);
  if(!ExecuteSystemCmd("/etc/data/uci_ex.sh get upnpd.config.enabled", result, sizeof(result)))
  {
    LOG_MSG_ERROR("Failed to execute command uci get upnpd.config.enabled", 0, 0, 0);
    *qmi_err_num = QMI_ERR_INTERNAL_V01;
    return false;
  }

  if(atoi(result) == 1)
    *enable_UPNP = QCMAP_MSGR_UPNP_MODE_UP_V01;
  else
    *enable_UPNP = QCMAP_MSGR_UPNP_MODE_DOWN_V01;
  LOG_MSG_INFO1("Get UPnP status succes, the UPnP status is %d ", *enable_UPNP, 0, 0);
  return true;
}

/*===========================================================================
  FUNCTION SetUPNPNotifyInterval
  ===========================================================================*/
/*!
  @brief
  Changes the UPnP notify interval

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
boolean QCMAP_LAN_Multimedia::SetUPNPNotifyInterval
(
  int upnp_notify_int,
  qmi_error_type_v01 *qmi_err_num
)
{
  int current_upnp_notify;
  if (upnp_notify_int < MIN_NOTIFY_INTERVAL || upnp_notify_int > MAX_NOTIFY_INTERVAL)
  {
    LOG_MSG_ERROR("Invalid Setting", 0, 0, 0);
    *qmi_err_num = QMI_ERR_INVALID_ARG_V01;
    return false;
  }
  if(!GetUPnPNotifyInterval(&current_upnp_notify, qmi_err_num))
  {
    LOG_MSG_INFO1("Check current upnp_notify value failed", 0, 0, 0);
    *qmi_err_num = QMI_ERR_INVALID_ARG_V01;
    return false;
  }
  if(current_upnp_notify == upnp_notify_int)
  {
    LOG_MSG_INFO1("Same upnp_notify value, ignore repeat setting", 0, 0, 0);
    *qmi_err_num = QMI_ERR_NO_EFFECT_V01;
    return false;
  }
  else
  {
  QCMAP_LAN_CLIENT_RUN_COMMANDS("%s %s %d", MINIUPNPD_CONFIG_FILE, SET_NOTIFY_INTERVAL, upnp_notify_int);
  LOG_MSG_INFO1("Set UPnP notify interval success, notify_interval is %d", upnp_notify_int, 0, 0);
  return true;
  }
}

/*===========================================================================
  FUNCTION GetUPnPNotifyInterval
  ===========================================================================*/
/*!
  @brief
  Returns the UPnP notify interval

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
boolean QCMAP_LAN_Multimedia::GetUPnPNotifyInterval
(
  int *upnp_notify_int,
  qmi_error_type_v01 *qmi_err_num
)
{
  char result[QCMAP_MAX_SCAN_SIZE] = {0};
  if(QCMAP_LAN_Client::UciGetUtility("upnpd", "config", true, "notify_interval", result, 0))
  {
    *upnp_notify_int = atoi(result);
    return true;
  }
  else
  {
    *qmi_err_num = QMI_ERR_INTERNAL_V01;
    return false;
  }
}

/*===========================================================================
  FUNCTION Enable M-DNS
  ===========================================================================*/
/*!
  @brief
  Starts the M-DNS daemon

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
boolean QCMAP_LAN_Multimedia::EnableMDNS
(
  qmi_error_type_v01 *qmi_err_num
)
{
  qcmap_msgr_mdns_mode_enum_v01 current_mdns_state;
  char command[QCMAP_MAX_COMMAND_LEN] = {0};

  LOG_MSG_INFO1("Check the current status first!", 0, 0, 0);
  if(!GetMDNSStatus(&current_mdns_state, qmi_err_num))
  {
    LOG_MSG_INFO1("Current status checking fail", 0, 0, 0);
    *qmi_err_num = QMI_ERR_INTERNAL_V01;
    return false;
  }
  if(current_mdns_state == QCMAP_MSGR_MDNS_MODE_UP_V01)
  {
    *qmi_err_num = QMI_ERR_NO_EFFECT_V01;
    LOG_MSG_ERROR("MDNS Aready Enabled", 0, 0, 0);
    return false;
  }
  else
  {
    QCMAP_LAN_CLIENT_RUN_COMMANDS("%s %s", MDNS_CONFIG_FILE, ENABLE_MDNS);
    LOG_MSG_INFO1("Double check the setting result ", 0, 0, 0);
    if(!GetMDNSStatus(&current_mdns_state, qmi_err_num))
    {
      LOG_MSG_INFO1("Double checking fail", 0, 0, 0);
      *qmi_err_num = QMI_ERR_INTERNAL_V01;
      return false;
    }
    if(current_mdns_state == QCMAP_MSGR_MDNS_MODE_UP_V01)
    {
      LOG_MSG_INFO1("Enable mdns done", 0, 0, 0);
      return true;
    }
    else
    {
      LOG_MSG_INFO1("Enable mdns fail", 0, 0, 0);
      *qmi_err_num = QMI_ERR_INTERNAL_V01;
      return false;
    }
  }
}

/*===========================================================================
  FUNCTION DisableMDNS
  ===========================================================================*/
/*!
  @brief
  Stops the M-DNS daemon

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
boolean QCMAP_LAN_Multimedia::DisableMDNS
(
  qmi_error_type_v01 *qmi_err_num
)
{
  qcmap_msgr_mdns_mode_enum_v01 current_mdns_state;
  LOG_MSG_INFO1("Check the current status first!", 0, 0, 0);
  if(!GetMDNSStatus(&current_mdns_state, qmi_err_num))
  {
    LOG_MSG_INFO1("Current status checking fail", 0, 0, 0);
    *qmi_err_num = QMI_ERR_INTERNAL_V01;
    return false;
  }
  if(current_mdns_state == QCMAP_MSGR_MDNS_MODE_DOWN_V01)
  {
    *qmi_err_num = QMI_ERR_NO_EFFECT_V01;
    LOG_MSG_ERROR("MDNS Aready Disabled", 0, 0, 0);
    return false;
  }
  else
  {
    QCMAP_LAN_CLIENT_RUN_COMMANDS("%s %s", MDNS_CONFIG_FILE, DISABLE_MDNS);
    LOG_MSG_INFO1("Double check the setting result ", 0, 0, 0);
    if(!GetMDNSStatus(&current_mdns_state, qmi_err_num))
    {
      LOG_MSG_INFO1("Double checking fail", 0, 0, 0);
      return false;
    }
    if(current_mdns_state == QCMAP_MSGR_MDNS_MODE_DOWN_V01)
    {
      LOG_MSG_INFO1("Disable MDNS done", 0, 0, 0);
      return true;
    }
    else
    {
      LOG_MSG_INFO1("Disable MDNS fail", 0, 0, 0);
      *qmi_err_num = QMI_ERR_INTERNAL_V01;
      return false;
    }
  }

}

/*===========================================================================
  FUNCTION GetMDNSStatus
  ===========================================================================*/
/*!
  @brief
  Returns the status of MDNS

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
boolean QCMAP_LAN_Multimedia::GetMDNSStatus
(
  qcmap_msgr_mdns_mode_enum_v01  *mdns_state,
  qmi_error_type_v01 *qmi_err_num
)
{
  char result[QCMAP_MAX_SCAN_SIZE]={0};

  LOG_MSG_INFO1("Execute command service avahi-daemon status", 0, 0, 0);
  if(!ExecuteSystemCmd("service avahi-daemon status", result, sizeof(result)))
  {
    LOG_MSG_ERROR("Failed to execute command service avahi-daemon status", 0, 0, 0);
    *qmi_err_num = QMI_ERR_INTERNAL_V01;
    return false;
  }
  if(((std::string) result).find("running") != std::string::npos)
    *mdns_state = QCMAP_MSGR_MDNS_MODE_UP_V01;
  else
    *mdns_state = QCMAP_MSGR_MDNS_MODE_DOWN_V01;
  LOG_MSG_INFO1("Get MDNS status succes, the MDNS status is %d. avahi-daemon status: %s ", *mdns_state, result, 0);
  return true;

}

