/*====================================================

FILE:  QCMAP_EZMesh_Client.cpp

SERVICES:
QCMAP Client EZMesh specific Implementation

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
  04/30/23   ab         Add support for EZMesh bring up in Openwrt
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
#include "ds_util.h"
#include "ds_string.h"
#include "QCMAP_LAN_Client.h"
#include "QCMAP_EZMesh_Client.h"

#define QCMAP_EZMESH_BRING_UP_MAX_TIMEOUT    90  //By default consider 90sec wait-time
#define QCMAP_EZMESH_SLEEP_INTERVAL          3   //Check ezmesh daemon status for every 3sec interval


/*===================================================================
  Class Definitions
  ===================================================================*/

/*===========================================================================
  FUNCTION ReloadEthIossModules
  ===========================================================================*/
/*!
@brief
  This function sets the EZMesh mode in IPACM XML via IPA_CLI

@parameters

@return
  void

- Side Effects
- None

- Side Effects
- None
*/
/*=========================================================================*/
static void ReloadEthIossModules(void)
{
  char command[QCMAP_MAX_COMMAND_LEN] = {'\0'};

  /* Unload the emac ioss modules
   * 1. unload r8125, aqc and ieamc ioss first
   * 2. unload ioss next */
  memset(command, 0, QCMAP_MAX_COMMAND_LEN);
  snprintf(command, QCMAP_MAX_COMMAND_LEN, "%s", UNLOAD_ETH_R8125_IOSS_MODULE);
  ds_system_call(command, strlen(command));

  memset(command, 0, QCMAP_MAX_COMMAND_LEN);
  snprintf(command, QCMAP_MAX_COMMAND_LEN, "%s", UNLOAD_ETH_AQC_IOSS_MODULE);
  ds_system_call(command, strlen(command));

  memset(command, 0, QCMAP_MAX_COMMAND_LEN);
  snprintf(command, QCMAP_MAX_COMMAND_LEN, "%s", UNLOAD_ETH_EMAC_IOSS_MODULE);
  ds_system_call(command, strlen(command));

  memset(command, 0, QCMAP_MAX_COMMAND_LEN);
  snprintf(command, QCMAP_MAX_COMMAND_LEN, "%s", UNLOAD_ETH_IOSS_MODULE);
  ds_system_call(command, strlen(command));

  /* Load the emac ioss modules
   * 1. load ioss first
   * 2. load ieamc, aqc and r8125 ioss next */
  memset(command, 0, QCMAP_MAX_COMMAND_LEN);
  snprintf(command, QCMAP_MAX_COMMAND_LEN, "%s", LOAD_ETH_IOSS_MODULE);
  ds_system_call(command, strlen(command));

  memset(command, 0, QCMAP_MAX_COMMAND_LEN);
  snprintf(command, QCMAP_MAX_COMMAND_LEN, "%s", LOAD_ETH_EMAC_IOSS_MODULE);
  ds_system_call(command, strlen(command));

  memset(command, 0, QCMAP_MAX_COMMAND_LEN);
  snprintf(command, QCMAP_MAX_COMMAND_LEN, "%s", LOAD_ETH_AQC_IOSS_MODULE);
  ds_system_call(command, strlen(command));

  memset(command, 0, QCMAP_MAX_COMMAND_LEN);
  snprintf(command, QCMAP_MAX_COMMAND_LEN, "%s", LOAD_ETH_R8125_IOSS_MODULE);
  ds_system_call(command, strlen(command));
  return;
}

/*===========================================================================
  FUNCTION SetIPAEZMeshMode
  ===========================================================================*/
/*!
@brief
  This function sets the EZMesh mode in IPACM XML via IPA_CLI

@parameters
  @param[in]    uint8_t     ezmesh_enable
  @param[in]    uint8_t     ezmesh_capability
  @param[in]    uint8_t     traffic_seperation_enable

@return
  true  - on success
  false - on failure


- Side Effects
- None

- Side Effects
- None
*/
/*=========================================================================*/
static boolean SetIPAEZMeshMode
(
  uint8_t   ezmesh_enable,
  uint8_t   ezmesh_capability,
  uint8_t   traffic_seperation_enable
)
{
  char command[QCMAP_MAX_COMMAND_LEN] = {'\0'};
  QCMAP_LAN_Client *QcMapLanClient = new QCMAP_LAN_Client();
  qcmap_cpe_wkk_enum wkk_type = QcMapLanClient->UtilGetCpeWkkType();

  if(ezmesh_enable)
  {
    if(IS_EZMESH_CAPABILITY_R2_R3_R4(ezmesh_capability) &&
       (wkk_type == QCMAP_CPE_WKK_V2))
    {
      if(traffic_seperation_enable)
      {
        snprintf(command, QCMAP_MAX_COMMAND_LEN, "%s %d %s",
                 UPDATE_EZMESH_ENABLE_TO_IPA, ezmesh_capability, UPDATE_EZMESH_VLAN_ENABLE_TO_IPA);
        ds_system_call(command, strlen(command));

        ReloadEthIossModules();
      }
      else
      {
        snprintf(command, QCMAP_MAX_COMMAND_LEN, "%s %d %s",
                 UPDATE_EZMESH_ENABLE_TO_IPA, ezmesh_capability, UPDATE_EZMESH_VLAN_DISABLE_TO_IPA);
        ds_system_call(command, strlen(command));
      }
    }
    else
    {
      snprintf(command, QCMAP_MAX_COMMAND_LEN, "%s %d",
               UPDATE_EZMESH_ENABLE_TO_IPA, ezmesh_capability);
      ds_system_call(command, strlen(command));
    }
  }
  else
  {
    snprintf(command, QCMAP_MAX_COMMAND_LEN, "%s", UPDATE_EZMESH_DISABLE_TO_IPA);
    ds_system_call(command, strlen(command));

    if(IS_EZMESH_CAPABILITY_R2_R3_R4(ezmesh_capability) &&
       traffic_seperation_enable && (wkk_type == QCMAP_CPE_WKK_V2))
    {
      ReloadEthIossModules();
    }
  }

  if(QcMapLanClient)
    delete QcMapLanClient;

  return true;
}

/*===========================================================================
  FUNCTION SetIPAEZMeshSerivcePriorityState
  ===========================================================================*/
/*!
  @brief
  This function inform the EZMesh service prioritization state to IPACM

  @param[in]    uint8_t     service_prioritization_state

  @return
  true  - on success
  false - on failure

  - Side Effects
  - None

  - Side Effects
  - None
*/
/*=========================================================================*/
static boolean SetIPAEZMeshSerivcePriorityState
(
  uint8_t  service_prioritization_state
)
{
  char command[QCMAP_MAX_COMMAND_LEN] = {'\0'};

  if(service_prioritization_state)
  {
    snprintf(command, QCMAP_MAX_COMMAND_LEN, "%s",
             UPDATE_EZMESH_DSCP_ENABLE_TO_IPA);
    ds_system_call(command, strlen(command));
  }
  else
  {
    snprintf(command, QCMAP_MAX_COMMAND_LEN, "%s",
             UPDATE_EZMESH_DSCP_DISABLE_TO_IPA);
    ds_system_call(command, strlen(command));
  }

  return true;
}

/*===========================================================================
  FUNCTION DisableEZMesh()
===========================================================================*/
/** @ingroup qcmap_disable_ezmesh

  Disable EZMesh upon getting request.

  @param[in]      qcmap_msgr_ezmesh_mode_enum_v01 ezmesh_state

  @return
  void
*/
/*=========================================================================*/
void QCMAP_LAN_Client::DisableEZMesh
(
  qcmap_msgr_ezmesh_mode_enum_v01  ezmesh_state
)
{
  char command[QCMAP_MAX_COMMAND_LEN] = {'\0'};
  qcmap_msgr_ezmesh_capability_enum_v01 capability = 0;
  boolean is_ezmesh_r2_cfg_valid = false;
  qcmap_msgr_ezmesh_r2_config_v01 ezmesh_r2_config;
  char result[QCMAP_MAX_SCAN_SIZE] = {'\0'};
  uint8_t service_prioritization_state = 0;
  qmi_error_type_v01 qmi_err_num;

  /* set ezmesh enable flag to 0 */
  snprintf(command, QCMAP_MAX_COMMAND_LEN, "%s %s %d",
           WLAN_EZMESH_CONFIG_FILE, WLAN_SET_EZMESH_CONFIG, ezmesh_state);
  ds_system_call(command, strlen(command));

  UCI_EZMESH_GET_INT_OPTION(capability, "ezmesh", "capability");
  UCI_EZMESH_GET_INT_OPTION(is_ezmesh_r2_cfg_valid, "r2_config", "traffic_separation");
  if (is_ezmesh_r2_cfg_valid &&
      IS_EZMESH_CAPABILITY_R2_R3_R4(capability))
  {
    QCMAP_LAN_Client::GetEZMeshR2Config(&ezmesh_r2_config);
    /* Clean up the vlan-bridge and firewall uci config */
    for (int i = 0; i < ezmesh_r2_config.mapping_len; i++)
    {
      if (ezmesh_r2_config.mapping[i].fh_ap_type != QCMAP_MSGR_PRIMARY_FRONTHAUL_AP_V01)
      {
        QCMAP_LAN_Client::DeleteEZMeshBridgeVLANContext(ezmesh_r2_config.mapping[i].vlan_id);
        QCMAP_LAN_Client::SetBridgeVLANContext(ezmesh_r2_config.mapping[i].vlan_id);
      }
    }
  }

  /* Update IPA about ezmesh disablement */
  SetIPAEZMeshMode(false, capability, is_ezmesh_r2_cfg_valid);

  /* Call the repacd script with stop */
  memset(command, 0, QCMAP_MAX_COMMAND_LEN);
  snprintf(command, QCMAP_MAX_COMMAND_LEN, "%s %s",
           WLAN_EZMESH_CONFIG_FILE, WLAN_SET_EZMESH_REPACD_STOP);
  ds_system_call(command, strlen(command));

  UCI_EZMESH_GET_INT_OPTION(service_prioritization_state, "r3_config", "service_priority");
  if (IS_EZMESH_CAPABILITY_R3_R4(capability) && service_prioritization_state)
  {
    /* Update IPA about service priority disable */
    SetIPAEZMeshSerivcePriorityState(0);
  }

  /* Disable WLAN on bootup flag to as part of ezmesh config */
  QCMAP_LAN_Client::SetWLANBootupConfigEx(QCMAP_MSGR_DISABLE_ON_BOOT_V01, &qmi_err_num);
  return;
}

/*===========================================================================
  FUNCTION ValidateEZMeshConfig()
===========================================================================*/
/** @ingroup qcmap_set_ezmesh_config

  Validate EZMesh config upon getting request from QCMAP to set ezmesh.

  @param[in]      qcmap_ezmesh_config *ezmesh_config

  @return
  TRUE -- Success \n
  FALSE -- Failure \n
*/
/*=========================================================================*/
boolean QCMAP_LAN_Client::ValidateEZMeshConfig
(
  qcmap_ezmesh_config               *ezmesh_config,
  qcmap_msgr_ezmesh_mode_enum_v01   enable
)
{
  char result[QCMAP_MAX_SCAN_SIZE] = {'\0'};
  qcmap_msgr_ezmesh_mode_enum_v01 ezmesh_state = QCMAP_MSGR_EZMESH_MODE_ENUM_MIN_ENUM_VAL_V01;

  if(ezmesh_config == NULL)
  {
    LOG_MSG_ERROR("NULL pointer passed ",0,0,0);
    return false;
  }

  UCI_EZMESH_GET_INT_OPTION(ezmesh_state, "ezmesh", "enable_state");
  if (ezmesh_state == enable)
  {
    LOG_MSG_ERROR("EZMesh is already %s",(ezmesh_state ? "Enabled" : "Disabled"),0,0);
    return false;
  }

  return true;
}

/*===========================================================================
FUNCTION CreateEZMeshBridgeVLANContext()
===========================================================================*/
/** @ingroup qcmap_create_ezmesh_bridge_vlan_context

  creates bridge vlan context linked to ezmesh

  @param[in]      bridge_id                 bridge vlan ID

  @return
  void
*/
/*=========================================================================*/
void QCMAP_LAN_Client::CreateEZMeshBridgeVLANContext
(
  const int16_t bridge_id
)
{
  char cmd[QCMAP_MAX_COMMAND_LEN] = {0};

  /* Send bridge_id and profile_handle information to lan config */
  snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "%s %s %u", VLAN_CONFIG,
           WLAN_SET_EZMESH_CREATE_BRIDGE_CONTEXT, bridge_id);
  ds_system_call(cmd, strlen(cmd));

  return;
} /* End CreateEZMeshBridgeVLANContext */

/*===========================================================================
FUNCTION DeleteEZMeshBridgeVLANContext()
===========================================================================*/
/** @ingroup qcmap_delete_ezmesh_bridge_vlan_context

  deletes bridge vlan context linked to ezmesh

  @param[in]      bridge_id                 bridge vlan ID

  @return
  void
*/
/*=========================================================================*/
void QCMAP_LAN_Client::DeleteEZMeshBridgeVLANContext
(
  const int16_t bridge_id
)
{
  char cmd[QCMAP_MAX_COMMAND_LEN] = {0};

  /* Send bridge_id and profile_handle information to lan config */
  snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "%s %s %u", VLAN_CONFIG,
           WLAN_SET_EZMESH_DELETE_BRIDGE_CONTEXT, bridge_id);
  ds_system_call(cmd, strlen(cmd));

  return;
}

/*===========================================================================
  FUNCTION SetEZMeshConfig()
===========================================================================*/
/** @ingroup qcmap_set_ezmesh_config

  Set EZMesh upon getting request from QCMAP.

  @param[in]      boolean new_config
  @param[in]      qcmap_ezmesh_config *ezmesh_config

  @return
  TRUE -- Success \n
  FALSE -- Failure \n
*/
/*=========================================================================*/
boolean QCMAP_LAN_Client::SetEZMeshConfig
(
  qcmap_msgr_ezmesh_mode_enum_v01   enable,
  boolean                           new_config,
  qcmap_ezmesh_config               *ezmesh_config,
  qmi_error_type_v01                *qmi_err_num
)
{
  char command[QCMAP_MAX_COMMAND_LEN] = {'\0'};
  char tmp[MAX_COMMAND_STR_LEN] = {'\0'};
  qcmap_msgr_ezmesh_capability_enum_v01 capability = 0;
  boolean is_ezmesh_r2_cfg_valid = false;
  qcmap_msgr_ezmesh_r2_config_v01 ezmesh_r2_config;
  uint32_t ap_count_2g = 0, ap_count_5g = 0, ap_count_6g = 0;
  uint32_t timeout_value = 0, retry = 0;
  uint32_t ap_cfg_len = 0;
  uint32_t max_timeout = QCMAP_EZMESH_BRING_UP_MAX_TIMEOUT;
  boolean is_ezmesh_started = false;
  char result[QCMAP_MAX_SCAN_SIZE] = {'\0'};
  uint8_t  iface_ap_type = QCMAP_MSGR_PRIMARY_AP_V01;
  int i = 0;

  /*Validate ezmesh config */
  if(!QCMAP_LAN_Client::ValidateEZMeshConfig(ezmesh_config, enable))
  {
    LOG_MSG_ERROR("Invalid EZMesh config",0,0,0);
    *qmi_err_num = QMI_ERR_INTERNAL_V01;
    return false;
  }

  if (enable == QCMAP_MSGR_EZMESH_ENABLE_V01)
  {
    /* Set wlan to AP mode */
    snprintf(command, QCMAP_MAX_COMMAND_LEN, "%s %s %d",
             WLAN_CONFIG_FILE, WLAN_SET_MODE,  QCMAP_MSGR_WLAN_MODE_AP_V01);
    ds_system_call(command, strlen(command));

    if (new_config)
    {
      /* Set ezmesh enable,capability config */
      memset(command, 0, QCMAP_MAX_COMMAND_LEN);
      snprintf(command, QCMAP_MAX_COMMAND_LEN, "%s %s %d %d %d",
               WLAN_EZMESH_CONFIG_FILE, WLAN_SET_EZMESH_CONFIG, enable,
               ezmesh_config->capability, ezmesh_config->ezmesh_cfg.ap_config_len);
      ds_system_call(command, strlen(command));

      for(i = 0; i < ezmesh_config->ezmesh_cfg.ap_config_len; i++,iface_ap_type++)
      {
        memset(command, 0, QCMAP_MAX_COMMAND_LEN);
        if (iface_ap_type == QCMAP_MSGR_ALL_AP_V01)
          iface_ap_type++;
        ezmesh_config->ezmesh_cfg.ap_config[i].wlan_ap_type = iface_ap_type;

        snprintf(command, QCMAP_MAX_COMMAND_LEN, "%s %s %d %d %d %d",
                 WLAN_EZMESH_CONFIG_FILE, WLAN_SET_EZMESH_AP_CONFIG,
                 ezmesh_config->ezmesh_cfg.ap_config[i].band, ezmesh_config->ezmesh_cfg.ap_config[i].guest_ap_profile,
                 ezmesh_config->ezmesh_cfg.ap_config[i].ap_type, ezmesh_config->ezmesh_cfg.ap_config[i].wlan_ap_type);
        ds_system_call(command, strlen(command));
        if (ezmesh_config->ezmesh_cfg.ap_config[i].band == QCMAP_WLAN_2GHz_BAND)
        {
          ap_count_2g++;
        }
        else if(ezmesh_config->ezmesh_cfg.ap_config[i].band == QCMAP_WLAN_5GHz_BAND)
        {
          ap_count_5g++;
        }
        else
        {
          ap_count_6g++;
        }
      }

      /* update the 2g/5g/6g ap_count */
      memset(command, 0, QCMAP_MAX_COMMAND_LEN);
      snprintf(command, QCMAP_MAX_COMMAND_LEN, "%s %s %d %d %d",
               WLAN_EZMESH_CONFIG_FILE, WLAN_SET_EZMESH_PER_BAND_COUNT, ap_count_2g, ap_count_5g, ap_count_6g);
      ds_system_call(command, strlen(command));

      memset(command, 0, QCMAP_MAX_COMMAND_LEN);
      snprintf(command, QCMAP_MAX_COMMAND_LEN, "%s %s %d",
               WLAN_EZMESH_CONFIG_FILE, WLAN_SET_EZMESH_TRAFFIC_SEPARATION_CONFIG, ezmesh_config->is_ezmesh_r2_cfg_valid);
      ds_system_call(command, strlen(command));

      if (ezmesh_config->is_ezmesh_r2_cfg_valid &&
          IS_EZMESH_CAPABILITY_R2_R3_R4(ezmesh_config->capability))
      {
        memset(command, 0, QCMAP_MAX_COMMAND_LEN);
        snprintf(command, QCMAP_MAX_COMMAND_LEN, "%s %s %d",
                 WLAN_EZMESH_CONFIG_FILE, WLAN_SET_EZMESH_VLAN_CONFIG, ezmesh_config->ezmesh_r2_cfg.mapping_len);
        for (i = 0; i < ezmesh_config->ezmesh_r2_cfg.mapping_len; i++)
        {
          snprintf(tmp, MAX_COMMAND_STR_LEN," %d %d",
                   ezmesh_config->ezmesh_r2_cfg.mapping[i].vlan_id, ezmesh_config->ezmesh_r2_cfg.mapping[i].fh_ap_type);
          strlcat(command, tmp, QCMAP_MAX_COMMAND_LEN);
          memset(tmp, 0, MAX_COMMAND_STR_LEN);
        }
        ds_system_call(command, strlen(command));

        for (i = 0; i < ezmesh_config->ezmesh_r2_cfg.mapping_len; i++)
        {
          /* Update bridge-vlan with network, dhcp, firewall uci configs */
          if (ezmesh_config->ezmesh_r2_cfg.mapping[i].fh_ap_type != QCMAP_MSGR_PRIMARY_FRONTHAUL_AP_V01)
          {
            QCMAP_LAN_Client::CreateEZMeshBridgeVLANContext(ezmesh_config->ezmesh_r2_cfg.mapping[i].vlan_id);
            QCMAP_LAN_Client::SetBridgeVLANContext(ezmesh_config->ezmesh_r2_cfg.mapping[i].vlan_id);
          }
        }
      }

      /* Update IPA about ezmesh enablement */
      SetIPAEZMeshMode(enable, ezmesh_config->capability, ezmesh_config->is_ezmesh_r2_cfg_valid);
    }
    else
    {
      UCI_EZMESH_GET_INT_OPTION(ap_cfg_len, "ezmesh", "ap_config_len");
      if (ap_cfg_len == 0)
      {
        LOG_MSG_ERROR("EZMesh is not configured before, please configure with new config",0,0,0);
        *qmi_err_num = QMI_ERR_INTERNAL_V01;
        return false;
      }
      /* set ezmesh enable flag */
      memset(command, 0, QCMAP_MAX_COMMAND_LEN);
      snprintf(command, QCMAP_MAX_COMMAND_LEN, "%s %s %d",
               WLAN_EZMESH_CONFIG_FILE, WLAN_SET_EZMESH_CONFIG, enable);
      ds_system_call(command, strlen(command));

      UCI_EZMESH_GET_INT_OPTION(capability, "ezmesh", "capability");
      UCI_EZMESH_GET_INT_OPTION(is_ezmesh_r2_cfg_valid, "r2_config", "traffic_separation");
      if (is_ezmesh_r2_cfg_valid &&
          IS_EZMESH_CAPABILITY_R2_R3_R4(capability))
      {
          QCMAP_LAN_Client::GetEZMeshR2Config(&ezmesh_r2_config);
        /* Update bridge-vlan with network, dhcp, firewall uci configs */
        for (i = 0; i < ezmesh_r2_config.mapping_len; i++)
        {
          if (ezmesh_r2_config.mapping[i].fh_ap_type != QCMAP_MSGR_PRIMARY_FRONTHAUL_AP_V01)
          {
            QCMAP_LAN_Client::CreateEZMeshBridgeVLANContext(ezmesh_r2_config.mapping[i].vlan_id);
            QCMAP_LAN_Client::SetBridgeVLANContext(ezmesh_r2_config.mapping[i].vlan_id);
          }
        }
      }

      /* Update IPA about ezmesh enablement */
      SetIPAEZMeshMode(enable, capability, is_ezmesh_r2_cfg_valid);
    }

    /* Update IPA about ezmesh service priority state */
    if (IS_EZMESH_CAPABILITY_R3_R4(ezmesh_config->capability) &&
        ezmesh_config->is_service_prioritization_state_valid)
    {
      if(!QCMAP_LAN_Client::SetEZMeshServicePriority(ezmesh_config->is_service_prioritization_state_valid,
                                                     ezmesh_config->service_prioritization_state,
                                                     qmi_err_num))
      {
        LOG_MSG_ERROR("EZMesh Service Prioritization update failed",0,0,0);
        return false;
      }
    }

    if (new_config)
    {
      /* Call the repacd script with start */
      memset(command, 0, QCMAP_MAX_COMMAND_LEN);
      snprintf(command, QCMAP_MAX_COMMAND_LEN, "%s %s",
               WLAN_EZMESH_CONFIG_FILE, WLAN_SET_EZMESH_REPACD_START);
      ds_system_call(command, strlen(command));
    }
    else
    {
      /* Call the repacd script with start */
      memset(command, 0, QCMAP_MAX_COMMAND_LEN);
      snprintf(command, QCMAP_MAX_COMMAND_LEN, "%s %s",
               WLAN_EZMESH_CONFIG_FILE, WLAN_SET_EZMESH_REPACD_RESTART);
      ds_system_call(command, strlen(command));
    }

    /* Read the timeout value from qcmap_ezmesh uci and update max_timeout if value is > 90sec */
    UCI_EZMESH_GET_INT_OPTION(timeout_value, "ezmesh", "bringup_timeout");
    if (timeout_value > QCMAP_EZMESH_BRING_UP_MAX_TIMEOUT)
      max_timeout = timeout_value;

    while(retry <= (max_timeout/QCMAP_EZMESH_SLEEP_INTERVAL))
    {
      sleep(QCMAP_EZMESH_SLEEP_INTERVAL);
      memset(command, 0, QCMAP_MAX_COMMAND_LEN);
      snprintf(command, QCMAP_MAX_COMMAND_LEN, "%s", IS_EZMESH_PROCESS_RUNNING);
      memset(result, 0, QCMAP_MAX_SCAN_SIZE);
      if (ExecuteSystemCmd((const char *)command, result, sizeof(result)))
      {
        if (strlen(result) != 0)
        {
          LOG_MSG_INFO1("EZMesh started successfully with pid: %s", result, 0, 0);
          is_ezmesh_started = true;
          break;
        }
      }
      retry++;
    }
    if (is_ezmesh_started)
    {
      /* Update the iface names to qcmap_ezmesh uci config */
      memset(command, 0, QCMAP_MAX_COMMAND_LEN);
      snprintf(command, QCMAP_MAX_COMMAND_LEN, "%s %s",
               WLAN_EZMESH_CONFIG_FILE, WLAN_SET_EZMESH_UPDATE_IFACES_CONFIG);
      ds_system_call(command, strlen(command));

      /* Enable WLAN on bootup flag to bring ezmesh up on bootup */
      QCMAP_LAN_Client::SetWLANBootupConfigEx(QCMAP_MSGR_ENABLE_ON_BOOT_V01, qmi_err_num);
    }
    else
    {
      LOG_MSG_ERROR("EZMesh daemon is not started successfully after max_timeout %d, so Disabling the EZMesh", max_timeout, 0, 0);
      QCMAP_LAN_Client::DisableEZMesh(QCMAP_MSGR_EZMESH_DISABLE_V01);
      *qmi_err_num = QMI_ERR_INTERNAL_V01;
      return false;
    }
  }
  else
  {
    /* Update whether user wants to clean-up the ezmesh config or not */
    memset(command, 0, QCMAP_MAX_COMMAND_LEN);
    snprintf(command, MAX_COMMAND_STR_LEN, "/etc/data/uci_ex.sh set qcmap_ezmesh.@ezmesh[0].clean_up_config='%d'",new_config);
    ds_system_call(command, strlen(command));

    QCMAP_LAN_Client::DisableEZMesh(QCMAP_MSGR_EZMESH_DISABLE_V01);
  }

  return true;
}

/*===========================================================================
  FUNCTION GetEZMeshR2Config()
===========================================================================*/
/** @ingroup qcmap_get_ezmesh_config

  Get EZMesh R2 config upon getting request.

  @param[in]      qcmap_msgr_ezmesh_r2_config_v01 *ezmesh_r2_config

  @return
  TRUE -- Success \n
  FALSE -- Failure \n
*/
/*=========================================================================*/
void QCMAP_LAN_Client::GetEZMeshR2Config
(
  qcmap_msgr_ezmesh_r2_config_v01 *ezmesh_r2_config
)
{
  char result[QCMAP_MAX_SCAN_SIZE] = {'\0'};

  memset(ezmesh_r2_config, 0, sizeof(qcmap_msgr_ezmesh_r2_config_v01));
  UCI_EZMESH_GET_INT_OPTION(ezmesh_r2_config->mapping_len, "r2_config", "no_of_vlans");
  for (int i = 0; i < ezmesh_r2_config->mapping_len; i++)
  {
    switch(i)
    {
      case 0:
        UCI_EZMESH_GET_INT_OPTION(ezmesh_r2_config->mapping[i].vlan_id, "r2_config", "primaryap_vlan_id");
        UCI_EZMESH_GET_INT_OPTION(ezmesh_r2_config->mapping[i].fh_ap_type, "r2_config", "primaryap_fh_type");
        break;

      case 1:
        UCI_EZMESH_GET_INT_OPTION(ezmesh_r2_config->mapping[i].vlan_id, "r2_config", "guestapone_vlan_id");
        UCI_EZMESH_GET_INT_OPTION(ezmesh_r2_config->mapping[i].fh_ap_type, "r2_config", "guestapone_fh_type");
        break;

      case 2:
        UCI_EZMESH_GET_INT_OPTION(ezmesh_r2_config->mapping[i].vlan_id, "r2_config", "guestaptwo_vlan_id");
        UCI_EZMESH_GET_INT_OPTION(ezmesh_r2_config->mapping[i].fh_ap_type, "r2_config", "guestaptwo_fh_type");
        break;

      case 3:
        UCI_EZMESH_GET_INT_OPTION(ezmesh_r2_config->mapping[i].vlan_id, "r2_config", "guestapthree_vlan_id");
        UCI_EZMESH_GET_INT_OPTION(ezmesh_r2_config->mapping[i].fh_ap_type, "r2_config", "guestapthree_fh_type");
        break;

      default:
        LOG_MSG_ERROR("Invalid vlan mapping index received: %d",i,0,0);
    }
  }

  return;
}

/*===========================================================================
  FUNCTION GetEZMeshConfig()
===========================================================================*/
/** @ingroup qcmap_get_ezmesh_config

  Get EZMesh upon getting request from QCMAP.

  @param[in]      qcmap_msgr_ezmesh_mode_enum_v01 *ezmesh_status
  @param[in]      qcmap_ezmesh_config *ezmesh_config

  @return
  TRUE -- Success \n
  FALSE -- Failure \n
*/
/*=========================================================================*/
boolean QCMAP_LAN_Client::GetEZMeshConfig
(
  qcmap_msgr_ezmesh_mode_enum_v01  *ezmesh_status,
  qcmap_ezmesh_config     *ezmesh_config,
  qmi_error_type_v01      *qmi_err_num
)
{
  char result[QCMAP_MAX_SCAN_SIZE] = {'\0'};
  qcmap_msgr_ezmesh_r2_config_v01 ezmesh_r2_config;
  qcmap_msgr_ezmesh_mode_enum_v01     enable;
  uint16 wlan_ap_type;

  if(ezmesh_config == NULL)
  {
    LOG_MSG_ERROR("NULL pointer passed ",0,0,0);
    *qmi_err_num = QMI_ERR_INVALID_ARG_V01;
    return false;
  }

  UCI_EZMESH_GET_INT_OPTION(enable, "ezmesh", "enable_state");
  *ezmesh_status = enable;
  UCI_EZMESH_GET_INT_OPTION(ezmesh_config->capability, "ezmesh", "capability");
  UCI_EZMESH_GET_INT_OPTION(ezmesh_config->ezmesh_cfg.ap_config_len, "ezmesh", "ap_config_len");
  if (ezmesh_config->ezmesh_cfg.ap_config_len > 0)
    ezmesh_config->is_ezmesh_cfg_valid = true;

  for(int i = 0; i < ezmesh_config->ezmesh_cfg.ap_config_len; i++)
  {
    switch(i)
    {
      case 0:
        UCI_EZMESH_GET_INT_OPTION(ezmesh_config->ezmesh_cfg.ap_config[i].band, "primaryap", "band");
        UCI_EZMESH_GET_INT_OPTION(ezmesh_config->ezmesh_cfg.ap_config[i].guest_ap_profile, "primaryap", "accessprofile");
        UCI_EZMESH_GET_INT_OPTION(ezmesh_config->ezmesh_cfg.ap_config[i].ap_type, "primaryap", "ezmesh_aptype");
        UCI_EZMESH_GET_INT_OPTION(wlan_ap_type, "primaryap", "wlan_ap_type");
        break;

      case 1:
        UCI_EZMESH_GET_INT_OPTION(ezmesh_config->ezmesh_cfg.ap_config[i].band, "guestap1", "band");
        UCI_EZMESH_GET_INT_OPTION(ezmesh_config->ezmesh_cfg.ap_config[i].guest_ap_profile, "guestap1", "accessprofile");
        UCI_EZMESH_GET_INT_OPTION(ezmesh_config->ezmesh_cfg.ap_config[i].ap_type, "guestap1", "ezmesh_aptype");
        UCI_EZMESH_GET_INT_OPTION(wlan_ap_type, "guestap1", "wlan_ap_type");
        break;

      case 2:
        UCI_EZMESH_GET_INT_OPTION(ezmesh_config->ezmesh_cfg.ap_config[i].band, "guestap2", "band");
        UCI_EZMESH_GET_INT_OPTION(ezmesh_config->ezmesh_cfg.ap_config[i].guest_ap_profile, "guestap2", "accessprofile");
        UCI_EZMESH_GET_INT_OPTION(ezmesh_config->ezmesh_cfg.ap_config[i].ap_type, "guestap2", "ezmesh_aptype");
        UCI_EZMESH_GET_INT_OPTION(wlan_ap_type, "guestap2", "wlan_ap_type");
        break;

      case 3:
        UCI_EZMESH_GET_INT_OPTION(ezmesh_config->ezmesh_cfg.ap_config[i].band, "guestap3", "band");
        UCI_EZMESH_GET_INT_OPTION(ezmesh_config->ezmesh_cfg.ap_config[i].guest_ap_profile, "guestap3", "accessprofile");
        UCI_EZMESH_GET_INT_OPTION(ezmesh_config->ezmesh_cfg.ap_config[i].ap_type, "guestap3", "ezmesh_aptype");
        UCI_EZMESH_GET_INT_OPTION(wlan_ap_type, "guestap3", "wlan_ap_type");
        break;

      case 4:
        UCI_EZMESH_GET_INT_OPTION(ezmesh_config->ezmesh_cfg.ap_config[i].band, "guestap4", "band");
        UCI_EZMESH_GET_INT_OPTION(ezmesh_config->ezmesh_cfg.ap_config[i].guest_ap_profile, "guestap4", "accessprofile");
        UCI_EZMESH_GET_INT_OPTION(ezmesh_config->ezmesh_cfg.ap_config[i].ap_type, "guestap4", "ezmesh_aptype");
        UCI_EZMESH_GET_INT_OPTION(wlan_ap_type, "guestap4", "wlan_ap_type");
        break;

      case 5:
        UCI_EZMESH_GET_INT_OPTION(ezmesh_config->ezmesh_cfg.ap_config[i].band, "guestap5", "band");
        UCI_EZMESH_GET_INT_OPTION(ezmesh_config->ezmesh_cfg.ap_config[i].guest_ap_profile, "guestap5", "accessprofile");
        UCI_EZMESH_GET_INT_OPTION(ezmesh_config->ezmesh_cfg.ap_config[i].ap_type, "guestap5", "ezmesh_aptype");
        UCI_EZMESH_GET_INT_OPTION(wlan_ap_type, "guestap5", "wlan_ap_type");
        break;

      case 6:
        UCI_EZMESH_GET_INT_OPTION(ezmesh_config->ezmesh_cfg.ap_config[i].band, "guestap6", "band");
        UCI_EZMESH_GET_INT_OPTION(ezmesh_config->ezmesh_cfg.ap_config[i].guest_ap_profile, "guestap6", "accessprofile");
        UCI_EZMESH_GET_INT_OPTION(ezmesh_config->ezmesh_cfg.ap_config[i].ap_type, "guestap6", "ezmesh_aptype");
        UCI_EZMESH_GET_INT_OPTION(wlan_ap_type, "guestap6", "wlan_ap_type");
        break;

      case 7:
        UCI_EZMESH_GET_INT_OPTION(ezmesh_config->ezmesh_cfg.ap_config[i].band, "guestap7", "band");
        UCI_EZMESH_GET_INT_OPTION(ezmesh_config->ezmesh_cfg.ap_config[i].guest_ap_profile, "guestap7", "accessprofile");
        UCI_EZMESH_GET_INT_OPTION(ezmesh_config->ezmesh_cfg.ap_config[i].ap_type, "guestap7", "ezmesh_aptype");
        UCI_EZMESH_GET_INT_OPTION(wlan_ap_type, "guestap7", "wlan_ap_type");
        break;

      case 8:
        UCI_EZMESH_GET_INT_OPTION(ezmesh_config->ezmesh_cfg.ap_config[i].band, "guestap8", "band");
        UCI_EZMESH_GET_INT_OPTION(ezmesh_config->ezmesh_cfg.ap_config[i].guest_ap_profile, "guestap8", "accessprofile");
        UCI_EZMESH_GET_INT_OPTION(ezmesh_config->ezmesh_cfg.ap_config[i].ap_type, "guestap8", "ezmesh_aptype");
        UCI_EZMESH_GET_INT_OPTION(wlan_ap_type, "guestap8", "wlan_ap_type");
        break;

      case 9:
        UCI_EZMESH_GET_INT_OPTION(ezmesh_config->ezmesh_cfg.ap_config[i].band, "guestap9", "band");
        UCI_EZMESH_GET_INT_OPTION(ezmesh_config->ezmesh_cfg.ap_config[i].guest_ap_profile, "guestap9", "accessprofile");
        UCI_EZMESH_GET_INT_OPTION(ezmesh_config->ezmesh_cfg.ap_config[i].ap_type, "guestap9", "ezmesh_aptype");
        UCI_EZMESH_GET_INT_OPTION(wlan_ap_type, "guestap9", "wlan_ap_type");
        break;

      case 10:
        UCI_EZMESH_GET_INT_OPTION(ezmesh_config->ezmesh_cfg.ap_config[i].band, "guestap10", "band");
        UCI_EZMESH_GET_INT_OPTION(ezmesh_config->ezmesh_cfg.ap_config[i].guest_ap_profile, "guestap10", "accessprofile");
        UCI_EZMESH_GET_INT_OPTION(ezmesh_config->ezmesh_cfg.ap_config[i].ap_type, "guestap10", "ezmesh_aptype");
        UCI_EZMESH_GET_INT_OPTION(wlan_ap_type, "guestap10", "wlan_ap_type");
        break;

      case 11:
        UCI_EZMESH_GET_INT_OPTION(ezmesh_config->ezmesh_cfg.ap_config[i].band, "guestap11", "band");
        UCI_EZMESH_GET_INT_OPTION(ezmesh_config->ezmesh_cfg.ap_config[i].guest_ap_profile, "guestap11", "accessprofile");
        UCI_EZMESH_GET_INT_OPTION(ezmesh_config->ezmesh_cfg.ap_config[i].ap_type, "guestap11", "ezmesh_aptype");
        UCI_EZMESH_GET_INT_OPTION(wlan_ap_type, "guestap11", "wlan_ap_type");
        break;

      case 12:
        UCI_EZMESH_GET_INT_OPTION(ezmesh_config->ezmesh_cfg.ap_config[i].band, "guestap12", "band");
        UCI_EZMESH_GET_INT_OPTION(ezmesh_config->ezmesh_cfg.ap_config[i].guest_ap_profile, "guestap12", "accessprofile");
        UCI_EZMESH_GET_INT_OPTION(ezmesh_config->ezmesh_cfg.ap_config[i].ap_type, "guestap12", "ezmesh_aptype");
        UCI_EZMESH_GET_INT_OPTION(wlan_ap_type, "guestap12", "wlan_ap_type");
        break;

      case 13:
        UCI_EZMESH_GET_INT_OPTION(ezmesh_config->ezmesh_cfg.ap_config[i].band, "guestap13", "band");
        UCI_EZMESH_GET_INT_OPTION(ezmesh_config->ezmesh_cfg.ap_config[i].guest_ap_profile, "guestap13", "accessprofile");
        UCI_EZMESH_GET_INT_OPTION(ezmesh_config->ezmesh_cfg.ap_config[i].ap_type, "guestap13", "ezmesh_aptype");
        UCI_EZMESH_GET_INT_OPTION(wlan_ap_type, "guestap13", "wlan_ap_type");
        break;

      default:
        LOG_MSG_ERROR("Invalid ap config index received: %d",i,0,0);
    }
    ezmesh_config->ezmesh_cfg.ap_config[i].wlan_ap_type = GetWLANEXIfaceIndex(wlan_ap_type);

  }

  UCI_EZMESH_GET_INT_OPTION(ezmesh_config->is_ezmesh_r2_cfg_valid, "r2_config", "traffic_separation");
  if (ezmesh_config->is_ezmesh_r2_cfg_valid &&
      IS_EZMESH_CAPABILITY_R2_R3_R4(ezmesh_config->capability))
  {
    QCMAP_LAN_Client::GetEZMeshR2Config(&ezmesh_r2_config);
    memcpy(&ezmesh_config->ezmesh_r2_cfg, &ezmesh_r2_config, sizeof(qcmap_msgr_ezmesh_r2_config_v01));
  }

  if (IS_EZMESH_CAPABILITY_R3_R4(ezmesh_config->capability))
  {
    ezmesh_config->is_service_prioritization_state_valid = true;
    UCI_EZMESH_GET_INT_OPTION(ezmesh_config->service_prioritization_state, "r3_config", "service_priority");
  }

  return true;
}

/*=====================================================================
  FUNCTION ActivateEZMeshHostapdConfig
======================================================================*/
/*!
@brief
  - Activates the hostapd configuration of ezmesh from qcmap client

  @param[in]      qcmap_msgr_ezmesh_mode_enum_v01 *ap_list
  @param[in]      qcmap_activate_hostapd_action_enum action_type

  @return
  TRUE -- Success \n
  FALSE -- Failure \n
*/
/*=========================================================================*/
boolean QCMAP_LAN_Client::ActivateEZMeshHostapdConfig
(
  qcmap_hostapd_ap_config_list *ap_list,
  qcmap_activate_hostapd_action_enum action_type,
  qmi_error_type_v01      *qmi_err_num
)
{
  char cmd[QCMAP_MAX_COMMAND_LEN] = {'\0'};
  char result[QCMAP_MAX_SCAN_SIZE] = {'\0'};

  snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "%s %s %d %d",
           WLAN_EZMESH_CONFIG_FILE, ACTIVATE_EZMESH_HOSTAPD_CONFIG,
           ap_list->list_len, action_type);

  if (ExecuteSystemCmd((const char *)cmd, result, sizeof(result)))
  {
    if (!strstr(result, ACTIVATE_HOSTAPD_VALID))
    {
      LOG_MSG_ERROR("Failed to activate hostapd for ezmesh", 0, 0, 0);
      *qmi_err_num = QMI_ERR_INVALID_ARG_V01;
      return false;
    }
  }
  else
  {
    *qmi_err_num = QMI_ERR_INVALID_ARG_V01;
    return false;
  }

  if ((ap_list->list_len == 1) && (ap_list->ap_type[0] == QCMAP_ALL_AP))
  {
    memset(cmd, 0, sizeof(cmd));
    snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "%s %s %d",
             WLAN_EZMESH_CONFIG_FILE, ACTIVATE_EZMESH_HOSTAPD_AP_CONFIG,
             ap_list->ap_type[0]);
    if (ExecuteSystemCmd((const char *)cmd, result, sizeof(result)))
    {
      if (!strstr(result, ACTIVATE_HOSTAPD_SUCCESS))
      {
        LOG_MSG_ERROR("Failed to activate hostapd for ezmesh ALL APs", 0, 0, 0);
        *qmi_err_num = QMI_ERR_INVALID_ARG_V01;
        return false;
      }
    }
    else
    {
      *qmi_err_num = QMI_ERR_INVALID_ARG_V01;
      return false;
    }
  }
  else
  {
    for (int i = 0; i < ap_list->list_len; i++)
    {
      memset(cmd, 0, sizeof(cmd));
      snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "%s %s %d",
               WLAN_EZMESH_CONFIG_FILE, ACTIVATE_EZMESH_HOSTAPD_AP_CONFIG,
               ap_list->ap_type[i]);
      ds_system_call(cmd, strlen(cmd));
    }
/*
    memset(cmd, 0, sizeof(cmd));
    snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "%s %s",
             WLAN_EZMESH_CONFIG_FILE, ACTIVATE_EZMESH_HOSTAPD_RESTART);
    ds_system_call(cmd, strlen(cmd));
*/
  }

  return true;
}

/*===========================================================================
  FUNCTION SetEZMeshServicePriority
  ===========================================================================*/
/*!
  @brief
  Sets EZMesh Service Prioritization state config

  @param[in]     uint8_t             service_prioritization_state_valid,
  @param[in]     uint8_t             service_prioritization_state,
  @param[in]     qmi_error_type_v01  *qmi_err_num

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
boolean QCMAP_LAN_Client::SetEZMeshServicePriority
(
  uint8_t                 service_prioritization_state_valid,
  uint8_t                 service_prioritization_state,
  qmi_error_type_v01      *qmi_err_num
)
{
  char command[QCMAP_MAX_COMMAND_LEN] = {'\0'};
  char result[QCMAP_MAX_SCAN_SIZE] = {'\0'};
  uint8_t ezmesh_enable = 0;
  qcmap_msgr_ezmesh_capability_enum_v01 capability = 0;

  UCI_EZMESH_GET_INT_OPTION(ezmesh_enable, "ezmesh", "enable_state");
  UCI_EZMESH_GET_INT_OPTION(capability, "ezmesh", "capability");
  if (!ezmesh_enable && !IS_EZMESH_CAPABILITY_R3_R4(capability))
  {
    LOG_MSG_ERROR("EZMesh is not enable or EZMesh is not in R3/R4 capability",0,0,0);
    *qmi_err_num = QMI_ERR_INVALID_ARG_V01;
    return false;
  }

  if (!service_prioritization_state_valid)
  {
    LOG_MSG_ERROR("Invalid service prioritization state",0,0,0);
    *qmi_err_num = QMI_ERR_INVALID_ARG_V01;
    return false;
  }

  snprintf(command, QCMAP_MAX_COMMAND_LEN, "%s %s %d",
           WLAN_EZMESH_CONFIG_FILE, WLAN_SET_EZMESH_R3_SERVICE_PRIORITY,
           service_prioritization_state);
  ds_system_call(command, strlen(command));

  if (SetIPAEZMeshSerivcePriorityState(service_prioritization_state) == false)
  {
    LOG_MSG_INFO1("Unable to update Ezmesh service priority state to IPACM", 0, 0, 0);
    *qmi_err_num = QMI_ERR_INTERNAL_V01;
    return false;
  }

  return true;
}

