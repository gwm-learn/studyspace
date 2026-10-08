#ifndef _QCMAP_LAN_CLIENT_COMMON_H_
#define _QCMAP_LAN_CLIENT_COMMON_H_

/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                   _ Q C M A P _ E Z M E S H _ C L I E N T . H

*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/
/**
  @file QCMAP_LAN_Client_Common.h
  @brief QCMAP LAN Client Common public function declarations.
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

FILE:  QCMAP_LAN_Client_Common.h

SERVICES:
   QCMAP LAN Client Common

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
  04/30/23   ab         Added Common API Support in Openwrt
===========================================================================*/
/* group: qcmap_lan_client_common */

/** @addtogroup qcmap_lan_common_constants
    @{ */
/** Macro for all filenames we use for uci operations
 *  Add the corresp. index in <enum class config_file>
 *  whenever a new entry is added.
 */
#define owrt_filename ((char const*[]){ "qcmap_lan", "qcmap_firewall", "firewall", "network" ,"dhcp", "qcmap_wlan", "gsb", "qcmap_ezmesh", "qcmap_current_lan_config", "qcmap_wlan_current" })

#define QCMAP_MAX_COMMAND_LEN     600   /* Max Command length */

#define QCMAP_WLAN_2GHz_BAND     2
#define QCMAP_WLAN_5GHz_BAND     5
#define QCMAP_WLAN_6GHz_BAND     6

/* Unload the emac ioss modules */
#define UNLOAD_ETH_R8125_IOSS_MODULE  "rmmod r8125_ioss.ko"
#define UNLOAD_ETH_AQC_IOSS_MODULE    "rmmod aqc_ioss.ko"
#define UNLOAD_ETH_EMAC_IOSS_MODULE   "rmmod iemac_ioss.ko"
#define UNLOAD_ETH_IOSS_MODULE        "rmmod ioss.ko"

/* Load the emac ioss modules */
#define LOAD_ETH_IOSS_MODULE          "insmod /lib/modules/emac/ioss.ko"
#define LOAD_ETH_EMAC_IOSS_MODULE     "insmod /lib/modules/emac/iemac_ioss.ko"
#define LOAD_ETH_AQC_IOSS_MODULE      "insmod /lib/modules/emac/aqc_ioss.ko"
#define LOAD_ETH_R8125_IOSS_MODULE    "insmod /lib/modules/emac/r8125_ioss.ko"

/** Shell script path for DHCP option script */
#define DHCP_OPT_FILE "/etc/data/dhcpOption.sh"
#define FEATURE_DHCP_TZ 0
#define FEATURE_DHCP_VENDOR_INFO 1

/** @} */ /* end_addtogroup qcmap_lan_common_constants */

/** @addtogroup qcmap_lan_client_common_macros
    @{ */
/* Macro to get uci need define a char array with name result,  */
#define UCI_EZMESH_GET_INT_OPTION(retValue, sectionName, optionName) \
        { \
          memset(result, 0, QCMAP_MAX_SCAN_SIZE); \
          if (!QCMAP_LAN_Client::UciGetUtility(owrt_filename[7], sectionName, 0, optionName, result, 0)) \
          {\
            LOG_MSG_ERROR("Failed to UCI GET %s/%s",sectionName,optionName,0);\
            return false;\
          }\
          retValue = atoi(result);\
        }

/** this enum is used for indexing owrt_filename[] */
enum class config_file {
  QCMAP_LAN = 0,
  QCMAP_FIREWALL = 1,
  FIREWALL = 2,
  NETWORK = 3,
  DHCP = 4,
  QCMAP_WLAN = 5,
  GSB = 6,
  QCMAP_EZMESH = 7
};

typedef enum {
  QCMAP_PROFILE_FULL_ACCESS = 0, /**<  AP Can Access complete LAN Including Internet  */
  QCMAP_PROFILE_INTERNET_ONLY = 1 /**<  AP Can Access Only Internet               */
}qcmap_access_profile;

typedef enum {
  QCMAP_ACTIVATE_HOSTAPD_AP_ENUM_MIN_ENUM_VAL = -2147483647, /**< To force a 32 bit signed enum.  Do not change or use*/
  QCMAP_PRIMARY_AP = 0x01, /**<  Primary AP.  */
  QCMAP_GUEST_AP = 0x02, /**<  Guest AP.  */
  QCMAP_GUEST_AP_2 = 0x03, /**<  Guest AP 2.  */
  QCMAP_GUEST_AP_3 = 0x04, /**<  Guest AP 3.  */
  QCMAP_ALL_AP = 0x05, /**<  All Primary and Guest APs.  */
  QCMAP_GUEST_AP_4 = 0x06, /**<  Guest AP 4  */
  QCMAP_GUEST_AP_5 = 0x07, /**<  Guest AP 5  */
  QCMAP_GUEST_AP_6 = 0x08, /**<  Guest AP 6  */
  QCMAP_GUEST_AP_7 = 0x09, /**<  Guest AP 7  */
  QCMAP_GUEST_AP_8 = 0x0A, /**<  Guest AP 8  */
  QCMAP_GUEST_AP_9 = 0x0B, /**<  Guest AP 9  */
  QCMAP_GUEST_AP_10 = 0x0C, /**<  Guest AP 10  */
  QCMAP_GUEST_AP_11 = 0x0D, /**<  Guest AP 11  */
  QCMAP_GUEST_AP_12 = 0x0E, /**<  Guest AP 12  */
  QCMAP_GUEST_AP_13 = 0x0F, /**<  Guest AP 13  */
#if 0
  QCMAP_GUEST_AP_14 = 0x10, /**<  Guest AP 14  */
  QCMAP_GUEST_AP_15 = 0x20, /**<  Guest AP 15  */
  QCMAP_GUEST_AP_16 = 0x30, /**<  Guest AP 16  */
  QCMAP_GUEST_AP_17 = 0x40, /**<  Guest AP 17  */
  QCMAP_GUEST_AP_18 = 0x50, /**<  Guest AP 18  */
  QCMAP_GUEST_AP_19 = 0x60, /**<  Guest AP 19  */
  QCMAP_GUEST_AP_20 = 0x70, /**<  Guest AP 20  */
#endif
  QCMAP_ACTIVATE_HOSTAPD_AP_ENUM_MAX_ENUM_VAL = 2147483647 /**< To force a 32 bit signed enum.  Do not change or use*/
}qcmap_activate_hostapd_ap_enum;

/** @} */ /* end_addtogroup qcmap_lan_client_common_macros */

#endif
