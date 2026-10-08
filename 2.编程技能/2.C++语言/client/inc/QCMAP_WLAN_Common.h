#ifndef _QCMAP_WLAN_COMMON_H_
#define _QCMAP_WLAN_COMMON_H_

/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                   _ Q C M A P _ W L A N _ C O M M O N . H

*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/
/**
  @file QCMAP_WLAN_Common.h
  @brief QCMAP WLAN Common public function declarations.
         As a note, the client app should not perform time intensive tasks in
         callback context. It should be minimal handling and majority of
         tasks should be handled in client thread context
 */

/*===========================================================================
NOTE: The @brief description above does not appear in the PDF.
      The description that displays in the PDF is maintained in the
      xxx_mainpage.dox file. Contact Tech Pubs for assistance.
===========================================================================*/

/*===========================================================================

FILE:  QCMAP_WLAN_Common.h

SERVICES:
   QCMAP WLAN Common Class

===========================================================================*/
/*===========================================================================

Copyright (c) 2022-2023 Qualcomm Technologies, Inc.
All rights reserved.
Confidential and Proprietary - Qualcomm Technologies, Inc.
===========================================================================*/
/*===========================================================================

                         EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  when       who        what, where, why
  --------   ---        -------------------------------------------------------

===========================================================================*/

/*===========================================================================

Define Structure:
  1. #include
  2. constants
  3. enum constants
  4. strings constants
  5. strings for shell command
  6. strings for path
  7. typedef struct
  8. macro
  9. class definitions
  10. function definitions

===========================================================================*/

#include "QCMAP_LAN_Util.h"
#include "QCMAP_LAN_Client.h"
#include "QCMAP_Client.h"
#include "QCMAP_LAN_Client_Common.h"


#ifdef __cplusplus
extern "C"
{
#endif

#ifdef __cplusplus
}
#endif

/*===========================================================================*/
/** @addtogroup 2.constants
@{ */

#define MAX_FILE_PATH_LEN 256
#define QCMAP_MAX_IFACE_NAME_SIZE_V01 16

/** @} */ /* end_addtogroup 2. constants */
/*===========================================================================*/


/*===========================================================================*/
/** @addtogroup 3.enum constants
@{ */

/** @} */ /* end_addtogroup 3.enum constants */
/*===========================================================================*/


/*===========================================================================*/
/** @addtogroup 4.strings constants
@{ */

#define WLAN_CONFIG_FILE "/etc/data/wlanConfig.sh"

/** uci setting of wlan bootup */
#define UCI_WLAN_BOOTUP_SETTING "qcmap_lan.@no_of_configs[0].wlan_bootup_enable"

/** uci setting of wlan mode */
#define UCI_WLAN_CONFIG_MODE "qcmap_wlan.@wlanconfig[0].mode"


/** @} */ /* end_addtogroup 4.strings constants */
/*===========================================================================*/



/*===========================================================================*/
/** @addtogroup 5.strings for shell command
@{ */

#define CMD_ACTIVATE_HOSTAPD_CONFIG    "activate_hostapd_config"    /** Activate Hostapd with the new config */
#define CMD_ACTIVATE_SUPPLICANT_CONFIG "activate_supplicant_config" /** Activate Supplicant with the new config */

#define CMD_DELETE_GUEST_AP_ACCESS_RULES  "delete_guest_ap_access_rules"  /** DELETE GUEST AP ACCESS RULES */
#define CMD_INSTALL_GUEST_AP_ACCESS_RULES "install_guest_ap_access_rules" /** INSTALL GUEST AP ACCESS RULES */

#define CMD_DISABLE_WLAN             "disable_wlan"
#define CMD_ENABLE_WLAN              "enable_wlan"
#define IS_WIFI_UP                   "is_wifi_up"

#define SET_COEX_CONFIG              "set_coex_config"         /** Enabling the coex **/
#define CMD_GET_ACTIVE_WLAN_INFO     "get_active_wlan_info"    /** Get active wlan device info*/
#define CMD_GET_SAP_STATUS           "get_sap_status"          /** GET SAP STATUS */
#define CMD_GET_STA_STATUS           "get_sta_status"          /** Get STA connection status*/
#define CMD_GET_SUPPLICANT_STATUS    "get_supplicant_status"   /** GET SUPPLICANT STATUS */
#define CMD_GET_ACTIVE_WLAN_INFO_7AP "get_active_wlan_info_7ap"/** Get active wlan device info*/
#define CMD_GET_ACTIVE_MLD_WLAN_INFO "get_active_mld_wlan_info"/** Get active mld wlan device info*/


#define CMD_SET_GUEST_7AP_CONFIG     "set_7apguest_config"     /** Set WLAN guest AP config */
#define CMD_SET_GUEST_AP_CONFIG      "set_guest_config"        /** Set WLAN guest AP config */
#define CMD_SET_PRIMARY_AP_CONFIG    "set_primary_config"      /** Set WLAN primary AP config */
#define CMD_SET_STA_CONFIG           "set_sta_config"          /** Set WLAN STA config mode */
#define CMD_SET_MLD_STA_CONFIG       "set_mld_sta_config"      /** Set MLD WLAN STA config mode */
#define CMD_SET_AP_CONFIG            "set_ap_config"           /** Set WLAN AP config */
#define CMD_SET_MLD_AP_CONFIG        "set_mld_ap_config"       /** Set MLD WLAN AP config */
#define CMD_SET_MLD_AP_CONFIG_LEN    "set_mld_ap_config_len"   /** Set MLD AP config length*/

#define CMD_UPDATE_WIRELESS_NETWORK  "update_wireless_network" /** Update wireless and network files*/
#define CMD_UPDATE_WIRELESS_NETWORK_EX  "update_wireless_network_ex" /** Update wireless and network files EX3*/
#define CMD_WLAN_SET_BOOUP_ENABLE    "set_wlan_bootup_enable"  /** Set WLAN enable on boot up */
#define CMD_WLAN_SET_MODE            "set_wlan_mode"           /** Set WLAN mode */
#define CMD_MLD_WLAN_SET_MODE        "set_mld_wlan_mode"       /** Set WLAN MLD mode */


#define CMD_START_HOSTAPD_CLI_SERVICE_STA "start_hostapd_cli_service_sta" /**<   Connection type. */
#define CMD_START_WPA_CLI_SERVICE         "start_wpa_cli_service"         /** START WPA CLI */

#define CMD_AP_STA_MODE_CONNECT    "ap_sta_mode_connect"       /** AP STA CONNECT */
#define CMD_AP_STA_MODE_DISCONNECT "ap_sta_mode_disconnect"    /** AP STA DISCONNECT */

#define CMD_RESET_WLAN_ATBOOTUP "reset_wlan_atbootup"
#define CMD_RESTART_TETHERED_WLAN_CLIENT  "restart_tethered_wlan_client"

/* Switch wlan enable band */
#define CMD_SWITCH_WLAN_ENABLE_BAND "switch_wlan_enable_band"


#define RESULT_ACTIVATE_HOSTAPD_SUCCESS    "Success"  /* ACTIVATE HOSTAPD SUCCESS */
#define RESULT_ACTIVATE_SUPPLICANT_SUCCESS "Success"  /* ACTIVATE SUPPLICANT SUCCESS */
#define RESULT_SAP_STATE_DFS     "DFS" /** SAP STATE DFS*/
#define RESULT_SAP_STATE_ENABLED "ENABLED" /** SAP STATE ENABLED*/
#define RESULT_STA_ONLY_MODE     "STA Only Mode"  /** STA Only Mode*/
#define RESULT_SUPPLICANT_RUNNING_NORMAL "Supplicant running normal" /** SUPPLICANT RUNNING NORMAL*/


#define WKK_WLAN_DEVICE "ath"   /** WKK WLAN interface name */
#define HMT_WLAN_DEVICE "wlan"  /** HMT WLAN interface name */


/* Macro to get uci need define a char array with name result,  */
#define UCI_WLAN_GET_INT_OPTION_WITH_IDX(retValue, sectionName, optionName, idx) \
        { \
          memset(result, 0, QCMAP_MAX_SCAN_SIZE); \
          if (!QCMAP_LAN_Client::UciGetUtility(owrt_filename[5], sectionName, 0, optionName, result, idx)) \
          {\
            LOG_MSG_ERROR("Failed to UCI GET %s/%s",sectionName,optionName,0);\
            return false;\
          }\
          retValue = atoi(result);\
        }

/* Macro to get uci need define a char array with name result,  */
#define UCI_WLAN_GET_STR_OPTION(retValue, sectionName, optionName) \
{ \
  memset(cmd, 0, QCMAP_MAX_SCAN_SIZE); \
  if (!QCMAP_LAN_Client::UciGetUtility(owrt_filename[5], sectionName, 0, optionName, cmd, 0)) \
  {\
    LOG_MSG_ERROR("Failed to UCI GET %s/%s",sectionName,optionName,0);\
    return false;\
  }\
  strlcpy(retValue, cmd, QCMAP_MAX_IFACE_NAME_SIZE_V01);\
}

/* Macro to get uci need define a char array with name result,  */
#define UCI_WLAN_GET_STR_OPTION_WITH_IDX(retValue, sectionName, optionName, idx) \
{ \
  memset(cmd, 0, QCMAP_MAX_SCAN_SIZE); \
  if (!QCMAP_LAN_Client::UciGetUtility(owrt_filename[5], sectionName, 0, optionName, cmd, idx)) \
  {\
    LOG_MSG_ERROR("Failed to UCI GET %s/%s",sectionName,optionName,0);\
    return false;\
  }\
  strlcpy(retValue, cmd, QCMAP_MAX_IFACE_NAME_SIZE_V01);\
}


/** @} */ /* end_addtogroup 5.strings for shell command */
/*===========================================================================*/



/*===========================================================================*/
/** @addtogroup 6.strings for path
@{ */



/** @} */ /* end_addtogroup 6.strings for path */
/*===========================================================================*/


/*===========================================================================*/
/** @addtogroup 7.typedef struct
@{ */



/** @} */ /* end_addtogroup 7.typedef struct */
/*===========================================================================*/


/*===========================================================================*/
/** @addtogroup 8.macro
@{ */

/*QCMAP_VALIDATE_BAND_INFO*/
#define QCMAP_VALIDATE_BAND_INFO(band)(band == 2 || band == 5 || band == 6 ? true : false)

/* the maco caller need define a char array with name result, and a "in_addr addr" variable*/
#define UCI_WLAN_GET_ADDR_OPTION(retValue, sectionName, optionName) \
          { \
            memset(result, 0, QCMAP_MAX_SCAN_SIZE); \
            if (!QCMAP_LAN_Client::UciGetUtility(owrt_filename[5], sectionName, 0, optionName, result, 0)) \
            {\
              LOG_MSG_ERROR("Failed to UCI GET %s.%s",sectionName,optionName,0);\
              return false;\
            }\
            memset(&addr,0,sizeof(in_addr));\
            if (inet_aton(result, &addr)) {\
                retValue = ntohl(addr.s_addr);\
            }\
          }

/* Macro to get uci need define a char array with name result,  */
#define UCI_WLAN_GET_INT_OPTION(retValue, sectionName, optionName) \
        { \
          memset(result, 0, QCMAP_MAX_SCAN_SIZE); \
          if (!QCMAP_LAN_Client::UciGetUtility(owrt_filename[5], sectionName, 0, optionName, result, 0)) \
          {\
            LOG_MSG_ERROR("Failed to UCI GET %s/%s",sectionName,optionName,0);\
            return false;\
          }\
          retValue = atoi(result);\
        }

/* the maco caller need define a char array with name result, and a "in_addr addr" variable*/
#define UCI_CUR_WLAN_GET_ADDR_OPTION(retValue, sectionName, optionName) \
          { \
            memset(result, 0, QCMAP_MAX_SCAN_SIZE); \
            if (!QCMAP_LAN_Client::UciGetUtility(owrt_filename[9], sectionName, 0, optionName, result, 0)) \
            {\
              LOG_MSG_ERROR("Failed to UCI GET %s.%s",sectionName,optionName,0);\
              return false;\
            }\
            memset(&addr,0,sizeof(in_addr));\
            if (inet_aton(result, &addr)) {\
                retValue = ntohl(addr.s_addr);\
            }\
          }

/* Macro to get uci need define a char array with name result,  */
#define UCI_CUR_WLAN_GET_INT_OPTION(retValue, sectionName, optionName) \
        { \
          memset(result, 0, QCMAP_MAX_SCAN_SIZE); \
          if (!QCMAP_LAN_Client::UciGetUtility(owrt_filename[9], sectionName, 0, optionName, result, 0)) \
          {\
            LOG_MSG_ERROR("Failed to UCI GET %s/%s",sectionName,optionName,0);\
            return false;\
          }\
          retValue = atoi(result);\
        }

/** @} */ /* end_addtogroup 8.macro */
/*===========================================================================*/


//===================================================================
//              Class Definitions
//===================================================================

/**  @ingroup qcmap_lan_class */
class QCMAP_WLAN_Common
{
private:
  virtual boolean FillSpecificWlanIfInfo(qcmap_msgr_wlan_if_info_t *wlan_info_cfg, int wlan_mode) = 0;

public:
  char mWLANConfigFile[MAX_FILE_PATH_LEN];
  boolean IsWlanEnabled;

public:

/*===========================================================================
FUNCTION QCMAP_WLAN_Common()
===========================================================================*/
/** @ingroup qcmap_lan_class

   Constructor for the WLAN client class.

   This constructor initializes the WLAN Client.

   @return
   None.
*/
/*=========================================================================*/
QCMAP_WLAN_Common(void);

virtual ~QCMAP_WLAN_Common() = 0;
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
boolean EnableWLAN(qmi_error_type_v01 *qmi_err_num);

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
boolean DisableWLAN(qmi_error_type_v01 *qmi_err_num);

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
boolean ActivateWLAN(qmi_error_type_v01 *qmi_err_num);

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
boolean
SetWLANConfigEx
(
  qcmap_wlan_ex2_config& wlan_config,
  qmi_error_type_v01     *qmi_err_num
);

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
  qmi_error_type_v01 qmi_err_num

@note
  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
boolean
SetWLANConfigEx3
(
  qcmap_wlan_ex3_config_t& wlan_config,
  qmi_error_type_v01     *qmi_err_num
);

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
boolean
GetWLANConfigEx3
(
  qcmap_wlan_ex3_config_t *wlan_config,
  qmi_error_type_v01     *qmi_err_num
);


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
boolean
SetWLANBootupConfigEx
(
  qcmap_msgr_bootup_flag_v01 wlan_enable,
  qmi_error_type_v01 *qmi_err_num
);

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

  - Side EffectsS
    - None
*/
/*=========================================================================*/
boolean
GetWLANBootupConfigEx
(
  qcmap_bootup_enable_config *bootup_config,
  qmi_error_type_v01 *qmi_err_num
);

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
virtual boolean
GetWLANConfigEx
(
  qcmap_wlan_ex_config *wlan_config,
  qmi_error_type_v01 *qmi_err_num
);

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
virtual boolean
GetActiveWLANConfigEx
(
  qcmap_wlan_ex_config *wlan_config,
  qmi_error_type_v01 *qmi_err_num
);

virtual boolean
IsWifiUp
(
   qmi_error_type_v01 *qmi_err_num
) = 0;

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
bool
GetWLANStatus
(
  qcmap_msgr_wlan_mode_enum_v01 *wlan_mode,
  qmi_error_type_v01 *qmi_err_num
);

/*===========================================================================
FUNCTION SetCoexConfig()
===========================================================================*/
/*

  Enable/Disable the CoEX feature to reduce co-channel interference
  between WLAN <-> WWAN channels.

  @param[in]  coex_state   Enable/Disable CoEX.

  @return
  TRUE -- Success. \n
  FALSE -- Failure.

  @dependencies
  None
*/
boolean
SetCoexConfig
(
  int coex_state
);

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
  qcmap_msgr_station_mode_status_enum_v01 sta_status

@note
  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
bool
GetStationModeStatus
(
  qcmap_msgr_station_mode_status_enum_v01 *sta_status,
  qmi_error_type_v01 *qmi_err_num
);

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
boolean
GetActiveWlanIfInfo
(
  qcmap_msgr_wlan_if_info_t *wlan_info_cfg,
  qmi_error_type_v01 *qmi_err_num
);

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
boolean
ActivateHostapdConfig
(
  qcmap_msgr_activate_hostapd_ap_enum_v01 ap_type,
  qcmap_msgr_activate_hostapd_action_enum_v01 action_type,
  qmi_error_type_v01 *qmi_err_num
);

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
boolean
ActivateSupplicantConfig
(
  qmi_error_type_v01 *qmi_err_num
);

boolean
SwitchWlanEnableBand
(
  qcmap_msgr_sap_band_status_enum_v01 sap_5g_enable_state,
  qcmap_msgr_sta_band_status_enum_v01 sta_5g_enable_state
);

boolean
ProcessStaIndicate
(
  qcmap_msgr_sta_connect_status_enum_v01 sta_connection_state
);


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
boolean DisAssociateClient(const char* mac_addr_str) ;
boolean IsWiFiDevcies(const char *phy_iface);
boolean IsBridgeMode();
boolean IsStationStatic();
boolean InstallGuestAPRules();
boolean UnInstallGuestAPRules();

virtual boolean DisAssociateClient_Device(const char* device, const char* mac_addr_str) = 0;


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
virtual boolean RestartTetheredWLANClient() = 0;

virtual boolean ResetWLANatBootup() = 0;
};
#endif /* _QCMAP_WLAN_COMMON_H_ */
