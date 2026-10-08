/*====================================================

FILE:  QCMAP_LAN_Util.h

SERVICES:
QCMAP LAN Utility header file

=====================================================

  Copyright (c) 2023 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Confidential and Proprietary - Qualcomm Technologies, Inc.

=====================================================*/

#ifndef _QCMAP_LAN_COMMON_H_
#define _QCMAP_LAN_COMMON_H_


#ifdef FEATURE_QTIMAP_OFFTARGET
#include <glib.h>
#endif
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
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
#include "qcmap_client_util.h"

#include <sstream>
#ifdef FEATURE_EXTERNAL_AP
#include "string.h"
#else
#include "stringl.h"
#include <string>
#include "comdef.h"
#endif /*FEATURE_EXTERNAL_AP */
#include "ds_util.h"
#ifdef FEATURE_QTIMAP_OFFTARGET
#include <tf_qcmap.h>
#endif

#ifdef USE_GLIB
#include <glib.h>
#define strlcpy g_strlcpy
#define strlcat g_strlcat
#endif


/** @addtogroup qcmap_lan_constants
@{ */

/** Macro for all filenames we use for uci operations
 *  Add the corresp. index in <enum class config_file>
 *  whenever a new entry is added.
 */
/** Max string length */
#define QCMAP_MAX_STRING_LEN 255

/** Max comand string length*/
#undef MAX_COMMAND_STR_LEN
#define MAX_COMMAND_STR_LEN 600

/** Max scan size */
#define QCMAP_MAX_SCAN_SIZE 100

/** @} */ /* end_addtogroup qcmap_lan_constants */


/*===========================================================================
LOG Msg Macros
=============================================================================*/

#ifdef FEATURE_EXTERNAL_AP
#define  LOG_MSG_INFO1(...) \
  printf("INFO1: "); \
  printf(__VA_ARGS__); \
  printf("\n")
#define  LOG_MSG_INFO2(...) \
  printf("INFO2: "); \
  printf(__VA_ARGS__); \
  printf("\n")
#define  LOG_MSG_INFO3(...) \
  printf("INFO3: "); \
  printf(__VA_ARGS__); \
  printf("\n")
#define  LOG_MSG_ERROR(...) \
  printf("ERROR: "); \
  printf(__VA_ARGS__); \
  printf("\n")

#elif defined(FEATURE_QTIMAP_OFFTARGET)
#undef LOG_MSG_INFO1
#define LOG_MSG_INFO1(fmtString, x, y, z) \
{ \
  if ( x != 0 && y !=0 && z != 0) \
    fprintf(stderr, "\nINFO1:" fmtString"\n", x, y, z); \
  else if ( x != 0 && y != 0) \
    fprintf(stderr, "\nINFO1:" fmtString"\n", x, y); \
  else if ( x != 0) \
    fprintf(stderr, "\nINFO1:" fmtString"\n", x); \
  else \
    fprintf(stderr, "\nINFO1:" fmtString"\n"); \
}
#undef LOG_MSG_INFO2
#define LOG_MSG_INFO2(fmtString, x, y, z) \
{ \
  if ( x != 0 && y !=0 && z != 0) \
    fprintf(stderr, "\nINFO1:" fmtString"\n", x, y, z); \
  else if ( x != 0 && y != 0) \
    fprintf(stderr, "\nINFO1:" fmtString"\n", x, y); \
  else if ( x != 0) \
    fprintf(stderr, "\nINFO1:" fmtString"\n", x); \
  else \
    fprintf(stderr, "\nINFO1:" fmtString"\n"); \
}

#undef LOG_MSG_INFO3
#define LOG_MSG_INFO3(fmtString, x, y, z) \
{ \
  if ( x != 0 && y !=0 && z != 0) \
    fprintf(stderr, "\nINFO3:" fmtString"\n", x, y, z); \
  else if ( x != 0 && y != 0) \
    fprintf(stderr, "\nINFO3:" fmtString"\n", x, y); \
  else if ( x != 0) \
    fprintf(stderr, "\nINFO3:" fmtString"\n", x); \
  else \
    fprintf(stderr, "\nINFO3:" fmtString"\n"); \
}

#undef LOG_MSG_ERROR
#define LOG_MSG_ERROR(fmtString, x, y, z) \
{ \
  if ( x != 0 && y !=0 && z != 0) \
    fprintf(stderr, "\nError:" fmtString"\n", x, y, z); \
  else if ( x != 0 && y != 0) \
    fprintf(stderr, "\nError:" fmtString"\n", x, y); \
  else if ( x != 0) \
    fprintf(stderr, "\nError:" fmtString"\n", x); \
  else \
    fprintf(stderr, "\nError:" fmtString"\n"); \
}
#else
/** @addtogroup qcmap_lan_macros
@{ */

/*
Log Message Macros
*/
/** Macro for a high-level message. */
#define LOG_MSG_INFO1_LEVEL           MSG_LEGACY_HIGH

/** Macro for a medium-level message. */
#define LOG_MSG_INFO2_LEVEL           MSG_LEGACY_MED

/** Macro for a low-level message. */
#define LOG_MSG_INFO3_LEVEL           MSG_LEGACY_LOW

/**  Macro for a error message. */
#define LOG_MSG_ERROR_LEVEL           MSG_LEGACY_ERROR

/** Macro to print the log message information. */
#define PRINT_MSG( level, fmtString, x, y, z)                         \
        MSG_SPRINTF_4( MSG_SSID_LINUX_DATA, level, "%s(): " fmtString,      \
                       __FUNCTION__, x, y, z);

/** Macro to print a high-level message. */
#define LOG_MSG_INFO1( fmtString, x, y, z)                            \
{                                                                     \
  PRINT_MSG( LOG_MSG_INFO1_LEVEL, fmtString, x, y, z);                \
}

/** Macro to print a medium-level message. @newpage */
#define LOG_MSG_INFO2( fmtString, x, y, z)                            \
{                                                                     \
  PRINT_MSG( LOG_MSG_INFO2_LEVEL, fmtString, x, y, z);                \
}
/** Macro to print a low-level message. */
#define LOG_MSG_INFO3( fmtString, x, y, z)                            \
{                                                                     \
  PRINT_MSG( LOG_MSG_INFO3_LEVEL, fmtString, x, y, z);                \
}
/** Macro to print an error message. */
#define LOG_MSG_ERROR( fmtString, x, y, z)                            \
{                                                                     \
  PRINT_MSG( LOG_MSG_ERROR_LEVEL, fmtString, x, y, z);                \
}

#endif

#define QCMAP_LAN_CLIENT_RUN_COMMANDS(format, ...) {\
      char command[QCMAP_MAX_COMMAND_LEN] = {0};\
      snprintf(command, QCMAP_MAX_COMMAND_LEN, format, ##__VA_ARGS__);\
      ds_system_call(command, strlen(command));\
    }

#define IPv4_DISABLE_NAT_EXCLUSIVITY_CHECK(profile_handle, qmi_err_num, ret_val)        \
{                                                                                       \
   boolean ipv4_nat_disable;                                                            \
   if(QCMAP_LAN_Client::GetV4NATConfig(profile_handle, ipv4_nat_disable, qmi_err_num))  \
   {                                                                                    \
     if(ipv4_nat_disable)                                                               \
     {                                                                                  \
       LOG_MSG_ERROR(                                                                   \
       "Exclusive Feature IPv4 NAT Disable Config is enabled. Returning..",             \
        0,0,0);                                                                         \
        *qmi_err_num = QMI_ERR_INCOMPATIBLE_STATE_V01;                                  \
        return (ret_val);                                                               \
     }                                                                                  \
   }                                                                                    \
   else                                                                                 \
   {                                                                                    \
     LOG_MSG_ERROR("GetV4NATConfig Failed", 0,0,0);                                     \
     return false;                                                                      \
   }                                                                                    \
}

/*=====================================================================
  FUNCTION ExecuteSystemCmd
======================================================================*/
/*!
@brief
  - executes the system cmd and returns the output of the command

@return
  true - Success
  false - Failure

@note
  - fgets() is used to read the output of the command so the returned output
    will be either of size (result_len -1) characters, or till the newline char
    or end-of-file character whichever comes first

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
boolean
ExecuteSystemCmd
(
  const char *cmd,
  char *result,
  uint8_t result_len,
  int *error_num = 0
);

/*===========================================================================
FUNCTION  GetWLANEXIfaceIndex
==========================================================================*/
/*!
@brief
This function is get the WLAN Iface Index given the ap_type and iface_type

@parameters
  qcmap_msgr_activate_hostapd_ap_enum_v01 ap_type

@return
qcmap_msgr_wlan_iface_index_enum_v01

@note

- Dependencies
-None

- Side Effects
- None
*/
/*=========================================================================*/
qcmap_msgr_wlan_iface_index_enum_v01 GetWLANEXIfaceIndex
(
  uint16 ap_type
);

#endif
