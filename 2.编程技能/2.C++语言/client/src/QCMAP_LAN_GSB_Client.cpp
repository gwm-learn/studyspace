/*====================================================

FILE:  QCMAP_LAN_GSB_Client.cpp

SERVICES:
QCMAP LAN GSB Client Implementation

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


/*=====================================================================
  FUNCTION SetGSBUnloadFlag
======================================================================*/
/*!
@brief
  - Set GSB Unload flag

@return
  none

@note
  - Dependencies
  - None

  - Side Effects
  - None
*/
/*=========================================================================*/
void SetGSBUnloadFlag()
{
  char command[MAX_COMMAND_STR_LEN];
  snprintf(command, MAX_COMMAND_STR_LEN,
            "data_path_opt -mGSB -o%d", (int)MSG_TYPE_UNLOAD);
  ds_system_call(command,strlen(command));
}

/*=====================================================================
  FUNCTION UnLoadGSB
======================================================================*/
/*!
@brief
  - UnLoad GSB from qcmap client

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
boolean QCMAP_LAN_Client::UnLoadGSB()
{
  char cmd[MAX_COMMAND_STR_LEN];
  int GSBEnableFlag = 0;

  UCI_GET_INT_OPTION(GSBEnableFlag, \
      static_cast<int>(config_file::GSB), "gsb_enable_info", 0, "gsb_enable_flag")
  if (!GSBEnableFlag)
  {
    LOG_MSG_ERROR("GSB is not enabled", 0, 0, 0);
    /* this does not stop the config process, so not an error*/
    return false;
  }

  snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "%s %s", GSB_CONFIG_FILE, "UNLOAD_GSB");
  ds_system_call(cmd, strlen(cmd));

  return true;
}


/*=====================================================================
  FUNCTION EnableGSB
======================================================================*/
/*!
@brief
  - Enable GSB from qcmap client

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
boolean QCMAP_LAN_Client::EnableGSB(qmi_error_type_v01 *qmi_err_num)
{
  /* Enable GSB */
  char cmd[QCMAP_MAX_COMMAND_LEN] = {0};

  snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "%s %s", GSB_CONFIG_FILE, "ENABLE_GSB");
  ds_system_call(cmd, strlen(cmd));

  return true;
}

/*=====================================================================
  FUNCTION DisableGSB
======================================================================*/
/*!
@brief
  - Disable GSB from qcmap client

@return
  true - Success
  false - Failure

@note
  - Dependencies
  - None

  - Side Effects
  - None

*/
/*=====================================================================*/
boolean QCMAP_LAN_Client::DisableGSB(qmi_error_type_v01 *qmi_err_num)
{
  /* Disable GSB */
  char cmd[QCMAP_MAX_COMMAND_LEN] = {0};
  qcmap_gsb_config config[QCMAP_MAX_IF_SUPPORTED];
  uint8_t num_of_entries = 0;
  bool if_stopped[QCMAP_MAX_IF_SUPPORTED];
  int ret = 0;
  int i = 0;
  int GSBEnableFlag = 0;
  memset(if_stopped, 0, QCMAP_MAX_IF_SUPPORTED);

  //if not enabled return 0
  UCI_GET_INT_OPTION(GSBEnableFlag, \
      static_cast<int>(config_file::GSB), "gsb_enable_info", 0, "gsb_enable_flag")
  if (!GSBEnableFlag)
  {
    LOG_MSG_ERROR("GSB not enabled",0,0,0);
    *qmi_err_num = QMI_ERR_INTERNAL_V01;
    return false;
  }

  // if enabled, then read configuration
  if (!(QCMAP_LAN_Client::GetGSBConfig(config, &num_of_entries, qmi_err_num)))
  {
    LOG_MSG_ERROR("Get GSB config failed!!",0,0,0);
    *qmi_err_num = QMI_ERR_INTERNAL_V01;
  }

  //if no configuration entry found then return
  if (!num_of_entries)
  {
    LOG_MSG_INFO1("No entry found, Unloading GSB",0,0,0);
    *qmi_err_num = QMI_ERR_INTERNAL_V01;

    /* unload GSB module */
    QCMAP_LAN_Client::UnLoadGSB();
    usleep(QCMAP_GSB_DELAY_COUNT * 50);

    return true;
  }

  SetGSBUnloadFlag();

  /*Stop all the enabled IF (the one which are configured only)
          If not enabled then move to next step*/
  for (i = 0 ; i < num_of_entries; i++)
  {
    ret = isInterfaceUP(config[i].if_name);
    if (ret > IF_STATUS_DOWN)
    {
      //stop IF
      ChangeIFState(config[i].if_name, IF_STATE_DOWN);
      if_stopped [i]= true;
    }
  }

  /* unload GSB module */
  QCMAP_LAN_Client::UnLoadGSB();
  usleep(QCMAP_GSB_DELAY_COUNT * 50);

  /* restart the interfaces if stopped */
  for (int i = 0 ; i < num_of_entries; i++)
  {
    if (if_stopped[i])
    {
      ChangeIFState(config[i].if_name, IF_STATE_UP);
    }
  }

  return true;
}


/*===========================================================================
FUNCTION SetGSBConfig()
===========================================================================*/
/*!
@brief
  - SetGSBConfig from qcmap client

@return
  true - Success
  false - Failure

@param[in]
  qcmap_gsb_config gsb_config

@note
  - Dependencies
  - None

  - Side Effects
  - None
*/
/*=========================================================================*/

boolean QCMAP_LAN_Client::SetGSBConfig
(
  qcmap_gsb_config *gsb_config,
  qmi_error_type_v01 *qmi_err_num
)
{
  char cmd[QCMAP_MAX_COMMAND_LEN] = {0};
  char strApIP[INET_ADDRSTRLEN] = {0};

  if(gsb_config == NULL || (gsb_config->if_name == NULL))
  {
    LOG_MSG_ERROR("NULL pointer passed ",0,0,0);
    *qmi_err_num = QMI_ERR_INVALID_ARG_V01;
    return false;
  }

  /* validating input range */
  if (gsb_config->bw_reqd_in_mb > 900 || gsb_config->if_high_watermark > 600 ||
        gsb_config->if_low_watermark > 599 ||
          gsb_config->if_type < QCMAP_INTERFACE_TYPE_WLAN_AP ||
            gsb_config->if_type > QCMAP_INTERFACE_TYPE_ETHERNET)
  {
    LOG_MSG_ERROR("Invalid range detected",0,0,0);
    *qmi_err_num = QMI_ERR_INVALID_ARG_V01;
    return false;
  }

  QCMAP_LOG_FUNC_ENTRY();

  readable_addr(AF_INET, &gsb_config->ap_ip, strApIP);

  snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "%s %s %s %u %u %u %d %s",
                            GSB_CONFIG_FILE,
                            "SET_GSB_CONFIG",
                            gsb_config->if_name,
                            gsb_config->bw_reqd_in_mb,
                            gsb_config->if_high_watermark,
                            gsb_config->if_low_watermark,
                            gsb_config->if_type,
                            strApIP);
  ds_system_call(cmd, strlen(cmd));

  return true;
} /*End SetGSBConfig() */


/*===========================================================================
FUNCTION GetGSBConfig()
===========================================================================*/
/*!
@brief
  - GetGSBConfig from qcmap client

@return
  true - Success
  false - Failure

@param[in]
  qcmap_gsb_config gsb_config
  uint8 num_of_entries

@note
  - Dependencies
  - None

  - Side Effects
  - None
*/
/*=========================================================================*/
boolean QCMAP_LAN_Client::GetGSBConfig
(
  qcmap_gsb_config  *gsb_config,
  uint8_t           *num_of_entries,
  qmi_error_type_v01 *qmi_err_num
)
{
  int i = 0;
  char cmd[QCMAP_MAX_COMMAND_LEN] = {0};
  in_addr addr;
  if(gsb_config == NULL || num_of_entries == NULL)
  {
    LOG_MSG_ERROR("NULL pointer passed ",0,0,0);
    *qmi_err_num = QMI_ERR_INVALID_ARG_V01;
    return false;
  }

  UCI_GET_INT_OPTION(*num_of_entries, \
      static_cast<int>(config_file::GSB), "no_of_configs", 0, "num_of_entries")
  if (*num_of_entries > QCMAP_MAX_IF_SUPPORTED || *num_of_entries == 0)
  {
    LOG_MSG_INFO1("Too many entries or none: %d", *num_of_entries,0,0);
    *qmi_err_num = QMI_ERR_INVALID_ARG_V01;
    return false;
  }

  for (i = 0; i < *num_of_entries; i++)
  {
    UCI_GET_STR_OPTION(gsb_config[i].if_name, \
        static_cast<int>(config_file::GSB), "gsb_info", i, "if_name")

    UCI_GET_INT_OPTION(gsb_config[i].bw_reqd_in_mb, \
        static_cast<int>(config_file::GSB), "gsb_info", i, "bw_reqd_in_mb")
    UCI_GET_INT_OPTION(gsb_config[i].if_high_watermark, \
        static_cast<int>(config_file::GSB), "gsb_info", i, "if_high_watermark")
    UCI_GET_INT_OPTION(gsb_config[i].if_low_watermark, \
        static_cast<int>(config_file::GSB), "gsb_info", i, "if_low_watermark")
    UCI_GET_INT_OPTION(gsb_config[i].if_type, \
        static_cast<int>(config_file::GSB), "gsb_info", i, "if_type")

    UCI_GET_ADDR_OPTION(gsb_config[i].ap_ip, \
        static_cast<int>(config_file::GSB), "gsb_info", i, "ap_ip")
  }

  return true;
}


/*=========================================================================
FUNCTION DeleteGSBConfig()
===========================================================================*/
/*!
@brief
  - DeleteGSBConfig from qcmap client

@param[in]
    char * if_name

@note
  - Dependencies
  - None

  - Side Effects
  - None

*/
/*=======================================================================*/
boolean QCMAP_LAN_Client::DeleteGSBConfig
(
  char* if_name,
  qmi_error_type_v01 *qmi_err_num
)
{
  /* Delete GSB config */
  char cmd[QCMAP_MAX_COMMAND_LEN] = {0};

  if(if_name == NULL)
  {
    LOG_MSG_ERROR("NULL pointer passed ",0,0,0);
    *qmi_err_num = QMI_ERR_INVALID_ARG_V01;
    return false;
  }

  LOG_MSG_INFO1("removing %s IFACE from GSB",if_name,0,0);

  snprintf(cmd, QCMAP_MAX_COMMAND_LEN, "%s %s %s",
                            GSB_CONFIG_FILE,
                            "DELETE_GSB_CONFIG",
                            if_name);
  ds_system_call(cmd, strlen(cmd));

  return true;

}
