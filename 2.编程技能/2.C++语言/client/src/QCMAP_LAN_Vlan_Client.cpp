/*====================================================

FILE:  QCMAP_LAN_Vlan_Client.cpp

SERVICES:
QCMAP LAN Vlan Client Implementation

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
FUNCTION CreateVLANConfig()
===========================================================================*/
/** @ingroup qcmap_set_vlan_config

  Set VLAN upon getting request from QCMAP LAN client.

  @datatypes
  qcmap_lan_vlan_conf_t

  @param[in]      qcmap_lan_vlan_conf_t vlan_config

  @return
  TRUE -- Success \n
  FALSE -- Failure \n
*/
/*=========================================================================*/
boolean QCMAP_LAN_Client::CreateVLANConfig
(
  qcmap_lan_vlan_conf_t vlan_config,
  qmi_error_type_v01 *qmi_err_num,
  bool *is_accelerated
)
{
  char cmd[QCMAP_MAX_COMMAND_LEN] = {0};
  char result[QCMAP_MAX_SCAN_SIZE] = {0};
  uint32_t eth_type;
  int32_t nic_index=INT_MIN;
  uint8_t ezmesh_enable = 0;
  int profile_handle;

/*
      | 1. ETH       |
      | 2. ECM       |
      | 3. RNDIS     |
      | 4. ETH-NIC2  |
*/

  profile_handle = GetDefaultProfilefromUCI();
  IPv4_DISABLE_NAT_EXCLUSIVITY_CHECK(profile_handle, qmi_err_num, false);

  if ( QCMAP_INTERFACE_TYPE_ETH == vlan_config.intf_type )
  {
    nic_index = 0;
  }
  else if ( QCMAP_INTERFACE_TYPE_ETH_NIC2 == vlan_config.intf_type )
  {
    nic_index = 1;
  }

  if ( INT_MIN != nic_index)
  {
    // Get the eth nw type and validate
    if ( QCMAP_LAN_Client::UciGetUtility(owrt_filename[0], "eth_nic_config", 0, "type", result, nic_index) )
    {
      eth_type = atoi(result);
    }
    else
    {
      LOG_MSG_ERROR("UciGetUtility failed to fetch the info ",0,0,0);
      *qmi_err_num = QMI_ERR_INTERNAL_V01;
      return false;
    }

    if ( QCMAP_ETH_MODE_WAN == eth_type )
    {
      LOG_MSG_ERROR("QCMAP_LAN_CLIENT: Invalid eth type:(%d)", eth_type, 0, 0);
      *qmi_err_num = QMI_ERR_INVALID_ID_V01;
      return false;
    }
  }

  /*Validate phy_iface_type and vlan id */
  if((vlan_config.vlan_id <= 0) || (vlan_config.vlan_id > MAX_VLAN_ID))
  {
    LOG_MSG_ERROR("QCMAP_LAN_CLIENT: Invalid vlan id:(%d)", vlan_config.vlan_id, 0, 0);
    *qmi_err_num = QMI_ERR_INVALID_ID_V01;
    return false;
  }

  if((vlan_config.intf_type < MIN_IFACE_TYPE ) || (vlan_config.intf_type > MAX_IFACE_TYPE))
  {
    LOG_MSG_ERROR("QCMAP_LAN_CLIENT: Invalid phy_iface_type:(%d)", vlan_config.intf_type, 0, 0);
    *qmi_err_num = QMI_ERR_INTERFACE_NOT_FOUND_V01;
    return false;
  }

  UCI_EZMESH_GET_INT_OPTION(ezmesh_enable, "ezmesh", "enable_state");
  if(ezmesh_enable == QCMAP_MSGR_EZMESH_ENABLE_V01)
  {
    qcmap_msgr_ezmesh_capability_enum_v01 capability = 0;
    boolean is_ezmesh_r2_cfg_valid = false;
    qcmap_msgr_ezmesh_r2_config_v01 ezmesh_r2_config;

    UCI_EZMESH_GET_INT_OPTION(capability, "ezmesh", "capability");
    UCI_EZMESH_GET_INT_OPTION(is_ezmesh_r2_cfg_valid, "r2_config", "traffic_separation");
    if (is_ezmesh_r2_cfg_valid &&
        IS_EZMESH_CAPABILITY_R2_R3_R4(capability))
    {
      QCMAP_LAN_Client::GetEZMeshR2Config(&ezmesh_r2_config);
      /* Clean up the vlan-bridge and firewall uci config */
      for (int i = 0; i < ezmesh_r2_config.mapping_len; i++)
      {
        if(ezmesh_r2_config.mapping[i].vlan_id == vlan_config.vlan_id)
        {
          LOG_MSG_ERROR("VLAN ID '%d' is already configured for EZMesh",vlan_config.vlan_id,0,0);
          printf("Vlan id '%d' is already configure for EZMesh\n",vlan_config.vlan_id);
          return false;
        }
      }
    }
  }

  char command[MAX_COMMAND_STR_LEN] = {0};
  snprintf(command, MAX_COMMAND_STR_LEN,"%s add_vlan %d %d %d", VLAN_CONFIG,
           vlan_config.intf_type, vlan_config.vlan_id, vlan_config.is_accelerated);
  ds_system_call(command, strlen(command));

  memset(cmd, 0, QCMAP_MAX_COMMAND_LEN);
  memset(result, 0, QCMAP_MAX_SCAN_SIZE);
  snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "%s %s %d",
            VLAN_CONFIG, GET_IPA_OFFLOAD_STATUS, vlan_config.vlan_id);
  if (ExecuteSystemCmd((const char *)cmd, result,
                  sizeof(result)))
  {
    if(atoi(result) == 1)
    {
      LOG_MSG_INFO1("VLAN ID: %d is offloaded", vlan_config.vlan_id,0,0);
      *is_accelerated = true;
    }
    else
    {
      LOG_MSG_INFO1("VLAN ID: %d is not offloaded", vlan_config.vlan_id,0,0);
      *is_accelerated = false;
    }
  }
  memset(cmd, 0, QCMAP_MAX_COMMAND_LEN);
  memset(result, 0, QCMAP_MAX_SCAN_SIZE);
  snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "%s %s",
            UCI_GET_COMMAND, QCMAP_IP_PT_FEATURE_MODE);
  if (ExecuteSystemCmd((const char *)cmd, result,
                  sizeof(result)))
  {
    int feature_mode = atoi(result);
    if ((qcmap_lan_ip_passthrough_feature_mode_enum)feature_mode == IP_PASSTHROUGH_MODE_WITHOUT_NAT)
    {
      /* Indicates IPPT feature mode is in WITHOUT_NAT configuration
         1. Need to set DHCP ignore option on the bridges

      /* Set DHCP ignore option on bridges */
      SetResetDHCPIgnoreOption(true);
    }
  }

  /* Update bridge-vlan context */
  QCMAP_LAN_Client::SetBridgeVLANContext(vlan_config.vlan_id);

  return true;
} /*End SetVlanConfig() */

/*===========================================================================
FUNCTION DeleteVlanConfig()
===========================================================================*/
/** @ingroup qcmap_set_vlan_config

  delete VLAN upon getting request from QCMAP LAN client.

  @datatypes
  qcmap_lan_vlan_conf_t

  @param[in]      qcmap_lan_vlan_conf_t vlan_config

  @return
  TRUE -- Success \n
  FALSE -- Failure \n
*/
/*=========================================================================*/
boolean QCMAP_LAN_Client::DeleteVlanConfig
(
  qcmap_lan_vlan_conf_t vlan_config,
  qmi_error_type_v01 *qmi_err_num
)
{
/*
      | 1. ETH       |
      | 2. ECM       |
      | 3. RNDIS     |
      | 4. ETH-NIC2  |
*/

  int profile_index = -1, profile_num = -1;
  char cmd[QCMAP_MAX_COMMAND_LEN] = {0};
  char result[QCMAP_MAX_SCAN_SIZE] = {0};
  uint8_t ezmesh_enable = 0;
  int16_t bridge_context;
  int profile_handle;

  profile_handle = GetDefaultProfilefromUCI();

  IPv4_DISABLE_NAT_EXCLUSIVITY_CHECK(profile_handle, qmi_err_num, false);

  /*Validate phy_iface_type and vlan id */
  if((vlan_config.vlan_id <= 0) || (vlan_config.vlan_id > MAX_VLAN_ID))
  {
    LOG_MSG_ERROR("QCMAP_LAN_CLIENT: Invalid vlan id:(%d)", vlan_config.vlan_id, 0, 0);
    *qmi_err_num = QMI_ERR_INVALID_ID_V01;
    return false;
  }

  if((vlan_config.intf_type < MIN_IFACE_TYPE) || (vlan_config.intf_type > MAX_IFACE_TYPE))
  {
    LOG_MSG_ERROR("QCMAP_LAN_CLIENT: Invalid phy_iface_type:(%d)", vlan_config.intf_type, 0, 0);
    *qmi_err_num = QMI_ERR_INTERFACE_NOT_FOUND_V01;
    return false;
  }

  UCI_EZMESH_GET_INT_OPTION(ezmesh_enable, "ezmesh", "enable_state");
  if(ezmesh_enable == QCMAP_MSGR_EZMESH_ENABLE_V01)
  {
    qcmap_msgr_ezmesh_capability_enum_v01 capability = 0;
    boolean is_ezmesh_r2_cfg_valid = false;
    qcmap_msgr_ezmesh_r2_config_v01 ezmesh_r2_config;

    UCI_EZMESH_GET_INT_OPTION(capability, "ezmesh", "capability");
    UCI_EZMESH_GET_INT_OPTION(is_ezmesh_r2_cfg_valid, "r2_config", "traffic_separation");
    if (is_ezmesh_r2_cfg_valid &&
        IS_EZMESH_CAPABILITY_R2_R3_R4(capability))
    {
      QCMAP_LAN_Client::GetEZMeshR2Config(&ezmesh_r2_config);
      /* Clean up the vlan-bridge and firewall uci config */
      for (int i = 0; i < ezmesh_r2_config.mapping_len; i++)
      {
        if(ezmesh_r2_config.mapping[i].vlan_id == vlan_config.vlan_id)
        {
          LOG_MSG_ERROR("VLAN ID '%d' is already configured for EZMesh",vlan_config.vlan_id,0,0);
          printf("Vlan id '%d' is already configure for EZMesh\n",vlan_config.vlan_id);
          return false;
        }
      }
    }
  }

  /* Call CheckIfVLANMappedToIPPTPDN() to check if vlan id is mapped to any pdn where IPPT is set to
  enabled mode */
  if (QCMAP_LAN_Client::CheckIfVLANMappedToIPPTPDN(vlan_config.vlan_id, &profile_index, &profile_num, qmi_err_num))
  {
    /* Indicates the current vlan_id is found to be mapped to a pdn and its ippt_bridge_context is
    set to the vlan id to be deleted */
    printf(" Please ensure to set new IPPT configurations linked to vlan_id: %d\n", vlan_config.vlan_id);
    printf(" Disabling IP Passthrough for PDN: %d\n", profile_num);
    fflush(stdout);
    /* Set ippt_enable state to MODE_DOWN */
    memset(cmd, 0, QCMAP_MAX_COMMAND_LEN);
    snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "%s set_ippt_state %d %d", IPPT_FILE,
             profile_index, QCMAP_LAN_IP_PASSTHROUGH_MODE_DOWN);
    ds_system_call(cmd, strlen(cmd));
  }

  char command[MAX_COMMAND_STR_LEN] = {0};
  snprintf(command, MAX_COMMAND_STR_LEN,"%s del_vlan %d %d", VLAN_CONFIG, vlan_config.intf_type, vlan_config.vlan_id);
  ds_system_call(command, strlen(command));

  /* Check if bridge_context is same as vlan_id */
  if (!QCMAP_LAN_Client::GetBridgeVLANContext(&bridge_context))
  {
    LOG_MSG_ERROR("Failed to get the current bridge context", 0, 0, 0);
    *qmi_err_num = QMI_ERR_INTERNAL_V01;
    return false;
  }

  if (bridge_context == vlan_config.vlan_id)
  {
    /* Update bridge-vlan context */
    QCMAP_LAN_Client::SetBridgeVLANContext(DEFAULT_BRIDGE_ID);
  }

  return true;
} /*End DeleteVlanConfig() */

/*===========================================================================
FUNCTION GetVlanConfig()
===========================================================================*/
/** @ingroup qcmap_get_vlan_config

  Show VLAN upon getting request from QCMAP.

   @param[in,out]      qcmap_vlan_conf_t *vlan_config

  @return
  TRUE -- Success \n
  FALSE -- Failure \n
*/
/*=========================================================================*/
boolean QCMAP_LAN_Client::GetVlanConfig
(
  qcmap_vlan_conf_t *vlan_config,
  qmi_error_type_v01 *qmi_err_num
)
{
  char result[QCMAP_MAX_SCAN_SIZE] = {0};
  uint16_t total_vlan_count=0;
  uint16_t vlan_id;
  /**< no_of_vlans is total vlan section configured in qcmap_lan via UCI */
  uint16_t no_of_vlans;
  char phy_iface_name[QCMAP_MAX_SCAN_SIZE]={0};
  std::string intf;
  qcmap_interface_type_enum  iface_type;
  char offloaded_ifaces[QCMAP_MAX_SCAN_SIZE]={0};
  char *iface=NULL, *ptr2;
  int is_ipa_offloaded=0;

  if(vlan_config == NULL)
  {
    LOG_MSG_ERROR("vlan_config_list or total_vlan is NULL",0,0,0);
    *qmi_err_num = QMI_ERR_INVALID_ARG_V01;
    return false;
  }

  if (!UciGetUtility(owrt_filename[static_cast<int>(config_file::QCMAP_LAN)],
       "no_of_configs", 0, "no_of_vlans", result, 0))
  {
    LOG_MSG_ERROR("UciGetUtility failed", 0, 0, 0);
    *qmi_err_num = QMI_ERR_INTERNAL_V01;
    return false;
  }
  no_of_vlans = atoi(result);
  for(int i=0; i<no_of_vlans; i++)
  {
    memset(result,0,QCMAP_MAX_SCAN_SIZE);
    if(!UciGetUtility(owrt_filename[static_cast<int>(config_file::QCMAP_LAN)],
       "vlan", 0, "vlan_id", result, i))
    {
      LOG_MSG_ERROR("UciGetUtility failed", 0, 0, 0);
      *qmi_err_num = QMI_ERR_INTERNAL_V01;
      return false;
    }
    vlan_id=atoi(result);

    memset(result,0,QCMAP_MAX_SCAN_SIZE);
    memset(phy_iface_name, 0, QCMAP_MAX_SCAN_SIZE);
    if(!UciGetUtility(owrt_filename[static_cast<int>(config_file::QCMAP_LAN)],
       "vlan", 0, "phy_interface", result, i))
    {
      LOG_MSG_ERROR("UciGetUtility failed", 0, 0, 0);
      *qmi_err_num = QMI_ERR_INTERNAL_V01;
      return false;
    }
    strlcpy(phy_iface_name, result, strlen(result));
    std::stringstream intf_list(phy_iface_name);
    while(std::getline(intf_list,intf,' '))
    {
      const char *tmp = intf.c_str();
      vlan_config->vlan_config_list_ex[total_vlan_count].vlan_id=vlan_id;
      if (GetIfaceEnumFromName(tmp, &iface_type))
      {
        vlan_config->vlan_config_list_ex[total_vlan_count].intf_type = iface_type;
        memset(result,0,QCMAP_MAX_SCAN_SIZE);
        if (UciGetUtility(owrt_filename[static_cast<int>(config_file::QCMAP_LAN)],
            "vlan", 0, "ipa_offload", result, i))
        {
          is_ipa_offloaded = atoi(result);
          if (is_ipa_offloaded == 1)
          {
            if (UciGetUtility(owrt_filename[static_cast<int>(config_file::QCMAP_LAN)],
                "vlan", 0, "offloaded_phy_interfaces", result, i))
            {
              strlcpy(offloaded_ifaces, result, strlen(result));
              iface = strtok_r(offloaded_ifaces," ", &ptr2);
              while (iface != NULL)
              {
                if (strncmp(iface, tmp, strlen(tmp)) == 0)
                {
                  vlan_config->vlan_config_list_ex[total_vlan_count].is_accelerated = 1;
                  break;
                }
                iface = strtok_r(NULL," ", &ptr2);
              }
            }
          }
        }
        else
        {
          LOG_MSG_ERROR("UciGetUtility failed", 0, 0, 0);
          return false;
        }
      }
      else
      {
        LOG_MSG_ERROR("failed to get the interface type for interface name:%s", tmp, 0, 0);
        vlan_config->vlan_config_list_ex[total_vlan_count].intf_type = QCMAP_INTERFACE_TYPE_ENUM_MIN_ENUM_VAL;
      }
      /**< total_vlan_count refers total number of vlan configured on device */
      total_vlan_count++;
    }
  }

  vlan_config->vlan_config_list_len = total_vlan_count;
  return true;
} /*End GetVlanConfig() */

/*===========================================================================
FUNCTION AddPDNToVLANMapping()
===========================================================================*/
/** @ingroup qcmap_add_pdn_to_vlan_mapping

  Maps user provided PDN and vlan-id info.

  @param[in]      vlan_id           QCMAP VLAN ID
  @param[in]      Profile_handle    QCMAP WWAN Profile handle

  @return
  TRUE -- Success \n
  FALSE -- Failure \n
*/
/*=========================================================================*/
boolean QCMAP_LAN_Client::AddPDNToVLANMapping
(
  int16_t vlan_id,
  uint32_t profile_handle,
  qmi_error_type_v01 *qmi_err_num
)
{
  /* Query uci for active_profile list */
  boolean is_pdn_active = false;
  char cmd[QCMAP_MAX_COMMAND_LEN] = {0};
  char ipv4config[QCMAP_MAX_STRING_LEN] = {0};
  char ipv6config[QCMAP_MAX_STRING_LEN] = {0};

  IPv4_DISABLE_NAT_EXCLUSIVITY_CHECK(profile_handle, qmi_err_num, false);

  snprintf(ipv4config, QCMAP_MAX_STRING_LEN, "/tmp/ipv4config%d",
            profile_handle);
  snprintf(ipv6config, QCMAP_MAX_STRING_LEN, "/tmp/ipv6config%d",
            profile_handle);

  /* Check if ipv4config/ipv6config file is present */
  if ((QCMAP_LAN_Client::CheckIfFileExists(ipv4config)) ||
       (QCMAP_LAN_Client::CheckIfFileExists(ipv6config)))
  {
    /* Set PDN active to true */
    is_pdn_active = true;
  }
  /* Send all information to vlan_pdn_map script */
  snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "%s %s %u %u", BACKHAUL_WWAN_CONFIG_FILE,
           QCMAP_ADD_MAP_VLAN_PDN, vlan_id, profile_handle);
  ds_system_call(cmd, strlen(cmd));

  /* Update bridge-vlan context */
  QCMAP_LAN_Client::SetBridgeVLANContext(vlan_id);

  /* Send all information to script */
  snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "%s %s %u", IP_COLLISION_FILE,
           QCMAP_CHECK_ADDRESS_CONFLICT, profile_handle);
  ds_system_call(cmd, strlen(cmd));

  if ( !QCMAP_LAN_Client::CheckAndActivateIPCollision(profile_handle) )
  {
    LOG_MSG_ERROR("CheckAndActivateIPCollision Failed for profile_handle %d",profile_handle,0,0);
  }

  return true;
} /* End AddPDNToVLANMapping */

/*===========================================================================
FUNCTION DeletePDNToVLANMapping()
===========================================================================*/
/** @ingroup qcmap_delete_pdn_to_vlan_mapping

  Maps user provided PDN and vlan-id info.

  @param[in]      vlan_id           QCMAP VLAN ID
  @param[in]      Profile_handle    QCMAP WWAN Profile handle

  @return
  TRUE -- Success \n
  FALSE -- Failure \n
*/
/*=========================================================================*/
boolean QCMAP_LAN_Client::DeletePDNToVLANMapping
(
  int16_t vlan_id,
  uint32_t profile_handle,
  qmi_error_type_v01 *qmi_err_num
)
{
  char cmd[QCMAP_MAX_COMMAND_LEN] = {0};
  int profile_index = -1, profile_num = -1;

  IPv4_DISABLE_NAT_EXCLUSIVITY_CHECK(profile_handle, qmi_err_num, false);

  /* Call CheckIfVLANMappedToIPPTPDN() to check if vlan id is mapped to any pdn where IPPT is set to
  enabled mode */
  if (QCMAP_LAN_Client::CheckIfVLANMappedToIPPTPDN(vlan_id, &profile_index, &profile_num, qmi_err_num))
  {
    /* Indicates the current vlan_id is found to be mapped to a pdn and its ippt_bridge_context is
    set to the vlan id to be deleted */
    printf(" Please ensure to set new IPPT configurations linked to vlan_id: %d\n", vlan_id);
    printf(" Disabling IP Passthrough for PDN: %d\n", profile_num);
    fflush(stdout);
    /* Set ippt_enable state to MODE_DOWN */
    memset(cmd, 0, QCMAP_MAX_COMMAND_LEN);
    snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "%s set_ippt_state %d %d", IPPT_FILE,
             profile_index, QCMAP_LAN_IP_PASSTHROUGH_MODE_DOWN);
    ds_system_call(cmd, strlen(cmd));
  }

  /* Send all information to vlan_pdn_map script */
  snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "%s %s %u %u", BACKHAUL_WWAN_CONFIG_FILE,
           QCMAP_DEL_MAP_VLAN_PDN, vlan_id, profile_handle);
  ds_system_call(cmd, strlen(cmd));

  return true;
} /* End DeletePDNToVLANMapping */

/*===========================================================================
FUNCTION GetPDNToVLANMapping()
===========================================================================*/
/** @ingroup qcmap_get_pdn_to_vlan_mapping

  Maps user provided PDN and vlan-id info.

  @param[in]      mapping           VLAN to PDN mapping

  @return
  TRUE -- Success \n
  FALSE -- Failure \n
*/
/*=========================================================================*/
boolean QCMAP_LAN_Client::GetPDNToVLANMapping
(
  qcmap_pdn_to_vlan_mapping *mappings,
  int *num_entries,
  qmi_error_type_v01 *qmi_err_num
)
{
  qcmap_pdn_to_vlan_mapping pdn_to_vlan_map[QCMAP_MAX_BACKHAULS];
  char result[QCMAP_MAX_SCAN_SIZE] = {0};
  char vid_list[QCMAP_MAX_SCAN_SIZE] = {0};
  char profile_num[QCMAP_MAX_SCAN_SIZE] = {0};
  char cmd[QCMAP_MAX_COMMAND_LEN] = {0};
  int i=0;
  char *ptr1=NULL, *ptr2;
  memset(pdn_to_vlan_map, 0, QCMAP_MAX_BACKHAULS * sizeof(qcmap_pdn_to_vlan_mapping));
  /* Check for passed parameter validity */
  if ((mappings == NULL) || (num_entries == NULL))
  {
    LOG_MSG_ERROR("NULL pointer passed", 0, 0, 0);
    *qmi_err_num = QMI_ERR_INVALID_ARG_V01;
    return false;
  }
  /* Query Number of VLAN's */
  snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "%s %s", UCI_GET_COMMAND, UCI_QUERY_NO_OF_PROFILES);
  if (ExecuteSystemCmd((const char *)cmd, result, sizeof(result)))
  {
    /* Iterate through the num of profiles to fetch associated vlan_ids */
    int no_of_profiles = atoi(result);
    for ( i = 0; i < no_of_profiles; i++ )
    {
      /* Get profile ID from profile config */
      memset(profile_num, 0, QCMAP_MAX_SCAN_SIZE);
      memset(cmd, 0, QCMAP_MAX_COMMAND_LEN);
      snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "%s qcmap_lan.@profile[%d].profile_id",
                UCI_GET_COMMAND, i);
      if (!ExecuteSystemCmd((const char *)cmd, profile_num, sizeof(profile_num)))
      {
        LOG_MSG_ERROR("Failed to execute command : %s",cmd,0,0);
        *qmi_err_num = QMI_ERR_INTERNAL_V01;
        return false;
      }
      /* Iterate through vid_list to store profile_num and vlan_id in pdn_to_vlan_map */
      memset(vid_list, 0, QCMAP_MAX_SCAN_SIZE);
      memset(cmd, 0, QCMAP_MAX_COMMAND_LEN);
      snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "%s qcmap_lan.@profile[%d].vlan_ids",
                UCI_GET_COMMAND, i);
      if (ExecuteSystemCmd((const char *)cmd, vid_list, sizeof(vid_list)))
      {
        pdn_to_vlan_map[i].vlan_id_len = 0;
        ptr1 = strtok_r(vid_list," ", &ptr2);
        while (ptr1 != NULL)
        {
          int vlan_id_length=pdn_to_vlan_map[i].vlan_id_len;
          pdn_to_vlan_map[i].profile_handle = atoi(profile_num);
          pdn_to_vlan_map[i].vlan_id[vlan_id_length] = atoi(ptr1);
          pdn_to_vlan_map[i].vlan_id_len++;
          ptr1 = strtok_r(NULL," ", &ptr2);
        }
      }
      else
      {
        LOG_MSG_ERROR("PDN is not linked to vlan_ID: %s", profile_num, 0, 0);
      }
    }
  }
  else
  {
    LOG_MSG_ERROR(" Unable to query number of vlans", 0, 0, 0);
    return false;
  }
  /* Copy the address of struct to pointer */
  *num_entries = i;
  memcpy(mappings, &pdn_to_vlan_map, *num_entries * sizeof(qcmap_pdn_to_vlan_mapping));
  return true;

} /* End GetPDNToVLANMapping */


/*===========================================================================
FUNCTION SetBridgeVLANContext()
===========================================================================*/
/** @ingroup qcmap_set_bridge_vlan_context

  sets bridge vlan context linked ot a pdn

  @param[in]      bridge_id                 bridge vlan ID

  @return
  void
*/
/*=========================================================================*/
void QCMAP_LAN_Client::SetBridgeVLANContext
(
  const int16_t bridge_id
)
{
  char cmd[QCMAP_MAX_COMMAND_LEN] = {0};

  this->bridge_id = bridge_id;
  return;
} /* End SetBridgeVLANContext */

/*===========================================================================
FUNCTION GetBridgeVLANContext()
===========================================================================*/
/** @ingroup qcmap_get_bridge_vlan_context

  gets bridge vlan context linked ot a pdn

  @param[in]      bridge_id                 bridge vlan ID

  @return
  TRUE -- Success \n
  FALSE -- Failure
*/
/*=========================================================================*/
boolean QCMAP_LAN_Client::GetBridgeVLANContext
(
  int16_t *bridge_id
)
{
  if (NULL == bridge_id)
  {
    LOG_MSG_ERROR("Null parameter passed", 0, 0, 0);
    return false;
  }

  *bridge_id = this->bridge_id;
   return true;
} /* End GetBridgeVLANContext */

/*===========================================================================
FUNCTION GetMappedVLANPerPDN()
===========================================================================*/
/** @ingroup qcmap_get_mapped_vlan_per_pdn

  gets mapped vlan id's per pdn from qcmap_lan uci database

  @param[in]      profile_idx            Profile index of a PDN in qcmap_lan
  @param[in]      bridge_vlan_ids         IPPT bridge vlan ID's

  @return
  TRUE -- Success \n
  FALSE -- Failure
*/
/*=========================================================================*/
boolean QCMAP_LAN_Client::GetMappedVLANPerPDN
(
  const int profile_idx,
  char *bridge_vlan_ids,
  qmi_error_type_v01 *qmi_err_num
)
{
  char result[QCMAP_MAX_SCAN_SIZE] = {0};

  if (NULL == bridge_vlan_ids)
  {
    LOG_MSG_ERROR("Null parameter of bridge_vlan_ids passed", 0, 0, 0);
    return false;
  }

  /* Get saved vlan_id's under wwan handle in qcmap_lan */
  if (QCMAP_LAN_Client::UciGetUtility(owrt_filename[0], "profile", 0, "vlan_ids", result, profile_idx))
  {
    strlcpy(bridge_vlan_ids, result, strlen(result)+1);
    return true;
  }
  *qmi_err_num = QMI_ERR_INTERNAL_V01;
  return false;
} /* End GetMappedVLANPerPDN() */

/*===========================================================================
FUNCTION CheckIfVLANMappedToIPPTPDN()
===========================================================================*/
/** @ingroup qcmap_check_if_vlan_mapped_to_IPPT_PDN

  checks if vlan is mapped to a pdn with IPPT set to enabled mode

  @param[in]      vlan_id                 vlan ID to be deleted
  @param[in]      profile_index           profile index if mapping is found
  @param[in]      profile_num             profile number if mapping is found

  @return
  TRUE -- Success \n
  FALSE -- Failure
*/
/*=========================================================================*/
boolean QCMAP_LAN_Client::CheckIfVLANMappedToIPPTPDN
(
  uint16_t vlan_id,
  int *profile_index,
  int *profile_num,
  qmi_error_type_v01 *qmi_err_num
)
{
  int no_of_profiles = -1, profile_idx = -1;
  int16_t bridge_context = -1;
  int profile_id = -1;

  if (NULL == profile_index)
  {
    LOG_MSG_ERROR("Null parameter of profile_index passed", 0, 0, 0);
    *qmi_err_num = QMI_ERR_INVALID_ARG_V01;
    return false;
  }

  /* Get the number of profiles */
  if (!QCMAP_LAN_Client::GetNumberOfProfiles(&no_of_profiles))
  {
    LOG_MSG_ERROR("Failed to get number of profiles from qcmap_lan", 0, 0, 0);
    *qmi_err_num = QMI_ERR_INVALID_ARG_V01;
    return false;
  }
  /* Iterate through each of the profiles */
  for (profile_idx = 0; profile_idx < no_of_profiles; profile_idx++)
  {
    /* check if ippt_enable is set */
    if (QCMAP_LAN_Client::GetEnableIPPTValuePerPDN((const int)profile_idx))
    {
      /* Indicates IPPT enable parameter is set */
      /* Get profile_id linked to profile_idx */
      if (QCMAP_LAN_Client::GetPDNProfileNumber(profile_idx, &profile_id))
      {
        LOG_MSG_INFO1(" Profile handle of PDN where ippt is found to be enabled: %d",
                      profile_id, 0, 0);
      }
      /* get ippt_bridge_context */
      if (!QCMAP_LAN_Client::GetIPPTBridgeContext((const int)profile_idx, &bridge_context))
      {
        LOG_MSG_ERROR("Failed to obtain ippt_bridge_contextippt_bridge_context linked to PDN: %d",
                      profile_id, 0, 0);
        continue;
      }
      /* validate it against vlan to be deleted */
      if ((uint16_t)bridge_context == vlan_id)
      {
        /* if yes, print a warning message and return profile_idx and profile_num */
        LOG_MSG_ERROR("Warning: IPPT bridge context of Profile number: %d is set to the user provided vlan-id: %d",
                      profile_id, vlan_id, 0);
        *profile_index = profile_idx;
        *profile_num = profile_id;
        return true;
      }
    }
  }
  return false;
} /* End CheckIfVLANMappedToIPPTPDN() */
