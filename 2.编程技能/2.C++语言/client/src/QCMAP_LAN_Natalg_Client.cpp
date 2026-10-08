/*====================================================

FILE:  QCMAP_LAN_Natalg_Client.cpp

SERVICES:
QCMAP LAN Natalg Client Implementation

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
FUNCTION SetNatType()
===========================================================================*/
/** @ingroup qcmap_set_nat_type

  Set NAT.It triggers NAT_ALG_VPN_CONFIG_FILE shell script which setup NAT rules.

  @datatypes
  int,char*

  @param[in]      wan_profile_handle      BH profile handle number \n
  @param[in]      nat_type                Type of NAT

  @return
  void
*/
/*=========================================================================*/

void QCMAP_LAN_Client::SetNatType
(
  uint32_t wan_profile_handle,
  const char* nat_type,
  qmi_error_type_v01 *qmi_err_num
)
{
  if (wan_profile_handle == 0)
  {
    LOG_MSG_ERROR("Invalid wan profile handle passed, error 0x%x", QMI_ERR_INVALID_ID_V01,0,0);
    *qmi_err_num = QMI_ERR_INVALID_ID_V01;
    return;
  }

  IPv4_DISABLE_NAT_EXCLUSIVITY_CHECK(wan_profile_handle, qmi_err_num, false);

  if (nat_type == NULL)
  {
    LOG_MSG_ERROR("Nat Type is Invalid, error 0x%x", QMI_ERR_INVALID_ARG_V01,0,0);
    *qmi_err_num = QMI_ERR_INVALID_ARG_V01;
    return;
  }
  char publicIpAddr[QCMAP_LAN_MAX_IPV4_ADDR_SIZE];
  char output[QCMAP_MAX_SCAN_SIZE]={0};
  bool ret_val=GetNatType(wan_profile_handle,output, qmi_err_num);
  char cur_nat_type[QCMAP_MAX_SCAN_SIZE]={0};

  if(ret_val)
  {
    if(!strncmp(output,SYMMETRIC_NAT,sizeof(SYMMETRIC_NAT)))
    {
      strlcpy(cur_nat_type, SYMMETRIC_NAT, strlen(SYMMETRIC_NAT)+1);
    }
    else if(!strncmp(output,PORT_RESTRICTED_CONE_NAT,sizeof(PORT_RESTRICTED_CONE_NAT)))
    {
      strlcpy(cur_nat_type, PORT_RESTRICTED_CONE_NAT, strlen(PORT_RESTRICTED_CONE_NAT)+1);
    }
    else if(!strncmp(output,FULL_CONE_NAT,sizeof(FULL_CONE_NAT)))
    {
      strlcpy(cur_nat_type, FULL_CONE_NAT, strlen(FULL_CONE_NAT)+1);
    }
    else if(!strncmp(output, ADDRESS_RESTRICTED_CONE_NAT,sizeof(ADDRESS_RESTRICTED_CONE_NAT)))
    {
      strlcpy(cur_nat_type, ADDRESS_RESTRICTED_CONE_NAT, strlen(ADDRESS_RESTRICTED_CONE_NAT)+1);
    }
  }
  else
  {
    LOG_MSG_ERROR("\n Get NAT Type fail", 0,0,0);
    return;
  }

  if(output[strlen(output) - 1] == '\n')
  {
     output[strlen(output) - 1]  = '\0';
  }

  /* NAT already set as desired type, directly return*/
  if ( !strncmp(output,nat_type,sizeof(SYMMETRIC_NAT)) )
  {
    *qmi_err_num=QMI_ERR_NO_EFFECT_V01;
    LOG_MSG_INFO1("NAT already set as desired type", 0, 0, 0);
    return;
  }

  if(GetV4PublicIP(publicIpAddr, wan_profile_handle))
  {
    /* Delete the conntrack connections based on source NAT and destination NAT flags. */
    QCMAP_LAN_CLIENT_RUN_COMMANDS("conntrack -D -q %s", publicIpAddr);
    QCMAP_LAN_CLIENT_RUN_COMMANDS("conntrack -D -d %s", publicIpAddr);
  }

  QCMAP_LAN_CLIENT_RUN_COMMANDS("%s %s %s %d",NAT_ALG_VPN_CONFIG_FILE,"SET_NAT",nat_type,wan_profile_handle);
  *qmi_err_num = QMI_ERR_NONE_V01;

  return;
}

/*===========================================================================
FUNCTION GetNatType()
===========================================================================*/
/** @ingroup qcmap_set_nat_type

  Get NAT Type. It triggers NAT_ALG_VPN_CONFIG_FILE shell script which print the NAT rules.

  @datatypes
  int,char*

  @param[in]      wan_profile_handle      BH profile handle number
  @param[in]      output                  To store type of NAT of a particular profile id.

  @return
  TRUE -- Success \n
  FALSE -- Failure \n
*/
/*=========================================================================*/

bool QCMAP_LAN_Client::GetNatType
(
  uint32_t wan_profile_handle,
  const char* output,
  qmi_error_type_v01 *qmi_err_num
)
{
  char command[QCMAP_MAX_COMMAND_LEN] = {0};
  char result[QCMAP_MAX_SCAN_SIZE] = {0};

  if (wan_profile_handle == 0)
  {
    LOG_MSG_ERROR("Invalid wan profile handle passed ",0,0,0);
    *qmi_err_num = QMI_ERR_INVALID_ID_V01;
    return false;
  }
  if (output == NULL)
  {
    LOG_MSG_ERROR("Output is NULL",0,0,0);
    return false;
  }
  snprintf(command, QCMAP_MAX_COMMAND_LEN,"%s %s %d",NAT_ALG_VPN_CONFIG_FILE,"GET_NAT", wan_profile_handle);
  if (!ExecuteSystemCmd((const char *)command,result,sizeof(result)))
  {
    LOG_MSG_ERROR("failed to execute command : %s",command,0,0);
    *qmi_err_num = QMI_ERR_INTERNAL_V01;
    return false;
  }
  int i = atoi(result);
  memset(result,0,QCMAP_MAX_SCAN_SIZE);
  if ( QCMAP_LAN_Client::UciGetUtility(owrt_filename[0], "profile", 0, "nat_type", result,i) )
  {
    strlcpy(output, result, strlen(result)+1);
    return true;
  }
  *qmi_err_num = QMI_ERR_INTERNAL_V01;
  return false;
}

/*===========================================================================
FUNCTION EnableNatType()
===========================================================================*/
/** @ingroup qcmap_set_nat_type

  Enable NAT Type. It enable the NAT rules on a particular profile handle when backhaul is UP.

  @datatypes
  int

  @param[in]      wan_profile_handle      BH profile handle number

  @return
  void
*/
/*=========================================================================*/

boolean QCMAP_LAN_Client::EnableNatType
(
 uint32_t wan_profile_handle,
 qmi_error_type_v01 *qmi_err_num
)
{
  if (wan_profile_handle == 0)
  {
    LOG_MSG_ERROR("Invalid wan profile handle passed ",0,0,0);
    *qmi_err_num = QMI_ERR_INVALID_ID_V01;
    return;
  }

  IPv4_DISABLE_NAT_EXCLUSIVITY_CHECK(wan_profile_handle, qmi_err_num, false);

  QCMAP_LAN_CLIENT_RUN_COMMANDS("%s %s %d",NAT_ALG_VPN_CONFIG_FILE,"ENABLE_NAT", wan_profile_handle);
  return true;
}

/*===========================================================================
   FUNCTION AddDMZ()
===========================================================================*/
/** @ingroup qcmap_add_dmz

  Adds the DMZ IP address

  @param[in] dmz_ip   DMZ IP to be added; address is in host byte order.
  @param[in] wan_profile_handle      BH profile handle number
  @param[in] qmi_err_num             To indicate if DMZ is already exists

  @return
  TRUE -- Success.
  FALSE -- Failure.

  @dependencies
  QCMobileAP must be enabled.
*/
/*=========================================================================*/
boolean QCMAP_LAN_Client::AddDMZ
(
  uint32_t wan_profile_handle,
  char* dmz_ip,
  qmi_error_type_v01 *qmi_err_num
)
{
  char command[QCMAP_MAX_COMMAND_LEN] = {0};
  char result[QCMAP_MAX_SCAN_SIZE] = {0};
  qmi_error_type_v01 error_num_temp = QMI_ERR_NONE_V01;

  IPv4_DISABLE_NAT_EXCLUSIVITY_CHECK(wan_profile_handle, qmi_err_num, false);

  /* check if firewall entry is not exceeding the max limit 128*/
  if( !checkSNATEntryLimit( &error_num_temp ) )
  {
    if( error_num_temp == QMI_ERR_INTERNAL_V01)
    {
      LOG_MSG_ERROR("UciGetUtility Failed to fetch info.", 0,0,0);
    }
    else
    {
      LOG_MSG_ERROR("Failed to addDMZ, max entry limit 128 exceeds ", 0,0,0);
    }
    return false;
  }
  if (wan_profile_handle < 1)
  {
    LOG_MSG_ERROR("Invalid wan profile handle passed ", 0,0,0);
    return false;
  }
  if (dmz_ip == NULL || qmi_err_num == NULL)
  {
    LOG_MSG_ERROR("Invalid DMZ or qmi_error_type_v01 IP\n", 0,0,0);
    return false;
  }
  char dmz_ip_owrt[QCMAP_IPV4_ADDR_LEN]={0};
  if (GetDMZ(wan_profile_handle, dmz_ip_owrt, &error_num_temp))
  {
    if (strlen(dmz_ip_owrt) != 0)
    {
       *qmi_err_num = QMI_ERR_NO_EFFECT_V01;
       return false;
    }
  }
  else
  {
    LOG_MSG_ERROR("Failed to check if DMZ is already configured \n", 0,0,0);
    return false;
  }
  snprintf(command, QCMAP_MAX_COMMAND_LEN, "%s %s %d %s", NAT_ALG_VPN_CONFIG_FILE, "ADD_DMZ", wan_profile_handle, dmz_ip);
  ds_system_call(command, strlen(command));
  return true;
} /*End AddDMZ() */


/*===========================================================================
   FUNCTION GetDMZ()
===========================================================================*/
/** @ingroup qcmap_get_dmz

    Gets DMZ IP address

    @param[in] wan_profile_handle      BH profile handle number
    @param[in] dmz_ip                  To store DMZ IP address

    @return
    TRUE -- Success.
    FALSE -- Failure.

    @dependencies
    QCMobileAP must be enabled.
*/
/*=========================================================================*/
boolean QCMAP_LAN_Client::GetDMZ
(
  uint32_t wan_profile_handle,
  char *dmz_ip,
  qmi_error_type_v01 *qmi_err_num
)
{
  uint32_t no_of_redirects = 0;
  uint32_t name = 0;
  char result[QCMAP_MAX_SCAN_SIZE] = {0};
  if (wan_profile_handle < 1)
  {
    LOG_MSG_ERROR("Invalid wan profile handle passed ",0,0,0);
    return false;
  }

  if (dmz_ip == NULL)
  {
    LOG_MSG_ERROR("Output is NULL",0,0,0);
    *qmi_err_num = QMI_ERR_INVALID_ARG_V01;
    return false;
  }

  if (UciGetUtility(owrt_filename[static_cast<int>(config_file::QCMAP_LAN)],
      "no_of_configs", 0, "no_of_redirects", result, 0))
  {
    no_of_redirects = atoi(result);
  }
  else
  {
    LOG_MSG_ERROR("UciGetUtility failed ",0,0,0);
    *qmi_err_num = QMI_ERR_INTERNAL_V01;
    return false;
  }
  for (uint32_t i = 0; i < no_of_redirects; i++)
  {
    memset(result, 0, QCMAP_MAX_SCAN_SIZE);
    if (UciGetUtility(owrt_filename[static_cast<int>(config_file::FIREWALL)],
        "redirect", 0, "name", result, i))
    {
      name = atoi(result);
      if (name == wan_profile_handle)
      {
        memset(result, 0, QCMAP_MAX_SCAN_SIZE);
        if (UciGetUtility(owrt_filename[static_cast<int>(config_file::FIREWALL)],
            "redirect", 0, "dest_ip", result, i))
        {
          if (strlen(result) != 0)
          {
            strlcpy(dmz_ip, result, strlen(result)+1);
            break;
          }
        }
        else
        {
          LOG_MSG_ERROR("UciGetUtility failed ",0,0,0);
          *qmi_err_num = QMI_ERR_INTERNAL_V01;
          return false;
        }
      }
    }
    else
    {
      LOG_MSG_ERROR("UciGetUtility failed ",0,0,0);
      *qmi_err_num = QMI_ERR_INTERNAL_V01;
      return false;
    }
   }
  return true;
}/*End GetDMZ() */

/*===========================================================================
   FUNCTION DeleteDMZ()
===========================================================================*/
/** @ingroup qcmap_delete_dmz

  Delete DMZ IP from QCMAP_LAN UCI DB and delete DMZ config as well

  @param[in] wan_profile_handle      BH profile handle number

  @return
  TRUE -- Success.
  FALSE -- Failure.

  @dependencies
  QCMobileAP must be enabled.
*/
/*=========================================================================*/

boolean QCMAP_LAN_Client::DeleteDMZ
(
  uint32_t wan_profile_handle,
  qmi_error_type_v01 *qmi_err_num
)
{
  char command[QCMAP_MAX_COMMAND_LEN] = {0};
  char result[QCMAP_MAX_SCAN_SIZE] = {0};
  if (wan_profile_handle < 1)
  {
    LOG_MSG_ERROR("Invalid wan profile handle passed ", 0,0,0);
    *qmi_err_num = QMI_ERR_INVALID_PROFILE_V01;
    return false;
  }

  IPv4_DISABLE_NAT_EXCLUSIVITY_CHECK(wan_profile_handle, qmi_err_num, false);

  snprintf(command, QCMAP_MAX_COMMAND_LEN, "%s %s %d", NAT_ALG_VPN_CONFIG_FILE, "DELETE_DMZ", wan_profile_handle);
  ds_system_call(command, strlen(command));
  return true;
}/*End DeleteDMZ() */

/*===========================================================================
  FUNCTION AlgUsrCfg
==========================================================================*/
/*!
@brief
  Enable/Disable Algs Functionality in uci.

@return
  true  - on success
  false - on failure

@note

@dependencies
QCMobileAP must be enabled. @newpage

@Side Effects
None
*/
/*=========================================================================*/
boolean QCMAP_LAN_Client::AlgUsrCfg
(
  qcmap_alg_action_enum action,
  uint32_t wan_profile_handle,
  qcmap_msgr_alg_type_mask_v01 alg_type,
  qmi_error_type_v01    *qmi_err_num
)
{
  qcmap_rtsp_alg_enum rtsp_alg_cfg = (action == QCMAP_ENABLE_ALG) ? QCMAP_RTSP_ALG_ENABLED : QCMAP_RTSP_ALG_DISABLED;
  qcmap_sip_alg_enum sip_alg_cfg = (action == QCMAP_ENABLE_ALG) ? QCMAP_SIP_ALG_ENABLED : QCMAP_SIP_ALG_DISABLED;

  if(alg_type & QCMAP_MSGR_MASK_RTSP_ALG_V01)
  {
    if (!QCMAP_LAN_Client::UciSetUtility(owrt_filename[0], "alg_cfg", 0,
        "rtspalg_enable", rtsp_alg_cfg) )
    {
      return false;
    }
  }
  else
  {
    if (!QCMAP_LAN_Client::UciSetUtility(owrt_filename[0], "alg_cfg", 0,
        "sipalg_enable", sip_alg_cfg) )
    {
      return false;
    }
  }

  if ( !GetBackhaulStatus(wan_profile_handle, IP_V4, qmi_err_num) )
  {
    if (action == QCMAP_ENABLE_ALG)
    {
      LOG_MSG_ERROR("IPv4 backhaul down:cannot enable ALG now, but will change cfg", 0, 0, 0);
    }
    else if (action == QCMAP_DISABLE_ALG)
    {
      LOG_MSG_ERROR(" IPv4 backhaul down: cannot disable ALG", 0,0,0);
    }
    *qmi_err_num=QMI_ERR_INTERFACE_NOT_FOUND_V01;
    return false;
  }

  return true;
}

/*===========================================================================
  FUNCTION EnableAlg
==========================================================================*/
/*!
@brief
  Enables Algs Functionality.

@return
  true  - on success
  false - on failure

@note

@dependencies
QCMobileAP must be enabled. @newpage

@Side Effects
None
*/
/*=========================================================================*/
boolean QCMAP_LAN_Client::EnableAlg
(
  uint32_t wan_profile_handle,
  qcmap_msgr_alg_type_mask_v01 alg_type,
  qmi_error_type_v01    *qmi_err_num
)
{
  bool ret_rtsp = false;
  bool ret_sip = false;
  qcmap_alg_action_enum action=QCMAP_ENABLE_ALG;

  if (!AlgUsrCfg(action, wan_profile_handle, alg_type, qmi_err_num))
  {
    LOG_MSG_ERROR("Fail to enable ALG uci cfg", 0, 0, 0);
    return false;
  }

  if( (alg_type & QCMAP_MSGR_MASK_RTSP_ALG_V01) ==
       QCMAP_MSGR_MASK_RTSP_ALG_V01 )
  {
    /*-----------------------------------------------------------------------
         Install RTSP ALG Kernel Module
        ----------------------------------------------------------------------*/
    ret_rtsp = EnableRTSPAlg(qmi_err_num);
    if ( ret_rtsp != true )
    {
      LOG_MSG_ERROR("Fail to enable RTSP ALG: = %d.\n",
                   ret_rtsp, 0, 0);
    }
    else
    {
      LOG_MSG_INFO1("Successfully enabled RTSP ALG", 0, 0, 0);
    }
  }

  if(alg_type & QCMAP_MSGR_MASK_SIP_ALG_V01)
  {
    ret_sip = EnableSIPAlg(qmi_err_num);
    if ( ret_sip != true )
    {
      LOG_MSG_ERROR("Fail to enable SIP ALG, Ret value = %d",
                    ret_sip, 0, 0);
    }
    else
    {
      LOG_MSG_INFO1("Successfully enabled SIP ALG", 0, 0, 0);
    }
  }

  /*check if any of the requested ALG failed */
  if (((alg_type & QCMAP_MSGR_MASK_RTSP_ALG_V01) && (ret_rtsp == false)) ||
      ((alg_type & QCMAP_MSGR_MASK_SIP_ALG_V01) && (ret_sip == false)))
  {
    LOG_MSG_ERROR("EnableAlg(): Failed for ret_rtsp:%d & ret_sip:%d", ret_rtsp, ret_sip, 0);
    /*check if it is a partial failure in case of multi*/
    if(((alg_type & QCMAP_MSGR_MASK_RTSP_ALG_V01) && (alg_type & QCMAP_MSGR_MASK_SIP_ALG_V01)) &&
       ret_sip != ret_rtsp)
    {
      *qmi_err_num=QMI_ERR_OP_PARTIAL_FAILURE_V01;
      return false;
    }
    return false;
  }
  *qmi_err_num=QMI_ERR_NONE_V01;
  return true;
}

/*===========================================================================
  FUNCTION DisableAlg
==========================================================================*/
/*!
@brief
  Disables Algs Functionality.

@return
  true  - on success
  false - on failure

@note

- Dependencies
- None

- Side Effects
- None
*/
/*=========================================================================*/
boolean QCMAP_LAN_Client::DisableAlg
(
  uint32_t wan_profile_handle,
  qcmap_msgr_alg_type_mask_v01 alg_type,
  qmi_error_type_v01    *qmi_err_num
)
{
  bool ret_rtsp = false;
  bool ret_sip = false;
  qcmap_alg_action_enum action=QCMAP_DISABLE_ALG;

  if (!AlgUsrCfg(action, wan_profile_handle, alg_type, qmi_err_num))
  {
    LOG_MSG_ERROR("Fail to disable ALG uci cfg", 0, 0, 0);
    return false;
  }

  if(alg_type & QCMAP_MSGR_MASK_RTSP_ALG_V01)
  {
    /*-----------------------------------------------------------------------
         Unload the RTSP ALG module from Kernel
         -----------------------------------------------------------------------*/
    ret_rtsp = DisableRTSPAlg(qmi_err_num);
    if ( ret_rtsp != true )
    {
      LOG_MSG_ERROR("Fail to disable RTSP ALG. error = %d.\n",
                    ret_rtsp, 0, 0);
    }
    else
    {
      LOG_MSG_INFO1("Successfully disabled RTSP ALG", 0, 0, 0);
    }
  }

  if(alg_type & QCMAP_MSGR_MASK_SIP_ALG_V01)
  {
    ret_sip = DisableSIPAlg(qmi_err_num);
    if ( ret_sip != true )
    {
      LOG_MSG_ERROR("Fail to disable SIP ALG, Ret value = %d.\n",
                    ret_sip, 0, 0);
    }
    else
    {
      LOG_MSG_INFO1("Successfully disabled SIP ALG", 0, 0, 0);
    }
  }

  /*check if any of the requested ALG failed */
  if (((alg_type & QCMAP_MSGR_MASK_RTSP_ALG_V01) && (ret_rtsp == false)) ||
      ((alg_type & QCMAP_MSGR_MASK_SIP_ALG_V01) && (ret_sip == false)))
  {
    LOG_MSG_ERROR("DisableAlg(): Failed for ret_rtsp:%d & ret_sip:%d", ret_rtsp, ret_sip, 0);
   /*check if it is a partial failure in case of multi*/
    if(((alg_type & QCMAP_MSGR_MASK_RTSP_ALG_V01) && (alg_type & QCMAP_MSGR_MASK_SIP_ALG_V01)) &&
         ret_sip != ret_rtsp)
    {
      *qmi_err_num=QMI_ERR_OP_PARTIAL_FAILURE_V01;
      return false;
    }
    return false;
  }
  *qmi_err_num=QMI_ERR_NONE_V01;
  return true;
}

/*===========================================================================
  FUNCTION EnableRTSPAlg
==========================================================================*/
/*!
@brief
  Enables RTSP Alg Functionality.

@return
  true  - on success
  false - on failure

@note

- Dependencies
- None

- Side Effects
- None
*/
/*=========================================================================*/
boolean QCMAP_LAN_Client::EnableRTSPAlg
(
  qmi_error_type_v01    *qmi_err_num
)
{
  QCMAP_LAN_CLIENT_RUN_COMMANDS("%s %s",NAT_ALG_VPN_CONFIG_FILE, CMD_ENABLE_RTSP_ALG);
  LOG_MSG_INFO1("RTSP ALG enabled", 0, 0, 0);
  *qmi_err_num=QMI_ERR_NONE_V01;
  return true;
}

/*===========================================================================
  FUNCTION DisableRTSPAlg
==========================================================================*/
/*!
@brief
  Disables RTSP Alg Functionality.

@return
  true  - on success
  false - on failure

@note

- Dependencies
- None

- Side Effects
- None
*/
/*=========================================================================*/
boolean QCMAP_LAN_Client::DisableRTSPAlg
(
  qmi_error_type_v01    *qmi_err_num
)
{
  QCMAP_LAN_CLIENT_RUN_COMMANDS("%s %s",NAT_ALG_VPN_CONFIG_FILE, CMD_DISABLE_RTSP_ALG);
  LOG_MSG_INFO1("RTSP ALG disabled", 0, 0, 0);
  *qmi_err_num=QMI_ERR_NONE_V01;
  return true;
}

/*===========================================================================
  FUNCTION EnableSIPAlg
==========================================================================*/
/*!
@brief
  - Enables SIP Alg Functionality.
  - Writes to a proc/sys entry which indicates netfilter to enable SIP ALG
    processing (corresponding code added in kernel)

@return
  true  - on success
  false - on failure

@note

- Dependencies
- None

- Side Effects
- None
*/
/*=========================================================================*/
boolean QCMAP_LAN_Client::EnableSIPAlg
(
  qmi_error_type_v01    *qmi_err_num
)
{
  QCMAP_LAN_CLIENT_RUN_COMMANDS("%s %s",NAT_ALG_VPN_CONFIG_FILE, CMD_ENABLE_SIP_ALG);
  LOG_MSG_INFO1("SIP ALG enabled", 0, 0, 0);
  *qmi_err_num=QMI_ERR_NONE_V01;
  return true;
}

/*===========================================================================
  FUNCTION DisableSIPAlg
==========================================================================*/
/*!
@brief
  - Disables SIP Alg Functionality.
  - Writes to a proc/sys entry which indicates netfilter to bypass SIP ALG
    processing (corresponding code added in kernel)

@return
  true  - on success
  false - on failure

@note

- Dependencies
- None

- Side Effects
- None
*/
/*=========================================================================*/
boolean QCMAP_LAN_Client::DisableSIPAlg
(
  qmi_error_type_v01    *qmi_err_num
)
{
  QCMAP_LAN_CLIENT_RUN_COMMANDS("%s %s",NAT_ALG_VPN_CONFIG_FILE, CMD_DISABLE_SIP_ALG);
  LOG_MSG_INFO1("SIP ALG disabled", 0, 0, 0);
  *qmi_err_num=QMI_ERR_NONE_V01;
  return true;
}

/*===========================================================================
  FUNCTION SetSIPServerInfo
==========================================================================*/
/*!
@brief
  - Sets the default user configured SIP Server Information onto qcmap config.
  - Restarts DHCP server with the user provided SIP server information.

@return
  true  - on success
  false - on failure

@note

- Dependencies
- None

- Side Effects
- None
*/
/*=========================================================================*/
boolean QCMAP_LAN_Client::SetSIPServerInfo
(
  qcmap_msgr_sip_server_info_v01 *default_sip_server_info,
  qmi_error_type_v01              *qmi_err_num
)
{
  if (default_sip_server_info->pcscf_info_type == QCMAP_MSGR_PCSCF_IP_ADDRESS_V01)
  {
    char strIPv4Addr   [INET_ADDRSTRLEN];
    memset(strIPv4Addr,    0, INET_ADDRSTRLEN);
    readable_addr(AF_INET, &default_sip_server_info->pcscf_ip_addr, strIPv4Addr);
    QCMAP_LAN_CLIENT_RUN_COMMANDS("%s %s %d %s",NAT_ALG_VPN_CONFIG_FILE, CMD_SET_SIP_SERVER,
                                 QCMAP_MSGR_PCSCF_IP_ADDRESS_V01, strIPv4Addr);
  }
  else if(default_sip_server_info->pcscf_info_type == QCMAP_MSGR_PCSCF_FQDN_V01)
  {
    QCMAP_LAN_CLIENT_RUN_COMMANDS("%s %s %d %s",NAT_ALG_VPN_CONFIG_FILE, CMD_SET_SIP_SERVER,
                                   QCMAP_MSGR_PCSCF_FQDN_V01, default_sip_server_info->pcscf_fqdn);
  }

  /* Perform dnsmasq reload */
  LOG_MSG_INFO1("Perform dnsmasq reload", 0, 0, 0);
  PerformDnsmasqReload();

  *qmi_err_num=QMI_ERR_NONE_V01;
  return true;
}

/*===========================================================================
  FUNCTION GetSIPServerInfo
==========================================================================*/
/*!
@brief
  - Gets the default user configured SIP Server Information

@return
  true  - on success
  false - on failure

@note

- Dependencies
- None

- Side Effects
- None
*/
/*=========================================================================*/
boolean QCMAP_LAN_Client::GetUsrSIPServerInfo
(
  qcmap_msgr_sip_server_info_v01 *default_sip_server_info,
  qmi_error_type_v01              *qmi_err_num
)
{
  char result[QCMAP_MAX_SCAN_SIZE] = {0};
  char cmd[QCMAP_MAX_COMMAND_LEN] = {0};

  if (!QCMAP_LAN_Client::UciGetUtility(owrt_filename[0], "alg_cfg", 0,
      "default_sip_server_config_type", result, 0))
  {
    *qmi_err_num=QMI_ERR_INTERNAL_V01;
    return false;
  }

  if ( strncmp(result, "FQDN", strlen("FQDN")) == 0 )
  {
    default_sip_server_info->pcscf_info_type = QCMAP_MSGR_PCSCF_FQDN_V01;
    LOG_MSG_INFO1("case FQDN %d", default_sip_server_info->pcscf_info_type, 0, 0);
  }
  else if ( strncmp(result, "IP", strlen("IP")) == 0 )
  {
    default_sip_server_info->pcscf_info_type = QCMAP_MSGR_PCSCF_IP_ADDRESS_V01;
    LOG_MSG_INFO1("case IP %d", default_sip_server_info->pcscf_info_type, 0, 0);
  }

  memset(result,0,QCMAP_MAX_SCAN_SIZE);
  snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "%s %s",
                            NAT_ALG_VPN_CONFIG_FILE, "get_sip_server_info");
  if (!ExecuteSystemCmd((const char *)cmd, result, sizeof(result)))
  {
    LOG_MSG_ERROR("Failed to get usr cfg SIP server info", 0, 0, 0);
    *qmi_err_num=QMI_ERR_INTERNAL_V01;
    return false;
  }

  if(result[strlen(result) - 1] == '\n')
  {
    result[strlen(result) - 1]  = '\0';
  }

  if (default_sip_server_info->pcscf_info_type == QCMAP_MSGR_PCSCF_IP_ADDRESS_V01)
  {
    in_addr addr;
    memset(&addr,0,sizeof(in_addr));
    if (inet_aton(result, &addr))
    {
      default_sip_server_info->pcscf_ip_addr = addr.s_addr;
      LOG_MSG_INFO1("case IP addr.s_addr %d", addr.s_addr, 0, 0);
    }
  }
  else if(default_sip_server_info->pcscf_info_type == QCMAP_MSGR_PCSCF_FQDN_V01)
  {
    strlcpy(default_sip_server_info->pcscf_fqdn, result, strlen(result)+1);
    LOG_MSG_INFO1("case FQDN %s", default_sip_server_info->pcscf_fqdn, 0, 0);
  }

  LOG_MSG_INFO1("Successfully get usr SIP server info", 0, 0, 0);
  *qmi_err_num=QMI_ERR_NONE_V01;
  return true;
}

/*===========================================================================
  FUNCTION UpdateNetworkSIPServerToDHCP
==========================================================================*/
/*!
@brief
  - Update network SIP server info to DHCP uci config

@return
  true  - on success
  false - on failure

@note

- Dependencies
- None

- Side Effects
- None
*/
/*=========================================================================*/
boolean QCMAP_LAN_Client::UpdateNetworkSIPServerToDHCP
(
  qcmap_msgr_sip_server_info_v01 *network_sip_server_info,
  qmi_error_type_v01              *qmi_err_num
)
{
  if (network_sip_server_info->pcscf_info_type == QCMAP_MSGR_PCSCF_IP_ADDRESS_V01)
  {
    QCMAP_LAN_CLIENT_RUN_COMMANDS("%s %s %d %s",NAT_ALG_VPN_CONFIG_FILE,"update_network_sip_server_to_dhcp",
                                  network_sip_server_info->pcscf_ip_addr);
  }
  else if(network_sip_server_info->pcscf_info_type == QCMAP_MSGR_PCSCF_FQDN_V01)
  {
    QCMAP_LAN_CLIENT_RUN_COMMANDS("%s %s %d %s",NAT_ALG_VPN_CONFIG_FILE,"update_network_sip_server_to_dhcp",
                                  network_sip_server_info->pcscf_fqdn);
  }

  LOG_MSG_INFO1("successfully update network SIP server info to DHCP", 0, 0, 0);
  *qmi_err_num=QMI_ERR_NONE_V01;
  return true;
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
boolean QCMAP_LAN_Client::AddPortTriggerEntry
(
  uint32_t                               wan_profile_handle,
  qcmap_msgr_port_trigger_entry_conf_t   port_trigger_entry,
  int                                   *handle,
  qmi_error_type_v01                     *qmi_err_num
)
{
  char command[QCMAP_MAX_COMMAND_LEN] = {0};

  if (wan_profile_handle < 1)
  {
    LOG_MSG_ERROR("Invalid wan profile handle passed ", 0,0,0);
    *qmi_err_num = QMI_ERR_INVALID_ID_V01;
    return false;
  }

  srand((int)time(0));
  *handle = rand()%10000;

  if ( port_trigger_entry.trigger_protocol > 0 &&
       port_trigger_entry.forward_protocol > 0 &&
       port_trigger_entry.trigger_start_port > 0 &&
       port_trigger_entry.trigger_end_port > 0 &&
       port_trigger_entry.forward_start_port > 0 &&
       port_trigger_entry.forward_end_port > 0 &&
       port_trigger_entry.timer > 0)
  {
    snprintf(command, QCMAP_MAX_COMMAND_LEN, "%s %s %d %d %d %d %d %d %d %d %d",
                      NAT_ALG_VPN_CONFIG_FILE, "ADD_PORT_TRIGGER",
                      port_trigger_entry.trigger_start_port,
                      port_trigger_entry.trigger_end_port,
                      port_trigger_entry.forward_start_port,
                      port_trigger_entry.forward_end_port,
                      port_trigger_entry.trigger_protocol,
                      port_trigger_entry.forward_protocol,
                      port_trigger_entry.timer,
                      *handle,
                      wan_profile_handle);

    ds_system_call(command, strlen(command));
  }
  return true;
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
boolean QCMAP_LAN_Client::DeletePortTriggerEntry
(
  uint32_t wan_profile_handle,
  int      handle,
  qmi_error_type_v01                     *qmi_err_num
)
{
  char command[QCMAP_MAX_COMMAND_LEN] = {0};

  if (wan_profile_handle < 1)
  {
    LOG_MSG_ERROR("Invalid wan profile handle passed ", 0,0,0);
    *qmi_err_num = QMI_ERR_INVALID_ID_V01;
    return false;
  }
  snprintf(command, QCMAP_MAX_COMMAND_LEN, "%s %s %d %d", NAT_ALG_VPN_CONFIG_FILE, "DELETE_PORT_TRIGGER",
                                                          handle, wan_profile_handle);
  ds_system_call(command, strlen(command));
  return true;
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
boolean QCMAP_LAN_Client::GetPortTriggerEntry
(
  qcmap_msgr_port_trigger_conf_t         *port_trigger,
  int                                    handle,
  qmi_error_type_v01                     *qmi_err_num
)
{
  char command[QCMAP_MAX_COMMAND_LEN] = {0};
  char result[QCMAP_MAX_SCAN_SIZE] = {0};
  int err_val = 0;

  snprintf(command, QCMAP_MAX_COMMAND_LEN, "%s %s", UCI_GET_COMMAND, UCI_QUERY_NO_OF_PORT_TRIGGER_ENTRIES);

  if (!ExecuteSystemCmd((const char*)command, result, sizeof(result),&err_val))
  {
    if (err_val == EINVAL)
    {
      *qmi_err_num = QMI_ERR_NONE_V01;
      LOG_MSG_ERROR("GetPortTriggerEntry is not configured errno=%d config not found", err_val, 0, 0);
    }
    LOG_MSG_ERROR("Failed to execute command : %s",command,0,0);
    return false;
  }

  int j = 0;
  int no_of_port_trigger_info=atoi(result);
  port_trigger->num_of_entries = 0;
  LOG_MSG_ERROR("GetPortTriggerEntry for handle: %d",handle,0,0);

  for (int i=0; i<no_of_port_trigger_info; i++)
  {
    snprintf(command, QCMAP_MAX_COMMAND_LEN, "%s qcmap_lan.@port_trigger_info[%d].handle", UCI_GET_COMMAND, i);
    if (!ExecuteSystemCmd((const char*)command, result, sizeof(result)))
    {
      LOG_MSG_ERROR("Failed to execute command : %s",command,0,0);
      return false;
    }

    int handle_stored=atoi(result);

    memset(command, 0, QCMAP_MAX_COMMAND_LEN);

    if ( handle == 0 || (handle != 0 && handle == handle_stored) )
    {
      port_trigger->port_trigger_entry[j].handle = atoi(result);

      memset(command,0,QCMAP_MAX_COMMAND_LEN);
      memset(result,0,QCMAP_MAX_SCAN_SIZE);

      snprintf(command, QCMAP_MAX_COMMAND_LEN, "%s qcmap_lan.@port_trigger_info[%d].trigger_start_port", UCI_GET_COMMAND, i);
      if (!ExecuteSystemCmd((const char*)command, result, sizeof(result)))
      {
        LOG_MSG_ERROR("Failed to execute command : %s",command,0,0);
        return false;
      }

      port_trigger->port_trigger_entry[j].trigger_start_port = atoi(result);

      memset(command, 0, QCMAP_MAX_COMMAND_LEN);
      memset(result, 0, QCMAP_MAX_SCAN_SIZE);

      snprintf(command, QCMAP_MAX_COMMAND_LEN, "%s qcmap_lan.@port_trigger_info[%d].trigger_end_port", UCI_GET_COMMAND, i);
      if (!ExecuteSystemCmd((const char*)command, result, sizeof(result)))
      {
        LOG_MSG_ERROR("Failed to execute command : %s",command,0,0);
        return false;
      }

      port_trigger->port_trigger_entry[j].trigger_end_port = atoi(result);

      memset(command, 0, QCMAP_MAX_COMMAND_LEN);
      memset(result, 0, QCMAP_MAX_SCAN_SIZE);

      snprintf(command, QCMAP_MAX_COMMAND_LEN, "%s qcmap_lan.@port_trigger_info[%d].trigger_protocol", UCI_GET_COMMAND, i);
      if (!ExecuteSystemCmd((const char*)command, result, sizeof(result)))
      {
        LOG_MSG_ERROR("Failed to execute command : %s",command,0,0);
        return false;
      }

      if ( strncmp(result, TCP_PROTO, strlen(TCP_PROTO)) == 0 )
      {
        port_trigger->port_trigger_entry[j].trigger_protocol = PS_IPPROTO_TCP;
      }
      else if ( strncmp(result, UDP_PROTO, strlen(UDP_PROTO)) == 0 )
      {
        port_trigger->port_trigger_entry[j].trigger_protocol = PS_IPPROTO_UDP;
      }

      memset(command, 0, QCMAP_MAX_COMMAND_LEN);
      memset(result, 0, QCMAP_MAX_SCAN_SIZE);

      snprintf(command, QCMAP_MAX_COMMAND_LEN, "%s qcmap_lan.@port_trigger_info[%d].forward_start_port", UCI_GET_COMMAND, i);
      if (!ExecuteSystemCmd((const char*)command, result, sizeof(result)))
      {
        LOG_MSG_ERROR("Failed to execute command : %s",command,0,0);
        return false;
      }

      port_trigger->port_trigger_entry[j].forward_start_port = atoi(result);

      memset(command, 0, QCMAP_MAX_COMMAND_LEN);
      memset(result, 0, QCMAP_MAX_SCAN_SIZE);

      snprintf(command, QCMAP_MAX_COMMAND_LEN, "%s qcmap_lan.@port_trigger_info[%d].forward_end_port", UCI_GET_COMMAND, i);
      if (!ExecuteSystemCmd((const char*)command, result, sizeof(result)))
      {
        LOG_MSG_ERROR("Failed to execute command : %s",command,0,0);
        return false;
      }

      port_trigger->port_trigger_entry[j].forward_end_port = atoi(result);

      memset(command, 0, QCMAP_MAX_COMMAND_LEN);
      memset(result, 0, QCMAP_MAX_SCAN_SIZE);

      snprintf(command, QCMAP_MAX_COMMAND_LEN, "%s qcmap_lan.@port_trigger_info[%d].forward_protocol", UCI_GET_COMMAND, i);
      if (!ExecuteSystemCmd((const char*)command, result, sizeof(result)))
      {
        LOG_MSG_ERROR("Failed to execute command : %s",command,0,0);
        return false;
      }

      if ( strncmp(result, TCP_PROTO, strlen(TCP_PROTO)) == 0 )
      {
        port_trigger->port_trigger_entry[j].forward_protocol = PS_IPPROTO_TCP;
      }
      else if ( strncmp(result, UDP_PROTO, strlen(UDP_PROTO)) == 0 )
      {
        port_trigger->port_trigger_entry[j].forward_protocol = PS_IPPROTO_UDP;
      }

      memset(command, 0, QCMAP_MAX_COMMAND_LEN);
      memset(result, 0, QCMAP_MAX_SCAN_SIZE);

      snprintf(command, QCMAP_MAX_COMMAND_LEN, "%s qcmap_lan.@port_trigger_info[%d].timer", UCI_GET_COMMAND, i);
      if (!ExecuteSystemCmd((const char*)command, result, sizeof(result)))
      {
        LOG_MSG_ERROR("Failed to execute command : %s",command,0,0);
        return false;
      }

      port_trigger->port_trigger_entry[j].timer = atoi(result);
      port_trigger->num_of_entries++;
      j++;
    }
  }
  return true;
}
#endif


/*===========================================================================
  FUNCTION SetNatTimeoutOnApps
==========================================================================*/
/*!
@brief
  Will set the NAT timeout value for the identified nat type.

@parameters
  qcmap_nat_timeout_enum          timeout_type
  uint32_t                        timeout_value

@return
  true  - on success
  flase - on failure

@note
- Dependencies
- None

- Side Effects
- None
*/
/*=========================================================================*/
boolean QCMAP_LAN_Client::SetNatTimeoutOnApps
(
  qcmap_nat_timeout_enum          timeout_type,
  uint32_t                        timeout_value,
  qmi_error_type_v01 *qmi_err_num
)
{
  char command[MAX_COMMAND_STR_LEN]={0};
  char timeout_value_string[MAX_COMMAND_STR_LEN]={0};
  int ret = QCMAP_CM_ERROR;
  snprintf(timeout_value_string, MAX_COMMAND_STR_LEN, "%d", timeout_value);
  ret = QCMAP_LAN_Client::CompareKernelVer(KERNEL_VERSION_4_9);
  if (ret == false)
  {
    /*If Kernel Version is below 4.9*/
    switch ( timeout_type )
    {
      case QCMAP_NAT_TIMEOUT_GENERIC:
        snprintf( command, MAX_COMMAND_STR_LEN,
            "echo %d > /proc/sys/net/ipv4/netfilter/ip_conntrack_generic_timeout",
             timeout_value);
        if (!QCMAP_LAN_Client::UciSetUtility(owrt_filename[0], "nat_timeout_values", 0,
            "nat_timeout_generic",timeout_value_string) )
        {
          *qmi_err_num = QMI_ERR_INTERNAL_V01;
          return false;
        }
        break;
      case QCMAP_NAT_TIMEOUT_ICMP:
        snprintf( command, MAX_COMMAND_STR_LEN,
             "echo %d > /proc/sys/net/ipv4/netfilter/ip_conntrack_icmp_timeout", timeout_value);
        if (!QCMAP_LAN_Client::UciSetUtility(owrt_filename[0], "nat_timeout_values", 0,
            "nat_timeout_icmp",timeout_value_string) )
        {
          *qmi_err_num = QMI_ERR_INTERNAL_V01;
          return false;
        }
        break;
      case QCMAP_NAT_TIMEOUT_TCP_ESTABLISHED:
        snprintf( command, MAX_COMMAND_STR_LEN,
             "echo %d > /proc/sys/net/ipv4/netfilter/ip_conntrack_tcp_timeout_established", timeout_value);
        if (!QCMAP_LAN_Client::UciSetUtility(owrt_filename[0], "nat_timeout_values", 0,
            "nat_timeout_tcp_established",timeout_value_string) )
        {
          *qmi_err_num = QMI_ERR_INTERNAL_V01;
          return false;
        }
        break;
      case QCMAP_NAT_TIMEOUT_UDP:
        snprintf( command, MAX_COMMAND_STR_LEN,
             "echo %d > /proc/sys/net/ipv4/netfilter/ip_conntrack_udp_timeout", timeout_value);
        ds_system_call(command, strlen(command));
        snprintf( command, MAX_COMMAND_STR_LEN,
             "echo %d > /proc/sys/net/ipv4/netfilter/ip_conntrack_udp_timeout_stream", timeout_value);
        if (!QCMAP_LAN_Client::UciSetUtility(owrt_filename[0], "nat_timeout_values", 0,
            "nat_timeout_udp",timeout_value_string) )
        {
          *qmi_err_num = QMI_ERR_INTERNAL_V01;
          return false;
        }
        break;
      default:
        LOG_MSG_INFO1("Timeout Type:%d not supported.\n", timeout_type,0,0);
        *qmi_err_num = QMI_ERR_NOT_SUPPORTED_V01;
        return false;
    }
  }
  else
  {
    /*Default and if Kernel Version is >= 4.9*/
    switch ( timeout_type )
    {
      case QCMAP_NAT_TIMEOUT_GENERIC:
        snprintf( command, MAX_COMMAND_STR_LEN,
            "echo %d > /proc/sys/net/netfilter/nf_conntrack_generic_timeout", timeout_value);
        if (!QCMAP_LAN_Client::UciSetUtility(owrt_filename[0], "nat_timeout_values", 0,
            "nat_timeout_generic",timeout_value_string) )
        {
          *qmi_err_num = QMI_ERR_INTERNAL_V01;
          return false;
        }
        break;
      case QCMAP_NAT_TIMEOUT_ICMP:
        snprintf( command, MAX_COMMAND_STR_LEN,
            "echo %d > /proc/sys/net/netfilter/nf_conntrack_icmp_timeout", timeout_value);
        if (!QCMAP_LAN_Client::UciSetUtility(owrt_filename[0], "nat_timeout_values", 0,
            "nat_timeout_icmp",timeout_value_string) )
        {
          *qmi_err_num = QMI_ERR_INTERNAL_V01;
          return false;
        }
        break;

      case QCMAP_NAT_TIMEOUT_ICMPV6:
        snprintf( command, MAX_COMMAND_STR_LEN,
            "echo %d > /proc/sys/net/netfilter/nf_conntrack_icmpv6_timeout", timeout_value);
        if (!QCMAP_LAN_Client::UciSetUtility(owrt_filename[0], "nat_timeout_values", 0,
            "nat_timeout_icmpv6",timeout_value_string) )
        {
          *qmi_err_num = QMI_ERR_INTERNAL_V01;
          return false;
        }
        break;
      case QCMAP_NAT_TIMEOUT_TCP_ESTABLISHED:
        snprintf( command, MAX_COMMAND_STR_LEN,
            "echo %d > /proc/sys/net/netfilter/nf_conntrack_tcp_timeout_established", timeout_value);
        if (!QCMAP_LAN_Client::UciSetUtility(owrt_filename[0], "nat_timeout_values", 0,
            "nat_timeout_tcp_established",timeout_value_string) )
        {
          *qmi_err_num = QMI_ERR_INTERNAL_V01;
          return false;
        }
        break;
      case QCMAP_NAT_TIMEOUT_UDP:
        snprintf( command, MAX_COMMAND_STR_LEN,
            "echo %d > /proc/sys/net/netfilter/nf_conntrack_udp_timeout", timeout_value);
        ds_system_call(command, strlen(command));
        snprintf( command, MAX_COMMAND_STR_LEN,
            "echo %d > /proc/sys/net/netfilter/nf_conntrack_udp_timeout_stream", timeout_value);
        if (!QCMAP_LAN_Client::UciSetUtility(owrt_filename[0], "nat_timeout_values", 0,
            "nat_timeout_udp",timeout_value_string) )
        {
          *qmi_err_num = QMI_ERR_INTERNAL_V01;
          return false;
        }
        break;
      default:
        LOG_MSG_INFO1("Timeout Type:%d not supported.\n", timeout_type,0,0);
        *qmi_err_num = QMI_ERR_NOT_SUPPORTED_V01;
        return false;
    }
  }
  ds_system_call(command, strlen(command));
  ExecuteUCICommit();
  return true;
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
boolean QCMAP_LAN_Client::SetNatTimeout
(
  qcmap_nat_timeout_enum          timeout_type,
  uint32                          timeout_value,
  qmi_error_type_v01 *qmi_err_num
)
{
  int profile_handle;

  qmi_error_type_v01 error_num_temp =QMI_ERR_NONE_V01;

  profile_handle = GetDefaultProfilefromUCI();
  IPv4_DISABLE_NAT_EXCLUSIVITY_CHECK(profile_handle, qmi_err_num, false);

  if (timeout_value < QCMAP_NAT_ENTRY_MIN_TIMEOUT || timeout_value > QCMAP_NAT_ENTRY_MAX_TIMEOUT)
  {
    LOG_MSG_ERROR("Timeout value should be greater than: %d and lesser than %d Got: %d.\n",
                  QCMAP_NAT_ENTRY_MIN_TIMEOUT,QCMAP_NAT_ENTRY_MAX_TIMEOUT, timeout_value);
    *qmi_err_num = QMI_ERR_INVALID_ARG_V01;
     return false;
  }

  if(QCMAP_LAN_Client::SetNatTimeoutOnApps(timeout_type, timeout_value, &error_num_temp))
  {
      LOG_MSG_INFO1("Set NAT Timeout done successfully", 0, 0,0);
      return true;
  }
  else
  {
    *qmi_err_num = error_num_temp;
  }

  return false;

}

/*===========================================================================
  FUNCTION GetNatTimeoutOnApps
==========================================================================*/
/*!
@brief
  Get the NAT timeout value for the requested nat type.

@parameters
  qcmap_nat_timeout_enum           timeout_type
  uint32_t                        *timeout_value

@return
  true  - on success
  flase - on failure

@note
- Dependencies
- None

- Side Effects
- None
*/
/*=========================================================================*/
boolean QCMAP_LAN_Client::GetNatTimeoutOnApps
(
  qcmap_nat_timeout_enum            timeout_type,
  uint32_t                         *timeout_value,
  qmi_error_type_v01 *qmi_err_num
)
{
  char command[MAX_COMMAND_STR_LEN]={0};
  char result[QCMAP_MAX_SCAN_SIZE]={0};
  bool ret;

  if ( timeout_value == NULL )
  {
    LOG_MSG_ERROR("Time out is NULL", 0, 0 ,0);
    *qmi_err_num = QMI_ERR_INVALID_ARG_V01;
    return false;
  }
  ret = QCMAP_LAN_Client::CompareKernelVer(KERNEL_VERSION_4_9);
  if (ret == false)
  {
    /*If Kernel Version is below 4.9*/
    switch ( timeout_type )
    {
      case QCMAP_NAT_TIMEOUT_GENERIC:
        snprintf( command, MAX_COMMAND_STR_LEN,"cat /proc/sys/net/ipv4/netfilter/ip_conntrack_generic_timeout");
        break;
      case QCMAP_NAT_TIMEOUT_ICMP:
        snprintf( command, MAX_COMMAND_STR_LEN,"cat /proc/sys/net/ipv4/netfilter/ip_conntrack_icmp_timeout");
        break;
      case QCMAP_NAT_TIMEOUT_TCP_ESTABLISHED:
        snprintf( command, MAX_COMMAND_STR_LEN,"cat /proc/sys/net/ipv4/netfilter/ip_conntrack_tcp_timeout_established");
        break;
      case QCMAP_NAT_TIMEOUT_UDP:
        snprintf( command, MAX_COMMAND_STR_LEN,"cat /proc/sys/net/ipv4/netfilter/ip_conntrack_udp_timeout");
        break;
      default:
        LOG_MSG_INFO1("Timeout Type:%d not supported.\n", timeout_type,0,0);
        *qmi_err_num = QMI_ERR_OP_DEVICE_UNSUPPORTED_V01;
        return false;
    }
  }
  else
  {
    switch ( timeout_type )
    {
    /*Default and if Kernel Version is >= 4.9*/
       case QCMAP_NAT_TIMEOUT_GENERIC:
         snprintf( command, MAX_COMMAND_STR_LEN,"cat /proc/sys/net/netfilter/nf_conntrack_generic_timeout");
         break;
       case QCMAP_NAT_TIMEOUT_ICMP:
         snprintf( command, MAX_COMMAND_STR_LEN,"cat /proc/sys/net/netfilter/nf_conntrack_icmp_timeout");
         break;
       case QCMAP_NAT_TIMEOUT_ICMPV6:
         snprintf( command, MAX_COMMAND_STR_LEN,"cat /proc/sys/net/netfilter/nf_conntrack_icmpv6_timeout");
         break;
       case QCMAP_NAT_TIMEOUT_TCP_ESTABLISHED:
         snprintf( command, MAX_COMMAND_STR_LEN,"cat /proc/sys/net/netfilter/nf_conntrack_tcp_timeout_established");
         break;
       case QCMAP_NAT_TIMEOUT_UDP:
         snprintf( command, MAX_COMMAND_STR_LEN,"cat /proc/sys/net/netfilter/nf_conntrack_udp_timeout");
         break;
       default:
         LOG_MSG_INFO1("Timeout Type:%d not supported.\n", timeout_type,0,0);
         *qmi_err_num = QMI_ERR_NOT_SUPPORTED_V01;
         return false;
    }
  }
  if(!ExecuteSystemCmd((const char *)command,result,sizeof(result)))
  {
    LOG_MSG_ERROR("Failed to execute command : %s",command,0,0);
    *qmi_err_num = QMI_ERR_INTERNAL_V01;
    return false;
  }
  *timeout_value=atoi(result);

  LOG_MSG_INFO1("Timeout Type: %d Timeout Value: %d.\n", timeout_type, *timeout_value,0);

  return true;
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
boolean QCMAP_LAN_Client::GetNatTimeout
(
  qcmap_nat_timeout_enum            timeout_type,
  uint32                           *timeout_value,
  qmi_error_type_v01 *qmi_err_num
)
{
  qmi_error_type_v01 error_num_temp = QMI_ERR_NONE_V01;
  if (!QCMAP_LAN_Client::GetNatTimeoutOnApps(timeout_type, timeout_value, &error_num_temp))
  {
    *qmi_err_num = error_num_temp;
    return false;
  }
  return true;
}

/*===========================================================================
  FUNCTION SetupTinyProxy
==========================================================================*/
/*!
@brief
  Setup tiny proxy, run iptables cmd

@return
  true  - success
  false - failure

@note
- Dependencies
- None

- Side Effects
- None
*/
/*=========================================================================*/
boolean QCMAP_LAN_Client::SetupTinyProxy(void)
{
  QCMAP_LAN_CLIENT_RUN_COMMANDS("%s %s", TINYPROXY_CONFIG_FILE, SETUP_TINYPROXY);

  LOG_MSG_INFO1("starting tinyproxy service",0,0,0);

  return true;
}

/*===========================================================================
  FUNCTION StopTinyProxy
==========================================================================*/
/*!
@brief
  Stop tiny proxy, delete iptables cmd

@return
  true  - success
  false - failure

@note
- Dependencies
- None

- Side Effects
- None
*/
/*=========================================================================*/
boolean QCMAP_LAN_Client::StopTinyProxy(void)
{
  QCMAP_LAN_CLIENT_RUN_COMMANDS("%s %s", TINYPROXY_CONFIG_FILE, STOP_TINYPROXY);

  LOG_MSG_INFO1("stop tinyproxy service",0,0,0);

  return true;
}

/*===========================================================================
  FUNCTION EnableTinyProxy
==========================================================================*/
/*!
@brief
  Enable tiny proxy

@return
  true  - success
  false - failure

@note
- Dependencies
- None

- Side Effects
- None
*/
/*=========================================================================*/
boolean QCMAP_LAN_Client::EnableTinyProxy
(
    qmi_error_type_v01 *qmi_err_num
)
{
  /*stoping tinyproxy first*/
  StopTinyProxy();
  /*end of stoping tinyproxy*/

  QCMAP_LAN_CLIENT_RUN_COMMANDS("%s %s", TINYPROXY_CONFIG_FILE, ENABLE_TINYPROXY);

  return true;
}

/*===========================================================================
  FUNCTION DisableTinyProxy
==========================================================================*/
/*!
@brief
  Disable tiny proxy

@return
  false - failure
  true  - success

@note
- Dependencies
- None

- Side Effects
- None
*/
/*=========================================================================*/
boolean QCMAP_LAN_Client::DisableTinyProxy
(
    qmi_error_type_v01 *qmi_err_num
)
{
  QCMAP_LAN_CLIENT_RUN_COMMANDS("%s %s", TINYPROXY_CONFIG_FILE, DISABLE_TINYPROXY);
  return true;
}

/*===========================================================================
  FUNCTION GetTinyProxyStatus
==========================================================================*/
/*!
@brief
  Get TinyProxy  Status

@return
  true  - success
  false - failure

@note
- Dependencies
- None

- Side Effects
- None
*/
/*=========================================================================*/
boolean QCMAP_LAN_Client::GetTinyProxyStatus
(
  qcmap_msgr_tiny_proxy_mode_enum_v01 *tinyproxy_status,
  qmi_error_type_v01 *qmi_err_num
)
{
  char command[QCMAP_MAX_COMMAND_LEN] = {0};
  char result[QCMAP_MAX_SCAN_SIZE] = {0};
  int status=0;

  if(tinyproxy_status == NULL)
  {
    LOG_MSG_ERROR("NULL pointer passed ",0,0,0);
    return false;
  }

  snprintf(command, QCMAP_MAX_COMMAND_LEN, "%s %s", UCI_GET_COMMAND, UCI_QUERY_TINYPROXY_STATUS);

  if (!ExecuteSystemCmd((const char*)command, result, sizeof(result)))
  {
    /*If uci entry is empty, treat as tinyproxy not set*/
    LOG_MSG_ERROR("Failed to execute command : %s",command,0,0);
    *tinyproxy_status = QCMAP_MSGR_TINY_PROXY_MODE_ENUM_MIN_ENUM_VAL_V01;
    return true;
  }

  /*Get tinyproxy status from script: QCMAP_TINYPROXY_DISABLED=2; QCMAP_TINYPROXY_ENABLED=1*/
  status=atoi(result);
  if (status == 1)
  {
    *tinyproxy_status = QCMAP_MSGR_TINY_PROXY_MODE_UP_V01;
  }
  else if(status == 2)
  {
    *tinyproxy_status = QCMAP_MSGR_TINY_PROXY_MODE_DOWN_V01;
  }

  LOG_MSG_INFO1("Successfuly get tinyproxy status : %d", *tinyproxy_status,0,0);
  return true;
}

/*===========================================================================
  FUNCTION AddStaticNatEntry()
===========================================================================*/
/**
  @ingroup qcmap_add_snat_entry
  Add a snat entry to UCI DB upon user request.
  @datatypes
  qcmap_snat_config_t
  @param[in]      qcmap_snat_config_t    snat_config\n
  @return
  TRUE -- Success \n
  FALSE -- Failure \n
*/

/*=========================================================================*/
boolean QCMAP_LAN_Client::AddStaticNatEntry
(
  qcmap_snat_config_t snat_config,
  uint32_t profile_handle,
  qmi_error_type_v01 *qmi_err_num
)
{
  char command[MAX_COMMAND_STR_LEN] = {0};
  qmi_error_type_v01 error_num_temp = QMI_ERR_NONE_V01;

  QCMAP_LOG_FUNC_ENTRY();

  IPv4_DISABLE_NAT_EXCLUSIVITY_CHECK(profile_handle, qmi_err_num, false);

  /* check if firewall entry is not exceeding the max limit 128*/
  if( !checkSNATEntryLimit( &error_num_temp ) )
  {
    if( error_num_temp == QMI_ERR_INTERNAL_V01 )
    {
      LOG_MSG_ERROR("UciGetUtility Failed to fetch info.", 0,0,0);
    }
    else
    {
      LOG_MSG_ERROR("Failed to addStaticNatEntry, max entry limit 50 exceeds ", 0,0,0);
    }
    return false;
  }

  switch(snat_config.protocol)
  {
    case ICMP:
    {
       snprintf(command, MAX_COMMAND_STR_LEN,"%s add_port_fwd %s %s %d",
                         NAT_ALG_VPN_CONFIG_FILE,ICMP_PROTO,snat_config.private_ip_addr, profile_handle);
       ds_system_call(command, strlen(command));
       break;
    }
    case TCP:
    {
       snprintf(command, MAX_COMMAND_STR_LEN,"%s add_port_fwd %s %s %d %d %d",NAT_ALG_VPN_CONFIG_FILE,
                TCP_PROTO,snat_config.private_ip_addr, snat_config.private_port,snat_config.global_port, profile_handle);
       ds_system_call(command, strlen(command));
       break;
    }
    case UDP:
    {
       snprintf(command, MAX_COMMAND_STR_LEN,"%s add_port_fwd %s %s %d %d %d",NAT_ALG_VPN_CONFIG_FILE,
                UDP_PROTO,snat_config.private_ip_addr, snat_config.private_port,snat_config.global_port, profile_handle);
       ds_system_call(command, strlen(command));
       break;
    }
    case TCP_UDP:
    {
       snprintf(command, MAX_COMMAND_STR_LEN,"%s add_port_fwd %s %s %d %d %d",NAT_ALG_VPN_CONFIG_FILE,
                TCP_UDP_PROTO,snat_config.private_ip_addr, snat_config.private_port,snat_config.global_port, profile_handle);
       ds_system_call(command, strlen(command));
       break;
    }
    default:
    {
      LOG_MSG_ERROR("Invalid proto option", 0, 0, 0);
      *qmi_err_num = QMI_ERR_INVALID_ARG_V01;
      return false;
    }
  }
  return true;

} /*End AddStaticNatEntry() */

/*===========================================================================
  FUNCTION DeleteStaticNatEntry()
===========================================================================*/
/**
  @ingroup qcmap_delete_snat_entry
  Deletes a snat entry to UCI DB upon user request.
  @datatypes
  qcmap_snat_config_t
  @param[in]      qcmap_snat_config_t    snat_config\n
  @return
  TRUE -- Success \n
  FALSE -- Failure \n
*/

/*=========================================================================*/
boolean QCMAP_LAN_Client::DeleteStaticNatEntry
(
  qcmap_snat_config_t snat_config,
  uint32_t profile_handle,
  qmi_error_type_v01 *qmi_err_num
)
{
  char command[MAX_COMMAND_STR_LEN] = {0};

  IPv4_DISABLE_NAT_EXCLUSIVITY_CHECK(profile_handle, qmi_err_num, false);

  switch(snat_config.protocol)
  {
    case ICMP:
    {
       snprintf(command, MAX_COMMAND_STR_LEN,"%s del_port_fwd %s %s %d",
                         NAT_ALG_VPN_CONFIG_FILE,ICMP_PROTO,snat_config.private_ip_addr, profile_handle);
       ds_system_call(command, strlen(command));
       break;
    }
    case TCP:
    {
       snprintf(command, MAX_COMMAND_STR_LEN,"%s del_port_fwd %s %s %d %d %d",NAT_ALG_VPN_CONFIG_FILE,
                TCP_PROTO,snat_config.private_ip_addr, snat_config.private_port,snat_config.global_port, profile_handle);
       ds_system_call(command, strlen(command));
       break;
    }
    case UDP:
    {
       snprintf(command, MAX_COMMAND_STR_LEN,"%s del_port_fwd %s %s %d %d %d",NAT_ALG_VPN_CONFIG_FILE,
                UDP_PROTO,snat_config.private_ip_addr, snat_config.private_port,snat_config.global_port, profile_handle);
       ds_system_call(command, strlen(command));
       break;
    }
    case TCP_UDP:
    {
       snprintf(command, MAX_COMMAND_STR_LEN,"%s del_port_fwd %s %s %d %d %d",NAT_ALG_VPN_CONFIG_FILE,
                TCP_UDP_PROTO,snat_config.private_ip_addr, snat_config.private_port,snat_config.global_port, profile_handle);
       ds_system_call(command, strlen(command));
       break;
    }
    default:
    {
      LOG_MSG_ERROR("Invalid proto option", 0, 0, 0);
      *qmi_err_num = QMI_ERR_INVALID_ARG_V01;
      return false;
    }
  }
  return true;

} /*End DeleteStaticNatEntry() */

/*===========================================================================
  FUNCTION GetStaticNatConfig()
===========================================================================*/
/**
  @ingroup qcmap_get_snat_entry
  Get snat entry from  UCI DB upon user request.
  @datatypes
  qcmap_snat_config_t
  uint16_t
  @param[in/out]      qcmap_snat_config_t*    snat_config\n
  @param[in/out]      uint16_t    num_entries\n
  @return
  TRUE -- Success \n
  FALSE -- Failure \n
*/

/*=========================================================================*/
boolean QCMAP_LAN_Client::GetStaticNatConfig
(
  qcmap_snat_config_t *snat_config,
  uint16_t *num_entries,
  uint32_t profile_handle,
  qmi_error_type_v01 *qmi_err_num
)
{
  char command[MAX_COMMAND_STR_LEN] = {0};
  char result[QCMAP_MAX_SCAN_SIZE] = {0};
  char snat_entry_name[MAX_COMMAND_STR_LEN] = {0};
  int total_no_of_redirects = 0;
  int idx = 0;
  int  profile_index;

  if(snat_config == NULL || num_entries == NULL)
  {
    LOG_MSG_ERROR("snat_config or num_entries is NULL",0,0,0);
    *qmi_err_num = QMI_ERR_INVALID_ARG_V01;
    return false;
  }

  profile_index = GetProfileIndex(profile_handle);
  if (profile_index == QCMAP_LAN_INVALID)
  {
    /* indicates profile_idx not found */
    LOG_MSG_ERROR(" Profile index for profile: %d not found in qcmap_lan. Exiting!",
                   profile_handle, 0, 0);
    return false;
  }

  /**Get number of SNAT entry in the PDN*/
  snprintf(command, MAX_COMMAND_STR_LEN,"/etc/data/uci_ex.sh get qcmap_lan.@profile[%d].no_of_snat_rules",profile_index);
  if (!ExecuteSystemCmd((const char*)command,result,sizeof(result)))
  {
    LOG_MSG_ERROR("Failed to execute command : %s",command,0,0);
    return false;
  }
  *num_entries = atoi(result);
  if(*num_entries == 0)
  {
    return true;
  }

  /**Get total number of SNAT entry*/
  snprintf(command, MAX_COMMAND_STR_LEN,"%s %s",UCI_GET_COMMAND, UCI_QUERY_NO_OF_REDIRECTS_COMMAND);
  if (!ExecuteSystemCmd((const char*)command,result,sizeof(result)))
  {
    LOG_MSG_ERROR("Failed to execute command : %s",command,0,0);
    *qmi_err_num = QMI_ERR_INTERNAL_V01;
    return false;
  }
  total_no_of_redirects=atoi(result);

  snprintf(snat_entry_name, MAX_COMMAND_STR_LEN, "StaticNAT-%d", profile_handle);

  for(int i=0;i<total_no_of_redirects;i++)
  {
    memset(command,0,MAX_COMMAND_STR_LEN);
    memset(result,0,QCMAP_MAX_SCAN_SIZE);
    snprintf(command, MAX_COMMAND_STR_LEN, "%s firewall.@redirect[%d].name",UCI_GET_COMMAND,i);
    if(!ExecuteSystemCmd((const char*)command,result,sizeof(result)))
    {
      LOG_MSG_ERROR("Failed to execute command : %s",command,0,0);
      return false;
    }

    if(strncmp(result, snat_entry_name, strlen(snat_entry_name))==0)
    {
      memset(result,0,QCMAP_MAX_SCAN_SIZE);
      /**Get the proto*/
      snprintf(command, MAX_COMMAND_STR_LEN, "%s firewall.@redirect[%d].proto",UCI_GET_COMMAND,i);
      if(!ExecuteSystemCmd((const char*)command,result,sizeof(result)))
      {
        LOG_MSG_ERROR("Failed to execute command : %s",command,0,0);
        *qmi_err_num = QMI_ERR_INTERNAL_V01;
        return false;
      }

      if(strncmp(result, ICMP_PROTO, strlen(ICMP_PROTO))==0)
      {
        snat_config[idx].protocol=ICMP;
      }
      else if(strncmp(result, TCP_UDP_PROTO, strlen(TCP_UDP_PROTO))==0)
      {
        snat_config[idx].protocol=TCP_UDP;
      }
      else if(strncmp(result, UDP_PROTO, strlen(UDP_PROTO))==0)
      {
        snat_config[idx].protocol=UDP;
      }
      else if(strncmp(result, TCP_PROTO, strlen(TCP_PROTO))==0)
      {
        snat_config[idx].protocol=TCP;
      }
      memset(command,0,MAX_COMMAND_STR_LEN);
      memset(result,0,QCMAP_MAX_SCAN_SIZE);
      snprintf(command, MAX_COMMAND_STR_LEN, "%s firewall.@redirect[%d].dest_ip",UCI_GET_COMMAND,i);
      if(!ExecuteSystemCmd((const char*)command,result,sizeof(result)))
      {
        LOG_MSG_ERROR("Failed to execute command : %s",command,0,0);
        *qmi_err_num = QMI_ERR_INTERNAL_V01;
        return false;
      }
      strlcpy(snat_config[idx].private_ip_addr, result, QCMAP_IPV4_ADDR_LEN);
      if(snat_config[idx].protocol != ICMP)
      {
        memset(command,0,MAX_COMMAND_STR_LEN);
        memset(result,0,QCMAP_MAX_SCAN_SIZE);
        snprintf(command, MAX_COMMAND_STR_LEN, "%s firewall.@redirect[%d].dest_port",UCI_GET_COMMAND,i);
        if(!ExecuteSystemCmd((const char*)command,result,sizeof(result)))
        {
          LOG_MSG_ERROR("Failed to execute command : %s",command,0,0);
          *qmi_err_num = QMI_ERR_INTERNAL_V01;
          return false;
        }
        snat_config[idx].private_port=atoi(result);
        memset(command,0,MAX_COMMAND_STR_LEN);
        memset(result,0,QCMAP_MAX_SCAN_SIZE);
        snprintf(command, MAX_COMMAND_STR_LEN, "%s firewall.@redirect[%d].src_dport",UCI_GET_COMMAND,i);
        if(!ExecuteSystemCmd((const char*)command,result,sizeof(result)))
        {
          LOG_MSG_ERROR("Failed to execute command : %s",command,0,0);
          *qmi_err_num = QMI_ERR_INTERNAL_V01;
          return false;
        }
        snat_config[idx].global_port=atoi(result);

      }
      idx++;
    }
  }

  return true;
}

/*===========================================================================
  FUNCTION SetV4NATConfig
  ===========================================================================*/
/*!
  @brief
  Set IPv4 NAT Configuration.This function is dependent on the network side configurations
  to work as expected. After enabling IPv4 NAT disable configuration,  data  packets  with
  source address as LAN IP will go out to network from UE.By default, NAT will be enabled.

  @datatypes
  unit32_t
  boolean
  qmi_error_type_v01

  @param[in]  wan_profile_handle         wan_profile handle
  @param[in]  ipv4_nat_disable           Enable/Disable IPV4 Disable NAT Configuration
  @param[out] qmi_err_num                Error code returned by the server

  @return
  true  - on Success
  false - on Failure

 */
/*=========================================================================*/
boolean QCMAP_LAN_Client::SetV4NATConfig
(
  uint32_t            wan_profile_handle,
  boolean             &ipv4_nat_disable,
  qmi_error_type_v01  *qmi_err_num
)
{
  bool ret_val = false;
  int profile_index;
  int default_profile;
  char result[QCMAP_MAX_SCAN_SIZE] = {0};

  QCMAP_LOG_FUNC_ENTRY();

  /* Currently feature is supported on default PDN only
  So,here we are using only default_profile to fetch ipv4_nat_disable option
  If this feature need to be extended to on-demand PDN's replace default_profile
  with wan_profile_handle param that is passed to the function */

  default_profile = GetDefaultProfilefromUCI();
  if (default_profile == QCMAP_LAN_INVALID)
  {
    /* indicates profile_idx not found */
    LOG_MSG_ERROR(" Default profile not found in qcmap_lan. Exiting!", 0, 0, 0);
    *qmi_err_num = QMI_ERR_INTERNAL_V01;
    return false;
  }

  profile_index = GetProfileIndex(default_profile);
  if (profile_index == QCMAP_LAN_INVALID)
  {
    /* indicates profile_idx not found */
    LOG_MSG_ERROR(" Profile index for profile: %d not found in qcmap_lan. Exiting!",
                   default_profile, 0, 0);
    *qmi_err_num = QMI_ERR_INTERNAL_V01;
    return false;
  }

  /* Get current status of NATConfig(ipv4_nat_disable) */
  if (!QCMAP_LAN_Client::UciGetUtility(owrt_filename[0], "profile", 0,
       "ipv4_nat_disable", result, profile_index) )
  {
    LOG_MSG_ERROR("UciGetUtility failed ",0,0,0);
    *qmi_err_num = QMI_ERR_INTERNAL_V01;
    return false;
  }

  /* Check if nat option is already set to current request*/
  if((bool)atoi(result) == ipv4_nat_disable)
  {
    LOG_MSG_ERROR("ipv4_nat_disable is already %d",ipv4_nat_disable,0,0);
    *qmi_err_num = QMI_ERR_NO_EFFECT_V01;
    return true;
  }

  /* Enable or disable ipv4_nat_disable UCI option */
  QCMAP_LAN_CLIENT_RUN_COMMANDS("%s %s %d %d", BACKHAUL_WWAN_CONFIG_FILE,
                   SET_V4_NAT_CONFIG, default_profile, ipv4_nat_disable);
  LOG_MSG_INFO1("ipv4_nat_disable UCI option changed successfully.", 0, 0, 0);

  ret_val=true;

  return ret_val;
}

/*===========================================================================
  FUNCTION GetV4NATConfig
  ===========================================================================*/
/*!
  @brief
  Get Current Status of IPv4 NAT Configuration.


  @datatypes
  unit32_t
  boolean
  qmi_error_type_v01

  @param[in]  wan_profile_handle         wan_profile handle
  @param[out] ipv4_nat_disable           Current Status of IPV4 Disable NAT Configuration
  @param[out] qmi_err_num                Error code returned by the server

  @return
  true  - on Success
  false - on Failure

 */
/*=========================================================================*/
boolean QCMAP_LAN_Client::GetV4NATConfig
(
  uint32_t            wan_profile_handle,
  boolean             &ipv4_nat_disable,
  qmi_error_type_v01  *qmi_err_num
)
{
  bool ret_val = false;
  int profile_index;
  int default_profile;
  char result[QCMAP_MAX_SCAN_SIZE] = {0};

  QCMAP_LOG_FUNC_ENTRY();

  /* Currently feature is supported on default PDN only
  So,here we are using only default_profile to fetch ipv4_nat_disable option
  If this feature need to be extended to on-demand PDN's replace default_profile
  with wan_profile_handle param that is passed to the function */

  default_profile = GetDefaultProfilefromUCI();
  if (default_profile == QCMAP_LAN_INVALID)
  {
    /* indicates profile_idx not found */
    LOG_MSG_ERROR(" Default profile not found in qcmap_lan. Exiting!", 0, 0, 0);
    *qmi_err_num = QMI_ERR_INTERNAL_V01;
    return false;
  }

  profile_index = GetProfileIndex(default_profile);
  if (profile_index == QCMAP_LAN_INVALID)
  {
    /* indicates profile_idx not found */
    LOG_MSG_ERROR(" Profile index for profile: %d not found in qcmap_lan. Exiting!",
                   default_profile, 0, 0);
    *qmi_err_num = QMI_ERR_INTERNAL_V01;
    return false;
  }

  /* Get current status of NATConfig(ipv4_nat_disable) */
  if (!QCMAP_LAN_Client::UciGetUtility(owrt_filename[0], "profile", 0,
       "ipv4_nat_disable", result, profile_index) )
  {
    LOG_MSG_ERROR("UciGetUtility failed ",0,0,0);
    *qmi_err_num = QMI_ERR_INTERNAL_V01;
    return false;
  }
  ipv4_nat_disable=(bool)atoi(result);
  LOG_MSG_INFO1("Get ipv4_nat_disable UCI option successfully.", 0, 0, 0);

  ret_val=true;

  return ret_val;
}

