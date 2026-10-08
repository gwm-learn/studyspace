#ifndef _QCMAP_EZMESH_CLIENT_H_
#define _QCMAP_EZMESH_CLIENT_H_

/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                   _ Q C M A P _ E Z M E S H _ C L I E N T . H

*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/
/**
  @file QCMAP_EZMesh_Client.h
  @brief QCMAP EZMesh public function declarations.
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

FILE:  QCMAP_EZMesh_Client.h

SERVICES:
   QCMAP Client EZMesh

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
  04/30/23   ab         Added EZMesh API Support in Openwrt
===========================================================================*/
/* group: qcmap_ezmesh */
#include "QCMAP_LAN_Client_Common.h"

/** @addtogroup qcmap_ezmesh_macros
    @{ */

/** Shell script path for WLAN config */
#define WLAN_EZMESH_CONFIG_FILE "/etc/data/wlanEZMeshConfig.sh"

/** Set EZMesh enable & capability config */
#define WLAN_SET_EZMESH_CONFIG "set_ezmesh_config"

/** Set EZMesh primary AP config */
#define WLAN_SET_EZMESH_AP_CONFIG "set_ezmesh_ap_config"

/** Set EZMesh AP counts per band */
#define WLAN_SET_EZMESH_PER_BAND_COUNT "set_ezmesh_per_band_count"

/** Set EZMesh traffic separation config */
#define WLAN_SET_EZMESH_TRAFFIC_SEPARATION_CONFIG "set_ezmesh_r2_traffic_separation"

/** Set EZMesh vlan mapping config */
#define WLAN_SET_EZMESH_VLAN_CONFIG "set_ezmesh_vlan_config"

/** Set EZMesh R3 service priority config */
#define WLAN_SET_EZMESH_R3_SERVICE_PRIORITY "set_ezmesh_r3_service_priority"

/** Set EZMesh vlan bridge context */
#define WLAN_SET_EZMESH_CREATE_BRIDGE_CONTEXT "set_ezmesh_create_bridge_context"

/** Delete EZMesh vlan bridge context */
#define WLAN_SET_EZMESH_DELETE_BRIDGE_CONTEXT "set_ezmesh_delete_bridge_context"

/** Create and update wirless config file */
#define WLAN_SET_EZMESH_UPDATE_WIRELESS "set_ezmesh_update_wireless"

/** Update the qcmap_ezmesh uci with iface names once ezmesh bring up success */
#define WLAN_SET_EZMESH_UPDATE_IFACES_CONFIG "set_ezmesh_update_ifaces"

/** Call repacd script to start */
#define WLAN_SET_EZMESH_REPACD_START "set_ezmesh_repacd_start"

/** Call repacd script to stop */
#define WLAN_SET_EZMESH_REPACD_STOP "set_ezmesh_repacd_stop"

/** Call repacd script to restart */
#define WLAN_SET_EZMESH_REPACD_RESTART "set_ezmesh_repacd_restart"

/** Call repacd script to restore */
#define WLAN_SET_EZMESH_REPACD_RESTORE "set_ezmesh_repacd_restore"

/** Call wifi on ezmesh enable bootup */
#define WLAN_EZMESH_ENABLE_ON_BOOTUP "enable_ezmesh_on_bootup"

/** Activate ezmesh Hostapd config */
#define ACTIVATE_EZMESH_HOSTAPD_CONFIG "activate_ezmesh_hostapd_config"

/** Activate ezmesh Hostapd config */
#define ACTIVATE_EZMESH_HOSTAPD_AP_CONFIG "activate_ezmesh_hostapd_ap_config"

/** Activate ezmesh Hostapd restart */
#define ACTIVATE_EZMESH_HOSTAPD_RESTART "activate_ezmesh_hostapd_restart"

/** ACTIVATE_HOSTAPD_VALID */
#define ACTIVATE_HOSTAPD_VALID "Valid"

/** Command to get the ezmesh daemon status */
#define IS_EZMESH_PROCESS_RUNNING "pidof ezmesh"

/** IPA CLI commands to update the ezmesh status */
#define UPDATE_EZMESH_ENABLE_TO_IPA "ipa -X easymesh enable mode"
#define UPDATE_EZMESH_VLAN_ENABLE_TO_IPA "EnableVlan"
#define UPDATE_EZMESH_VLAN_DISABLE_TO_IPA "DisableVlan"

#define UPDATE_EZMESH_DISABLE_TO_IPA "ipa -X easymesh disable"

/** IPA CLI commands to update the dscp service priority status */
#define UPDATE_EZMESH_DSCP_ENABLE_TO_IPA "ipa -X dscp enable"

#define UPDATE_EZMESH_DSCP_DISABLE_TO_IPA "ipa -X dscp disable"

/** @} */ /* end_addtogroup qcmap_ezmesh_macros */

#if 0
/* QCMAP EZMesh WLAN iface index enum */
typedef enum {
  QCMAP_WLAN_IFACE_INDEX_ENUM_MIN_ENUM_VAL = -2147483647, /**< To force a 32 bit signed enum.  Do not change or use*/
  QCMAP_WLAN_IFACE_PRIMARY_AP = 0x00, /**<  Primary AP iface  */
  QCMAP_WLAN_IFACE_GUEST_AP_ONE = 0x01, /**<  Guest AP iface  */
  QCMAP_WLAN_IFACE_GUEST_AP_TWO = 0x02, /**<  Guest AP 2 iface  */
  QCMAP_WLAN_IFACE_GUEST_AP_THREE = 0x03, /**<  Guest AP 3 iface  */
  QCMAP_WLAN_IFACE_GUEST_AP_FOUR = 0x04, /**<  Guest AP 4 iface  */
  QCMAP_WLAN_IFACE_GUEST_AP_FIVE = 0x05, /**<  Guest AP 5 iface  */
  QCMAP_WLAN_IFACE_GUEST_AP_SIX = 0x06, /**<  Guest AP 6 iface  */
  QCMAP_WLAN_IFACE_GUEST_AP_SEVEN = 0x07, /**<  Guest AP 7 iface  */
  QCMAP_WLAN_IFACE_GUEST_AP_EIGHT = 0x08, /**<  Guest AP 8 iface  */
  QCMAP_WLAN_IFACE_GUEST_AP_NINE = 0x09, /**<  Guest AP 9 iface  */
  QCMAP_WLAN_IFACE_GUEST_AP_TEN = 0x0A, /**<  Guest AP 10 iface  */
  QCMAP_WLAN_IFACE_GUEST_AP_ELEVEN = 0x0B, /**<  Guest AP 11 iface  */
  QCMAP_WLAN_IFACE_GUEST_AP_TWELVE = 0x0C, /**<  Guest AP 12 iface  */
  QCMAP_WLAN_IFACE_GUEST_AP_THIRTEEN = 0x0D, /**<  Guest AP 13 iface  */
#if 0
  QCMAP_WLAN_IFACE_GUEST_AP_FOURTEEN = 0x0F, /**<  Guest AP 14 iface  */
  QCMAP_WLAN_IFACE_GUEST_AP_FIFTEEN = 0x10, /**<  Guest AP 15 iface  */
  QCMAP_WLAN_IFACE_GUEST_AP_SIXTEEN = 0x11, /**<  Guest AP 16 iface  */
  QCMAP_WLAN_IFACE_GUEST_AP_SEVENTEEN = 0x12, /**<  Guest AP 17 iface  */
  QCMAP_WLAN_IFACE_GUEST_AP_EIGHTEEN = 0x13, /**<  Guest AP 18 iface  */
  QCMAP_WLAN_IFACE_GUEST_AP_NINETEEN = 0x14, /**<  Guest AP 19 iface  */
  QCMAP_WLAN_IFACE_GUEST_AP_TWENTY = 0x15, /**<  Guest AP 20 iface  */
#endif
  QCMAP_WLAN_IFACE_INDEX_ENUM_MAX_ENUM_VAL = 2147483647 /**< To force a 32 bit signed enum.  Do not change or use*/
}qcmap_ezmesh_iface_index_enum;
#endif

/** @} */ /* end_addtogroup qcmap_ezmesh_datatypes */


#endif
