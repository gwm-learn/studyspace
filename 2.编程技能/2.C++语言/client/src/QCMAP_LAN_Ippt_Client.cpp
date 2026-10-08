/*====================================================

FILE:  QCMAP_LAN_Ippt_Client.cpp

SERVICES:
QCMAP LAN Ippt Client Implementation

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

/*===========================================================================
FUNCTION ResetIPPTFeatureMode()
===========================================================================*/
/** @ingroup section_ResetIPPTFeatureMode

  Reset IPPT feature mode provided by the user/caller

  @datatypes
  qcmap_lan_client_feature_mode_config

  @param[in] qcmap_lan_client_feature_mode_config *feature_mode_config
  @param[out] qmi_error_type_v01 *qmi_err_num

  @return
  TRUE -- Success \n
  FALSE -- Failure \n
*/
/*=========================================================================*/
boolean QCMAP_LAN_Client::ResetIPPTFeatureMode
(
  qcmap_lan_client_feature_mode_config *feature_mode_config,
  qmi_error_type_v01                  *qmi_err_num
)
{
  char cmd[QCMAP_MAX_COMMAND_LEN] = {0};
  char result[QCMAP_MAX_SCAN_SIZE] = {0};
  
  /* Check if there are any active PDN's */
  snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "ls -d /tmp/ipv* | wc -l");
  if (ExecuteSystemCmd((const char *)cmd, result,
                        sizeof(result)))
  {
    int num = atoi(result);
    if (num > 0)
    {
      /*indicates active backhaul */
      LOG_MSG_ERROR(" Active Backhaul's found. Exiting", 0, 0, 0);
      *qmi_err_num = QMI_ERR_INTERNAL_V01;
      return false;
    }
    LOG_MSG_INFO1("No active Backhaul's found!", 0, 0, 0);

    /* Check if with_nat configuration is WITH_NAT or WITHOUT_NAT */
    memset(cmd, 0, QCMAP_MAX_COMMAND_LEN);
    memset(result, 0, QCMAP_MAX_SCAN_SIZE);
    snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "%s %s",
              UCI_GET_COMMAND, QCMAP_IP_PT_FEATURE_MODE);
    if (ExecuteSystemCmd((const char *)cmd, result,
                    sizeof(result)))
    {
      int feature_mode = atoi(result);
      if ((qcmap_lan_ip_passthrough_feature_mode_enum)feature_mode == IP_PASSTHROUGH_MODE_WITH_NAT)
      {
        /* Indicates IPPT feature mode is already in WITH_NAT configuration */
        LOG_MSG_INFO1("IPPT feature mode is already in WITH_NAT mode.", 0, 0, 0);
        memset(cmd, 0, QCMAP_MAX_COMMAND_LEN);
        snprintf(cmd, QCMAP_MAX_COMMAND_LEN,"%s %s",FIREWALL_CONFIG_FILE, RESET_FIREWALL_ON_IPPT_WO_NAT);
        ds_system_call(cmd, strlen(cmd));
        return true;
      }
      else if ((qcmap_lan_ip_passthrough_feature_mode_enum)feature_mode == IP_PASSTHROUGH_MODE_WITHOUT_NAT)
      {
        /* Indicates IPPT feature mode is in WITHOUT_NAT configuration
           1. Need to reset DHCP ignore option on all bridges
           2. Need to Set feature mode to WITH_NAT as default configuration */

        /* Set the feature mode to default value */
        memset(cmd, 0, QCMAP_MAX_COMMAND_LEN);
        snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "%s %s=%d",
                 UCI_SET_COMMAND, QCMAP_IP_PT_FEATURE_MODE, IP_PASSTHROUGH_MODE_WITH_NAT);
        ds_system_call(cmd, strlen(cmd));

        /* Reset DHCP ignore option on all bridges */
        SetResetDHCPIgnoreOption(false);

        /* Commit uci */
        ExecuteUCICommit();

        memset(cmd, 0, QCMAP_MAX_COMMAND_LEN);
        snprintf(cmd, QCMAP_MAX_COMMAND_LEN,"%s %s",FIREWALL_CONFIG_FILE,RESET_FIREWALL_ON_IPPT_WO_NAT);
        ds_system_call(cmd, strlen(cmd));
        return true;
      }
    }
    else
    {
      LOG_MSG_ERROR("Unable to query IP Passthrough feature mode", 0, 0, 0);
      *qmi_err_num = QMI_ERR_INTERNAL_V01;
      return false;
    }
  }
  else
  {
    LOG_MSG_ERROR(" Unable to query active PDN's", 0, 0, 0);
    *qmi_err_num = QMI_ERR_INTERNAL_V01;
    return false;
  }
}

/*===========================================================================
FUNCTION GetIPPTFeatureMode()
===========================================================================*/
/** @ingroup section_GetIPPTFeatureMode

  Get IPPT feature mode requested by the user/caller

  @datatypes
  qcmap_lan_client_feature_mode_config

  @param[in] uint64_t  *enabled_features
  @param[in] qcmap_lan_client_feature_mode_config *feature_mode_config
  @param[out] qmi_error_type_v01 *qmi_err_num

  @return
  TRUE -- Success \n
  FALSE -- Failure \n
*/
/*=========================================================================*/
boolean QCMAP_LAN_Client::GetIPPTFeatureMode
(
  uint64_t                             *enabled_features,
  qcmap_lan_client_feature_mode_config *feature_mode_config,
  qmi_error_type_v01                   *qmi_err_num
)
{
  char cmd[QCMAP_MAX_COMMAND_LEN] = {0};
  char result[QCMAP_MAX_SCAN_SIZE] = {0};
  
  snprintf(cmd, QCMAP_MAX_COMMAND_LEN, " %s %s",
            UCI_GET_COMMAND, QCMAP_IP_PT_FEATURE_MODE);
  if (ExecuteSystemCmd((const char *)cmd, result,
                        sizeof(result)))
  {
    *enabled_features |= QCMAP_MSGR_MASK_ENABLE_FEATURE_IP_PASSTHROUGH_V01;
    int feature_mode = atoi(result);
    if ((qcmap_lan_ip_passthrough_feature_mode_enum)feature_mode == IP_PASSTHROUGH_MODE_WITH_NAT)
    {
      /* Populate feature mode config */
      feature_mode_config->ip_passthrough_feature_valid = true;
      feature_mode_config->ip_passthrough_feature_mode = IP_PASSTHROUGH_MODE_WITH_NAT;
    }
    else if ((qcmap_lan_ip_passthrough_feature_mode_enum)feature_mode == IP_PASSTHROUGH_MODE_WITHOUT_NAT)
    {
      /* Populate feature mode config */
      feature_mode_config->ip_passthrough_feature_valid = true;
      feature_mode_config->ip_passthrough_feature_mode = IP_PASSTHROUGH_MODE_WITHOUT_NAT;
    }
    
    return true;
  }
  else
  {
    LOG_MSG_ERROR("Get IP Passtrough feature mode failed", 0, 0, 0);
    *qmi_err_num = QMI_ERR_INTERNAL_V01;
    return false;
  }
}

/*===========================================================================
FUNCTION SetIPPTFeatureMode()
===========================================================================*/
/** @ingroup section_SetIPPTFeatureMode

  Set IPPT feature mode provided by the user/caller

  @datatypes
  qcmap_lan_client_feature_mode_config

  @param[in] qcmap_lan_client_feature_mode_config *feature_mode_config
  @param[out] qmi_error_type_v01 *qmi_err_num

  @return
  TRUE -- Success \n
  FALSE -- Failure \n
*/
/*=========================================================================*/
boolean QCMAP_LAN_Client::SetIPPTFeatureMode
(
  qcmap_lan_client_feature_mode_config *feature_mode_config,
  qmi_error_type_v01 * qmi_err_num
)
{
  char cmd[QCMAP_MAX_COMMAND_LEN] = {0};
  char result[QCMAP_MAX_SCAN_SIZE] = {0};
  
   /* Check if there are any active PDN's */
  snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "ls -d /tmp/ipv* | wc -l");
  if (ExecuteSystemCmd((const char *)cmd, result,
                        sizeof(result)))
  {
    int num = atoi(result);
    if (num > 0)
    {
      /*indicates active backhaul */
      LOG_MSG_ERROR(" Active Backhaul's found. Exiting", 0, 0, 0);
      return false;
    }
    LOG_MSG_INFO1(" No active backhaul's found!", 0, 0, 0);

    /* Check if saved feature_mode is same as user defined feature_mode */
    memset(cmd, 0, QCMAP_MAX_COMMAND_LEN);
    memset(result, 0, QCMAP_MAX_SCAN_SIZE);
    snprintf(cmd, QCMAP_MAX_COMMAND_LEN, " %s %s",
              UCI_GET_COMMAND, QCMAP_IP_PT_FEATURE_MODE);
    if (ExecuteSystemCmd((const char *)cmd, result,
                    sizeof(result)))
    {
      int feature_mode = atoi(result);
      if (feature_mode_config->ip_passthrough_feature_mode == (qcmap_lan_ip_passthrough_feature_mode_enum)feature_mode)
      {
        LOG_MSG_INFO1("User defined feature mode already set", 0, 0, 0);
        return true;
      }
      /* Set the feature mode to user defined value */
      memset(cmd, 0, QCMAP_MAX_COMMAND_LEN);
      snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "%s %s=%d",
               UCI_SET_COMMAND, QCMAP_IP_PT_FEATURE_MODE, feature_mode_config->ip_passthrough_feature_mode);
      ds_system_call(cmd, strlen(cmd));

      /* utility function to iterate over vlan's and set/reset dhcp ignore option */
      /* If WITH_NAT is set, we enable dhcp server on all bridges
         If WITHOUT_NAT is set, we disable dhcp server on all bridges */
      if (feature_mode_config->ip_passthrough_feature_mode == IP_PASSTHROUGH_MODE_WITH_NAT)
      {
        SetResetDHCPIgnoreOption(false);
      }
      else if (feature_mode_config->ip_passthrough_feature_mode == IP_PASSTHROUGH_MODE_WITHOUT_NAT)
      {
        SetResetDHCPIgnoreOption(true);
      }
      /* Commit uci */
      ExecuteUCICommit();
      return true;
    }
    else
    {
      LOG_MSG_ERROR(" Unable to query IP Passthrough feature mode", 0, 0, 0);
      *qmi_err_num = QMI_ERR_INTERNAL_V01;
      return false;
    }
  }
  else
  {
    LOG_MSG_ERROR(" Unable to query active PDN's", 0, 0, 0);
    *qmi_err_num = QMI_ERR_INTERNAL_V01;
    return false;
  }
}

/*===========================================================================
FUNCTION SetIPPassthroughConfig()
===========================================================================*/
/** @ingroup qcmap_set_ip_passthrough_config

  sets ip passthrough config parameters entered by user

  @datatypes
  qcmap_lan_ip_passthrough_mode_enum
  qcmap_lan_ip_passthrough_config

  @param[in]      enable_state              IP Passthrough Enable state \n
  @param[in]      new_config                boolean \n
  @param[in]      ip_passthrough_config     IP Passthrough config struct \n
  @param[in]      default_handle            Default Profile handle \n
  @param[in]      profile_handle            Current Profile Handle

  @return
  TRUE -- Success \n
  FALSE -- Failure \n
*/
/*=========================================================================*/
boolean QCMAP_LAN_Client::SetIPPassthroughConfig
(
  qcmap_lan_ip_passthrough_mode_enum enable_state,
  boolean new_config,
  qcmap_lan_ip_passthrough_config *ip_passthrough_config,
  const uint32_t default_handle,
  qmi_error_type_v01 *qmi_err_num
)
{
  char cmd[QCMAP_MAX_COMMAND_LEN] = {0};
  char str[QCMAP_MAX_COMMAND_LEN] = {0};
  char result[QCMAP_MAX_SCAN_SIZE] = {0};
  char mac_addr_string[QCMAP_LAN_MAC_ADDR_NUM_CHARS] = {0};
  bool mac_addr_non_empty = true;
  int active_ippt = -1, profile_idx = -1;
  int16_t current_bridge_context = -1;
  char bridge_vlan_ids[QCMAP_MAX_SCAN_SIZE] = {0};
  int16_t ip_collision_active = QCMAP_LAN_INVALID;
  int16_t vlan_idx = QCMAP_LAN_INVALID;
  int16_t profile_handle = QCMAP_LAN_INVALID, ippt_bridge_context = QCMAP_LAN_INVALID;
  int error_num;

  /* Get current/global bridge context */
  if (!QCMAP_LAN_Client::GetBridgeVLANContext(&current_bridge_context))
  {
    LOG_MSG_ERROR("Failed to get the current bridge context", 0, 0, 0);
    return false;
  }

  LOG_MSG_INFO1("\nCurrent_bridge_context = %d",current_bridge_context,0,0);

  if (current_bridge_context == 0)
  {
    profile_handle = default_handle;
  }
  else
  {
    /* Get vlan index from qcmap_lan */
    vlan_idx = GetVlanIndex(current_bridge_context);

    /* Get mappped PDN based on vlan index from qcmap_lan */
    if ((profile_handle = GetMappedPDNforVlan(vlan_idx, qmi_err_num))
                                              == QCMAP_LAN_INVALID )
    {
      LOG_MSG_ERROR("Failed while trying to retreive mapped PDN",0,0,0);
      return false;
    }
  }

  LOG_MSG_INFO1("\nbridge_context = %d and mapped to PDN %d",
                  current_bridge_context, profile_handle,0);

  IPv4_DISABLE_NAT_EXCLUSIVITY_CHECK(profile_handle, qmi_err_num, false);

  /* Calling Utility function to check if wan rules are present in uci database */
  if (!CheckIfNetworkRulesExist(NETWORK_WAN, profile_handle))
  {
    LOG_MSG_ERROR("Failed to validate network wan rules presence in uci database"
                  "for profile: %d", profile_handle, 0, 0);
    *qmi_err_num = QMI_ERR_INTERNAL_V01;
    return false;
  }

  /* Check enable_state */
  if (enable_state == QCMAP_LAN_IP_PASSTHROUGH_MODE_UP)
  {
    if (new_config && (NULL == ip_passthrough_config))
    {
      LOG_MSG_ERROR(" Invalid ippt_config pointer passed", 0, 0, 0);
      *qmi_err_num = QMI_ERR_INVALID_ARG_V01;
      return false;
    }

    /* Get the profile index in qcmap_lan */
    profile_idx = GetProfileIndex(profile_handle);
    if (profile_idx == QCMAP_LAN_INVALID)
    {
      /* indicates profile_idx not found */
      LOG_MSG_ERROR(" Profile index for profile: %d not found in qcmap_lan. Exiting!",
                    profile_handle, 0, 0);
      *qmi_err_num = QMI_ERR_INVALID_PROFILE_V01;
      return false;
    }

    /* Check if active_ippt is already set for wan<Current_Profile> */
    active_ippt = GetActiveIPPT(profile_idx);
    if (active_ippt != QCMAP_LAN_INVALID)
    {
      if (active_ippt == 1)
      {
        /* Get the ippt_bridge_context and print that info */
        if (QCMAP_LAN_Client::GetIPPTBridgeContext(profile_idx, &current_bridge_context))
        {
          LOG_MSG_ERROR("IPPT already active on the PDN: %d; ippt_bridge_context: %d",
                        profile_handle, current_bridge_context, 0);
        }
        return false;
      }
    }

    /* If new_config is set */
    if (new_config)
    {
      /* Check if MAC address is empty
         MAC_ADDR is used only for WiFi/Eth/Eth-nic2 device_types */
      if ((ip_passthrough_config->device_type == QCMAP_LAN_DEVICE_TYPE_ANY_AP) ||
          (ip_passthrough_config->device_type == QCMAP_LAN_DEVICE_TYPE_ETHERNET) ||
          (ip_passthrough_config->device_type == QCMAP_LAN_DEVICE_TYPE_ETHERNET_NIC2))
      {
        mac_addr_non_empty = check_non_empty_mac_addr(ip_passthrough_config->mac_addr,
                                                       mac_addr_string);
        if (!mac_addr_non_empty)
        {
          LOG_MSG_ERROR("Empty MAC address", 0, 0, 0);
          *qmi_err_num = QMI_ERR_INVALID_ARG_V01;
          return false;
        }
      }

      /* Check if device_type is ANY_AP(wifi) and current_bh is not def_bh */
      /*NOTE: default handle condition can be removed when IPPT on VLAN over
        WiFi is introduced */
      if ((ip_passthrough_config->device_type == QCMAP_LAN_DEVICE_TYPE_ANY_AP)
           && (profile_handle != default_handle))
      {
        /* IP Passthrough over WiFi is supported only on default bridge */
        LOG_MSG_ERROR("IP Passthrough over WiFi is not supported for PDN: %d",
                       profile_handle, 0, 0);
        return false;
      }

      /* Get saved vlan_id's under wwan handle in qcmap_lan */
      if (!QCMAP_LAN_Client::GetMappedVLANPerPDN(profile_idx,bridge_vlan_ids, qmi_err_num))
      {
        LOG_MSG_ERROR("Failed to obtain bridge_vlan_ids linked to PDN: %d", profile_handle, 0, 0);
        return false;
      }
      /* check if global bridge context is in saved vlan-id's */
      if (!ValidateBridgeContext(bridge_vlan_ids, (const int16_t)current_bridge_context, profile_handle))
      {
        LOG_MSG_ERROR(" The current global bridge context is not mapped to PDN: %d", profile_handle, 0, 0);
        *qmi_err_num = QMI_ERR_INVALID_OPERATION_V01;
        return false;
      }
      /* If yes, then set the new ippt_bridge_context parameter in qcmap_lan */
      /* Store all values */
      switch (ip_passthrough_config->device_type)
      {
        case QCMAP_LAN_DEVICE_TYPE_USB:
          memset(cmd, 0, QCMAP_MAX_COMMAND_LEN);
          snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "%s add_ippt %d USB %d %s",
                    IPPT_FILE,
                    profile_idx,
                    current_bridge_context,
                    ip_passthrough_config->client_device_name);
          ds_system_call(cmd, strlen(cmd));
          LOG_MSG_INFO1("\nDevice Type = USB", 0, 0, 0);
        break;

        case QCMAP_LAN_DEVICE_TYPE_ETHERNET:
          memset(cmd, 0, QCMAP_MAX_COMMAND_LEN);
          snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "%s add_ippt %d ETH %d %s",
                    IPPT_FILE,
                    profile_idx,
                    current_bridge_context,
                    mac_addr_string);
          ds_system_call(cmd, strlen(cmd));
          LOG_MSG_INFO1("\nDevice Type = ETH", 0, 0, 0);
        break;

        case QCMAP_LAN_DEVICE_TYPE_ETHERNET_NIC2:
          memset(cmd, 0, QCMAP_MAX_COMMAND_LEN);
          snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "%s add_ippt %d ETH_NIC2 %d %s",
                    IPPT_FILE,
                    profile_idx,
                    current_bridge_context,
                    mac_addr_string);
          ds_system_call(cmd, strlen(cmd));
          LOG_MSG_INFO1("\nDevice Type = ETH_NIC2", 0, 0, 0);
        break;

        case QCMAP_LAN_DEVICE_TYPE_ANY_AP:
          memset(cmd, 0, QCMAP_MAX_COMMAND_LEN);
          snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "%s add_ippt %d WiFi %d %s",
                    IPPT_FILE,
                    profile_idx,
                    current_bridge_context,
                    mac_addr_string);
          ds_system_call(cmd, strlen(cmd));
          LOG_MSG_INFO1("\nDevice Type = WiFi", 0, 0, 0);
        break;

        case QCMAP_LAN_DEVICE_TYPE_ANY:
          memset(cmd, 0, QCMAP_MAX_COMMAND_LEN);
          snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "%s add_ippt %d Any %d",
                    IPPT_FILE,
                    profile_idx,
                    current_bridge_context);
          ds_system_call(cmd, strlen(cmd));
          LOG_MSG_INFO1("\nDevice Type = Any", 0, 0, 0);
        break;

        default:
          LOG_MSG_ERROR("Invalid Device Type", 0, 0, 0);
          return false;
      }
    }
    else
    {
      /* Case when user did not enter new config */
      /* NOTE: Even though we do not use the config returned,
         this step is required to ensure parameters are present to setup IPPT */
      if (!QCMAP_LAN_Client::CheckIfIPPTConfigExists(ip_passthrough_config, profile_handle))
      {
        LOG_MSG_ERROR("Failed to validate IP Passthrough config presence for"
                      "Profile handle:%d", profile_handle, 0, 0);
        return false;
      }

      /* Get ippt_bridge_context */
      if (!QCMAP_LAN_Client::GetIPPTBridgeContext(profile_idx, &current_bridge_context))
      {
        LOG_MSG_ERROR("Failed to obtain ippt_bridge_context linked to PDN: %d", profile_handle, 0, 0);
        return false;
      }
      /* Get saved vlan_id's under wwan handle in qcmap_lan */
      if (!QCMAP_LAN_Client::GetMappedVLANPerPDN(profile_idx,bridge_vlan_ids, qmi_err_num))
      {
        LOG_MSG_ERROR("Failed to obtain bridge_vlan_ids linked to PDN: %d", profile_handle, 0, 0);
        return false;
      }
      /* check if current bridge context is in saved vlan-id's */
      if (!ValidateBridgeContext(bridge_vlan_ids, (const int16_t)current_bridge_context, profile_handle))
      {
        LOG_MSG_ERROR(" The current global bridge context is not mapped to PDN: %d. Please reset new IPPT configuration",
                       profile_handle, 0, 0);
        return false;
      }
    }

    /* Set ippt_enable state to MODE_UP */
    memset(cmd, 0, QCMAP_MAX_COMMAND_LEN);
    snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "%s set_ippt_state %d %d", IPPT_FILE,
             profile_idx, enable_state);
    ds_system_call(cmd, strlen(cmd));

  } /* End bring-up */

  /* If enable_state is set to MODE-DOWN */
  else if (enable_state == QCMAP_LAN_IP_PASSTHROUGH_MODE_DOWN)
  {
    /* Get the profile index in qcmap_lan */
    profile_idx = GetProfileIndex(profile_handle);
    if (profile_idx == QCMAP_LAN_INVALID)
    {
      /* indicates profile_idx not found */
      LOG_MSG_ERROR(" Profile index for profile: %d not found in qcmap_lan. Exiting!",
                    profile_handle, 0, 0);
      *qmi_err_num = QMI_ERR_INVALID_PROFILE_V01;
      return false;
    }

    /* Get information from GetIPPassthroughConfig() */
    /* NOTE: Even though we do not use the config returned,
         this step is required to ensure parameters are present to disable IPPT */
    if (!QCMAP_LAN_Client::CheckIfIPPTConfigExists(ip_passthrough_config, profile_handle))
    {
      LOG_MSG_ERROR("Failed to validate IP Passthrough config presence for"
                    "Profile handle:%d", profile_handle, 0, 0);
      return false;
    }

    /* Set ippt_enable state to MODE_DOWN */
    memset(cmd, 0, QCMAP_MAX_COMMAND_LEN);
    snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "%s set_ippt_state %d %d", IPPT_FILE,
             profile_idx, enable_state);
    ds_system_call(cmd, strlen(cmd));

    /* Check if active_ippt is set */
    active_ippt = GetActiveIPPT(profile_idx);
    if (active_ippt == QCMAP_LAN_INVALID)
    {
      LOG_MSG_ERROR("Error in retrieving active_ippt value for profile handle: %d",
                    profile_handle, 0, 0);
       *qmi_err_num = QMI_ERR_INVALID_OPERATION_V01;
      return false;
    }

    if (active_ippt == 0)
    {
      LOG_MSG_ERROR("IPPT already disabled on the PDN: %d", profile_handle, 0, 0);
      return true;
    }
  } /* End tear-down */
  else
  {
    LOG_MSG_ERROR("Unknown state value passed. Exiting!", 0, 0, 0);
    *qmi_err_num = QMI_ERR_UNKNOWN_V01;
    return false;
  }


  /* Common Part for Enable/Disable IPPT */
  /* Check if Backhaul on current profile number is already up */
  memset(cmd,    0, QCMAP_MAX_COMMAND_LEN);
  memset(result, 0, QCMAP_MAX_SCAN_SIZE);
  snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "/tmp/ipv4config%d", profile_handle);
  if (!QCMAP_LAN_Client::CheckIfFileExists(cmd))
  {
    LOG_MSG_INFO1("IPV4 Backhaul for PDN: %d not up. Saving the IPPT configuration!",
                  profile_handle, 0, 0);
    return true;
  }

  /* Check if ip_collision is already enabled and update IPPT as required */
  memset(cmd, 0, QCMAP_MAX_COMMAND_LEN);
  memset(result, 0, QCMAP_MAX_SCAN_SIZE);
  if ( QCMAP_LAN_Client::UciGetUtility( owrt_filename[0], "profile", 0,
                                       "ip_collision_active", result, profile_idx) )
  {
    ip_collision_active = atoi(result);
    LOG_MSG_INFO1("ip_collision_active [1:Active, 0:Inactive] %d", ip_collision_active, 0, 0);
    if (ip_collision_active == 1)
    {
      memset(cmd, 0, QCMAP_MAX_COMMAND_LEN);
      memset(result, 0, QCMAP_MAX_SCAN_SIZE);
      snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "%s %s",
                UCI_GET_COMMAND, QCMAP_IP_PT_FEATURE_MODE);
      if (ExecuteSystemCmd((const char *)cmd, result,
                        sizeof(result)))
      {
        int feature_mode = atoi(result);
        if ((qcmap_lan_ip_passthrough_feature_mode_enum)feature_mode
                                    == IP_PASSTHROUGH_MODE_WITH_NAT)
        {
          /* IPPT with NAT not supported in Collision mode.In case IPPT_WITH_NAT is
             enabled when ip_collision is already enabled we ignore IPPT */
          LOG_MSG_ERROR("IPPT with NAT not supported in Collision mode", 0, 0, 0);
          return true;
        }
        else if ((qcmap_lan_ip_passthrough_feature_mode_enum)feature_mode
                                      == IP_PASSTHROUGH_MODE_WITHOUT_NAT)
        {
          /* In case of IPPT_WITHOUT_NAT collision is igonred and IPPT is enabled
             Need to set this flag so that collision can be re-enabled once IPPT is disabled
             This flag gets reset as part of setup_ipcollision*/
          if (!QCMAP_LAN_Client::UciSetUtility(owrt_filename[0], "profile", 0,
                            "ip_collision_enable_in_progress", 1,profile_idx) )
          {
            LOG_MSG_ERROR("Setting ip_collision_enable_in_progress failed", 0, 0, 0);
            return false;
          }
        }
      }
    }
  }

  /* Invoke rmnet_update.sh script and pass the first parameter as "up".
     This command will inturn trigger NetIfD executing teardown_interface()
     followed by setup_interface() of rmnet.script */
  if (!UpdateRmnetFile(RMNET_BRING_UP_CMD, profile_handle, BACKHAUL_V4))
  {
    LOG_MSG_ERROR("Failed to update NetIfD!", 0, 0, 0);
    *qmi_err_num = QMI_ERR_INTERNAL_V01;
    return false;
  }

  /* Perform dnsmasq restart and link toggle based on uci option queries */
  QCMAP_LAN_Client::CheckIPPTMode(profile_handle);


  return true;
} /* End SetIPPassthroughConfig */


/*===========================================================================
FUNCTION GetIPPassthroughConfig()
===========================================================================*/
/** @ingroup qcmap_get_ip_passthrough_config

  gets ip passthrough config parameters

  @datatypes
  qcmap_lan_ip_passthrough_mode_enum
  qcmap_lan_ip_passthrough_config

  @param[in]      enable_state          IP Passthrough Enable state \n
  @param[in]      ip_passthrough_config     IP Passthrough config struct \n
  @param[in]      profile_handle            Current Profile Handle

  @return
  TRUE -- Success \n
  FALSE -- Failure \n
*/
/*=========================================================================*/
boolean QCMAP_LAN_Client::GetIPPassthroughConfig
(
  qcmap_lan_ip_passthrough_mode_enum *enable_state,
  qcmap_lan_ip_passthrough_config *ip_passthrough_config,
  qmi_error_type_v01 *qmi_err_num
)
{
  char cmd[QCMAP_MAX_COMMAND_LEN] = {0};
  char result[QCMAP_MAX_SCAN_SIZE] = {0};
  char dev_type[QCMAP_MAX_SCAN_SIZE] = {0};
  int no_of_profiles = -1, profile_idx = -1;
  int16_t current_bridge_context = -1;
  int16_t ippt_bridge_context = -1;
  int error_num;
  int16_t vlan_idx = QCMAP_LAN_INVALID;
  int16_t profile_handle = QCMAP_LAN_INVALID;

  LOG_MSG_INFO1("\nGetIPPassthrough in API",0,0,0);

  /* Get current_bridge_context and IPPT_bridge_context */
  if (!QCMAP_LAN_Client::GetBridgeVLANContext(&current_bridge_context))
  {
    LOG_MSG_ERROR("Failed to get the current bridge context", 0, 0, 0);
    *qmi_err_num = QCMAP_LAN_INVALID;
    return false;
  }

  LOG_MSG_INFO1("\nCurrent_bridge_context = %d",current_bridge_context,0,0);

  if (current_bridge_context == 0)
  {
    profile_handle =  GetDefaultProfilefromUCI();
  }
  else
  {
    /* Get vlan index from qcmap_lan */
    vlan_idx = GetVlanIndex(current_bridge_context);

    /* Get mappped PDN based on vlan index from qcmap_lan */
   if ((profile_handle = GetMappedPDNforVlan(vlan_idx, qmi_err_num))
                                              == QCMAP_LAN_INVALID )
    {
      LOG_MSG_ERROR("Failed while trying to retreive mapped PDN",0,0,0);
      if (*qmi_err_num == QMI_ERR_NONE_V01)
      {
        *enable_state = 0;
        return true;
      }
      return false;
    }
  }

  LOG_MSG_INFO1("\nbridge_context = %d and mapped to PDN %d",
                  current_bridge_context, profile_handle,0);

  /* Calling Utility function to check if wan rules are present in uci database */
  if (!CheckIfNetworkRulesExist(NETWORK_WAN, profile_handle))
  {
    LOG_MSG_ERROR("Failed to validate network wan rules presence in uci database"
                  "for profile: %d", profile_handle, 0, 0);
    return false;
  }

  /* Get the profile index in qcmap_lan */
  profile_idx = GetProfileIndex(profile_handle);
  if (profile_idx == QCMAP_LAN_INVALID)
  {
    /* indicates profile_idx not found */
    LOG_MSG_ERROR(" Profile index for profile: %d not found in qcmap_lan. Exiting!",
                  profile_handle, 0, 0);
    *qmi_err_num = QMI_ERR_INVALID_INDEX_V01;
    return false;
  }

  /* Get ippt_enable state */
  memset(cmd, 0, QCMAP_MAX_COMMAND_LEN);
  memset(result, 0, QCMAP_MAX_SCAN_SIZE);
  snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "%s qcmap_lan.@profile[%d].ippt_enable",
           UCI_GET_COMMAND, profile_idx);
  if (!ExecuteSystemCmd((const char *)cmd, result,
                       sizeof(result), &error_num))
  {
    /* Indicates ippt_enable is not present */
    LOG_MSG_ERROR("IPPT enable state for PDN: %d not found!", profile_handle, 0, 0);
    *qmi_err_num = QMI_ERR_INTERNAL_V01;
    /* Indicates invalid argument : uci not found */
    if (error_num == EINVAL)
    {
      *qmi_err_num = QMI_ERR_NONE_V01;
      LOG_MSG_ERROR("IPPT is not configured errno=%d config not found", error_num, 0, 0);
      *enable_state = 0;
      return true;
    }
    return false;
  }
  *enable_state = atoi(result);

   if (!QCMAP_LAN_Client::GetIPPTBridgeContext(profile_idx, &ippt_bridge_context))
  {
    LOG_MSG_ERROR("Failed to get the ippt bridge context",0, 0, 0);
    *qmi_err_num = QCMAP_LAN_INVALID;
    return false;
  }
  /*If current_bridge context doesn't matches ippt_bridge context
   we should not display ippt_config  Update qmi_err_num to keep track of mismatch */
  if (current_bridge_context != ippt_bridge_context)
  {
    LOG_MSG_ERROR("Current_bridge_context(= %d) doesn't match ippt_bridge_context(= %d)",
                   current_bridge_context, ippt_bridge_context, 0);
    *qmi_err_num = QMI_ERR_INVALID_ARG_V01;
    *enable_state = 0;
    return true;
  }

  /* Get Device type */
  memset(cmd, 0, QCMAP_MAX_COMMAND_LEN);
  memset(result, 0, QCMAP_MAX_SCAN_SIZE);
  snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "%s qcmap_lan.@profile[%d].ippt_device_type",
           UCI_GET_COMMAND, profile_idx);
  if (!ExecuteSystemCmd((const char *)cmd, result,
                       sizeof(result)))
  {
    /* Indicates ippt_device_type not present */
    LOG_MSG_ERROR("Device type for PDN: %d not found!", profile_handle, 0, 0);
    *qmi_err_num = QMI_ERR_OP_DEVICE_UNSUPPORTED_V01;
    return false;
  }
  strlcpy(dev_type, result, strlen(result));
  LOG_MSG_INFO1("Device_type: %s!",dev_type, 0, 0);

  /* Condition to check dev_type */
  if (strncmp(dev_type, "USB", QCMAP_MAX_STRING_LEN) == 0)
  {
    /* Get host name */
    memset(cmd, 0, QCMAP_MAX_COMMAND_LEN);
    memset(result, 0, QCMAP_MAX_SCAN_SIZE);
    snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "%s qcmap_lan.@profile[%d].ippt_host_name",
             UCI_GET_COMMAND, profile_idx);
    if (!ExecuteSystemCmd((const char *)cmd, result,
                         sizeof(result)))
    {
      /* Indicates ippt_host_name not present */
      LOG_MSG_ERROR("Host name for PDN: %d not found!", profile_handle, 0, 0);
      *qmi_err_num = QMI_ERR_INTERFACE_NOT_FOUND_V01;
      return false;
    }
    ip_passthrough_config->device_type = QCMAP_LAN_DEVICE_TYPE_USB;
    strlcpy(ip_passthrough_config->client_device_name, result, QCMAP_MAX_DEVICE_NAME);
  }
  else if ((strncmp(dev_type, "ETH", QCMAP_MAX_STRING_LEN) == 0) ||
           (strncmp(dev_type, "ETH_NIC2", QCMAP_MAX_STRING_LEN) == 0) ||
           (strncmp(dev_type, "WiFi", QCMAP_MAX_STRING_LEN) == 0))
  {
    /* Get MAC Addr */
    memset(cmd, 0, QCMAP_MAX_COMMAND_LEN);
    memset(result, 0, QCMAP_MAX_SCAN_SIZE);
    snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "%s qcmap_lan.@profile[%d].ippt_mac_addr",
             UCI_GET_COMMAND, profile_idx);
    if (!ExecuteSystemCmd((const char *)cmd, result,
                         sizeof(result)))
    {
      /* Indicates ippt_mac_addr not present */
      LOG_MSG_ERROR("MAC Addr for PDN: %d not found!", profile_handle, 0, 0);
      *qmi_err_num = QMI_ERR_INTERNAL_V01;
      return false;
    }
    LOG_MSG_INFO1("MAC Addr: %s",result, 0, 0);
    ip_passthrough_config->device_type = QCMAP_LAN_DEVICE_TYPE_ETHERNET;
    if (strncmp(dev_type, "ETH_NIC2", QCMAP_MAX_STRING_LEN) == 0)
      ip_passthrough_config->device_type = QCMAP_LAN_DEVICE_TYPE_ETHERNET_NIC2;
    else if (strncmp(dev_type, "WiFi", QCMAP_MAX_STRING_LEN) == 0)
      ip_passthrough_config->device_type = QCMAP_LAN_DEVICE_TYPE_ANY_AP;
    for (int i = 0; i < QCMAP_MAC_ADDR_LEN; i++)
    {
      ip_passthrough_config->mac_addr[i] =
        (ds_hex_to_dec(result[i * 3]) << 4) | ds_hex_to_dec(result[i * 3 + 1]);
    }
  }
  else if (strncmp(dev_type,"Any", QCMAP_MAX_STRING_LEN) == 0)
  {
    ip_passthrough_config->device_type = QCMAP_LAN_DEVICE_TYPE_ANY;
  }
  else
  {
    ip_passthrough_config->device_type = QCMAP_LAN_DEVICE_TYPE_NONE;
  }

  return true;
} /* End GetIPPassthroughConfig */

/*===========================================================================
FUNCTION GetIPPassthroughState()
===========================================================================*/
/** @ingroup qcmap_get_ip_passthrough_state

  gets ip passthrough state info

  @param[in]      active_state              IP Passthrough active state \n
  @param[in]      profile_handle            Current Profile Handle

  @return
  TRUE -- Success \n
  FALSE -- Failure \n
*/
/*=========================================================================*/
boolean QCMAP_LAN_Client::GetIPPassthroughState
(
  boolean *active_state,
  qmi_error_type_v01 *qmi_err_num
)
{
  char cmd[QCMAP_MAX_COMMAND_LEN] = {0};
  char result[QCMAP_MAX_SCAN_SIZE] = {0};
  int active_ippt = -1, profile_idx = -1;
  int16_t current_bridge_context = -1;
  int16_t ippt_bridge_context = -1;
  int error_num;
  int16_t vlan_idx = QCMAP_LAN_INVALID;
  int16_t profile_handle = QCMAP_LAN_INVALID;

  LOG_MSG_INFO1("\nGetIPPassthrough in API",0,0,0);

  /* Get current_bridge_context and IPPT_bridge_context */
  if (!QCMAP_LAN_Client::GetBridgeVLANContext(&current_bridge_context))
  {
    LOG_MSG_ERROR("Failed to get the current bridge context", 0, 0, 0);
    *qmi_err_num = QCMAP_LAN_INVALID;
    return false;
  }

  LOG_MSG_INFO1("\nCurrent_bridge_context = %d",current_bridge_context,0,0);

  if (current_bridge_context == 0)
  {
    profile_handle = GetDefaultProfilefromUCI();
  }
  else
  {
    /* Get vlan index from qcmap_lan */
    vlan_idx = GetVlanIndex(current_bridge_context);

    /* Get mappped PDN based on vlan index from qcmap_lan */
    if ((profile_handle = GetMappedPDNforVlan(vlan_idx, qmi_err_num))
                                               == QCMAP_LAN_INVALID )
    {
      LOG_MSG_ERROR("Failed while trying to retreive mapped PDN",0,0,0);
      if (*qmi_err_num == QMI_ERR_NONE_V01)
      {
        *active_state = false;
        return true;
      }
      return false;
    }
  }

  LOG_MSG_INFO1("\nbridge_context = %d and mapped to PDN %d",
                   current_bridge_context, profile_handle,0);

  /* Calling Utility function to check if wan rules are present in uci database */
  if (!CheckIfNetworkRulesExist(NETWORK_WAN, profile_handle))
  {
    LOG_MSG_ERROR("Failed to validate network wan rules presence in uci database"
                  "for profile: %d", profile_handle, 0, 0);
    *qmi_err_num = QMI_ERR_INTERNAL_V01;
    return false;
  }

  /* Get the profile index in qcmap_lan */
  profile_idx = GetProfileIndex(profile_handle);
  if (profile_idx == QCMAP_LAN_INVALID)
  {
    /* indicates profile_idx not found */
    LOG_MSG_ERROR(" Profile index for profile: %d not found in qcmap_lan. Exiting!",
                  profile_handle, 0, 0);
    *qmi_err_num = QMI_ERR_INVALID_PROFILE_V01;
    return false;
  }

  /* Get ippt_enable state */
  memset(cmd, 0, QCMAP_MAX_COMMAND_LEN);
  memset(result, 0, QCMAP_MAX_SCAN_SIZE);
  snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "%s qcmap_lan.@profile[%d].ippt_enable",
           UCI_GET_COMMAND, profile_idx);
  if (!ExecuteSystemCmd((const char *)cmd, result,
                       sizeof(result), &error_num))
  {
    /* Indicates ippt_enable is not present */
    LOG_MSG_ERROR("IPPT enable state for PDN: %d not found!", profile_handle, 0, 0);
    /* Indicates invalid argument : uci not found */
    if (error_num == EINVAL)
    {
      /* Indicates IPPT is not configured.
         update active_state to false */
      *active_state = false;
      return true;
    }
    *qmi_err_num = QMI_ERR_INTERNAL_V01;
    return false;
  }

  /* Get active_ippt uci configuration */
  active_ippt = GetActiveIPPT(profile_idx);
  if (active_ippt == QCMAP_LAN_INVALID)
  {
    LOG_MSG_ERROR("Error in retrieving active_ippt value for profile handle: %d",
                  profile_handle, 0, 0);
    *qmi_err_num = QCMAP_LAN_INVALID;
    return false;
  }
  /* Get current_bridge_context and IPPT_bridge_context */
  if (!QCMAP_LAN_Client::GetBridgeVLANContext(&current_bridge_context))
  {
    LOG_MSG_ERROR("Failed to get the current bridge context", 0, 0, 0);
    *qmi_err_num = QCMAP_LAN_INVALID;
    return false;
  }
  if (!QCMAP_LAN_Client::GetIPPTBridgeContext(profile_idx, &ippt_bridge_context))
  {
    LOG_MSG_ERROR("Failed to get the ippt bridge context",0, 0, 0);
    *qmi_err_num = QCMAP_LAN_INVALID;
    return false;
  }
  /*If current_bridge context doesn't matches ippt_bridge context
   we should display ippt_state as not set */
  if (current_bridge_context != ippt_bridge_context)
  {
    LOG_MSG_ERROR("Current_bridge_context(= %d) doesn't match ippt_bridge_context(= %d)",
                   current_bridge_context, ippt_bridge_context, 0);
    *active_state = false;
    return true;
  }
  if (active_ippt == 1)
  {
    LOG_MSG_INFO1(" IPPT is currently active for PDN: %d", profile_handle, 0, 0);
    *active_state = true;
    return true;
  }
  else if (active_ippt == 0)
  {
    LOG_MSG_INFO1(" IPPT is currently inactive for PDN: %d", profile_handle, 0, 0);
    *active_state = false;
    return true;
  }
  else
  {
    LOG_MSG_ERROR("Invalid active_ippt value found for PDN: %d", profile_handle, 0, 0);
    *qmi_err_num = QMI_ERR_INTERNAL_V01;
    return false;
  }
} /* End GetIPPassthroughState */


/*=====================================================================
  FUNCTION CheckIfIPPTConfigExists
======================================================================*/
/*!
@brief
  - Checks if IPPT config entry exists in qcmap_lan database

@return
  true - Success
  false - Failure

@note
  - Checks if IPPT config entry exists in qcmap_lan database

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
boolean QCMAP_LAN_Client::CheckIfIPPTConfigExists
(
  qcmap_lan_ip_passthrough_config *ip_passthrough_config,
  const uint32_t profile_handle
)
{
  qcmap_lan_ip_passthrough_mode_enum enable_state;
  qmi_error_type_v01 qmi_err_num;
  qmi_err_num=QMI_ERR_NONE_V01;
  char result[QCMAP_MAX_SCAN_SIZE] = {0};
  int profile_idx = -1;

  if (NULL == ip_passthrough_config)
  {
    LOG_MSG_ERROR(" Invalid ippt_config pointer passed", 0, 0, 0);
    return false;
  }

  if (QCMAP_LAN_Client::GetIPPassthroughConfig(&enable_state,
                                               ip_passthrough_config,
                                                       &qmi_err_num))
  {
    /* Indicates there is an existing configuration. Check for device type */
    if (!((ip_passthrough_config->device_type == QCMAP_LAN_DEVICE_TYPE_USB) ||
          (ip_passthrough_config->device_type == QCMAP_LAN_DEVICE_TYPE_ETHERNET) ||
          (ip_passthrough_config->device_type == QCMAP_LAN_DEVICE_TYPE_ETHERNET_NIC2) ||
          (ip_passthrough_config->device_type == QCMAP_LAN_DEVICE_TYPE_ANY_AP) ||
          (ip_passthrough_config->device_type == QCMAP_LAN_DEVICE_TYPE_ANY)))
    {
      LOG_MSG_ERROR("No IPPT related device type found", 0, 0, 0);
      return false;
    }
    LOG_MSG_INFO1("Retrieved device type: %d", ip_passthrough_config->device_type, 0, 0);
  }
  else
  {
    LOG_MSG_ERROR("Get IP Passthrough Config failed\n",0,0,0);
    return false;
  }
  return true;
} /* End CheckIfIPPTConfigExists */


/*===========================================================================
  FUNCTION CheckIPPTState
  ===========================================================================*/
/*!
  @brief
  Check IPPT is enabled or not for a client based on mac address or hostname as index

  @return
  true  - on Success
  false - on Failure

  @parameters
  qcmap_dhcp_reservation  *dhcp_reserv_record,
  const uint32_t profile_handle

  @note

  - Dependencies
  - None

  - Side Effects
  - None
 */
/*=========================================================================*/

boolean QCMAP_LAN_Client::CheckIPPTState
(
 qcmap_dhcp_reservation  *dhcp_reserv_record,
 const uint32_t profile_handle
)
{
  qcmap_lan_ip_passthrough_config ip_passthrough_config;
  qcmap_lan_ip_passthrough_mode_enum enable_state;
  qmi_error_type_v01 qmi_err_num;
  boolean enable = TRUE;
  char mac_addr_str[QCMAP_LAN_MAC_ADDR_NUM_CHARS] = {0}; /*char array to store mac addr of  IPPT enabled client */
  char mac_addr_dhcp[QCMAP_LAN_MAC_ADDR_NUM_CHARS] = {0}; /*char array to store mac addr of DHCP Reserv client*/
  memset(&ip_passthrough_config, 0, sizeof(qcmap_lan_ip_passthrough_config));

  if (GetIPPassthroughConfig(&enable_state, &ip_passthrough_config, &qmi_err_num))
  {
    if(enable_state)
    {
      /* device type */
      if (ip_passthrough_config.device_type == QCMAP_LAN_DEVICE_TYPE_USB)
      {
        /* device name */
        if (! (strncmp(ip_passthrough_config.client_device_name, dhcp_reserv_record->client_device_name, strlen(dhcp_reserv_record->client_device_name)) ) )
        {
          LOG_MSG_INFO1("For the IPPT Enabled Client DHCP reservation is Not Allowed",0,0,0);
          return false;
        }
      }
      else
      {
        /* mac address */
        ds_mac_addr_ntop(ip_passthrough_config.mac_addr,mac_addr_str);
        ds_mac_addr_ntop(dhcp_reserv_record->client_mac_addr,mac_addr_dhcp);
        if ( strncmp(mac_addr_str,mac_addr_dhcp,strlen(mac_addr_str) ) == 0)
        {
          LOG_MSG_INFO1("For the IPPT Enabled Client DHCP reservation is Not Allowed",0,0,0);
          return false;
        }
      }
    }
  }
  return true;
}

/*=====================================================================
  FUNCTION CheckIPPTMode
======================================================================*/
/*!
@brief
  - Check if backhaul was brought up on IPPT mode and invoke functions
  for performing dnsmasq restart and phy link toggle

@return
  void

@note
  - System call to check if Backhaul has been brought up in IPPT mode
  with information available in uci database

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
void QCMAP_LAN_Client::CheckIPPTMode
(
  const uint32_t profile_handle,
  int event_type = 0
)
{
  char cmd[QCMAP_MAX_COMMAND_LEN] = {0};
  char result[QCMAP_MAX_SCAN_SIZE] = {0};
  char phy_iface[QCMAP_MAX_SCAN_SIZE] = {0};
  char mac_addr_str[QCMAP_MAX_SCAN_SIZE] = {0};
  int profile_idx = -1, active_ippt = -1;
  qcmap_lan_ip_passthrough_mode_enum enable_state;
  boolean found = false;

  /* Get profile_idx from qcmap_lan */
  profile_idx = GetProfileIndex(profile_handle);
  if (profile_idx == QCMAP_LAN_INVALID)
  {
    /* indicates profile_idx not found */
    LOG_MSG_ERROR(" Profile index for profile: %d not found in qcmap_lan. Exiting!",
                  profile_handle, 0, 0);
    return;
  }

  /* Get ippt_enable state */
  snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "%s qcmap_lan.@profile[%d].ippt_enable",
           UCI_GET_COMMAND, profile_idx);
  if (!ExecuteSystemCmd((const char *)cmd, result,
                       sizeof(result)))
  {
    /* Indicates ippt_enable is not present */
    LOG_MSG_ERROR("IPPT enable state for PDN: %d not found!", profile_handle, 0, 0);
    return;
  }
  enable_state = atoi(result);
  LOG_MSG_INFO1("enable_state value: %d", enable_state, 0, 0);

  if (enable_state == QCMAP_LAN_IP_PASSTHROUGH_MODE_UP)
  {
    /* If enabled, indicates backhaul will be brought up in IPPT mode
       and we need to sleep for 2 sec - This is done to ensure rmnet.script
       will be completely executed and we can continue with post
       BH processing*/
    sleep(2);

    /* Check if active_ippt is enabled */
    active_ippt = GetActiveIPPT(profile_idx);
    if (active_ippt != QCMAP_LAN_INVALID)
    {
      if (active_ippt == 1)
      {
        /* If enabled, indicates IPPT call is set.
        Query ippt_phy_iface parameter for a total of 5 iterations */
        for (int i = 0; i < 5; i++)
        {
          memset(cmd, 0, QCMAP_MAX_COMMAND_LEN);
          memset(result, 0, QCMAP_MAX_SCAN_SIZE);
          snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "%s qcmap_lan.@profile[%d].ippt_phy_iface",
                   UCI_GET_COMMAND, profile_idx);
          if (!ExecuteSystemCmd((const char *)cmd, result,
                             sizeof(result)))
          {
            /* Indicates ippt_phy_iface is not present */
            LOG_MSG_ERROR("IPPT phy iface for PDN: %d not found!", profile_handle, 0, 0);
            sleep(0.3);
          }
          else
          {
            LOG_MSG_INFO1(" phy iface successfully queried from uci for PDN: %d",
                           profile_handle, 0, 0);
            found = true;
            break;
          }
        }

        /* Perform dnsmasq reload */
        LOG_MSG_INFO1("Perform dnsmasq reload ", 0, 0, 0);
        //PerformDnsmasqReload();

        if (!found)
        {
          LOG_MSG_ERROR("IPPT phy iface not ofund after 5 iterations!", 0, 0, 0);
          return;
        }

        /* NOTE: we do not have any further checks in place since the very fact that
           ippt_phy_iface is available indicates setup_interface() of rmnet.script
           file has executed completely */
        strlcpy(phy_iface, result, strlen(result));

        /* Perform link toggle */
        LOG_MSG_INFO1("Perform link toggle: %s",phy_iface, 0, 0);
        //QCMAP_LAN_Client::PerformLinkToggle_ForIPPT((const char *)phy_iface, profile_idx);
        return;
      }
      else
      {
        /* NOTE: control will enter this condition when user brings down an active IPPT call.
        Expectation: We need not perform dnsmasq restart and link toggle. Instead we need to
        delete the dynamic parameter: ippt_phy_iface */
        LOG_MSG_ERROR("active_ippt is found to be disabled while enable_ippt is in MODE_UP",
                       0, 0, 0);

        /* Perform dnsmasq reload */
        LOG_MSG_INFO1("Perform dnsmasq reload", 0, 0, 0);
        //PerformDnsmasqReload();

        /* Check if ippt_phy_iface is available */
        memset(cmd, 0, QCMAP_MAX_COMMAND_LEN);
        memset(result, 0, QCMAP_MAX_SCAN_SIZE);
        snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "%s qcmap_lan.@profile[%d].ippt_phy_iface",
                 UCI_GET_COMMAND, profile_idx);
        if (!ExecuteSystemCmd((const char *)cmd, result,
                             sizeof(result)))
        {
          /* Indicates ippt_phy_iface is not present */
          LOG_MSG_ERROR("IPPT phy iface for PDN: %d not found!", profile_handle, 0, 0);
          return;
        }

        strlcpy(phy_iface, result, strlen(result));
        LOG_MSG_INFO1("ippt_phy_iface value: %s", phy_iface, 0, 0);

        /* Perform Link toggle */
        LOG_MSG_INFO1("Perform link toggle enable_state down", 0, 0, 0);
        //QCMAP_LAN_Client::PerformLinkToggle_ForIPPT((const char *)phy_iface, profile_idx);

        /* Delete ippt_phy_iface entry from qcmap_lan db */
        DeleteIPPTPhyIface(profile_idx);
        ExecuteUCICommit();
        return;
      }
    }
  }
  /* When ippt_enable = 0 and call is brought up /down we shouldn't perform
     dnsmasq reload and link toggle operations. event_type parameter is used to achieve this */
  else if (enable_state == QCMAP_LAN_IP_PASSTHROUGH_MODE_DOWN && event_type != BH_EVENT)
  {
    /* Indicates one of 2 possibilities
       1. The enable_state option might have been set by the user
       in the previous iteration and currently the call is being
       brought up in regular mode
       Note : For above event we shouldn't do perform dnsmasq reload or link toggle
       so we are not allowing entry to this block by using event_type param
       2. User wishes to disable IPPT mode during an active backhaul */

    /* Perform dnsmasq reload */
    LOG_MSG_INFO1("Perform dnsmasq reload", 0, 0, 0);
    //PerformDnsmasqReload();

    /* Check if ippt_phy_iface is available */
    memset(cmd, 0, QCMAP_MAX_COMMAND_LEN);
    memset(result, 0, QCMAP_MAX_SCAN_SIZE);
    snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "%s qcmap_lan.@profile[%d].ippt_phy_iface",
             UCI_GET_COMMAND, profile_idx);
    if (!ExecuteSystemCmd((const char *)cmd, result,
                         sizeof(result)))
    {
      /* Indicates ippt_phy_iface is not present */
      LOG_MSG_ERROR("IPPT phy iface for PDN: %d not found!", profile_handle, 0, 0);
      return;
    }

    strlcpy(phy_iface, result, strlen(result));

    /* Perform Link toggle */
    LOG_MSG_INFO1("Perform link toggle enable_state down on ippt_phy_iface(%s) ", phy_iface, 0, 0);
    //QCMAP_LAN_Client::PerformLinkToggle_ForIPPT((const char *)phy_iface, profile_idx);

    /* Delete ippt_phy_iface entry from qcmap_lan db */
    DeleteIPPTPhyIface(profile_idx);
    ExecuteUCICommit();
  }
  return;
} /* End CheckIPPTMode */

/*=====================================================================
  FUNCTION DeleteIPPTPhyIface
======================================================================*/
/*!
@brief
  - Deletes IPPT Phy interface from qcmap_lan

@return
  none

@note
  - Executes delete command for ippt_phy_iface

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
void DeleteIPPTPhyIface
(
  const uint32_t profile_idx
)
{
  char cmd[QCMAP_MAX_COMMAND_LEN] = {0};

  snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "/etc/data/uci_ex.sh del qcmap_lan.@profile[%d].ippt_phy_iface",
            profile_idx);
  ds_system_call(cmd, strlen(cmd));

  return;
} /* End DeleteIPPTPhyIface */


/*=====================================================================
  FUNCTION PerformLinkToggle_ForIPPT
======================================================================*/
/*!
@brief
  - Performs link toggle based on phy parameter passed and IPPT mode

@return
  void

@note
  - Performs link toggle based on usb/eth/wifi parameter passed and IPPT mode

- Dependencies
  - None

- Side Effects
  - None
*/
/*=========================================================================*/
void QCMAP_LAN_Client::PerformLinkToggle_ForIPPT
(
  const char *phy_iface,
  int profile_idx=0
)
{
  char cmd[QCMAP_MAX_COMMAND_LEN] = {0};
  char result[QCMAP_MAX_SCAN_SIZE] = {0};
  char mac_addr_str[QCMAP_MAX_SCAN_SIZE] = {0};
  qmi_error_type_v01 qmi_err_num;
  uint64_t features = 0;

  qcmap_lan_client_feature_mode_config feature_mode;
  if(!GetFeatureMode(&features, &feature_mode, &qmi_err_num))
  {
      LOG_MSG_ERROR("GetFeatureMode Failue!", 0, 0, 0);
      return;
  }

  //IPPT without NAT case, toggle the interface.
  if(feature_mode.ip_passthrough_feature_valid &&
     feature_mode.ip_passthrough_feature_mode == IP_PASSTHROUGH_MODE_WITHOUT_NAT)
  {
      return PerformLinkToggle(phy_iface);
  }

  //IPPT with NAT case, non-WiFi device, togger the interface
  if (IsWiFiDevcies(phy_iface) == false)
  {
      return PerformLinkToggle(phy_iface);
  }

  //IPPT with NAT case, WiFi device, togger the client only
  snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "%s qcmap_lan.@profile[%d].ippt_mac_addr",
             UCI_GET_COMMAND, profile_idx);
  if (!ExecuteSystemCmd((const char *)cmd, result, sizeof(result)))
  {
      /* Indicates ippt_mac_addr is not present */
      LOG_MSG_ERROR("IPPT mac addr for PDN: %d not found!", profile_idx, 0, 0);
      return;
  }

  strlcpy(mac_addr_str, result, strlen(result));
  LOG_MSG_INFO1("ippt_mac_addr: %s", mac_addr_str, 0, 0);

  memset(result, 0, sizeof(result));

  if (NULL !=mac_addr_str)
  {
       DisAssociateClient(mac_addr_str);
  }
  else
  {
      LOG_MSG_ERROR("mac_addr_str is NULL. check ippt_mac_addr in qcmap_lan", 0, 0, 0);
  }
  return;

}

/*=====================================================================
  FUNCTION PerformLinkToggle
======================================================================*/
/*!
@brief
  - Performs link toggle based on phy parameter passed

@return
  void

@note
  - Performs link toggle based on usb/eth/wifi parameter passed

- Dependencies
  - None

- Side Effects
  - None
*/
/*=========================================================================*/
void QCMAP_LAN_Client::PerformLinkToggle
(
  const char *phy_iface
)
{
  char cmd[QCMAP_MAX_COMMAND_LEN] = {0};
  char result[QCMAP_MAX_SCAN_SIZE] = {0};
  qcmap_lan_device_type_enum device_type = QCMAP_LAN_DEVICE_TYPE_NONE;

  /* Condition to check phy_iface */
  if (strncmp(phy_iface, USB_PHY, QCMAP_MAX_STRING_LEN) == 0)
  {
    /* toggle usb interface */
    device_type = QCMAP_LAN_DEVICE_TYPE_USB;
  }
  else if (strncmp(phy_iface, ETH_PHY, QCMAP_MAX_STRING_LEN) == 0)
  {
    /* toggle eth0 interface */
    device_type = QCMAP_LAN_DEVICE_TYPE_ETHERNET;
  }
  else if (strncmp(phy_iface, ETH_NIC2_PHY, QCMAP_MAX_STRING_LEN) == 0)
  {
    /* toggle eth1 interface */
    device_type = QCMAP_LAN_DEVICE_TYPE_ETHERNET_NIC2;
  }
  else if (IsWiFiDevcies(phy_iface))
  {
    device_type = QCMAP_LAN_DEVICE_TYPE_ALL_AP;
  }
  else if (strncmp(phy_iface, ALL_LINKS, QCMAP_MAX_STRING_LEN) == 0)
  {
    device_type = QCMAP_LAN_DEVICE_TYPE_ANY;
  }

  QCMAP_LAN_Client::RestartTetheredClient(device_type);

  return;
} /* End PerformLinkToggle */

/*===========================================================================
FUNCTION GetIPPTBridgeContext()
===========================================================================*/
/** @ingroup qcmap_get_ippt_bridge_context

  gets bridge context linked to a pdn for which IPPT is enabled

  @param[in]      profile_idx            Profile index of a PDN in qcmap_lan
  @param[in]      bridge_context         IPPT bridge context

  @return
  TRUE -- Success \n
  FALSE -- Failure
*/
/*=========================================================================*/
boolean QCMAP_LAN_Client::GetIPPTBridgeContext
(
  const int profile_idx,
  int16_t *bridge_context
)
{
  char result[QCMAP_MAX_SCAN_SIZE] = {0};

  if (NULL == bridge_context)
  {
    LOG_MSG_ERROR("Null parameter of bridge_context passed", 0, 0, 0);
    return false;
  }

  /* Get ippt_bridge_context */
  if (QCMAP_LAN_Client::UciGetUtility(owrt_filename[0], "profile", 0, "ippt_bridge_context", result, profile_idx))
  {
    *bridge_context = atoi(result);
    return true;
  }
  return false;
} /* End GetIPPTBridgeContext() */

/*===========================================================================
FUNCTION GetEnableIPPTValuePerPDN()
===========================================================================*/
/** @ingroup qcmap_get_enable_ippt_value_per_pdn

  gets the enable_ippt value saved per pdn from uci database

  @param[in]      profile_idx                 profile index

  @return
  TRUE -- Success \n
  FALSE -- Failure
*/
/*=========================================================================*/
boolean QCMAP_LAN_Client::GetEnableIPPTValuePerPDN
(
  const int profile_idx
)
{
  char result[QCMAP_MAX_SCAN_SIZE] = {0};

  if (QCMAP_LAN_Client::UciGetUtility(owrt_filename[0], "profile", 0, "ippt_enable", result, profile_idx))
  {
    /* Indicates IPPT is set to enable */
    return true;
  }
  return false;
} /* End GetEnableIPPTValuePerPDN() */

/*===========================================================================
FUNCTION GetIPPTStatus()
===========================================================================*/
/** @ingroup qcmap_get_ippt_status

  Function updates enable_status if IPPT WITH NAT FCD/ IPPT WITOUT_NAT is
  enabled on the bridge passed

  @param[in]      enable_status                 profile index
  @param[in]      bridge_id                     current_bridge_context
  @param[in]      qmi_err_num                   error no

  @return
  TRUE --  Success \n
  FALSE -- Failure
*/
/*=========================================================================*/
boolean QCMAP_LAN_Client::GetIPPTStatus
(
  bool *enable_status,
  int16_t bridge_id,
  qmi_error_type_v01 *qmi_err_num
)
{
  uint64 features = 0;
  int profile_index = QCMAP_LAN_INVALID;
  int profile_num = QCMAP_LAN_INVALID;
  int active_ippt = QCMAP_LAN_INVALID;
  char dev_type[QCMAP_MAX_SCAN_SIZE] = {0};
  char command[QCMAP_MAX_COMMAND_LEN] = {0};
  char result[QCMAP_MAX_SCAN_SIZE] = {0};
  qcmap_lan_client_feature_mode_config feature_mode_config;

  QCMAP_LOG_FUNC_ENTRY();

  memset(&feature_mode_config, 0, sizeof(qcmap_lan_client_feature_mode_config));

  *enable_status = false;

  if (QCMAP_LAN_Client::CheckIfVLANMappedToIPPTPDN(bridge_id, &profile_index,
                                                   &profile_num, qmi_err_num))
  {
    LOG_MSG_INFO1("IPPT is configured on bridge(=%d) and pdn (=%d)", bridge_id, profile_num, 0);
    active_ippt = GetActiveIPPT(profile_index);
    if (active_ippt == 1)
    {
      LOG_MSG_INFO1("IPPT is active on bridge(=%d) and pdn (=%d)", bridge_id, profile_num, 0);
      if (QCMAP_LAN_Client::GetFeatureMode(&features, &feature_mode_config, qmi_err_num))
      {
        if (features & QCMAP_MSGR_MASK_ENABLE_FEATURE_IP_PASSTHROUGH_V01)
        {
          /* Get Device type */
          memset(command, 0, QCMAP_MAX_COMMAND_LEN);
          memset(result, 0, QCMAP_MAX_SCAN_SIZE);
          snprintf(command, QCMAP_MAX_COMMAND_LEN, "%s qcmap_lan.@profile[%d].ippt_device_type",
                   UCI_GET_COMMAND, profile_index);
          if (ExecuteSystemCmd((const char *)command, result, sizeof(result)))
          {
            strlcpy(dev_type, result, strlen(result));
            LOG_MSG_INFO1("Device_type: %s!",dev_type, 0, 0);
            if ((feature_mode_config.ip_passthrough_feature_mode == IP_PASSTHROUGH_MODE_WITH_NAT  &&
                 (strncmp(dev_type,"Any",QCMAP_MAX_STRING_LEN) == 0)) ||
                 (feature_mode_config.ip_passthrough_feature_mode ==
                                   IP_PASSTHROUGH_MODE_WITHOUT_NAT))
            {
              LOG_MSG_INFO1("SetLANConfig rejected,IPPT is active Feature_mode(=%d),dev_type(=%s)",
                             feature_mode_config.ip_passthrough_feature_mode, dev_type, 0);
              *qmi_err_num = QMI_ERR_INCOMPATIBLE_STATE_V01;
              *enable_status=true;
              return true;
            }
          }
        }
      }
      else
      {
        LOG_MSG_ERROR("Failed to get feature mode", 0, 0, 0);
        *qmi_err_num = QMI_ERR_INTERNAL_V01;
         return false;
      }
    }
  }
  return true;
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
  default_handle
  profile_handle
  qmi_error_type_v01

  @param[in] filter_config               Filter config
  default_handle                         default profile
  profile_handle                         current profile
  @param[out] qmi_err_num                Error code returned by the server

  @return
  TRUE upon success. \n
  FALSE upon failure.

  @Dependencies
  QCMobileAP must be enabled.
*/
/*=========================================================================*/
boolean QCMAP_LAN_Client::SetIPPassthroughSoftwarePathFilters
(
 qcmap_msgr_sw_path_filters_conf_t       *filter_config,
 const uint32_t default_handle,
 const uint32_t profile_handle,
 qmi_error_type_v01 *qmi_err_num
)
{

 char cmd[QCMAP_MAX_COMMAND_LEN] = {0};
 char result[QCMAP_MAX_SCAN_SIZE] = {0};
 boolean active_state;
 int profile_idx = -1;
 qcmap_lan_ip_passthrough_mode_enum enable_state;
 qcmap_lan_ip_passthrough_config passthrough_config;
 memset(&enable_state, 0, sizeof(qcmap_msgr_ip_passthrough_mode_enum_v01));
 memset(&passthrough_config, 0, sizeof(qcmap_msgr_ip_passthrough_config_v01));

 if ((NULL == filter_config) || (NULL == qmi_err_num))
 {
   LOG_MSG_ERROR("Filter config or qmi_err_num is NULL", 0, 0, 0);
   return false;
 }

 if (!GetIPPassthroughConfig(&enable_state, &passthrough_config, qmi_err_num))
 {
   LOG_MSG_ERROR("Failed to get Passthrough config. Error: 0x%x", *qmi_err_num, 0, 0);
   *qmi_err_num = QMI_ERR_INVALID_ARG_V01;
   return false;
 }

 if (passthrough_config.device_type == -1)
 {
   LOG_MSG_ERROR("IP Passthrough config is not set for the bridge", 0, 0, 0);
   return false;
 }

 profile_idx = GetProfileIndex(profile_handle);
 if (profile_idx == QCMAP_LAN_INVALID)
 {
    /* indicates profile_idx not found */
    LOG_MSG_ERROR(" Profile index for profile: %d not found in qcmap_lan. Exiting!",
                  profile_handle, 0, 0);
    *qmi_err_num = QMI_ERR_INVALID_INDEX_V01;
    return false;
 }

 if (QCMAP_LAN_Client::GetIPPassthroughState(&active_state, qmi_err_num))
 {
   if(active_state)
   {
     //delete rules from apps
     LOG_MSG_INFO1("IPPT is Active deleting the sw path filters ", 0, 0, 0);
     memset(cmd, 0, QCMAP_MAX_COMMAND_LEN);
     snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "%s delete_ippt_sw_path_filters %d", IPPT_FILE,
              profile_handle);
     ds_system_call(cmd, strlen(cmd));
   }

 }

 memset(cmd, 0, QCMAP_MAX_COMMAND_LEN);
 snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "%s qcmap_lan.@profile[%d].total_swpath_filters",
          UCI_GET_COMMAND, profile_idx);
 if (ExecuteSystemCmd((const char *)cmd, result,
                sizeof(result)))
 {
   int total_no_of_filters = atoi(result);
   for(int index = 0; index < total_no_of_filters; index++)
   {
     memset(cmd, 0, QCMAP_MAX_COMMAND_LEN);
     snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "%s qcmap_lan.@profile[%d].start_port%d=%d",
                 UCI_DELETE_COMMAND, profile_idx, index+1, filter_config->filters[index].port_range.start_port);
     ds_system_call(cmd, strlen(cmd));
     memset(cmd, 0, QCMAP_MAX_COMMAND_LEN);
     snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "%s qcmap_lan.@profile[%d].end_port%d=%d",
                 UCI_DELETE_COMMAND, profile_idx, index+1, filter_config->filters[index].port_range.end_port);
     ds_system_call(cmd, strlen(cmd));
     memset(cmd, 0, QCMAP_MAX_COMMAND_LEN);
     snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "%s qcmap_lan.@profile[%d].protocol%d=%d",
                 UCI_DELETE_COMMAND, profile_idx, index+1, filter_config->filters[index].protocol);
     ds_system_call(cmd, strlen(cmd));
     snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "%s qcmap_lan.@profile[%d].total_swpath_filters=%d",
                 UCI_SET_COMMAND, profile_idx, (total_no_of_filters - (index + 1)));
     ds_system_call(cmd, strlen(cmd));
     /* Commit uci */
     ExecuteUCICommit();
   }
 }

 for (int i = 0; i < filter_config->num_of_filters; i++)
 {
   memset(cmd, 0, QCMAP_MAX_COMMAND_LEN);
   snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "%s qcmap_lan.@profile[%d].start_port%d=%d",
               UCI_SET_COMMAND, profile_idx, i+1, filter_config->filters[i].port_range.start_port);
   ds_system_call(cmd, strlen(cmd));
   memset(cmd, 0, QCMAP_MAX_COMMAND_LEN);
   snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "%s qcmap_lan.@profile[%d].end_port%d=%d",
               UCI_SET_COMMAND, profile_idx, i+1, filter_config->filters[i].port_range.end_port);
   ds_system_call(cmd, strlen(cmd));
   memset(cmd, 0, QCMAP_MAX_COMMAND_LEN);
   snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "%s qcmap_lan.@profile[%d].protocol%d=%d",
               UCI_SET_COMMAND, profile_idx, i+1, filter_config->filters[i].protocol);
   ds_system_call(cmd, strlen(cmd));
   snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "%s qcmap_lan.@profile[%d].total_swpath_filters=%d",
               UCI_SET_COMMAND, profile_idx, filter_config->num_of_filters);
   ds_system_call(cmd, strlen(cmd));


   /* Commit uci */
   ExecuteUCICommit();
 }

 if (QCMAP_LAN_Client::GetIPPassthroughState(&active_state, qmi_err_num))
 {
   if(active_state)
   {
     //Add rules on apps
     LOG_MSG_INFO1("IPPT is Active Adding the sw path filters ", 0, 0, 0);
     memset(cmd, 0, QCMAP_MAX_COMMAND_LEN);
     snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "%s add_ippt_sw_path_filters %d", IPPT_FILE,
              profile_handle);
     ds_system_call(cmd, strlen(cmd));
   }

 }
 *qmi_err_num = QMI_ERR_NONE_V01;
 return true;

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
  default_handle                         default profile
  profile_handle                         current profile
  @param[out] qmi_err_num                Error code returned by the server

  @return
  TRUE upon success. \n
  FALSE upon failure.

  @Dependencies
  QCMobileAP must be enabled.
*/
/*=========================================================================*/
boolean QCMAP_LAN_Client::GetIPPassthroughSoftwarePathFilters
(
  qcmap_msgr_sw_path_filters_conf_t        *filter_config,
  const uint32_t default_handle,
  const uint32_t profile_handle,
  qmi_error_type_v01                       *qmi_err_num
)
{
   char cmd[QCMAP_MAX_COMMAND_LEN] = {0};
   char result[QCMAP_MAX_SCAN_SIZE] = {0};
   int profile_idx = -1;

   if ((NULL == filter_config) || (NULL == qmi_err_num))
   {
     LOG_MSG_ERROR("Invalid/NULL arguments", 0, 0, 0);
     return false;
   }

   profile_idx = GetProfileIndex(profile_handle);
   if (profile_idx == QCMAP_LAN_INVALID)
   {
      /* indicates profile_idx not found */
      LOG_MSG_ERROR(" Profile index for profile: %d not found in qcmap_lan. Exiting!",
                    profile_handle, 0, 0);
      *qmi_err_num = QMI_ERR_INVALID_INDEX_V01;
      return false;
   }

   ZERO_INIT_ARG(*filter_config);

    memset(cmd, 0, QCMAP_MAX_COMMAND_LEN);
    memset(result, 0, QCMAP_MAX_SCAN_SIZE);
    snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "%s qcmap_lan.@profile[%d].total_swpath_filters",
               UCI_GET_COMMAND, profile_idx);
    if (ExecuteSystemCmd((const char *)cmd, result,
                    sizeof(result)))
    {
      if(filter_config->num_of_filters = atoi(result))
      {
        for(int i = 0; i < filter_config->num_of_filters; i++)
        {
          memset(cmd, 0, QCMAP_MAX_COMMAND_LEN);
          memset(result, 0, QCMAP_MAX_SCAN_SIZE);
          snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "%s qcmap_lan.@profile[%d].start_port%d",
                     UCI_GET_COMMAND, profile_idx, i+1);
          if (ExecuteSystemCmd((const char *)cmd, result,
                      sizeof(result)))
          {
            filter_config->filters[i].port_range.start_port = atoi(result);
          }
          memset(cmd, 0, QCMAP_MAX_COMMAND_LEN);
          memset(result, 0, QCMAP_MAX_SCAN_SIZE);
          snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "%s qcmap_lan.@profile[%d].end_port%d",
                     UCI_GET_COMMAND, profile_idx, i+1);
          if (ExecuteSystemCmd((const char *)cmd, result,
                      sizeof(result)))
          {
            filter_config->filters[i].port_range.end_port = atoi(result);
          }
          memset(cmd, 0, QCMAP_MAX_COMMAND_LEN);
          memset(result, 0, QCMAP_MAX_SCAN_SIZE);
          snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "%s qcmap_lan.@profile[%d].protocol%d",
                     UCI_GET_COMMAND, profile_idx, i+1);
          if (ExecuteSystemCmd((const char *)cmd, result,
                      sizeof(result)))
          {
            filter_config->filters[i].protocol = atoi(result);
          }
        }
      }
      else
      {
        filter_config->num_of_filters = 0;
        return true;
      }
    }
    *qmi_err_num = QMI_ERR_NONE_V01;
    return true;
}
