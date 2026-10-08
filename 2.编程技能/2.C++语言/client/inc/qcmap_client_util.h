#ifndef _QCMAP_CLIENT_UTIL_H_
#define _QCMAP_CLIENT_UTIL_H_

/******************************************************************************

                         qcmap_client_util.h

******************************************************************************/

/******************************************************************************

  @file    qcmap_firewall.h

  DESCRIPTION
  Header file for firewall data structure.

  ---------------------------------------------------------------------------
  Copyright (c) 2011-2013, 2019, 2021 Qualcomm Technologies, Inc.  All Rights Reserved.
  Qualcomm Technologies Proprietary and Confidential.
  ---------------------------------------------------------------------------

******************************************************************************/

/******************************************************************************

                      EDIT HISTORY FOR FILE

when       who        what, where, why
--------   ---        -------------------------------------------------------
07/11/12   bnn         9x25

******************************************************************************/

/*===========================================================================

                          INCLUDE FILES FOR MODULE

===========================================================================*/
#ifdef FEATURE_EXTERNAL_AP
  #include <inttypes.h>
  /* Constants and Types */
  #ifdef TRUE
  #undef TRUE
  #endif

  #ifdef FALSE
  #undef FALSE
  #endif

  #define TRUE   1   /* Boolean true value. */
  #define FALSE  0   /* Boolean false value. */

  typedef  uint8_t            boolean;     /* Boolean value type. */

  typedef  uint64_t           uint64;      /* Unsigned 32 bit value */
  typedef  uint32_t           uint32;      /* Unsigned 32 bit value */
  typedef  uint16_t           uint16;      /* Unsigned 16 bit value */
  typedef  uint8_t            uint8;       /* Unsigned 8  bit value */

  typedef  int32_t            int32;       /* Signed 32 bit value */
  typedef  int16_t            int16;       /* Signed 16 bit value */
  typedef  int8_t             int8;        /* Signed 8  bit value */
#else
  #include "comdef.h"
#endif /*FEATURE_EXTERNAL_AP */
#include "qcmap_firewall_util.h"
#include "qualcomm_mobile_access_point_msgr_v01.h"
/*===========================================================================
MACRO IPV4_ADDR_MSG()

DESCRIPTION
  This macro prints an IPV4 address to F3.

PARAMETERS
  ip_addr: The IPV4 address in host byte order.

RETURN VALUE
  none
===========================================================================*/
#ifdef FEATURE_EXTERNAL_AP
#define  IPV4_ADDR_MSG(...) do {break;} while(0);
#else
#define IPV4_ADDR_MSG(ip_addr) MSG_4(MSG_SSID_DS, \
                        MSG_LEGACY_HIGH, \
                        "IPV4 Address is %d.%d.%d.%d", \
                        (unsigned char)(ip_addr), \
                        (unsigned char)(ip_addr >> 8), \
                        (unsigned char)(ip_addr >> 16) , \
                        (unsigned char)(ip_addr >> 24))

#endif /*FEATURE_EXTERNAL_AP */
/*===========================================================================
MACRO IPV6_ADDR_MSG()

DESCRIPTION
  This macro prints an IPV6 address to F3.

PARAMETERS
  ip_addr: The IPV6 address in network byte order.

RETURN VALUE
  none
===========================================================================*/
#ifdef FEATURE_EXTERNAL_AP
#define  IPV6_ADDR_MSG(...) do {break;} while(0);
#else
#define IPV6_ADDR_MSG(ip_addr) MSG_8(MSG_SSID_DS, \
                        MSG_LEGACY_HIGH, \
                        "IPV6 Address %x:%x:%x:%x:%x:%x:%x:%x", \
                        (uint16)(ps_ntohs(ip_addr[0])), \
                        (uint16)(ps_ntohs(ip_addr[0] >> 16)), \
                        (uint16)(ps_ntohs(ip_addr[0] >> 32)) , \
                        (uint16)(ps_ntohs(ip_addr[0] >> 48)), \
                        (uint16)(ps_ntohs(ip_addr[1])), \
                        (uint16)(ps_ntohs(ip_addr[1] >> 16)), \
                        (uint16)(ps_ntohs(ip_addr[1] >> 32)) , \
                        (uint16)(ps_ntohs(ip_addr[1] >> 48)))

#endif /*FEATURE_EXTERNAL_AP */
#define MAX_COMMAND_STR_LEN 200
#define IPV4_ADDR_LEN 4
#define IPV6_ADDR_LEN 16
#define IPTABLE_CHAIN 10
#define QCMAP_MAX_FIREWALL_ENTRY_SUPPORTED 128

/* Zero Initialize an variable */
#define ZERO_INIT_ARG(arg)              memset(&arg, 0, sizeof(arg));


/** Max Child SA IPsec tunnel can have */
#define QCMAP_MAX_IPSEC_CHILD 10
/** Max string length */
#define QCMAP_MAX_STRING_LEN 255

/*Data structure for IPsec*/
typedef enum
{
  QCMAP_IPSEC_INVALID_TOPOLOGY = 0x0,    /**< Invalid Topology */
  QCMAP_IPSEC_HOST_TO_HOST_TOPOLOGY,     /**< IPsec Host-to-Host Topology */
  QCMAP_IPSEC_SITE_TO_SITE_WITHOUT_NAT_TOPOLOGY,   /**< IPsec Site-to-Site-Without-NAT Topology */
  QCMAP_IPSEC_HOST_TO_HOST_AND_SITE_TO_SITE_WITHOUT_NAT_TOPOLOGY  /**< IPsec Host-to-Host and Site-to-Site-Without-NAT Topology */
} qcmap_ipsec_topology_e;


typedef enum
{
  QCMAP_IPSEC_INVALID_TUNNEL_TYPE = 0,  /**< Invalid Tunnel Type */
  QCMAP_IPSEC_V4_ESP_TUNNEL_MODE_TUNNEL_TYPE, /**< IPv4 Tunnel Type */
  QCMAP_IPSEC_V6_ESP_TUNNEL_MODE_TUNNEL_TYPE  /**< IPv6 Tunnel Type */
} qcmap_ipsec_tunnel_type_e;


typedef enum
{
  QCMAP_IPSEC_INVALID_AUTHENTICATION_TYPE = 0,  /**< Invalid Authentication Type */
  QCMAP_IPSEC_PSK_AUTHENTICATION_TYPE,   /**< PSK Authentication Type */
  QCMAP_IPSEC_X509_AUTHENTICATION_TYPE   /**< X509 Authentication Type */
} qcmap_ipsec_authentication_type_e;


typedef enum
{
  QCMAP_IPSEC_INVALID_PROTOCOL_TYPE = 0, /**< Invalid Protocol Type */
  QCMAP_IPSEC_TCP_PROTOCOL_TYPE,  /**< TCP Protocol Type */
  QCMAP_IPSEC_UDP_PROTOCOL_TYPE   /**< UDP Protocol Type */
} qcmap_ipsec_protocol_type_e;



typedef struct
{
  char ike_identifier[QCMAP_MAX_STRING_LEN]={0};
  /**< IKE identifier for IKE session */

  qcmap_ipsec_tunnel_type_e tunnel_type;
  /**< IPsec Tunnel type */

  qcmap_ipsec_authentication_type_e auth_type;
  /**< IPsec Authentication Type */

  char remote_ep_addr[QCMAP_MAX_STRING_LEN];
  /**< IPsec remote endpoint address */

  char local_identifier[QCMAP_MAX_STRING_LEN];
  /**< IPsec local identifier */

  char remote_identifier[QCMAP_MAX_STRING_LEN];
  /**< IPsec remote identifier */

  uint32_t tunnel_ip;
  /**< IPsec tunnel ip */

  uint8_t rekey_interval_hrs;
  /**< IPsec IKE rekey time in hours */

} qcmap_ipsec_ike_config_t;


typedef struct
{
  char child_identifier[QCMAP_MAX_STRING_LEN]={0};
  /**< IPsec child SA identifier */

  uint16_t port_id;
  /**< IPsec port id for UDP/TCP packet*/

  boolean port_range;
  /**< Check there's a IPsec port range */

  uint16_t end_port_id;
  /**< If there's a port_range, the end port of the tunnel */

  qcmap_ipsec_protocol_type_e protocol_type;
  /**< What type of protocol used for the tunnel */

  char local_addr[QCMAP_MAX_STRING_LEN];
  /**< IPsec local traffic selector */

  char remote_addr[QCMAP_MAX_STRING_LEN];
  /**< IPsec remote traffic selector */

  boolean hw_offload;
  /**< IPsec hardware offload option */

  boolean trap_action;
  /**< IPsec start action: trap/start */

  uint8_t rekey_interval_hrs;
  /**< IPsec child rekey time in hours */

} qcmap_ipsec_child_config_t;


typedef struct
{
  uint32_t profile_id;
  /**< Profile id for IPsec tunnel */

  qcmap_ipsec_topology_e topology;
  /**< IPsec tunnel topology */

  qcmap_ipsec_ike_config_t ike_cfg;
  /**< IPsec IKE configuration */

  uint8_t child_entries;
  /**< Number of child SAs the IKE session has */

  qcmap_ipsec_child_config_t child_cfg[QCMAP_MAX_IPSEC_CHILD];
  /**< IPsec child SAs configuration */

} qcmap_ipsec_config_t;


typedef enum
{
  QCMAP_IPSEC_TUNNEL_DISCONNECTED = 0,
  /**< IPsec tunnel disconnected state */

  QCMAP_IPSEC_TUNNEL_INPROGRESS,
  /**< IPsec tunnel in_progress state */

  QCMAP_IPSEC_TUNNEL_CONNECTED
  /**< IPsec tunnel connected state */

} qcmap_ipsec_tunnel_state_e;


typedef struct
{
  char child_identifier[QCMAP_MAX_STRING_LEN];
  /**< IPsec tunnel child identifier for status */

  bool enable_status;
  /**< IPsec tunnel enable status */

  qcmap_ipsec_tunnel_state_e state;
  /**< IPsec tunnel current state */

} qcmap_ipsec_tunnel_status_t;


typedef struct
{
  uint8_t tunnel_entries;
  /**< Number of IPsec tunnels */

  qcmap_ipsec_tunnel_status_t tunnel_status[QCMAP_MAX_IPSEC_CHILD];
  /**< IPsec tunnel status for each tunnel */

} qcmap_ipsec_tunnel_state_info_t;

/*---------------------------------------------------------------------------
           FireWall Entry Configuration.
-----------------------------------------------------------------------------*/
typedef struct
{
  ip_filter_type filter_spec;
  uint32         firewall_handle;
  /* Direction of the firewall. */
  qcmap_msgr_firewall_direction firewall_direction ;
} qcmap_msgr_firewall_entry_conf_t;

/*---------------------------------------------------------------------------
            FireWall handle list configuration.
---------------------------------------------------------------------------*/
typedef struct
{
  uint32 handle_list[QCMAP_MAX_FIREWALL_ENTRY_SUPPORTED];
  ip_version_enum_type ip_family;
  int num_of_entries;
} qcmap_msgr_get_firewall_handle_list_conf_t;

/*---------------------------------------------------------------------------
            FireWall configuration.
---------------------------------------------------------------------------*/
typedef union
{
  qcmap_msgr_firewall_entry_conf_t extd_firewall_entry;
  qcmap_msgr_get_firewall_handle_list_conf_t extd_firewall_handle_list;
  ip_version_enum_type ip_family;
} qcmap_msgr_firewall_conf_t;
/*---------------------------------------------------------------------------
            VLAN configuration.
---------------------------------------------------------------------------*/
typedef struct
{
  unsigned short vlan_config_list_len;
  qcmap_msgr_vlan_config_v01 vlan_config_list[QCMAP_MSGR_MAX_VLAN_ENTRIES_V01];
  /* Depreceated, use vlan_config_list_ex */

  qcmap_msgr_vlan_config_ex_v01 vlan_config_list_ex[QCMAP_MSGR_MAX_VLAN_ENTRIES_V01];
} qcmap_msgr_vlan_conf_t;
/*---------------------------------------------------------------------------
            L2TP configuration.
---------------------------------------------------------------------------*/
typedef struct
{
  qcmap_msgr_l2tp_mode_enum_v01 mode;
  uint8 l2tp_config_list_len;
  qcmap_msgr_l2tp_config_v01 l2tp_config_list[QCMAP_MSGR_L2TP_MAX_TUNNELS_V01];
  qcmap_msgr_l2tp_mtu_config_v01 l2tp_mtu_config;
  qcmap_msgr_l2tp_TCP_MSS_config_v01 l2tp_mss_config;
  uint32 l2tp_mtu_size;
} qcmap_msgr_l2tp_conf_t;
/*---------------------------------------------------------------------------
            WLAN interface configuration.
---------------------------------------------------------------------------*/
typedef struct
{
  unsigned short wlan_if_info_len;
  qcmap_msgr_wlan_if_info_v01 wlan_if_info[QCMAP_MSGR_MAX_WLAN_IFACE_V01];
} qcmap_msgr_wlan_if_info_t;

#ifdef FEATURE_PORT_TRIGGER
/*---------------------------------------------------------------------------
           Port Trigger Entry Configuration.
-----------------------------------------------------------------------------*/
typedef struct
{
  int      handle;
  uint16_t trigger_start_port;
  uint16_t trigger_end_port;
  uint16_t forward_start_port;
  uint16_t forward_end_port;
  uint16_t trigger_protocol;
  uint16_t forward_protocol;
  int      timer;
} qcmap_msgr_port_trigger_entry_conf_t;

/*---------------------------------------------------------------------------
           Port Trigger Configuration.
-----------------------------------------------------------------------------*/
typedef struct
{
  qcmap_msgr_port_trigger_entry_conf_t port_trigger_entry[QCMAP_MSGR_MAX_PORT_TRIGGER_ENTRIES_V01];
  int                                  num_of_entries;
} qcmap_msgr_port_trigger_conf_t;
#endif

/*
 * For 7AP support we need maximum of 20 guest AP
 *  Below changes need to be removed and should be handled by IDL
 *  We will handle this as part of re acrh3
 */

#define QCMAP_MSGR_MAX_GUEST_AP_COUNT 20


typedef struct {
  qcmap_msgr_wlan_mode_enum_v01         wlan_mode;
  /**< WLAN Mode.
  */
  int                                   primary_ap_band;
  /**< Primary AP Band.
  */
  qcmap_msgr_station_mode_config_v01    station_config;
  /**< Station Configuration.
  */
  int                                   station_band;
  /**< Station Band
  */
  int                                   ap_config_len;
  /** AP Band Config Size.
   */
  qcmap_msgr_wlan_ap_band_config_v01    ap_config[QCMAP_MSGR_MAX_GUEST_AP_COUNT_V01];
  /** AP Band Config
   */
  qcmap_msgr_guest_profile_config_v01   guest_profile;
  /** Guest Access Profile Config
   */
} qcmap_wlan_ex_config;


typedef struct {
  qcmap_msgr_wlan_mode_enum_v01         wlan_mode;
  /**< WLAN Mode.
  */
  int                                   primary_ap_band;
  /**< Primary AP Band.
  */
  qcmap_msgr_station_mode_config_v01    station_config;
  /**< Station Configuration.
  */
  int                                   station_band;
  /**< Station Band
  */
  int                                   ap_config_len;
  /** AP Band Config Size.
   */
  qcmap_msgr_wlan_ap_band_config_v01    ap_config[QCMAP_MSGR_MAX_GUEST_AP_COUNT];
  /** AP Band Config
   */
  qcmap_msgr_guest_profile_config_v01   guest_profile;
  /** Guest Access Profile Config
   */

  int                                   guestap_count_2g;
  int                                   guestap_count_5g;
  int                                   guestap_count_6g;
    /**< Guest ap count in each radio 2G/5G/6G */

} qcmap_wlan_ex2_config;

/**< Maximum number AP count */
#define QCMAP_MSGR_MAX_AP_COUNT 21

/**< Maximum number MLD LINK count */
#define QCMAP_MSGR_MAX_MLD_LINK 21

/**< Maximum number MLD AP count */
#define QCMAP_MSGR_MAX_MLD_AP_COUNT 7

/**< Maximum number AP LINK count per MLD */
#define QCMAP_MAX_NUMBER_OF_MLD_LINK 3

/**< MLD AP IFACE type */
#define QCMAP_MSGR_WLAN_IFACE_MLD_AP 0x0F

/**< MLD STA IFACE type */
#define QCMAP_MSGR_WLAN_IFACE_MLD_STA 0x10

/**< AP config  */
typedef struct {
  uint8_t                          band;
  qcmap_msgr_access_profile_v01    accessprofile;
}qcmap_msgr_ap_config_t;

/**< Station config*/
typedef struct {
  int                                     band;
  qcmap_msgr_sta_connection_enum_v01      conn_type;
  qcmap_msgr_sta_static_ip_config_v01     static_ip_config;
  uint8_t                                 ap_sta_bridge_mode;
}qcmap_msgr_station_config_t;

/**< MLD AP config */
typedef struct {
  uint8_t                           no_of_mld_link;
  uint8_t                           band[QCMAP_MAX_NUMBER_OF_MLD_LINK];
  qcmap_msgr_access_profile_v01     accessprofile;
} qcmap_msgr_mld_ap_t;

/**< MLD station config */
typedef struct {
  uint8_t                                 no_of_mld_link;
  uint8_t                                 band[QCMAP_MAX_NUMBER_OF_MLD_LINK];
  qcmap_msgr_sta_connection_enum_v01      conn_type;
  qcmap_msgr_sta_static_ip_config_v01     static_ip_config;
  uint8_t                                 ap_sta_bridge_mode;
}qcmap_msgr_mld_sta_config_t;

/**< MLD WLAN mode */
typedef enum {
  QCMAP_MSGR_MLD_WLAN_MODE_ENUM_MIN_ENUM_VAL = -2147483647, /**< To force a 32 bit signed enum.  Do not change or use*/
  QCMAP_MSGR_MLD_WLAN_MODE_AP = 0x01,
  QCMAP_MSGR_MLD_WLAN_MODE_AP_STA = 0x03,
  QCMAP_MSGR_MLD_WLAN_MODE_STA = 0x06,
  QCMAP_MSGR_MLD_WLAN_MODE_ENUM_MAX_ENUM_VAL = 2147483647 /**< To force a 32 bit signed enum.  Do not change or use*/
}qcmap_msgr_mld_wlan_mode_enum;


// New structure for qcmap_wlan_ex3_config;
typedef struct {

  /*to store non-mld config */
  // possible WLAN mode : AP, AP-STA or STA
  qcmap_msgr_wlan_mode_enum_v01     wlan_mode;
  qcmap_msgr_ap_config_t            ap_config[QCMAP_MSGR_MAX_AP_COUNT];
  qcmap_msgr_station_config_t       station_config;
  uint8_t                           ap_config_len;
  uint8_t                           ap_count_2g;
  uint8_t                           ap_count_5g;
  uint8_t                           ap_count_6g;
  boolean                           is_sta_configured;

  /*to store mld config*/
  qcmap_msgr_mld_wlan_mode_enum      mld_wlan_mode;
  qcmap_msgr_mld_ap_t                mld_ap_config[QCMAP_MSGR_MAX_MLD_AP_COUNT];
  qcmap_msgr_mld_sta_config_t        mld_sta_config;
  uint8_t                            mld_ap_config_len;
  boolean                            is_mld_sta_configured;
} qcmap_wlan_ex3_config_t;


typedef struct {
  qcmap_msgr_bootup_flag_v01 mobileap_enable;
  /**< Mobileap BootUp Enable Flag.
   */
  qcmap_msgr_bootup_flag_v01 wlan_enable;
  /**< WLAN BootUp Enable Flag.
   */
  qcmap_msgr_bootup_flag_v01 calibration_enable;
  /**< WLAN Calibration Bootup Enable Flag.
   */
} qcmap_bootup_enable_config;


/*---------------------------------------------------------------------------
           Software Path Filter Configuration
-----------------------------------------------------------------------------*/

#define QCMAP_CM_MAX_SW_PATH_FILTERS 5

typedef struct
{
  qcmap_msgr_ip_pt_sw_path_filter_enum_v01  filter_type;
  uint8_t                                   num_of_filters;
  uint32_t                                  public_gateway_ip;
  qcmap_msgr_port_range_and_protocol_v01    filters[QCMAP_CM_MAX_SW_PATH_FILTERS];
} qcmap_msgr_sw_path_filters_conf_t;

#endif
