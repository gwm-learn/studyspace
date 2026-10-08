/*====================================================

FILE:  QCMAP_Client_OWRT.cpp

SERVICES:
QCMAP Client Implementation

=====================================================

  Copyright 2023 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Confidential and Proprietary - Qualcomm Technologies, Inc.

=====================================================*/

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
#include "ds_util.h"
#include "ds_string.h"
#include "qualcomm_mobile_access_point_msgr_v01.h"
#include "QCMAP_Client.h"
#include "QCMAP_WLAN_Common.h"
#include "QCMAP_LAN_Multimedia.h"

#include <stdarg.h>

#define QCMAP_MSGR_QMI_TIMEOUT_VALUE     90000
#define DEFAULT_PROFILE_HANDLE           0         /* Default Profile Handle for WWAN */

#define QCMAP_QMI_SERVER_INSTANCE_ID_0       0x0
#define QCMAP_QMI_SERVER_INSTANCE_ID_1       0x1
#define QCMAP_QMI_SERVER_POLLING_TIMEOUT     5000

/*---------------------------------------------------------------------------
  Return values indicating error status
---------------------------------------------------------------------------*/
#define QCMAP_CM_SUCCESS               0         /* Successful operation   */
#define QCMAP_CM_ERROR                -1         /* Unsuccessful operation */
#define TRUE                           1

#define BZERO_QMI_MSG(qmi_msg) memset(&qmi_msg, 0, sizeof(qmi_msg))

 /* Set QMI Optional Param to specified value */
#define QCMAP_QMI_SET_OPTIONAL_PARAM(param, value) param ## _valid = true; \
                                                    param           = value;

 /* Get QMI Optional Param, if TLV is valid */
#define QCMAP_QMI_GET_OPTIONAL_PARAM(param, invalid_value) (param ## _valid == true) ? param : invalid_value

#define QCMAP_LOG(...)                         \
  LOG_MSG_INFO1( "%s %d:", __FILE__, __LINE__,0); \
  LOG_MSG_INFO1( __VA_ARGS__ ,0,0); \
  LOG_MSG_INFO3("Client Handles, MDM/Standalone=%p, EAP=%p, Preferred=%p",  \
                 m_qmi_qcmap_instance0_handle, m_qmi_qcmap_instance1_handle,\
                 m_qmi_qcmap_preferred_handle)

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


extern QCMAP_LAN_Client *QcMapLanClient;
extern QCMAP_LAN_Multimedia *QCMapMMObj ;

static boolean GetWWANInfo(uint32_t *default_handle, uint32_t *profile_handle, QCMAP_Client *QcMapClient)
{
  /*
      todo:
      remove QCMAP_Client *QcMapClient via extern
  */
  qcmap_wwan_policy_list_info wwan_policy_list;
  qmi_error_type_v01 qmi_err_num;

  if (QcMapClient->GetWWANPolicyListEx(&wwan_policy_list, &qmi_err_num))
  {
    if (wwan_policy_list.wwan_policy_len == 1)
    {
      /* check if default_profile_handle is valid */
      if (wwan_policy_list.default_profile_handle_valid)
      {
        *default_handle = wwan_policy_list.default_profile_handle;
        *profile_handle = wwan_policy_list.wwan_policy[0].profile_handle;
        return true;
      }
      else
      {
        printf("default profile handle is invalid");
        return false;
      }
    }
    QcMapClient->GetWWANProfilePreference(profile_handle, &qmi_err_num);
    for (int i=0; i < wwan_policy_list.wwan_policy_len; i++)
    {
      if (wwan_policy_list.wwan_policy[i].profile_handle == wwan_policy_list.default_profile_handle)
        *default_handle = wwan_policy_list.wwan_policy[i].profile_handle;
    }
    return true;
  }
  return false;
}

/*===========================================================================
  FUNCTION GetBackhaulStatus
  ===========================================================================*/
/*!
  @brief
  Gets Backahul Status and backhaul type and ip version if connected

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
boolean QCMAP_Client::GetBackhaulStatus
(
  qcmap_backhaul_status_info_type *backhaul_status_info,
  qmi_error_type_v01 *qmi_err_num
)
{
  qcmap_msgr_get_backhaul_status_resp_msg_v01 get_backhaul_status_resp_msg;
  qmi_client_error_type qmi_error;

  QCMAP_LOG_FUNC_ENTRY();

  if (qmi_err_num == NULL)
  {
    LOG_MSG_ERROR("GetBackhaulStatus Failed: NULL Args",0,0,0);
    return false;
  }

  if (backhaul_status_info == NULL)
  {
    *qmi_err_num = QMI_ERR_INVALID_ARG_V01;
    LOG_MSG_ERROR("GetBackhaulStatus Failed: %d",*qmi_err_num,0,0);
    return false;
  }

  memset(backhaul_status_info, 0x0, sizeof(qcmap_backhaul_status_info_type));

  profile_handle_type_v01 current_profile_handle;
  if (!GetWWANProfilePreference(&current_profile_handle, qmi_err_num))
  {
    LOG_MSG_ERROR("Error getting current profile handle 0x%x.\n ", qmi_err_num,0,0);
    return false;
  }

  if (!QcMapLanClient->GetCurrentActiveBackhaul(current_profile_handle, (qcmap_backhaul_status_info_ex_type *)backhaul_status_info, qmi_err_num))
  {
    LOG_MSG_ERROR("Error getting current active backhaul 0x%x.\n ", qmi_err_num,0,0);
    return false;
  }

  LOG_MSG_INFO1("Get Backhaul Status is success , Backhaul type : %d, "
                "V4 status : %d, V6 status : %d", backhaul_status_info->backhaul_type,
                 backhaul_status_info->backhaul_v4_available,
                 backhaul_status_info->backhaul_v6_available);

  return true;
}


/*===========================================================================
  FUNCTION GetBackhaulStatusEx
  ===========================================================================*/
/*!
  @brief
  Gets Backahul Status and backhaul type and ip version if connected

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

boolean QCMAP_Client::GetBackhaulStatusEx
(
  qcmap_backhaul_status_info_ex_type *backhaul_status_info,
  qmi_error_type_v01 *qmi_err_num
)
{
  QCMAP_LOG_FUNC_ENTRY();

  if (qmi_err_num == NULL)
  {
    LOG_MSG_ERROR("GetBackhaulStatus Failed: NULL Args",0,0,0);
    return false;
  }

  if (backhaul_status_info == NULL)
  {
    *qmi_err_num = QMI_ERR_INVALID_ARG_V01;
    LOG_MSG_ERROR("GetBackhaulStatus Failed: %d",*qmi_err_num,0,0);
    return false;
  }

  memset(backhaul_status_info, 0x0, sizeof(qcmap_backhaul_status_info_ex_type));

  profile_handle_type_v01 current_profile_handle;
  if (!GetWWANProfilePreference(&current_profile_handle, qmi_err_num))
  {
    LOG_MSG_ERROR("Error getting current profile handle 0x%x.\n ", qmi_err_num,0,0);
    return false;
  }

  if (!QcMapLanClient->GetCurrentActiveBackhaul(current_profile_handle, backhaul_status_info, qmi_err_num))
  {
    LOG_MSG_ERROR("Error getting current active backhaul 0x%x.\n ", qmi_err_num,0,0);
    return false;
  }

  LOG_MSG_INFO1("Get Backhaul Status is success , Backhaul type : %d, "
                "V4 status : %d, V6 status : %d", backhaul_status_info->backhaul_type,
                 backhaul_status_info->backhaul_v4_available,
                 backhaul_status_info->backhaul_v6_available);
  LOG_MSG_INFO1("Get Backhaul Status is success , Backhaul type : %d, "
                "ETH status : %d", backhaul_status_info->backhaul_type,
                 backhaul_status_info->backhaul_eth_available,0);

  return true;
}


/*===========================================================================
  FUNCTION AddFireWallEntry
  ===========================================================================*/
/*!
  @brief
  Encode a firewall configuration into a msgr message and sends the same to
  QCMAP connection manager to add firewall configuration

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
boolean QCMAP_Client::AddFireWallEntry
(
  qcmap_msgr_firewall_conf_t *firewall_conf,
  qmi_error_type_v01         *qmi_err_num
)
{
  profile_handle_type_v01 current_profile_handle;

  if ( !(this->mobile_ap_handle > 0 && this->m_qcmap_msgr_enable == true) )
  {
    *qmi_err_num = QMI_ERR_INVALID_HANDLE_V01;
    return false;
  }
  if (!this->GetWWANProfilePreference(&current_profile_handle, qmi_err_num))
  {
    printf("Error getting current profile handle 0x%x.\n ", qmi_err_num);
    return false;
  }

  if ( QcMapLanClient->AddFireWallEntry(firewall_conf,current_profile_handle, qmi_err_num)== true)
  {
    return true;
  }
  else
  {
    LOG_MSG_ERROR("Add Firewall Entry failed , Error: 0x%x", qmi_err_num,0,0);
    return false;
  }
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


boolean QCMAP_Client::SetFirewall(
  boolean             enable_firewall,
  boolean             pkts_allowed,
  qmi_error_type_v01  *qmi_err_num
)
{

    profile_handle_type_v01 current_profile_handle;
    if ( !(this->mobile_ap_handle > 0 && this->m_qcmap_msgr_enable == true) )
    {
      *qmi_err_num = QMI_ERR_INVALID_HANDLE_V01;
      return false;
    }
    if (!this->GetWWANProfilePreference(&current_profile_handle, qmi_err_num))
    {
      printf("Error getting current profile handle 0x%x.\n ", qmi_err_num);
      return false;
    }
    if(QcMapLanClient->SetFirewall(enable_firewall, pkts_allowed, current_profile_handle, qmi_err_num))
    {
       return true;
    }
    else
    {
      LOG_MSG_ERROR("Set Firewall Failed , Error: 0x%x", qmi_err_num,0,0);
      return false;
    }


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
boolean QCMAP_Client::GetFirewall
(
  boolean               *enable_firewall,
  boolean               *pkts_allowed,
  qmi_error_type_v01    *qmi_err_num
)
{
    profile_handle_type_v01 current_profile_handle;
    if (!this->GetWWANProfilePreference(&current_profile_handle, qmi_err_num))
    {
      printf("Error getting current profile handle 0x%x.\n ", qmi_err_num);
    }
    if (QcMapLanClient->GetFirewall(current_profile_handle, enable_firewall, pkts_allowed, qmi_err_num))
    {
       return true;
    }
    else
    {
      LOG_MSG_ERROR("Get Firewall Failed , Error: 0x%x", qmi_err_num,0,0);
      return false;
    }

}

/*===========================================================================
  FUNCTION GetFireWallHandleList
  ===========================================================================*/
/*!
  @brief
  Gets the firewall handle list
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

boolean QCMAP_Client::GetFireWallHandlesList
(
  qcmap_msgr_get_firewall_handle_list_conf_t *handlelist,
  qmi_error_type_v01                         *qmi_err_num
)
{

    profile_handle_type_v01 current_profile_handle;
    int handle_list_len = 0;

    if (!this->GetWWANProfilePreference(&current_profile_handle, qmi_err_num))
    {
     printf("Error getting current profile handle 0x%x.\n ", qmi_err_num);
    return false;
    }

    if((handlelist != NULL) && QcMapLanClient->GetFireWallHandlesList(current_profile_handle, handlelist, qmi_err_num))
    {
      return true;
    }
    else
    {
      LOG_MSG_ERROR("Display Firewall Failed , Error: 0x%x", qmi_err_num,0,0);
      return false;
    }

}


/*==========================================================================
  FUNCTION DeleteFireWallEntry
  ===========================================================================*/
/*!
  @brief
  Sends delete message for firewall entry identified by the handle
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
int QCMAP_Client::DeleteFireWallEntry(int handle, qmi_error_type_v01  *qmi_err_num)
{

    profile_handle_type_v01 current_profile_handle;
    if (!this->GetWWANProfilePreference(&current_profile_handle, qmi_err_num))
    {
      printf("Error getting current profile handle 0x%x.\n ", qmi_err_num);
      return false;
    }
    if (QcMapLanClient->DeleteFireWallEntry(handle,current_profile_handle, qmi_err_num))
    {
      return true;
    }
    else
    {
      LOG_MSG_ERROR("Delete firewall failed, Error: 0x%x", qmi_err_num,0,0);
      return false;
    }
}


/*===========================================================================
  FUNCTION SetHWMACFilteringState()
  ===========================================================================*/
  /** @ingroup qcmap_set_hw_mac_filter_state

  Configure the HW Filtering State.

  @datatypes
  qcmap_msgr_config_state_enum_v01 \n
  qcmap_hw_filter_config \n
  qmi_error_type_v01
  @param[in] state                       Sets HW Filtering current state
  @param[in] hw_filter_config            HW Filter Configuration
  @param[out]  resp_msg                  Response Message
  @param[out] qmi_err_num                Error code returned by the server.

  @return
  TRUE upon success. \n
  FALSE upon failure.

  @dependencies
  None
  */

  /*=========================================================================*/
bool QCMAP_Client::SetHWMACFilteringState
(
  qcmap_msgr_config_state_enum_v01    state,
  qcmap_hw_filter_config              hw_filter_config,
  qmi_error_type_v01                  *qmi_err_num
)
{
    qcmap_hdw_filter_config             hw_filter_config_lan;
    memset(&hw_filter_config_lan, 0, sizeof(qcmap_hdw_filter_config));

    hw_filter_config_lan.num_of_clients = hw_filter_config.num_of_clients;
    hw_filter_config_lan.num_of_iface = hw_filter_config.num_of_iface;
    hw_filter_config_lan.num_of_ip_segments = hw_filter_config.num_of_ip_segments;

    hw_filter_config_lan.mac_flt_state =  (qcmap_config_state)hw_filter_config.mac_flt_state;
    hw_filter_config_lan.iface_filter_state = (qcmap_config_state)hw_filter_config.iface_filter_state;

    if ( !(this->mobile_ap_handle > 0 && this->m_qcmap_msgr_enable == true) )
    {
      *qmi_err_num = QMI_ERR_INVALID_HANDLE_V01;
      return false;
    }

    hw_filter_config_lan.ip_segment_filter_state = (qcmap_config_state)hw_filter_config.ip_segment_filter_state;
    memcpy(hw_filter_config_lan.client_list, hw_filter_config.client_list, sizeof(hw_filter_config.client_list));
    memcpy(hw_filter_config_lan.ip_segment_filter_list, hw_filter_config.ip_segment_filter_list, sizeof(hw_filter_config.ip_segment_filter_list));
    memcpy(hw_filter_config_lan.if_name_filter_list, hw_filter_config.if_name_filter_list, sizeof(hw_filter_config.if_name_filter_list));

    if(QcMapLanClient->SetHWMACFilteringState((qcmap_config_state)state, hw_filter_config_lan, qmi_err_num))
    {
      return true;
    }
    else
    {
      if(QcMapLanClient->ResetHWFilteringStateOnDisable(QCMAP_CONFIG_DISABLE))
      {
        printf("\nSet Hardware Filtering State Failed \n");
      }
      return false;
    }
}


  /*===========================================================================
  FUNCTION GetHWMACFilteringState()
  ===========================================================================*/
  /** @ingroup qcmap_get_hw_mac_filter_state

  Get the current HW Filtering State.

  @datatypes
  qcmap_msgr_config_state_enum_v01 \n
  qcmap_hw_filter_config \n
  qmi_error_type_v01
  @param[out] status                      Current status of Hardware MAC Filtering State
  @param[out] hw_filter_config           MAC Filter Configuration
  @param[out] qmi_err_num                 Error code returned by the server.

  @return
  TRUE upon success. \n
  FALSE upon failure.

  @dependencies
  None
  */

  /*=========================================================================*/
bool QCMAP_Client::GetHWMACFilteringState
(
  qcmap_msgr_config_state_enum_v01    *status,
  qcmap_hw_filter_config              *hw_filter_config,
  qmi_error_type_v01                  *qmi_err_num
)
{
    qcmap_hdw_filter_config hw_filter_config_lan;
    qcmap_config_state   status_lan = QCMAP_CONFIG_STATE_ENUM_MAX_ENUM_VAL;
    memset(&hw_filter_config_lan, 0, sizeof(qcmap_hdw_filter_config));

    if (QcMapLanClient->GetHWFilteringState(&status_lan, &hw_filter_config_lan, qmi_err_num))
    {
        hw_filter_config->num_of_clients = hw_filter_config_lan.num_of_clients;
        hw_filter_config->num_of_iface = hw_filter_config_lan.num_of_iface;
        hw_filter_config->num_of_ip_segments = hw_filter_config_lan.num_of_ip_segments;

        hw_filter_config->mac_flt_state =  (qcmap_msgr_config_state_enum_v01)hw_filter_config_lan.mac_flt_state;
        hw_filter_config->iface_filter_state = (qcmap_msgr_config_state_enum_v01)hw_filter_config_lan.iface_filter_state;

        hw_filter_config->ip_segment_filter_state = (qcmap_msgr_config_state_enum_v01)hw_filter_config_lan.ip_segment_filter_state;
        memcpy(hw_filter_config->client_list, hw_filter_config_lan.client_list, sizeof(hw_filter_config->client_list));
        memcpy(hw_filter_config->ip_segment_filter_list, hw_filter_config_lan.ip_segment_filter_list, sizeof(hw_filter_config->ip_segment_filter_list));
        memcpy(hw_filter_config->if_name_filter_list, hw_filter_config_lan.if_name_filter_list, sizeof(hw_filter_config->if_name_filter_list));

       *status = (qcmap_msgr_config_state_enum_v01)status_lan;

      return true;
    }
    else
    {
      LOG_MSG_ERROR("Get HW Filtering Feature State failed,Error %d", qmi_err_num, 0, 0);
      return false;
    }

}

/*===========================================================================
  FUNCTION GetEthernetNicConfig
  ===========================================================================*/
/*!
  @brief
  Gets Ethernet Tethering Mode and NIC config

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
boolean QCMAP_Client::GetEthernetNicConfig
(
  qcmap_eth_config       *eth_config,
  qmi_error_type_v01     *qmi_err_num
)
{
    qcmap_eth_config_t lan_eth_cfg;
    ZERO_INIT_ARG(lan_eth_cfg);

    if (QcMapLanClient->GetEthernetNicConfig(&lan_eth_cfg, qmi_err_num))
    {
      eth_config->is_eth_nics_config_valid = lan_eth_cfg.is_eth_nics_config_valid;
      eth_config->is_macsec_nic_config_valid = lan_eth_cfg.is_macsec_nic_config_valid;
      eth_config->mode = (qcmap_msgr_ethernet_mode_v01)lan_eth_cfg.mode;
      eth_config->no_of_nics = lan_eth_cfg.no_of_nics;

      for(int i=0; i<eth_config->no_of_nics; i++)
      {
        strlcpy((eth_config->eth_nic_config[i]).eth_iface_name,lan_eth_cfg.eth_nic_config[i].eth_iface_name, QCMAP_MAX_IFACE_NAME_SIZE);
        (eth_config->eth_nic_config[i]).eth_nic_type = (qcmap_eth_network_type_v01)lan_eth_cfg.eth_nic_config[i].eth_nic_nw_type;
      }

      eth_config->is_macsec_nic_config_valid = lan_eth_cfg.is_macsec_nic_config_valid;
      if (lan_eth_cfg.is_macsec_nic_config_valid)
      {
        eth_config->no_of_macsec_nics = lan_eth_cfg.no_of_macsec_nics;
        for(int i=0; i<lan_eth_cfg.no_of_macsec_nics; i++)
        {
          eth_config->macsec_nic_config[i].state = (qcmap_msgr_config_state_enum_v01)lan_eth_cfg.macsec_nic_config[i].state;
          strlcpy(eth_config->macsec_nic_config[i].eth_nic_iface_name,lan_eth_cfg.macsec_nic_config[i].eth_nic_iface_name, QCMAP_MAX_IFACE_NAME_SIZE);
          strlcpy(eth_config->macsec_nic_config[i].macsec_iface_name,lan_eth_cfg.macsec_nic_config[i].macsec_iface_name, QCMAP_MAX_IFACE_NAME_SIZE);
          eth_config->macsec_nic_config[i].macsec_mode = (qcmap_msgr_macsec_mode_enum_v01)lan_eth_cfg.macsec_nic_config[i].macsec_mode;
        }
      }

      return true;
    }
    else
    {
      LOG_MSG_ERROR("Failed to Get Ethernet Mode , Error: 0x%x", qmi_err_num,0,0);
      return false;
    }
}

/*===========================================================================
  FUNCTION SetEthernetNicConfig
  ===========================================================================*/
/*!
  @brief
  Sets Ethernet Mode and NIC config
  Use this API to config single NIC and Dual NIC config

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
boolean QCMAP_Client::SetEthernetNicConfig
(
  qcmap_eth_config    eth_config,
  qmi_error_type_v01  *qmi_err_num
)
{

  qcmap_eth_config_t lan_eth_cfg;
  memset(&lan_eth_cfg, 0x0, sizeof(qcmap_eth_config_t));

//  memcpy(eth_cfg.eth_nic_config, eth_config.eth_nic_config, sizeof(eth_config.eth_nic_config));
//  memcpy(eth_cfg.macsec_nic_config, eth_config.macsec_nic_config, sizeof(eth_config.macsec_nic_config));

  if ( !(this->mobile_ap_handle > 0 && this->m_qcmap_msgr_enable == true) )
  {
    *qmi_err_num = QMI_ERR_INVALID_HANDLE_V01;
    return false;
  }

  lan_eth_cfg.is_eth_nics_config_valid = eth_config.is_eth_nics_config_valid;
  lan_eth_cfg.is_macsec_nic_config_valid = eth_config.is_macsec_nic_config_valid;
  lan_eth_cfg.mode = (qcmap_ethernet_mode)eth_config.mode;
  lan_eth_cfg.no_of_nics = eth_config.no_of_nics;

  for(int i=0; i<eth_config.no_of_nics; i++)
  {
    strlcpy(lan_eth_cfg.eth_nic_config[i].eth_iface_name, eth_config.eth_nic_config[i].eth_iface_name, QCMAP_MAX_IFACE_NAME_SIZE);
    lan_eth_cfg.eth_nic_config[i].eth_nic_nw_type = (qcmap_eth_network_type)eth_config.eth_nic_config[i].eth_nic_type;
  }

  if (eth_config.is_macsec_nic_config_valid)
  {
    lan_eth_cfg.no_of_macsec_nics = eth_config.no_of_macsec_nics;
    for(int i=0; i<eth_config.no_of_macsec_nics; i++)
    {
      strlcpy(lan_eth_cfg.macsec_nic_config[i].eth_nic_iface_name, eth_config.macsec_nic_config[i].eth_nic_iface_name, QCMAP_MAX_IFACE_NAME_SIZE);
      //macsec name is no longer passed by user (autolearned instead), sending placeholer name
      strlcpy(lan_eth_cfg.macsec_nic_config[i].macsec_iface_name, "tmpmacsec", QCMAP_MAX_IFACE_NAME_SIZE);
      lan_eth_cfg.macsec_nic_config[i].state = (qcmap_config_state_enum)eth_config.macsec_nic_config[i].state;
      lan_eth_cfg.macsec_nic_config[i].macsec_mode = (qcmap_macsec_mode_enum)eth_config.macsec_nic_config[i].macsec_mode;
    }
  }

  int ret = QcMapLanClient->SetEthernetNicConfig(lan_eth_cfg, qmi_err_num);
  if(ret)
  {
    return true;
  }
  else
  {
    LOG_MSG_ERROR("Fail to set Ethernet NIC config , Error: 0x%x", *qmi_err_num,0,0);
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
boolean QCMAP_Client::BringupBTTethering
(
  qmi_error_type_v01                *qmi_err_num,
  qcmap_bt_tethering_mode_enum_v01   bt_tethering_mode
)
{
  if(bt_tethering_mode != QCMAP_MSGR_BT_MODE_LAN_V01 && bt_tethering_mode != QCMAP_MSGR_BT_MODE_WAN_V01)
  {
    *qmi_err_num = QMI_ERR_INVALID_ARG_V01;
    return false;
  }

  return QcMapLanClient->BringupBTTethering((qcmap_bt_mode)bt_tethering_mode);
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
boolean QCMAP_Client::BringdownBTTethering(qmi_error_type_v01 *qmi_err_num)
{
  qcmap_msgr_bt_tethering_status_enum_v01  bt_teth_status;
  qcmap_bt_tethering_mode_enum_v01         bt_teth_mode;
  if(GetBTTetheringStatus(&bt_teth_status, qmi_err_num, &bt_teth_mode) == false)
    return false;

  return QcMapLanClient->BringdownBTTethering((qcmap_bt_mode)bt_teth_mode);
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
boolean QCMAP_Client::GetBTTetheringStatus
(
  qcmap_msgr_bt_tethering_status_enum_v01  *bt_teth_status,
  qmi_error_type_v01                       *qmi_err_num,
  qcmap_bt_tethering_mode_enum_v01         *bt_teth_mode
)
{
    qcmap_bt_tethering_status    bt_teth_status_lan;
    qcmap_bt_mode                bt_teth_mode_lan;
    if (QcMapLanClient->GetBTTetheringStatus(&bt_teth_status_lan, qmi_err_num, &bt_teth_mode_lan))
    {
       *bt_teth_status = (qcmap_msgr_bt_tethering_status_enum_v01)bt_teth_status_lan;
       *bt_teth_mode = (qcmap_bt_tethering_mode_enum_v01)bt_teth_mode_lan;
        return true;
      }
      else
    {
       LOG_MSG_ERROR("  Failed to Get BT Tethering Status: 0x%x", *qmi_err_num,0,0);
       return false;
    }
}


/*===========================================================================
  FUNCTION GetStaticNatConfig
  ===========================================================================*/
/*!
  @brief
  Deletes a static nat entry

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
boolean QCMAP_Client::GetStaticNatConfig
(
  qcmap_msgr_snat_entry_config_v01 *snat_config,
  int                              *num_entries,
  qmi_error_type_v01               *qmi_err_num
)
{
    qcmap_snat_config_t snat_configs_lan[QCMAP_MAX_SNAT_ENTRIES];

    uint32_t ipconvert = 0;
    int result = 0;

    memset(&snat_configs_lan, 0, QCMAP_MAX_SNAT_ENTRIES*sizeof(qcmap_snat_config_t));
    profile_handle_type_v01 current_profile_handle;

    if (!this->GetWWANProfilePreference(&current_profile_handle, qmi_err_num))
    {
       printf("Error getting current profile handle 0x%x.\n ", *qmi_err_num);
       return false;
    }
    if (QcMapLanClient->GetStaticNatConfig(snat_configs_lan, (uint16_t*)num_entries, current_profile_handle, qmi_err_num))
    {
      if(*num_entries > 0)
      {
        for (int i=0; i<*num_entries; i++)
        {
          result = inet_pton(AF_INET, snat_configs_lan[i].private_ip_addr, &ipconvert);
          snat_config[i].private_ip_addr = ntohl(ipconvert);
          snat_config[i].protocol = snat_configs_lan[i].protocol;

          if(snat_configs_lan[i].protocol != ICMP )
          {
            snat_config[i].private_port = snat_configs_lan[i].private_port;
            snat_config[i].global_port = snat_configs_lan[i].global_port;
          }
        }
      }
      return true;
    }
    else
    {
        return false;
    }


}


/*===========================================================================
  FUNCTION AddStaticNatEntry
  ===========================================================================*/
/*!
  @brief
  Add a static nat entry

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
boolean QCMAP_Client::AddStaticNatEntry
(
  qcmap_msgr_snat_entry_config_v01 *snat_entry,
  qmi_error_type_v01               *qmi_err_num
)
{
    qcmap_snat_config_t snat_config;
    ZERO_INIT_ARG(snat_config);

    if ( !(this->mobile_ap_handle > 0 && this->m_qcmap_msgr_enable == true) )
    {
      *qmi_err_num = QMI_ERR_INVALID_HANDLE_V01;
      return false;
    }

    struct in_addr addr;
    snat_config.private_port = snat_entry->private_port;
    snat_config.global_port = snat_entry->global_port;
    snat_config.protocol = snat_entry->protocol;
    addr.s_addr = htonl(snat_entry->private_ip_addr);
    char * ip_addr = inet_ntoa(addr);
    strlcpy(snat_config.private_ip_addr, ip_addr, QCMAP_IPV4_ADDR_LEN);

    profile_handle_type_v01 current_profile_handle;
    if (!this->GetWWANProfilePreference(&current_profile_handle, qmi_err_num))
    {
      printf("Error getting current profile handle 0x%x.\n ", qmi_err_num);
      return false;
    }
    if(QcMapLanClient->AddStaticNatEntry(snat_config, current_profile_handle, qmi_err_num))
    {
      return true;
    }
    else
    {
      LOG_MSG_ERROR("Failed to addStaticNatEntry , Error: 0x%x", *qmi_err_num,0,0);
      return false;
    }

}

/*===========================================================================
  FUNCTION DeleteStaticNatEntry
  ===========================================================================*/
/*!
  @brief
  Deletes a static nat entry

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
boolean QCMAP_Client::DeleteStaticNatEntry
(
  qcmap_msgr_snat_entry_config_v01 *snat_entry,
  qmi_error_type_v01               *qmi_err_num
)
{

     /*
       changes needed pass the snat_entry from cli_main to this function need to change the structure information
       inputs from the user are present in the snat_entry.. put them in the snat_config
     */

     profile_handle_type_v01 current_profile_handle;
     if (!this->GetWWANProfilePreference(&current_profile_handle, qmi_err_num))
     {
        printf("Error getting current profile handle 0x%x.\n ", *qmi_err_num);
        return false;
     }
     if ( !(this->mobile_ap_handle > 0 && this->m_qcmap_msgr_enable == true) )
     {
       *qmi_err_num = QMI_ERR_INVALID_HANDLE_V01;
       return false;
     }

    struct in_addr addr;
    qcmap_snat_config_t snat_config;
    ZERO_INIT_ARG(snat_config);
    ZERO_INIT_ARG(addr);

    addr.s_addr = htonl(snat_entry->private_ip_addr);
    char * ip_addr = inet_ntoa(addr);
    strlcpy(snat_config.private_ip_addr, ip_addr, QCMAP_IPV4_ADDR_LEN);

    snat_config.private_port = snat_entry->private_port;
    snat_config.global_port = snat_entry->global_port;
    snat_config.protocol = snat_entry->protocol;

//    memcpy(snat_config.private_ip_addr, snat_config.private_ip_addr, sizeof(snat_config.private_ip_addr));

    if(QcMapLanClient->DeleteStaticNatEntry(snat_config, current_profile_handle, qmi_err_num))
    {
      return true;
    }
    else
    {
      LOG_MSG_ERROR("Failed to DeleteStaticNatEntry , Error0x%x", *qmi_err_num,0,0);
      return false;
    }

}

/*===========================================================================
  FUNCTION GetWLANConfigEx
  ===========================================================================*/
/*!
  @brief
  Gets the current configured WLAN Configuration.

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

boolean QCMAP_Client::GetWLANConfigEx
(
  qcmap_wlan_ex_config                *wlan_ex_config,
  qmi_error_type_v01                  *qmi_err_num
)
{
   return QcMapLanClient->GetWLANConfigEx(wlan_ex_config, qmi_err_num);
}

/*===========================================================================
  FUNCTION SetWLANConfigEx
  ===========================================================================*/
/*!
  @brief
  Sets the WLAN mode, Primary AP configuration and guest ap access profile

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
boolean QCMAP_Client::SetWLANConfigEx2
(
  qcmap_wlan_ex2_config    wlan_ex_config,
  qmi_error_type_v01     *qmi_err_num
)
{
    if ( !(this->mobile_ap_handle > 0 && this->m_qcmap_msgr_enable == true) )
    {
      *qmi_err_num = QMI_ERR_INVALID_HANDLE_V01;
      return false;
    }
   return QcMapLanClient->SetWLANConfigEx(wlan_ex_config, qmi_err_num);
}


boolean QCMAP_Client::SetWLANConfigEx
(
  qcmap_wlan_ex_config    wlan_ex_config,
  qmi_error_type_v01     *qmi_err_num
)
{
  /* for OWRT this function is overloaded with 3 new guest ap count param*/
   qcmap_wlan_ex2_config    ex2_config;
   memcpy(&ex2_config, &wlan_ex_config, sizeof(qcmap_wlan_ex_config));
   ex2_config.guestap_count_2g = 0;
   ex2_config.guestap_count_5g = 0;
   ex2_config.guestap_count_6g = 0;

   return QcMapLanClient->SetWLANConfigEx(ex2_config, qmi_err_num);
}

/*===========================================================================
  FUNCTION SetWLANConfigEx3
  ===========================================================================*/
/*!
  @brief
  Sets the WLAN mode, Primary AP configuration and guest ap access profile

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
boolean QCMAP_Client::SetWLANConfigEx3
(
  qcmap_wlan_ex3_config_t    wlan_ex3_config,
  qmi_error_type_v01     *qmi_err_num
)
{
   if ( !(this->mobile_ap_handle > 0 && this->m_qcmap_msgr_enable == true) )
   {
      *qmi_err_num = QMI_ERR_INVALID_HANDLE_V01;
      return false;
   }
   return QcMapLanClient->SetWLANConfigEx3(wlan_ex3_config, qmi_err_num);
}


/*===========================================================================
  FUNCTION GetWLANConfigEx3
  ===========================================================================*/
/*!
  @brief
  Sets the WLAN mode, Primary AP configuration and guest ap access profile

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
boolean QCMAP_Client::GetWLANConfigEx3
(
  qcmap_wlan_ex3_config_t*    wlan_ex3_config,
  qmi_error_type_v01     *qmi_err_num
)
{
   if ( !(this->mobile_ap_handle > 0 && this->m_qcmap_msgr_enable == true) )
   {
      *qmi_err_num = QMI_ERR_INVALID_HANDLE_V01;
      return false;
   }
   return QcMapLanClient->GetWLANConfigEx3(wlan_ex3_config, qmi_err_num);
}


/*===========================================================================
  FUNCTION GetWLANStatus
  ===========================================================================*/
/*!
  @brief
  Gets the current mode in which WLAN is brought up.

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
boolean QCMAP_Client::GetWLANStatus
(
  qcmap_msgr_wlan_mode_enum_v01 *wlan_mode,
  qmi_error_type_v01 *qmi_err_num
)
{
  return QcMapLanClient->GetWLANStatus(wlan_mode, qmi_err_num);
}

/*===========================================================================
  FUNCTION GetDMZ
  ===========================================================================*/
/*!
  @brief
  Gets a DMZ entry

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
boolean QCMAP_Client::GetDMZ(uint32_t *dmz_ip, qmi_error_type_v01 *qmi_err_num)
{

   char dmz_ip_owrt[QCMAP_IPV4_ADDR_LEN]={0};
   uint32_t current_profile_handle=0;
   if (!this->GetWWANProfilePreference(&current_profile_handle, qmi_err_num))
   {
     printf("Error getting current profile handle 0x%x.\n ", qmi_err_num);
     return false;
   }
   if (QcMapLanClient->GetDMZ(current_profile_handle, dmz_ip_owrt, qmi_err_num))
   {
     if(strlen(dmz_ip_owrt) == 0)
     {
       *dmz_ip = 0;
       return true;
     }
     else
     {
        /* need to add conversion from ip address to integer */
        *dmz_ip = ntohl(inet_addr(dmz_ip_owrt));
        return true;
     }
   }
   else
     return false;

}


/*===========================================================================
  FUNCTION DeleteDMZ
  ===========================================================================*/
/*!
  @brief
  Deletes a DMZ entry

  @return
  true  - on Success
  false - on Failure

  @note

  - Dependencies
  - macsec iface name in <qcmap_msgr_macsec_nic_config_v01>
    doesnt take impact as iface names will be dynamically fetched now.
    To see the mapped interfaces, please use getEthernetNicConfig API

  - Side Effects
  - None
 */
/*=========================================================================*/
boolean QCMAP_Client::DeleteDMZ(qmi_error_type_v01 *qmi_err_num)
{
    uint32_t current_profile_handle=0;
    if ( !(this->mobile_ap_handle > 0 && this->m_qcmap_msgr_enable == true) )
    {
      *qmi_err_num = QMI_ERR_INVALID_HANDLE_V01;
      return false;
    }
    if (!this->GetWWANProfilePreference(&current_profile_handle, qmi_err_num))
    {
      printf("Error getting current profile handle 0x%x.\n ", *qmi_err_num);
      return false;
    }
    if (QcMapLanClient->DeleteDMZ(current_profile_handle, qmi_err_num))
    {
       return true;
    }
    else
    {
       LOG_MSG_ERROR("DeleteDMZ Failed , Error: 0x%x", *qmi_err_num,0,0);
       return false;
    }
}

/*===========================================================================
  FUNCTION AddDMZ
  ===========================================================================*/
/*!
  @brief
  Adds a DMZ entry

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
boolean QCMAP_Client::AddDMZ(uint32 dmz_ip, qmi_error_type_v01 *qmi_err_num)
{
    char  scan_string[50];
    char dmz_ip_owrt[QCMAP_IPV4_ADDR_LEN];
    struct in_addr addr;
    addr.s_addr = htonl(dmz_ip);
    char * ip_addr = inet_ntoa(addr);
    strlcpy(dmz_ip_owrt, ip_addr, sizeof(dmz_ip_owrt));

    uint32_t current_profile_handle=0;
    if ( !(this->mobile_ap_handle > 0 && this->m_qcmap_msgr_enable == true) )
    {
      *qmi_err_num = QMI_ERR_INVALID_HANDLE_V01;
      return false;
    }
    if (!this->GetWWANProfilePreference(&current_profile_handle, qmi_err_num))
    {
      printf("Error getting current profile handle 0x%x.\n ", *qmi_err_num);
      return false;
    }
    if (QcMapLanClient->AddDMZ(current_profile_handle, dmz_ip_owrt, qmi_err_num))
    {
      return true;
    }
    else
    {
      if (*qmi_err_num == QMI_ERR_GENERAL_V01)
      {
         return false;
      }
      else
      {
        LOG_MSG_ERROR("AddDMZ Failed , Error: 0x%x", *qmi_err_num,0,0);
        return false;
      }
    }
}

/*===========================================================================
  FUNCTION EnableAlg
  ===========================================================================*/
/*!
  @brief
  Enables ALGs Functionality.

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
boolean QCMAP_Client::EnableAlg
(
  qcmap_msgr_alg_type_mask_v01  alg_types,
  qmi_error_type_v01           *qmi_err_num
)
{
  profile_handle_type_v01 current_profile_handle;
  if (!GetWWANProfilePreference(&current_profile_handle, qmi_err_num))
  {
    LOG_MSG_ERROR("Error getting current profile handle 0x%x.\n ", qmi_err_num,0,0);
    return false;
  }

  boolean ret = QcMapLanClient->EnableAlg(current_profile_handle, alg_types, qmi_err_num);
  LOG_MSG_INFO1("Successfully enable ALG",0,0,0);

  return ret;
}

/*===========================================================================
  FUNCTION DisableAlg
  ===========================================================================*/
/*!
  @brief
  Disables ALGs Functionality.

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
boolean QCMAP_Client::DisableAlg
(
  qcmap_msgr_alg_type_mask_v01  alg_types,
  qmi_error_type_v01           *qmi_err_num
)
{
  profile_handle_type_v01 current_profile_handle;
  if (!GetWWANProfilePreference(&current_profile_handle, qmi_err_num))
  {
    LOG_MSG_ERROR("Error getting current profile handle 0x%x.\n ", qmi_err_num,0,0);
    return false;
  }

  boolean ret = QcMapLanClient->DisableAlg(current_profile_handle, alg_types, qmi_err_num);
  LOG_MSG_INFO1("Successfully disable ALG",0,0,0);

  return ret;
}

/*=============================================================================
  FUNCTION SetSIPServerInfo
==============================================================================*/
/*!
  @brief
  - Populates the necessary fields in the QMI_QCMAP_MSGR_SET_SIP_SERVER_INFO_REQ
    message
  - Sends a QMI message to QCMAP server to set the SIP server information

  @return
  true  - on Success
  false - on Failure

  @note

  - Dependencies
  - None

  - Side Effects
  - None
 */
/*=============================================================================*/
boolean QCMAP_Client::SetSIPServerInfo
(
  qcmap_msgr_sip_server_info_v01  *sip_server_info,
  qmi_error_type_v01              *qmi_err_num
)
{
  if ( !(this->mobile_ap_handle > 0 && this->m_qcmap_msgr_enable == true) )
  {
    *qmi_err_num = QMI_ERR_INVALID_HANDLE_V01;
    return false;
  }
  boolean ret = QcMapLanClient->SetSIPServerInfo(sip_server_info, qmi_err_num);
  LOG_MSG_INFO1("Successfully Set SIP server Info",0,0,0);

  return ret;
}

/*===========================================================================
  FUNCTION SetLANConfig
  ===========================================================================*/
/*!
  @brief
  Sets the LAN mode.

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
boolean QCMAP_Client::SetLANConfig(qcmap_msgr_lan_config_v01 lan_config,
                                             qmi_error_type_v01 *qmi_err_num)
{

    if ( !(this->mobile_ap_handle > 0 && this->m_qcmap_msgr_enable == true) )
    {
      *qmi_err_num = QMI_ERR_INVALID_HANDLE_V01;
      return false;
    }
    qcmap_lan_config lan_config_owrt;
    memset(&lan_config_owrt, 0, sizeof(qcmap_lan_config));

    lan_config_owrt.gw_ip = lan_config.gw_ip;
    lan_config_owrt.netmask = lan_config.netmask;
    lan_config_owrt.enable_dhcp = lan_config.enable_dhcp;
    lan_config_owrt.dhcp_config.dhcp_start_ip = lan_config.dhcp_config.dhcp_start_ip;
    lan_config_owrt.dhcp_config.dhcp_end_ip = lan_config.dhcp_config.dhcp_end_ip;
    lan_config_owrt.dhcp_config.lease_time = lan_config.dhcp_config.lease_time;

    if (QcMapLanClient->SetLANConfig(&lan_config_owrt, qmi_err_num))
    {
      *qmi_err_num = QMI_ERR_NONE_V01;
       return true;
    }
    else
    {
       return false;
    }
}

/*===========================================================================
  FUNCTION GetLANConfig
  ===========================================================================*/
/*!
  @brief
  Gets the current configured Lan Configuration.

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
boolean QCMAP_Client::GetLANConfig
(
  qcmap_msgr_lan_config_v01 *lan_config,
  qmi_error_type_v01 *qmi_err_num
)
{
    qcmap_lan_config lan_config_lan;
    memset(&lan_config_lan,0,sizeof(qcmap_lan_config));

    if(QcMapLanClient->GetLANConfig(&lan_config_lan, qmi_err_num))
    {
       lan_config->gw_ip = lan_config_lan.gw_ip;
       lan_config->netmask = lan_config_lan.netmask;
       lan_config->enable_dhcp =lan_config_lan.enable_dhcp;
       memcpy(&(lan_config->dhcp_config), &(lan_config_lan.dhcp_config), sizeof(lan_config_lan.dhcp_config));

       return true;
    }
    else
    {
       return false;
    }

}


/*===========================================================================
  FUNCTION ActivateLAN
  ===========================================================================*/
/*!
  @brief
  Activates the LAN interface with the current available config.

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
boolean QCMAP_Client::ActivateLAN
(
  qmi_error_type_v01 *qmi_err_num
)
{
    if ( !(this->mobile_ap_handle > 0 && this->m_qcmap_msgr_enable == true) )
    {
      *qmi_err_num = QMI_ERR_INVALID_HANDLE_V01;
      return false;
    }
    profile_handle_type_v01 current_profile_handle;
    if (qmi_err_num == NULL)
    {
      LOG_MSG_ERROR("NULL pointer if qmi_err_num passed", 0, 0, 0);
      return false;
    }

    if (!this->GetWWANProfilePreference(&current_profile_handle, qmi_err_num))
    {
      printf("Error getting current profile handle 0x%x.\n ", *qmi_err_num);
      return;
    }
    return QcMapLanClient->ActivateLAN(current_profile_handle, qmi_err_num);
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
boolean QCMAP_Client::AddDHCPReservRecord
(
  qcmap_msgr_dhcp_reservation_v01  *dhcp_reserv_record,
  qmi_error_type_v01               *qmi_err_num
)
{
    in_addr addr;
    memset(&addr, 0 ,sizeof(in_addr));
    uint32_t default_handle = 0;
    uint32_t profile_handle = 0;

    if ( !(this->mobile_ap_handle > 0 && this->m_qcmap_msgr_enable == true) )
    {
      *qmi_err_num = QMI_ERR_INVALID_HANDLE_V01;
      return false;
    }

    qcmap_dhcp_reservation dhcp_reserv_record_owrt;
    memset(&dhcp_reserv_record_owrt,0,sizeof(qcmap_dhcp_reservation));

    memcpy(dhcp_reserv_record_owrt.client_mac_addr, dhcp_reserv_record->client_mac_addr, sizeof(dhcp_reserv_record->client_mac_addr));

    strlcpy(dhcp_reserv_record_owrt.client_device_name, dhcp_reserv_record->client_device_name, sizeof(dhcp_reserv_record_owrt.client_device_name));
    dhcp_reserv_record_owrt.enable_reservation = dhcp_reserv_record->enable_reservation;

    dhcp_reserv_record_owrt.client_reserved_ip = dhcp_reserv_record->client_reserved_ip;

    /* get Current BH and default BH info */
    if (!GetWWANInfo(&default_handle, &profile_handle, this))
    {
      printf("Failed to retrieve current profile handle\n");
      return false;
    }

    if (!(QcMapLanClient->CheckIPPTState(&dhcp_reserv_record_owrt, (const uint32_t)profile_handle)))
    {
      printf("\nFor the IPPT enabled Client adding DHCP reservation is not allowed");
      return false;
    }

    if( QcMapLanClient->AddDHCPReservRecord(&dhcp_reserv_record_owrt, qmi_err_num) )
    {
        return true;
    }
    else
    {
       LOG_MSG_ERROR("Failed to add DHCP Reservation record, Error: 0x%x", *qmi_err_num,0,0);
       return false;
    }

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
boolean QCMAP_Client::GetDHCPReservRecords
(
  qcmap_msgr_dhcp_reservation_v01  *dhcp_reserv_records,
  uint32_t                         *num_entries1,
  qmi_error_type_v01               *qmi_err_num
)
{
  uint32_t num_entries =0, i=0;
  in_addr tmpIP;
  uint8_t mac_addr_int[QCMAP_MSGR_MAC_ADDR_LEN_V01] = {0};
  char mac_addr_str[QCMAP_MSGR_MAC_ADDR_NUM_CHARS_V01]; /*char array of mac address*/
  qcmap_dhcp_reservation dhcp_reserv_record[QCMAP_MAX_DHCP_RESERVATION_ENTRIES];
  qcmap_msgr_dhcp_reservation_v01 *reserv_rec_tmp = NULL;
  memset(dhcp_reserv_record,0,QCMAP_MAX_DHCP_RESERVATION_ENTRIES*sizeof(qcmap_dhcp_reservation));

  if( QcMapLanClient->GetDHCPReservRecords(dhcp_reserv_record, &num_entries, qmi_err_num) )
  {
    *num_entries1 = num_entries;

    for ( i = 0;i < num_entries; i++)
    {
      reserv_rec_tmp = NULL;
      reserv_rec_tmp = &dhcp_reserv_records[i];

      if(ds_mac_addr_pton(dhcp_reserv_record[i].mac_addr_string, mac_addr_int))
      {
        for (int j = 0; j < QCMAP_MSGR_MAC_ADDR_LEN_V01; j++)
        {
          reserv_rec_tmp->client_mac_addr[j] = mac_addr_int[j];
        }
      }

      reserv_rec_tmp->enable_reservation = dhcp_reserv_record[i].enable_reservation;
      if (inet_pton(AF_INET, dhcp_reserv_record[i].reserved_ip_string, &tmpIP) == 1)
      {
        reserv_rec_tmp->client_reserved_ip = ntohl(tmpIP.s_addr);
      }

      dhcp_reserv_records[i].enable_reservation = dhcp_reserv_record[i].enable_reservation;
      strlcpy(reserv_rec_tmp->client_device_name, dhcp_reserv_record[i].client_device_name, sizeof(reserv_rec_tmp->client_device_name));
    }
    return true;
  }
  else
  {
    LOG_MSG_ERROR("Failed to Display DHCP Reservation record , Error: 0x%x", *qmi_err_num,0,0);
    return false;
  }
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
boolean QCMAP_Client::EditDHCPReservRecord
(
  uint32_t                         *addr,
  qcmap_msgr_dhcp_reservation_v01  *dhcp_reserv_record,
  qmi_error_type_v01               *qmi_err_num
)
{
  in_addr addr_lan;
  memset(&addr_lan, 0 ,sizeof(in_addr));
  uint8 mac_addr_int[QCMAP_MSGR_MAC_ADDR_LEN_V01];
  char  scan_string[50];
  qcmap_dhcp_reservation dhcp_reserv_record_owrt;
  memset(&dhcp_reserv_record_owrt,0,sizeof(qcmap_dhcp_reservation));

  if ( !(this->mobile_ap_handle > 0 && this->m_qcmap_msgr_enable == true) )
  {
    *qmi_err_num = QMI_ERR_INVALID_HANDLE_V01;
     return false;
  }

  memcpy(dhcp_reserv_record_owrt.client_mac_addr,dhcp_reserv_record->client_mac_addr, sizeof(dhcp_reserv_record->client_mac_addr));
  memcpy(dhcp_reserv_record_owrt.client_device_name,dhcp_reserv_record->client_device_name, sizeof(dhcp_reserv_record->client_device_name));
  dhcp_reserv_record_owrt.enable_reservation = dhcp_reserv_record->enable_reservation;

  dhcp_reserv_record_owrt.client_reserved_ip = dhcp_reserv_record->client_reserved_ip;

  if( QcMapLanClient->EditDHCPReservRecord( addr, &dhcp_reserv_record_owrt, qmi_err_num) )
  {
     return true;
  }
  else
  {
    LOG_MSG_ERROR("Failed to edit DHCP Reservation record , Error: 0x%x", *qmi_err_num,0,0);
    return false;
  }
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
boolean QCMAP_Client::DeleteDHCPReservRecord
(
  uint32_t             *addr,
  qmi_error_type_v01   *qmi_err_num
)
{

  if( QcMapLanClient->DeleteDHCPReservRecord(addr, qmi_err_num) )
  {
      return true;
  }
  else
  {
     LOG_MSG_ERROR("Failed to delete DHCP Reservation record , Error: 0x%x", *qmi_err_num,0,0);
     return false;
  }

}

/*===========================================================================
FUNCTION CreateVLANConfig()
===========================================================================*/
/** @ingroup section_vlan_config

  Creates the VLAN configuration.

  @datatypes
  qcmap_msgr_vlan_config_ex_v01 \n
  qmi_error_type_v01

  @param[in]  vlan_config       Sets the VLAN configuration.
  @param[out] qmi_err_num       Pointer to the error code returned by the
                                server.
  @param[out] is_accelerated    Is Accelerated returned by server

  @return
  TRUE -- Success. \n
  FALSE -- Failure.

  @dependencies
  None. @newpage
*/
/*=========================================================================*/
boolean QCMAP_Client::CreateVLANConfig
(
  qcmap_msgr_vlan_config_ex_v01     vlan_config,
  qmi_error_type_v01               *qmi_err_num,
  bool                             *is_accelerated
)
{
    if ( !(this->mobile_ap_handle > 0 && this->m_qcmap_msgr_enable == true) )
    {
      *qmi_err_num = QMI_ERR_INVALID_HANDLE_V01;
      return false;
    }
    qcmap_lan_vlan_conf_t vlan_config_ex;

    vlan_config_ex.intf_type = (qcmap_interface_type_enum)vlan_config.intf_type;
    vlan_config_ex.is_accelerated = vlan_config.is_accelerated;
    vlan_config_ex.vlan_id = vlan_config.vlan_id;

   // memcpy(vlan_config_ex.phy_iface_name, vlan_config., sizeof(vlan_config.phy_iface_name));

    return QcMapLanClient->CreateVLANConfig(vlan_config_ex, qmi_err_num, is_accelerated);
}

/*===========================================================================
  FUNCTION GetVlanConfig()
===========================================================================*/
/** @ingroup qcmap_show_vlan_config

  Show VLAN upon getting request from QCMAP.

  @return
  TRUE -- Success \n
  FALSE -- Failure \n
*/
/*=========================================================================*/
boolean QCMAP_Client::GetVLANConfig
(
  qcmap_msgr_vlan_conf_t *vlan_config,
  qmi_error_type_v01 *qmi_err_num
)
{
  qcmap_vlan_conf_t vlan_config_lan;
  memset(&vlan_config_lan, 0, sizeof(qcmap_vlan_conf_t));

  if(QcMapLanClient->GetVlanConfig(&vlan_config_lan, qmi_err_num))
  {

     vlan_config->vlan_config_list_len = vlan_config_lan.vlan_config_list_len;

     for(int i = 0; i< vlan_config_lan.vlan_config_list_len ; i++)
     {
         vlan_config->vlan_config_list_ex[i].intf_type =  (qcmap_msgr_interface_type_enum_v01)vlan_config_lan.vlan_config_list_ex[i].intf_type;
         vlan_config->vlan_config_list_ex[i].is_accelerated = (qcmap_msgr_interface_type_enum_v01)vlan_config_lan.vlan_config_list_ex[i].is_accelerated;
         vlan_config->vlan_config_list_ex[i].vlan_id = (qcmap_msgr_interface_type_enum_v01)vlan_config_lan.vlan_config_list_ex[i].vlan_id;
     }

     return true;

  }
  else
  {

     LOG_MSG_ERROR("Failed to GetVlanConfig , Error: 0x%x", *qmi_err_num,0,0);
     return false;
  }
}
/*===========================================================================
  FUNCTION DeleteVLANConfig
  ===========================================================================*/
/*!
  @brief
  Deletes VLAN Config

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
boolean QCMAP_Client::DeleteVLANConfig
(
  int16_t                             vlan_id,
  qcmap_msgr_interface_type_enum_v01  intf_type,
  qmi_error_type_v01                 *qmi_err_num
)

{
    qcmap_lan_vlan_conf_t vlan_config_ex;

    vlan_config_ex.intf_type = (qcmap_interface_type_enum)intf_type;
    vlan_config_ex.vlan_id = vlan_id;

    return QcMapLanClient->DeleteVlanConfig(vlan_config_ex, qmi_err_num);

}

/*===========================================================================
  FUNCTION SetNatType
  ===========================================================================*/
/*!
  @brief
  Enables the NAT type
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
boolean QCMAP_Client::SetNatType
(
  qcmap_msgr_nat_enum_v01  nat_type,
  qmi_error_type_v01      *qmi_err_num
)
{
    /*
       Handle print statements, and return types and the struct value passed.
    */
    if ( !(this->mobile_ap_handle > 0 && this->m_qcmap_msgr_enable == true) )
    {
      *qmi_err_num = QMI_ERR_INVALID_HANDLE_V01;
      return false;
    }

    profile_handle_type_v01 current_profile_handle;
    if (!this->GetWWANProfilePreference(&current_profile_handle, qmi_err_num))
    {
      printf("Error getting current profile handle 0x%x.\n ", *qmi_err_num);
      return false;
    }
    switch (nat_type)
    {
       case NAT_SYMMETRIC:
          QcMapLanClient->SetNatType(current_profile_handle,SYMMETRIC_NAT, qmi_err_num);
          break;
       case NAT_PORT_RESTRICTED_CONE:
          QcMapLanClient->SetNatType(current_profile_handle,PORT_RESTRICTED_CONE_NAT, qmi_err_num);
          break;
       case NAT_FULL_CONE:
          QcMapLanClient->SetNatType(current_profile_handle,FULL_CONE_NAT, qmi_err_num);
          break;
       case NAT_ADDRESS_RESTRICTED_CONE:
          QcMapLanClient->SetNatType(current_profile_handle,ADDRESS_RESTRICTED_CONE_NAT, qmi_err_num);
          break;
    }
    return true;


}

/*===========================================================================
  FUNCTION GetNatType
  ===========================================================================*/
/*!
  @brief
  Gets the NAT type enabled

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
boolean QCMAP_Client::GetNatType
(
  qcmap_msgr_nat_enum_v01  *nat_type,
  qmi_error_type_v01       *qmi_err_num
)
{
       profile_handle_type_v01 current_profile_handle;
       char output[QCMAP_MAX_SCAN_SIZE]={0};
       if ( !(this->mobile_ap_handle > 0 && this->m_qcmap_msgr_enable == true) )
       {
         *qmi_err_num = QMI_ERR_INVALID_HANDLE_V01;
         return false;
       }
       if(!this->GetWWANProfilePreference(&current_profile_handle, qmi_err_num))
       {
         printf("Error getting current profile handle 0x%x.\n ", *qmi_err_num);
         return false;
       }
       bool ret_val=QcMapLanClient->GetNatType(current_profile_handle,output, qmi_err_num);
       if(ret_val)
       {
           if(!strncmp(output,SYMMETRIC_NAT,sizeof(SYMMETRIC_NAT)))
           {
              *nat_type = QCMAP_MSGR_NAT_SYMMETRIC_NAT_V01;
           }
           else if(!strncmp(output,PORT_RESTRICTED_CONE_NAT,sizeof(PORT_RESTRICTED_CONE_NAT)))
           {
              *nat_type = QCMAP_MSGR_NAT_PORT_RESTRICTED_CONE_NAT_V01;
           }
           else if(!strncmp(output,FULL_CONE_NAT,sizeof(FULL_CONE_NAT)))
           {
              *nat_type = QCMAP_MSGR_NAT_FULL_CONE_NAT_V01;
           }
           else if(!strncmp(output,ADDRESS_RESTRICTED_CONE_NAT,sizeof(ADDRESS_RESTRICTED_CONE_NAT)))
           {
              *nat_type = QCMAP_MSGR_NAT_ADDRESS_RESTRICTED_NAT_V01;
           }
           else
           {
              printf("Invalid NAT Type");
           }
          return true;
       }
       else
       {
         LOG_MSG_ERROR("Failed to GetNatType , Error: 0x%x", *qmi_err_num,0,0);
         return false;
       }

}


/*===========================================================================
  FUNCTION SetNatTimeout
  ===========================================================================*/
/*!
  @brief
  Sets the timeout for the corresponding timeout type.

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
boolean QCMAP_Client::SetNatTimeout
(
  qcmap_msgr_nat_timeout_enum_v01 timeout_type,
  uint32                          timeout_value,
  qmi_error_type_v01             *qmi_err_num
)
{
    qcmap_nat_timeout_enum timeout_type_lan;
    timeout_type_lan = (qcmap_nat_timeout_enum)timeout_type;

    if ( !(this->mobile_ap_handle > 0 && this->m_qcmap_msgr_enable == true) )
    {
      *qmi_err_num = QMI_ERR_INVALID_HANDLE_V01;
      return false;
    }

    if (QcMapLanClient->SetNatTimeout(timeout_type_lan, (uint32)timeout_value, qmi_err_num ))
    {
       return true;
    }
    else
    {
      LOG_MSG_ERROR("NAT timeout set fails , Error: 0x%x", *qmi_err_num,0,0);
      return false;
    }
}


/*===========================================================================
  FUNCTION GetNatTimeout
  ===========================================================================*/
/*!
  @brief
  Gets the timeout for the corresponding timeout type.

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
boolean QCMAP_Client::GetNatTimeout
(
  qcmap_msgr_nat_timeout_enum_v01 timeout_type,
  uint32                         *timeout_value,
  qmi_error_type_v01             *qmi_err_num
)
{
    qcmap_nat_timeout_enum timeout_type_lan = (qcmap_nat_timeout_enum)timeout_type;

    if (QcMapLanClient->GetNatTimeout(timeout_type_lan, timeout_value, qmi_err_num))
    {
       timeout_type =  (qcmap_msgr_nat_timeout_enum_v01)timeout_type_lan;
       return true;
    }
    else
    {
      LOG_MSG_ERROR("NAT timeout get fails , Error: 0x%x", *qmi_err_num,0,0);
      return false;
    }

}



/*===========================================================================
  FUNCTION GetIPSECVpnPassthrough
  ===========================================================================*/
/*!
  @brief
  Will get the IpsecVpn Pass through mode

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
boolean QCMAP_Client::GetIPSECVpnPassthrough(boolean *enable, qmi_error_type_v01 *qmi_err_num)
{

    qcmap_lan_ipsec_vpn_passthrough_mode_enum enable_state;

    profile_handle_type_v01 current_profile_handle;

    if (!this->GetWWANProfilePreference(&current_profile_handle, qmi_err_num))
    {
       printf("Error getting current profile handle 0x%x.\n ", qmi_err_num);
       return false;
    }

    if (QcMapLanClient->GetIPSECVpnPassthrough(&enable_state, (const uint32_t)current_profile_handle, qmi_err_num))
    {
      if (enable_state == QCMAP_LAN_IPSEC_VPN_PASSTHROUGH_MODE_UP)
      {
        *enable = true;
      }
      else
      {
        *enable = false;
      }
      return true;
    }
    else
    {
      return false;
    }


}

/*===========================================================================
  FUNCTION SetIPSECVpnPassthrough
  ===========================================================================*/
/*!
  @brief
  Will enable the IpsecVpn Pass through mode

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
boolean QCMAP_Client::SetIPSECVpnPassthrough(boolean enable, qmi_error_type_v01 *qmi_err_num)
{

    qcmap_lan_ipsec_vpn_passthrough_mode_enum enable_state;
    uint32_t default_handle = 0;
    uint32_t profile_handle = 0;

    if ( !(this->mobile_ap_handle > 0 && this->m_qcmap_msgr_enable == true) )
    {
      *qmi_err_num = QMI_ERR_INVALID_HANDLE_V01;
      return false;
    }

    if(enable)
    {
       enable_state = QCMAP_LAN_IPSEC_VPN_PASSTHROUGH_MODE_UP;
    }
    else
    {
       enable_state = QCMAP_LAN_IPSEC_VPN_PASSTHROUGH_MODE_DOWN;
    }

        /* Get current BH and default BH info */
    if (!GetWWANInfo(&default_handle, &profile_handle, this))
    {
      printf("Failed to retrieve current profile handle\n");
      return false;
    }

    if (QcMapLanClient->SetIPSECVpnPassthrough(enable_state,
                                               (const uint32_t)default_handle,
                                               (const uint32_t)profile_handle, qmi_err_num))
    {
      return true;
    }
    else
    {
      /* if passthrough is already set/reset and still trying to set/reset */
      if(*qmi_err_num == QMI_ERR_NO_EFFECT_V01)
      {
         return true;
      }
      return false;
    }

}

/*===========================================================================
  FUNCTION SetIPSECVpnPassthroughv6
  ===========================================================================*/
/*!
  @brief
  Will enable the IpsecVpn Pass through mode for IPv6

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
boolean QCMAP_Client::SetIPSECVpnPassthrough_Ipv6
(
  boolean              enable,
  qmi_error_type_v01  *qmi_err_num
)
{
    qcmap_lan_ipsec_vpn_passthrough_v6_mode_enum enable_state;

    uint32_t default_handle = 0;
    uint32_t profile_handle = 0;
    if ( !(this->mobile_ap_handle > 0 && this->m_qcmap_msgr_enable == true) )
    {
      *qmi_err_num = QMI_ERR_INVALID_HANDLE_V01;
      return false;
    }
    if(enable)
    {
       enable_state = QCMAP_LAN_IPSEC_VPN_PASSTHROUGH_V6_MODE_UP;
    }
    else
    {
       enable_state = QCMAP_LAN_IPSEC_VPN_PASSTHROUGH_V6_MODE_DOWN;
    }

    /* Get current BH and default BH info */
    if (!GetWWANInfo(&default_handle, &profile_handle, this))
    {
      printf("Failed to retrieve current profile handle\n");
      return false ;
    }

    if (QcMapLanClient->SetIPSECVpnPassthroughIpv6(enable_state,
                                               (const uint32_t)default_handle,
                                               (const uint32_t)profile_handle, qmi_err_num))
    {
      return true;
    }
    else
    {
      /* if passthrough is already set/reset and still trying to set/reset */
      if(*qmi_err_num == QMI_ERR_NO_EFFECT_V01)
      {
         return true;
      }
      return false;
    }
}


/*===========================================================================
  FUNCTION GetIPSECVpnPassthrough_Ipv6
  ===========================================================================*/
/*!
  @brief
  Will get the IpsecVpn Pass through mode for IPv6

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
boolean QCMAP_Client::GetIPSECVpnPassthrough_Ipv6
(
  boolean             *enable,
  qmi_error_type_v01  *qmi_err_num
)
{

    qcmap_lan_ipsec_vpn_passthrough_v6_mode_enum enable_state;
    uint32_t default_handle = 0;
    uint32_t profile_handle = 0;

        /* get Current BH and default BH info */
    if (!GetWWANInfo(&default_handle, &profile_handle, this))
    {
      printf("Failed to retrieve current profile handle\n");
      return false;
    }

    if (QcMapLanClient->GetIPSECVpnPassthroughIpv6(&enable_state, (const uint32_t)profile_handle, qmi_err_num))
    {
      if (enable_state == QCMAP_LAN_IPSEC_VPN_PASSTHROUGH_V6_MODE_UP)
      {
        *enable = 1;
      }
      else
      {
        *enable = 0;
      }
      return true;
    }
    else
    {
      return false;
    }
}

/*===========================================================================
  FUNCTION SetPPTPVpnPassthrough_Ipv6
  ===========================================================================*/
/*!
  @brief
  Will enable the peer to peer vpn Pass through mode for IPv6
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
boolean QCMAP_Client::SetPPTPVpnPassthrough_Ipv6
(
  boolean               enable,
  qmi_error_type_v01   *qmi_err_num
)
{

    qcmap_lan_pptp_vpn_passthrough_v6_mode_enum enable_state;
    uint32_t default_handle = 0;
    uint32_t profile_handle = 0;
    if ( !(this->mobile_ap_handle > 0 && this->m_qcmap_msgr_enable == true) )
    {
      *qmi_err_num = QMI_ERR_INVALID_HANDLE_V01;
      return false;
    }

    if(enable)
    {
       enable_state = QCMAP_LAN_PPTP_VPN_PASSTHROUGH_V6_MODE_UP;
    }
    else
    {
       enable_state = QCMAP_LAN_PPTP_VPN_PASSTHROUGH_V6_MODE_DOWN;
    }

    /* Get current BH and default BH info */
    if (!GetWWANInfo(&default_handle, &profile_handle, this))
    {
      printf("Failed to retrieve current profile handle\n");
      return;
    }

    if (QcMapLanClient->SetPPTPVpnPassthroughIpv6(enable_state,
                                               (const uint32_t)default_handle,
                                               (const uint32_t)profile_handle, qmi_err_num))
    {
      return true;
    }
    else
    {
     /* if passthrough is already set/reset and still trying to set/reset */
      if(*qmi_err_num == QMI_ERR_NO_EFFECT_V01)
      {
         return true;
      }
      return false;
    }
}

/*===========================================================================
  FUNCTION GetPPTPVpnPassthrough_Ipv6
  ===========================================================================*/
/*!
  @brief
  Will get the peer to peer vpn Pass through mode set for IPv6
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
boolean QCMAP_Client::GetPPTPVpnPassthrough_Ipv6
(
  boolean             *enable,
  qmi_error_type_v01  *qmi_err_num
)
{

    qcmap_lan_pptp_vpn_passthrough_v6_mode_enum enable_state;
    uint32_t default_handle = 0;
    uint32_t profile_handle = 0;

    /* get Current BH and default BH info */
    if (!GetWWANInfo(&default_handle, &profile_handle, this))
    {
      printf("Failed to retrieve current profile handle\n");
      return false;
    }

    if (QcMapLanClient->GetPPTPVpnPassthroughIpv6(&enable_state, (const uint32_t)profile_handle, qmi_err_num))
    {
      if (enable_state == QCMAP_LAN_PPTP_VPN_PASSTHROUGH_V6_MODE_UP)
      {
        *enable = 1;
        return true;
      }
      else if (enable_state == QCMAP_LAN_PPTP_VPN_PASSTHROUGH_V6_MODE_DOWN)
      {
        *enable = 0;
        return true;
      }
      else
      {
        return false;
      }
    }
    else
    {
      return false;
    }
}

/*===========================================================================
  FUNCTION SetL2TPIPSECVpnPassthrough_Ipv6
  ===========================================================================*/
/*!
  @brief
  Will set the l2tp/ipsec vpn Pass through mode

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
boolean QCMAP_Client::SetL2TPIPSECVpnPassthrough_Ipv6
(
  boolean               enable,
  qmi_error_type_v01   *qmi_err_num
)
{
    qcmap_lan_l2tp_ipsec_vpn_passthrough_v6_mode_enum enable_state;
    uint32_t default_handle = 0;
    uint32_t profile_handle = 0;
    if ( !(this->mobile_ap_handle > 0 && this->m_qcmap_msgr_enable == true) )
    {
      *qmi_err_num = QMI_ERR_INVALID_HANDLE_V01;
      return false;
    }

    if(enable)
    {
       enable_state = QCMAP_LAN_L2TP_IPSEC_VPN_PASSTHROUGH_V6_MODE_UP;
    }
    else
    {
       enable_state = QCMAP_LAN_L2TP_IPSEC_VPN_PASSTHROUGH_V6_MODE_DOWN;
    }

    /* Get current BH and default BH info */
    if (!GetWWANInfo(&default_handle, &profile_handle, this))
    {
      printf("Failed to retrieve current profile handle\n");
      return false;
    }

    if (QcMapLanClient->SetL2TPIPSECVpnPassthroughIpv6(enable_state,
                                               (const uint32_t)default_handle,
                                               (const uint32_t)profile_handle, qmi_err_num))
    {
      return true;
    }
    else
    {
     /* if passthrough is already set/reset and still trying to set/reset */
      if(*qmi_err_num == QMI_ERR_NO_EFFECT_V01)
      {
         return true;
      }
      return false;
    }


}

/*===========================================================================
  FUNCTION GetL2TPVpnPassthrough_Ipv6
  ===========================================================================*/
/*!
  @brief
  Will get the Layer 2 Tunneling Protocol vpn Pass through mode for IPv6 is
  enabled or disabled

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
boolean QCMAP_Client::GetL2TPIPSECVpnPassthrough_Ipv6
(
  boolean              *enable,
  qmi_error_type_v01   *qmi_err_num
)
{

    qcmap_lan_l2tp_ipsec_vpn_passthrough_v6_mode_enum enable_state;
    uint32_t default_handle = 0;
    uint32_t profile_handle = 0;

    /* get Current BH and default BH info */
    if (!GetWWANInfo(&default_handle, &profile_handle, this))
    {
      printf("Failed to retrieve current profile handle\n");
      return false;
    }

    if (QcMapLanClient->GetL2TPIPSECVpnPassthroughIpv6(&enable_state, (const uint32_t)profile_handle, qmi_err_num))
    {
      if (enable_state == QCMAP_LAN_L2TP_IPSEC_VPN_PASSTHROUGH_V6_MODE_UP)
      {
        *enable = 1;
         return true;

      }
      else if (enable_state == QCMAP_LAN_L2TP_IPSEC_VPN_PASSTHROUGH_V6_MODE_DOWN)
      {
        *enable = 0;
         return true;

      }
      else
      {
        return false;
      }
    }
    else
    {
       return false;
    }
}


/*===========================================================================
  FUNCTION SetL2TPIPSECVpnPassthrough
  ===========================================================================*/
/*!
  @brief
  Will set the l2tp/ipsec vpn Pass through mode

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
boolean QCMAP_Client::SetL2TPIPSECVpnPassthrough(boolean enable, qmi_error_type_v01 *qmi_err_num)
{

    qcmap_lan_l2tp_ipsec_vpn_passthrough_mode_enum enable_state;
    uint32_t default_handle = 0;
    uint32_t profile_handle = 0;
    if ( !(this->mobile_ap_handle > 0 && this->m_qcmap_msgr_enable == true) )
    {
      *qmi_err_num = QMI_ERR_INVALID_HANDLE_V01;
      return false;
    }

    if(enable)
    {
       enable_state = QCMAP_LAN_L2TP_IPSEC_VPN_PASSTHROUGH_MODE_UP;
    }
    else
    {
       enable_state = QCMAP_LAN_L2TP_IPSEC_VPN_PASSTHROUGH_MODE_DOWN;
    }

    /* Get current BH and default BH info */
    if (!GetWWANInfo(&default_handle, &profile_handle, this))
    {
      printf("Failed to retrieve current profile handle\n");
      return false;
    }

    if (QcMapLanClient->SetL2TPIPSECVpnPassthrough(enable_state,
                                               (const uint32_t)default_handle,
                                               (const uint32_t)profile_handle, qmi_err_num))
    {
      return true;
    }
    else
    {
     /* if passthrough is already set/reset and still trying to set/reset */
      if(*qmi_err_num == QMI_ERR_NO_EFFECT_V01)
      {
         return true;
      }
      return false;
    }





}
/*===========================================================================
  FUNCTION SetPPTPVpnPassthrough
  ===========================================================================*/
/*!
  @brief
  Will enable the peer to peer vpn Pass through mode
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
boolean QCMAP_Client::SetPPTPVpnPassthrough(boolean enable, qmi_error_type_v01 *qmi_err_num)
{

    qcmap_lan_pptp_vpn_passthrough_mode_enum enable_state;
    uint32_t default_handle = 0;
    uint32_t profile_handle = 0;
    if ( !(this->mobile_ap_handle > 0 && this->m_qcmap_msgr_enable == true) )
    {
      *qmi_err_num = QMI_ERR_INVALID_HANDLE_V01;
      return false;
    }

   if(enable)
    {
       enable_state = QCMAP_LAN_PPTP_VPN_PASSTHROUGH_MODE_UP;
    }
    else
    {
       enable_state = QCMAP_LAN_PPTP_VPN_PASSTHROUGH_MODE_DOWN;
    }

    /* Get current BH and default BH info */
    if (!GetWWANInfo(&default_handle, &profile_handle, this))
    {
      printf("Failed to retrieve current profile handle\n");
      return false;
    }


    if (QcMapLanClient->SetPPTPVpnPassthrough(enable_state,
                                               (const uint32_t)default_handle,
                                               (const uint32_t)profile_handle, qmi_err_num))
    {
       return true;
    }
    else
    {
    /* if passthrough is already set/reset and still trying to set/reset */
      if(*qmi_err_num == QMI_ERR_NO_EFFECT_V01)
      {
         LOG_MSG_INFO1("debug log: ipsecvpnpassthrough ALREADY CASE",0,0,0);
         return true;
      }
       return false;
    }

}


/*===========================================================================
  FUNCTION GetPPTPVpnPassthrough
  ===========================================================================*/
/*!
  @brief
  Will get the peer to peer vpn Pass through mode set
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
boolean QCMAP_Client::GetPPTPVpnPassthrough(boolean *enable, qmi_error_type_v01 *qmi_err_num)
{

        qcmap_lan_pptp_vpn_passthrough_mode_enum enable_state;
        profile_handle_type_v01 current_profile_handle;
        if (!this->GetWWANProfilePreference(&current_profile_handle, qmi_err_num))
        {
          printf("Error getting current profile handle 0x%x.\n ", qmi_err_num);
          return false;
        }

        if (QcMapLanClient->GetPPTPVpnPassthrough(&enable_state, (const uint32_t)current_profile_handle, qmi_err_num))
        {
          if (enable_state == QCMAP_LAN_PPTP_VPN_PASSTHROUGH_MODE_UP)
          {
            *enable = true;
          }
          else
          {
            *enable = false;
          }
          return true;
        }
        else
        {
          return false;
        }

}


/*===========================================================================
  FUNCTION GetL2TPIPSECVpnPassthrough
  ===========================================================================*/
/*!
  @brief
  Will get the Layer 2 Tunneling Protocol vpn Pass through mode is enabled or
  disabled

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
boolean QCMAP_Client::GetL2TPIPSECVpnPassthrough(boolean *enable, qmi_error_type_v01 *qmi_err_num)
{

        qcmap_lan_l2tp_ipsec_vpn_passthrough_mode_enum enable_state;

        profile_handle_type_v01 current_profile_handle;
        if (!this->GetWWANProfilePreference(&current_profile_handle, qmi_err_num))
        {
          printf("Error getting current profile handle 0x%x.\n ", qmi_err_num);
          return false;
        }
        if (QcMapLanClient->GetL2TPIPSECVpnPassthrough(&enable_state, (const uint32_t)current_profile_handle, qmi_err_num))
        {
          if (enable_state == QCMAP_LAN_L2TP_IPSEC_VPN_PASSTHROUGH_MODE_UP)
            *enable = true;
          else
            *enable = false;

          return true;
        }
        else
        {
           return false;
        }

}



#ifdef FEATURE_PORT_TRIGGER
/*===========================================================================
  FUNCTION AddPortTriggerEntry
  ===========================================================================*/
/*!
  @brief
  API to add the port trigger entry.

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
bool QCMAP_Client::AddPortTriggerEntry
(
  qcmap_msgr_port_trigger_entry_conf_t   port_trigger_entry,
  int                                   *handle,
  qmi_error_type_v01                    *qmi_err_num
)
{
      uint32_t current_profile_handle=0;
      if (!this->GetWWANProfilePreference(&current_profile_handle, qmi_err_num))
      {
        printf("Error getting current profile handle 0x%x.\n ", qmi_err_num);
        return false;
      }
      if (QcMapLanClient->AddPortTriggerEntry(current_profile_handle, port_trigger_entry, handle, qmi_err_num))
      {
        return true;
      }
      else
      {
        return false;
      }
}

/*===========================================================================
  FUNCTION DeletePortTriggerEntry
  ===========================================================================*/
/*!
  @brief
  API to delete the port trigger entry.

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
bool QCMAP_Client::DeletePortTriggerEntry
(
  int                      handle,
  qmi_error_type_v01      *qmi_err_num
)
{

    uint32_t current_profile_handle=0;
    if (!this->GetWWANProfilePreference(&current_profile_handle, qmi_err_num))
    {
      printf("Error getting current profile handle 0x%x.\n ", *qmi_err_num);
      return false;
    }
    if (QcMapLanClient->DeletePortTriggerEntry(current_profile_handle, handle, qmi_err_num))
    {
      return true;
    }
    else
    {
      return false;
    }


}

/*===========================================================================
  FUNCTION GetPortTriggerEntry
  ===========================================================================*/
/*!
  @brief
  Gets the Port Trigger Entry Configuration from QCMAP connection manager of
  single entry or multiple entries.

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
bool QCMAP_Client::GetPortTriggerEntry
(
  int                             handle,
  qcmap_msgr_port_trigger_conf_t *port_trigger,
  qmi_error_type_v01             *qmi_err_num
)
{

    if(QcMapLanClient->GetPortTriggerEntry(port_trigger, handle, qmi_err_num))
    {
       /* give back the port list to the port_trigger*/
       return true;
    }
    else
    {
       return false;
    }

}

#endif

/*===========================================================================
  FUNCTION Get Station Mode status
  ===========================================================================*/
/*!
  @brief
  Gets Station Mode status

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
boolean QCMAP_Client::GetStationModeStatus
(
  qcmap_msgr_station_mode_status_enum_v01   *status,
  qmi_error_type_v01                        *qmi_err_num
)
{
  return QcMapLanClient->GetStationModeStatus(status, qmi_err_num);
}

/*===========================================================================
  FUNCTION ActivateHostapdConfig
  ===========================================================================*/
/*!
  @brief
  Activates the Hostapd with the current available config.

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
boolean QCMAP_Client::ActivateHostapdConfig
(
  qcmap_msgr_activate_hostapd_ap_enum_v01 ap_type,
  qcmap_msgr_activate_hostapd_action_enum_v01 action_type,
  qmi_error_type_v01 *qmi_err_num,
  boolean privileged_client
)
{
  return QcMapLanClient->ActivateHostapdConfig(ap_type, action_type, qmi_err_num);
}



/*===========================================================================
  FUNCTION ActivateSupplicantConfig
  ===========================================================================*/
/*!
  @brief
  Activates the Supplicant with the current available config.

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
boolean QCMAP_Client::ActivateSupplicantConfig
(
  qmi_error_type_v01 *qmi_err_num
)
{
  return QcMapLanClient->ActivateSupplicantConfig(qmi_err_num);
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
 */
/*=========================================================================*/
boolean QCMAP_Client::GetActiveWlanIfInfo
(
  qcmap_msgr_wlan_if_info_t *wlan_if_info,
  qmi_error_type_v01 *qmi_err_num
)
{
    if ( !(this->mobile_ap_handle > 0 && this->m_qcmap_msgr_enable == true) )
    {
      *qmi_err_num = QMI_ERR_INVALID_HANDLE_V01;
      return false;
    }
    return QcMapLanClient->GetActiveWlanIfInfo(wlan_if_info, qmi_err_num);
}

/*===========================================================================
  FUNCTION SetDhcpv6DNSConfig
  ===========================================================================*/
/*!
  @brief
  Set Dhcpv6 DNS proxy status (enable/disable)

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
boolean QCMAP_Client::SetDhcpv6DNSConfig
(
  qcmap_msgr_config_state_enum_v01 dhcpv6_dns_state,
  qmi_error_type_v01              *qmi_err_num
)
{
   /* Take care of the types of structure passed in the SetDhcpv6DNSConfig */
   qcmap_config_state     dhcpv6_dns_state_lan;
   dhcpv6_dns_state_lan = (qcmap_config_state)dhcpv6_dns_state;

   if ( !(this->mobile_ap_handle > 0 && this->m_qcmap_msgr_enable == true) )
   {
     *qmi_err_num = QMI_ERR_INVALID_HANDLE_V01;
     return false;
   }


   return QcMapLanClient->SetDhcpv6DNSConfig(dhcpv6_dns_state_lan, qmi_err_num);

}

/*===========================================================================
  FUNCTION GetDhcpv6DNSConfig
  ===========================================================================*/
/*!
  @brief
  Get Dhcpv6 DNS proxy status on device

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
boolean QCMAP_Client::GetDhcpv6DNSConfig
(
  qcmap_msgr_config_state_enum_v01 *dhcpv6_dns_state,
  qmi_error_type_v01               *qmi_err_num
)
{
    return QcMapLanClient->GetDhcpv6DNSConfig(dhcpv6_dns_state, qmi_err_num);
}

/*===========================================================================
 FUNCTION GetLANBridges
 ===========================================================================*/
/*!
 @brief
   Gets all configured LAN Bridges for VLAN

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
boolean QCMAP_Client::GetLANBridges
(
 qcmap_msgr_bridge_list_v01  *bridge_list,
 qmi_error_type_v01          *qmi_err_num
)
{

    int16_t current_bridge_context = -1;
    if (QcMapLanClient->GetBridgeVLANContext(&current_bridge_context))
    {
       bridge_list->curr_bridge = current_bridge_context;
       bridge_list->num_of_bridges = 1;
       return true;
    }
    else
    {
       return false;
    }
}


/*===========================================================================
  FUNCTION Set IP Passthrough Configuration
  ===========================================================================*/
/** @ingroup qcmap_set_ip_passthrough_config

  Set the IP Passthrough configuration for tethered client.

  @datatypes
  qcmap_msgr_ip_passthrough_mode_enum_v01
  qcmap_msgr_ip_passthrough_config_v01
  qmi_error_type_v01

  @param[in] enable_state                IP passthrough enable state.
  @param[in] new_config                  Use New/Existing config
  @param[in] ip_passthrough_config       IP passthrough Configuration.
  @param[out] qmi_err_num                Error code returned by the server.

  @return
  TRUE upon success. \n
  FALSE upon failure.

  @dependencies
  QCMobileAP must be enabled.
*/

/*=========================================================================*/
boolean QCMAP_Client::SetIPPassthroughConfig
(
  qcmap_msgr_ip_passthrough_mode_enum_v01  enable_state,
  bool                                     new_config,
  qcmap_msgr_ip_passthrough_config_v01    *ip_passthrough_config,
  qmi_error_type_v01                      *qmi_err_num
)
{

    qcmap_lan_ip_passthrough_config ip_passthrough_config_lan;
    int i = 0, ip_pass_config_type = 0, device_type = 0;
    uint32_t default_handle = 0;
    uint32_t profile_handle = 0;
    qcmap_lan_ip_passthrough_mode_enum enable_state_lan;
    enable_state_lan = (qcmap_lan_ip_passthrough_mode_enum)enable_state;

    memset(&ip_passthrough_config_lan, 0, sizeof(qcmap_lan_ip_passthrough_config));

    if ( !(this->mobile_ap_handle > 0 && this->m_qcmap_msgr_enable == true) )
    {
      *qmi_err_num = QMI_ERR_INVALID_HANDLE_V01;
      return false;
    }

    /* get Current BH and default BH info */
    if (!GetWWANInfo(&default_handle, &profile_handle, this))
    {
      printf("Failed to retrieve current profile handle\n");
      return false;
    }

    /* Check if ip_passthrough_config structure is NULL */
    if (NULL != ip_passthrough_config)
    {
      strlcpy(ip_passthrough_config_lan.client_device_name,
               ip_passthrough_config->client_device_name,
               strlen(ip_passthrough_config->client_device_name)+1);
      for (int i = 0; i < QCMAP_MSGR_MAC_ADDR_LEN_V01; i++)
      {
        ip_passthrough_config_lan.mac_addr[i] = ip_passthrough_config->mac_addr[i];
      }
      ip_passthrough_config_lan.device_type = (qcmap_lan_device_type_enum)ip_passthrough_config->device_type;
    }

    if (QcMapLanClient->SetIPPassthroughConfig(enable_state_lan,
                                               new_config,
                                               &ip_passthrough_config_lan,
                                               (const uint32_t)default_handle,
                                                                qmi_err_num))
    {
       return true;
    }
    else
    {
       return false;
    }
}


/*===========================================================================
FUNCTION GetIPPassthroughConfig()
===========================================================================*/
/** @ingroup qcmap_get_ip_passthrough_flag

  Get the IP Passthrough configuration.

  @datatypes
  qcmap_msgr_ip_passthrough_mode_enum_v01
  qcmap_msgr_ip_passthrough_config_v01
  qmi_error_type_v01

  @param[out] enable_state                Current state of IP Passthrough
  @param[out] ip_passthrough_config       IP Passthrough Configuration.
  @param[out] qmi_err_num                 Error code returned by the server.

  @return
  TRUE upon success. \n
  FALSE upon failure.

  @dependencies
  None
*/

/*=========================================================================*/
boolean QCMAP_Client::GetIPPassthroughConfig
(
  qcmap_msgr_ip_passthrough_mode_enum_v01  *enable_state,
  qcmap_msgr_ip_passthrough_config_v01     *ip_passthrough_config,
  qmi_error_type_v01                       *qmi_err_num
)
{
  qcmap_lan_ip_passthrough_config ip_passthrough_config_lan;
  qcmap_lan_ip_passthrough_mode_enum enable_state_lan;
  uint32_t default_handle = 0;
  uint32_t profile_handle = 0;

  /* Perform series of NULL checks */
  if ( NULL == enable_state || NULL == ip_passthrough_config)
  {
    LOG_MSG_ERROR("NULL parameters passed. Exiting!", 0, 0, 0);
    return false;
  }

  /* get Current BH and default BH info */
  if (!GetWWANInfo(&default_handle, &profile_handle, this))
  {
    printf("Failed to retrieve current profile handle\n");
    return false;
  }
  if (QcMapLanClient->GetIPPassthroughConfig(&enable_state_lan, &ip_passthrough_config_lan, qmi_err_num))
  {
    *enable_state = (qcmap_msgr_ip_passthrough_mode_enum_v01)enable_state_lan;
    ip_passthrough_config->device_type = (qcmap_msgr_device_type_enum_v01)ip_passthrough_config_lan.device_type;
    strlcpy(ip_passthrough_config->client_device_name,
             ip_passthrough_config_lan.client_device_name,
             strlen(ip_passthrough_config_lan.client_device_name)+1);
    for (int i = 0; i < QCMAP_MSGR_MAC_ADDR_LEN_V01; i++)
    {
      ip_passthrough_config->mac_addr[i] = ip_passthrough_config_lan.mac_addr[i];
    }
    return true;
  }
  else
  {
    return false;
  }
}

/*===========================================================================
FUNCTION GetIPPassthroughState()
===========================================================================*/
/** @ingroup qcmap_get_ip_passthrough_state

  Get the IP Passthrough active state.

  @datatypes
  qmi_error_type_v01

  @param[out] active_state                Current active state of IP Passthrough
  @param[out] qmi_err_num                 Error code returned by the server.

  @return
  TRUE upon success. \n
  FALSE upon failure.

  @dependencies
  None.
*/

/*=========================================================================*/
boolean QCMAP_Client::GetIPPassthroughState
(
  boolean             *state,
  qmi_error_type_v01  *qmi_err_num
)
{
    uint32_t default_handle = 0;
    uint32_t profile_handle = 0;
    /* Get Backhaul context */
    if (!GetWWANInfo(&default_handle, &profile_handle, this))
    {
      printf("Failed to retrieve current profile handle\n");
      return false;
    }
    if (QcMapLanClient->GetIPPassthroughState(state, qmi_err_num))
    {
       return true;
    }
    else
    {
       return false;
    }
}

/*===========================================================================
FUNCTION SetGSBConfig()
===========================================================================*/
/** @ingroup qcmap_set_gsb_config

  Sets GSB configuration.

  @datatypes
  qcmap_msgr_gsb_config_v01
  qmi_error_type_v01

  @param[out] qmi_err_num       Error code returned by the server.

  @return
  TRUE upon success. \n
  FALSE upon failure.

  @dependencies
  QCMobileAP must be enabled.
*/
/*=========================================================================*/
boolean QCMAP_Client::SetGSBConfig
(
  qcmap_msgr_gsb_config_v01  *config,
  qmi_error_type_v01         *qmi_err_num
)
{
    qcmap_gsb_config gsb_conf;
    memset(&gsb_conf, 0, sizeof(gsb_conf));

    if ( !(this->mobile_ap_handle > 0 && this->m_qcmap_msgr_enable == true) )
    {
      *qmi_err_num = QMI_ERR_INVALID_HANDLE_V01;
      return false;
    }

    /*
       fill the configuration from config to gsb_conf
    */
    memcpy(gsb_conf.if_name, config->if_name, sizeof(config->if_name));
    gsb_conf.bw_reqd_in_mb = config->bw_reqd_in_mb;
    gsb_conf.if_low_watermark = config->if_low_watermark;
    gsb_conf.if_high_watermark = config->if_high_watermark;
    gsb_conf.ap_ip = config->ap_ip;
    gsb_conf.if_type = (qcmap_gsb_interface_type_enum)config->if_type;

    if(QcMapLanClient->SetGSBConfig(&gsb_conf, qmi_err_num))
    {
      return true;
    }
    else
    {
      return false;
    }
}

/*===========================================================================
FUNCTION EnableGSB()
===========================================================================*/
/** @ingroup qcmap_enable_gsb

  Enables the GSB. This function loads generic software bridge kernel module
  and sends the configuration to GSB

  @datatypes
  qmi_error_type_v01

  @param[out] qmi_err_num       Error code returned by the server.

  @return
  TRUE upon success. \n
  FALSE upon failure.

  @dependencies
  QCMobileAP must be enabled.
*/
/*=========================================================================*/
boolean QCMAP_Client::EnableGSB
(
  qmi_error_type_v01 *qmi_err_num
)
{
    if(QcMapLanClient->EnableGSB(qmi_err_num))
    {
      return true;
    }
    else
      return false;
}


/*===========================================================================
FUNCTION DisableGSB()
===========================================================================*/
/** @ingroup qcmap_disable_gsb

  Disables the GSB. This function unloads generic software bridge kernel module.

  @datatypes
  qmi_error_type_v01

  @param[out] qmi_err_num       Error code returned by the server.

  @return
  TRUE upon success. \n
  FALSE upon failure.

  @dependencies
  QCMobileAP must be enabled.
*/
/*=========================================================================*/
boolean QCMAP_Client::DisableGSB
(
  qmi_error_type_v01 *qmi_err_num
)
{
  if(QcMapLanClient->DisableGSB(qmi_err_num))
  {
    return true;
  }
  else
    return false;
}

/*===========================================================================
FUNCTION GetGSBConfig()
===========================================================================*/
/** @ingroup qcmap_get_gsb_config

  Gets GSB configuration.

  @datatypes
  qcmap_msgr_gsb_config_v01
  num_of_entries
  qmi_error_type_v01

  @param[out] qmi_err_num  Error code returned by the server.

  @return
  TRUE upon success. \n
  FALSE upon failure.

  @dependencies
  None
*/
/*=========================================================================*/
boolean QCMAP_Client::GetGSBConfig
(
  qcmap_msgr_gsb_config_v01  *config,
  uint8                      *num_of_entries,
  qmi_error_type_v01         *qmi_err_num
)
{
    qcmap_gsb_config gsb_conf_arr[QCMAP_MAX_IF_SUPPORTED];
    memset(gsb_conf_arr, 0, sizeof(qcmap_gsb_config)*QCMAP_MAX_IF_SUPPORTED);
    memcpy(gsb_conf_arr, config,sizeof(config));

    if (QcMapLanClient->GetGSBConfig(gsb_conf_arr, num_of_entries, qmi_err_num))
    {
       return true;
    }
    else
    {
       return false;
    }
}

/*===========================================================================
FUNCTION DeleteGSBConfig()
===========================================================================*/
/** @ingroup qcmap_delete_gsb_config

  Deletes configuration stored for GSB

  @datatypes
  qmi_error_type_v01

  @param[out] qmi_err_num       Error code returned by the server.

  @return
  TRUE upon success. \n
  FALSE upon failure.

  @dependencies
  QCMobileAP must be enabled.
*/
/*=========================================================================*/
boolean QCMAP_Client::DeleteGSBConfig
(
  char               *if_name,
  qmi_error_type_v01 *qmi_err_num
)
{
    if((if_name != NULL) && QcMapLanClient->DeleteGSBConfig(if_name, qmi_err_num))
    {
      return true;
    }
    else
      return false;

}


/*===========================================================================
  FUNCTION
  ===========================================================================*/
/*!
  @brief
  Brings up the WLAN interface

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
boolean QCMAP_Client::EnableWLAN(qmi_error_type_v01 *qmi_err_num, boolean privileged_client)
{
    if (this->mobile_ap_handle > 0 && this->m_qcmap_msgr_enable == true)
    {
      if (QcMapLanClient->EnableWLAN(qmi_err_num))
      {
        /* notify server to switch between N79 and wlan */
        NotifyServerWlanStatus(true, qmi_err_num);

        return true;
      }
      else {
        *qmi_err_num = QMI_ERR_INTERNAL_V01;
        return false;
      }
    }
    else
      return false;
}

/*===========================================================================
  FUNCTION DisableWLAN
  ===========================================================================*/
/*!
  @brief
  Brings the WLAN interface down

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
boolean QCMAP_Client::DisableWLAN(qmi_error_type_v01 *qmi_err_num, boolean privileged_client)
{

    if ( this->mobile_ap_handle > 0 && this->m_qcmap_msgr_enable == true)
    {
      if (QcMapLanClient->DisableWLAN(qmi_err_num))
      {
        /* notify server to start the wlan hysteresis timer */
        NotifyServerWlanStatus(false, qmi_err_num);

        return true;
      }
      else {
        *qmi_err_num = QMI_ERR_INTERNAL_V01;
        return false;
      }
    }
    else
      return false;

}

/*===========================================================================
  FUNCTION ActivateWLAN
  ===========================================================================*/
/*!
  @brief
  Activates the WLAN interface with the current available config.

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
boolean QCMAP_Client::ActivateWLAN
(
  qmi_error_type_v01 *qmi_err_num
)
{
    if ( this->mobile_ap_handle > 0 && this->m_qcmap_msgr_enable == true)
    {
      if (QcMapLanClient->ActivateWLAN(qmi_err_num))
      {
        /* notify server to switch between N79 and wlan */
        NotifyServerWlanStatus(true, qmi_err_num);

        return true;
      }
      else {
        *qmi_err_num = QMI_ERR_INTERNAL_V01;
        return false;
      }
    }
    else
      return false;
}


/*===========================================================================
  FUNCTION AddPDNToVLANMapping
  ===========================================================================*/
/*!
  @brief
  Adds a PDN to VLAN mapping pair

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
boolean QCMAP_Client::AddPDNToVLANMapping
(
  int16_t                  vlan_id,
  profile_handle_type_v01  profile_handle,
  qmi_error_type_v01      *qmi_err_num
)
{

    if (!QcMapLanClient->AddPDNToVLANMapping(vlan_id, profile_handle, qmi_err_num))
    {
        LOG_MSG_ERROR("Failed to Add VLAN/Bridge %d to profile_handle %d", vlan_id,
               profile_handle, 0);
        return false;
    }
    else
    {
        LOG_MSG_INFO1("Add VLAN/Bridge %d to profile_handle %d Successful", vlan_id,
                      profile_handle, 0);
        return true;
    }
}

/*===========================================================================
  FUNCTION DeletePDNToVLANMapping
  ===========================================================================*/
/*!
  @brief
  Deletes a PDN to VLAN mapping pair

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
boolean QCMAP_Client::DeletePDNToVLANMapping
(
  int16_t                  vlan_id,
  profile_handle_type_v01  profile_handle,
  qmi_error_type_v01      *qmi_err_num
)
{
    if (!QcMapLanClient->DeletePDNToVLANMapping(vlan_id, profile_handle, qmi_err_num))
    {
        LOG_MSG_ERROR("Failed to Delete VLAN/Bridge %d to profile_handle %d\n", vlan_id,
               profile_handle, 0);
        return false;
    }
    else
    {
        LOG_MSG_INFO1("Delete mapping of VLAN/Bridge %d from profile_handle %d Successful",
                      vlan_id, profile_handle, 0);
        return true;
    }
}

/*===========================================================================
  FUNCTION GetPDNtoVLANMappingsEx
  ===========================================================================*/
/*!
  @brief
  Retrieves all the PDN to VLAN mapping pairs

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
boolean QCMAP_Client::GetPDNtoVLANMappingsEx
(
  qcmap_msgr_pdn_to_vlan_mapping_ex_v01 *pdn_vlan_mappings_ex,
  int                                   *num_entries,
  qmi_error_type_v01                    *qmi_err_num
)
{
    qcmap_pdn_to_vlan_mapping mappings[QCMAP_MAX_BACKHAULS];
    ZERO_INIT_ARG(mappings);

    if (QcMapLanClient->GetPDNToVLANMapping(mappings, num_entries, qmi_err_num))
    {
      for(int i = 0;i < *num_entries; i++)
      {
         pdn_vlan_mappings_ex[i].profile_handle = mappings[i].profile_handle;
         pdn_vlan_mappings_ex[i].vlan_id_len = mappings[i].vlan_id_len;
         memcpy(pdn_vlan_mappings_ex[i].vlan_id, mappings[i].vlan_id, sizeof(pdn_vlan_mappings_ex[i].vlan_id));
      }
      return true;
    }
    else
    {
      LOG_MSG_ERROR("No mappings Found , Error: 0x%x", *qmi_err_num,0,0);
      return false;
    }

}

/*===========================================================================
  FUNCTION SetActiveBackhaulPref
  ===========================================================================*/
/*!
  @brief
  Sets Backhaul Preference

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
boolean QCMAP_Client::SetActiveBackhaulPref
(
  backhaul_pref_t     *backhaul_pref_req,
  qmi_error_type_v01  *qmi_err_num
)
{
  qcmap_backhaul_pref_t qcmap_backhaul_pref;
   memset(&qcmap_backhaul_pref, 0, sizeof(qcmap_backhaul_pref_t));

   if ( !(this->mobile_ap_handle > 0 && this->m_qcmap_msgr_enable == true) )
   {
     *qmi_err_num = QMI_ERR_INVALID_HANDLE_V01;
     return false;
   }

   qcmap_backhaul_pref.first =  (qcmap_backhaul_type_enum)backhaul_pref_req->first;
   qcmap_backhaul_pref.second = (qcmap_backhaul_type_enum)backhaul_pref_req->second;
   qcmap_backhaul_pref.third = (qcmap_backhaul_type_enum)backhaul_pref_req->third;
   qcmap_backhaul_pref.fourth = (qcmap_backhaul_type_enum)backhaul_pref_req->fourth;
   qcmap_backhaul_pref.fifth = (qcmap_backhaul_type_enum)backhaul_pref_req->fifth;

   if(QcMapLanClient->SetActiveBackhaulPref(&qcmap_backhaul_pref, qmi_err_num))
   {

      return true;
   }
   else
   {
      LOG_MSG_ERROR("SetActiveBackhaulPref  Failed , Error: 0x%x", *qmi_err_num,0,0);
      return false;
   }
}


/*===========================================================================
  FUNCTION GetBackhaulPref
  ===========================================================================*/
/*!
  @brief
  Gets Current Backhaul Preference

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
boolean QCMAP_Client::GetBackhaulPref
(
  backhaul_pref_t     *backhaul_pref_resp,
  qmi_error_type_v01  *qmi_err_num
)
{
    qcmap_backhaul_pref_t qcmap_backhaul_pref_resp;
    memset(&qcmap_backhaul_pref_resp, 0, sizeof(*backhaul_pref_resp));

    if(QcMapLanClient->GetBackhaulPref(&qcmap_backhaul_pref_resp, qmi_err_num))
    {
       backhaul_pref_resp->first =  (qcmap_msgr_backhaul_type_enum_v01)qcmap_backhaul_pref_resp.first;
       backhaul_pref_resp->second = (qcmap_msgr_backhaul_type_enum_v01)qcmap_backhaul_pref_resp.second;
       backhaul_pref_resp->third = (qcmap_msgr_backhaul_type_enum_v01)qcmap_backhaul_pref_resp.third;
       backhaul_pref_resp->fourth = (qcmap_msgr_backhaul_type_enum_v01)qcmap_backhaul_pref_resp.fourth;
       backhaul_pref_resp->fifth = (qcmap_msgr_backhaul_type_enum_v01)qcmap_backhaul_pref_resp.fifth;

       return true;
    }
    else
    {
       LOG_MSG_ERROR("GetBackhaul Priority Failed , Error: 0x%x", *qmi_err_num,0,0);
       return false;
    }
}

/*===========================================================================
  FUNCTION SetEZMeshConfig()
=============================================================================*/
/** @ingroup qcmap_set_ezmesh_config

  Set EZMesh upon getting request from QCMAP.

  @param[in]      qcmap_msgr_ezmesh_mode_enum_v01 enable
  @param[in]      boolean new_config
  @param[in]      qcmap_ezmesh_config *ezmesh_config
  @param[in]      qmi_err_num      Pointer to the error code returned by the server.

  @return
  TRUE -- Success \n
  FALSE -- Failure \n
*/
/*=========================================================================*/
boolean QCMAP_Client::SetEZMeshConfig
(
  qcmap_msgr_ezmesh_mode_enum_v01          enable,
  bool                                     new_config,
  qcmap_ezmesh_config                      ezmesh_config,
  qmi_error_type_v01                       *qmi_err_num
)
{
    if ( !(this->mobile_ap_handle > 0 && this->m_qcmap_msgr_enable == true) )
    {
      *qmi_err_num = QMI_ERR_INVALID_HANDLE_V01;
      return false;
    }
    return QcMapLanClient->SetEZMeshConfig(enable,new_config,&ezmesh_config, qmi_err_num);
}

/*===========================================================================
  FUNCTION GetEZMeshConfig
  ===========================================================================*/
/*!
  @brief
  Gets EZMesh config

  @param[out] status           EZMesh Status
  @param[out] ezmesh_config    Gets the EZMesh configuration.
  @param[out] qmi_err_num      Pointer to the error code returned by the server.

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
boolean QCMAP_Client::GetEZMeshConfig
(
  qcmap_msgr_ezmesh_status_enum_v01          *status,
  qcmap_ezmesh_config                        *ezmesh_config,
  qmi_error_type_v01                         *qmi_err_num
)
{
    if ( !(this->mobile_ap_handle > 0 && this->m_qcmap_msgr_enable == true) )
    {
      *qmi_err_num = QMI_ERR_INVALID_HANDLE_V01;
      return false;
    }
    bool return_val = QcMapLanClient->GetEZMeshConfig((qcmap_msgr_ezmesh_mode_enum_v01*)status, ezmesh_config, qmi_err_num);
    if(return_val && *status == QCMAP_MSGR_EZMESH_ENABLED_V01)
        *status = QCMAP_MSGR_EZMESH_ENABLED_V01;

    return return_val;
}

/*===========================================================================
  FUNCTION ActivateHostapdConfig
  ===========================================================================*/
/*!
  @brief
  Activates the Hostapd with the current available config.

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
boolean QCMAP_Client::ActivateHostapdConfig
(
  qcmap_msgr_activate_hostapd_ap_enum_v01 ap_type,
  qcmap_msgr_activate_hostapd_action_enum_v01 action_type,
  qmi_error_type_v01 *qmi_err_num,
  boolean privileged_client,
  qcmap_hostapd_ap_config_list* ap_list
)
{
    qcmap_activate_hostapd_ap_enum ap_type_lan;
    qcmap_activate_hostapd_action_enum action_type_lan;

    if ( !(this->mobile_ap_handle > 0 && this->m_qcmap_msgr_enable == true) )
    {
      *qmi_err_num = QMI_ERR_INVALID_HANDLE_V01;
      return false;
    }
    action_type_lan = (qcmap_activate_hostapd_action_enum)action_type;

    return QcMapLanClient->ActivateEZMeshHostapdConfig(ap_list, action_type_lan, qmi_err_num);
}

/*===========================================================================
  FUNCTION SetEZMeshServicePriority
  ===========================================================================*/
/*!
  @brief
  Sets EZMesh Service Prioritization state config

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
boolean QCMAP_Client::SetEZMeshServicePriority
(
  qcmap_ezmesh_config           ezmesh_config,
  qmi_error_type_v01            *qmi_err_num
)
{
   if ( !(this->mobile_ap_handle > 0 && this->m_qcmap_msgr_enable == true) )
   {
     *qmi_err_num = QMI_ERR_INVALID_HANDLE_V01;
     return false;
   }
    ezmesh_config.is_service_prioritization_state_valid = 1;
    return QcMapLanClient->SetEZMeshServicePriority(ezmesh_config.is_service_prioritization_state_valid,
                                                    ezmesh_config.service_prioritization_state,
                                                    qmi_err_num);
}

/*===========================================================================
  FUNCTION ConfigureCoEXChannelAvoidance
  ===========================================================================*/
/*!
  @brief
  Enable/Disable the CoEX channel avoidance to reduce co-channel interference
  between WLAN <-> WWAN channels.
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
boolean QCMAP_Client::ConfigureCoEXChannelAvoidance
(
  bool coex_state,
  qmi_error_type_v01 *qmi_err_num
)
{
    int coex_state_lan = coex_state?1:2;
    if(QcMapLanClient->SetCoexConfig(coex_state_lan))
    {
       return true;
    }
    else
    {
       LOG_MSG_ERROR("setCoexConfig failed", 0,0,0);
       return false;
    }

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
int32_t QCMAP_Client:: WhitelistWLANChannels()
{
    return QcMapLanClient->WhitelistWLANChannels();
}

/*===========================================================================
  FUNCTION GetTinyProxyStatus
  ===========================================================================*/
/*!
  @brief
  Get TinyProxy Status

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
boolean QCMAP_Client::GetTinyProxyStatus
(
  qcmap_msgr_tiny_proxy_mode_enum_v01  *tiny_proxy_status,
  qmi_error_type_v01                   *qmi_err_num
)
{
  return QcMapLanClient->GetTinyProxyStatus(tiny_proxy_status, qmi_err_num);
}

/*===========================================================================
  FUNCTION EnableTinyProxy
  ===========================================================================*/
/*!
  @brief
  Enables TinyProxy

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
boolean QCMAP_Client::EnableTinyProxy(qmi_error_type_v01 *qmi_err_num)
{
  qcmap_msgr_wwan_status_enum_v01 v4_status, v6_status,eth_status;
  boolean ret = QcMapLanClient->EnableTinyProxy(qmi_err_num);
  if(ret)
  {
      /* Save the configuration of TinyProxy in qcmap_lan even when backhaul is disconnected */
    LOG_MSG_INFO1("Successfully enable tinyproxy config in qcmap_lan",0,0,0);

    if( GetWWANStatus(&v4_status, &v6_status, qmi_err_num))
    {
      if ( v4_status == QCMAP_MSGR_WWAN_STATUS_CONNECTED_V01 ||
           v6_status == QCMAP_MSGR_WWAN_STATUS_IPV6_CONNECTED_V01 )
      {
        QcMapLanClient->SetupTinyProxy( );
      }
    }
  }

  return ret;
}

/*===========================================================================
  FUNCTION DisableTinyProxy
  ===========================================================================*/
/*!
  @brief
  Disables TinyProxy

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
boolean QCMAP_Client::DisableTinyProxy(qmi_error_type_v01 *qmi_err_num)
{
  boolean ret = QcMapLanClient->DisableTinyProxy(qmi_err_num);
  if(ret)
  {
    QcMapLanClient->StopTinyProxy();
  }
  return ret;
}


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
boolean QCMAP_Client::EnableUPNP(qmi_error_type_v01 *qmi_err_num)
{
  return QCMapMMObj->EnableUPNP(qmi_err_num);
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
boolean QCMAP_Client::DisableUPNP(qmi_error_type_v01 *qmi_err_num)
{
  return QCMapMMObj->DisableUPNP(qmi_err_num);
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
boolean QCMAP_Client::SetUPNPState
(
  boolean              firewall_state,
  boolean              upnp_pinhole_flag,
  qmi_error_type_v01  *qmi_err_num
)
{
   if ( !(this->mobile_ap_handle > 0 && this->m_qcmap_msgr_enable == true) )
   {
     *qmi_err_num = QMI_ERR_INVALID_HANDLE_V01;
     return false;
   }
  return QcMapLanClient->SetUPNPState(upnp_pinhole_flag);
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
boolean QCMAP_Client::GetUPNPState
(
  boolean              *upnp_pinhole_flag,
  qmi_error_type_v01   *qmi_err_num
)
{
  return QcMapLanClient->GetUPNPState(upnp_pinhole_flag, qmi_err_num);
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
boolean QCMAP_Client::GetUPNPStatus
(
  qcmap_msgr_upnp_mode_enum_v01 *upnp_status,
  qmi_error_type_v01            *qmi_err_num
)
{
  return QCMapMMObj->GetUPNPStatus(upnp_status, qmi_err_num);
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
boolean QCMAP_Client::SetUPNPNotifyInterval
(
  int                 notify_int,
  qmi_error_type_v01 *qmi_err_num
)
{
   if ( !(this->mobile_ap_handle > 0 && this->m_qcmap_msgr_enable == true) )
   {
     *qmi_err_num = QMI_ERR_INVALID_HANDLE_V01;
     return false;
   }
  return QCMapMMObj->SetUPNPNotifyInterval(notify_int, qmi_err_num);
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
boolean QCMAP_Client::GetUPNPNotifyInterval
(
  int                 *notify_int,
  qmi_error_type_v01  *qmi_err_num
)
{
  return QCMapMMObj->GetUPnPNotifyInterval(notify_int, qmi_err_num);
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
boolean QCMAP_Client::EnableMDNS(qmi_error_type_v01 *qmi_err_num)
{
  return QCMapMMObj->EnableMDNS(qmi_err_num);
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
boolean QCMAP_Client::DisableMDNS(qmi_error_type_v01 *qmi_err_num)
{
  return QCMapMMObj->DisableMDNS(qmi_err_num);
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
boolean QCMAP_Client::GetMDNSStatus
(
  qcmap_msgr_mdns_mode_enum_v01 *mdns_status,
  qmi_error_type_v01            *qmi_err_num
)
{
  return QCMapMMObj->GetMDNSStatus(mdns_status, qmi_err_num);
}

/*===========================================================================
  FUNCTION NotifyServerWlanStatus
  ===========================================================================*/
/*!
  @brief
  notify WiFi enable/disable to QCMAP_ConnectionManager

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
boolean QCMAP_Client::NotifyServerWlanStatus
(
  bool                                        wlan_enable_status,
  qmi_error_type_v01                         *qmi_err_num
)
{
  qmi_client_error_type qmi_error, qmi_err_code = QMI_NO_ERR;
  qcmap_msgr_notify_server_action_req_msg_v01 notify_server_action_req_msg_v01;
  qcmap_msgr_notify_server_action_resp_msg_v01 notify_server_action_resp_msg_v01;

  QCMAP_LOG_FUNC_ENTRY();

  if (qmi_err_num == NULL)
  {
    LOG_MSG_ERROR("Input param is NULL",0,0,0);
    return false;
  }

  BZERO_QMI_MSG(notify_server_action_req_msg_v01);
  BZERO_QMI_MSG(notify_server_action_resp_msg_v01);

  notify_server_action_req_msg_v01.mobile_ap_handle = this->mobile_ap_handle;
  if (wlan_enable_status)
  {
    QCMAP_QMI_SET_OPTIONAL_PARAM(notify_server_action_req_msg_v01.wlan_event, QCMAP_MSGR_WLAN_ENABLE_V01);
  }
  else
  {
    QCMAP_QMI_SET_OPTIONAL_PARAM(notify_server_action_req_msg_v01.wlan_event, QCMAP_MSGR_WLAN_DISABLE_V01);
  }

  qmi_error = qmi_client_send_msg_sync(this->m_qmi_qcmap_preferred_handle,
                                       QMI_QCMAP_MSGR_NOTIFY_SERVER_ACTION_REQ_V01,
                                       &notify_server_action_req_msg_v01,
                                       sizeof(notify_server_action_req_msg_v01),
                                       (void*)&notify_server_action_resp_msg_v01,
                                       sizeof(notify_server_action_resp_msg_v01),
                                       QCMAP_MSGR_QMI_TIMEOUT_VALUE);

  LOG_MSG_INFO1("qmi_client_send_msg_sync(NotifyServerWlanStatus): error %d result %d",
      qmi_error, notify_server_action_resp_msg_v01.resp.result,0);

  if ( ( qmi_error == QMI_TIMEOUT_ERR ) ||
       ( qmi_error != QMI_NO_ERR ) ||
       ( notify_server_action_resp_msg_v01.resp.result != QMI_NO_ERR ) )
  {
    LOG_MSG_ERROR("Can not notify server event %d : %d",
        qmi_error, notify_server_action_resp_msg_v01.resp.error,0);
    *qmi_err_num = notify_server_action_resp_msg_v01.resp.error;
    return false;
  }

  LOG_MSG_INFO1("Sent wlan event %d msg successfully", notify_server_action_req_msg_v01.wlan_event, 0, 0);
  return true;
}

/*===========================================================================
FUNCTION SetIPsecTunnelInfo()
===========================================================================*/
/** @ingroup section_SetIPsecTunnelInfo

  Sets IPsec Tunnel

  @param[in] qcmap_ipsec_config_t   *ipsec_config
  @param[out] qmi_error_type_v01 *qmi_err_num

  @return
  TRUE -- Success \n
  FALSE -- Failure \n

  @dependencies
  QCMobileAP must be enabled. @newpage
*/
/*=========================================================================*/
boolean QCMAP_Client::SetIPsecTunnelInfo
(
  qcmap_ipsec_config_t *ipsec_config,
  qmi_error_type_v01 *qmi_err_num
)
{
  return QcMapLanClient->SetIPsecTunnelInfo(ipsec_config, qmi_err_num);
}



/*===========================================================================
FUNCTION ActivateIPsecTunnelInfo()
===========================================================================*/
/** @ingroup section_ActivateIPsecTunnelInfo

  Activates IPsec Tunnel

  @param[in] char *ike_identifier
  @param[in] char *child_identifier
  @param[out] qmi_error_type_v01 *qmi_err_num

  @return
  TRUE -- Success \n
  FALSE -- Failure \n

  @dependencies
  QCMobileAP must be enabled. @newpage
  If child_identifier is not included, all the child_id under ike_id will be activated
*/
/*=========================================================================*/
boolean QCMAP_Client::ActivateIPsecTunnelInfo
(
  char *ike_identifier,
  char *child_identifier,
  qmi_error_type_v01 *qmi_err_num
)
{
  return QcMapLanClient->ActivateIPsecTunnelInfo(ike_identifier, child_identifier, qmi_err_num);
}

/*===========================================================================
FUNCTION DeleteIPsecTunnel()
===========================================================================*/
/** @ingroup section_DeleteIPsecTunnel

  Delete IPsec Tunnel

  @param[in] char *ike_identifier
  @param[in] char *child_identifier
  @param[out] qmi_error_type_v01 *qmi_err_num

  @return
  TRUE -- Success \n
  FALSE -- Failure \n

  @dependencies
  QCMobileAP must be enabled. @newpage
  If child_identifier is not included, all the child_id under ike_id will be deleted
*/
/*=========================================================================*/
boolean QCMAP_Client::DeleteIPsecTunnel
(
  char *ike_identifier,
  char *child_identifier,
  qmi_error_type_v01 *qmi_err_num
)
{
  return QcMapLanClient->DeleteIPsecTunnel(ike_identifier, child_identifier, qmi_err_num);
}

/*===========================================================================
FUNCTION GetIPsecTunnelInfo()
===========================================================================*/
/** @ingroup section_GetIPsecTunnelInfo

  Get IPsec Tunnel Info

  @param[in] char *ike_identifier
  @param[in] char *child_identifier
  @param[in] qcmap_ipsec_config_t *ipsec_config
  @param[out] qmi_error_type_v01 *qmi_err_num

  @return
  TRUE -- Success \n
  FALSE -- Failure \n

  @dependencies
  QCMobileAP must be enabled. @newpage
  All the child SAs under the IKE_id will be returned
*/
/*=========================================================================*/
boolean QCMAP_Client::GetIPsecTunnelInfo
(
  char *ike_identifier,
  char *child_identifier,
  qcmap_ipsec_config_t *ipsec_config,
  qmi_error_type_v01 *qmi_err_num
)
{
  return QcMapLanClient->GetIPsecTunnelInfo(ike_identifier, child_identifier, ipsec_config, qmi_err_num);
}

/*===========================================================================
FUNCTION GetIPsecTunnelStateInfo()
===========================================================================*/
/** @ingroup section_GetIPsecTunnelStateInfo

  Get IPsec Tunnel State Info

  @param[in] char *ike_identifier
  @param[in] char *child_identifier
  @param[in] qcmap_ipsec_tunnel_state_info_t *state_info
  @param[out] qmi_error_type_v01 *qmi_err_num

  @return
  TRUE -- Success \n
  FALSE -- Failure \n

  @dependencies
  QCMobileAP must be enabled. @newpage
  All the child SAs under the IKE_id will be returned
*/
/*=========================================================================*/
boolean QCMAP_Client::GetIPsecTunnelStateInfo
(
  char *ike_identifier,
  char *child_identifier,
  qcmap_ipsec_tunnel_state_info_t *state_info,
  qmi_error_type_v01 *qmi_err_num
)
{
  return QcMapLanClient->GetIPsecTunnelStateInfo(ike_identifier, child_identifier, state_info, qmi_err_num);
}

/*===========================================================================
  FUNCTION GetFireWallEntry
  ===========================================================================*/
/*!
  @brief
  Gets the Firewall Configuration from  QCMAP connection manager  and
  decodes the message .

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
boolean QCMAP_Client::GetFireWallEntry
(
  qcmap_msgr_firewall_entry_conf_t  *firewall_entry,
  qmi_error_type_v01                *qmi_err_num
)
{
   if(QcMapLanClient->GetFireWallEntry_by_handle(firewall_entry, qmi_err_num))
    {
        return true;
    }
    else
    {
        LOG_MSG_ERROR("GetFireWallEntry_by_handle failed",0,0,0);
        return false;
    }
}

/*===========================================================================
  FUNCTION GetNetworkConfiguration()
  ===========================================================================*/
/*!
  @brief
  Gets the Network configuration

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
boolean QCMAP_Client::GetNetworkConfiguration
(
  qcmap_msgr_ip_family_enum_v01 ip_family,
  qcmap_nw_params_t            *qcmap_nw_params,
  qmi_error_type_v01           *qmi_err_num
)
{
  uint32_t current_profile_handle=0;
  qcmap_ip_family_enum ip_type=QCMAP_IP_FAMILY_INVALID;

  if ( qmi_err_num == NULL || qcmap_nw_params == NULL )
  {
    LOG_MSG_ERROR(" Null argument passed ",0,0,0);
    return false;
  }

  ip_type = (qcmap_ip_family_enum)ip_family;
  *qmi_err_num = QMI_ERR_NONE_V01;

  if ((ip_type != QCMAP_IP_FAMILY_V4) &&
#ifdef FEATURE_DATA_ETH_PDU
      (ip_type != QCMAP_IP_FAMILY_ETH) &&
#endif
      (ip_type != QCMAP_IP_FAMILY_V6))
  {
    LOG_MSG_ERROR("GetNetworkConfiguration(): Invalid IP family specified",0,0,0);
    *qmi_err_num = QMI_ERR_INVALID_IP_FAMILY_PREF_V01;
    return false;
  }


  if (!this->GetWWANProfilePreference(&current_profile_handle, qmi_err_num))
  {
    printf("Error getting current profile handle 0x%x.\n ", *qmi_err_num);
    return false;
  }
  if(!QcMapLanClient->GetNetworkConfig(qcmap_nw_params,
                                    current_profile_handle,
                                    ip_type,
                                    qmi_err_num))
  {
     LOG_MSG_ERROR("Failed to get Network Configuration - error = 0x%x",
                  *qmi_err_num,0,0);
    return false;
  }
  return true;

} /* End: GetNetworkConfiguration() */

/*===========================================================================
  FUNCTION SetV4NATConfig
  ===========================================================================*/
/*!
  @brief
  Set IPv4 NAT Configuration.This function is dependent on the network side configurations
  to work as expected. After enabling IPv4 NAT disable configuration,  data  packets  with
  source address as LAN IP will go out to network from UE.By default, NAT will be enabled.

  @datatypes
  boolean
  qmi_error_type_v01

  @param[in]  ipv4_nat_disable           Enable/Disable IPV4 Disable NAT Configuration
  @param[out] qmi_err_num                Error code returned by the server

  @return
  true  - on Success
  false - on Failure

 */
/*=========================================================================*/
boolean QCMAP_Client::SetV4NATConfig
(
  boolean             ipv4_nat_disable,
  qmi_error_type_v01  *qmi_err_num
)
{
  profile_handle_type_v01 current_profile_handle;
  if (!this->GetWWANProfilePreference(&current_profile_handle, qmi_err_num))
  {
    printf("Error getting current profile handle 0x%x.\n ", qmi_err_num);
    return false;
  }

  if(QcMapLanClient->SetV4NATConfig(current_profile_handle, ipv4_nat_disable, qmi_err_num))
  {
    return true;
  }
  else
  {
    LOG_MSG_ERROR("SetV4NATConfig failed",0,0,0);
    return false;
  }
}

/*===========================================================================
  FUNCTION GetV4NATConfig
  ===========================================================================*/
/*!
  @brief
  Get Current Status of IPv4 NAT Configuration.


  @datatypes
  boolean
  qmi_error_type_v01

  @param[out] ipv4_nat_disable           Current Status of IPV4 Disable NAT Configuration
  @param[out] qmi_err_num                Error code returned by the server

  @return
  true  - on Success
  false - on Failure

 */
/*=========================================================================*/
boolean QCMAP_Client::GetV4NATConfig
(
  boolean             &ipv4_nat_disable,
  qmi_error_type_v01  *qmi_err_num
)
{
  bool ret_val=false;
  profile_handle_type_v01 current_profile_handle;

  if (!this->GetWWANProfilePreference(&current_profile_handle, qmi_err_num))
  {
    printf("Error getting current profile handle 0x%x.\n ", qmi_err_num);
    return false;
  }

  if(QcMapLanClient->GetV4NATConfig(current_profile_handle, ipv4_nat_disable, qmi_err_num))
  {
    ret_val=true;
  }
  else
  {
    LOG_MSG_ERROR("GetV4NATConfig failed",0,0,0);
  }

  return ret_val;
}

/*===========================================================================
  FUNCTION SetIPPassthroughSoftwarePathFilters
===========================================================================*/
/** @ingroup qcmap_msgr_set_ip_pt_sw_path_filters

  Set IP Passthrough Software Path Filters
  This function supports Port-Protocol-IP filters where traffic destined
  to the public gateway IP will always take software path. Traffic destined
  to configured ports and protocol will be consumed on the LAN gateway
  interface.

  @datatypes
  qcmap_msgr_sw_path_filters_conf_t
  qmi_error_type_v01

  @param[in] filter_config               Filter config
  @param[out] qmi_err_num                Error code returned by the server

  @return
  TRUE upon success. \n
  FALSE upon failure.

  @Dependencies
  QCMobileAP must be enabled.
*/
/*=========================================================================*/
boolean QCMAP_Client::SetIPPassthroughSoftwarePathFilters
(
  qcmap_msgr_sw_path_filters_conf_t       *filter_config,
  qmi_error_type_v01                      *qmi_err_num
)
{
  qmi_client_error_type qmi_error;
  uint32_t default_handle = 0;
  uint32_t profile_handle = 0;

  QCMAP_LOG_FUNC_ENTRY();

  if ((NULL == filter_config) || (NULL == qmi_err_num))
  {
    LOG_MSG_ERROR("Invalid/NULL arguments", 0, 0, 0);
    return false;
  }

  if ( !(this->mobile_ap_handle > 0 && this->m_qcmap_msgr_enable == true) )
  {
    *qmi_err_num = QMI_ERR_INVALID_HANDLE_V01;
    return false;
  }

  if (filter_config->filter_type == QCMAP_MSGR_IP_PT_SW_PATH_GLOBAL_PORT_FILTER_V01)
  {
    LOG_MSG_ERROR("Global Port Filter Customization is not supported", 0, 0, 0);
    return false;
  }

  /* get Current BH and default BH info */
  if (!GetWWANInfo(&default_handle, &profile_handle, this))
  {
    printf("Failed to retrieve current profile handle\n");
    return false;
  }

  if (QcMapLanClient->SetIPPassthroughSoftwarePathFilters(filter_config,
                                                          (const uint32_t)default_handle,
                                                          (const uint32_t)profile_handle, qmi_err_num))
  {
    return true;
  }
  else
  {
    LOG_MSG_ERROR("Error setting the sw path filters", 0, 0, 0);
    return false;
  }

}

/*===========================================================================
  FUNCTION GetIPPassthroughSoftwarePathFilters
===========================================================================*/
/** @ingroup qcmap_msgr_get_ip_pt_sw_path_filters

  Get currently configured IP Passthrough software path filters

  @datatypes
  qcmap_msgr_sw_path_filters_conf_t
  qmi_error_type_v01

  @param[out] filter_config              Filter config
  @param[out] qmi_err_num                Error code returned by the server

  @return
  TRUE upon success. \n
  FALSE upon failure.

  @Dependencies
  QCMobileAP must be enabled.
*/
/*=========================================================================*/
boolean QCMAP_Client::GetIPPassthroughSoftwarePathFilters
(
  qcmap_msgr_sw_path_filters_conf_t        *filter_config,
  qmi_error_type_v01                       *qmi_err_num
)
{

  uint32_t default_handle = 0;
  uint32_t profile_handle = 0;
  QCMAP_LOG_FUNC_ENTRY();

  if ((NULL == filter_config) || (NULL == qmi_err_num))
  {
    LOG_MSG_ERROR("Invalid/NULL arguments", 0, 0, 0);
    return false;
  }

  if (filter_config->filter_type == QCMAP_MSGR_IP_PT_SW_PATH_GLOBAL_PORT_FILTER_V01)
  {
    LOG_MSG_ERROR("Global Port Filter Customization is not supported", 0, 0, 0);
    return false;
  }

  /* get Current BH and default BH info */
  if (!GetWWANInfo(&default_handle, &profile_handle, this))
  {
    printf("Failed to retrieve current profile handle\n");
    return false;
  }

  if (QcMapLanClient->GetIPPassthroughSoftwarePathFilters(filter_config,
                                                          (const uint32_t)default_handle,
                                                          (const uint32_t)profile_handle, qmi_err_num))
  {
     *qmi_err_num = QMI_ERR_NONE_V01;
     return true;
  }
  else
  {
    LOG_MSG_ERROR("Error in getting the sw path filters", 0, 0, 0);
    return false;
  }

}
