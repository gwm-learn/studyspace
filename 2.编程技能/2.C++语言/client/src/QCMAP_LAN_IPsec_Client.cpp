/*====================================================

FILE:  QCMAP_LAN_IPsec_Client.cpp

SERVICES:
QCMAP LAN IPsec Client Implementation

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
  08/23/23   ac         Add support for IPsec feature in Openwrt
  ===========================================================================*/

#include "QCMAP_LAN_Client.h"
#include "QCMAP_Client.h"


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
boolean QCMAP_LAN_Client::SetIPsecTunnelInfo
(
  qcmap_ipsec_config_t *ipsec_config,
  qmi_error_type_v01 *qmi_err_num
)
{
  char command[QCMAP_MAX_COMMAND_LEN*2] = {0};
  char result[QCMAP_MAX_STRING_LEN] = {0};
  in_addr addr;
  in6_addr ipv6_addr;
  qcmap_lan_client_feature_mode_config feature_mode_config;
  int no_of_profiles;
  int profile_id;
  uint64_t features = 0;

  if ( ipsec_config == NULL || ipsec_config->topology == 0)
  {
    LOG_MSG_ERROR("Invalid ipsec_config/topology passed",0,0,0);
    *qmi_err_num = QMI_ERR_INVALID_ARG_V01;
    return false;
  }

  /*Check if feature mode is enabled*/
  if(QCMAP_LAN_Client::GetFeatureMode(&features, &feature_mode_config, qmi_err_num)){
    if (feature_mode_config.ipsec_feature_valid) {
      if (!feature_mode_config.ipsec_feature_enable) {
         printf("IPsec feature NOT enabled!\n");
         LOG_MSG_ERROR("IPsec feature NOT enabled!",0,0,0);
         return false;
      }
    }
  }

  /*Check if profile_id is valid*/
  snprintf(command, QCMAP_MAX_COMMAND_LEN, "%s qcmap_lan.@no_of_configs[0].no_of_profiles", UCI_GET_COMMAND);
  if (ExecuteSystemCmd((const char *)command, result, sizeof(result))) {
    no_of_profiles = atoi(result);
    int ind=0;
    /*Loop over profiles to check if current profile exist*/
    for (ind=0; ind<no_of_profiles;ind++) {
      snprintf(command, QCMAP_MAX_COMMAND_LEN, "%s qcmap_lan.@profile[%d].profile_id", UCI_GET_COMMAND, ind);
      if (ExecuteSystemCmd((const char *)command, result, sizeof(result))) {
        result[strlen(result)-1]='\0';
        profile_id = atoi(result);
        if (profile_id == ipsec_config->profile_id) {
          break;
        }
      }
    }

    if (ind == no_of_profiles) {
      printf("Invalid profile_id\n");
      LOG_MSG_ERROR("Invalid profile_id ", 0, 0, 0);
      *qmi_err_num = QMI_ERR_INVALID_ARG_V01;
      return false;
    }
  } else {
    LOG_MSG_ERROR("Failed to get no_of_profiles ", 0, 0, 0);
    *qmi_err_num = QMI_ERR_INVALID_ARG_V01;
    return false;
  }



  /*Check if special character is presented in the ike_identifier*/
  for(int i=0;i<strlen(ipsec_config->ike_cfg.ike_identifier);i++){
    if (!isalpha(ipsec_config->ike_cfg.ike_identifier[i]) && !isdigit(ipsec_config->ike_cfg.ike_identifier[i])) {
       LOG_MSG_ERROR("Invalid ike_identifier",0,0,0);
       *qmi_err_num = QMI_ERR_INVALID_ARG_V01;
       return false;
    }
  }

  /*Check for dup ike_identifier*/
  snprintf(command, QCMAP_MAX_COMMAND_LEN, "%s ipsec.%s", UCI_GET_COMMAND, ipsec_config->ike_cfg.ike_identifier);
  if (ExecuteSystemCmd((const char *)command, result, sizeof(result))) {
    result[strlen(result)-1]='\0';
    if (strcmp(result,"remote")==0) {
      printf("This IKE identifier already exists\n");
      LOG_MSG_ERROR("This IKE identifier already exists",0,0,0);
      *qmi_err_num = QMI_ERR_INVALID_ARG_V01;
      return false;
    }
  }

  /*Check for dup remote_ep_addr*/
  snprintf(command, QCMAP_MAX_COMMAND_LEN, "%s %s %s", QCMAP_IPSEC_SCRIPT, "checkdupremoteepaddr", ipsec_config->ike_cfg.remote_ep_addr);
  if (ExecuteSystemCmd((const char *)command, result, sizeof(result))) {
    result[strlen(result)-1]='\0';
    if (strstr(result,"duplicate remote_ep_addr") != NULL) {
      printf("Duplicate remote_ep_addr\n");
      LOG_MSG_ERROR("This remote_ep_addr already exists", 0, 0, 0);
      *qmi_err_num = QMI_ERR_INVALID_ARG_V01;
      return false;
    }
  }


  /*Check tunnel_type*/
  if (ipsec_config->ike_cfg.tunnel_type == 0 ||
      (ipsec_config->ike_cfg.tunnel_type != QCMAP_IPSEC_V4_ESP_TUNNEL_MODE_TUNNEL_TYPE && ipsec_config->ike_cfg.tunnel_type != QCMAP_IPSEC_V6_ESP_TUNNEL_MODE_TUNNEL_TYPE)) {
    LOG_MSG_ERROR("Invalid Tunnel type", 0, 0, 0);
    *qmi_err_num = QMI_ERR_INVALID_ARG_V01;
    return false;
  }

  /*Check auth_type*/
  if (ipsec_config->ike_cfg.auth_type == 0 ||
      (ipsec_config->ike_cfg.auth_type != QCMAP_IPSEC_PSK_AUTHENTICATION_TYPE && ipsec_config->ike_cfg.auth_type != QCMAP_IPSEC_X509_AUTHENTICATION_TYPE)) {
    LOG_MSG_ERROR("Invalid auth type", 0, 0, 0);
    *qmi_err_num = QMI_ERR_INVALID_ARG_V01;
    return false;
  }

  /*Check local_identifier and remote_identifier*/
  for (int i = 0; i < strlen(ipsec_config->ike_cfg.local_identifier); i++) {
    if (!isalpha(ipsec_config->ike_cfg.local_identifier[i]) && !isdigit(ipsec_config->ike_cfg.local_identifier[i])) {
      LOG_MSG_ERROR("Invalid local_identifier",0,0,0);
      *qmi_err_num = QMI_ERR_INVALID_ARG_V01;
      return false;
   }
  }
  for(int i=0;i<strlen(ipsec_config->ike_cfg.remote_identifier);i++){
    if (!isalpha(ipsec_config->ike_cfg.remote_identifier[i]) && !isdigit(ipsec_config->ike_cfg.remote_identifier[i])) {
      LOG_MSG_ERROR("Invalid remote_identifier",0,0,0);
      *qmi_err_num = QMI_ERR_INVALID_ARG_V01;
      return false;
   }
  }

  /*Check for ike rekey time*/
  if (ipsec_config->ike_cfg.rekey_interval_hrs<=0 || ipsec_config->ike_cfg.rekey_interval_hrs>254) {
    LOG_MSG_ERROR("Invalid ike rekey time",0,0,0);
    *qmi_err_num = QMI_ERR_INVALID_ARG_V01;
    return false;
  }

  /*Check topology type*/
  if (ipsec_config->topology == 0) {
    LOG_MSG_ERROR("Invalid Topology type", 0, 0, 0);
    *qmi_err_num = QMI_ERR_INVALID_ARG_V01;
    return false;
  }


  if (ipsec_config->topology == QCMAP_IPSEC_HOST_TO_HOST_TOPOLOGY
      || ipsec_config->topology == QCMAP_IPSEC_SITE_TO_SITE_WITHOUT_NAT_TOPOLOGY
      || ipsec_config->topology == QCMAP_IPSEC_HOST_TO_HOST_AND_SITE_TO_SITE_WITHOUT_NAT_TOPOLOGY)
  {
    /*Check child entries*/
    if (ipsec_config->child_entries<0) {
      LOG_MSG_ERROR("Invalid Child entries", 0, 0, 0);
      *qmi_err_num = QMI_ERR_INVALID_ARG_V01;
      return false;
    }

    for (int i = 0; i < ipsec_config->child_entries; i++) {
      /*Check child_identifier*/
      for(int j=0;j<strlen(ipsec_config->child_cfg[i].child_identifier);j++){
        if (!isalpha(ipsec_config->child_cfg[i].child_identifier[j]) && !isdigit(ipsec_config->child_cfg[i].child_identifier[j])) {
          LOG_MSG_ERROR("Invalid child_identifier",0,0,0);
          *qmi_err_num = QMI_ERR_INVALID_ARG_V01;
          return false;
        }
      }

      /*Check dup child_identifier*/
      snprintf(command, QCMAP_MAX_COMMAND_LEN, "%s %s %s", QCMAP_IPSEC_SCRIPT, "checkdupchildid", ipsec_config->child_cfg[i].child_identifier);
       if (ExecuteSystemCmd((const char *)command, result, sizeof(result))) {
        result[strlen(result)-1]='\0';
        if (strstr(result,"duplicate child_identifier") != NULL) {
          printf("Duplicate child_identifier\n");
          LOG_MSG_ERROR("This child_identifier already exists", 0, 0, 0);
          *qmi_err_num = QMI_ERR_INVALID_ARG_V01;
          return false;
        }
       }

      /*Check child hw_offload*/
      if (ipsec_config->child_cfg[i].hw_offload != 0 && ipsec_config->child_cfg[i].hw_offload != 1) {
        LOG_MSG_ERROR("Invalid child_hw_offload_option",0,0,0);
        *qmi_err_num = QMI_ERR_INVALID_ARG_V01;
        return false;
      }

      /*Check trap_action*/
      if (ipsec_config->child_cfg[i].trap_action != 0 && ipsec_config->child_cfg[i].trap_action != 1) {
        LOG_MSG_ERROR("Invalid child_trap_action_option",0,0,0);
        *qmi_err_num = QMI_ERR_INVALID_ARG_V01;
        return false;
      }

      /*Check child_rekey_time*/
      if (ipsec_config->child_cfg[i].rekey_interval_hrs<=0 || ipsec_config->child_cfg[i].rekey_interval_hrs>255) {
        LOG_MSG_ERROR("Invalid child rekey time",0,0,0);
        *qmi_err_num = QMI_ERR_INVALID_ARG_V01;
        return false;
      }

      /*Check port id*/
      if (ipsec_config->child_cfg[i].port_id<0) {
        LOG_MSG_ERROR("Invalid child port_id",0,0,0);
        *qmi_err_num = QMI_ERR_INVALID_ARG_V01;
        return false;
      }

       /*Check port range*/
      if (ipsec_config->child_cfg[i].port_range != 0 && ipsec_config->child_cfg[i].port_range != 1) {
        LOG_MSG_ERROR("Invalid child port_range",0,0,0);
        *qmi_err_num = QMI_ERR_INVALID_ARG_V01;
        return false;
      }

      /*Check end_port_id*/
      if (ipsec_config->child_cfg[i].port_range == 1) {
        if (ipsec_config->child_cfg[i].end_port_id < 0 || ipsec_config->child_cfg[i].end_port_id <= ipsec_config->child_cfg[i].port_id) {
          LOG_MSG_ERROR("Invalid child end_port_id",0,0,0);
          *qmi_err_num = QMI_ERR_INVALID_ARG_V01;
          return false;
        }
      }

      /*Check protocol type*/
      if (ipsec_config->child_cfg[i].port_id != 0) {
        if (ipsec_config->child_cfg[i].protocol_type != QCMAP_IPSEC_TCP_PROTOCOL_TYPE && ipsec_config->child_cfg[i].protocol_type != QCMAP_IPSEC_UDP_PROTOCOL_TYPE) {
          LOG_MSG_ERROR("Invalid child protocol type",0,0,0);
          *qmi_err_num = QMI_ERR_INVALID_ARG_V01;
          return false;
        }
      }

      /*Check if local/remote address is NULL for S2S*/
      if (ipsec_config->topology == QCMAP_IPSEC_SITE_TO_SITE_WITHOUT_NAT_TOPOLOGY && 
           (!strlen(ipsec_config->child_cfg[i].local_addr) || !strlen(ipsec_config->child_cfg[i].remote_addr)))
      {
        LOG_MSG_ERROR("Empty subnet address for Site-to-Site without NAT topology",0,0,0);
        *qmi_err_num = QMI_ERR_INVALID_ARG_V01;
        return false;
      }

      /*Check local subnet*/
      if (strlen(ipsec_config->child_cfg[i].local_addr) != 0)
      {
        struct in_addr a4;
        struct in6_addr a6;

        if (ares_inet_net_pton(AF_INET, ipsec_config->child_cfg[i].local_addr, &a4, sizeof(a4)) != QCMAP_LAN_INVALID) {
          LOG_MSG_INFO1("Valid IPv4: %s.", ipsec_config->child_cfg[i].local_addr, 0, 0);
        }else if(ares_inet_net_pton(AF_INET6, ipsec_config->child_cfg[i].local_addr, &a6, sizeof(a6)) != QCMAP_LAN_INVALID){
          LOG_MSG_INFO1("Valid IPv6: %s.", ipsec_config->child_cfg[i].local_addr, 0, 0);
        }else{
          LOG_MSG_ERROR("Invalid local IP\n",0,0,0);
          *qmi_err_num = QMI_ERR_INVALID_ARG_V01;
          return false;
        }


        //Check dup local address
        snprintf(command, QCMAP_MAX_COMMAND_LEN, "%s %s %s", QCMAP_IPSEC_SCRIPT, "checkduplocalsubnet", ipsec_config->child_cfg[i].local_addr);
        if (ExecuteSystemCmd((const char *)command, result, sizeof(result)))
        {
          result[strlen(result)-1]='\0';
          if (strcmp(result,"duplicate local_subnet")==0)
          {
            printf("Local subnet %s already exists - duplicates not allowed\n",ipsec_config->child_cfg[i].local_addr);
            LOG_MSG_ERROR("Local subnet %s already exists - duplicates not allowed\n", ipsec_config->child_cfg[i].local_addr, 0, 0);
            *qmi_err_num = QMI_ERR_INVALID_ARG_V01;
            return false;
          }
        }
      }

      /*Check remote subnet*/
      if (strlen(ipsec_config->child_cfg[i].remote_addr) != 0)
      {
        struct in_addr a4;
        struct in6_addr a6;

        if (ares_inet_net_pton(AF_INET, ipsec_config->child_cfg[i].remote_addr, &a4, sizeof(a4)) != QCMAP_LAN_INVALID) {
          LOG_MSG_INFO1("Valid IPv4: %s.", ipsec_config->child_cfg[i].remote_addr, 0, 0);
        }else if(ares_inet_net_pton(AF_INET6, ipsec_config->child_cfg[i].remote_addr, &a6, sizeof(a6)) != QCMAP_LAN_INVALID){
          LOG_MSG_INFO1("Valid IPv6: %s.", ipsec_config->child_cfg[i].remote_addr, 0, 0);
        }else{
          LOG_MSG_ERROR("Invalid remote IP\n",0,0,0);
          *qmi_err_num = QMI_ERR_INVALID_ARG_V01;
          return false;
        }

        //Check dup remote address
        snprintf(command, QCMAP_MAX_COMMAND_LEN, "%s %s %s", QCMAP_IPSEC_SCRIPT, "checkdupremoteepaddrsubnet", ipsec_config->child_cfg[i].remote_addr);
        if (ExecuteSystemCmd((const char *)command, result, sizeof(result)))
        {
          result[strlen(result)-1]='\0';
          if (strcmp(result,"duplicate remote_subnet")==0)
          {
            printf("Remote subnet %s already exists - duplicates not allowed\n",ipsec_config->child_cfg[i].remote_addr);
            LOG_MSG_ERROR("Remote subnet %s already exists - duplicates not allowed\n", ipsec_config->child_cfg[i].remote_addr, 0, 0);
            *qmi_err_num = QMI_ERR_INVALID_ARG_V01;
            return false;
          }
        }
      }

      /*Check if local and remote address are the same IP type*/
      if (strlen(ipsec_config->child_cfg[i].local_addr) != 0 &&
            strlen(ipsec_config->child_cfg[i].remote_addr) != 0)
      {
        struct in_addr a4;
        struct in6_addr a6;
        bool ipv4_local = false;
        bool ipv6_local = false;

        if (ares_inet_net_pton(AF_INET, ipsec_config->child_cfg[i].local_addr, &a4, sizeof(a4)) != QCMAP_LAN_INVALID) {
          ipv4_local = true;
        }else if(ares_inet_net_pton(AF_INET6, ipsec_config->child_cfg[i].local_addr, &a6, sizeof(a6)) != QCMAP_LAN_INVALID){
          ipv6_local = true;
        }else{
          LOG_MSG_ERROR("Invalid local IP\n",0,0,0);
          *qmi_err_num = QMI_ERR_INVALID_ARG_V01;
          return false;
        }

        /*if remote_addr is ipv4 and local_addr is not ipv4*/
        if ((ares_inet_net_pton(AF_INET, ipsec_config->child_cfg[i].remote_addr, &a4, sizeof(a4)) != QCMAP_LAN_INVALID) && !ipv4_local)
        {
           LOG_MSG_ERROR("Incompatible with local address",0,0,0);
           *qmi_err_num = QMI_ERR_INVALID_ARG_V01;
           return false;
        }

        /*if remote_addr is ipv6 and local_addr is not ipv6*/
        if ((ares_inet_net_pton(AF_INET6, ipsec_config->child_cfg[i].remote_addr, &a6, sizeof(a6)) != QCMAP_LAN_INVALID) && !ipv6_local)
        {
           LOG_MSG_ERROR("Incompatible with local address",0,0,0);
           *qmi_err_num = QMI_ERR_INVALID_ARG_V01;
           return false;
        }

      }

    }

  }

  /*Send child info to UCI DB*/
  for (int i = 0; i < ipsec_config->child_entries; i++)
  {
    printf("\nChild %d\n",i+1);
    snprintf(command, QCMAP_MAX_COMMAND_LEN*2, "%s %s %d %s %d %d %s %s %s %u %d %s %d %d %u %u %d %u %d %s %s", QCMAP_IPSEC_SCRIPT, SET_IPSEC_CONFIG,
       ipsec_config->profile_id, ipsec_config->ike_cfg.ike_identifier,ipsec_config->ike_cfg.tunnel_type,
       ipsec_config->ike_cfg.auth_type, ipsec_config->ike_cfg.remote_ep_addr,
       ipsec_config->ike_cfg.local_identifier, ipsec_config->ike_cfg.remote_identifier,
       ipsec_config->ike_cfg.rekey_interval_hrs, ipsec_config->topology,
       ipsec_config->child_cfg[i].child_identifier, ipsec_config->child_cfg[i].hw_offload,
       ipsec_config->child_cfg[i].trap_action, ipsec_config->child_cfg[i].rekey_interval_hrs,
       ipsec_config->child_cfg[i].port_id, ipsec_config->child_cfg[i].port_range, ipsec_config->child_cfg[i].end_port_id,
       ipsec_config->child_cfg[i].protocol_type, ipsec_config->child_cfg[i].local_addr,
       ipsec_config->child_cfg[i].remote_addr);
    ds_system_call(command, strlen(command));
  }

  /*Check if the configuration write into UCI DB*/
  snprintf(command, QCMAP_MAX_COMMAND_LEN, "%s ipsec.%s", UCI_GET_COMMAND, ipsec_config->ike_cfg.ike_identifier);
  if (ExecuteSystemCmd((const char *)command, result, sizeof(result)))
  {
    result[strlen(result)-1]='\0';
    if (strcmp(result,"remote")==0)
    {
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
*/
/*=========================================================================*/
boolean QCMAP_LAN_Client::ActivateIPsecTunnelInfo
(
  char *ike_identifier,
  char *child_identifier,
  qmi_error_type_v01 *qmi_err_num
)
{
  char result[QCMAP_MAX_STRING_LEN] = {0};
  char command[QCMAP_MAX_COMMAND_LEN] = {0};
  int active_tunnels=0;
  qcmap_lan_client_feature_mode_config feature_mode_config;
  qcmap_ipsec_tunnel_state_info_t state_info;
  qmi_error_type_v01 qmi_err_num1;
  uint64_t features = 0;
  char *ptr;

  memset(&feature_mode_config, 0, sizeof(qcmap_lan_client_feature_mode_config));
  memset(command, 0, QCMAP_MAX_COMMAND_LEN);
  memset(result, 0, QCMAP_MAX_STRING_LEN);

  /*Check if feature mode is enabled*/
  if (QCMAP_LAN_Client::GetFeatureMode(&features, &feature_mode_config, qmi_err_num)){
    if (feature_mode_config.ipsec_feature_valid) {
      if (!feature_mode_config.ipsec_feature_enable) {
        printf("IPsec feature NOT enabled!\n");
        LOG_MSG_ERROR("IPsec feature NOT enabled!",0,0,0);
        return false;
      }
    }
  }

  /*Check if ike_identifier is null*/
  if (ike_identifier == NULL) {
    LOG_MSG_ERROR("No ike_identifier provided!",0,0,0);
    *qmi_err_num = QMI_ERR_INVALID_ARG_V01;
    return false;
  }

  /*Check if ike_identifier exisits in the UCI DB*/
  snprintf(command, QCMAP_MAX_COMMAND_LEN, " %s ipsec.%s", UCI_GET_COMMAND, ike_identifier);
  if (ExecuteSystemCmd((const char *)command, result, sizeof(result))) {
    result[strlen(result)-1]='\0';
    if (strcmp(result,"remote")!=0) {
      printf("IKE_identifier NOT found\n");
      LOG_MSG_ERROR("IKE_identifier NOT found ",0,0,0);
      return false;
    }
  } else {
    printf("IKE_identifier NOT found\n");
    LOG_MSG_ERROR("Failed to get ike info ",0,0,0);
    return false;
  }

  /*Check if child_identifier exisits in the UCI DB*/
  if (strlen(child_identifier)!=0) {
    snprintf(command, QCMAP_MAX_COMMAND_LEN, " %s ipsec.%s", UCI_GET_COMMAND, child_identifier);
    if (ExecuteSystemCmd((const char *)command, result, sizeof(result))) {
      result[strlen(result)-1]='\0';
      if (strcmp(result,"tunnel")!=0) {
        printf("Child_identifier NOT found\n");
        LOG_MSG_ERROR("Child_identifier NOT found ",0,0,0);
        return false;
      }
    } else {
      printf("Child_identifier NOT found\n");
      LOG_MSG_ERROR("Failed to get ike info ",0,0,0);
      return false;
    }
  }

  /*Check no_of_tunnels*/
  snprintf(command, QCMAP_MAX_COMMAND_LEN, "%s qcmap_lan.@global[0].ipsec_active_tunnels", UCI_GET_COMMAND);
  if (ExecuteSystemCmd((const char *)command, result, sizeof(result))) {
    active_tunnels=atoi(result);
    if (strlen(child_identifier)!=0) {
      if (active_tunnels>9) {
        printf("Reached maximum active child SAs\n");
        LOG_MSG_ERROR("Reached maximum active child SAs",0,0,0);
        return false;
      }
    } else {
      snprintf(command, QCMAP_MAX_COMMAND_LEN, " %s ipsec.%s.tunnel", UCI_GET_COMMAND, ike_identifier);
      if (ExecuteSystemCmd((const char *)command, result, sizeof(result))) {
        result[strlen(result)-1]='\0';
        int count = 0;
        char* token;
        char strCopy[QCMAP_MAX_STRING_LEN] = {0};

        /*Count no_of child SAs for this ike*/
        strlcpy(strCopy, result, sizeof(result));
        token = strtok_r(strCopy, " ", &ptr);

        while (token != NULL) {
          count++;
          token = strtok_r(NULL, " ", &ptr);
        }

        if (count+active_tunnels>10) {
          printf("Reached maximum active child SAs\n");
          LOG_MSG_ERROR("Reached maximum active child SAs",0,0,0);
          return false;
        }
      }
    }
  }else{
    printf("Failed to get ipsec_active_tunnels\n");
    LOG_MSG_ERROR("Failed to get ipsec_active_tunnels",0,0,0);
    return false;
  }


  /*Check if the child is already activated*/
  GetIPsecTunnelStateInfo(ike_identifier, child_identifier, &state_info, &qmi_err_num1);

  for (int i=0;i<state_info.tunnel_entries; i++) {
    if (strlen(child_identifier)!=0 && strcmp(state_info.tunnel_status[i].child_identifier, child_identifier)!=0) {
      /*If child_identifier is provided and not matching the current identifier*/
      continue;
    }

    if (state_info.tunnel_status[i].enable_status && state_info.tunnel_status[i].state==2) {
      printf("Skipping Activation.\nThis child SA %s is already connected\n", state_info.tunnel_status[i].child_identifier);
      LOG_MSG_ERROR("This child SA is already connected",0,0,0);
    }else{
      printf("Activating child %s\n", state_info.tunnel_status[i].child_identifier);
      snprintf(command, QCMAP_MAX_COMMAND_LEN, "%s %s %s %s", QCMAP_IPSEC_SCRIPT, ACTIVATE_IPSEC_TUNNEL, ike_identifier, state_info.tunnel_status[i].child_identifier);
      ds_system_call(command, strlen(command));
    }
  }

  return true;

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
*/
/*=========================================================================*/
boolean QCMAP_LAN_Client::DeleteIPsecTunnel
(
  char *ike_identifier,
  char *child_identifier,
  qmi_error_type_v01 *qmi_err_num
)
{
  char result[QCMAP_MAX_STRING_LEN] = {0};
  char command[QCMAP_MAX_COMMAND_LEN] = {0};
  qcmap_lan_client_feature_mode_config feature_mode_config;
  uint64_t features = 0;

  memset(&feature_mode_config, 0, sizeof(qcmap_lan_client_feature_mode_config));
  memset(command, 0, QCMAP_MAX_COMMAND_LEN);
  memset(result, 0, QCMAP_MAX_STRING_LEN);

  /*Check if feature mode is enabled*/
  if (QCMAP_LAN_Client::GetFeatureMode(&features, &feature_mode_config, qmi_err_num)){
    if (feature_mode_config.ipsec_feature_valid) {
      if (!feature_mode_config.ipsec_feature_enable) {
      printf("IPsec feature NOT enabled!\n");
      LOG_MSG_ERROR("IPsec feature NOT enabled!",0,0,0);
      return false;
      }
    }
  }

  /*Check if ike_identifier exisit*/
  if (ike_identifier == NULL) {
    LOG_MSG_ERROR("No ike_identifier provided!",0,0,0);
    *qmi_err_num = QMI_ERR_INVALID_ARG_V01;
    return false;
  }

  /*Check if ike_identifier exisits in the UCI DB*/
  snprintf(command, QCMAP_MAX_COMMAND_LEN, " %s ipsec.%s", UCI_GET_COMMAND, ike_identifier);
  if (ExecuteSystemCmd((const char *)command, result, sizeof(result))) {
    result[strlen(result)-1]='\0';
    if (strcmp(result,"remote")!=0) {
      printf("IKE_identifier NOT found\n");
      LOG_MSG_ERROR("ike_identifier NOT found ",0,0,0);
      return false;
    }
  } else {
    printf("IKE_identifier NOT found\n");
    LOG_MSG_ERROR("Failed to get ike info ",0,0,0);
    return false;
  }

  /*Check if child_identifier exisits in the UCI DB*/
  if (strlen(child_identifier)!=0) {
    snprintf(command, QCMAP_MAX_COMMAND_LEN, " %s ipsec.%s", UCI_GET_COMMAND, child_identifier);
    if (ExecuteSystemCmd((const char *)command, result, sizeof(result))) {
      result[strlen(result)-1]='\0';
      if (strcmp(result,"tunnel")!=0) {
        printf("Child_identifier NOT found\n");
        LOG_MSG_ERROR("child_identifier NOT found ",0,0,0);
        return false;
      }
    } else {
      printf("Child_identifier NOT found\n");
      LOG_MSG_ERROR("Failed to get ike info ",0,0,0);
      return false;
    }
  }


  printf("ikelan:%s, childid_lan:%s\n",ike_identifier,child_identifier);
  snprintf(command, QCMAP_MAX_COMMAND_LEN, "%s %s %s %s", QCMAP_IPSEC_SCRIPT, DELETE_IPSEC_TUNNEL, ike_identifier, child_identifier);
  ds_system_call(command, strlen(command));
  return true;
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
*/
/*=========================================================================*/
boolean QCMAP_LAN_Client::GetIPsecTunnelInfo
(
  char *ike_identifier,
  char *child_identifier,
  qcmap_ipsec_config_t *ipsec_config,
  qmi_error_type_v01 *qmi_err_num
)
{
  char result[QCMAP_MAX_STRING_LEN] = {0};
  char command[QCMAP_MAX_COMMAND_LEN] = {0};
  qcmap_lan_client_feature_mode_config feature_mode_config;
  uint64_t features = 0;
  char *ptr;

  memset(&feature_mode_config, 0, sizeof(qcmap_lan_client_feature_mode_config));
  memset(command, 0, QCMAP_MAX_COMMAND_LEN);
  memset(result, 0, QCMAP_MAX_STRING_LEN);

  /*Check if feature mode is enabled*/
  if (QCMAP_LAN_Client::GetFeatureMode(&features, &feature_mode_config, qmi_err_num)){
    if (feature_mode_config.ipsec_feature_valid) {
      if (!feature_mode_config.ipsec_feature_enable) {
        printf("IPsec feature NOT enabled!\n");
        LOG_MSG_ERROR("IPsec feature NOT enabled!",0,0,0);
        return false;
      }
    }
  }

  if ( ipsec_config == NULL ){
    LOG_MSG_ERROR("Invalid ipsec_config passed ",0,0,0);
    return false;
  }


  /*Check if ike_identifier exisits in the UCI DB*/
  snprintf(command, QCMAP_MAX_COMMAND_LEN, " %s ipsec.%s", UCI_GET_COMMAND, ike_identifier);
  if (ExecuteSystemCmd((const char *)command, result, sizeof(result))) {
    result[strlen(result)-1]='\0';
    if (strcmp(result,"remote")!=0) {
      printf("IKE_identifier NOT found\n");
      LOG_MSG_ERROR("ike_identifier NOT found ",0,0,0);
      return false;
    }
  } else {
    printf("IKE_identifier NOT found\n");
    LOG_MSG_ERROR("Failed to get ike info ",0,0,0);
    return false;
  }

  /*Check if child_identifier exisits in the UCI DB*/
  if (strlen(child_identifier)!=0) {
    snprintf(command, QCMAP_MAX_COMMAND_LEN, " %s ipsec.%s", UCI_GET_COMMAND, child_identifier);
    if (ExecuteSystemCmd((const char *)command, result, sizeof(result))) {
      result[strlen(result)-1]='\0';
      if (strcmp(result,"tunnel")!=0) {
        printf("Child_identifier NOT found\n");
          LOG_MSG_ERROR("child_identifier NOT found ",0,0,0);
        return false;
      }
    } else {
      printf("Child_identifier NOT found\n");
      LOG_MSG_ERROR("Failed to get ike info ",0,0,0);
      return false;
    }
  }

  /*Get profile id*/
  snprintf(command, QCMAP_MAX_COMMAND_LEN, " %s ipsec.%s.profile_id", UCI_GET_COMMAND, ike_identifier);
  if (ExecuteSystemCmd((const char *)command, result, sizeof(result))) {
    ipsec_config->profile_id = atoi(result);
  }else{
    LOG_MSG_ERROR("Failed to get profile_id ",0,0,0);
    return false;
  }

  /*Get topology*/
  snprintf(command, QCMAP_MAX_COMMAND_LEN, " %s ipsec.%s.topology_type", UCI_GET_COMMAND, ike_identifier);
  if (ExecuteSystemCmd((const char *)command, result, sizeof(result))) {
    ipsec_config->topology = atoi(result);
  }else{
    LOG_MSG_ERROR("Failed to get topology_type ",0,0,0);
    return false;
  }

  /*Get tunnel type*/
  snprintf(command, QCMAP_MAX_COMMAND_LEN, " %s ipsec.%s.tunnel_type", UCI_GET_COMMAND, ike_identifier);
  if (ExecuteSystemCmd((const char *)command, result, sizeof(result))) {
    ipsec_config->ike_cfg.tunnel_type = atoi(result);
  }else{
    LOG_MSG_ERROR("Failed to get tunnel_type ",0,0,0);
    return false;
  }

  /*Get gateway ip*/
  snprintf(command, QCMAP_MAX_COMMAND_LEN, " %s ipsec.%s.gateway", UCI_GET_COMMAND, ike_identifier);
  if (ExecuteSystemCmd((const char *)command, result, sizeof(result))) {
    result[strlen(result)-1]='\0';
    strlcpy(ipsec_config->ike_cfg.remote_ep_addr, result, sizeof(result));
  }else{
    LOG_MSG_ERROR("Failed to get gateway ip ",0,0,0);
    return false;
  }


  /*Get auth_type*/
  snprintf(command, QCMAP_MAX_COMMAND_LEN, " %s ipsec.%s.authentication_method", UCI_GET_COMMAND, ike_identifier);
  if (ExecuteSystemCmd((const char *)command, result, sizeof(result))) {
    result[strlen(result)-1]='\0';
  if (strcmp(result, "psk") == 0) {
    ipsec_config->ike_cfg.auth_type = QCMAP_IPSEC_PSK_AUTHENTICATION_TYPE;
  }else if(strcmp(result, "pubkey") == 0){
    ipsec_config->ike_cfg.auth_type = QCMAP_IPSEC_X509_AUTHENTICATION_TYPE;
  }else{
    ipsec_config->ike_cfg.auth_type = 0;
    LOG_MSG_ERROR("Invalid auth_type ",0,0,0);
    return false;
  }
  }else{
    LOG_MSG_ERROR("Failed to get auth_type ",0,0,0);
    return false;
  }

  /*Get local and remote identifier for x509 case*/
  if (ipsec_config->ike_cfg.auth_type == QCMAP_IPSEC_X509_AUTHENTICATION_TYPE) {
    snprintf(command, QCMAP_MAX_COMMAND_LEN, " %s ipsec.%s.local_identifier", UCI_GET_COMMAND, ike_identifier);
    if (ExecuteSystemCmd((const char *)command, result, sizeof(result))) {
      result[strlen(result)-1]='\0';
      strlcpy(ipsec_config->ike_cfg.local_identifier, result, sizeof(result));
    }else{
      LOG_MSG_ERROR("Failed to get local identifier ",0,0,0);
      return false;
    }

    snprintf(command, QCMAP_MAX_COMMAND_LEN, " %s ipsec.%s.remote_identifier", UCI_GET_COMMAND, ike_identifier);
    if (ExecuteSystemCmd((const char *)command, result, sizeof(result))) {
      result[strlen(result)-1]='\0';
      strlcpy(ipsec_config->ike_cfg.remote_identifier, result, sizeof(result));
    }else{
      LOG_MSG_ERROR("Failed to get remote identifier ",0,0,0);
      return false;
    }
  }

  /*Get ike rekey time*/
  snprintf(command, QCMAP_MAX_COMMAND_LEN, " %s ipsec.%s.rekeytime", UCI_GET_COMMAND, ike_identifier);
  if (ExecuteSystemCmd((const char *)command, result, sizeof(result))) {
    result[strlen(result)-1]='\0';
    ipsec_config->ike_cfg.rekey_interval_hrs= atoi(result);
  }else{
    LOG_MSG_ERROR("Failed to get IKE rekeytime ",0,0,0);
    return false;
  }

  /*Get child_identifier*/
  //could be a list
  snprintf(command, QCMAP_MAX_COMMAND_LEN, " %s ipsec.%s.tunnel", UCI_GET_COMMAND, ike_identifier);
  uint8_t child_entries=0;
  if (ExecuteSystemCmd((const char *)command, result, sizeof(result))) {
    result[strlen(result)-1]='\0';
    char tmp[QCMAP_MAX_STRING_LEN] = {0};

    strlcpy(tmp, result, sizeof(result));
    char* token = strtok_r(tmp, " ", &ptr);

    while (token != NULL) {
      strlcpy(ipsec_config->child_cfg[child_entries].child_identifier, token, sizeof(token));

      /*child hw_offload*/
      snprintf(command, QCMAP_MAX_COMMAND_LEN, " %s ipsec.%s.hw_offload", UCI_GET_COMMAND, token);
      if (ExecuteSystemCmd((const char *)command, result, sizeof(result))) {
        result[strlen(result)-1]='\0';

        if (strcmp(result, "packet")==0) {
          ipsec_config->child_cfg[child_entries].hw_offload = true;
        }else if(strcmp(result, "no")==0){
          ipsec_config->child_cfg[child_entries].hw_offload = false;
        }else{
          LOG_MSG_ERROR("Invalid hw_offload option ",0,0,0);
          return false;
        }
      }

      /*child trap_action*/
      snprintf(command, QCMAP_MAX_COMMAND_LEN, " %s ipsec.%s.trap_action", UCI_GET_COMMAND, token);
      if (ExecuteSystemCmd((const char *)command, result, sizeof(result))) {
        result[strlen(result)-1]='\0';

        if (strcmp(result, "trap")==0) {
          ipsec_config->child_cfg[child_entries].trap_action = true;
        }else if(strcmp(result, "start")==0){
          ipsec_config->child_cfg[child_entries].trap_action = false;
        }else{
          LOG_MSG_ERROR("Invalid trap_action option ",0,0,0);
          return false;
        }
      }

      /*child rekey time*/
      snprintf(command, QCMAP_MAX_COMMAND_LEN, " %s ipsec.%s.rekeytime", UCI_GET_COMMAND, token);
      if (ExecuteSystemCmd((const char *)command, result, sizeof(result))) {
        result[strlen(result)-1]='\0';
        ipsec_config->child_cfg[child_entries].rekey_interval_hrs= atoi(result);
      }else{
        LOG_MSG_ERROR("Failed to get child rekeytime ",0,0,0);
        return false;
      }

      /*Child local subnets*/
      snprintf(command, QCMAP_MAX_COMMAND_LEN, " %s ipsec.%s.local_subnet", UCI_GET_COMMAND, token);
      if (ExecuteSystemCmd((const char *)command, result, sizeof(result))) {
        result[strlen(result)-1]='\0';
        strlcpy(ipsec_config->child_cfg[child_entries].local_addr, result, sizeof(result));
      }

      /*Child remote subnets*/
      snprintf(command, QCMAP_MAX_COMMAND_LEN, " %s ipsec.%s.remote_subnet", UCI_GET_COMMAND, token);
      if (ExecuteSystemCmd((const char *)command, result, sizeof(result))) {
        result[strlen(result)-1]='\0';
        strlcpy(ipsec_config->child_cfg[child_entries].remote_addr, result, sizeof(result));
      }

      child_entries = child_entries+1;
      token = strtok_r(NULL, " ", &ptr);
    }
  }else{
    LOG_MSG_ERROR("Failed to get child  ",0,0,0);
    return false;
  }

  ipsec_config->child_entries= child_entries;
  return true;

}



/*===========================================================================
FUNCTION GetIPsecTunnelStateInfo()
===========================================================================*/
/** @ingroup section_GetIPsecTunnelStateInfo

  Get IPsec Tunnel State Info

  @param[in] char *ike_identifier
  @param[in] char *child_identifier
  @param[in] qcmap_ipsec_config_t *ipsec_config
  @param[out] qmi_error_type_v01 *qmi_err_num

  @return
  TRUE -- Success \n
  FALSE -- Failure \n

  @dependencies
  QCMobileAP must be enabled. @newpage
*/
/*=========================================================================*/
boolean QCMAP_LAN_Client::GetIPsecTunnelStateInfo
(
  char *ike_identifier,
  char *child_identifier,
  qcmap_ipsec_tunnel_state_info_t *state_info,
  qmi_error_type_v01 *qmi_err_num
)
{
  char result[QCMAP_MAX_STRING_LEN] = {0};
  char command[QCMAP_MAX_COMMAND_LEN] = {0};
  qcmap_lan_client_feature_mode_config feature_mode_config;
  qcmap_ipsec_config_t ipsec_config;
  qmi_error_type_v01 qmi_err_num1;
  uint64_t features = 0;

  memset(&feature_mode_config, 0, sizeof(qcmap_lan_client_feature_mode_config));
  memset(command, 0, QCMAP_MAX_COMMAND_LEN);
  memset(result, 0, QCMAP_MAX_STRING_LEN);
  memset(&ipsec_config,0,sizeof(qcmap_ipsec_config_t));

  /*Check if feature mode is enabled*/
  if (QCMAP_LAN_Client::GetFeatureMode(&features, &feature_mode_config, qmi_err_num)){
    if (feature_mode_config.ipsec_feature_valid) {
      if (!feature_mode_config.ipsec_feature_enable) {
      printf("IPsec feature NOT enabled!\n");
      LOG_MSG_ERROR("IPsec feature NOT enabled!",0,0,0);
      return false;
      }
    }
  }

  /*Check if ike_identifier exisits in the UCI DB*/
  snprintf(command, QCMAP_MAX_COMMAND_LEN, " %s ipsec.%s", UCI_GET_COMMAND, ike_identifier);
  if (ExecuteSystemCmd((const char *)command, result, sizeof(result))) {
    result[strlen(result)-1]='\0';
    if (strcmp(result,"remote")!=0) {
      printf("IKE_identifier NOT found\n");
      LOG_MSG_ERROR("IKE_identifier NOT found ",0,0,0);
      return false;
    }
  } else {
    printf("IKE_identifier NOT found\n");
    LOG_MSG_ERROR("Failed to get ike info ",0,0,0);
    return false;
  }

  /*Check if child_identifier exisits in the UCI DB*/
  if (strlen(child_identifier)!=0) {
    snprintf(command, QCMAP_MAX_COMMAND_LEN, " %s ipsec.%s", UCI_GET_COMMAND, child_identifier);
    if (ExecuteSystemCmd((const char *)command, result, sizeof(result))) {
      result[strlen(result)-1]='\0';
      if (strcmp(result,"tunnel")!=0) {
        printf("Child_identifier NOT found\n");
        LOG_MSG_ERROR("Child_identifier NOT found ",0,0,0);
        return false;
      }
    } else {
      printf("Child_identifier NOT found\n");
      LOG_MSG_ERROR("Failed to get ike info ",0,0,0);
      return false;
    }
  }


  GetIPsecTunnelInfo(ike_identifier, child_identifier, &ipsec_config, &qmi_err_num1);

  state_info->tunnel_entries = ipsec_config.child_entries;

  for (int i = 0; i < ipsec_config.child_entries; i++) {

    strlcpy(state_info->tunnel_status[i].child_identifier, ipsec_config.child_cfg[i].child_identifier, sizeof(ipsec_config.child_cfg[i].child_identifier));

    /*Get enable option*/
    snprintf(command, QCMAP_MAX_STRING_LEN, " %s ipsec.%s.enabled", UCI_GET_COMMAND, ipsec_config.child_cfg[i].child_identifier);
    if (ExecuteSystemCmd((const char *)command, result, sizeof(result))) {
      state_info->tunnel_status[i].enable_status=atoi(result);
    }else{
      printf("Failed to get enable status\n");
      LOG_MSG_ERROR("Failed to get enable status ",0,0,0);
      return false;
    }

    /*Get state*/
    snprintf(command, QCMAP_MAX_COMMAND_LEN, "%s %s %s %s", QCMAP_IPSEC_SCRIPT, GET_IPSEC_TUNNEL_STATUS, ike_identifier, ipsec_config.child_cfg[i].child_identifier);
    if (ExecuteSystemCmd((const char *)command, result, sizeof(result))) {
      result[strlen(result)-1]='\0';
      printf("result: %s\n",result);
      if (strstr(result,"child is active") != NULL) {
        state_info->tunnel_status[i].state = QCMAP_IPSEC_TUNNEL_CONNECTED;
      }else if(strstr(result,"child is not active") != NULL){
        state_info->tunnel_status[i].state = QCMAP_IPSEC_TUNNEL_DISCONNECTED;
      }else if(strstr(result,"child is connecting") != NULL){
        state_info->tunnel_status[i].state = QCMAP_IPSEC_TUNNEL_INPROGRESS;
      }
    }

  }
  return true;

}



/*===========================================================================
FUNCTION ResetIPsecFeatureMode()
===========================================================================*/
/** @ingroup section_ResetIPsecFeatureMode

  Resets IPsec feature mode provided by the user/caller

  @datatypes
  qcmap_lan_client_feature_mode_config

  @param[in]  qcmap_lan_client_feature_mode_config *feature_mode_config
  @param[out] qmi_error_type_v01 *qmi_err_num

  @return
  TRUE -- Success \n
  FALSE -- Failure \n
*/
/*=========================================================================*/
boolean QCMAP_LAN_Client::ResetIPsecFeatureMode
(
  qcmap_lan_client_feature_mode_config *feature_mode_config,
  qmi_error_type_v01                  *qmi_err_num
)
{
  if (feature_mode_config->ipsec_feature_valid)
  {
    char cmd[QCMAP_MAX_COMMAND_LEN] = {0};
    char result[QCMAP_MAX_SCAN_SIZE] = {0};

    /*Disable IPsec enable option*/
    memset(cmd, 0, QCMAP_MAX_COMMAND_LEN);
    snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "%s %s=%d", UCI_SET_COMMAND, IPSEC_ENABLE_OPTION, 0);
    ds_system_call(cmd, strlen(cmd));
    ExecuteUCICommit();

    /*Set active no_of_tunnel to 0*/
    snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "%s %s=%d", UCI_SET_COMMAND, IPSEC_ACTIVE_TUNNEL, 0);
    ds_system_call(cmd, strlen(cmd));
    ExecuteUCICommit();

    /*Need to clean up IPsec DB*/
    snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "rm /etc/config/ipsec");
    ds_system_call(cmd, strlen(cmd));

    /*Clean up swanctl loaded connections*/
    snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "/etc/init.d/swanctl restart");
    ds_system_call(cmd, strlen(cmd));
    snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "swanctl --load-all");
    ds_system_call(cmd, strlen(cmd));

    /*Restart IPsec service*/
    snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "ipsec restart");
    ds_system_call(cmd, strlen(cmd));

    LOG_MSG_INFO1("IPsec feature reset", 0, 0, 0);
    *qmi_err_num = QMI_ERR_NONE_V01;
    return true;
  }
  else
  {
    LOG_MSG_ERROR("Reset IPsec feature mode failed, ipsec feature is not valid", 0, 0, 0);
    *qmi_err_num = QMI_ERR_MISSING_ARG_V01;
    return false;
  }

}

/*===========================================================================
FUNCTION GetIPsecFeatureMode()
===========================================================================*/
/** @ingroup section_GetIPsecFeatureMode

  Get IPsec feature mode requested by the user/caller

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
boolean QCMAP_LAN_Client::GetIPsecFeatureMode
(
  uint64_t                             *enabled_features,
  qcmap_lan_client_feature_mode_config *feature_mode_config,
  qmi_error_type_v01                   *qmi_err_num
)
{
  char cmd[QCMAP_MAX_COMMAND_LEN] = {0};
  char result[QCMAP_MAX_SCAN_SIZE] = {0};

   /*For IPsec*/
  snprintf(cmd, QCMAP_MAX_COMMAND_LEN, " %s %s", UCI_GET_COMMAND, IPSEC_ENABLE_OPTION);
  if (ExecuteSystemCmd((const char *)cmd, result, sizeof(result)))
  {
    *enabled_features |= QCMAP_ENABLE_IPSEC_FEATURE;
    int ipsec_feature_mode = atoi(result);
    feature_mode_config->ipsec_feature_valid = true;
    if (ipsec_feature_mode == 1)
    {
      feature_mode_config->ipsec_feature_enable = true;
      LOG_MSG_INFO1(" IPsec feature is enabled!", 0, 0, 0);
    }
    else
    {
      feature_mode_config->ipsec_feature_enable = false;
      LOG_MSG_INFO1(" IPsec feature is disabled!", 0, 0, 0);
    }
    *qmi_err_num = QMI_ERR_NONE_V01;
    return true;
  }
  else
  {
    LOG_MSG_ERROR("Get IPsec feature mode failed", 0, 0, 0);
    *qmi_err_num = QMI_ERR_INTERNAL_V01;
    return false;
  }
}


/*===========================================================================
FUNCTION SetIPsecFeatureMode()
===========================================================================*/
/** @ingroup section_SetIPsecFeatureMode

  Set IPsec feature mode provided by the user/caller

  @datatypes
  qcmap_lan_client_feature_mode_config

  @param[in] qcmap_lan_client_feature_mode_config *feature_mode_config
  @param[out] qmi_error_type_v01 *qmi_err_num

  @return
  TRUE -- Success \n
  FALSE -- Failure \n
*/
/*=========================================================================*/
boolean QCMAP_LAN_Client::SetIPsecFeatureMode
(
  qcmap_lan_client_feature_mode_config *feature_mode_config,
  qmi_error_type_v01 *qmi_err_num
)
{
  char cmd[QCMAP_MAX_COMMAND_LEN] = {0};
  char result[QCMAP_MAX_SCAN_SIZE] = {0};

  /*If feature mode is ipsec*/
  if (feature_mode_config->ipsec_feature_valid && feature_mode_config->ipsec_feature_enable)
  {
    /*Set IPsec global enable flag to true*/
    memset(cmd, 0, QCMAP_MAX_COMMAND_LEN);
    snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "%s %s=%d", UCI_SET_COMMAND, IPSEC_ENABLE_OPTION, feature_mode_config->ipsec_feature_enable);
    ds_system_call(cmd, strlen(cmd));
    ExecuteUCICommit();

    /*Disable ipsec.init interface to avoid auto duplicates config during reboot*/
    snprintf(cmd, QCMAP_MAX_COMMAND_LEN, IPSEC_INIT_STOP);
    ds_system_call(cmd, strlen(cmd));
    snprintf(cmd, QCMAP_MAX_COMMAND_LEN, IPSEC_INIT_DISABLE);
    ds_system_call(cmd, strlen(cmd));

    LOG_MSG_INFO1(" IPsec feature is set!", 0, 0, 0);
    *qmi_err_num = QMI_ERR_NONE_V01;
    return true;

  }
  else
  {
    LOG_MSG_ERROR("Set IPsec feature mode failed, ipsec feature is not enabled nor valid", 0, 0, 0);
    *qmi_err_num = QMI_ERR_MISSING_ARG_V01;
    return false;
  }
}