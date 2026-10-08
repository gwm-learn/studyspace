#ifndef _QCMAP_LAN_CLIENT_H_
#define _QCMAP_LAN_CLIENT_H_

/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                   _ Q C M A P _ L A N _ C L I E N T . H

*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/
/**
  @file QCMAP_LAN_Client.h
  @brief QCMAP LAN Client public function declarations.
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

FILE:  QCMAP_LAN_Client.h

SERVICES:
   QCMAP LAN Client Class

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
  12/12/22   dk         Added WLAN API Support in Openwrt
  11/28/22   ak         Added DHCP Reservation Support in Openwrt
  11/24/22   dk         Support for ETH backhaul
  10/05/22   sp         Introduce IP Passthrough for OpenWRT
  09/30/22   ak         Added Firewall Support in OpenWRT
  08/18/22   dk         Added SNAT support for OpenWRT
  07/26/22   sp         Enable MPDN-VLAN support for OpenWRT
  07/13/22   ak         Enable NAT for OpenWRT
  07/07/22   dk         Added VLAN support for openWRT
  06/08/21   sp         Created LAN client library
===========================================================================*/
/* group: qcmap_lan */
#include "QCMAP_LAN_Util.h"
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
#include "QCMAP_EZMesh_Client.h"
#include "QCMAP_Client.h"
#include "QCMAP_LAN_Client_Common.h"

#include <cstddef>

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

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef __cplusplus
}
#endif

using namespace std;

/** @addtogroup qcmap_lan_constants
@{ */

/** Enum value specifing the uci entry not found  */
#define UCI_ENTRY_NOT_FOUND  EINVAL

/** Maximum size of the file. */
#define QCMAP_MAX_FILE_PATH_LEN 100

/** Default profile handle */
#define QCMAP_DEFAULT_PROFILE_HANDLE 1

/** Max Interface name size */
#define QCMAP_MAX_IFACE_NAME_SIZE 16

/** Max Interface supported */
#define QCMAP_MAX_IF_SUPPORTED 32

#define QCMAP_MAX_WLAN_IFACE 21

/** IPV6 address length */
#define QCMAP_IPV6_ADDR_LEN 16

/** IPV4 address string length */
#define QCMAP_IPV4_ADDR_LEN 16

/** Max string length */
#define QCMAP_MAX_STRING_LEN 255

/** Max comand string length*/
#undef MAX_COMMAND_STR_LEN
#define MAX_COMMAND_STR_LEN 4000

/** MAX VLAN ID*/
#define MAX_VLAN_ID 4094

/** MAX SNAT entries*/
#define QCMAP_MAX_SNAT_ENTRIES 128

/** 32 bit mask */
#define MASK32 0xFFFFFFFF

/** 16 bit mask */
#define MASK16 0xFFFF

/** 8 bit mask */
#define MASK8 0xFF

/** Max Backhauls */
#define QCMAP_MAX_BACKHAULS 50

/** Max vlan entries */
#define QCMAP_MAX_VLAN_ENTRIES 2000

/** Max vlan per pdn entries */
#define QCMAP_MAX_VLAN_PER_PDN 50

/** Max scan size */
#define QCMAP_MAX_SCAN_SIZE 100

/** Max profile handle digits+1 */
#define QCMAP_MAX_PROFILE_HANDLE_SIZE 11

/** Max loop count for add firewall */
#define MAX_LOOP_COUNT 2

/** Setting the firewall mark */
#define FIREWALL_MARK 0x35

/** MAC address length */
#define QCMAP_MAC_ADDR_LEN 6

/** Max device name length */
#define QCMAP_MAX_DEVICE_NAME 100

/** MAC address characters */
#define QCMAP_LAN_MAC_ADDR_NUM_CHARS 18

/** Default bridge ID */
#define DEFAULT_BRIDGE_ID 0

/** Invalid value - generic */
#define QCMAP_LAN_INVALID -1

/** Max number of ETHC NICs*/
#define QCMAP_MAX_ETH_NICS 8

/** MAX CLIENTS */
#define MAX_CLIENT 768

/** Default dhcp start value */
#define DEFAULT_DHCP_START_VALUE 100

/** Maximum DHCP reservation entries */
#define QCMAP_MAX_DHCP_RESERVATION_ENTRIES 20

/** Max Guest AP count*/
#define QCMAP_MAX_GUEST_AP_COUNT 20

/** Default Timeout Values. */
#define QCMAP_NAT_ENTRY_DEFAULT_GENERIC_TIMEOUT 200

#define QCMAP_NAT_ENTRY_DEFAULT_ICMP_TIMEOUT 30

#define QCMAP_NAT_ENTRY_DEFAULT_TCP_TIMEOUT 3600

#define QCMAP_NAT_ENTRY_DEFAULT_UDP_TIMEOUT 60

#define QCMAP_NAT_ENTRY_MIN_TIMEOUT 30

#define QCMAP_NAT_ENTRY_MAX_TIMEOUT 8589934

#define KERNEL_VERSION_4_9  "4.9"

#define KERNEL_VERSION_LENGTH 100

#define PS_IPPROTO_DNS 53

/* Max IPV4 size 3 dots + 4 * 3 #s + 1 null */
#define QCMAP_LAN_MAX_IPV4_ADDR_SIZE   16

/* Min iface type value */
#define MIN_IFACE_TYPE 1

/* Max iface type value */
#define MAX_IFACE_TYPE 6

#define QCMAP_MAX_IFACE_FILTERING 8

#define QCMAP_MAX_IPV4_SEGMENT_FILTERING 16

#define QCMAP_MAX_HW_MAC_FILTER_CLIENTS 32

#define IPA_MAX_IFACE_FILTERING 4

#define MAX_IPA_CMD_LENGTH 1000

#define MAX_IPA_CMD_MAX_LENGTH 2048

/** DNS search list name length */
#define QCMAP_DOMAIN_NAME_MAX_V01 257

/** DNS search list length */
#define QCMAP_MAX_NUM_DNS_SEARCH_LIST 15

#define MAX_FIREWALL_ENTRY 128

/** uint32 form of default subnet 255.255.254.0 */
#define DEFAULT_SUBNET_INT 4294966784

/** Indicates checkIPPT is called when v4 BH is up/down */
#define BH_EVENT 1

/** Numeric zero */
#define NUMERIC_ZERO 0

/** Active IPPT */
#define IPPT_NOT_ACTIVE 0

/** Numeric one */
#define NUMERIC_ONE 1

/** MAX SIP Server entries  */
#define QCMAP_MSGR_MAX_SIP_SERVER_ENTRIES_V01 25

#define IPV4_FIREWALL_NOT_SUPPORTED 0

/** @} */ /* end_addtogroup qcmap_lan_constants */

/** @addtogroup gsb_type_constants
@{ */

#define QCMAP_GSB_DELAY_COUNT 10000 /*10 ms or 10000 us*/

#define NET_DEV_FILE_ROOT_PATH "/sys/class/net"

#define IF_STATE_UP "up"
#define IF_STATE_DOWN "down"
#define IF_STATUS_UP 1
#define IF_STATUS_DOWN 0

/** @} */ /* end_addtogroup gsb_type_constants */

/** @addtogroup nat_type_constants
@{ */

#define SYMMETRIC_NAT "SYM"

#define PORT_RESTRICTED_CONE_NAT "PORT_REST_CONE"

#define FULL_CONE_NAT "FULL_CONE"

#define ADDRESS_RESTRICTED_CONE_NAT "ADDR_REST_CONE"

/** @} */ /* end_addtogroup nat_type_constants */

/** @addtogroup vpn_passthrough_type_constants
@{ */

#define IPSEC_VPN_PASSTHROUGH_V4 "ipsecptv4"

#define IPSEC_VPN_PASSTHROUGH_V6 "ipsecptv6"

#define PPTP_VPN_PASSTHROUGH_V4 "pptpptv4"

#define PPTP_VPN_PASSTHROUGH_V6 "pptpptv6"

#define L2TPIPSEC_VPN_PASSTHROUGH_V4 "l2tp_ipsecptv4"

#define L2TPIPSEC_VPN_PASSTHROUGH_V6 "l2tp_ipsecptv6"

/** @} */ /* end_addtogroup vpn_passthrough_type_constants */

/** @addtogroup qcmap_lan_strings
@{ */
/** wwan bring up event */
#define RMNET_BRING_UP_CMD "up"

/** wwan tear down event */
#define RMNET_TEAR_DOWN_CMD "down"

/** eth pdu mode enable */
#define ETHPDU_MODE_ENABLE_CMD "enable"

/** eth pdu mode disable */
#define ETHPDU_MODE_DISABLE_CMD "disable"

/** invalid profile handle */
#define INVALID_ETH_PDU_PROFILE_HANDLE 255


/** Map add command */
#define QCMAP_ADD_MAP_VLAN_PDN "add_map"

/** Map delete command */
#define QCMAP_DEL_MAP_VLAN_PDN "del_map"

/** Map default config rule command */
#define QCMAP_CREATE_WWAN_PROFILE "create_wwan_profile"

/** Delete WWAN polciy command */
#define QCMAP_DELETE_WWAN_PROFILE "delete_wwan_profile"

/** Map add iface on wan_all list command */
#define QCMAP_ADD_WAN_IFACE_ON_WAN_ALL_LIST "add_wan_iface_on_wan_all_zone"

/** Map delete iface from wan_all list command */
#define QCMAP_DEL_WAN_IFACE_FROM_WAN_ALL_LIST "del_wan_iface_from_wan_all_zone"

/** uci set command */
#define UCI_SET_COMMAND "/etc/data/uci_ex.sh set"

/** uci get command */
#define UCI_GET_COMMAND "/etc/data/uci_ex.sh get"

/** uci delete command */
#define UCI_DELETE_COMMAND "/etc/data/uci_ex.sh delete"

/** uci command to get number of vlan's */
#define UCI_QUERY_NO_OF_VLAN_COMMAND "qcmap_lan.@no_of_configs[0].no_of_vlans"

/** uci command to get number of redirects */
#define UCI_QUERY_NO_OF_REDIRECTS_COMMAND "qcmap_lan.@no_of_configs[0].no_of_redirects"

/** uci command to get number of Port Trigger entires */
#define UCI_QUERY_NO_OF_PORT_TRIGGER_ENTRIES "qcmap_lan.@no_of_configs[0].no_of_port_trigger_info"

/** uci command to get total number of wwan profiles created */
#define UCI_QUERY_NO_OF_WWAN_PROFILES "qcmap_lan.@no_of_configs[0].no_of_profiles"

/** uci command to get total number of firewall rules */
#define UCI_QUERY_NO_OF_FIREWALL_ENTRIES "qcmap_lan.@no_of_configs[0].no_of_rules"

/** uci command to get number of Firewall rules added by user */
#define UCI_QUERY_NO_OF_FIREWALL_CONFIGURED "qcmap_firewall.@firewall[0].no_of_rules"

/** uci command to get current wlan bootup setting */
#define UCI_QUERY_WLAN_BOOTUP_SETTING "qcmap_lan.@no_of_configs[0].wlan_bootup_enable"

/** uci command to get wlan mode */
#define UCI_QUERY_WLAN_CONFIG_MODE "qcmap_wlan.@wlanconfig[0].mode"

/** TCP Protocol string */
#define TCP_PROTO  "tcp"

/** UDP Protocol string */
#define UDP_PROTO  "udp"

/** ICMP Protocol string */
#define ICMP_PROTO  "icmp"

/** TCP_UDP Protocol string */
#define TCP_UDP_PROTO  "tcpudp"

/** ESP Protocol string */
#define ESP_PROTO  "esp"

/** Static Protocol string */
#define STATIC_PROTO  "static"

/** DHCP Protocol string */
#define DHCP_PROTO  "dhcp"

/** uci command to get number of profiles*/
#define UCI_QUERY_NO_OF_PROFILES "qcmap_lan.@no_of_configs[0].no_of_profiles"

/** uci command to get number of vlans*/
#define UCI_QUERY_NO_OF_VLANS "qcmap_lan.@no_of_configs[0].no_of_vlans"

/** Set firewall macro */
#define SET_FIREWALL "set_firewall"

/** Display firewall macro */
#define DISPLAY_FIREWALL "display_firewall"

/** Delete firewall macro */
#define DELETE_FIREWALL "delete_firewall"

/** Enable firewall macro */
#define ENABLE_FIREWALL "enable_firewall"

/** Disable firewall macro */
#define DISABLE_FIREWALL "disable_firewall"

/** Target CONNMARK String */
#define TARGET_CONNMARK "CONNMARK"

/** IPV4 string */
#define IP_V4_STRING "ipv4"

/** IPV6 string */
#define IP_V6_STRING "ipv6"

/** uci command for current ip passthrough feature mode */
#define QCMAP_IP_PT_FEATURE_MODE "qcmap_lan.@no_of_configs[0].with_nat"

/** uci command for Firewall Support for ip passthrough without NAT feature mode */
#define QCMAP_IPPT_WO_NAT_FW_SUPPORT "qcmap_lan.@no_of_configs[0].ippt_wo_nat_firewall_support"

/** uci command for current eth pdu feature mode */
#define QCMAP_ETH_PDU_FEATURE_MODE "qcmap_lan.@eth_pdu[0].enable"

/** uci command for prefix delegation mode */
#define QCMAP_QUERY_IPV6_PD_MODE "qcmap_lan.@lan[0].qcmap_prefix_delegation_mode"

/** uci set command */
#define UCI_SET_COMMAND "/etc/data/uci_ex.sh set"

/** MAC Null String */
#define MAC_NULL_STR "00:00:00:00:00:00"

/** add bridge context string */
#define BRIDGE_CONTEXT "bridge_context"

/** UCI command to query bridge_context */
#define QCMAP_QUERY_BRIDGE_CONTEXT "qcmap_lan.@no_of_configs[0].bridge_context"

/** dnsmasq reload command */
#define DNSMASQ_RELOAD_COMMAND "/etc/init.d/dnsmasq reload"

/** dnsmasq reload command */
#define ODHCPD_RELOAD_COMMAND "/etc/init.d/odhcpd reload"

/** network wan string */
#define NETWORK_WAN "network.wan5g"

/** network lan string */
#define NETWORK_LAN "network.lan"

/** uci commit */
#define UCI_COMMIT "/etc/data/uci_ex.sh commit"

/** Set_lan_config */
#define SET_LAN_CONFIG "setlanconfig"

/** lan ip */
#define LAN_IP "ipaddr"

/** lan subnet_mask */
#define LAN_NETMASK "netmask"

/** dhcp ignore option */
#define DHCP_IGNORE "ignore"

/** dhcp start option */
#define DHCP_START "start"

/** dhcp limit option */
#define DHCP_LIMIT "limit"

/** dhcp lease option */
#define DHCP_LEASETIME "leasetime"

/** Network reload */
#define NETWORK_RELOAD "/etc/init.d/network reload"

/** Ubus call network reload */
#define UBUS_CALL_NETWORK_RELOAD "ubus call network reload"

/** BRCTL SHOW */
#define BRCTL_SHOW "brctl show"

/** QUERY to grep count*/
#define QUERY_GREP_COUNT "| grep -c "

/** ETH interface */
#define ETH_INTERFACE "eth"

/** ETHNIC2 interface */
#define ETHNIC2_INTERFACE "eth_nic2"

/** RNDIS interface */
#define RNDIS_INTERFACE "rndis"

/** ECM interface */
#define ECM_INTERFACE "ecm"

/** BT interface */
#define BT_INTERFACE "bt-pan"

/** PHY IFACE NAMES  */
#define PHY_IFACE_NAMES "phy_iface_names"

/** PHY IFACE WIFI DEV NAMES  */
#define PHY_IFACE_WIFI_DEV_NAME "ippt_phy_wifi_dev"

/** To get CDT value from UCI */
#define CDT "cdt"

/** 3xWKK model name string */
#define CPE_WKK_V1_STR "Qualcomm Technologies, Inc. SDXPINN IDP CPE"

/** 2xWKK model name string */
#define CPE_WKK_V2_STR "Qualcomm Technologies, Inc. SDXPINN IDP CPE V2"

/** ETH Restart AutoNeg */
#define ETH_RESTART "ethtool -r"

/** STOP USB */
#define STOP_USB "/sbin/start_usb stop"

/** START USB */
#define START_USB "/sbin/start_usb start"

/** RESTART USB */
#define RESTART_USB "nohup /sbin/start_usb restart </dev/null >/dev/null 2>&1"

/** Add DHCP reservation macro */
#define ADD_DHCP_RESERVATION "add_dhcp_reservation"

/** Edit DHCP reservation macro */
#define EDIT_DHCP_RESERVATION "edit_dhcp_reservation"

/** Delete DHCP reservation macro */
#define DELETE_DHCP_RESERVATION "delete_dhcp_reservation"

#define WLAN_SET_MODE "set_wlan_mode"

/* worst case 12 sec*/
#define QCMAP_HOSTAPD_START_POLL_MAX_COUNT 800

/* 15 ms */
#define QCMAP_HOSTAPD_POLL_DELAY_MS  15000

/* ACTIVATE HOSTAPD SUCCESS */
#define ACTIVATE_HOSTAPD_SUCCESS  "Success"

/* ACTIVATE SUPPLICANT SUCCESS */
#define ACTIVATE_SUPPLICANT_SUCCESS  "Success"

/** Path for v4 conntrack entries */
#define CONNTRACK_ENTRIES "/tmp/data/conntrack_entries.txt"

/** Perform dnsmasq restart */
#define DNSMASQ_RESTART_COMMAND "/etc/init.d/dnsmasq restart"

/** USB string */
#define USB_PHY "usb"

/** ETH0 string */
#define ETH_PHY "eth0"

/** ETH_NIC2 string */
#define ETH_NIC2_PHY "eth1"

/** check address conflict */
#define QCMAP_CHECK_ADDRESS_CONFLICT "ip_collision_check_address_conflict"

/** check address conflict from bridge id*/
#define QCMAP_CHECK_ADDRESS_CONFLICT_FROM_BRIDGE_ID "ip_collision_check_address_conflict_from_bridge_id"

/** check address conflict */
#define QCMAP_SET_IP_COLLISION_STATE "ip_collision_state"

/** Any(FCD) string */
#define ALL_LINKS "any"

#define ADD_DHCP_FAILED "Add DHCP Reservation Failed"

/** Query default pdn from qcmap_lan db */
#define QCMAP_GET_DEFUALT_PDN "qcmap_lan.@no_of_configs[0].default_pdn"

/** Stop mwan3 */
#define MWAN3_STOP "mwan3 stop &"

/** Start mwan3 */
#define MWAN3_START "mwan3 start &"

/** Query default pdn from qcmap_lan db */
#define QCMAP_GET_IPPT_PDN_COUNT "qcmap_lan.@no_of_configs[0].ippt_pdn_count"

/** Get default pdn from qcmap_lan db */
#define QCMAP_GET_DEFAULT_PROFILE_ID "/etc/data/uci_ex.sh get qcmap_lan.@no_of_configs[0].default_pdn"

/*Update Default profile */
#define UPDATE_DEFAULT_PROFILE "update_default_profile"

/** dhcp v6 dad forward */
#define QCMAP_IPV6_DAD_FORWARD "dhcp.odhcpd.dad_forward"

/** Get pd activated config from qcmap_lan */
#define QCMAP_GET_PD_ACTIVATED_CONFIG "/etc/data/uci_ex.sh get qcmap_lan.@lan[0].qcmap_prefix_delegation_activated"

/** Get qcmap ext router mode enabled config from qcmap_lan */
#define QCMAP_GET_PD_EXT_ROUTER_MODE_ENABLED_CONFIG "/etc/data/uci_ex.sh get qcmap_lan.@lan[0].qcmap_ext_router_mode_enabled"

/** Get pd available config from qcmap_lan */
#define QCMAP_GET_PD_AVAILABLE_CONFIG "/etc/data/uci_ex.sh get qcmap_lan.@lan[0].qcmap_delegated_prefix_available"

/** @} */ /* end_addtogroup qcmap_lan_strings */

/** @addtogroup qcmap_lan_file_path
@{ */
/** Shell script to trigger Netifd */
#define RMNET_UPDATE_FILE "/lib/netifd/rmnet_update.sh"

#define RMNET_ETH_UPDATE_FILE "/lib/netifd/rmneteth_update.sh"

#define SET_LAN_CONFIG_ON_ACTIVATE_LAN "SetLANConfigOnLanActivation"

/** config file path */
#define CONFIG_FILE_PATH "/tmp/"

/** VLAN config file path*/
#define VLAN_CONFIG "/etc/data/lan_config.sh"

/** GET IPA OFFLOAD STATUS*/
#define GET_IPA_OFFLOAD_STATUS "get_ipa_offload_status"

/** VLAN-PDN Map file path*/
#define BACKHAUL_WWAN_CONFIG_FILE "/etc/data/backhaulWWANConfig.sh"

/** Shell script to add NAT Rules */
#define NAT_ALG_VPN_CONFIG_FILE "/etc/data/nat_alg_vpn_config.sh"

/** Shell script to set and add Firewall Rules */
#define FIREWALL_CONFIG_FILE "/etc/data/firewallConfig.sh"

/** Shell script to tethering config */
#define TETHERING_CONFIG_FILE "/etc/data/tethering.sh"

/** QCMAP LAN ENABLE PD ACTIVATED */
#define QCMAP_LAN_ENABLE_PD_ACTIVATED "qcmap_lan_enable_pd_activated"

/** QCMAP LAN ENABLE EXT ROUTER MODE */
#define QCMAP_LAN_ENABLE_EXT_ROUTER_MODE "qcmap_lan_enable_ext_router_mode"

/** ETH LAN router mode*/
#define LAN_ROUTER "LAN ROUTER"

/** ETH WAN router mode*/
#define WAN_ROUTER "WAN ROUTER"

/** ETH WAN_LAN router mode*/
#define WAN_LAN_ROUTER "WAN_LAN ROUTER"

/** Set eth config*/
#define SET_ETH_CONFIG "set_eth_config"

/** Set eth type*/
#define SET_ETH_TYPE "set_eth_type"

/** Enable macsec*/
#define ENABLE_MACSEC "enable_macsec"

/** Disable macsec*/
#define DISABLE_MACSEC "disable_macsec"

/** Start macsec*/
#define START_MACSEC "start_macsec"

/** Stop macsec*/
#define STOP_MACSEC "stop_macsec"

/** Add macsec to bridge*/
#define ADD_MACSEC "add_macsec"

/** Remove macsec from bridge*/
#define DELETE_MACSEC "delete_macsec"

/** Shell script path for WLAN config */
#define WLAN_CONFIG_FILE "/etc/data/wlanConfig.sh"

#define IPv6_CONNTRACK_FILTER_PATH "/tmp/data/v6conntrack.txt"
/** Shell script path for WLAN config */
#define FACTORY_RESET_CONFIG_FILE "/etc/data/restoreFactoryConfig.sh"

/** Shell script path for BT config */
#define BT_CONFIG_FILE "/etc/data/bt_tethering.sh"

/** Set bt bring up config*/
#define BT_BRING_UP "bring_up_bt"

/** Set bt bring down config*/
#define BT_BRING_DOWN "bring_down_bt"

/** Shell script to add NAT Rules */
#define BACKHAUL_COMMMON_CONFIG_FILE "/etc/data/backhaulCommonConfig.sh"

/** Set backahaul prefrences */
#define SET_BACKHAUL_PREF "set_bh_pref"

/** UCI command to get backhaul prefrences. */
#define UCI_GET_BH_PREF "/etc/data/uci_ex.sh get mwan3.backhaul_pref.use_member"

/** Update DNS Search List*/
#define UPDATE_DNSSL "util_update_dns_search_list"

/** Shell script path for GSB config */
#define GSB_CONFIG_FILE "/etc/data/gsbConfig.sh"

#define BACKHAUL_WWAN_NAME "wan5g"
#define BACKHAUL_WLAN_NAME "wanwlan"
#define BACKHAUL_ETH_NAME  "waneth"
#define BACKHAUL_USB_NAME  "wanusb"
#define BACKHAUL_BT_NAME   "wanbt"

/** Shell script path for tinyproxy config */
#define TINYPROXY_CONFIG_FILE "/etc/data/tinyproxyConfig.sh"

/** Enable tinyproxy config*/
#define ENABLE_TINYPROXY "enable_tinyproxy"

/** Disable tinyproxy config*/
#define DISABLE_TINYPROXY "disable_tinyproxy"

/** Setup tinyproxy config*/
#define SETUP_TINYPROXY "setup_tinyproxy"

/** Stop tinyproxy config*/
#define STOP_TINYPROXY "stop_tinyproxy"

/** Get tinyproxy status*/
#define GET_TINYPROXY_STATUS "get_tingproxy_status"

/** Enable rtsp alg*/
#define CMD_ENABLE_RTSP_ALG "enable_rtsp_alg"

/** Disable rtsp alg*/
#define CMD_DISABLE_RTSP_ALG "disable_rtsp_alg"

/** Enable rtsp alg*/
#define CMD_ENABLE_SIP_ALG "enable_sip_alg"

/** Disable sip alg*/
#define CMD_DISABLE_SIP_ALG "disable_sip_alg"

/** Set sip server*/
#define CMD_SET_SIP_SERVER "set_sip_server"

/** Update network sip server*/
#define CMD_UPDATE_SIP_SERVER "update_network_sip_server"

/** uci command to get tinyproxy status */
#define UCI_QUERY_TINYPROXY_STATUS "qcmap_lan.@no_of_configs[0].enable_tinyproxy"

/** IPPT shell script */
#define IPPT_FILE "/etc/data/ippt.sh"

/** IP Collision shell script */
#define IP_COLLISION_FILE "/etc/data/ip_collision.sh"

/** Lan Util shell script */
#define LAN_UTIL_SCRIPT "/etc/data/lanUtils.sh"

/** Dhcp reload pd option file */
#define DHCP_RELOAD_PD_OPTION_FILE "/etc/data/dhcp_reload_pd_option.sh"

/** Enable pd_manager */
#define QCMAP_LAN_ENABLE_PD_MANAGER "qcmap_lan_enable_pd_manager"

/** Enable dhcp pd_config */
#define DHCP_ENABLE_PD_CONFIG "dhcp_enable_pd_config"

/** Get backhaul file */
#define GET_BACKHAUL_FILE "get_backhaul_file"

#define ACTIVATE_DHCP_RECORDS_ON_ACTIVATE_LAN "ActivateDHCPRecordsonLanActivation"

#define RESET_FIREWALL_ON_IPPT_WO_NAT "reset_firewall_on_ippt_wo_nat_mode_is_set_on_boot_up"

/** @} */ /* end_addtogroup qcmap_lan_file_path */

/** @addtogroup qcmap_lan_file_name
@{ */
/** IPV4 config file name */
#define IPV4_CONFIG_FILE_NAME "ipv4config"

/** IPV6 config file name */
#define IPV6_CONFIG_FILE_NAME "ipv6config"


/** Resolv file path */
#define RESOLV_PATH "/tmp/resolv.conf"

/** ETHPDU config file name */
#define ETHPDU_CONFIG_FILE_NAME "ethpduconfig"

#define CHECK_EXTERNAL_IPV4_CONNTARCK "check_external_ipv4_conntracks"

/** DHCP Vendor Info config */
#define AP_MODE  "ap"

#define DEFAULT_SSID  "QSoftAP_5G"

#define DNSMASQ_CONFIG_FILE               "/var/etc/dnsmasq.conf.lan_dns"

/** uci del command */
#define UCI_DEL_COMMAND "/etc/data/uci_ex.sh del_list"

/** Function name to increase/decrease restart_link_count while toggling link */
#define INC_DEC_RESTART_LINK_COUNT "util_inc_dec_restart_link_count"

#define INCREMENT 1

#define ETH_LINK_DETECTION "ethtool %s | grep -i 'Link detected' | awk -F ': ' '{print $2}'"

#define LINK_DETECTED "yes"

/** @} */ /* end_addtogroup qcmap_lan_file_name */

/** add MTU options for downstream*/
#define QCMAP_WWAN_ADD_MTU_OPTIONS "util_add_mtu_options"

/** del MTU options for downstream*/
#define QCMAP_WWAN_DELETE_MTU_OPTIONS "util_delete_mtu_options"

/** update MTU for TCP MSS*/
#define QCMAP_WWAN_UPDATE_MTU "util_update_mtu"

/** add DNSv6 options for downstream*/
#define QCMAP_LAN_ADD_DNSV6_OPTIONS "util_add_dnsv6_options"

/** del DNSv6 options for downstream*/
#define QCMAP_LAN_DEL_DNSV6_OPTIONS "util_del_dnsv6_options"

#define DELETE_DHCP_LEASE_ENTRY "delete_dhcp_lease_entry"

#define ETH_PDU_INVALID_DEVICE_INDEX 255

#define IPV4_CONVERT_NETWORK_ADDRESS_TO_STRING(ipv4_nw_address, ipv4_str)           \
          {                                                                         \
            struct sockaddr_in              ipv4_addr;                              \
            ipv4_addr.sin_addr.s_addr = ipv4_nw_address;                            \
            inet_ntop(AF_INET, &(ipv4_addr.sin_addr), ipv4_str, INET_ADDRSTRLEN);   \
          }

#define IPV6_CONVERT_NETWORK_ADDRESS_TO_STRING(ipv6_nw_address, ipv6_str)            \
          {                                                                          \
            struct sockaddr_in6             ipv6_addr;                               \
            memcpy(ipv6_addr.sin6_addr.s6_addr, ipv6_nw_address,                     \
                       sizeof(ipv6_addr.sin6_addr.s6_addr));                         \
            inet_ntop(AF_INET6, &(ipv6_addr.sin6_addr), ipv6_str, INET6_ADDRSTRLEN); \
          }

//char result[QCMAP_MAX_SCAN_SIZE] = {0};
//in_addr addr;

/* Macro to get uci need define a char array with name result,  */
#define UCI_GET_STR_OPTION(retValue, uciFileType, sectionName, i, optionName) \
        { \
          memset(cmd, 0, QCMAP_MAX_SCAN_SIZE); \
          if (!UciGetUtility(owrt_filename[uciFileType], sectionName, i, optionName, cmd, 0)) \
          {\
            LOG_MSG_ERROR("Failed to UCI GET %s/%s",sectionName,optionName,0);\
            return false;\
          }\
          strlcpy(retValue, cmd, strlen(cmd));\
        }

/* Macro to get uci need define a char array with name result,  */
#define UCI_GET_INT_OPTION(retValue, uciFileType, sectionName, i, optionName) \
        { \
          memset(cmd, 0, QCMAP_MAX_SCAN_SIZE); \
          if (!UciGetUtility(owrt_filename[uciFileType], sectionName, i, optionName, cmd, 0)) \
          {\
            LOG_MSG_ERROR("Failed to UCI GET %s/%s",sectionName,optionName,0);\
            return false;\
          }\
          retValue = atoi(cmd);\
        }

/* the maco caller need define a char array with name result, and a "in_addr addr" variable*/
#define UCI_GET_ADDR_OPTION(retValue, uciFileType, sectionName, i, optionName) \
          { \
            memset(cmd, 0, QCMAP_MAX_SCAN_SIZE); \
            if (!UciGetUtility(owrt_filename[uciFileType], sectionName, i, optionName, cmd, 0)) \
            {\
              LOG_MSG_ERROR("Failed to UCI GET %s.%s",sectionName,optionName,0);\
              return false;\
            }\
            memset(&addr,0,sizeof(in_addr));\
            if (inet_aton(cmd, &addr)) {\
                retValue = ntohl(addr.s_addr);\
            }\
          }
/** @} */

#define IN6_IS_ADDR_UNSPECIFIED_32(a)  \
       ((a[0] == 0) &&  \
        (a[1] == 0) &&  \
        (a[2] == 0) &&  \
        (a[3] == 0))

/*MAX FIREWALL ENTRY*/
#define MAX_FIREWALL_ENTRY                   128

#define INET6_ADDRSTRLEN                     46    /* INET6 address string*/

/*QCMAP_VALIDATE_BAND_INFO*/
#define QCMAP_VALIDATE_BAND_INFO(band)(band == 2 || band == 5 || band == 6 ? true : false)

/*ETH Mode*/
#define QCMAP_ETH_MODE_LAN 0
#define QCMAP_ETH_MODE_WAN 1

/*---------------------------------------------------------------------------
  Return values indicating error status
---------------------------------------------------------------------------*/
#define QCMAP_CM_SUCCESS               0         /* Successful operation   */
#define QCMAP_CM_ERROR                -1         /* Unsuccessful operation */
#define TRUE                           1

/*---------------------------------------------------------------------------
  Logging definitions
---------------------------------------------------------------------------*/
#define QCMAP_LOG(...)                         \
 LOG_MSG_INFO1( "%s %d:", __FILE__, __LINE__,0); \
 LOG_MSG_INFO1( __VA_ARGS__ ,0,0);

#define QCMAP_LOG_FUNC_ENTRY()  \
 QCMAP_LOG                   \
(                              \
       "Entering function %s\n",  \
       __FUNCTION__               \
)

#define QCMAP_LOG_FUNC_EXIT()   \
 QCMAP_LOG                   \
(                              \
       "Exiting function %s\n",   \
       __FUNCTION__ \
)

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef __cplusplus
}
#endif

/** IPsec script   */
#define QCMAP_IPSEC_SCRIPT "/etc/data/QCMAP_ipsec.sh"

/** Set ipsec tunnel   */
#define SET_IPSEC_CONFIG "setIpsecTunnel"

/** Activate ipsec tunnel   */
#define ACTIVATE_IPSEC_TUNNEL "activateIpsecTunnel"

/** Delete ipsec tunnel   */
#define DELETE_IPSEC_TUNNEL "deleteIpsecTunnel"

/** Get ipsec tunnel status   */
#define GET_IPSEC_TUNNEL_STATUS "getIpsecTunnelStatus"

/** IPsec enable option   */
#define IPSEC_ENABLE_OPTION "qcmap_lan.@global[0].ipsec_enable"

/** IPsec active tunnels*/
#define IPSEC_ACTIVE_TUNNEL "qcmap_lan.@global[0].ipsec_active_tunnels"

/** IPsec init file stop  */
#define IPSEC_INIT_STOP "/etc/init.d/ipsec stop"

/** IPsec init file disable  */
#define IPSEC_INIT_DISABLE "/etc/init.d/ipsec disable"

/* Invalid Profile Handle */
#define INVALID_PROFILE_HANDLE   0

/* Invalid Profile Handle */
#define INVALID_BRIDGE_ID   -1

/** SetIPV4NAT Config  */
#define SET_V4_NAT_CONFIG "Setv4NATConfig"

/** @} */ /* end_addtogroup qcmap_lan_macros */


/** @addtogroup qcmap_lan_datatypes
@{ */


#ifndef NS_IN6ADDRSZ
#define NS_IN6ADDRSZ sizeof(struct in6_addr)
#endif

#ifndef NS_INADDRSZ
#define NS_INADDRSZ sizeof(struct in_addr)
#endif

#ifndef NS_INT16SZ
#define NS_INT16SZ 2
#endif


/** QCMAP interface type enum */
typedef enum {
  QCMAP_INTERFACE_TYPE_ENUM_MIN_ENUM_VAL = -2147483647, /**< To force a 32 bit signed enum.  Do not change or use*/
  QCMAP_INTERFACE_TYPE_WLAN = 0x01, /**<  Interface type is WLAN  */
  QCMAP_INTERFACE_TYPE_ETH = 0x02, /**<  Interface type is Ethernet  */
  QCMAP_INTERFACE_TYPE_ECM = 0x03, /**<  Interface type is ECM  */
  QCMAP_INTERFACE_TYPE_RNDIS = 0x04, /**<  Interface type is RNDIS  */
  QCMAP_INTERFACE_TYPE_MHI = 0x05, /**<  Interface type is MHI  */
  QCMAP_INTERFACE_TYPE_ETH_NIC2 = 0x06, /**<  Interface type is Ethernet with NIC2 */
  QCMAP_INTERFACE_TYPE_ENUM_MAX_ENUM_VAL = 2147483647 /**< To force a 32 bit signed enum.  Do not change or use*/
}qcmap_interface_type_enum;

/** QCMAP device type enum */
typedef enum{

  QCMAP_LAN_DEVICE_TYPE_NONE = -1,
  /**< Device type is NONE */

  QCMAP_LAN_DEVICE_TYPE_USB = 0x03,
  /**<  Device type is USB */

  QCMAP_LAN_DEVICE_TYPE_ETHERNET = 0x04,
  /**<  Device type is ETHERNET */

  QCMAP_LAN_DEVICE_TYPE_ANY_AP = 0x08,
  /**<  Device type is Any AP(WiFi) */

  QCMAP_LAN_DEVICE_TYPE_ANY = 0x09,
  /**<  Device type is Any(FCD) */

  QCMAP_LAN_DEVICE_TYPE_ETHERNET_NIC2 = 0x10,
  /**<  Device type is ETHERNET with NIC2 */

  QCMAP_LAN_DEVICE_TYPE_ALL_AP = 0x17
  /**<  Device type is All AP(WiFi) */

}qcmap_lan_device_type_enum;

/** IP Passthrough feature mode enum */
typedef enum {

  IP_PASSTHROUGH_MODE_WITH_NAT = 0x01,
  /**<  IP Passthrough feature mode with NAT. Default when not set */

  IP_PASSTHROUGH_MODE_WITHOUT_NAT = 0x02
  /**< IP Passthrough feature mode without NAT */
}qcmap_lan_ip_passthrough_feature_mode_enum;

/** ETH PDU feature mode enum */
typedef enum {

  ETH_PDU_MODE_DISABLE = 0x00,
  /**<  IP Passthrough feature mode with NAT. Default when not set */

  ETH_PDU_MODE_ENABLE = 0x01
  /**< IP Passthrough feature mode without NAT */
}qcmap_lan_eth_pdu_feature_mode_enum;


/** QCMAP IP Passthrough mode enum */
typedef enum{

  QCMAP_LAN_IP_PASSTHROUGH_MODE_DOWN = 0x00,
  /**<  IP Passthrough mode is down */

  QCMAP_LAN_IP_PASSTHROUGH_MODE_UP = 0x01
  /**<  IP Passthrough mode is up */
}qcmap_lan_ip_passthrough_mode_enum;

/** Data type for QCMAP Backhaul Enable types */
typedef enum {

  SETUP = 0x0001,
  /**<   Backhaul is v4 */

  RECONFIG = 0x0002,
  /**<   Backhaul is v6 */

} qcmap_backhaul_enable_type;

/** Data type for QCMAP Backhaul types */
typedef enum {

  BACKHAUL_V4 = 0x0001,
  /**<   Backhaul is v4 */

  BACKHAUL_V6 = 0x0002,
  /**<   Backhaul is v6 */

  BACKHAUL_ETHPDU = 0x0003
  /**<   Backhaul is ETHPDU */
} qcmap_backhaul_type;

/** Data types for protocol types */
typedef enum
{

   ICMP = 1,
   /**< ICMP protocol */

   TCP = 6,
   /**< TCP Protocol */

   UDP = 17,
   /**< UDP Protocol */

   TCP_UDP = 253
  /**< TCP UDP Protocol */

 }  qcmap_protocol_enum_type;

/** Data type for NAT Types */
typedef enum {
  NAT_SYMMETRIC = 0,
  NAT_PORT_RESTRICTED_CONE,
  NAT_FULL_CONE,
  NAT_ADDRESS_RESTRICTED_CONE
}nat_type_enum_t;

/** Error types for indicating type of errors */

typedef enum
{
   ERR_NONE = 0x0000,
   /**< No error */
 } error_type;

 /* Ethernet mode */
typedef enum {

  /**<  ETH mode LAN router */
  QCMAP_ETHERNET_LAN_ROUTER = 0x00,

  /**<   ETH mode WAN router  */
  QCMAP_ETHERNET_WAN_ROUTER = 0x01,

  /**<   ETH mode WAN_LAN router  */
  QCMAP_ETHERNET_WAN_LAN_ROUTER = 0x02,

}qcmap_ethernet_mode;

 /* BT mode */
typedef enum {

  /**< To force a 32 bit signed enum.  Do not change or use*/
  QCMAP_BT_MIN_ENUM_VAL = -2147483647,

  /**<  BT mode LAN router */
  QCMAP_BT_LAN_ROUTER = 0x00,

  /**<   BT mode WAN router  */
  QCMAP_BT_WAN_ROUTER = 0x01,

  /**< To force a 32 bit signed enum.  Do not change or use*/
  QCMAP_BT_MAX_ENUM_VAL = 2147483647

}qcmap_bt_mode;

 /* BT status */
typedef enum {

  /**<  BT mode LAN router */
  QCMAP_BT_TETHERING_MODE_DOWN = 0x01,

  /**<   BT mode WAN router  */
  QCMAP_BT_TETHERING_MODE_UP = 0x02,

}qcmap_bt_tethering_status;

typedef enum {

  /**<   Ethernet mode LAN type.*/
  QCMAP_ETHERNET_LAN_TYPE = 0x00,

  /**<   Ethernet mode WAN type.*/
  QCMAP_ETHERNET_WAN_TYPE = 0x01

}qcmap_eth_network_type;

typedef enum {
  /**< MACSEC state disabled*/
  QCMAP_MSGR_CONFIG_DISABLE = 0x00,

  /**< MACSEC state enabled*/
  QCMAP_MSGR_CONFIG_ENABLE = 0x01,

  /**< MACSEC state restart*/
  QCMAP_MSGR_CONFIG_RESTART = 0x02

}qcmap_config_state_enum;

typedef enum {
  /*MACSEC mode supplicant*/
  QCMAP_MSGR_MACSEC_MODE_SUPPLICANT = 0x01,

  /*MACSEC mode authenticator */
  QCMAP_MSGR_MACSEC_MODE_AUTHENTICATOR = 0x02

}qcmap_macsec_mode_enum;


typedef enum {
  QCMAP_WLAN_DEV_INVALID = 0x01, /**<  WLAN device is invalid  */
  QCMAP_WLAN_DEV_HMT = 0x02, /**<  WLAN device is HMT  */
  QCMAP_WLAN_DEV_WKK = 0x03, /**<  WLAN device is WKK  */
}qcmap_wlan_dev_enum;

typedef enum {
  QCMAP_WLAN_IFACE_PRIMARY_AP = 0x00, /**<  Primary AP iface  */
  QCMAP_WLAN_IFACE_STATION = 0x01, /**<  Station iface  */
  QCMAP_WLAN_IFACE_GUEST_AP = 0x02, /**<  Guest AP 1 index  */
}qcmap_wlan_iface_index_enum;

typedef enum {
  QCMAP_WLAN_IFACE_INACTIVE = 0x00, /**<  IF is inactive */
  QCMAP_WLAN_IFACE_ACTIVE = 0x01, /**<  IF is active  */
}qcmap_wlan_iface_active_state_enum;

typedef enum {
  QCMAP_STA_CONNECTION_DYNAMIC = 0x01, /**<  Dynamic  */
  QCMAP_STA_CONNECTION_STATIC = 0x02 /**<  Static  */
}qcmap_sta_connection_enum;

typedef enum {
  QCMAP_STA_CONNECTITED = 0x01, /**<  STA Connected  */
  QCMAP_STA_DISCONNECTED = 0x02 /**<  STA disconnected  */
}qcmap_sta_status_enum;

typedef enum {
  QCMAP_NAT_TIMEOUT_ENUM_MIN_ENUM_VAL = -2147483647, /**< To force a 32 bit signed enum.  Do not change or use*/
  QCMAP_NAT_TIMEOUT_GENERIC = 0x01, /**<   Generic NAT timeout  */
  QCMAP_NAT_TIMEOUT_ICMP = 0x02, /**<   NAT timeout for ICMP  */
  QCMAP_NAT_TIMEOUT_TCP_ESTABLISHED = 0x03, /**<   NAT timeout for the TCP established  */
  QCMAP_NAT_TIMEOUT_UDP = 0x04, /**<   NAT timeout for UDP  */
  QCMAP_NAT_TIMEOUT_UDP_STREAM = 0x05, /**<   NAT timeout for UDP stream  */
  QCMAP_NAT_TIMEOUT_ICMPV6 = 0x06, /**<   NAT timeout for ICMPv6  */
  QCMAP_NAT_TIMEOUT_ENUM_MAX_ENUM_VAL = 2147483647 /**< To force a 32 bit signed enum.  Do not change or use*/
}qcmap_nat_timeout_enum;

typedef enum {
  QCMAP_ENABLE_ALG = 0, /**<  Enable ALG  */
  QCMAP_DISABLE_ALG = 1, /**<  Disable ALG  */
}qcmap_alg_action_enum;

typedef enum {
  QCMAP_RTSP_ALG_DISABLED = 0, /**<  RTSP ALG disabled  */
  QCMAP_RTSP_ALG_ENABLED = 1, /**<  RTSP ALG enabled  */
}qcmap_rtsp_alg_enum;

typedef enum {
  QCMAP_SIP_ALG_DISABLED = 0, /**<  SIP ALG disabled  */
  QCMAP_SIP_ALG_ENABLED = 1, /**<  SIP ALG enabled  */
}qcmap_sip_alg_enum;

typedef enum {
  QCMAP_CDT_UNKNOWN = 0, /**<  Invalid Value */
  QCMAP_CDT_HMT = 1,     /**<  HMT CDT */
  QCMAP_CDT_WKK = 2,     /**<  WKK CDT */
}qcmap_cdt_enum;

typedef enum {
  QCMAP_CPE_WKK_UNKNOWN = 0, /**<  Invalid Value */
  QCMAP_CPE_WKK_V1 = 1,     /**<  3xWKK CPE */
  QCMAP_CPE_WKK_V2 = 2,     /**<  2xWKK CPE */
}qcmap_cpe_wkk_enum;

typedef struct
{
  char dns_search_name[QCMAP_DOMAIN_NAME_MAX_V01];
  /**< Individual DNS Search string name */
} dns_seach_list_info;

/** this enum is used for IP type */
typedef enum {
  QCMAP_IP_FAMILY_INVALID = 0x00, /**<  invalid version  */
  QCMAP_IP_FAMILY_V4 = 0x04, /**<  IPv4 version  */
  QCMAP_IP_FAMILY_V6 = 0x06, /**<  IPv6 version  */
  QCMAP_IP_FAMILY_V4V6 = 0x0A, /**<  Dual mode version  */
  QCMAP_IP_FAMILY_ETH = 0x0C /**<  Eth mode version  */
}qcmap_ip_family_enum;

/**  qcmap wwan backhaul info */
typedef struct {

  uint32_t profile_handle;
  /**<   QCMAP Profile Handle. */

  char iface_name[QCMAP_MAX_IFACE_NAME_SIZE];
  /**<   WWAN interface on which backhaul is established */

  uint32_t v4_addr;
  /**<   Public IPv4 Address */

  uint32_t v4_gw_addr;
  /**<   IPv4 Gateway Address */

  uint32_t v4_pri_dns_addr;
  /**<   IPv4 Primary DNS Address */

  uint32_t v4_sec_dns_addr;
  /**<   IPv4 Secondary DNS Address */

  uint32_t v4_addr_subnet_mask;
  /**<   IPv4 subnet mask */

  uint8_t v6_addr[QCMAP_IPV6_ADDR_LEN];
  /**<   Public IPv6 Address */

  uint8_t v6_gw_addr[QCMAP_IPV6_ADDR_LEN];
  /**<   IPv6 Gateway Address */

  uint8_t v6_pri_dns_addr[QCMAP_IPV6_ADDR_LEN];
  /**<   IPv6 Primary DNS Address */

  uint8_t v6_sec_dns_addr[QCMAP_IPV6_ADDR_LEN];
  /**<   IPv6 Secondary DNS Address */

  uint8_t v6_addr_prefix_len;
  /**<   IPv6 Prefix length */

  uint32_t dns_search_list_len;
  /**<   DNS list len */

  dns_seach_list_info dns_search_list[QCMAP_MAX_NUM_DNS_SEARCH_LIST];
  /**<   DNS Search List */


  uint16_t  v4_mtu;
  /**<   v4 wwan mtu */

  uint16_t  v6_mtu;
  /**<   v6 wwan mtu */

  uint16_t vlan_start;
  /**<   Vlan for eth pdu mapping start. */

  uint16_t vlan_end;
  /**<   Vlan for eth pdu mapping end. */

} qcmap_wwan_backhaul_info;

typedef struct {
  char private_ip_addr[QCMAP_IPV4_ADDR_LEN];
  /**<   Private IP address. */

  uint16_t private_port;
  /**<   Private port. */

  uint16_t global_port;
  /**<   Global port. */

  uint8_t protocol;
  /**<   Protocol. */

}qcmap_snat_config_t; /*Type*/

/** IPv6 address quadlet */
typedef struct {

  uint32_t quadlet1;
  /**< quadlet 1 */

  uint32_t quadlet2;
  /**< quadlet 2 */

  uint32_t quadlet3;
  /**< quadlet 3 */

  uint32_t quadlet4;
  /**< quadlet 4*/
} qcmap_ipv6_addr;

/** Vlan info to display configured VLANS */
typedef struct {

  char phy_iface_name[QCMAP_MAX_IFACE_NAME_SIZE];
  /**< interface name */

  qcmap_interface_type_enum intf_type;
  /**< interface type */

  uint16_t vlan_id;
  /**< VLAN ID */

  uint8_t is_accelerated;
  /**< ipa offload flag */
}qcmap_lan_vlan_conf_t;

/** Vlan to PDN mapping */
typedef struct {

  uint32_t profile_handle;
  /**<   Profile number to be used for PDN. */

  uint32_t vlan_id_len;
  /**< Must be set to # of elements in vlan_id */

  int16_t vlan_id[QCMAP_MAX_VLAN_PER_PDN];
  /**<   list of vlan ids mapped. */
}qcmap_pdn_to_vlan_mapping;

/** IP Passthrough feature mode */
typedef struct {

  bool ip_passthrough_feature_valid;
  /**< Identifies if ip passthough feature mode is set >*/

  qcmap_lan_ip_passthrough_feature_mode_enum ip_passthrough_feature_mode;
  /**< Identifies the ip passthrough feature mode >*/

  bool dhcp_lan_options_feature_valid;
  /**< Identifies if DHCP LAN options feature is set >*/

  qcmap_msgr_dhcp_lan_options_feature_mode_mask_v01 dhcp_lan_options_feature_modes;
  /**< Identifies the DHCP LAN options feature modes >*/

  /**< NOTE: add DHCP reservation/EoGRE feature mode entries here>*/
  bool eth_pdu_feature_valid;
  /**< Identifies if eth pdu feature mode is set >*/

  qcmap_lan_eth_pdu_feature_mode_enum eth_pdu_feature_mode;
  /**< Identifies the eth pdu feature mode >*/

  uint8_t eth_device_index;
  /**< Identifies eth device index >*/

  bool ipsec_feature_valid;
  /**< Identifies if ipsec feature mode is set >*/

  bool ipsec_feature_enable;
  /**< Identifies if ipsec feature mode >*/

}qcmap_lan_client_feature_mode_config;

/** IP Passthrough config */
typedef struct {

  qcmap_lan_device_type_enum device_type;
  /**< Identifies the IP Passthrough device type */

  uint8_t mac_addr[QCMAP_MAC_ADDR_LEN];
  /**< Identifies the device MAC address */

  char client_device_name[QCMAP_MAX_DEVICE_NAME];
   /**< Device name */
}qcmap_lan_ip_passthrough_config;

/** QCMAP IPSec VPN Passthrough mode enum */
typedef enum{

  QCMAP_LAN_IPSEC_VPN_PASSTHROUGH_MODE_DOWN = 0x00,
  /**<  IPSec VPN Passthrough mode is down */

  QCMAP_LAN_IPSEC_VPN_PASSTHROUGH_MODE_UP = 0x01
  /**<  IPSec VPN Passthrough mode is up */
}qcmap_lan_ipsec_vpn_passthrough_mode_enum;

/** QCMAP PPTP VPN Passthrough mode enum */
typedef enum{

  QCMAP_LAN_PPTP_VPN_PASSTHROUGH_MODE_DOWN = 0x00,
  /**<  PPTP VPN Passthrough mode is down */

  QCMAP_LAN_PPTP_VPN_PASSTHROUGH_MODE_UP = 0x01
  /**<  PPTP VPN Passthrough mode is up */
}qcmap_lan_pptp_vpn_passthrough_mode_enum;

/** QCMAP L2TP/IPSec VPN Passthrough mode enum */
typedef enum{

  QCMAP_LAN_L2TP_IPSEC_VPN_PASSTHROUGH_MODE_DOWN = 0x00,
  /**<  L2TP/IPSec VPN Passthrough mode is down */

  QCMAP_LAN_L2TP_IPSEC_VPN_PASSTHROUGH_MODE_UP = 0x01
  /**<  L2TP/IPSec VPN Passthrough mode is up */
}qcmap_lan_l2tp_ipsec_vpn_passthrough_mode_enum;

/** QCMAP IPV6 IPSec VPN Passthrough mode enum */
typedef enum{

  QCMAP_LAN_IPSEC_VPN_PASSTHROUGH_V6_MODE_DOWN = 0x00,
  /**<  IPV6 IPSec VPN Passthrough mode is down */

  QCMAP_LAN_IPSEC_VPN_PASSTHROUGH_V6_MODE_UP = 0x01
  /**<  IPV6 IPSec VPN Passthrough mode is up */
}qcmap_lan_ipsec_vpn_passthrough_v6_mode_enum;

/** QCMAP IPV6 PPTP VPN Passthrough mode enum */
typedef enum{

  QCMAP_LAN_PPTP_VPN_PASSTHROUGH_V6_MODE_DOWN = 0x00,
  /**<  IPV6 PPTP VPN Passthrough mode is down */

  QCMAP_LAN_PPTP_VPN_PASSTHROUGH_V6_MODE_UP = 0x01
  /**<  IPV6 PPTP VPN Passthrough mode is up */
}qcmap_lan_pptp_vpn_passthrough_v6_mode_enum;

/** QCMAP IPV6 L2TP/IPSec VPN Passthrough mode enum */
typedef enum{

  QCMAP_LAN_L2TP_IPSEC_VPN_PASSTHROUGH_V6_MODE_DOWN = 0x00,
  /**<  IPV6 L2TP/IPSec VPN Passthrough mode is down */

  QCMAP_LAN_L2TP_IPSEC_VPN_PASSTHROUGH_V6_MODE_UP = 0x01
  /**<  IPV6 L2TP/IPSec VPN Passthrough mode is up */
}qcmap_lan_l2tp_ipsec_vpn_passthrough_v6_mode_enum;

typedef struct {

  // TODO: Keeping this for now to avoid compilation errors from FTA.
  // Eventually we need to remove this
  /**<   Ethernet NIC configured interface name.*/
  char eth_iface_name[QCMAP_MAX_IFACE_NAME_SIZE];

  /**<   Ethernet NIC network type.*/
  qcmap_eth_network_type eth_nic_nw_type;

  /**<   Ethernet NIC configured interface type as either LAN or WAN.*/
  qcmap_interface_type_enum eth_nic_type;

}qcmap_eth_nic_config;

typedef struct {
  qcmap_config_state_enum state;

  char macsec_iface_name[QCMAP_MAX_IFACE_NAME_SIZE];

  qcmap_macsec_mode_enum macsec_mode;

  /*  mode on which macsec will have to run */
  char eth_nic_iface_name[QCMAP_MAX_IFACE_NAME_SIZE];

  /*  ethernet interface name to set macsec config  */
  uint32_t mtu_size;
}qcmap_macsec_nic_config;

/*Data type for ETH mode and NIC config*/
typedef struct
{
  /* Ethernet mode */
  qcmap_ethernet_mode          mode;

  /* No of NIC's configured */
  uint32_t                     no_of_nics;

  /* Validation flag for eth_nic_config */
  bool                         is_eth_nics_config_valid;

  /* ETH NIC config */
  qcmap_eth_nic_config         eth_nic_config[QCMAP_MAX_ETH_NICS];

  /* Validation flag for MACsec NIC config */
  bool                         is_macsec_nic_config_valid;

    /* Validation flag for MACsec NIC config */
  int                          no_of_macsec_nics;

  /* MACsec NIC Config */
  qcmap_macsec_nic_config      macsec_nic_config[QCMAP_MAX_ETH_NICS];

} qcmap_eth_config_t;


typedef enum {
  QCMAP_BACKHAUL_TYPE_ENUM_MIN_ENUM_VAL = -2147483647,
  /**< To force a 32 bit signed enum.  Do not change or use*/

  QCMAP_WWAN_BACKHAUL = 0x01,
  /**<  WWAN Backhaul  */

  QCMAP_USB_CRADLE_BACKHAUL = 0x02,
  /**<  Cradle  Backhaul  */

  QCMAP_WLAN_BACKHAUL = 0x03,
  /**<  WLAN  Backhaul  */

  QCMAP_ETHERNET_BACKHAUL = 0x04,
  /**<  ETHERNET  Backhaul  */

  QCMAP_BT_BACKHAUL = 0x05,
  /**<  BT WAN Backhaul  */

  QCMAP_BACKHAUL_TYPE_ENUM_MAX_ENUM_VAL = 2147483647
  /**< To force a 32 bit signed enum.  Do not change or use*/

}qcmap_backhaul_type_enum;

/** Data type for the backhaul preference. */
typedef struct
{
  qcmap_backhaul_type_enum first;
  /**< First backhaul preference. */

  qcmap_backhaul_type_enum second;
  /**< Second backhaul preference. */

  qcmap_backhaul_type_enum third;
  /**< Third backhaul preference. */

  qcmap_backhaul_type_enum fourth;
  /**< Fourth backhaul preference. */

  qcmap_backhaul_type_enum fifth;
  /**< Fifth backhaul preference. */

}qcmap_backhaul_pref_t;

/** Data structure for DHCP config */
typedef struct {

   uint32_t dhcp_start_ip;
   /**<   DHCP start IP address. */

   uint32_t dhcp_end_ip;
   /**<   DHCP end IP address. */

   uint32_t lease_time;
   /**<   DHCP lease time, in seconds.*/
}qcmap_dhcp_config;

/** Data structure to add LAN config */
typedef struct {

  uint32_t gw_ip;
  /**<   IP address of the gateway. */

  uint32_t netmask;
  /**<   Subnet mask. */

  uint8_t enable_dhcp;
  /**<   Whether to enable DHCP; boolean value. */

  qcmap_dhcp_config dhcp_config;
  /**<   DHCP configuration. Used only when DHCP is enabled. */
}qcmap_lan_config;

/** Struct for DHCP reservation */
typedef struct {

  uint8_t client_mac_addr[QCMAP_MAC_ADDR_LEN];
  /**<   MAC address of the device. */

  char mac_addr_string[QCMAP_LAN_MAC_ADDR_NUM_CHARS]={0};
  /**<  MAC Addr String.  */

  uint32_t client_reserved_ip;
  /**<   Reserved IP for the AP client. */

  char reserved_ip_string[QCMAP_MAX_SCAN_SIZE]={0};
  /**<   Reserved IP string for the AP client. */

  char client_device_name[QCMAP_MAX_DEVICE_NAME];
  /**<   Device name. */

  uint8_t enable_reservation;
  /**<   To enable/disable DHCP reservation; boolean value. */
}qcmap_dhcp_reservation;  /* Type */

extern const char* wlan_mode_str[];


/** Struct for VLAN configuration */
typedef struct
{
  unsigned short vlan_config_list_len;
  qcmap_lan_vlan_conf_t vlan_config_list_ex[QCMAP_MAX_VLAN_ENTRIES];
} qcmap_vlan_conf_t;

typedef enum {
  QCMAP_CONFIG_STATE_ENUM_MIN_ENUM_VAL = -2147483647, /**< To force a 32 bit signed enum.  Do not change or use*/
  QCMAP_CONFIG_DISABLE = 0x00,
  QCMAP_CONFIG_ENABLE = 0x01,
  QCMAP_CONFIG_RESTART = 0x02,
  QCMAP_CONFIG_STATE_ENUM_MAX_ENUM_VAL = 2147483647 /**< To force a 32 bit signed enum.  Do not change or use*/
}qcmap_config_state;

/**  Data structure for Getting MAC Address List.
 */
typedef struct {

  uint8_t hw_filtering_mac_addr[QCMAP_MAC_ADDR_LEN];
  /**<   Mac address of client */
}qcmap_hw_mac_filter;  /* Type */

/** @addtogroup qcmap_gsb_datatypes
@{ */

typedef enum {
  MSG_TYPE_ADD = 0x01,
  MSG_TYPE_DEL = 0x02,
  MSG_TYPE_UNLOAD = 0x03
}gsb_msg_type_enum;

typedef enum {
  QCMAP_WLAN_MODE_ENUM_MIN_ENUM_VAL = -2147483647, /**< To force a 32 bit signed enum.  Do not change or use*/
  QCMAP_INTERFACE_TYPE_WLAN_AP = 0x01, /**<  Interface type is WLAN in AP mode */
  QCMAP_INTERFACE_TYPE_WLAN_STA = 0x02, /**<  Interface type is WLAN in STA mode */
  QCMAP_INTERFACE_TYPE_ETHERNET = 0x03, /**<  Interface type is Ethernet  */
  QCMAP_WLAN_MODE_ENUM_MAX_ENUM_VAL = 2147483647 /**< To force a 32 bit signed enum.  Do not change or use*/
}qcmap_gsb_interface_type_enum;

/**  Data structure used to add GSB configuration entry
 */
typedef struct {

  char if_name[QCMAP_MAX_IFACE_NAME_SIZE];
  /**<   Name of Interface, for example, eth0, wlan1, etc. */

  uint32_t bw_reqd_in_mb;
  /**<   Bandwidth requirement for interface */

  uint16_t if_high_watermark;
  /**<   High watermark requirement for interface */

  uint16_t if_low_watermark;
  /**<   Low watermark requirement for interface */

  qcmap_gsb_interface_type_enum if_type;
  /**<   Interface type. No need to use WLAN IF types
       for reference platforms For reference platform, WLAN IF
       is bridged to IPA dynamically.*/

  uint32_t ap_ip;
  /**<   IP address of A7. A7 IP information is managed
       with QCMAP if this parameter is not passed with API.
       Without QCMAP, this information should be passed to
       GSB using API. */
}qcmap_gsb_config;  /* Type */

/** @} */ /* end_addtogroup qcmap_gsb_datatypes */

/**  Data structure to set IP segments for filtering.
 */
typedef struct {

  uint32_t ip_segment_start;
  /**<   IP segment start. */

  uint32_t ip_segment_end;
  /**<   IP segment end. */
}qcmap_ip_segment_filter;  /* Type */

/**  Data structure to set iface Name For Filtering.
 */
typedef struct {

  uint32_t if_name_len;  /**< Must be set to # of elements in if_name */
  char if_name[QCMAP_MAX_IFACE_NAME_SIZE];
  /**<   Maximum iface name size. */
}qcmap_if_name_filter;  /* Type */

/*Data type for Hardware MAC filter config*/
typedef struct
{
  qcmap_config_state mac_flt_state;
  /**< Identifies the MAC Filtered enable state. */

  int num_of_clients;
  /**< Identifies the number of MAC filtered clients. */

  qcmap_hw_mac_filter client_list[QCMAP_MAX_HW_MAC_FILTER_CLIENTS];
  /**< Identifies the list of MAC Addresses for which MAC Filtering is enabled. */

  qcmap_config_state ip_segment_filter_state;
  /**< Identifies the IP Segment Filtering State. */

  int num_of_ip_segments;
  /**< Identifies the number of IP Segments configured. */

  qcmap_ip_segment_filter ip_segment_filter_list[QCMAP_MAX_IPV4_SEGMENT_FILTERING];
  /**< Identifies the list of IP Segments for which IP Segment Filtering is enabled. */

  qcmap_config_state iface_filter_state;
  /**< Identifies the Interface Filtering enable state. */

  int num_of_iface;
  /**< Identifies the number of interfaces for which Iface Filtering is enabled. */

  qcmap_if_name_filter if_name_filter_list[QCMAP_MAX_IFACE_FILTERING];
  /**< Identifies the list of interfaces for which Iface Filtering is enabled. */
}qcmap_hdw_filter_config;

#ifndef FEATURE_QCMAP_OFFTARGET
typedef enum
{
SET_VALUE = 0,
GET_VALUE,
DELETE_VALUE
}qcmap_action_type;
#endif

typedef enum {
  QCMAP_HOSTAPD_START = 0x01, /**<  Start hostapd.  */
  QCMAP_HOSTAPD_STOP = 0x02, /**<  Stop hostapd.  */
  QCMAP_HOSTAPD_RESTART = 0x03 /**<  Restart hostapd.  */
}qcmap_activate_hostapd_action_enum;

/** @} */ /* end_addtogroup qcmap_lan_datatypes */


/*=====================================================================
  FUNCTION SetResetDHCPIgnoreOption
======================================================================*/
/*!
@brief
  - Sets or Resets DHCP ignore option on each of the bridge's

@return
  true - Success
  false - Failure

@note
  - Setting dhcp ignore to 1 to stop DHCP server functioning
  on bridge's.

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
#ifdef FEATURE_QCMAP_OFFTARGET
boolean
SetResetDHCPIgnoreOption
(
  boolean reset
);
#else
boolean
SetResetDHCPIgnoreOption
(
  boolean reset,
  const std::nullptr_t bridge_id = NULL
);
#endif

/*=====================================================================
  FUNCTION check_non_empty_mac_addr
======================================================================*/
/*!
@brief
  Check for empty mac address

@return
  bool

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
boolean
check_non_empty_mac_addr
(
  uint8_t *mac,
  char mac_addr_string[]
);

/*=====================================================================
  FUNCTION CheckIfNetworkRulesExist
======================================================================*/
/*!
@brief
  - Checks if lan or wan network rules exist in uci database

@return
  true - Success
  false - Failure

@note
  - System call to check if provided lan or wan network rules exist
  in uci database

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
boolean
CheckIfNetworkRulesExist
(
  const char *network_interface,
  const uint32_t profile_handle
);

/*=====================================================================
  FUNCTION GetProfileIndex
======================================================================*/
/*!
@brief
  - Get profile index of passed profile number in qcmap_lan database

@return
  profile_idx

@note
  - returns the profile index of passed profile number in qcmap_lan database

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
int
GetProfileIndex
(
  const uint32_t profile_handle
);

/*=====================================================================
  FUNCTION UpdateRmnetFile
======================================================================*/
/*!
@brief
  - Performs network up or down based on an event

@return
  true - Success
  false - Failure

@note
  - System call to check if provided lan or wan network rules exist
  in uci database

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
boolean
UpdateRmnetFile
(
  const char *evt,
  const uint32_t profile_handle,
  qcmap_backhaul_type bh_type
);

/*=====================================================================
  FUNCTION UpdateRmnetEthFile
======================================================================*/
/*!
@brief
  - Performs network up or down based on an event for eth pdu call

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
boolean
UpdateRmnetEthFile
(
  const char *evt,
  const uint32_t profile_handle
);

/*=====================================================================
  FUNCTION GetActiveIPPT
======================================================================*/
/*!
@brief
  - Get active IPPT value of passed profile number in qcmap_lan database

@return
  profile_idx

@note
  - returns the active IPPT value of passed profile number in qcmap_lan
   database

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
int
GetActiveIPPT
(
  const uint32_t profile_idx
);

/*=====================================================================
  FUNCTION GetActiveIpsecVpnPt
======================================================================*/
/*!
@brief
  - Get ipsecpt_enable value of passed profile number in qcmap_lan database

@return
  ipsecpt_enable

@note
  - returns ipsecpt_enable value of passed profile number in qcmap_lan
   database

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
int
GetActiveIpsecVpnPt
(
  const uint32_t profile_idx
);

/*=====================================================================
  FUNCTION GetActivePptpVpnPt
======================================================================*/
/*!
@brief
  - Get pptppt_enable value of passed profile number in qcmap_lan database

@return
  pptppt_enable

@note
  - returns pptppt_enable value of passed profile number in qcmap_lan
   database

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
int
GetActivePptpVpnPt
(
  const uint32_t profile_idx
);

/*=====================================================================
  FUNCTION GetActiveIpsecVpnPtIpv6
======================================================================*/
/*!
@brief
  - Get active IPV6 IPSec VPN passthrough value of passed profile number in qcmap_lan database

@return
  active_ipsecptv6

@note
  - returns the active IPV6 IPSec VPN passthrough value of passed profile number in qcmap_lan
   database

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
int
GetActiveIpsecVpnPtIpv6
(
  const uint32_t profile_idx
);

/*=====================================================================
  FUNCTION GetActivePptpVpnPtIpv6
======================================================================*/
/*!
@brief
  - Get pptpptv6_enable value of passed profile number in qcmap_lan database

@return
  pptpptv6_enable

@note
  - returns pptpptv6_enable value of passed profile number in qcmap_lan
   database

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
int
GetActivePptpVpnPtIpv6
(
  const uint32_t profile_idx
);

/*=====================================================================
  FUNCTION ExecuteUCICommit
======================================================================*/
/*!
@brief
  - Executes uci commit command

@return
  none

@note
  - Executes uci commit command using dsi_system_call

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
void
ExecuteUCICommit();

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
void
DeleteIPPTPhyIface
(
  const uint32_t profile_idx
);

/*=====================================================================
  FUNCTION PerformDnsmasqRestart
======================================================================*/
/*!
@brief
  - Executes dnsmasq restart command

@return
  none

@note
  - Executes dnsmasq restart command using dsi_system_call

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
void
PerformDnsmasqRestart();

/*===========================================================================
FUNCTION ValidateBridgeContext()
===========================================================================*/
/** @ingroup qcmap_validate_bridge_context

  Checks if global bridge context is in saved vlan-id list of profile handle

  @param[in]      vlan_ids           list of vlan-id's mapped to the profile
  @param[in]      bridge_context     current global bridge context
  @param[in]      profile_handle     Current WWAN profile number

  @return
  TRUE -- Success \n
  FALSE -- Failure \n
*/
/*=========================================================================*/
boolean
ValidateBridgeContext
(
  char *bridge_vlan_ids,
  const int16_t bridge_context,
  const uint32_t profile_handle
);

/*=====================================================================
  FUNCTION isInterfaceUP
======================================================================*/
/*!
@brief
  - Get the If status

@return
  -IF_STATUS_UP
  -IF_STATUS_DOWN

@param[in]
  char* if_name

@note
  - Dependencies
  - None

  - Side Effects
  - None
*/
/*=========================================================================*/
int
isInterfaceUP(char* if_name);

/*=====================================================================
  FUNCTION ChangeIFState
======================================================================*/
/*!
@brief
  - Change If state

@return
  QCMAP_CM_SUCCESS - Success
  QCMAP_CM_ERROR - Failure

@param[in]
  char* if_name
  char* state

@note
  - Dependencies
  - None

  - Side Effects
  - None
*/
/*=========================================================================*/
void
ChangeIFState(char* if_name, char* state);

/*=====================================================================
  FUNCTION SendMSGToGSB
======================================================================*/
/*!
@brief
  - Send message to GSB

@return
  QCMAP_CM_SUCCESS - Success
  QCMAP_CM_ERROR - Failure

@param[in]
  -qcmap_gsb_config conf
  -int code

@note
  - Dependencies
  - None

  - Side Effects
  - None
*/
/*=========================================================================*/
int
SendMSGToGSB(qcmap_msgr_gsb_config_v01 *conf, int code);

/*=====================================================================
  FUNCTION GetDefaultProfilefromUCI
======================================================================*/
/*!
@brief
  - Gets the default profile handle information from qcmap_lan db

@return
  - int default_profile

@param[in]
  - None

@note
  - Dependencies
  - None

  - Side Effects
  - None
*/
/*=========================================================================*/
int
GetDefaultProfilefromUCI();

/*=====================================================================
  FUNCTION StopMWAN3
======================================================================*/
/*!
@brief
  - Issues command to stop mwan3 process

@return
  - void

@param[in]
  - None

@note
  - Dependencies
  - None

  - Side Effects
  - None
*/
/*=========================================================================*/
void
StopMWAN3();

/*=====================================================================
  FUNCTION StartMWAN3
======================================================================*/
/*!
@brief
  - Issues command to start mwan3 process

@return
  - void

@param[in]
  - None

@note
  - Dependencies
  - None

  - Side Effects
  - None
*/
/*=========================================================================*/
void
StartMWAN3();

/*=====================================================================
  FUNCTION PerformStartStopMWAN3
======================================================================*/
/*!
@brief
  - Checks if ippt is enabled on Profile
  - Gets default profile number from qcmap_lan
  - Checks if profile handle is default profile
  - performs mwan3 stop

@return
  - void

@param[in]
  - profile handle
  - event (Bringup/teardown)

@note
  - Dependencies
  - None

  - Side Effects
  - None
*/
/*=========================================================================*/
void
PerformStartStopMWAN3(const uint32_t profile_handle, const char *evt);

/*=====================================================================
  FUNCTION CheckEnableIPPT
======================================================================*/
/*!
@brief
  - Checks if ippt is enabled on the Profile

@return
  - enable_state

@param[in]
  - profile handle

@note
  - Dependencies
  - None

  - Side Effects
  - None
*/
/*=========================================================================*/
int
CheckEnableIPPT(const uint32_t profile_handle);

/*=====================================================================
  FUNCTION CheckMWANHandling
======================================================================*/
/*!
@brief
  - Checks if ippt is enabled on Profile
  - Gets default profile number from qcmap_lan
  - Checks if profile handle is default profile
  - Returns a boolean value if post-processing is required

@return
    true - Success
    false - Failure

@param[in]
  - profile handle

@note
  - Dependencies
  - None

  - Side Effects
  - None
*/
/*=========================================================================*/
boolean
CheckMWANHandling(const uint32_t profile_handle, const char *evt);

/*=====================================================================
  FUNCTION GetIPPTPDNCountfromUCI
======================================================================*/
/*!
@brief
  - Gets the IPPT PDN count information from qcmap_lan db

@return
  - int IPPT PDN count

@param[in]
  - None

@note
  - Dependencies
  - None

  - Side Effects
  - None
*/
/*=========================================================================*/
int
GetIPPTPDNCountfromUCI();

/*=====================================================================
  FUNCTION GetCurrentBackhaul
======================================================================*/
/*!
@brief
  - Gets the bh_presentv4/v6 information from qcmap_lan db

@return
  - true - Success
    false - Failure

@note
  - Dependencies
  - None

  - Side Effects
  - None
*/
/*=========================================================================*/
boolean
GetCurrentBackhaul(char *bh_present_v4,
                   char *bh_present_v6,
                   int profile_idx);

/*=====================================================================
  FUNCTION ares_inet_net_pton
======================================================================*/
 /*!
@brief
 - convert network number from presentation to network format.
      accepts hex octets, hex strings, decimal octets, and /CIDR.
      "size" is in bytes and describes "dst"


@return
 - number of bits, either imputed classfully or specified with /CIDR,
       or -1 if some failure occurred (check errno)

@note
  - Dependencies
  - None

  - Side Effects
  - None

 */
/*=========================================================================*/
int
ares_inet_net_pton(int af,
                   const char *src,
                   void *dst,
                   size_t size);

//===================================================================
//              Class Definitions
//===================================================================

class QCMAP_WLAN_Common;

/**  @ingroup qcmap_lan_class */
class QCMAP_LAN_Client
{
  private:
    /* Private Member Functions */
    void Init();
    boolean AddFireWallEntry_Internal();

    QCMAP_WLAN_Common    *m_pQCMapWlanObj = NULL;
    boolean IsWlanEnabled;


  public:
    int16_t bridge_id;
    boolean dhcp_reservations_updated;

/*===========================================================================
FUNCTION QCMAP_LAN_Client()
===========================================================================*/
/** @ingroup qcmap_lan_class

   Constructor for the LAN client library QCMAP_LAN_Client class.

   This constructor initializes the LAN Client.

   @return
   None.
*/
/*=========================================================================*/
QCMAP_LAN_Client
(
  void
);


/*===========================================================================
  FUNCTION AddFireWallEntry_Internal
  ===========================================================================*/
/*!
  @brief
  Add a firewall configuration according to user config
  Used by AddFireWallEntry() and AddUPNPPinholeEntry()

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

boolean AddFireWallEntry_Internal
(
  qcmap_msgr_firewall_conf_t *firewall_conf,
  uint32_t wan_profile_handle,
  boolean  upnp_pinhole,
  qmi_error_type_v01 *qmi_err_num
);

/*===========================================================================
FUNCTION EnableNetIFd()
===========================================================================*/
/** @ingroup qcmap_enable_netifd

  Enables Netifd. It writes wwan Backhaul parameters to a temp file
  and triggers rmnet_update shell script which is an indication to
  NetIFd to read the temp file and set up LAN routes.

  @datatypes
  qcmap_wwan_backhaul_info \n
  qcmap_backhaul_type

  @param[in]  wwan_info  WWAN Backhaul information \n
  @param[in]  bh_type    WWAN Backhaul type(v4/v6)

  @return
  TRUE -- Success \n
  FALSE -- Failure \n
*/
/*=========================================================================*/
boolean
EnableNetIFd
(
  qcmap_wwan_backhaul_info *wwan_info,
  qcmap_backhaul_type bh_type,
  qcmap_backhaul_enable_type enable_type = SETUP
);

/*===========================================================================
FUNCTION GetIPv6Prefix()
===========================================================================*/
/** @ingroup qcmap_get_ipv6_prefix

  Calculates IPv6 prefix from IPv6 address and prefix length.

  @param[in]      v6_addr_str      IPv6 Address string \n
  @param[in,out]  v6_prefix_str    IPv6 Prefix string \n
  @param[in]      v6_prefix_len    IPv6 Prefix length

  @return
  TRUE -- Success \n
  FALSE -- Failure \n
*/
/*=========================================================================*/
static boolean
GetIPv6Prefix
(
  const char *v6_addr_str,
  unsigned char *v6_prefix_str,
  uint8_t prefix_len
);

/*===========================================================================
FUNCTION ReverseByteOrder()
===========================================================================*/
/** @ingroup qcmap_reverse_byte_order

  Reverses the byte order.

  @param[in]      addr      IPv6 quadlet  \n

  @return
  uint32_t -- Reversed byte order
*/
/*=========================================================================*/
static uint32_t
ReverseByteOrder
(
  uint32_t addr
);

/*===========================================================================
FUNCTION DelConfigFile()
===========================================================================*/
/** @ingroup qcmap_delete_config_file

  Deletes config file in /tmp directory during BH tear down event.

  @datatypes
  qcmap_backhaul_type

  @param[in]      profile_num      BH profile number \n
  @param[in]      bh_type          WWAN Backhaul type(v4/v6)

  @return
  TRUE -- Success \n
  FALSE -- Failure \n
*/
/*=========================================================================*/
boolean
DelConfigFile
(
  uint32_t profile_num,
  qcmap_backhaul_type bh_type
);


/*===========================================================================
FUNCTION GetCDTValue()
===========================================================================*/
/** @ingroup

  Get CDT Value of the hardware

  @return
  qcmap_cdt_enum value \n
*/
/*=========================================================================*/

qcmap_cdt_enum utilGetCDTValue();

/*===========================================================================
FUNCTION UtilGetCpeWkkType()
===========================================================================*/
/** @ingroup

  Get WKK Type Value of the CPE hardware

  @return
  qcmap_cpe_wkk_enum value \n
*/
/*=========================================================================*/

qcmap_cpe_wkk_enum UtilGetCpeWkkType();

/*=====================================================================
  FUNCTION UciSetUtility
======================================================================*/
/*!
@brief
  - execute uci set command depending upon the inputs

@input
@input
  param[in]          filename                   name of the config file.
                                                A macro "owrt_filename" is defined for this.
                                                owrt_filename[0] --> qcmap_lan
                                                owrt_filename[1] --> qcmap_firewall
                                                owrt_filename[2] --> firewall
                                                owrt_filename[3] --> network
                                                owrt_filename[4] --> dhcp
  param[in]          config                     name of the uci config
  param[in]          IsConfigNameProvided       if name of the config section is provided i.e
                                                if the value is 1, then we dont need the index
                                                to set or get the option using uci.
                                                example :
                                                network.lan=interface
                                                network.lan.device='br-lan'
                                                network.lan.proto='static'
                                                network.lan.netmask='255.255.255.0'
                                                network.lan.ip6assign='60'
                                                network.lan.ipaddr='192.168.2.1'

                                                config interface 'lan'  <--Here we have name of
                                                                           config, so we can access
                                                                           option by config name,
                                                                           we dont need index here
                                                                           and here config will
                                                                           be "lan"
                                                  option device 'br-lan'
                                                  option proto 'static'
                                                  option netmask '255.255.255.0'
                                                  option ip6assign '60'
                                                  option ipaddr '192.168.2.1'

                                                network.@device[0]=device
                                                network.@device[0].name='br-lan'
                                                network.@device[0].type='bridge'
                                                network.@device[0].ports='eth0' 'eth0' 'eth1' 'rndis0' 'ecm0'

                                                In this case,we just have
                                                config device         <-- Here we dont have a name,
                                                                          so we have to use index to
                                                                          get or set option values.
                                                                          Hence,Here config will
                                                                          be "device"
                                                  option name 'br-lan'
                                                  option type 'bridge'
                                                  list ports 'eth0'
                                                  list ports 'eth0'
                                                  list ports 'eth1'
                                                  list ports 'rndis0'
                                                  list ports 'ecm0'
  param[in]          option                     uci option name
  param[in]          value                      value we want to set
  param[in]          index                      index

@return
  true - Success
  false - Failure

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
boolean
UciSetUtility
(
  const char* filename,
  const char* config,
  boolean     IsConfigNameProvided,
  const char* option,
  char*       value,
  int         index = 0
);

/*=====================================================================
  FUNCTION UciSetUtility
======================================================================*/
/*!
@brief
  - execute uci set command depending upon the inputs
  - Its a overload function of UciSetUtility having set value as integer

@input
  param[in]          filename                   name of the config file.
                                                A macro "owrt_filename" is defined for this.
                                                owrt_filename[0] --> qcmap_lan
                                                owrt_filename[1] --> qcmap_firewall
                                                owrt_filename[2] --> firewall
                                                owrt_filename[3] --> network
                                                owrt_filename[4] --> dhcp
  param[in]          config                     name of the uci config
  param[in]          IsConfigNameProvided       if name of the config section is provided i.e
                                                if the value is 1, then we dont need the index
                                                to set or get the option using uci.
                                                example :
                                                network.lan=interface
                                                network.lan.device='br-lan'
                                                network.lan.proto='static'
                                                network.lan.netmask='255.255.255.0'
                                                network.lan.ip6assign='60'
                                                network.lan.ipaddr='192.168.2.1'

                                                config interface 'lan'  <--Here we have name of
                                                                           config, so we can access
                                                                           option by config name,
                                                                           we dont need index here
                                                                           and here config will
                                                                           be "lan"
                                                  option device 'br-lan'
                                                  option proto 'static'
                                                  option netmask '255.255.255.0'
                                                  option ip6assign '60'
                                                  option ipaddr '192.168.2.1'

                                                network.@device[0]=device
                                                network.@device[0].name='br-lan'
                                                network.@device[0].type='bridge'
                                                network.@device[0].ports='eth0' 'eth0' 'eth1' 'rndis0' 'ecm0'

                                                In this case,we just have
                                                config device         <-- Here we dont have a name,
                                                                          so we have to use index to
                                                                          get or set option values.
                                                                          Hence,Here config will
                                                                          be "device"
                                                  option name 'br-lan'
                                                  option type 'bridge'
                                                  list ports 'eth0'
                                                  list ports 'eth0'
                                                  list ports 'eth1'
                                                  list ports 'rndis0'
                                                  list ports 'ecm0'
  param[in]          option                     uci option name
  param[in]          value                      value we want to set(integer)
  param[in]          index                      index

@return
  true - Success
  false - Failure

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
boolean
UciSetUtility
(
  const char* filename,
  const char* config,
  boolean     IsConfigNameProvided,
  const char* option,
  int         value,
  int         index = 0
);

/*=====================================================================
  FUNCTION UciGetUtility
======================================================================*/
/*!
@brief
  - execute uci get command depending upon the inputs

@input
  param[in]          filename                   name of the  config file.
                                                A macro "owrt_filename" is defined for this.
                                                owrt_filename[0] --> qcmap_lan
                                                owrt_filename[1] --> qcmap_firewall
                                                owrt_filename[2] --> firewall
                                                owrt_filename[3] --> network
                                                owrt_filename[4] --> dhcp
  param[in]          config                     name of the uci config
  param[in]          IsConfigNameProvided       if name of the config section is provided i.e
                                                if the value is 1, then we dont need the index
                                                to set or get the option using uci.
                                                example :
                                                network.lan=interface
                                                network.lan.device='br-lan'
                                                network.lan.proto='static'
                                                network.lan.netmask='255.255.255.0'
                                                network.lan.ip6assign='60'
                                                network.lan.ipaddr='192.168.2.1'

                                                config interface 'lan'  <--Here we have name of
                                                                           config, so we can access
                                                                           option by config name,
                                                                           we dont need index here
                                                                           and here config will
                                                                           be "lan"
                                                  option device 'br-lan'
                                                  option proto 'static'
                                                  option netmask '255.255.255.0'
                                                  option ip6assign '60'
                                                  option ipaddr '192.168.2.1'

                                                network.@device[0]=device
                                                network.@device[0].name='br-lan'
                                                network.@device[0].type='bridge'
                                                network.@device[0].ports='eth0' 'eth0' 'eth1' 'rndis0' 'ecm0'

                                                In this case,we just have
                                                config device         <-- Here we dont have a name,
                                                                          so we have to use index to
                                                                          get or set option values.
                                                                          Hence,Here config will
                                                                          be "device"
                                                  option name 'br-lan'
                                                  option type 'bridge'
                                                  list ports 'eth0'
                                                  list ports 'eth0'
                                                  list ports 'eth1'
                                                  list ports 'rndis0'
                                                  list ports 'ecm0'
  param[in]          option                     uci option name
  param[in]          result                     to store the output of uci get command
  param[in]          index                      index

@return
  true - Success
  false - Failure

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
static boolean
UciGetUtility
(
  const char* filename,
  const char* config,
  boolean     IsConfigNameProvided,
  const char* option,
  char*       result,
  int         index = 0,
  int *err_num = 0
);


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

void
SetNatType
(
  uint32_t wan_profile_handle,
  const char* nat_type,
  qmi_error_type_v01 *qmi_err_num
);

/*===========================================================================
FUNCTION GetNatType()
===========================================================================*/
/** @ingroup qcmap_set_nat_type

  Get NAT Type. It triggers NAT_ALG_VPN_CONFIG_FILE shell script which print the NAT rules.

  @datatypes
  int, char*

  @param[in]      wan_profile_handle      BH profile handle number \n
  @param[in]      output                  To store type of NAT of a particular profile id.

  @return
  true - Success
  false - Failure
*/
/*=========================================================================*/

bool
GetNatType
(
  uint32_t wan_profile_handle,
  const char* output,
  qmi_error_type_v01 *qmi_err_num
);

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

boolean
EnableNatType
(
  uint32_t wan_profile_handle,
  qmi_error_type_v01 *qmi_err_num
);

/*===========================================================================
   FUNCTION AddDMZ()
===========================================================================*/
/** @ingroup qcmap_add_dmz

  Adds the DMZ IP address

  @param[in] dmz_ip   DMZ IP to be added; address is in host byte order.
  @param[in] wan_profile_handle      BH profile handle number
  @param[in] error_type              1 - Indicates DMZ already exists

  @return
  TRUE -- Success.
  FALSE -- Failure.
*/
/*=========================================================================*/
boolean
AddDMZ
(
  uint32_t wan_profile_handle,
  char* dmz_ip,
  qmi_error_type_v01 *qmi_err_num
);
/*===========================================================================
   FUNCTION DeleteDMZ()
===========================================================================*/
/** @ingroup qcmap_delete_dmz

  Delete DMZ IP address

  @param[in] wan_profile_handle      BH profile handle number

  @return
  TRUE -- Success.
  FALSE -- Failure.
*/
/*=========================================================================*/
boolean
DeleteDMZ
(
  uint32_t wan_profile_handle,
  qmi_error_type_v01 *qmi_err_num
);
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
boolean
GetDMZ
(
  uint32_t wan_profile_handle,
  char *dmz_ip,
  qmi_error_type_v01 *qmi_err_num
);

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
boolean
AlgUsrCfg
(
  qcmap_alg_action_enum action,
  uint32_t wan_profile_handle,
  qcmap_msgr_alg_type_mask_v01 alg_type,
  qmi_error_type_v01    *qmi_err_num
);

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
boolean
EnableAlg
(
  uint32_t wan_profile_handle,
  qcmap_msgr_alg_type_mask_v01 alg_type,
  qmi_error_type_v01    *qmi_err_num
);

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
boolean
DisableAlg
(
  uint32_t wan_profile_handle,
  qcmap_msgr_alg_type_mask_v01 alg_type,
  qmi_error_type_v01    *qmi_err_num
);

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
boolean
EnableRTSPAlg
(
  qmi_error_type_v01 *qmi_err_num
);

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
boolean
DisableRTSPAlg
(
  qmi_error_type_v01 *qmi_err_num
);

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
boolean
EnableSIPAlg
(
  qmi_error_type_v01 *qmi_err_num
);

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
boolean
DisableSIPAlg
(
  qmi_error_type_v01 *qmi_err_num
);

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
boolean
SetSIPServerInfo
(
  qcmap_msgr_sip_server_info_v01 *default_sip_server_info,
  qmi_error_type_v01    *qmi_err_num
);

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
boolean
GetUsrSIPServerInfo
(
  qcmap_msgr_sip_server_info_v01 *default_sip_server_info,
  qmi_error_type_v01    *qmi_err_num
);

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
boolean
UpdateNetworkSIPServerToDHCP
(
  qcmap_msgr_sip_server_info_v01 *network_sip_server_info,
  qmi_error_type_v01    *qmi_err_num
);

#ifdef FEATURE_PORT_TRIGGER
/*===========================================================================
   FUNCTION AddPortTriggerEntry()
===========================================================================*/
/** @ingroup qcmap_add_port_trigger_entry

  Adds the port trigger entry

  @param[in] port_trigger_entry      The port trigger entry info stored.
  @param[in] wan_profile_handle      BH profile handle number
  @param[in] handle

  @return
  TRUE -- Success.
  FALSE -- Failure.
*/
/*=========================================================================*/
boolean
AddPortTriggerEntry
(
  uint32_t                               wan_profile_handle,
  qcmap_msgr_port_trigger_entry_conf_t   port_trigger_entry,
  int                                   *handle,
  qmi_error_type_v01                     *qmi_err_num
);

/*===========================================================================
   FUNCTION DeletePortTriggerEntry()
===========================================================================*/
/** @ingroup qcmap_delete_port_trigger_entry

    Delete the port trigger entry

    @param[in] wan_profile_handle      BH profile handle number
    @param[in] handle

    @return
    TRUE -- Success.
    FALSE -- Failure.
*/
/*=========================================================================*/
boolean
DeletePortTriggerEntry
(
  uint32_t wan_profile_handle,
  int      handle,
  qmi_error_type_v01                     *qmi_err_num
);

/*===========================================================================
   FUNCTION GetPortTriggerEntry()
===========================================================================*/
/** @ingroup qcmap_get_port_trigger_entry

    Gets the port trigger entry

    @param[in] port_trigger_entry      The port trigger entry info stored.
    @param[in] handle

    @return
    TRUE -- Success.
    FALSE -- Failure.
*/
/*=========================================================================*/
boolean
GetPortTriggerEntry
(
  qcmap_msgr_port_trigger_conf_t         *port_trigger,
  int                                    handle,
  qmi_error_type_v01                     *qmi_err_num
);
#endif /* FEATURE_PORT_TRIGGER */

/*===========================================================================
  FUNCTION CreateVLANConfig()
===========================================================================*/
/** @ingroup qcmap_set_vlan_config

  Set VLAN upon getting request from QCMAP.

  @datatypes
  qcmap_lan_vlan_conf_t

  @param[in]      qcmap_lan_vlan_conf_t vlan_config

  @return
  TRUE -- Success \n
  FALSE -- Failure \n
*/
/*=========================================================================*/
boolean
CreateVLANConfig
(
  qcmap_lan_vlan_conf_t vlan_config,
  qmi_error_type_v01 *qmi_err_num,
  bool *is_accelerated
);

boolean
IsWifiUp
(
   qmi_error_type_v01 *qmi_err_num
);
/*===========================================================================
  FUNCTION DeleteVlanConfig()
===========================================================================*/
/** @ingroup qcmap_delete_vlan_config

  Delete VLAN upon getting request from QCMAP.

  @datatypes
  qcmap_lan_vlan_conf_t

  @param[in]      qcmap_lan_vlan_conf_t vlan_config

  @return
  TRUE -- Success \n
  FALSE -- Failure \n
*/
/*=========================================================================*/
boolean
DeleteVlanConfig
(
  qcmap_lan_vlan_conf_t vlan_config,
  qmi_error_type_v01 *qmi_err_num
);

/*===========================================================================
  FUNCTION GetVlanConfig()
===========================================================================*/
/** @ingroup qcmap_show_vlan_config

  Show VLAN upon getting request from QCMAP.

  @return
  TRUE -- Success \n
  FALSE -- Failure \n
*/
/*=========================================================================*/
boolean
GetVlanConfig
(
  qcmap_vlan_conf_t *vlan_config,
  qmi_error_type_v01 *qmi_err_num
);

/*===========================================================================
  FUNCTION AddStaticNatEntry()
===========================================================================*/
/**
  @ingroup qcmap_add_static_nat_entry
  Add a snat entry to UCI DB upon user request
  @datatypes
  qcmap_snat_config_t
  @param[in]      qcmap_snat_config_t    snat_config\n
  @return
  TRUE -- Success \n
  FALSE -- Failure \n
*/
/*=========================================================================*/

boolean
AddStaticNatEntry
(
  qcmap_snat_config_t snat_config,
  uint32_t profile_handle,
  qmi_error_type_v01 *qmi_err_num
);

/*===========================================================================
  FUNCTION DeleteStaticNatEntry()
===========================================================================*/
/**
  @ingroup qcmap_delete_static_nat_entry
  Deletes a snat entry on UCI DB upon user request
  @datatypes
  qcmap_snat_config_t
  @param[in]      qcmap_snat_config_t    snat_config\n
  @return
  TRUE -- Success \n
  FALSE -- Failure \n
*/
/*=========================================================================*/

boolean
DeleteStaticNatEntry
(
  qcmap_snat_config_t snat_config,
  uint32_t profile_handle,
  qmi_error_type_v01 *qmi_err_num
);

/*===========================================================================
  FUNCTION GetStaticNatConfig()
===========================================================================*/
/**
  @ingroup qcmap_get_snat_entry
  Show snat entry from UCI DB upon user request.
  @datatypes
  qcmap_snat_config_t
  @param[in/out]      qcmap_snat_config_t*    snat_config\n
  @param[in/out]      uint16_t*               num_entries\n
  @return
  TRUE -- Success \n
  FALSE -- Failure \n
*/
/*=========================================================================*/
boolean
GetStaticNatConfig
(
  qcmap_snat_config_t *snat_config,
  uint16_t *num_entries,
  uint32_t profile_handle,
  qmi_error_type_v01 *qmi_err_num
);

  /*=====================================================================
  FUNCTION checkFirewallEntryLimit()
======================================================================*/
/*!
@brief
  - check if the number of entries exceeds 128

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

boolean checkFirewallEntryLimit
(
  qmi_error_type_v01 *qmi_err_num
);

/*=====================================================================
  FUNCTION checkSNATEntryLimit()
======================================================================*/
/*!
@brief
  - check if the number of snat entries exceeds 50

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
boolean checkSNATEntryLimit
(
  qmi_error_type_v01 *qmi_err_num
);

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
boolean
AddPDNToVLANMapping
(
  int16_t vlan_id,
  uint32_t profile_handle,
  qmi_error_type_v01 *qmi_err_num
);

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
boolean
DeletePDNToVLANMapping
(
  int16_t vlan_id,
  uint32_t profile_handle,
  qmi_error_type_v01 *qmi_err_num
);

/*===========================================================================
FUNCTION GetPDNToVLANMapping()
===========================================================================*/
/** @ingroup qcmap_get_pdn_to_vlan_mapping

  Maps user provided PDN and vlan-id info.

  @param[in]      mappings           VLAN to PDN mapping
  @param[in]      nuum_entries       VLAN to PDN mapping

  @return
  TRUE -- Success \n
  FALSE -- Failure \n
*/
/*=========================================================================*/
boolean
GetPDNToVLANMapping
(
  qcmap_pdn_to_vlan_mapping *mappings,
  int *num_entries,
  qmi_error_type_v01 *qmi_err_num
);

/*===========================================================================
FUNCTION CheckIfFileExists()
===========================================================================*/
/** @ingroup qcmap_check_if_file_exists

  Checks if file exists.

  @param[in]      flename           file name

  @return
  TRUE -- Success \n
  FALSE -- Failure \n
*/
/*=========================================================================*/
static boolean
CheckIfFileExists
(
  char *filename
);

/*===========================================================================
FUNCTION CreateWWANPolicy()
===========================================================================*/
/** @ingroup section_CreateWWANPolicy

  Creates a WWAN profile in UCI data base whenever a new profile is created.

  @datatypes
  uint32_t \n

  @param[in] profile_handle  profile_handle created

  @return
  void

  @dependencies
  QCMobileAP must be enabled. @newpage
*/
/*=========================================================================*/
void
CreateWWANPolicy
(
  uint32_t wan_profile_handle,
  qmi_error_type_v01 *qmi_err_num
);

/*===========================================================================
FUNCTION CreateWWANPolicyEx()
===========================================================================*/
/** @ingroup section_CreateWWANPolicyEx

  Creates a WWAN profile in UCI data base whenever a new profile is created.

  @datatypes
  uint32_t \n

  @param[in] profile_handle  profile_handle created
             ip_family       the ip family for this policy

  @return
  void

  @dependencies
  QCMobileAP must be enabled. @newpage
*/
/*=========================================================================*/
void
CreateWWANPolicyEx
(
  uint32_t wan_profile_handle,
  qcmap_msgr_ip_family_enum_v01      ip_family,
  qmi_error_type_v01 *qmi_err_num
);

/*===========================================================================
FUNCTION AddWanIfaceOnWanAllList()
===========================================================================*/
/** @ingroup section_AddWanIfaceOnWanAllList

  adds a wan iface corresponding to profile id as a list option in wan_all zone.

  @datatypes
  uint32_t \n

  @param[in] profile_handle         profile_handle of the wan profile.

  @return
  void

  @dependencies
  QCMobileAP must be enabled. @newpage
*/
/*=========================================================================*/
void
AddWanIfaceOnWanAllList
(
 uint32_t wan_profile_handle
);

/*===========================================================================
FUNCTION DelWanIfaceOnWanAllList()
===========================================================================*/
/** @ingroup section_DelWanIfaceOnWanAllList

  Deletes a wan iface corresponding to profile id as a list option in wan_all zone.

  @datatypes
  uint32_t \n

  @param[in] profile_handle         profile_handle of the wan profile.

  @return
  void

  @dependencies
  QCMobileAP must be enabled. @newpage
*/
/*=========================================================================*/
void
DelWanIfaceFromWanAllList
(
  uint32_t wan_profile_handle
);

/*===========================================================================
  FUNCTION SetFirewall
  ===========================================================================*/
/*!
  @brief
  Sets the Firewall Config with all params

  @return
  true  - on Success
  false - on Failure

  - Dependencies
  - None

  - Side Effects
  - None
 */
/*=========================================================================*/
boolean
SetFirewall
(
   boolean            enable_firewall,
   boolean            pkts_allowed,
   uint32_t           wan_profile_handle,
   qmi_error_type_v01 *qmi_err_num
 );

/*===========================================================================
  FUNCTION GetFirewall
  ===========================================================================*/
/*!
  @brief
  Gets the Firewall Config

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
boolean
GetFirewall
(
  uint32_t                   wan_profile_id,
  boolean                   *enable_firewall,
  boolean                   *pkts_allowed,
  qmi_error_type_v01 *qmi_err_num
);

/*===========================================================================
  FUNCTION AddFireWallEntry
  ===========================================================================*/
/*!
  @brief
  Add a firewall configuration according to user config

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

boolean
AddFireWallEntry
(
  qcmap_msgr_firewall_conf_t *firewall_conf,
  uint32_t wan_profile_handle,
  qmi_error_type_v01 *qmi_err_num
);


/*=============================================================================
  FUNCTION AddFireWallEntryUtilityV4
  ===========================================================================*/
/*!
  @brief
  Add a firewall configuration according to user config

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

boolean
AddFireWallEntryUtilityV4
(
  qcmap_msgr_firewall_entry_conf_t *firewall_entry,
  int idx,
  int count
);

/*===========================================================================
  FUNCTION AddFireWallEntryUtilityV6
  ===========================================================================*/
/*!
  @brief
  Add a firewall configuration according to user config

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

boolean
AddFireWallEntryUtilityV6
(
  qcmap_msgr_firewall_entry_conf_t *firewall_entry,
  int idx,
  int count
);

/*===========================================================================
  FUNCTION GetFireWallConfigList
  ===========================================================================
@brief
  Get all the firewall configuration list for Mobile AP from Uci

@input
  ip_version - version of IP
  wan_profile_handle - current wan profile handle
  *handle_list_len  - pointer to handle_list_len variable
  extd_firewall_handle_list -  firewall configuration list
  firewall_config  - firewall_config array
  qmi_err_num

@return
  boolean

@dependencies
  usr to provide input

@sideefects
  None
  =========================================================================*/
boolean
GetFireWallConfigList
(
  int ip_version,
  uint32_t wan_profile_handle,
  int* handle_list_len,
  qcmap_msgr_firewall_conf_t *extd_firewall_handle_list,
  qcmap_msgr_firewall_entry_conf_t *firewall_config,
  qmi_error_type_v01 *qmi_err_num
);

/*==========================================================================
FUNCTION DeleteFireWallEntry
===========================================================================*/
  /*!
  @brief
  delete firewall config for firewall entry identified by the handle

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
boolean
DeleteFireWallEntry
(
  int handle,
  uint32_t wan_profile_handle,
  qmi_error_type_v01 *qmi_err_num
);


/*===========================================================================
FUNCTION EnableFirewall
===========================================================================
  @brief
  Enable the firewall configuration rules when backhaul is Up

  @input
  wan_profile_handle, bh_type

  @return
  boolean

  @dependencies
  usr to provide input

  @sideefects
  None
=========================================================================*/
boolean
EnableFirewall
(
  uint32_t wan_profile_handle,
  qcmap_backhaul_type bh_type,
  qmi_error_type_v01 *qmi_err_num
);


/*===========================================================================
  FUNCTION DisableFirewall
  ===========================================================================
  @brief
  Disable the firewall configuration rules when backhaul down

  @input
  wan_profile_handle, bh_type

  @return
  boolean

  @dependencies
  usr to provide input

  @sideefects
  None
  =========================================================================*/
boolean
DisableFirewall
(
  uint32_t wan_profile_handle,
  qcmap_backhaul_type bh_type,
  qmi_error_type_v01 *qmi_err_num
);

/*===========================================================================
  FUNCTION GetEthPDUStatus
  ===========================================================================*/
/*!
  @brief
  Get current eth pdu enable status

  @return
  true  - on eth pdu enabled
  false - on eth pdu disabled

  @note

  - Dependencies
  - None

  - Side Effects
  - None
 */
/*=========================================================================*/
boolean
GetEthPDUStatus( );

/*===========================================================================
  FUNCTION GetCurrentActiveV4Backhaul
  ===========================================================================*/
/*!
  @brief
  Get current active v4 Backahul on current profile handle

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
boolean
GetCurrentActiveV4Backhaul
(
 qcmap_backhaul_status_info_ex_type *backhaul_status_info,
 char *bh_present_v4,
 qmi_error_type_v01 *qmi_err_num
);

/*===========================================================================
  FUNCTION GetCurrentActiveV6Backhaul
  ===========================================================================*/
/*!
  @brief
  Get current active v4 Backahul on current profile handle

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
boolean
GetCurrentActiveV6Backhaul
(
 qcmap_backhaul_status_info_ex_type *backhaul_status_info,
 char *bh_present_v6,
 qmi_error_type_v01 *qmi_err_num
);

/*===========================================================================
  FUNCTION GetCurrentActiveBackhaul
  ===========================================================================*/
/*!
  @brief
  Get current active Backahul on current profile handle

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
boolean
GetCurrentActiveBackhaul
(
 uint32_t wan_profile_handle,
 qcmap_backhaul_status_info_ex_type *backhaul_status_info,
 qmi_error_type_v01 *qmi_err_num
);

/*===========================================================================
  FUNCTION GetBackhaulStatus
  ===========================================================================
  @brief
  check whether the backhaul is up or not for the particular profile id

  @input
  profile_handle

  @return
  boolean

  @dependencies
  usr to provide input

  @sideefects
  None
  =========================================================================*/
boolean
GetBackhaulStatus
(
 uint32_t wan_profile_handle,
 ip_version_enum_type ip_version,
 qmi_error_type_v01 *qmi_err_num
);

/*=====================================================================
  FUNCTION GetDHCPLANOptionsFeatureModes
======================================================================*/
/*!
@brief
  - Get the currently configured DHCP LAN Options feature modes

@return
  - None

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
bool
GetDHCPLANOptionsFeatureModes
(
  uint64_t                             *enabled_features,
  qcmap_lan_client_feature_mode_config *feature_mode_config,
  qmi_error_type_v01                   *qmi_err_num
);

/*===========================================================================
FUNCTION SetFeatureMode()
===========================================================================*/
/** @ingroup qcmap_set_feature_mode

  Sets feature mode provided by the user

  @datatypes
  qcmap_lan_client_feature_mode_config

  @param[in]      feature_mode_config   feature mode struct

  @return
  TRUE -- Success \n
  FALSE -- Failure \n
*/
/*=========================================================================*/
boolean
SetFeatureMode
(
  qcmap_lan_client_feature_mode_config *feature_mode_config,
  qmi_error_type_v01 * qmi_err_num
);

/*===========================================================================
FUNCTION ResetFeatureMode()
===========================================================================*/
/** @ingroup qcmap_reset_feature_mode

  Resets feature mode provided by the user

  @datatypes
  qcmap_lan_client_feature_mode_config

  @param[in]      feature_mode_config   feature mode struct

  @return
  TRUE -- Success \n
  FALSE -- Failure \n
*/
/*=========================================================================*/
boolean
ResetFeatureMode
(
  qcmap_lan_client_feature_mode_config *feature_mode_config,
  qmi_error_type_v01                  *qmi_err_num
);

/*===========================================================================
FUNCTION GetFeatureMode()
===========================================================================*/
/** @ingroup qcmap_get_feature_mode

  gets feature mode requested by the user

  @datatypes
  qcmap_lan_client_feature_mode_config

  @param[in]      feature_mode_config   feature mode struct

  @return
  TRUE -- Success \n
  FALSE -- Failure \n
*/
/*=========================================================================*/
boolean
GetFeatureMode
(
  uint64_t                             *enabled_features,
  qcmap_lan_client_feature_mode_config *feature_mode_config,
  qmi_error_type_v01                   *qmi_err_num
);

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
boolean
SetIPPassthroughConfig
(
  qcmap_lan_ip_passthrough_mode_enum enable_state,
  boolean new_config,
  qcmap_lan_ip_passthrough_config *ip_passthrough_config,
  const uint32_t default_handle,
  qmi_error_type_v01 *qmi_err_num
);


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
boolean
GetIPPassthroughConfig
(
  qcmap_lan_ip_passthrough_mode_enum *enable_state,
  qcmap_lan_ip_passthrough_config *ip_passthrough_config,
  qmi_error_type_v01 *qmi_err_num
);

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
boolean
GetIPPassthroughState
(
  boolean *active_state,
  qmi_error_type_v01 *qmi_err_num
);

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
void
SetBridgeVLANContext
(
  const int16_t bridge_id
);

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
boolean
GetBridgeVLANContext
(
  int16_t *bridge_id
);

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
boolean
CheckIfIPPTConfigExists
(
  qcmap_lan_ip_passthrough_config *ip_passthrough_config,
  const uint32_t profile_handle
);

/*==========================================================================
FUNCTION GetEthNiCNameFromType()
===========================================================================*/
/** @ingroup qcmap_get_eth_nic_config

  Get ETH NIC config.

  @param[in]        qcmap_interface_type_enum

  @return
  TRUE -- Success \n
  FALSE -- Failure \n
*/
/*=========================================================================*/
boolean GetEthNiCNameFromType
(
  qcmap_interface_type_enum  eth_nic_type,
  char* const eth_nic_name,
  uint32_t length
);

/*==========================================================================
FUNCTION GetEthNiCTypeFromName()
===========================================================================*/
/** @ingroup qcmap_get_eth_nic_config

  Get ETH NIC config.

  @param[in]        char*

  @return
  TRUE -- Success \n
  FALSE -- Failure \n
*/
/*=========================================================================*/
qcmap_interface_type_enum  GetEthNiCTypeFromName
(
  const char *eth_nic_name
);

/*==========================================================================
FUNCTION util_get_eth_nic_number()
===========================================================================*/
/** @ingroup qcmap_get_eth_nic_config

  Get eth nic number from eth_iface_name.

  @param[in]        char*

  @return
  1 -- Nic 1 \n
  2 -- Nic 2 \n
*/
/*=========================================================================*/
int util_get_eth_nic_number
(
  char *eth_iface_name
);

/*==========================================================================
FUNCTION GetEthernetNicConfig()
===========================================================================*/
/** @ingroup qcmap_get_eth_nic_config

  Get Eth NIC config type.

  @param[in]      qcmap_eth_config_t* eth_config.

  @return type
  TRUE -- Success
  FALSE -- Failure
*/
/*=========================================================================*/
boolean
GetEthernetNicConfig
(
  qcmap_eth_config_t *eth_config,
  qmi_error_type_v01 *qmi_err_num
);

/*===========================================================================
FUNCTION SetEthernetNicConfig()
===========================================================================*/
/** @ingroup qcmap_set_eth_nic_config

  Get ETH NIC config.

  @param[in]      qcmap_eth_config_t eth_config.

  @return
  TRUE -- Success \n
  FALSE -- Failure \n
*/
/*=========================================================================*/
boolean
SetEthernetNicConfig
(
  const qcmap_eth_config_t eth_config,
  qmi_error_type_v01 *qmi_err_num
);

/*===========================================================================
FUNCTION SetLANConfig()
===========================================================================*/
/** @ingroup section_SetLANConfig

  Sets LAN config

  @datatypes
  qcmap_lan_config \n

  @param[in] qcmap_lan_config   *lan_config

  @return
  bool

  @dependencies
  QCMobileAP must be enabled. @newpage
*/
/*=========================================================================*/
boolean
SetLANConfig
(
   qcmap_lan_config *lan_config,
   qmi_error_type_v01 *qmi_err_num
);

/*===========================================================================
FUNCTION GetLANConfig()
===========================================================================*/
/** @ingroup section_GetLANConfig

  Gets LAN config

  @datatypes
  qcmap_lan_config \n

  @param[in] qcmap_lan_config   *lan_config

  @return
  bool

  @dependencies
  QCMobileAP must be enabled. @newpage
*/
/*=========================================================================*/
boolean
GetLANConfig
(
   qcmap_lan_config *lan_config,
   qmi_error_type_v01 *qmi_err_num
);

/*=====================================================================
  FUNCTION IsLanCfgUpdated
======================================================================*/
/*!
@brief
  Check is current applied LAN config and the prev LAN config are the same

@parameters
   none

@return
   returns succesful if prev/running and current lan config are different

@note
- Dependencies
- None

- Side Effects
- None
*/
/*====================================================================*/
boolean IsLanCfgUpdated
(
  void
);

/*===========================================================================
FUNCTION ActivateLAN()
===========================================================================*/
/** @ingroup section_ActivateLAN

  Activates LAN

  @return
  TRUE -- Success \n
  FALSE -- Failure \n

  @dependencies
  QCMobileAP must be enabled. @newpage
*/
/*=========================================================================*/
boolean
ActivateLAN
(
  const uint32_t profile_handle,
  qmi_error_type_v01 *qmi_err_num
);

/*===========================================================================
FUNCTION CheckAndActivateIPCollisionFromBridgeId()
===========================================================================*/
/** @ingroup section_CheckAndActivateIPCollision

  Checks for the ip collision and activates the configuration from bridge id

  @return
  TRUE -- Success \n
  FALSE -- Failure \n

  @dependencies
  QCMobileAP must be enabled. @newpage
*/
/*=========================================================================*/
boolean CheckAndActivateIPCollisionFromBridgeId();

/*===========================================================================
FUNCTION ActivateIPCollision()
===========================================================================*/
/** @ingroup section_ActivateIPCollision

  Checks for the ip collision and activates the configuration

  @return
  TRUE -- Success \n
  FALSE -- Failure \n

  @dependencies
  QCMobileAP must be enabled. @newpage
*/
/*=========================================================================*/
boolean CheckAndActivateIPCollision(
  uint32_t    wan_profile_id
);

/*===========================================================================
  FUNCTION IsLegacyModeEnabled
  ===========================================================================*/
/*!
  @brief
  get legacy enabled or not

  @return
  true	- on Success
  false - on Failure

  @note

  - Dependencies
  - None

  - Side Effects
  - None
 */
/*=========================================================================*/
boolean IsLegacyModeEnabled();

/*===========================================================================
FUNCTION RestartTetheredClient()
===========================================================================*/
/** @ingroup section_RestartTetheredClient

  Toggles the tethered link

  @param[in] qcmap_lan_device_type_enum dev_type

  @return
  TRUE -- Success \n
  FALSE -- Failure \n

  @dependencies
  None
*/
/*=========================================================================*/
boolean
RestartTetheredClient
(
   qcmap_lan_device_type_enum dev_type
);

/*===========================================================================
FUNCTION IsTetheredLinkUp()
===========================================================================*/
/** @ingroup section_IsTetheredLinkUp

  Checks if link is up

  @param[in] char *ifname

  @return
  TRUE -- Success \n
  FALSE -- Failure \n

  @dependencies
  None
*/
/*=========================================================================*/
boolean
IsTetheredLinkUp
(
   char *ifname
);

/*===========================================================================
FUNCTION ToggleEth()
===========================================================================*/
/** @ingroup section_ToggleEth

  Toggles the ethernet link

  @param[in] char *ifname

  @return
  None

  @dependencies
  None
*/
/*=========================================================================*/
void
ToggleEth
(
   char *ifname
);

/*===========================================================================
FUNCTION ToggleUSB()
===========================================================================*/
/** @ingroup section_ToggleUSB

  Toggles the USB link

  @return
  None

  @dependencies
  None
*/
/*=========================================================================*/
void
ToggleUSB();

/*=====================================================================
  FUNCTION PerformDnsmasqReload
======================================================================*/
/*!
@brief
  - Executes dnsmasq reload command

@return
  none

@note
  - Executes dnsmasq reload command using dsi_system_call

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
void
PerformDnsmasqReload();

/*=====================================================================
  FUNCTION GetDefaultWiFiSSID
======================================================================*/
/*!
@brief
  returns the currently configured ssid from wireless file
  this function is called from the HandleDHCPVendorInformation

  This API is only valid for the Primary AP that to only in AP mode.
  Not valid for any other modes.

@return
  true if ssid is populated in the input string else false

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
bool GetSSID(char * ssid);

/*=====================================================================
  FUNCTION SetResetDCHPVendorInfo
======================================================================*/
/*!
@brief
  To set/reset dhcp vendor information in lan client side

@return
  true if set/reset dhcp vendor info successfully

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
bool SetResetDCHPVendorInfo(bool               is_reset);


/*=========================================================================
  FUNCTION AddDHCPReservRecord
  ===========================================================================*/
/*!
  @brief
  Add a DHCP Reservation Record

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
boolean
AddDHCPReservRecord
(
  qcmap_dhcp_reservation  *dhcp_reserv_record,
  qmi_error_type_v01 *qmi_err_num
);

/*===========================================================================
  FUNCTION GetDHCPReservRecords
  ===========================================================================*/
/*!
  @brief
  Display DHCP Reservation Records

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
boolean
GetDHCPReservRecords
(
  qcmap_dhcp_reservation  *dhcp_reserv_records,
  uint32_t                *num_entries,
  qmi_error_type_v01 *qmi_err_num
);

/*===========================================================================
  FUNCTION EditDHCPReservRecord
  ===========================================================================*/
/*!
  @brief
  Edit a DHCP record based on MAC or IP address as index

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
boolean
EditDHCPReservRecord
(
   uint32_t                         *client_addr,
   qcmap_dhcp_reservation           *dhcp_reserv_record,
   qmi_error_type_v01 *qmi_err_num
);

/*===========================================================================
  FUNCTION DeleteDHCPReservRecord
  ===========================================================================*/
/*!
  @brief
  delete a DHCP record based on MAC or IP address as index

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
boolean
DeleteDHCPReservRecord
(
  uint32_t             *addr,
  qmi_error_type_v01 *qmi_err_num
);

/*===========================================================================
  FUNCTION DeleteOldDHCPReservRecord
  ===========================================================================*/
/*!
  @brief
  delete old DHCP record which are outside pool based on IP address as index

  @return
  true  - on Success
  false - on Failure

  @parameters
  qmi_error_type_v01 *qmi_err_num

  @note

  - Dependencies
  - None

  - Side Effects
  - None
 */
/*=========================================================================*/
boolean
DeleteOldDHCPReservRecord
(
  qmi_error_type_v01 *qmi_err_num
);

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
boolean
CheckIPPTState
(
  qcmap_dhcp_reservation  *dhcp_reserv_record,
  const uint32_t profile_handle
);

/*===========================================================================
FUNCTION  WhitelistWLANChannels
==========================================================================*/
/*!
@brief
Whitelist all the blacklisted WLAN channels and trigger ACS.

@return
QCMAP_CM_SUCCESS
QCMAP_CM_ERROR

@note

- Dependencies
- None

- Side Effects
- None
*/
/*=========================================================================*/
int32_t WhitelistWLANChannels();

/*===========================================================================
FUNCTION SetCoexConfig()
===========================================================================*/
/*

  Enable/Disable the CoEX channel avoidance to reduce co-channel interference
  between WLAN <-> WWAN channels.

  @param[in]  coex_state   Enable/Disable CoEX channel avoidance.

  @return
  TRUE -- Success. \n
  FALSE -- Failure.

  @dependencies
  None
*/
boolean
SetCoexConfig
(
  int coex_state
);
/*=====================================================================
  FUNCTION EnableWLAN
======================================================================*/
/*!
@brief
  - Enable WLAN from qcmap client

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
boolean
EnableWLAN
(
  qmi_error_type_v01  *qmi_err_num
);

boolean EnableWLAN();

/*=====================================================================
  FUNCTION DisableWLAN
======================================================================*/
/*!
@brief
  - Disable WLAN from qcmap client

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
boolean
DisableWLAN
(
  qmi_error_type_v01      *qmi_err_num
);

/*=====================================================================
  FUNCTION ActivateWLAN
======================================================================*/
/*!
@brief
  - ActivateWLAN WLAN from qcmap client

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
boolean
ActivateWLAN
(
  qmi_error_type_v01      *qmi_err_num
);

/*=====================================================================
  FUNCTION SetWLANConfigEx
======================================================================*/
/*!
@brief
  - SetWLANConfigEx WLAN from qcmap client

@return
  true - Success
  false - Failure

@param[in]
  qcmap_wlan_ex_config wlan_config

@note
  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
boolean
SetWLANConfigEx
(
  qcmap_wlan_ex2_config& wlan_config,
  qmi_error_type_v01 *qmi_err_num
);


/*=====================================================================
  FUNCTION SetWLANConfigEx3
======================================================================*/
/*!
@brief
  - SetWLANConfigEx3 WLAN from qcmap client

@return
  true - Success
  false - Failure

@param[in]
  qcmap_wlan_ex3_config_t wlan_config
  qmi_error_type_v01  qmi_err_num

@note
  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
boolean
SetWLANConfigEx3
(
  qcmap_wlan_ex3_config_t& wlan_config,
  qmi_error_type_v01 *qmi_err_num
);


/*=====================================================================
  FUNCTION GetWLANConfigEx3
======================================================================*/
/*!
@brief
  - GetWLANConfigEx3 WLAN from qcmap client

@return
  true - Success
  false - Failure

@param[in]
  qcmap_wlan_ex3_config_t* wlan_config

@note
  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
boolean
GetWLANConfigEx3
(
  qcmap_wlan_ex3_config_t *wlan_config,
  qmi_error_type_v01     *qmi_err_num
);


/*=====================================================================
  FUNCTION SetWLANBootupConfigEx
======================================================================*/
/*!
@brief
  - SetWLANBootupConfigEx WLAN from qcmap client

@return
  true - Success
  false - Failure

@param[in]
  qcmap_wlan_enable_bootup_conf wlan_bootup_enable_config

@note
  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
boolean
SetWLANBootupConfigEx
(
  qcmap_msgr_bootup_flag_v01 wlan_bootup_enable_config,
  qmi_error_type_v01       *qmi_err_num
);

/*=====================================================================
  FUNCTION GetWLANBootupConfigEx
======================================================================*/
/*!
@brief
  - GetWLANBootupConfigEx WLAN from qcmap client

@return
  true - Success
  false - Failure

@param[in]
  qcmap_wlan_enable_bootup_conf wlan_bootup_enable_config

@note
  - Dependencies
    - None

  - Side EffectsS
    - None
*/
/*=========================================================================*/
boolean
GetWLANBootupConfigEx
(
  qcmap_bootup_enable_config *wlan_bootup_enable_config,
  qmi_error_type_v01 *qmi_err_num
);

/*=====================================================================
  FUNCTION GetWLANConfigEx
======================================================================*/
/*!
@brief
  - GetWLANConfigEx WLAN from qcmap client

@return
  true - Success
  false - Failure

@param[in/out]
  qcmap_wlan_ex_config *wlan_config

@note
  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
boolean
GetWLANConfigEx
(
  qcmap_wlan_ex_config *wlan_config,
  qmi_error_type_v01 *qmi_err_num
);


/*=====================================================================
  FUNCTION GetWLANStatus
======================================================================*/
/*!
@brief
  - GetWLANStatus from qcmap client

@return
  true - Success
  false - Failure


@param[in/out]
  qcmap_msgr_wlan_mode_enum_v01& wlan_mode


@note
  - Dependencies
    - None

  - Side Effects
    - None
*/
bool
GetWLANStatus
(
  qcmap_msgr_wlan_mode_enum_v01 *wlan_mode,
  qmi_error_type_v01 *qmi_err_num
)
;

/*=====================================================================
  FUNCTION GetStationModeStatus
======================================================================*/
/*!
@brief
  - GetStationModeStatus from qcmap client

@return
  true - Success
  false - Failure


@param[in/out]
  qcmap_sta_status_enum& sta_status

@note
  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
bool
GetStationModeStatus
(
  qcmap_msgr_station_mode_status_enum_v01* sta_status,
  qmi_error_type_v01 *qmi_err_num
)
;

/*===========================================================================
  FUNCTION GetActiveWlanIfInfo
  ===========================================================================*/
/*!
  @brief
  Obtains information from active WLAN interfaces.

  @return
  true  - on Success
  false - on Failure

  @note

  - Dependencies
  - WLAN should be enabled

  - Side Effects
  - None

  - Sample output:
    +---------+---------+-------------+-----------+
    |        |         |             |           |
    | IF Name | AP type |  Card Type |   State   |
    |        |         |             |           |
    +---------+---------+-------------+-----------+
    |    ath0|  Primary|       WAKIKI|    Enabled|
    +---------+---------+-------------+-----------+
    |   ath01|    Guest|       WAKIKI|    Enabled|
    +---------+---------+-------------+-----------+

 */
/*=========================================================================*/
boolean
GetActiveWlanIfInfo
(
  qcmap_msgr_wlan_if_info_t *wlan_info_cfg,
  qmi_error_type_v01 *qmi_err_num
);

/*=====================================================
  FUNCTION GetKernelVer
======================================================*/
/*!
@brief
  Queries the kernel version.

@parameters
  char *version

@return
  true  - on success
  false - on failure

@note

- Dependencies
- None

- Side Effects
- None
*/
/*=====================================================*/
boolean
GetKernelVer
(
  char *version
);

/*=====================================================
  FUNCTION CompareKernelVer
======================================================*/
/*!
@brief
  Compares the kernel version with given kernel version.

@parameters
  char     *compare_kernel_ver

@return
  true  - if actual kernel version higher than or equal to
          given kernel version
  false - if actual kernel version less than given kernel version
  -1    - on failure

@note

- Dependencies
- None

- Side Effects
- None
*/
/*=====================================================*/
int
CompareKernelVer
(
  const char *compare_kernel_ver
);

/*===========================================================================
  FUNCTION SetNatTimeoutOnApps
==========================================================================*/
/*!
@brief
  Will set the NAT timeout value for the identified nat type.

@parameters
  qcmap_nat_timeout_enum timeout_type
  uint32_t                    timeout_value

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
boolean
SetNatTimeoutOnApps
(
  qcmap_nat_timeout_enum   timeout_type,
  uint32_t                 timeout_value,
  qmi_error_type_v01 *qmi_err_num
);

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
boolean
SetNatTimeout
(
  qcmap_nat_timeout_enum          timeout_type,
  uint32                          timeout_value,
  qmi_error_type_v01 *qmi_err_num
);

/*===========================================================================
  FUNCTION GetNatTimeoutOnApps
==========================================================================*/
/*!
@brief
  Get the NAT timeout value for the requested nat type.

@parameters
  qcmap_nat_timeout_enum          timeout_type
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
boolean
GetNatTimeoutOnApps
(
  qcmap_nat_timeout_enum    timeout_type,
  uint32_t                  *timeout_value,
  qmi_error_type_v01 *qmi_err_num
);

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
boolean
GetNatTimeout
(
  qcmap_nat_timeout_enum          timeout_type,
  uint32                         *timeout_value,
  qmi_error_type_v01 *qmi_err_num
);

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
boolean
SetupTinyProxy(void);

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
boolean
StopTinyProxy(void);

/*===========================================================================
  FUNCTION EnableTinyProxy
==========================================================================*/
/*!
@brief
  Enable tiny proxy

@return
  true  - enabled
  false - disabled

@note
- Dependencies
- None

- Side Effects
- None
*/
/*=========================================================================*/
boolean
EnableTinyProxy
(
    qmi_error_type_v01 *qmi_err_num
);

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
boolean
DisableTinyProxy
(
    qmi_error_type_v01 *qmi_err_num
);

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
 boolean
 GetTinyProxyStatus
 (
  qcmap_msgr_tiny_proxy_mode_enum_v01 *tinyproxy_status,
  qmi_error_type_v01 *qmi_err_num
 );

/*==========================================================================
FUNCTION DeleteWWANPolicy()
===========================================================================*/
/** @ingroup qcmap_delete_wwan_policy

  Delete WWAN policy.

  @param[in] uint32_t profile_handle.
  @return
  TRUE -- Success \n
  FALSE -- Failure \n
*/
/*=========================================================================*/

boolean
DeleteWWANPolicy
(
  uint32_t profile_handle,
  qmi_error_type_v01 *qmi_err_num
);


/*===========================================================================
  FUNCTION DeleteConntrackEntryForDropIPv4FirewallEntries()
==========================================================================*/
/*!
@brief
  Delete the client conntrack when we are adding a DROP firewall entry for
  that IPv4 address and port combination

@parameters
  firewall_entry - firewall entry to add
  protocol_num   - protocol number

@return
  bool

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
bool
DeleteConntrackEntryForDropIPv4FirewallEntries
(
  qcmap_msgr_firewall_entry_conf_t *firewall_entry,
  uint8_t protocol_num,
  qmi_error_type_v01 *qmi_err_num
);

/*======================================================
  FUNCTION:  DeleteConntrackEntryForAcceptIPv4FirewallEntries
  =====================================================*/
  /*!
      @brief
      Delete the client conntrack when we are adding a ACCEPT firewall entry for
      that IPv4 address and port combination

      @params
         IPv6 address
         Protocol
         UDP Port start number
         UDP Port Range
         TCP Port start number
         TCP Port Range

      @return
      false - failure
      true - success
  */
/*====================================================*/
bool
DeleteConntrackEntryForAcceptIPv4FirewallEntries
(
  qcmap_msgr_firewall_entry_conf_t *firewall_entry,
  uint8_t protocol_num,
  qmi_error_type_v01 *qmi_err_num
);

/*======================================================
  FUNCTION:  DeleteConntrackEntryForAcceptIPv6FirewallEntries
  =====================================================*/
  /*!
      @brief
      Delete the client conntrack when we are adding a firewall entry for
      that IPv6 address and port combination

      @params
         IPv6 address
         Protocol
         UDP Port start number
         UDP Port Range
         TCP Port start number
         TCP Port Range

      @return
      void
  */
/*====================================================*/
void
DeleteConntrackEntryForAcceptIPv6FirewallEntries
(
  qcmap_msgr_firewall_entry_conf_t *firewall_entry,
  uint8_t protocol_num,
  qmi_error_type_v01 *qmi_err_num
);

/*======================================================
  FUNCTION:  DeleteConntrackEntryForDropIPv6FirewallEntries
  =====================================================*/
  /*!
      @brief
      Delete the client conntrack when we are adding a firewall entry for
      that IPv6 address and port combination

      @params
         IPv6 address
         Protocol
         UDP Port start number
         UDP Port Range
         TCP Port start number
         TCP Port Range

      @return
      void
  */
/*====================================================*/
void
DeleteConntrackEntryForDropIPv6FirewallEntries
(
  qcmap_msgr_firewall_entry_conf_t *firewall_entry,
  uint8_t protocol_num,
  qmi_error_type_v01 *qmi_err_num
);

/*===========================================================================
FUNCTION GetBackhaulType()
===========================================================================*/
/** @ingroup char*

  Get Backhaul prefrences.

  @param[in]      char*       pointer to backhaul name.

  @return
  qcmap_backhaul_type_enum type
*/
/*=========================================================================*/

static qcmap_backhaul_type_enum
GetBackhaulType
(
  char*wan
);

/*===========================================================================
FUNCTION GetWANType()
===========================================================================*/
/** @ingroup qcmap_backhaul_type_enum

  Get wan type.

  @param[in]      qcmap_backhaul_type_enum wan_type.

  @return
  pointer to wan type name
*/
/*=========================================================================*/

const char*
GetWANType
(
  qcmap_backhaul_type_enum type
);

/*===========================================================================
FUNCTION SetActiveBackhaulPref()
===========================================================================*/
/** @ingroup qcmap_set_active_backhaul_pref

  Set Backhaul prefrences.

  @param[in]      qcmap_backhaul_pref*       pointer to backhaul pref request.

  @return
  TRUE -- Success \n
  FALSE -- Failure \n
*/
/*=========================================================================*/

boolean
SetActiveBackhaulPref
(
  qcmap_backhaul_pref_t *qcmap_backhaul_pref,
  qmi_error_type_v01 *qmi_err_num
);

/*===========================================================================
FUNCTION GetBackhaulPref()
===========================================================================*/
/** @ingroup qcmap_get_active_backhaul_pref

  Get Backhaul prefrences.

  @param[in]      qcmap_backhaul_pref*       pointer to backhaul pref response.

  @return
  TRUE -- Success \n
  FALSE -- Failure \n
*/
/*=========================================================================*/

boolean
GetBackhaulPref
(
qcmap_backhaul_pref_t *qcmap_backhaul_pref_resp,
qmi_error_type_v01 *qmi_err_num
);

/*===========================================================================
  FUNCTION BringupBTTethering
  ===========================================================================*/
/*!
  @brief
  Brings up the BT Tethering

  @return
  true - on Success
  false - on Failure

  @note

  - Dependencies
  - None

  - Side Effects
  - None
 */
/*=========================================================================*/
boolean
BringupBTTethering
(
  qcmap_bt_mode   bt_tethering_mode
);

/*===========================================================================
  FUNCTION BringdownBTTethering
  ===========================================================================*/
/*!
  @brief
  Brings down the BT Tethering

  @return
  true - on Success
  false - on Failure

  @note

  - Dependencies
  - None

  - Side Effects
  - None
 */
/*=========================================================================*/
boolean
BringdownBTTethering
(
  qcmap_bt_mode   bt_tethering_mode
);

/*===========================================================================
  FUNCTION GetBTTetheringStatus
  ===========================================================================*/
/*!
  @brief
  Displays the BT Tethering current status & mode

  @return
  true - on Success
  false - on Failure

  @note

  - Dependencies
  - None

  - Side Effects
  - None
 */
/*=========================================================================*/
boolean
GetBTTetheringStatus
(
  qcmap_bt_tethering_status    *bt_teth_status,
  qmi_error_type_v01           *qmi_err_num,
  qcmap_bt_mode                *bt_teth_mode
);

boolean GetBTTetheringStatus
(
  qcmap_bt_tethering_status    *bt_teth_status,
  qcmap_bt_mode                *bt_teth_mode
);

/*==========================================================================
FUNCTION RestoreFactoryConfig()
===========================================================================*/
/*!
@brief
  Perform factory reset.

@return
  TRUE -- Success \n
  FALSE -- Failure \n
*/
/*=========================================================================*/

boolean RestoreFactoryConfig
(
  qmi_error_type_v01 *qmi_err_num
);


/*===========================================================================
  FUNCTION GetTetheredIfaceNameFromUCI
==========================================================================*/
/*!
@brief
  Gets iface name for a given tethered iface type from UCI.


@return
  true - if operation is successful
  false - if operation fails.

@note
- Dependencies
- None

- Side Effects
- None
*/
/*=========================================================================*/
boolean GetTetheredIfaceNameFromUCI
(
  char *iface_str,
  char *iface_name
);

/*===========================================================================
  FUNCTION GetIfaceNameFromEnum
==========================================================================*/
/*!
@brief
  Gets iface name for a given iface_enum.


@return
  true - if operation is successful
  false - if operation fails.

@note
- Dependencies
- None
- Side Effects
- None
*/
/*=========================================================================*/
bool GetIfaceNameFromEnum
(
  qcmap_interface_type_enum iface_type,
  char                      *iface_name
);

/*===========================================================================
  FUNCTION GetIfaceEnumFromName
==========================================================================*/
/*!
@brief
  Gets iface name for a given iface_enum.


@return
  true - if operation is successful
  false - if operation fails.

@note
- Dependencies
- None

- Side Effects
- None
*/
/*=========================================================================*/
bool GetIfaceEnumFromName
(
  const char                 *iface_name,
  qcmap_interface_type_enum  *iface_type
);

/*===========================================================================
FUNCTION SetIPSECVpnPassthrough()
===========================================================================*/
/** @ingroup qcmap_set_ipsec_vpn_passthrough_config

  sets IPSec VPN Passthrough

  @datatypes
  qcmap_lan_ipsec_vpn_passthrough_mode_enum

  @param[in]      enable_state              IPsec VPN Passthrough Enable state \n
  @param[in]      default_handle            Default Profile handle \n
  @param[in]      profile_handle            Current Profile Handle

  @return
  TRUE -- Success \n
  FALSE -- Failure \n
*/
/*=========================================================================*/
boolean
SetIPSECVpnPassthrough
(
  qcmap_lan_ipsec_vpn_passthrough_mode_enum enable_state,
  const uint32_t default_handle,
  const uint32_t profile_handle,
  qmi_error_type_v01 * qmi_err_num
);

/*===========================================================================
FUNCTION GetIPSECVpnPassthrough()
===========================================================================*/
/** @ingroup qcmap_get_ipsec_vpn_passthrough_config

  gets IPSec VPN Passthrough enable flag

  @datatypes
  qcmap_lan_ipsec_vpn_passthrough_mode_enum

  @param[in]      enable_state              IPsec VPN Passthrough Enable state \n
  @param[in]      profile_handle            Current Profile Handle

  @return
  TRUE -- Success \n
  FALSE -- Failure \n
*/
/*=========================================================================*/
boolean
GetIPSECVpnPassthrough
(
  qcmap_lan_ipsec_vpn_passthrough_mode_enum *enable_state,
  const uint32_t profile_handle,
  qmi_error_type_v01 *qmi_err_num
);

/*===========================================================================
FUNCTION SetPPTPVpnPassthrough()
===========================================================================*/
/** @ingroup qcmap_set_pptp_vpn_passthrough_config

  sets PPTP VPN Passthrough

  @datatypes
  qcmap_lan_pptp_vpn_passthrough_mode_enum

  @param[in]      enable_state              PPTP VPN Passthrough Enable state \n
  @param[in]      default_handle            Default Profile handle \n
  @param[in]      profile_handle            Current Profile Handle

  @return
  TRUE -- Success \n
  FALSE -- Failure \n
*/
/*=========================================================================*/
boolean
SetPPTPVpnPassthrough
(
  qcmap_lan_pptp_vpn_passthrough_mode_enum enable_state,
  const uint32_t default_handle,
  const uint32_t profile_handle,
  qmi_error_type_v01 *qmi_err_num
);

/*===========================================================================
FUNCTION GetPPTPVpnPassthrough()
===========================================================================*/
/** @ingroup qcmap_get_pptp_vpn_passthrough_config

  gets PPTP VPN Passthrough enable flag

  @datatypes
  qcmap_lan_pptp_vpn_passthrough_mode_enum

  @param[in]      enable_state              PPTP VPN Passthrough Enable state \n
  @param[in]      profile_handle            Current Profile Handle

  @return
  TRUE -- Success \n
  FALSE -- Failure \n
*/
/*=========================================================================*/
boolean
GetPPTPVpnPassthrough
(
  qcmap_lan_pptp_vpn_passthrough_mode_enum *enable_state,
  const uint32_t profile_handle,
  qmi_error_type_v01 *qmi_err_num
);

/*===========================================================================
FUNCTION SetL2TPIPSECVpnPassthrough()
===========================================================================*/
/** @ingroup qcmap_set_l2tp_ipsec_vpn_passthrough_config

  sets L2TP/IPSec VPN Passthrough

  @datatypes
  qcmap_lan_l2tp_ipsec_vpn_passthrough_mode_enum

  @param[in]      enable_state              L2TP/IPSec VPN Passthrough Enable state \n
  @param[in]      default_handle            Default Profile handle \n
  @param[in]      profile_handle            Current Profile Handle

  @return
  TRUE -- Success \n
  FALSE -- Failure \n
*/
/*=========================================================================*/
boolean
SetL2TPIPSECVpnPassthrough
(
  qcmap_lan_l2tp_ipsec_vpn_passthrough_mode_enum enable_state,
  const uint32_t default_handle,
  const uint32_t profile_handle,
  qmi_error_type_v01 *qmi_err_num
);

/*===========================================================================
FUNCTION GetL2TPIPSECVpnPassthrough()
===========================================================================*/
/** @ingroup qcmap_get_l2tp_ipsec_vpn_passthrough_config

  gets L2TP/IPSec VPN Passthrough enable flag

  @datatypes
  qcmap_lan_l2tp_ipsec_vpn_passthrough_mode_enum

  @param[in]      enable_state              L2TP/IPSec VPN Passthrough Enable state \n
  @param[in]      profile_handle            Current Profile Handle

  @return
  TRUE -- Success \n
  FALSE -- Failure \n
*/
/*=========================================================================*/
boolean
GetL2TPIPSECVpnPassthrough
(
  qcmap_lan_l2tp_ipsec_vpn_passthrough_mode_enum *enable_state,
  const uint32_t profile_handle,
  qmi_error_type_v01 *qmi_err_num
);

/*===========================================================================
FUNCTION SetIPSECVpnPassthroughIpv6()
===========================================================================*/
/** @ingroup qcmap_set_ipsec_vpn_passthrough_v6_config

  sets IPV6 IPSec VPN Passthrough

  @datatypes
  qcmap_lan_ipsec_vpn_passthrough_v6_mode_enum

  @param[in]      enable_state              IPV6 IPsec VPN Passthrough Enable state \n
  @param[in]      default_handle            Default Profile handle \n
  @param[in]      profile_handle            Current Profile Handle

  @return
  TRUE -- Success \n
  FALSE -- Failure \n
*/
/*=========================================================================*/
boolean
SetIPSECVpnPassthroughIpv6
(
  qcmap_lan_ipsec_vpn_passthrough_v6_mode_enum enable_state,
  const uint32_t default_handle,
  const uint32_t profile_handle,
  qmi_error_type_v01 * qmi_err_num
);

/*===========================================================================
FUNCTION GetIPSECVpnPassthroughIpv6()
===========================================================================*/
/** @ingroup qcmap_get_ipsec_vpn_passthrough_v6_config

  gets IPV6 IPSec VPN Passthrough enable flag

  @datatypes
  qcmap_lan_ipsec_vpn_passthrough_v6_mode_enum

  @param[in]      enable_state              IPV6 IPsec VPN Passthrough Enable state \n
  @param[in]      profile_handle            Current Profile Handle

  @return
  TRUE -- Success \n
  FALSE -- Failure \n
*/
/*=========================================================================*/
boolean
GetIPSECVpnPassthroughIpv6
(
  qcmap_lan_ipsec_vpn_passthrough_v6_mode_enum *enable_state,
  const uint32_t profile_handle,
  qmi_error_type_v01 *qmi_err_num
);

/*===========================================================================
FUNCTION SetPPTPVpnPassthroughIpv6()
===========================================================================*/
/** @ingroup qcmap_set_pptp_vpn_passthrough_v6_config

  sets IPV6 PPTP VPN Passthrough

  @datatypes
  qcmap_lan_pptp_vpn_passthrough_v6_mode_enum

  @param[in]      enable_state              IPV6 PPTP VPN Passthrough Enable state \n
  @param[in]      default_handle            Default Profile handle \n
  @param[in]      profile_handle            Current Profile Handle

  @return
  TRUE -- Success \n
  FALSE -- Failure \n
*/
/*=========================================================================*/
boolean
SetPPTPVpnPassthroughIpv6
(
  qcmap_lan_pptp_vpn_passthrough_v6_mode_enum enable_state,
  const uint32_t default_handle,
  const uint32_t profile_handle,
  qmi_error_type_v01 *qmi_err_num
);

/*===========================================================================
FUNCTION GetPPTPVpnPassthroughIpv6()
===========================================================================*/
/** @ingroup qcmap_get_pptp_vpn_passthrough_v6_config

  gets IPV6 PPTP VPN Passthrough enable flag

  @datatypes
  qcmap_lan_pptp_vpn_passthrough_v6_mode_enum

  @param[in]      enable_state              IPV6 PPTP VPN Passthrough Enable state \n
  @param[in]      profile_handle            Current Profile Handle

  @return
  TRUE -- Success \n
  FALSE -- Failure \n
*/
/*=========================================================================*/
boolean
GetPPTPVpnPassthroughIpv6
(
  qcmap_lan_pptp_vpn_passthrough_v6_mode_enum *enable_state,
  const uint32_t profile_handle,
  qmi_error_type_v01 *qmi_err_num
);

/*===========================================================================
FUNCTION SetL2TPIPSECVpnPassthroughIpv6()
===========================================================================*/
/** @ingroup qcmap_set_l2tp_ipsec_vpn_passthrough_v6_config

  sets IPV6 L2TP/IPSec VPN Passthrough

  @datatypes
  qcmap_lan_l2tp_ipsec_vpn_passthrough_v6_mode_enum

  @param[in]      enable_state              IPV6 L2TP/IPSec VPN Passthrough Enable state \n
  @param[in]      default_handle            Default Profile handle \n
  @param[in]      profile_handle            Current Profile Handle

  @return
  TRUE -- Success \n
  FALSE -- Failure \n
*/
/*=========================================================================*/
boolean
SetL2TPIPSECVpnPassthroughIpv6
(
  qcmap_lan_l2tp_ipsec_vpn_passthrough_v6_mode_enum enable_state,
  const uint32_t default_handle,
  const uint32_t profile_handle,
  qmi_error_type_v01 *qmi_err_num
);

/*===========================================================================
FUNCTION GetL2TPIPSECVpnPassthroughIpv6()
===========================================================================*/
/** @ingroup qcmap_get_l2tp_ipsec_vpn_passthrough_v6_config

  gets IPV6 L2TP/IPSec VPN Passthrough enable flag

  @datatypes
  qcmap_lan_l2tp_ipsec_vpn_passthrough_v6_mode_enum

  @param[in]      enable_state              IPV6 L2TP/IPSec VPN Passthrough Enable state \n
  @param[in]      profile_handle            Current Profile Handle

  @return
  TRUE -- Success \n
  FALSE -- Failure \n
*/
/*=========================================================================*/
boolean
GetL2TPIPSECVpnPassthroughIpv6
(
  qcmap_lan_l2tp_ipsec_vpn_passthrough_v6_mode_enum *enable_state,
  const uint32_t profile_handle,
  qmi_error_type_v01 *qmi_err_num
);

/*===========================================================================
  FUNCTION SendHWFilteringInfoToIPA
==========================================================================*/
/*!
@brief
  Sends Hardware Filtering Information to IPA through ioctl.

@parameters
  qcmap_config_state state
  qcmap_hw_mac_filter_config mac_filter_config

@return
  FALSE - on failure
  TRUE -  on success

@note
- Dependencies
- None

- Side Effects
- None
*/
/*=========================================================================*/

bool
SendHWFilteringInfoToIPA
(
  qcmap_hdw_filter_config              *hw_filter_config
);

/*===========================================================================
  FUNCTION GetSetHWFilteringStateFromUciConfig
==========================================================================*/
/*!
@brief
  action : GET - Gets HW Control Filtering Info from Uci Config.
  action : SET - Sets HW Control Filtering Info to Uci Config.

@parameters
  qcmap_action_type action
  qcmap_config_state *state
  qcmap_hdw_filter_config *hw_filter_config

@return
  FALSE - on failure
  TRUE -  on success

@note
- Dependencies
- None

- Side Effects
- None
*/
/*=========================================================================*/
bool
GetSetHWFilteringStateFromUciConfig
(
#ifndef FEATURE_QCMAP_OFFTARGET
  qcmap_action_type                   action,
#endif
  qcmap_config_state                  *state,
  qcmap_hdw_filter_config              *hw_filter_config
);

/*===========================================================================
  FUNCTION SetHWMACFilteringState()
  ===========================================================================*/
  /** @ingroup qcmap_set_hw_mac_filter_state

  Configure the HW Filtering State.

  @datatypes
  qcmap_config_state \n
  qcmap_hdw_filter_config \n
  @param[in] state                       Sets HW Filtering current state
  @param[in] hw_filter_config            HW Filter Configuration

  @return
  TRUE upon success. \n
  FALSE upon failure.

  @dependencies
  None
  */

  /*=========================================================================*/
bool
SetHWMACFilteringState
(
  qcmap_config_state                  state,
  qcmap_hdw_filter_config              hw_filter_config,
  qmi_error_type_v01                  *qmi_err_num
);

/*===========================================================================
  FUNCTION GetHWFilteringState
==========================================================================*/
/*!
@brief
  Gets the current HW Filtering State.

@parameters
  qcmap_config_state *state
  qcmap_hdw_filter_config *hw_filter_config

@return
  FALSE - on failure
  TRUE -  on success

@note
- Dependencies
- None

- Side Effects
- None
*/
/*=========================================================================*/
bool
GetHWFilteringState
(
  qcmap_config_state                  *status,
  qcmap_hdw_filter_config              *hw_filter_config,
  qmi_error_type_v01                  *qmi_err_num
);


/*===========================================================================
  FUNCTION ResetHWFilteringStateOnDisable
==========================================================================*/
/*!
@brief
  Reset Uci Config and IPA config when HW Filtering is disabled

@parameters
  qcmap_config_state state

@return
  FALSE - on failure
  TRUE -  on success

@note
- Dependencies
- None

- Side Effects
- None
*/
/*=========================================================================*/
bool
ResetHWFilteringStateOnDisable
(
  qcmap_config_state  state
);

/*===========================================================================
FUNCTION dns_list_to_string
===========================================================================*/
/*!
  @brief
  Converts array of dns name strings to one string,
  where each name is seprated by a ' ' character

  @parameters
    char                           *dns_string
    uint32_t                        buf_size
    dns_seach_list_info            *dns_list
    uint32_t                        list_len

  @return
  None

  @note
  - Dependencies
  - None

  - Side Effects
  - None
  */
/*=========================================================================*/
void
dns_list_to_string
(
  char                           *dns_string,
  uint32_t                        buf_size,
  dns_seach_list_info            *dns_list,
  uint32_t                        list_len
);

/*===========================================================================
  FUNCTION CheckForDuplicateFirewallRule
  ===========================================================================*/
/*!
  @brief
  Check For Duplicate Firewall Rule

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
boolean
CheckForDuplicateFirewallRule
(
  qcmap_msgr_firewall_entry_conf_t *firewall_entry,
  uint32_t wan_profile_handle,
  qmi_error_type_v01 *qmi_err_num
);

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
void PerformLinkToggle_ForIPPT
(
  const char *phy_iface,
  int profile_idx=0
);


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
void
PerformLinkToggle
(
  const char *phy_iface
);

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
void
CheckIPPTMode
(
  const uint32_t profile_handle,
  int event_type = 0
);

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
boolean
GetIPPTBridgeContext
(
  const int profile_idx,
  int16_t *bridge_context
);

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
boolean
GetMappedVLANPerPDN
(
  const int profile_idx,
  char *bridge_vlan_ids,
  qmi_error_type_v01 *qmi_err_num

);

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
boolean
CheckIfVLANMappedToIPPTPDN
(
  uint16_t vlan_id,
  int *profile_index,
  int *profile_num,
  qmi_error_type_v01 *qmi_err_num
);

/*===========================================================================
FUNCTION GetNumberOfProfiles()
===========================================================================*/
/** @ingroup qcmap_get_number_of_profiles

  gets number of profiles from qcmap_lan uci database

  @param[in]      no_of_profiles                 number of profiles

  @return
  TRUE -- Success \n
  FALSE -- Failure
*/
/*=========================================================================*/
boolean
GetNumberOfProfiles
(
  int *no_of_profiles
);

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
boolean
GetEnableIPPTValuePerPDN
(
  const int profile_idx
);

/*===========================================================================
FUNCTION GetPDNProfileNumber()
===========================================================================*/
/** @ingroup qcmap_get_pdn_profile_number

  gets the profile_id value saved per pdn from qcmap_lan uci database

  @param[in]      profile_idx                 profile index
  @param[in]      profile_id                  profile handle number

  @return
  TRUE -- Success \n
  FALSE -- Failure
*/
/*=========================================================================*/
boolean
GetPDNProfileNumber
(
  const int profile_idx,
  int *profile_id
);

/*===========================================================================
  FUNCTION DeleteDNSFromResolv
==========================================================================*/
/*!
@brief
  Deletes the DNS informatoin from resolv.conf file

@parameters
- char* primary dns addr
- char* secondary dns addr
- char* dns_search_str
- boolean pri_dns_valid
- boolean sec_dns_valid
- boolean dns_search_valid
- int max_buffer_len

@return
- None

@note
==========================================================================*/

void
DeleteDNSFromResolv
(
  char                           *pri_dns_addr,
  char                           *sec_dns_addr,
  char                           *dns_search_str,
  boolean                         pri_dns_valid,
  boolean                         sec_dns_valid,
  boolean                         dns_search_valid,
  int                             max_buffer_len
);
/*===========================================================================
  FUNCTION ConfigureNetworkOnEthPduModeChange
==========================================================================*/
/*!
@brief
  Configure Network On Eth Pdu Mode Change

@parameters
  eth pdu mode
  device indx

@return
  None

@note
- Dependencies
- None

- Side Effects
- None
*/
/*=========================================================================*/
void ConfigureNetworkOnEthPduModeChange(qcmap_lan_eth_pdu_feature_mode_enum EthPduMode,uint8_t index);

/*=====================================================================
  FUNCTION ActivateHostapdConfig
======================================================================*/
/*!
@brief
  - Activates the hostapd configuration from qcmap client

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
boolean
ActivateHostapdConfig
(
  qcmap_msgr_activate_hostapd_ap_enum_v01 ap_type,
  qcmap_msgr_activate_hostapd_action_enum_v01 action_type,
  qmi_error_type_v01 *qmi_err_num
);

/*=====================================================================
  FUNCTION ActivateSupplicantConfig
======================================================================*/
/*!
@brief
  - Activates the WPA supplicant configuration from qcmap client

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
boolean
ActivateSupplicantConfig(qmi_error_type_v01 *qmi_err_num);

/*=====================================================================
  FUNCTION GetV4PublicIP
======================================================================*/
/*!
@brief
  - Fetching the public IP from the /tmp/ipv4config File

@return
  true - Success
  false - Failure

@param
   char * ipv4_addr
   uint32_t profile_handle

@note
  - Dependencies
  - None

  - Side Effects
  - None
*/
/*=========================================================================*/
boolean
GetV4PublicIP
(
  char * ipv4_addr,
  uint32_t profile_handle
);

/*=====================================================================
  FUNCTION GetNetworkConfig
======================================================================*/
/*!
@brief
  - Fetching the config details from the config File

@return
  true - Success
  false - Failure

@param
   uint32_t profile_handle

@note
  - Dependencies
  - None

  - Side Effects
  - None
*/
/*=========================================================================*/
boolean
GetNetworkConfig
(
  qcmap_nw_params_t *qcmap_nw_params,
  uint32_t profile_handle,
  qcmap_ip_family_enum ip_type,
  qmi_error_type_v01 *qmi_err_num
);

/*=====================================================================
  FUNCTION GetV4NetworkConfig
======================================================================*/
/*!
@brief
  - Fetching the config details of IPV4 from the config File

@return
  true - Success
  false - Failure

@param
   uint32_t profile_handle

@note
  - Dependencies
  - None

  - Side Effects
  - None
*/
/*=========================================================================*/
static boolean
GetV4NetworkConfig
(
  qcmap_nw_params_t *qcmap_nw_params,
  uint32_t profile_handle,
  char *bh_present_v4,
  qmi_error_type_v01 *qmi_err_num
);

/*=====================================================================
  FUNCTION GetV6NetworkConfig
======================================================================*/
/*!
@brief
  - Fetching the config details of IPV6 from the config File

@return
  true - Success
  false - Failure

@param
   uint32_t profile_handle

@note
  - Dependencies
  - None

  - Side Effects
  - None
*/
/*=========================================================================*/
static boolean
GetV6NetworkConfig
(
  qcmap_nw_params_t *qcmap_nw_params,
  uint32_t profile_handle,
  char *bh_present_v6,
  qmi_error_type_v01 *qmi_err_num
);

/*=====================================================================
  FUNCTION GetETHPDUNetworkConfig
======================================================================*/
/*!
@brief
  - Fetching the config details of ETH PDU from the config File

@return
  true - Success
  false - Failure

@param
   uint32_t profile_handle

@note
  - Dependencies
  - None

  - Side Effects
  - None
*/
/*=========================================================================*/
static boolean
GetETHPDUNetworkConfig
(
  qcmap_nw_params_t *qcmap_nw_params,
  uint32_t profile_handle,
  qmi_error_type_v01 *qmi_err_num
);

/*===========================================================================
FUNCTION updateDNSSL
===========================================================================*/
/*!
  @brief
    Updates DNS Search List info
  @parameters
    qcmap_wwan_backhaul_info *bh_info

  @return
  None

  @note
  - Dependencies
  - None

  - Side Effects
  - None
  */
/*=========================================================================*/
void
updateDNSSL
(
  qcmap_wwan_backhaul_info *bh_info
);

/*====================================================================
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
/*==================================================================*/
boolean
UnLoadGSB();

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
boolean
EnableGSB(qmi_error_type_v01 *qmi_err_num);

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
/*=========================================================================*/
boolean
DisableGSB(qmi_error_type_v01 *qmi_err_num);

/*===========================================================================
FUNCTION SetGSBConfig()
===========================================================================*/
/*!
@brief
  - SetGSBConfig from qcmap client

@param[in]
  qcmap_gsb_config gsb_config

*/
boolean
SetGSBConfig
(
  qcmap_gsb_config *gsb_config,
  qmi_error_type_v01 *qmi_err_num
);

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
  qcmap_gsb_config *gsb_config
  uint8 *num_of_entries

@note
  - Dependencies
  - None

  - Side Effects
  - None
*/
/*=========================================================================*/
boolean
GetGSBConfig
(
  qcmap_gsb_config *gsb_config,
  uint8_t *num_of_entries,
  qmi_error_type_v01 *qmi_err_num
);


/*===========================================================================
FUNCTION DeleteGSBConfig()
===========================================================================*/
/*!
@brief
  - DeleteGSBConfig from qcmap client

@return
  true - Success
  false - Failure


@param[in]
  char* if_name

@note
  - Dependencies
  - None

  - Side Effects
  - None
*/
boolean
DeleteGSBConfig
(
  char* if_name,
  qmi_error_type_v01 *qmi_err_num
);

/*=====================================================================
  FUNCTION ReplaceConfigItem
======================================================================*/
/*!
@brief
  - Replaces given line with new line in a file

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
bool ReplaceConfigItem
(
  string    filename,
  string    lineToDelete,
  string    newLine
);

/*=====================================================================
  FUNCTION updateWWANMTU
======================================================================*/
/*!
@brief
  - Updates WWAN MTU towards LAN side

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
bool updateWWANMTU
(
  uint32_t       profile_idx,
  uint8_t        ip_type,
  uint16_t       mtu_info
);

/*=====================================================================
  FUNCTION UpdateWWANPolicy
======================================================================*/
/*!
@brief
  - Updates WWAN Policy on lan lib

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
bool UpdateWWANPolicy
(
  uint32_t  current_profile_handle,
  qmi_error_type_v01 *qmi_err_num
);

/*=====================================================================
  FUNCTION SetDhcpv6DNSConfig
======================================================================*/
/*!
@brief
  - Triggers add/del_dns_options when Proxy DNS v6 is enabled

@return
  - True if success else false

@param[in]
  -   qcmap_config_state     dhcpv6_dns_state
      qmi_error_type_v01    *qmi_err_num

@note
  - Dependencies
  - None

  - Side Effects
  - None
*/
/*=========================================================================*/
boolean SetDhcpv6DNSConfig
(
  qcmap_config_state     dhcpv6_dns_state,
  qmi_error_type_v01    *qmi_err_num
);

/*=====================================================================
  FUNCTION GetDhcpv6DNSConfig
======================================================================*/
/*!
@brief
  - Get current Proxy DNS v6 value

@return
  - True if success else false

@param[in]
  qcmap_msgr_config_state_enum_v01 *dhcpv6_dns_state
  qmi_error_type_v01               *qmi_err_num

@note
  - Dependencies
  - None

  - Side Effects
  - None
*/
/*=========================================================================*/
boolean GetDhcpv6DNSConfig
(
  qcmap_msgr_config_state_enum_v01 *dhcpv6_dns_state,
  qmi_error_type_v01               *qmi_err_num
);

/*===========================================================================
  FUNCTION SetUPNPState
  ===========================================================================*/
/*!
  @brief
  Sets the UPNP State

  @return
  true  - on Success
  false - on Failure

  - Dependencies
  - None

  - Side Effects
  - None
 */
/*=========================================================================*/
boolean SetUPNPState
(
  boolean upnp_pinhole_flag
);

/*===========================================================================
  FUNCTION GetUPNPStatus
  ===========================================================================*/
/*!
  @brief
  Gets the UPNP state

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
boolean GetUPNPState
(
  boolean *upnp_pinhole_flag,
  qmi_error_type_v01 *qmi_err_num
);

/*===========================================================================
  FUNCTION AddUPNPPinholeEntry
  ===========================================================================*/
/*!
  @brief
  Add a firewall configuration for UPnP

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

boolean AddUPNPPinholeEntry
(
  qcmap_msgr_firewall_conf_t *firewall_conf,
  uint32_t wan_profile_handle,
  boolean  upnp_pinhole,
  qmi_error_type_v01 *qmi_err_num
);

/*===========================================================================
  FUNCTION SwitchWlanEnableBand
===========================================================================*/
/*!
@brief
  This function is to switch sap/sta between 5GHz and 2.5GHz

@parameters
- sap_5g_enable_state
- sta_5g_enable_state

@return
  true
  false

@note
- Dependencies
- None

- Side Effects
- None
*/
/*=========================================================================*/
boolean SwitchWlanEnableBand
(
  qcmap_msgr_sap_band_status_enum_v01 sap_5g_enable_state,
  qcmap_msgr_sta_band_status_enum_v01 sta_5g_enable_state
);

/*===========================================================================
  FUNCTION ProcessStaStatusInd
===========================================================================*/
/*!
@brief
  This function is to process station associate/disassociate

@parameters
- sta_connection_state

@return
  true
  false

@note
- Dependencies
- None

- Side Effects
- None
*/
/*=========================================================================*/
boolean ProcessStaStatusInd
(
  qcmap_msgr_sta_connect_status_enum_v01 sta_connection_state
);

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
void
DisableEZMesh
(
  qcmap_msgr_ezmesh_mode_enum_v01  ezmesh_state
);

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
boolean
SetEZMeshConfig
(
  qcmap_msgr_ezmesh_mode_enum_v01   enable,
  boolean                           new_config,
  qcmap_ezmesh_config               *ezmesh_config,
  qmi_error_type_v01                *qmi_err_num
);

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
boolean
GetEZMeshConfig
(
  qcmap_msgr_ezmesh_mode_enum_v01  *ezmesh_status,
  qcmap_ezmesh_config     *ezmesh_config,
  qmi_error_type_v01      *qmi_err_num
);

/*===========================================================================
  FUNCTION GetEZMeshR2Config()
===========================================================================*/
/** @ingroup qcmap_get_ezmesh_config

  Get EZMesh R2 config upon getting request.

  @param[in]      qcmap_msgr_ezmesh_r2_config_v01 *ezmesh_r2_config

  @return
  void
*/
/*=========================================================================*/
void
GetEZMeshR2Config
(
  qcmap_msgr_ezmesh_r2_config_v01 *ezmesh_r2_config
);

/*===========================================================================
  FUNCTION ValidateEZMeshConfig()
===========================================================================*/
/** @ingroup qcmap_get_ezmesh_config

  Validate EZMesh config upon getting request from QCMAP to set ezmesh.

  @param[in]      qcmap_ezmesh_config *ezmesh_config

  @return
  TRUE -- Success \n
  FALSE -- Failure \n
*/
/*=========================================================================*/
boolean
ValidateEZMeshConfig
(
  qcmap_ezmesh_config               *ezmesh_config,
  qcmap_msgr_ezmesh_mode_enum_v01   enable
);

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
void
CreateEZMeshBridgeVLANContext
(
  const int16_t bridge_id
);

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
void
DeleteEZMeshBridgeVLANContext
(
  const int16_t bridge_id
);

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
boolean
ActivateEZMeshHostapdConfig
(
  qcmap_hostapd_ap_config_list *ap_list,
  qcmap_activate_hostapd_action_enum action_type,
  qmi_error_type_v01      *qmi_err_num
);

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
boolean
SetEZMeshServicePriority
(
  uint8_t                 service_prioritization_state_valid,
  uint8_t                 service_prioritization_state,
  qmi_error_type_v01      *qmi_err_num
);

boolean DisAssociateClient(const char* mac_addr_str);
boolean ResetWLANatBootup();
boolean IsWiFiDevcies(const char *phy_iface);
boolean InstallGuestAPRules();
boolean UnInstallGuestAPRules();
boolean IsWLANEnable();

/*===========================================================================
FUNCTION GetActiveLANConfig()
===========================================================================*/
/** @ingroup section_GetActiveLANConfig

  Gets LAN config

  @datatypes
  qcmap_lan_config \n

  @param[in] qcmap_lan_config   *lan_config
  @param[in].qmi_error_type_v01    *qmi_err_num

  @return
  bool

  @dependencies
  QCMobileAP must be enabled. @newpage
*/
/*=========================================================================*/
boolean
GetActiveLANConfig
(
   qcmap_lan_config *lan_config,
   qmi_error_type_v01 *qmi_err_num
);

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
  QCMobileAP must be enabled.
  IPsec feature mode must be enabled.
  No other feature allowed. @newpage
*/
/*=========================================================================*/
boolean SetIPsecTunnelInfo
(
  qcmap_ipsec_config_t *ipsec_config,
  qmi_error_type_v01 *qmi_err_num
);

/*===========================================================================
FUNCTION ActivateIPsecTunnelInfo()
===========================================================================*/
/** @ingroup section_SetIPsecTunnelInfo

  Activates IPsec Tunnel

  @param[in] char *ike_identifier
  @param[in] char *child_identifier
  @param[out] qmi_error_type_v01 *qmi_err_num

  @return
  TRUE -- Success \n
  FALSE -- Failure \n

  @dependencies
  QCMobileAP must be enabled. @newpage
  If child_identifier is not included, all the child_id under ike_id will be activated
  IPsec feature mode must be enabled.
  No other feature allowed.
*/
/*=========================================================================*/
boolean ActivateIPsecTunnelInfo
(
  char *ike_identifier,
  char *child_identifier,
  qmi_error_type_v01 *qmi_err_num
);


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
  If child_identifier is not included, all the child_id under ike_id will be deleted
  IPsec feature mode must be enabled.
  No other feature allowed.
*/
/*=========================================================================*/
boolean DeleteIPsecTunnel
(
  char *ike_identifier,
  char *child_identifier,
  qmi_error_type_v01 *qmi_err_num
);

/*===========================================================================
FUNCTION GetIPsecTunnelInfo()
===========================================================================*/
/** @ingroup section_GetIPsecTunnelInfo

  Get IPsec Tunnel Info

  @param[in] char *ike_identifier
  @param[in] char *child_identifier
  @param[in] qcmap_ipsec_config_t   *ipsec_config
  @param[out] qmi_error_type_v01 *qmi_err_num

  @return
  TRUE -- Success \n
  FALSE -- Failure \n

  @dependencies
  QCMobileAP must be enabled. @newpage
  All the child SAs under the IKE_id will be returned
  IPsec feature mode must be enabled.
  No other feature allowed.
*/
/*=========================================================================*/
boolean GetIPsecTunnelInfo
(
  char *ike_identifier,
  char *child_identifier,
  qcmap_ipsec_config_t *ipsec_config,
  qmi_error_type_v01 *qmi_err_num
);

/*===========================================================================
FUNCTION GetIPsecTunnelStateInfo()
===========================================================================*/
/** @ingroup section_GetIPsecTunnelStateInfo

  Get IPsec Tunnel State Info

  @param[in] char *ike_identifier
  @param[in] char *child_identifier
  @param[in] qcmap_ipsec_tunnel_state_info_t *state_info
  @param[out] qmi_error_type_v01 *qmi_err_num

  @return
  TRUE -- Success \n
  FALSE -- Failure \n

  @dependencies
  QCMobileAP must be enabled. @newpage
  All the child SAs under the IKE_id will be returned
  IPsec feature mode must be enabled.
  No other feature allowed.
*/
/*=========================================================================*/
boolean GetIPsecTunnelStateInfo
(
  char *ike_identifier,
  char *child_identifier,
  qcmap_ipsec_tunnel_state_info_t *state_info,
  qmi_error_type_v01 *qmi_err_num
);


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
boolean ResetIPsecFeatureMode
(
  qcmap_lan_client_feature_mode_config *feature_mode_config,
  qmi_error_type_v01                  *qmi_err_num
);

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
boolean GetIPsecFeatureMode
(
  uint64_t                             *enabled_features,
  qcmap_lan_client_feature_mode_config *feature_mode_config,
  qmi_error_type_v01                   *qmi_err_num
);

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
boolean SetIPsecFeatureMode
(
  qcmap_lan_client_feature_mode_config *feature_mode_config,
  qmi_error_type_v01 *qmi_err_num
);

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
boolean ResetIPPTFeatureMode
(
  qcmap_lan_client_feature_mode_config *feature_mode_config,
  qmi_error_type_v01                  *qmi_err_num
);

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
boolean GetIPPTFeatureMode
(
  uint64_t                             *enabled_features,
  qcmap_lan_client_feature_mode_config *feature_mode_config,
  qmi_error_type_v01                   *qmi_err_num
);

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
boolean SetIPPTFeatureMode
(
  qcmap_lan_client_feature_mode_config *feature_mode_config,
  qmi_error_type_v01 *qmi_err_num
);

/*===========================================================================
FUNCTION GetFireWallHandlesList
===========================================================================
@brief
  Get all the firewall Handle list for Mobile AP

@input
  wan_profile_handle - current wan profile handle
  *handlelist  - pointer to qcmap_msgr_get_firewall_handle_list_conf_t
  qmi_err_num

@return
  void

@dependencies
  usr to provide input

@sideefects
  None
=========================================================================*/
boolean
GetFireWallHandlesList
(
  uint32_t wan_profile_handle,
  qcmap_msgr_get_firewall_handle_list_conf_t *handlelist,
  qmi_error_type_v01 *qmi_err_num
);

/*===========================================================================
FUNCTION GetFireWallEntry_by_handle
===========================================================================
@brief
  Get all the firewall entry using the handle

@input
  ip_version
  wan_profile_handle
  firewall_entry
  qmi_err_num


@return
  boolean

@dependencies
  usr to provide input

@sideefects
  None
=========================================================================*/

boolean GetFireWallEntry_by_handle
(
  qcmap_msgr_firewall_entry_conf_t  *firewall_entry,
  qmi_error_type_v01                *qmi_err_num
);


/*===========================================================================
FUNCTION Get_Index_by_handle
===========================================================================
@brief
   get the index of a firewall entry using handle
@input
  wan_profile_handle
  qmi_err_num


@return
  boolean

@dependencies
  usr to provide input

@sideefects
  None
=========================================================================*/
int Get_Index_by_handle
(
  uint32 handle,
  qmi_error_type_v01                *qmi_err_num
);

/*===========================================================================
FUNCTION GetIPPTStatus()
===========================================================================*/
/** @ingroup qcmap_get_ippt_status

  Function updates enable_status if IPPT WITH NAT FCD/ IPPT WITOUT_NAT is
  enabled on the bridge passed

  @param[in]     enable_status                profile index
  @param[in]     bridge_id                    current_bridge_context
  @param[in]     qmi_err_num                  error no

  @return
  TRUE --  Success \n
  FALSE -- Failure
*/
/*=========================================================================*/
boolean GetIPPTStatus
(
  bool *enable_status,
  int16_t bridge_id,
  qmi_error_type_v01 *qmi_err_num
);

/*===========================================================================
  FUNCTION CheckIpptFeatureModeWithFirewallSupport
  ===========================================================================*/
/*!
  @brief
  Function will check whether if IPPt is set w/o NAT mode and firewall support
  flag is set or not. If Both are set, will not allow ipv4 firewall rule addition.

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
boolean
CheckIpptFeatureModeWithFirewallSupport
(
  qmi_error_type_v01 *qmi_err_num
);

/*==========================================================================
  FUNCTION SetV4NATconfig
  ===========================================================================*/
/*!
  @brief
  Set IPv4 NAT Configuration.This function is dependent on the network side configurations
  to work as expected. After enabling IPv4 NAT disable configuration,  data  packets  with
  source address as LAN IP will go out to network from UE.By default, NAT will be enabled.

  @datatypes
  uint32_t
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
boolean
SetV4NATConfig
(
  uint32_t            wan_profile_handle,
  boolean             &ipv4_nat_disable,
  qmi_error_type_v01  *qmi_err_num
);

/*===========================================================================
  FUNCTION SetIPv6PDManager
  ===========================================================================*/
/*!
  @brief
  Set Ipv6 pd_manager to qcmap_lan.

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
boolean
SetIPv6PDManager
(
  bool enable
);

/*===========================================================================
  FUNCTION SetIPv6PDActivatedConfig
  ===========================================================================*/
/*!
  @brief
  Set/delete Ipv6 Prefix delegation activated config.
  IDU mode, Disable the PD first, will set this config to 0

  @return
  true	- on Success
  false - on Failure

  @note

  - Dependencies
  - None

  - Side Effects
  - None
 */
/*=========================================================================*/
boolean
SetIPv6PDActivatedConfig
(
  int value
);

/*===========================================================================
  FUNCTION RecyclePrefixForModeChange
  ===========================================================================*/
/*!
  @brief
  When change mode, Legacy->IDU or IDU->Legacy, need restart tethered clients to recycle prefix first

  @return
  true	- on Success
  false - on Failure

  @note

  - Dependencies
  - None

  - Side Effects
  - None
 */
/*=========================================================================*/
boolean
RecyclePrefixForModeChange
(
  bool enable
);

/*===========================================================================
  FUNCTION SetExtRouterModeEnabled
  ===========================================================================*/
/*!
  @brief
  set/delete external router mode enabled config to qcmap_lan

  @return
  true	- on Success
  false - on Failure

  @note

  - Dependencies
  - None

  - Side Effects
  - None
 */
/*=========================================================================*/
boolean
SetExtRouterModeEnabled
(
  int enable
);

/*===========================================================================
  FUNCTION GetV4NATConfig
  ===========================================================================*/
/*!
  @brief
  Get Current Status of IPv4 NAT Configuration.


  @datatypes
  uint32_t
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
boolean
GetV4NATConfig
(
  uint32_t            wan_profile_handle,
  boolean             &ipv4_nat_disable,
  qmi_error_type_v01  *qmi_err_num
);

boolean DisableIPV4
(
   qmi_error_type_v01 *qmi_err_num
);

boolean DisableIPV6
(
   qmi_error_type_v01 *qmi_err_num
);

boolean EnableIPV4
(
   qmi_error_type_v01 *qmi_err_num
);

boolean EnableIPV6
(
   qmi_error_type_v01 *qmi_err_num
);

/*===========================================================================
FUNCTION IsLinkDetected()
===========================================================================*/
/** @ingroup section_IsTetheredLinkUp

  Checks if link is up

  @param[in] char *ifname

  @return
  TRUE -- Success \n
  FALSE -- Failure \n

  @dependencies
  None
*/
/*=========================================================================*/
boolean IsLinkDetected
(
   char *ifname
);

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
boolean SetIPPassthroughSoftwarePathFilters
(
 qcmap_msgr_sw_path_filters_conf_t       *filter_config,
 const uint32_t default_handle,
 const uint32_t profile_handle,
 qmi_error_type_v01 *qmi_err_num
);

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
boolean GetIPPassthroughSoftwarePathFilters
(
  qcmap_msgr_sw_path_filters_conf_t        *filter_config,
  const uint32_t default_handle,
  const uint32_t profile_handle,
  qmi_error_type_v01                       *qmi_err_num
);
};

/*=====================================================================
  FUNCTION GetVlanIndex
======================================================================*/
/*!
@brief
  - Get VLAN index of passed vlan_id in qcmap_lan database

@return
  vlan_id

@note
  - returns the VLAN index of passed vlan_id in qcmap_lan database

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
int
GetVlanIndex
(
  const uint32_t vlan_id
);


/*===========================================================================
 FUNCTION readable_addr
  ===========================================================================
  @brief
  converts the a numeric address into a text string suitable
  for presentation
  @input
  domain - identifies ipv4 or ipv6 domain
  addr   - contains the numeric address
  str    - this is an ouput value contains address in text string
  @return
  0  - success
  -1 - failure
  @dependencies
  It depends on inet_ntop()
  @sideefects
  None
  =========================================================================*/

int
readable_addr
(
  int domain,
  const uint32 *addr,
  char *str
);

/*===========================================================================
FUNCTION CountNumOfSetBits()
===========================================================================*/
/** @ingroup section_CountNumOfSetBits

  Count Number Of Set Bits in a Number

  @param[in] uint32_t  number

  @return
  int -> num of set bits

  @dependencies
  QCMobileAP must be enabled. @newpage
*/
/*=========================================================================*/
int CountNumOfSetBits(uint32_t num);

boolean GetWWANInfo(uint32_t *default_handle, uint32_t *profile_handle);

/*=====================================================================
  FUNCTION GetMappedPDNforVlan
======================================================================*/
/*!
@brief
  - Get mapped PDN for a given VLAN index

@return
  profile_handle/PDN

@note
  - returns the mapped PDN to passed vlan_id in qcmap_lan database

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
int GetMappedPDNforVlan
(
  const uint32_t vlan_idx,
  qmi_error_type_v01 *qmi_err_num
);


#endif /* _QCMAP_LAN_CLIENT_H_ */
