/*!
  @file
  qcmap_cli_main.cpp

  @brief
  basic QCMAP Command Line Module Client Main

  Copyright (c) 2011-2023 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Confidential and Proprietary - Qualcomm Technologies, Inc.

*/
/*=========================================================================*/

/*=========================================================================

                        EDIT HISTORY FOR MODULE

This section contains comments describing changes made to the module.
Notice that changes are listed in reverse chronological order.

$Header: $

when       who     what, where, why
--------   ---     ----------------------------------------------------------
07/11/12   gk      Created module.
10/26/12   cp      Added support for Dual AP and different types of NAT.
02/27/13   cp      Added support to get IPV6 WAN status.
04/17/13   mp      Added support to get IPv6 WWAN/STA mode configuration.
06/12/13   sg      Added support for DHCP reservation.
09/17/13   at      Added support to Enable/Disable ALGs
01/11/14   sr      Added support for connected devices in SoftAP
03/25/17   spr     Added support for Multi-PDN.
05/20/17   gs      Added GSB Support
===========================================================================*/

/*===========================================================================

                     INCLUDE FILES FOR MODULE

===========================================================================*/
#include <string>
#ifndef FEATURE_EXTERNAL_AP
#include "comdef.h"
#endif /*FEATURE_EXTERNAL_AP */
#include "QCMAP_Client.h"
#include "limits.h"
#include "ds_util.h"
#include "QCMAP_LAN_Client.h"
#include "QCMAP_LAN_Multimedia.h"
#include <algorithm>
#include <stdarg.h>

/* The original delay for dss_init was 6 seconds. Allowing this value to be
   set at runtime to determine if delay still necessary. */
#define QCMAP_DEFAULT_DSS_INIT_TIME    6
#define MAX_PORT_VALUE           65535
#define MIN_DHCP_LEASE 120 /*Lease time in seconds */
#define MIN_NOTIFY_INTERVAL 30
#define MAX_NOTIFY_INTERVAL 60000
#define MAC_HEX_STRING "0123456789abcdefABCDEF" /*MAC hex check*/
#define MAC_NULL_STRING "00:00:00:00:00:00" /*MAC Null String*/
#define INET_ADDRSTRLEN        16
#define INET6_ADDRSTRLEN       46
#define QCMAP_MAX_FIREWALL_ENTRY 128    /*will replace this once define in IDL*/

#define CLI_DEBUG_OPTION             500
#define ENABLE_STA_ONLY_DEBUG_MODE   CLI_DEBUG_OPTION+1
#define DISABLE_STA_ONLY_DEBUG_MODE  CLI_DEBUG_OPTION+2
#define REGISTER_FOR_WLAN_STATUS_IND CLI_DEBUG_OPTION+3

#define MAX_BACKHAUL_SUPPORTED       5
#define MAX_UINT32_VAL               4294967295   /*2^32 - 2*/
#define MAX_UINT16_VAL               65535        /*2^16 - 1*/
#define MAX_UINT8_VAL               255        /*2^8 - 1*/
#define MAX_VLAN_ID                  4094/*vlan 4095 is max and it is reserved*/
#define MIN_VLAN_ID                  1
#define MAX_BACKHAUL_TYPE_LENGTH     128
#define IPA_MAX_IFACE_FILTERING      4

#define MTPE_DL_TRIGGER_CODE 0x64646666

#define MIN_R1_EZMESH_AP              2
#define MIN_R2_EZMESH_AP              3
#define MAX_R1_EZMESH_AP              12
#define MAX_R2_EZMESH_AP              14
#define MIN_R2_EZMESH_VLAN_MAPPING    2
#define MAX_R2_EZMESH_VLAN_MAPPING    4

uint8 mac_addr_int[QCMAP_MSGR_MAC_ADDR_LEN_V01] = {0}; /*byte array of mac address*/
char mac_addr_str[QCMAP_MSGR_MAC_ADDR_NUM_CHARS_V01] = {0}; /*char array of mac address*/
/*if any client doesnt need all indications to be registerd by default,
    use QCMAP_DEFAULT_IND_MASK instead which has minimal mask*/
unsigned long indication_mask = QCMAP_REG_ALL_IND_MASK;

#define QCMAP_CLI_LOG(...)                         \
  fprintf( stderr, "\n%s %d:", __FILE__, __LINE__); \
  fprintf( stderr, __VA_ARGS__ )

#define WLAN_CARD_TYPE(type) ((type == QCMAP_MSGR_WLAN_DEV_TUF_V01)? "TUFFELO" : \
                              ((type == QCMAP_MSGR_WLAN_DEV_ROME_V01)? "ROME": \
                                ((type == QCMAP_MSGR_WLAN_DEV_HAST_V01)? "HASTINGS": \
                                 ((type == QCMAP_MSGR_WLAN_DEV_PINE_V01)? "PINE": \
                                  ((type == QCMAP_MSGR_WLAN_DEV_HMT_V01)? "HAMILTON": \
                                    ((type == QCMAP_MSGR_WLAN_DEV_WKK_V01)? "WAIKIKI": \
                                  "INVALID"))))))

#define WLAN_STATE_TYPE(type) ((type == QCMAP_MSGR_WLAN_IFACE_ACTIVE_V01)? "Enabled" : "Disabled")

#define WLAN_AP_TYPE(type) \
  ((type == QCMAP_MSGR_WLAN_IFACE_PRIMARY_AP_V01)? "Primary" : \
    ((type == QCMAP_MSGR_WLAN_IFACE_GUEST_AP_ONE_V01) ? "Guest AP" : \
      ((type == QCMAP_MSGR_WLAN_IFACE_GUEST_AP_TWO_V01) ? "Guest AP 2" : \
       ((type == QCMAP_MSGR_WLAN_IFACE_STATION_V01) ? "Station" : \
        ((type == QCMAP_MSGR_WLAN_IFACE_GUEST_AP_THREE_V01) ? "Guest AP 3" : \
         ((type == QCMAP_MSGR_WLAN_IFACE_GUEST_AP_FOUR_V01) ? "Guest AP 4" : \
          ((type == QCMAP_MSGR_WLAN_IFACE_GUEST_AP_FIVE_V01) ? "Guest AP 5" : \
           ((type == QCMAP_MSGR_WLAN_IFACE_GUEST_AP_SIX_V01) ? "Guest AP 6" : \
            ((type == QCMAP_MSGR_WLAN_IFACE_GUEST_AP_SEVEN_V01) ? "GuestAP 7" : \
             ((type == QCMAP_MSGR_WLAN_IFACE_GUEST_AP_EIGHT_V01) ? "Guest AP 8" : \
              ((type == QCMAP_MSGR_WLAN_IFACE_GUEST_AP_NINE_V01) ? "Guest AP 9" : \
               ((type == QCMAP_MSGR_WLAN_IFACE_GUEST_AP_TEN_V01) ? "GuestAP 10" : \
                ((type == QCMAP_MSGR_WLAN_IFACE_GUEST_AP_ELEVEN_V01) ? "Guest AP 11" : \
                ((type == QCMAP_MSGR_WLAN_IFACE_GUEST_AP_TWELVE_V01) ? "Guest AP 12" : \
                 ((type == QCMAP_MSGR_WLAN_IFACE_GUEST_AP_THIRTEEN_V01) ? "Guest AP 13" : \
                 (type == QCMAP_MSGR_WLAN_IFACE_MLD_AP) ? "MLD AP" : "NA")))))))))))))))

#define WLAN_AP_PROFILE(type) ((type == QCMAP_MSGR_PROFILE_FULL_ACCESS_V01)? "Full Access" : "Internet Only Access")

#define EZMESH_STATE_TYPE(type) ((type == QCMAP_MSGR_EZMESH_ENABLE_V01)? "Enabled" : \
                                 (type == QCMAP_MSGR_EZMESH_DISABLE_V01)? "Disabled" : "INVALID")

#define EZMESH_CAPABILITY_TYPE(type) ((type == QCMAP_MSGR_EZMESH_R1_capability_V01)? "R1" : \
                                       ((type == QCMAP_MSGR_EZMESH_R2_capability_V01)? "R2": \
                                        ((type == QCMAP_MSGR_EZMESH_R3_capability_V01)? "R3": \
                                         (type == QCMAP_MSGR_EZMESH_R4_capability_V01)? "R4": "INVALID")))
#define EZMESH_AP_TYPE(type) \
  ((type == QCMAP_MSGR_PRIMARY_FRONTHAUL_AP_V01)? "Primary FH AP" : \
    ((type == QCMAP_MSGR_GUEST_FRONTHAUL_AP_V01) ? "Guest FH AP" : \
      ((type == QCMAP_MSGR_GUEST_FRONTHAUL_AP_2_V01) ? "Guest FH AP 2" : \
       ((type == QCMAP_MSGR_GUEST_FRONTHAUL_AP_3_V01) ? "Guest FH AP 3" : \
        ((type == QCMAP_MSGR_ADDITIONAL_FH_AP_1_V01) ? "Additional FH AP" : \
         ((type == QCMAP_MSGR_ADDITIONAL_FH_AP_2_V01) ? "Additional FH AP 2" : \
          ((type == QCMAP_MSGR_ADDITIONAL_FH_AP_3_V01) ? "Additional FH AP 3" : \
           ((type == QCMAP_MSGR_BACKHAUL_AP_R1_V01) ? "BH AP(R1)" : \
            ((type == QCMAP_MSGR_BACKHAUL_AP_R2_V01) ? "BH AP(R2)" : \
             (type == QCMAP_MSGR_SMART_MONITOR_AP_V01) ? "Smart Monitor AP" : "INVALID")))))))))

#define EZMESH_FH_AP_TYPE(type) \
  ((type == QCMAP_MSGR_PRIMARY_FRONTHAUL_AP_V01)? "Primary FH AP" : \
    ((type == QCMAP_MSGR_GUEST_FRONTHAUL_AP_V01) ? "Guest FH AP" : \
      ((type == QCMAP_MSGR_GUEST_FRONTHAUL_AP_2_V01) ? "Guest FH AP 2" : \
       (type == QCMAP_MSGR_GUEST_FRONTHAUL_AP_3_V01) ? "Guest FH AP 3" : "INVALID")))

#define CONVERT_IPV4_NETWORK_ADDRESS_TO_STRING(ipv4_nw_address, ipv4_str)           \
          {                                                                         \
            struct sockaddr_in              ipv4_addr;                              \
            ipv4_addr.sin_addr.s_addr = ipv4_nw_address;                            \
            inet_ntop(AF_INET, &(ipv4_addr.sin_addr), ipv4_str, INET_ADDRSTRLEN);   \
          }

#define CONVERT_IPV6_NETWORK_ADDRESS_TO_STRING(ipv6_nw_address, ipv6_str)            \
          {                                                                          \
            struct sockaddr_in6             ipv6_addr;                               \
            memcpy(ipv6_addr.sin6_addr.s6_addr, ipv6_nw_address,                     \
                       sizeof(ipv6_addr.sin6_addr.s6_addr));                         \
            inet_ntop(AF_INET6, &(ipv6_addr.sin6_addr), ipv6_str, INET6_ADDRSTRLEN); \
          }

#define ASK_USER_FOR_INPUT_INT_PARAM(userText, userInput)                            \
           {                                                                         \
              printf("   " userText );                                               \
              fflush(stdout);                                                        \
              if (fgets(scan_string, sizeof(scan_string), stdin) != NULL)            \
              {                                                                      \
                userInput = atoi(scan_string);                                       \
              }                                                                      \
              printf("\n");                                                          \
           }

#define ASK_USER_FOR_MANDATORY_PARAM(userText, dataType, param)                   \
           {                                                                      \
              printf("   Enter " #userText " dataType " #dataType " : ");         \
              fflush(stdout);                                                     \
              if (fgets(scan_string, sizeof(scan_string), stdin) != NULL)         \
              {                                                                   \
                param = atoi(scan_string);                                        \
              }                                                                   \
              printf("\n");                                                       \
           }

#define ASK_USER_FOR_OPTIONAL_PARAM(userText, dataType, param)                  \
           {                                                                    \
              int userInput = 0;                                                \
              printf("   Do you want to enter " #userText " (1-Yes, 0-No) : "); \
              fflush(stdout);                                                   \
              if (fgets(scan_string, sizeof(scan_string), stdin) != NULL)       \
              {                                                                 \
                userInput = atoi(scan_string);                                  \
              }                                                                 \
              if (userInput == 1)                                               \
              {                                                                 \
                printf("   Enter Value (dataType/Values=" #dataType ") : ");    \
                if (fgets(scan_string, sizeof(scan_string), stdin) != NULL)     \
                {                                                               \
                  param = atoi(scan_string);                                    \
                }                                                               \
                param ## _valid = true;                                         \
              }                                                                 \
              printf("\n");                                                     \
           }

#define ASK_USER_FOR_OPTIONAL_PARAM_LIST(userText, dataType, param)                   \
            {                                                                         \
              printf("   Do you want to enter " #userText " List: (1=Yes, 0=No) : "); \
              if (fgets(scan_string, sizeof(scan_string), stdin) != NULL)             \
              {                                                                       \
                int userInput = 0;                                                    \
                userInput = atoi(scan_string);                                        \
                if (userInput == 1)                                                   \
                {                                                                     \
                  printf("   How many " #userText " List, you want to add? :");       \
                  if (fgets(scan_string, sizeof(scan_string), stdin) != NULL)         \
                  {                                                                   \
                    int list_len = 0;                                                 \
                    list_len = atoi(scan_string);                                     \
                    if (list_len > 0)                                                 \
                    {                                                                 \
                      for (int i=0; i < list_len; i++)                                \
                      {                                                               \
                        printf("   Enter " #userText "[%d] dataType " #dataType " : ", i); \
                        if (fgets(scan_string, sizeof(scan_string), stdin) != NULL)   \
                        {                                                             \
                          param[i] = atoi(scan_string);                               \
                        }                                                             \
                      }                                                               \
                      param ## _valid = true;                                         \
                      param ## _len   = list_len;                                     \
                    }                                                                 \
                  }                                                                   \
                }                                                                     \
              }                                                                       \
            }

#define SHOW_MANDATORY_RESPONSE_TO_USER(userText, param)                      \
           {                                                                  \
             printf ( "   " #userText " : %d\n", param);                      \
           }

#define SHOW_OPTIONAL_RESPONSE_TO_USER(userText, param)                       \
           {                                                                  \
             if (param ## _valid)                                             \
               printf ( "   " #userText " : %d\n", param);                    \
           }

#define INDENT_TEXT printf("   ");

#define SHOW_OPTIONAL_RESPONSE_LIST_TO_USER(userText, param)               \
           {                                                               \
              if (param ## _valid)                                         \
              {                                                            \
                 printf("   " #userText, " Length=%d\n", param ## _len);   \
                 for (int i=0; i< param ## _len; i++)                      \
                 {                                                         \
                    printf ("      Item[%d]=%d\n", i, param[i]);           \
                 }                                                         \
               }                                                           \
             }

#define SHOW_INDICATION_MSG_TO_USER(indText)                               \
            {                                                              \
              printf("   Received Indication - " #indText "\n");           \
            }

#define IS_CHAR_ALPHA_NUM(ch)           \
         ((ch >='a' && ch <= 'z') ||    \
          (ch >= '0' && ch <= '9' ))    \

#define IS_CHAR_NUM(ch)                 \
        (ch >= '0' && ch <= '9')        \

#define VALID_STR_INPUT(scan_string)                                         \
        (!(scan_string[0] == '\0' || scan_string[0] == '\n'))                \

#define VALID_NUMERIC_INPUT(scan_string)                                  \
        (VALID_STR_INPUT(scan_string) && IsStrNum(scan_string))           \

#define VALID_IF_NAME_CHAR(s) ((((s >= 'a') && (s <= 'z')) || \
                             ((s >= '0') && (s <= '9')) || \
                              (s == '-') || (s == '_')) ? true : false)

#define IS_NON_PORT_BASED_PROTO(protocol)((protocol == PS_IPPROTO_ICMP) || \
                                          (protocol == PS_IPPROTO_ICMP6) || \
                                          (protocol == PS_IPPROTO_ESP)       \
                                           ? true : false)

#define READ_AND_VALIDATE_INT_VALUE(input_var, min_val, max_val)                    \
        while(1)                                                                    \
        {                                                                           \
          memset(scan_string, 0, QCMAP_MSGR_MAX_FILE_PATH_LEN);                     \
          fflush(stdout);                                                           \
          if(strlen(scan_string) == 0 && scan_string[0] == '\0' )                   \
          {                                                                         \
              fgets(scan_string, sizeof(scan_string), stdin);                           \
              input_var = atoi(scan_string);                                            \
              if (!VALID_NUMERIC_INPUT(scan_string) ||                                  \
                  (input_var < min_val) || (input_var > max_val))                       \
              {                                                                         \
                printf("\nInvalid input. Please provide value in range [%d - %d]: ",    \
                       min_val, max_val);                                               \
                continue;                                                               \
              }                                                                         \
          }                                                                            \
          break;                                                                    \
        }                                                                           \

#define READ_AND_VALIDATE_ARRAY_VALUE(input_var, array_val, validate_string)        \
        while (1)                                                                   \
        {                                                                           \
          memset(scan_string, 0, QCMAP_MSGR_MAX_FILE_PATH_LEN);                     \
          fflush(stdout);                                                           \
          if (strlen(scan_string) == 0 && scan_string[0] == '\0')                   \
          {                                                                           \
            fgets(scan_string, sizeof(scan_string), stdin);                           \
            if(*scan_string == '\n')                                                  \
              printf("\nEmpty input. Please provide validate value(%s):",validate_string); \
            else                                                                      \
            {                                                                         \
              input_var = atoi(scan_string);                                          \
              int arr_len = sizeof(array_val) / sizeof(array_val[0]);                 \
              int correct_num = 0;                                                    \
              for(int i = 0; i < arr_len; i ++)                                       \
              {                                                                       \
                if(array_val[i] == input_var)                                         \
                {                                                                     \
                  correct_num++ ;                                                     \
                  break;                                                              \
                }                                                                     \
              }                                                                       \
              if(correct_num == 0)                                                    \
              {                                                                       \
                printf("\nInvalid input. Please provide validate value(%s):",validate_string); \
                continue;                                                             \
              }                                                                       \
              break;                                                                  \
            }                                                                         \
          }                                                                           \
          break;                                                                      \
      }                                                                             \


boolean read_firewall_conf(qcmap_msgr_firewall_conf_t *extd_firewall_add);
void Dump_firewall_conf( qcmap_msgr_firewall_entry_conf_t *firewall_entry);

/* CLI restructure function declarations */
void mobileApConfig(int);
void lanConfig(int);
void nat_alg_vpn_config(int);
void wlanConfig(int);
void firewallConfig(int);
void backhaulConfig(int);
void backhaulCommConfig(int);
profile_handle_type_v01 ChooseWWANProfileHandle();
int16_t ChooseLANBridge(void);
void backhaulWWANConfig(int);
void backhaulWWANUpdateConfig( int );
void tetheringConfig(int);
void mediaServiceConfig(int);
void v2xServiceConfig(int);
void eogreTunnelConfig(int);
void getConnectedDevicesInfoConfig(int);
void ipsecConfig(int);

/* private static helper functions for SOCKSv5 Configuration */
static boolean EnableDisableSOCKSv5(qmi_error_type_v01 *qmi_err_num);
static boolean SetSOCKSv5Config(qmi_error_type_v01 *qmi_err_num);
static boolean PromptUserSOCKSv5UnameAssoc(qmi_error_type_v01 *qmi_err_num);
static boolean GetUserInputSOCKSv5AuthMethod(unsigned char *auth_method);
static boolean GetUserInputSOCKSv5LANIface(char *lan_iface);
static boolean GetUserInputSOCKSv5ConfigFilePath(char* conf_file, char* auth_file);
static boolean GetUserInputSOCKSv5Uname(char *uname);
static boolean GetUserInputSOCKSv5ServiceNo(unsigned int *service_no);
static boolean CheckSOCKSv5UnameLen(char *str);

/*private static helper function for ipsec configuration*/
static boolean setHostToHostConfig(qcmap_ipsec_config_t *ipsec_config);
static boolean setSiteToSiteWithoutNATConfig(qcmap_ipsec_config_t *ipsec_config);

QCMAP_Client *QcMapClient = NULL;
boolean is_ipv6nat_enabled = false;

const char* options_list[]=
{
"1.  MobileAP Configuration                   ",
"2.  LAN Configuration                        ",
"3.  NAT/ALG/VPN Configuration                ",
"4.  WLAN Configuration                       ",
"5.  Firewall Configuration                   ",
"6.  Backhaul Configuration                   ",
"7.  Tethering Configuration                  ",
"8.  Media Service Configuration              ",
"9.  Generic Software Bridge Configuration    ",
"10. V2X Service Configuration               ",
"11. Modem Throughput Estimation Configuration",
"12. IPsec Configuration(WWAN)"
};


const char* mobileAp_configuration_list[]=
{
"1.  Display Current Config                                   ",
"2.  Enable/Disable mobileap                                  ",
"3.  Get MobileAP status                                      ",
"4.  Get Connected Device info                                ",
"5.  Enable/Disable/Reset Packet Stats                        ",
"6.  Get Packet Stats Status                                  ",
"7.  Restore Factory Default Settings(** Will Reboot Device ) ",
"8.  Teardown/Disable and Exit                                ",
"9.  Set Data Path Optimization Flag                          ",
"10. Get Data Path Optimization Flag                          ",
"11. Set Device Mode                                          ",
"12. Get Device Mode                                          ",
"13. Set Client Configuration                                 ",
"14. Set N79 config                                           ",
"15. Get N79 config                                           ",
"16. Register for indications with indication mask            ",
"17. Set Feature Mode                                         ",
"18. Get Feature Mode                                         ",
"19. Reset Feature Mode                                       ",
"20. Set Client Preference (Profile handle, Subs_Id, etc)     ",
"21. Get Client Preference (Profile handle, Subs_Id, etc)     ",
"22. Unregister for indications with indication mask          "
};

const char* lan_configuration_list[]=
{
"1.  Set  LAN Config                                           ",
"2.  Get  LAN Config                                           ",
"3.  Activate  LAN                                             ",
"4.  Add DHCP Reservation Record                               ",
"5.  Get DHCP Reservation Records                              ",
"6.  Edit DHCP Reservation Record                              ",
"7.  Delete DHCP Reservation Record                            ",
"8.  Set/Get Gateway URL                                       ",
"9.  Add VLAN Interface                                        ",
"10. Get VLAN Interfaces                                       ",
"11. Delete VLAN Interface                                     ",
"12. Set L2TP Unmnaged Tunnel state                            ",
"13. Set L2TP Config                                           ",
"14. Get L2TP Config                                           ",
"15. Delete L2TP Config                                        ",
"16. Set Bridge-VLAN Context                                   ",
"17. Get Bridge-VLAN Context                                   ",
"18. Set Early Ethernet Mode                                   ",
"19. Get Early Ethernet Mode                                   ",
"20. Get SW IP Channel configuration                           ",
"21. Setup SW IP Channel Interface                             "
};

const char* nat_alg_vpn_configuration_list[]=
{
"1.  Add SNAT Entry                                           ",
"2.  Delete SNAT Entry                                        ",
"3.  Get SNAT Config                                          ",
"4.  Set NAT Type                                             ",
"5.  Get NAT Type                                             ",
"6.  Set NAT Timeout                                          ",
"7.  Get NAT Timeout                                          ",
"8.  Add DMZ IP                                               ",
"9.  Get DMZ IP                                               ",
"10. Delete DMZ IP                                            ",
"11. Set IPSEC VPN Passthrough                                ",
"12. Get IPSEC VPN Passthrough                                ",
"13. Set PPTP VPN Passthrough                                 ",
"14. Get PPTP VPN Passthrough                                 ",
"15. Set L2TP/IPSEC VPN Passthrough                           ",
"16. Get L2TP/IPSEC VPN Passthrough                           ",
"17. Enable/Disable ALG                                       ",
"18. Set SIP server info                                      ",
"19. Get SIP server info                                      ",
"20. Set Initial Packet Threshold                             ",
"21. Get Initial Packet Threshold                             ",
"22. Enable/Disable SOCKSv5Proxy                              ",
"23. Set SOCKSv5 Proxy Config                                 ",
"24. Get SOCKSv5 Proxy Config                                 ",
"25. Enable/Disable IPv6 NAT                                  ",
"26. Get IPv6 NAT Status                                      ",
"27. Add IPv6 SNAT Entry                                      ",
"28. Delete IPv6 SNAT Entry                                   ",
"29. Get IPv6 SNAT Config                                     ",
"30. Add IPv6 DMZ IP                                          ",
"31. Get IPv6 DMZ IP                                          ",
"32. Delete IPv6 DMZ IP                                       ",
"33. Set IPv6 IPSEC VPN Passthrough                           ",
"34. Get IPv6 IPSEC VPN Passthrough                           ",
"35. Set IPv6 PPTP VPN Passthrough                            ",
"36. Get IPv6 PPTP VPN Passthrough                            ",
"37. Set IPv6 L2TP/IPSEC VPN Passthrough                      ",
"38. Get IPv6 L2TP/IPSEC VPN Passthrough                      ",
"39. Add/Delete Port Trigger Entry                            ",
"40. Display Port Trigger Entries                             ",
"41. Set IPV4 NAT Config                                      ",
"42. Get IPV4 NAT Config                                      "
};

const char* wlan_configuration_list[]
{
"1.  Enable/Disable WLAN                                      ",
"2.  Activate WLAN                                            ",
"3.  Set WLAN Config                                          ",
"4.  Get WLAN Config                                          ",
"5.  Get WLAN Status                                          ",
"6.  Set MobileAP/WLAN Bootup Config                          ",
"7.  Get MobileAP/WLAN Bootup Config                          ",
"8.  Get Station Mode Status                                  ",
"9.  Activate Hostapd Config                                  ",
"10. Activate Supplicant Config                               ",
"11. Get WLAN IF information                                  ",
"12. Set Always on WLAN                                       ",
"13. Get Always on WLAN                                       ",
"14. Set Peer to Peer Role                                    ",
"15. Get Peer to Peer Role                                    ",
"16. Set WLAN Ex Config                                       ",
"17. Get WLAN Ex Config                                       ",
"18. Enable/Disable CoEX                                      ",
"19. Set EZMesh Config                                        ",
"20. Get EZMesh Config                                        ",
"21. Activate Hostapd Restart for EZMesh Config               ",
"22. Set EZMesh Service Prioritization state                  ",
"23. Set WLAN Ex3 Config                                      ",
"24. Get WLAN Ex3 Config                                       "
};

const char* firewall_configuration_list[]
{
"1. Add Firewall Entry                                        ",
"2. Set Firewall Config                                       ",
"3. Get Firewall Config                                       ",
"4. Display Firewalls                                         ",
"5. Delete Firewall Entry                                     ",
"6. Set HW Filtering State                                    ",
"7. Get HW Filtering State                                    "
};

const char* backhaul_configuration_list[]
{
"1. Backhaul Common                                           ",
"2. Backhaul WWAN                                             "
};

const char* backhaul_common_configuration_list[]
{
"1.  Get Network Configuration         ",
"2.  Configure Active Backhaul Priority",
"3.  Get Backhaul Priority             ",
"4.  Get Data Bitrate                  ",
"5.  Enable/Disable IPV4               ",
"6.  Enable/Disable IPV6               ",
"7.  Get IPv4 State                    ",
"8.  Get IPv6 State                    ",
"9.  Get Backhaul Status               ",
"10. Get Bearer Tech                   ",
"11. Get All Connected PDNs            ",

};

const char* backhaul_wwan_configuration_list[]
{
"1.  Connect/Disconnect Backhaul       ",
"2.  Get WWAN status                    ",
"3.  Get WWAN Statistics               ",
"4.  Reset WWAN Statistics             ",
"5.  Set Webserver WWAN access flag    ",
"6.  Get Webserver WWAN access flag    ",
"7.  Set WWAN Profile                  ",
"8.  Get WWAN Profile                  ",
"9.  Set Prefix Delegation Config      ",
"10. Get Prefix Delegation Config      ",
"11. Get Prefix Delegation Status      ",
"12. Enable/Disable TinyProxy          ",
"13. Get TinyProxy Status              ",
"14. Set IP Passthrough Config         ",
"15. Get IP Passthrough Config         ",
"16. Get IP Passthrough State          ",
"17. Set Autoconnect Config            ",
"18. Get Autoconnect Config            ",
"19. Set Roaming                       ",
"20. Get Roaming                       ",
"21. Enable/Disable DDNS               ",
"22. Set DDNS Config                   ",
"23. Get DDNS Config                   ",
"24. Switch WWAN Profile               ",
"25. Create WWAN Profile               ",
"26. Update WWAN Profile               ",
"27. Delete WWAN Profile               ",
"28. Add/Delete PDN to VLAN Mapping    ",
"29. Get All PDN to VLAN Mappings      ",
"30. Set PMIP mode                     ",
"31. Get PMIP mode                     ",
"32. Get WWAN Roaming status           ",
"33. Get Current Profile Handle        ",
"34. Eth over GRE Tunnel Config        ",
"35. Set IP Passthrough S/W Path Filter",
"36. Get IP Passthrough S/W Path Filter",
"37. Set DHCPv6 DNS proxy config       ",
"38. Get DHCPv6 DNS proxy state        ",
"39. Set IPv6 External Router Mode     ",
"40. Get IPv6 External Router Mode     ",
"41. Not Supported                     ",
"42. Enable/Get QoS Flow Indications   ",
"43. Configure DDS Recommendation      ",
"44. Switch DDS                        ",
"45. Get Current DDS                   "
};

const char *backhaul_wwan_update_configuration_list[]
{
"1. Update Tech Type",
"2. Update 3GPP Profile",
"3. Update 3GPP2 Profile",
//"4. Update 3GPP (V6) Profile",
//"5. Update 3GPP2 (V6) Profile",
"4. Update All (3GPP/3GPP2) Profile",
"5. Set as default Profile",
"6. Update Subscription-Id",
"7. Update APN Name",
};

const char* tethering_configuration_list[]
{
"1. Set Cradle Mode                                           ",
"2. Get Cradle Mode                                           ",
"3. Set Ethernet NIC/MACsec Config                            ",
"4. Get Ethernet NIC/MACsec Config                            ",
"5. Get BT Tethering Status                                   ",
"6. Set Dun Dongle Mode                                       ",
"7. Get Dun Dongle Mode                                       "
};

const char* media_service_configuration_list[]
{
"1.  Enable/Disable UPnP                                      ",
"2.  Get UPnP Status                                          ",
"3.  Set UPnP Notify Interval                                 ",
"4.  Get UPnP Notify Interval                                 ",
"5.  Set UPNPPinhole State                                    ",
"6.  Get UPNPPinhole State                                    ",
"7.  Enable/Disable DLNA                                      ",
"8.  Set DLNA Notify Interval                                 ",
"9.  Get DLNA Notify Interval                                 ",
"10. Get DLNA Status                                          ",
"11. Set DLNA Media Directory                                 ",
"12. Get DLNA Media Directory                                 ",
"13. Set DLNAWhitelisting                                     ",
"14. Get DLNAWhitelisting                                     ",
"15. Add DLNAWhitelistingIP                                   ",
"16. Delete DLNAWhitelistingIP                                ",
"17. Enable/Disable M-DNS                                     ",
"18. Get MDNS Status                                          "
};

const char* software_bridge_config_list[]=
{
"1. Set Generic Software Bridge Config                         ",
"2. Enable/Disable(1/0) Generic Software Bridge                ",
"3. Get Generic Software Bridge Config                         ",
"4. Delete Generic Software Bridge Config                         ",
};

const char *v2x_config_list[]=
{
  "1.  V2X SPS Flow Register Request",
  "2.  V2X SPS Flow De-Register Request",
  "3.  V2X SPS Flow Update Request",
  "4.  V2X SPS Flow Get Info Request",
  "5.  V2X Non-SPS Flow Register Request",
  "6.  V2X Non-SPS Flow De-Register Request",
  "7.  V2X Service Subscribe Request",
  "8.  V2X Service Subscription Info Request",
  "9.  V2X Send Config File Request",
  "10. V2X Update Src L2 Info Request",
  "11. V2X Tunnel Mode Info Request",
  "12. V2X Get Capability Info Request"
};

const char *feature_modes_list[]=
{
  "1. IP Passthrough",
  "2. DHCP LAN Options",
  "3. EoGRE Mode",
  "4. MPLSoGRE Mode",
  "5. Eth Pdu Mode",
  "6. IPSEC"
};


const char *eogre_config_list[]=
{
  "1. Set EoGRE Interface               ",
  "2. Get EoGRE Interface               ",
  "3. Set EoGRE DSCP_TOS Marking            ",
  "4. Get EoGRE DSCP_TOS Marking            ",
  "5. Delete EoGRE DSCP_TOS Marking         "
};

const char* mtpe_configuration_list[]
{
"1. Configure Modem Throughput Test                           ",
"2. Start Modem Throughput Test                               ",
"3. Stop Modem Throughput Test                                ",
"4. Get Throughput Configuration Info                         ",
"5. Teardown Modem Throughput Test                            ",
"6. Get Modem Throughput Test History                         "
};

const char* get_connected_devices_info_config_list[]
{
"1. Get Connected Devices Info                        ",
"2. Get Connected Devices Info (fragmentation support)"
};
const char *backhaul_wwan_qos_flow_ind_list[]
{
  "1. Get Global QoS Flow Indication"
};

/* QCMAP Interface names */
const char *gIntfNames[] = {"wlan", "eth", "ecm", "rndis", "mhi", "eth-nic2"};

const char* IPSEC_configuration_list[]=
{
  "1. Set IPsec Tunnel",
  "2. Activate IPsec Tunnel",
  "3. Delete IPsec Tunnel",
  "4. Get IPsec Info",
  "5. Get IPsec Status Info"
};

const char* IPSEC_topology_list[]=
{
  "1. IPsec Host-to-Host",
  "2. IPsec Site-to-Site without NAT",
  "3. IPsec Host-to-Host and IPsec Site-to-Site without NAT"
};

/* MTPE History Buffers */
static mtpe_history_entry *mtpe_history_entries = NULL;
static uint32_t mtpe_txn_id = 0;
static uint32_t len_mtpe_history = 0;
static boolean mtpe_history_ready_flag = false;
static pthread_mutex_t mtpe_hist_mutex;
static pthread_cond_t mtpe_hist_result_cond;

void print_mtpe_entry(mtpe_history_entry *entry){
    printf("%s [RTT: %u ms, GPS: (%.5f, %.5f), Peak Throughput: %.5f kbps]\n",
            ctime(&entry->timestamp),
            entry->avg_ping_time,
            entry->latitude,
            entry->longitude,
            entry->peak_rate);
}

// Process and aggregate the data from incoming MTPE indication(s)
boolean ProcessMTPEHistoryIndication(
    qcmap_msgr_mtpe_history_ind_msg_v01 *ind_data,
    mtpe_history_entry *in_mtpe_history_entries,
    uint32_t *len_mtpe_history,
    uint32 txn_id,
    boolean *mtpe_history_ready
)
{
    qmi_error_type_v01 *qmi_err_num;
    LOG_MSG_INFO1("TXN_ID %d (%d)", ind_data->transaction_id, txn_id, 0);
    LOG_MSG_INFO1("TXN_valid %d, list_valid %d, pending %d",
        ind_data->transaction_id_valid,
        ind_data->mtpe_history_list_valid,
        ind_data->num_inds_pending_valid);
    // check if history items are related to the same request
    if(ind_data->transaction_id_valid &&
        (ind_data->transaction_id == txn_id) &&
       ind_data->mtpe_history_list_valid) {
         memcpy(((char*)(in_mtpe_history_entries) +
                (sizeof(qcmap_msgr_mtpe_history_entry_msg_v01) * (*len_mtpe_history))),
                ind_data->mtpe_history_list,
                (ind_data->mtpe_history_list_len * sizeof(qcmap_msgr_mtpe_history_entry_msg_v01)));
         *len_mtpe_history = *len_mtpe_history + ind_data->mtpe_history_list_len;
    }else{
        LOG_MSG_ERROR("MTPE history items invalid or TXN ID invalid",0,0,0)
        *mtpe_history_ready = true;
        return false;
    }
    if(ind_data->num_inds_pending_valid && (ind_data->num_inds_pending == 0)){
        LOG_MSG_INFO1("MTPE READY",0,0,0);
        *mtpe_history_ready = true;
    }else{
        *mtpe_history_ready = false;
    }
    return true;
}

/* Teardown/Disable and Exit should always be the last option.
  Always keep an even number of elements in array.  If array size is odd,
  add an emtpy placeholder to the end of the array.  When adding new options
  always replace empty options first.*/

/*===========================================================================
  FUNCTION  DisplayEthernetNicConfig
===========================================================================*/
void DisplayEthernetNicConfig(qcmap_eth_config eth_config)
{
    bool macsec_found = FALSE;
    int macsec_index = 0;
    printf("   ETH NIC/MACsec config: %d\n", eth_config.no_of_nics);
    fflush(stdout);
    for (int i = 0; i < eth_config.no_of_nics; i++)
    {
      if(eth_config.is_macsec_nic_config_valid)
      {
        macsec_found = FALSE;
        for(int j = 0; j < eth_config.no_of_macsec_nics; j++)
        {
          if( strncmp(eth_config.macsec_nic_config[j].eth_nic_iface_name, eth_config.eth_nic_config[i].eth_iface_name, QCMAP_MAX_IFACE_NAME_SIZE) == 0)
          {
            macsec_found = TRUE;
            macsec_index = j;
            break;
          }
        }
        if(macsec_found)
        {
          printf("   NICNo       NICType      NICIFace     MACsecState MACsecIface MACsecMode  \n");
          printf("   %-12d\t%-12s\t%-12s\t%-12s\t%-12s\t%-12s\n", i+1, ((eth_config.eth_nic_config[i].eth_nic_type)?("WAN"):("LAN")),\
                      eth_config.eth_nic_config[i].eth_iface_name,
                      ((eth_config.macsec_nic_config[macsec_index].state) ? ("Enable"):("Disable")),
                      eth_config.macsec_nic_config[macsec_index].macsec_iface_name,
                      ((eth_config.macsec_nic_config[macsec_index].macsec_mode == QCMAP_MSGR_MACSEC_MODE_SUPPLICANT_V01) ? \
                       ("SUPPLICANT"):("AUTHENTICATOR")));
        }
        else
        {
          printf("   NICNo       NICType      NICIFace     MACsecState MACsecIface MACsecMode  \n");
          printf("   %-12d\t%-12s\t%-12s\t%-12s\t%-12s\t%-12s\n", i+1, ((eth_config.eth_nic_config[i].eth_nic_type)?("WAN"):("LAN")),\
                      eth_config.eth_nic_config[i].eth_iface_name,
                      (("Disable")),
                      (""),
                      (""));
        }
      }
      else
      {
         printf("   NICNo       NICType      NICIFace     \n");
         printf("   %-12d\t%-12s\t%-12s\n", i+1, ((eth_config.eth_nic_config[i].eth_nic_type)?("WAN"):("LAN")),\
                  eth_config.eth_nic_config[i].eth_iface_name);
      }
    }
  fflush(stdout);
  return;
}

/*===========================================================================
  FUNCTION  DisplayClientInfo
  ===========================================================================*/
void DisplayClientInfo(qcmap_msgr_packet_stats_status_ind_msg_v01 ind_data)
{
  char tmpIPv4[INET_ADDRSTRLEN];
  in6_addr allipv6[QCMAP_MSGR_MAX_IPV6_ADDR_V01];
  in6_addr tmpipv6;
  uint8 zero_buff[QCMAP_MSGR_IPV6_ADDR_LEN_V01] = {0};
  char ip6_addr_buf[INET6_ADDRSTRLEN];
  uint32_t connDevCount = 0;
  char mac_addr_str[QCMAP_MSGR_MAC_ADDR_NUM_CHARS_V01]; /*char array of mac address*/
  memset(tmpIPv4,0,INET_ADDRSTRLEN);
  uint32_t entries = ind_data.number_of_entries;

  printf("\n CLI: ind_type %d, conn_client_num  %d , entry %d\n", ind_data.conn_status, entries);
  if (entries != 0)
  {
    // Displaying the information in appropriate fashion
    for (connDevCount = 0; connDevCount < entries; connDevCount++)
    {
      if (connDevCount == QCMAP_MSGR_MAX_CONNECTED_DEVICES_V01)
      {
        break;
      }
      ds_mac_addr_ntop(ind_data.info[connDevCount].client_mac_addr,
                       mac_addr_str);
      printf("MAC Address : %s \n",mac_addr_str);
      if(inet_ntop(AF_INET,
                   (void *)&ind_data.info[connDevCount].ipv4_addr,tmpIPv4,
                   INET_ADDRSTRLEN))
      {
        printf("IPv4 Address : %s \n",tmpIPv4);
      }
      memset(&tmpipv6, 0, sizeof(tmpipv6));
      memcpy(&tmpipv6.s6_addr,
             ind_data.info[connDevCount].ll_ipv6_addr,
             QCMAP_MSGR_IPV6_ADDR_LEN_V01);
      if(inet_ntop(AF_INET6,
                   (void*)&tmpipv6, ip6_addr_buf, sizeof(ip6_addr_buf)))
      {
        printf("Link Local IPv6 Address : %s\n",ip6_addr_buf);
      }

     for (int i = 0; i < QCMAP_MSGR_MAX_IPV6_ADDR_V01; i++)
     {
       memset(&allipv6[i], 0, sizeof(in6_addr));
       memcpy(&allipv6[i].s6_addr,
              ind_data.info[connDevCount].ipv6[i].addr,
               QCMAP_MSGR_IPV6_ADDR_LEN_V01);
       if (!memcmp(&allipv6[i].s6_addr, zero_buff,QCMAP_MSGR_IPV6_ADDR_LEN_V01))
         break;
       if(inet_ntop(AF_INET6,
                    (void*)&allipv6[i], ip6_addr_buf, sizeof(ip6_addr_buf)))
       {
         printf("IPv6 Address %d: %s\n",i, ip6_addr_buf);
       }
     }
     switch (ind_data.info[connDevCount].device_type)
     {
       case QCMAP_MSGR_DEVICE_TYPE_PRIMARY_AP_V01:
         printf("Device Type : Primary AP\n");
         break;
       case QCMAP_MSGR_DEVICE_TYPE_GUEST_AP_V01:
         printf("Device Type :Guest AP1\n");
         break;
       case QCMAP_MSGR_DEVICE_TYPE_GUEST_AP_2_V01:
         printf("Device Type :Guest AP2\n");
         break;
       case QCMAP_MSGR_DEVICE_TYPE_USB_V01:
         printf("Device Type :USB\n");
         break;
       case QCMAP_MSGR_DEVICE_TYPE_ETHERNET_V01:
         printf("Device Type :Ethernet\n");
         break;
       case QCMAP_MSGR_DEVICE_TYPE_ETHERNET_NIC2_V01:
         printf("Device Type :Ethernet-NIC2\n");
         break;
       default:
         printf("Device Type : Invalid\n");
         break;
     }
     printf("Host Name : %s\n",
             ind_data.info[connDevCount].host_name);
     printf("rx bytes : %lu\n",
             ind_data.info[connDevCount].bytes_rx);
     printf("tx bytes : %lu\n",
             ind_data.info[connDevCount].bytes_tx);
     printf("Lease Expiry Time (in minutes) : %d\n\n\n",
             ind_data.info[connDevCount].lease_expiry_time);
    }
  }
  else
  {
    printf("\n CLI:No Connected Device to this Access Point \n");
  }
}

/*===========================================================================
  FUNCTION  IsStrNum()
  ===========================================================================*/
/*!
  @brief
  check if the string is a number

  @return
    true - all of the values are numbers,
    false - if any of the characters are not number
  @note
  - Dependencies
    none

  - Side Effects
      returns true if there is a space between numbers, only the first number will be read as the user's input
 */
/*=========================================================================*/
boolean IsStrNum(char* str)
{
  if (str == NULL)
  {
    printf("str is null");
    return false;
  }
  /*check for number and white spaces*/
  boolean valid = false;  
  boolean all_white_spaces = true;                       
  for (int i = 0; i < strlen(str)-1; i++)                     
  {     
    if (str[i] != ' ' && str[i] != '\t')
    {
      all_white_spaces = false;
      if (IS_CHAR_NUM(str[i]))
      {
        valid = true;
      }
      else
      {
        valid = false;
        break;
      }
    }                                                                                                          
  } 
  /*no input if all white spaces*/
  if (all_white_spaces)
  {
    valid = false;
  }
  return valid;   
}

/*===========================================================================
  FUNCTION  convert_backhaul_enum_to_string
  ===========================================================================*/
/*!
  @brief
  Converts backhaul enum to string.

  @return
  void

  @note

  - Dependencies
  - None

  - Side Effects
  - None
 */
/*=========================================================================*/
boolean convert_backhaul_enum_to_string
(
  qcmap_msgr_backhaul_type_enum_v01 backhaul_type,
  char *backhaul_type_string
)
{
  if (backhaul_type_string == NULL)
  {
    LOG_MSG_ERROR("NULL Args", 0, 0, 0);
    return false;
  }
  switch (backhaul_type)
  {
    case QCMAP_MSGR_WWAN_BACKHAUL_V01:
      snprintf(backhaul_type_string,MAX_BACKHAUL_TYPE_LENGTH,"WWAN");
      break;
    case QCMAP_MSGR_USB_CRADLE_BACKHAUL_V01:
      snprintf(backhaul_type_string,MAX_BACKHAUL_TYPE_LENGTH,"USB CRADLE");
      break;
    case QCMAP_MSGR_WLAN_BACKHAUL_V01:
      snprintf(backhaul_type_string,MAX_BACKHAUL_TYPE_LENGTH,"WLAN");
      break;
    case QCMAP_MSGR_ETHERNET_BACKHAUL_V01:
      snprintf(backhaul_type_string,MAX_BACKHAUL_TYPE_LENGTH,"ETHERNET");
      break;
    case QCMAP_MSGR_BT_BACKHAUL_V01:
      snprintf(backhaul_type_string,MAX_BACKHAUL_TYPE_LENGTH,"BT");
      break;
    default:
      LOG_MSG_ERROR("Invalid Backhaul type", 0, 0, 0);
      return false;
      break;
  }
  return true;
}

/* Print Backhaul WWAN Details */
#define PRINT_BACKHAUL_WWAN_DETAILS(wwan_ind_msg)                                                         \
          {                                                                                               \
            char addr_str[INET6_ADDRSTRLEN];                                                              \
            printf("\n   Interface Name                 : %s\n", wwan_ind_msg.wwan_info.iface_name);      \
            CONVERT_IPV4_NETWORK_ADDRESS_TO_STRING(wwan_ind_msg.wwan_info.v4_addr, addr_str);             \
            printf("   Public IPv4 Address            : %s\n", addr_str);                                 \
            CONVERT_IPV4_NETWORK_ADDRESS_TO_STRING(wwan_ind_msg.wwan_info.v4_prim_dns_addr, addr_str);    \
            printf("   Primary DNS IPv4 Address       : %s\n", addr_str);                                 \
            CONVERT_IPV4_NETWORK_ADDRESS_TO_STRING(wwan_ind_msg.wwan_info.v4_sec_dns_addr, addr_str);     \
            printf("   Secondary DNS IPv4 Address    : %s\n", addr_str);                                  \
            CONVERT_IPV6_NETWORK_ADDRESS_TO_STRING(wwan_ind_msg.wwan_info.v6_addr, addr_str);             \
            printf("   Public IPv6 Address            : %s\n", addr_str);                                 \
            CONVERT_IPV6_NETWORK_ADDRESS_TO_STRING(wwan_ind_msg.wwan_info.v6_prim_dns_addr, addr_str);    \
            printf("   Primary DNS IPv6 Address       : %s\n", addr_str);                                 \
            CONVERT_IPV6_NETWORK_ADDRESS_TO_STRING(wwan_ind_msg.wwan_info.v6_sec_dns_addr, addr_str);     \
            printf("   Secondary DNS IPv6 Address    : %s\n\n", addr_str);                                \
          }

/*===========================================================================
  FUNCTION QCMAP_PRINTF_TAKE_INPUT()
  ===========================================================================
  @brief
  printf for OpenWRT value from user.
  @input
  void
  @return
  int value read from user.
  @dependencies
  usr to provide input
  @sideefects
  None
  =========================================================================*/
static inline void QCMAP_PRINTF_TAKE_INPUT(const char* format, ...)
{
  va_list args;
  va_start(args, format);
  int r;
  r = vprintf(format,args);
#ifdef PLATFORM_OPENWRT
  fflush(stdout);
#endif
  va_end(args);
  return;
}


/*===========================================================================
  FUNCTION  qcmap_msgr_qmi_qcmap_ind
  ===========================================================================*/
/*!
  @brief
  Processes an incoming QMI QCMAP Indication.

  @return
  void

  @note

  - Dependencies
  - None

  - Side Effects
  - None
 */
/*=========================================================================*/
void qcmap_msgr_qmi_qcmap_ind
(
 qmi_client_type user_handle,                    /* QMI user handle       */
 unsigned int    msg_id,                         /* Indicator message ID  */
 void           *ind_buf,                        /* Raw indication data   */
 unsigned int    ind_buf_len,                    /* Raw data length       */
 void           *ind_cb_data                     /* User call back handle */
)
{
  qmi_client_error_type qmi_error;
  profile_handle_type_v01 profile_handle;
  qcmap_msgr_subscription_enum_v01 subs_id = QCMAP_MSGR_SUBSCRIPTION_ENUM_MAX_ENUM_VAL_V01;
  char command[MAX_COMMAND_STR_LEN] = {0}, buffer[MAX_BACKHAUL_TYPE_LENGTH] = {0};
  QCMAP_CLI_LOG("qcmap_msgr_qmi_qcmap_ind: user_handle %X msg_id %d ind_buf_len %d.\n",
          user_handle, msg_id, ind_buf_len);

  switch (msg_id)
  {
    case QMI_QCMAP_MSGR_PACKET_STATS_STATUS_IND_V01:
    {
      qcmap_msgr_packet_stats_status_ind_msg_v01 ind_data;

      qmi_error = qmi_client_message_decode(user_handle,
                                            QMI_IDL_INDICATION,
                                            msg_id,
                                            ind_buf,
                                            ind_buf_len,
                                            &ind_data,
                                            sizeof(qcmap_msgr_packet_stats_status_ind_msg_v01));
      if (qmi_error != QMI_NO_ERR)
      {
        QCMAP_CLI_LOG("qcmap_msgr_qmi_qcmap_ind: qmi_client_message_decode error %d\n",qmi_error);
        break;
     }
      /* Process packet service status indication for packet stats for QCMAP*/
     switch (ind_data.conn_status)
     {
     case QCMAP_MSGR_PACKET_STATS_CLIENT_CONNECTED_V01:
       QCMAP_CLI_LOG("qcmap_msgr_qmi_qcmap_ind: A new client is Connected\n\n");
         break;
     case QCMAP_MSGR_PACKET_STATS_CLIENT_DISCONNECTED_V01:
       QCMAP_CLI_LOG("qcmap_msgr_qmi_qcmap_ind: Client is disconnected\n\n");
         break;
     case QCMAP_MSGR_PACKET_STATS_IPV4_UPDATED_V01:
       QCMAP_CLI_LOG("qcmap_msgr_qmi_qcmap_ind: IPV4 Updated\n\n");
         break;
     case QCMAP_MSGR_PACKET_STATS_IPV6_UPDATED_V01:
       QCMAP_CLI_LOG("qcmap_msgr_qmi_qcmap_ind: IPV6 Updated\n\n");
         break;
     case QCMAP_MSGR_PACKET_STATS_WLAN_DISABLED_V01:
       QCMAP_CLI_LOG("qcmap_msgr_qmi_qcmap_ind: WLAN Disabled\n\n");
         break;
     case QCMAP_MSGR_PACKET_STATS_MOBILEAP_DISABLED_V01:
       QCMAP_CLI_LOG("qcmap_msgr_qmi_qcmap_ind: MOBILEAP Teardown\n\n");
         break;
     case QCMAP_MSGR_PACKET_STATS_USB_DISCONNECTED_V01:
       QCMAP_CLI_LOG("qcmap_msgr_qmi_qcmap_ind: USB Disconnected\n\n");
         break;
     case QCMAP_MSGR_PACKET_STATS_IPV4_WWAN_DISCONNECTED_V01:
       QCMAP_CLI_LOG("qcmap_msgr_qmi_qcmap_ind: IPV4 BH Disconnected\n\n");
         break;
     case QCMAP_MSGR_PACKET_STATS_IPV6_WWAN_DISCONNECTED_V01:
       QCMAP_CLI_LOG("qcmap_msgr_qmi_qcmap_ind: IPV6 BH Disconnected\n\n");
         break;
     case QCMAP_MSGR_PACKET_STATS_BH_SWITCHED_V01:
       QCMAP_CLI_LOG("qcmap_msgr_qmi_qcmap_ind: Backhaul is Switched \n\n");
         break;
      }
      DisplayClientInfo(ind_data);
      break;
    }

    case QMI_QCMAP_MSGR_BRING_UP_WWAN_IND_V01:
    {
      qcmap_msgr_bring_up_wwan_ind_msg_v01 ind_data;

      qmi_error = qmi_client_message_decode(user_handle,
                                            QMI_IDL_INDICATION,
                                            msg_id,
                                            ind_buf,
                                            ind_buf_len,
                                            &ind_data,
                                            sizeof(qcmap_msgr_bring_up_wwan_ind_msg_v01));
      if (qmi_error != QMI_NO_ERR)
      {
        QCMAP_CLI_LOG("qcmap_msgr_qmi_qcmap_ind: qmi_client_message_decode error %d",qmi_error);
        break;
      }

      profile_handle = (ind_data.profile_handle_valid == TRUE) ? ind_data.profile_handle: 0xFFFF;
      subs_id        = (ind_data.subs_id_valid == TRUE) ? ind_data.subs_id: 0xFFFF;
      /* Process packet service status indication for WWAN for QCMAP*/
      if (ind_data.conn_status == QCMAP_MSGR_WWAN_STATUS_CONNECTED_V01)
      {
        if (ind_data.mobile_ap_handle == QcMapClient->mobile_ap_handle)
        {
          QCMAP_CLI_LOG("qcmap_msgr_qmi_qcmap_ind: PDN=%d (Subs_Id=%d) IPV4 WWAN Connected",
                          profile_handle, subs_id);
          PRINT_BACKHAUL_WWAN_DETAILS(ind_data);

          return;
        }
      }
      else if (ind_data.conn_status == QCMAP_MSGR_WWAN_STATUS_CONNECTING_FAIL_V01)
      {
        if (ind_data.mobile_ap_handle == QcMapClient->mobile_ap_handle)
        {
          QCMAP_CLI_LOG("qcmap_msgr_qmi_qcmap_ind: PDN=%d IPV4 WWAN Connecting Failed...WAN CallendType=%d CallendCode=%d",
                            profile_handle,
                            ind_data.wwan_call_end_reason.wwan_call_end_reason_type,
                            ind_data.wwan_call_end_reason.wwan_call_end_reason_code);
          return;
        }
      }
      else if (ind_data.conn_status == QCMAP_MSGR_WWAN_STATUS_IPV6_CONNECTED_V01)
      {
        if (ind_data.mobile_ap_handle == QcMapClient->mobile_ap_handle)
        {
          QCMAP_CLI_LOG("qcmap_msgr_qmi_qcmap_ind: PDN=%d (Subs_Id=%d)  IPV6 WWAN Connected",
                          profile_handle, subs_id);

          PRINT_BACKHAUL_WWAN_DETAILS(ind_data);
          return;
        }
      }
      else if (ind_data.conn_status == QCMAP_MSGR_WWAN_STATUS_IPV6_CONNECTING_FAIL_V01)
      {
        if (ind_data.mobile_ap_handle == QcMapClient->mobile_ap_handle)
        {
          QCMAP_CLI_LOG("qcmap_msgr_qmi_qcmap_ind: PDN=%d (Subs_Id=%d) IPV6 WWAN Connecting Failed...WAN CallendType=%d CallendCode=%d",
                                profile_handle,
                                subs_id,
                                ind_data.wwan_call_end_reason.wwan_call_end_reason_type,
                                ind_data.wwan_call_end_reason.wwan_call_end_reason_code);

          return;
        }
      }
      else if (ind_data.conn_status == QCMAP_MSGR_WWAN_STATUS_ETH_CONNECTED_V01)
      {
        if (ind_data.mobile_ap_handle == QcMapClient->mobile_ap_handle)
        {
          QCMAP_CLI_LOG("qcmap_msgr_qmi_qcmap_ind: PDN=%d (Subs_Id=%d)  ETH WWAN Connected",
                          profile_handle, subs_id);
          printf("\n   Eth Wwan Interface Name                 : %s\n", ind_data.wwan_info.iface_name);
          printf("\n   Eth Wwan Vlan Mapping Id Start          : %d\n", ind_data.wwan_info.vlan_start);
          printf("\n   Eth Wwan Vlan Mapping Id End            : %d\n", ind_data.wwan_info.vlan_end);
          return;
        }
      }
      else if (ind_data.conn_status == QCMAP_MSGR_WWAN_STATUS_ETH_CONNECTING_FAIL_V01)
      {
        if (ind_data.mobile_ap_handle == QcMapClient->mobile_ap_handle)
        {
          QCMAP_CLI_LOG("qcmap_msgr_qmi_qcmap_ind: PDN=%d (Subs_Id=%d) ETH WWAN Connecting Failed...WAN CallendType=%d CallendCode=%d",
                                profile_handle,
                                subs_id,
                                ind_data.wwan_call_end_reason.wwan_call_end_reason_type,
                                ind_data.wwan_call_end_reason.wwan_call_end_reason_code);

          return;
        }
      }
      break;
    }
    case QMI_QCMAP_MSGR_TEAR_DOWN_WWAN_IND_V01:
    {
      qcmap_msgr_tear_down_wwan_ind_msg_v01 ind_data;

      qmi_error = qmi_client_message_decode(user_handle,
                                            QMI_IDL_INDICATION,
                                            msg_id,
                                            ind_buf,
                                            ind_buf_len,
                                            &ind_data,
                                            sizeof(qcmap_msgr_tear_down_wwan_ind_msg_v01));
      if (qmi_error != QMI_NO_ERR)
      {
        QCMAP_CLI_LOG("qcmap_msgr_qmi_qcmap_ind: qmi_client_message_decode error %d",qmi_error);
        break;
      }

      profile_handle = (ind_data.profile_handle_valid == TRUE) ? ind_data.profile_handle: 0xFFFF;
      subs_id        = (ind_data.subs_id_valid == TRUE) ? ind_data.subs_id: 0xFFFF;
      if (ind_data.conn_status == QCMAP_MSGR_WWAN_STATUS_DISCONNECTED_V01)
      {
        if (ind_data.mobile_ap_handle == QcMapClient->mobile_ap_handle)
        {
          QCMAP_CLI_LOG("qcmap_msgr_qmi_qcmap_ind: PDN=%d (Subs_Id=%d) IPV4 WWAN Disconnected...WAN CallendType=%d CallendCode=%d",
                            profile_handle,
                            subs_id,
                            ind_data.wwan_call_end_reason.wwan_call_end_reason_type,
                            ind_data.wwan_call_end_reason.wwan_call_end_reason_code);

          return;
        }
      }
      else if (ind_data.conn_status == QCMAP_MSGR_WWAN_STATUS_DISCONNECTING_FAIL_V01)
      {
        if (ind_data.mobile_ap_handle == QcMapClient->mobile_ap_handle)
        {
          QCMAP_CLI_LOG("qcmap_msgr_qmi_qcmap_ind: PDN=%d (Subs_Id=%d) IPV4 WWAN Disconnecting Failed...WAN CallendType=%d CallendCode=%d",
                            profile_handle,
                            subs_id,
                            ind_data.wwan_call_end_reason.wwan_call_end_reason_type,
                            ind_data.wwan_call_end_reason.wwan_call_end_reason_code);
          return;
        }
      }
      else if (ind_data.conn_status == QCMAP_MSGR_WWAN_STATUS_IPV6_DISCONNECTED_V01)
      {
        if (ind_data.mobile_ap_handle == QcMapClient->mobile_ap_handle)
        {
          QCMAP_CLI_LOG("qcmap_msgr_qmi_qcmap_ind: PDN=%d (Subs_Id=%d) IPV6 WWAN Disconnected...WAN CallendType=%d CallendCode=%d",
                            profile_handle,
                            subs_id,
                            ind_data.wwan_call_end_reason.wwan_call_end_reason_type,
                            ind_data.wwan_call_end_reason.wwan_call_end_reason_code);

          return;
        }
      }
      else if (ind_data.conn_status == QCMAP_MSGR_WWAN_STATUS_IPV6_DISCONNECTING_FAIL_V01)
      {
        if (ind_data.mobile_ap_handle == QcMapClient->mobile_ap_handle)
        {
          QCMAP_CLI_LOG("qcmap_msgr_qmi_qcmap_ind: PDN=%d (Subs_Id=%d) IPV6 WWAN Disconnecting Failed...WAN CallendType=%d CallendCode=%d",
                            profile_handle,
                            subs_id,
                            ind_data.wwan_call_end_reason.wwan_call_end_reason_type,
                            ind_data.wwan_call_end_reason.wwan_call_end_reason_code);
          return;
        }
      }
      else if (ind_data.conn_status == QCMAP_MSGR_WWAN_STATUS_ETH_DISCONNECTED_V01)
      {
        if (ind_data.mobile_ap_handle == QcMapClient->mobile_ap_handle)
        {
          QCMAP_CLI_LOG("qcmap_msgr_qmi_qcmap_ind: PDN=%d (Subs_Id=%d) ETH WWAN Disconnected...WAN CallendType=%d CallendCode=%d",
                            profile_handle,
                            subs_id,
                            ind_data.wwan_call_end_reason.wwan_call_end_reason_type,
                            ind_data.wwan_call_end_reason.wwan_call_end_reason_code);

          return;
        }
      }
      else if (ind_data.conn_status == QCMAP_MSGR_WWAN_STATUS_ETH_DISCONNECTING_FAIL_V01)
      {
        if (ind_data.mobile_ap_handle == QcMapClient->mobile_ap_handle)
        {
          QCMAP_CLI_LOG("qcmap_msgr_qmi_qcmap_ind: PDN=%d (Subs_Id=%d) ETH WWAN Disconnecting Failed...WAN CallendType=%d CallendCode=%d",
                            profile_handle,
                            subs_id,
                            ind_data.wwan_call_end_reason.wwan_call_end_reason_type,
                            ind_data.wwan_call_end_reason.wwan_call_end_reason_code);
          return;
        }
      }
      break;
    }
    case QMI_QCMAP_MSGR_WWAN_STATUS_IND_V01:
    {
      qcmap_msgr_wwan_status_ind_msg_v01 ind_data;

      qmi_error = qmi_client_message_decode(user_handle,
                                            QMI_IDL_INDICATION,
                                            msg_id,
                                            ind_buf,
                                            ind_buf_len,
                                            &ind_data,
                                            sizeof(qcmap_msgr_wwan_status_ind_msg_v01));
      if (qmi_error != QMI_NO_ERR)
      {
        QCMAP_CLI_LOG("qcmap_msgr_qmi_qcmap_ind: qmi_client_message_decode error %d",qmi_error);
        break;
      }

      profile_handle = (ind_data.profile_handle_valid == TRUE) ? ind_data.profile_handle: 0xFFFF;
      subs_id        = (ind_data.subs_id_valid == TRUE) ? ind_data.subs_id: 0xFFFF;
      if (ind_data.wwan_status == QCMAP_MSGR_WWAN_STATUS_DISCONNECTED_V01)
      {
        QCMAP_CLI_LOG("qcmap_msgr_qmi_qcmap_ind: PDN=%d (Subs_Id=%d) IPV4 WWAN Disconnected...WAN CallendType=%d, CallendCode=%d",
                            profile_handle,
                            subs_id,
                            ind_data.wwan_call_end_reason.wwan_call_end_reason_type,
                            ind_data.wwan_call_end_reason.wwan_call_end_reason_code);
        return;
      }
      else if (ind_data.wwan_status == QCMAP_MSGR_WWAN_STATUS_DISCONNECTING_FAIL_V01)
      {
        QCMAP_CLI_LOG("qcmap_msgr_qmi_qcmap_ind: PDN=%d (Subs_Id=%d) IPV4 WWAN Disconnecting Failed...WAN CallendType=%d CallendCode=%d",
                            profile_handle,
                            subs_id,
                            ind_data.wwan_call_end_reason.wwan_call_end_reason_type,
                            ind_data.wwan_call_end_reason.wwan_call_end_reason_code);
        return;
      }
      else if (ind_data.wwan_status == QCMAP_MSGR_WWAN_STATUS_CONNECTED_V01)
      {
        QCMAP_CLI_LOG("qcmap_msgr_qmi_qcmap_ind: PDN=%d (Subs_Id=%d) IPv4 WWAN Connected...",
                        profile_handle, subs_id);
        return;
      }
      else if (ind_data.wwan_status == QCMAP_MSGR_WWAN_STATUS_CONNECTING_FAIL_V01)
      {
        QCMAP_CLI_LOG("qcmap_msgr_qmi_qcmap_ind: PDN=%d (Subs_Id=%d) IPV4 WWAN Connecting Failed...WAN CallendType=%d CallendCode=%d",
                            profile_handle,
                            subs_id,
                            ind_data.wwan_call_end_reason.wwan_call_end_reason_type,
                            ind_data.wwan_call_end_reason.wwan_call_end_reason_code);
        return;
      }
      else if (ind_data.wwan_status == QCMAP_MSGR_WWAN_STATUS_IPV6_DISCONNECTED_V01)
      {
        QCMAP_CLI_LOG("qcmap_msgr_qmi_qcmap_ind: PDN=%d (Subs_Id=%d) IPV6 WWAN Disconnected...WAN CallendType=%d CallendCode=%d",
                            profile_handle,
                            subs_id,
                            ind_data.wwan_call_end_reason.wwan_call_end_reason_type,
                            ind_data.wwan_call_end_reason.wwan_call_end_reason_code);
        return;
      }
      else if (ind_data.wwan_status == QCMAP_MSGR_WWAN_STATUS_IPV6_DISCONNECTING_FAIL_V01)
      {
        QCMAP_CLI_LOG("qcmap_msgr_qmi_qcmap_ind: PDN=%d (Subs_Id=%d) IPV6 WWAN Disconnecting Failed...WAN CallendType=%d CallendCode=%d",
                            profile_handle,
                            subs_id,
                            ind_data.wwan_call_end_reason.wwan_call_end_reason_type,
                            ind_data.wwan_call_end_reason.wwan_call_end_reason_code);
        return;
      }
      else if (ind_data.wwan_status == QCMAP_MSGR_WWAN_STATUS_IPV6_CONNECTED_V01)
      {
        QCMAP_CLI_LOG("qcmap_msgr_qmi_qcmap_ind: PDN=%d (Subs_Id=%d) IPV6 WWAN Connected...",
                        profile_handle, subs_id);
        return;
      }
      else if (ind_data.wwan_status == QCMAP_MSGR_WWAN_STATUS_IPV6_CONNECTING_FAIL_V01)
      {
        QCMAP_CLI_LOG("qcmap_msgr_qmi_qcmap_ind: PDN=%d (Subs_Id=%d) IPV6 WWAN Connecting Failed...WAN CallendType=%d CallendCode=%d",
                            profile_handle,
                            subs_id,
                            ind_data.wwan_call_end_reason.wwan_call_end_reason_type,
                            ind_data.wwan_call_end_reason.wwan_call_end_reason_code);
        return;
      }
      else if (ind_data.wwan_status == QCMAP_MSGR_WWAN_STATUS_ETH_DISCONNECTED_V01)
      {
        QCMAP_CLI_LOG("qcmap_msgr_qmi_qcmap_ind: PDN=%d (Subs_Id=%d) ETH WWAN Disconnected...WAN CallendType=%d CallendCode=%d",
                            profile_handle,
                            subs_id,
                            ind_data.wwan_call_end_reason.wwan_call_end_reason_type,
                            ind_data.wwan_call_end_reason.wwan_call_end_reason_code);
        return;
      }
      else if (ind_data.wwan_status == QCMAP_MSGR_WWAN_STATUS_ETH_DISCONNECTING_FAIL_V01)
      {
        QCMAP_CLI_LOG("qcmap_msgr_qmi_qcmap_ind: PDN=%d (Subs_Id=%d) ETH WWAN Disconnecting Failed...WAN CallendType=%d CallendCode=%d",
                            profile_handle,
                            subs_id,
                            ind_data.wwan_call_end_reason.wwan_call_end_reason_type,
                            ind_data.wwan_call_end_reason.wwan_call_end_reason_code);
        return;
      }
      else if (ind_data.wwan_status == QCMAP_MSGR_WWAN_STATUS_ETH_CONNECTED_V01)
      {
        QCMAP_CLI_LOG("qcmap_msgr_qmi_qcmap_ind: PDN=%d (Subs_Id=%d) ETH WWAN Connected...",
                        profile_handle, subs_id);
        return;
      }
      else if (ind_data.wwan_status == QCMAP_MSGR_WWAN_STATUS_ETH_CONNECTING_FAIL_V01)
      {
        QCMAP_CLI_LOG("qcmap_msgr_qmi_qcmap_ind: PDN=%d (Subs_Id=%d) ETH WWAN Connecting Failed...WAN CallendType=%d CallendCode=%d",
                            profile_handle,
                            subs_id,
                            ind_data.wwan_call_end_reason.wwan_call_end_reason_type,
                            ind_data.wwan_call_end_reason.wwan_call_end_reason_code);
        return;
      }

      break;
    }
  case QMI_QCMAP_MSGR_MOBILE_AP_STATUS_IND_V01:
    {
      qcmap_msgr_mobile_ap_status_ind_msg_v01 ind_data;

      qmi_error = qmi_client_message_decode(user_handle,
                                            QMI_IDL_INDICATION,
                                            msg_id,
                                            ind_buf,
                                            ind_buf_len,
                                            &ind_data,
                                            sizeof(qcmap_msgr_mobile_ap_status_ind_msg_v01));
      if (qmi_error != QMI_NO_ERR)
      {
        QCMAP_CLI_LOG("qcmap_msgr_qmi_qcmap_ind: qmi_client_message_decode error %d",qmi_error);
        break;
      }

      if (ind_data.mobile_ap_status == QCMAP_MSGR_MOBILE_AP_STATUS_CONNECTED_V01)
      {
        QCMAP_CLI_LOG("qcmap_msgr_qmi_qcmap_ind: Mobile AP Connected...");
        return;
      }
      else if (ind_data.mobile_ap_status == QCMAP_MSGR_MOBILE_AP_STATUS_DISCONNECTED_V01)
      {
        QCMAP_CLI_LOG("qcmap_msgr_qmi_qcmap_ind: Mobile AP Disconnected...");
        return;
      }
      break;
    }

  case QMI_QCMAP_MSGR_STATION_MODE_STATUS_IND_V01:
    {
      qcmap_msgr_station_mode_status_ind_msg_v01 ind_data;

      qmi_error = qmi_client_message_decode(user_handle,
                                            QMI_IDL_INDICATION,
                                            msg_id,
                                            ind_buf,
                                            ind_buf_len,
                                            &ind_data,
                                            sizeof(qcmap_msgr_station_mode_status_ind_msg_v01));
      if (qmi_error != QMI_NO_ERR)
      {
        QCMAP_CLI_LOG("qcmap_msgr_qmi_qcmap_ind: qmi_client_message_decode error %d",qmi_error);
        break;
      }

      if (ind_data.station_mode_status == QCMAP_MSGR_STATION_MODE_CONNECTED_V01)
      {
        QCMAP_CLI_LOG("qcmap_msgr_qmi_qcmap_ind: Mobile AP Station mode Connected...");
        return;
      }
      else if (ind_data.station_mode_status == QCMAP_MSGR_STATION_MODE_DISCONNECTED_V01)
      {
        QCMAP_CLI_LOG("qcmap_msgr_qmi_qcmap_ind: Mobile AP Station mode Disconnected...");
        return;
      }
      else if (ind_data.station_mode_status == QCMAP_MSGR_STATION_MODE_ASSOCIATION_FAIL_V01)
      {
        QCMAP_CLI_LOG("qcmap_msgr_qmi_qcmap_ind: Mobile AP Station mode Association Failed. Going back to AP+STA Router Mode");
        return;
      }
      else if (ind_data.station_mode_status == QCMAP_MSGR_STATION_MODE_DHCP_IP_ASSIGNMENT_FAIL_V01)
      {
        QCMAP_CLI_LOG("qcmap_msgr_qmi_qcmap_ind: Mobile AP Station mode IP Assignment via DHCP Failed. Will switch to Static IP if available");
        return;
      }
      break;
    }

  case QMI_QCMAP_MSGR_CONNECTED_DEVICES_INFO_IND_V01:
    {
      qmi_error_type_v01 qmi_err_num;
      /*Initialize QMI Error Number*/
      qmi_err_num = QMI_ERR_NONE_V01;
      qcmap_msgr_connected_devices_info_ind_msg_v01 ind_data;
      char tmpIPv4[INET_ADDRSTRLEN] = {0};
      in6_addr allipv6[QCMAP_MSGR_MAX_IPV6_ADDR_V01];
      in6_addr tmpipv6;
      boolean flag = false;
      uint8 zero_buff[QCMAP_MSGR_IPV6_ADDR_LEN_V01] = {0};
      char ip6_addr_buf[INET6_ADDRSTRLEN] = {0};
      int connDevCount=0, num_entries=0;
      char mac_addr_str[QCMAP_MSGR_MAC_ADDR_NUM_CHARS_V01] = {0}; /*char array of mac address*/

      ZERO_INIT_ARG(ind_data);
      ZERO_INIT_ARG(tmpIPv4);
      ZERO_INIT_ARG(tmpipv6);
      ZERO_INIT_ARG(ip6_addr_buf);
      ZERO_INIT_ARG(mac_addr_str);

      qmi_error = qmi_client_message_decode(user_handle,
                          QMI_IDL_INDICATION,
                          msg_id,
                          ind_buf,
                          ind_buf_len,
                          &ind_data,
                          sizeof(qcmap_msgr_connected_devices_info_ind_msg_v01));

      if (qmi_error != QMI_NO_ERR)
      {
        QCMAP_CLI_LOG("qcmap_msgr_qmi_qcmap_ind: qmi_client_message_decode error %d",qmi_error);
        break;
      }

      if(ind_data.transaction_id_valid)
      {
        QCMAP_CLI_LOG("Indication received with transaction ID : %d", ind_data.transaction_id);
      }
      else
      {
        QCMAP_CLI_LOG("Indication received with invalid transaction ID");
        break;
      }

      if(ind_data.connected_devices_info_valid)
      {
        num_entries = ind_data.connected_devices_info_len;
        QCMAP_CLI_LOG("Num of Connected Devices Entries in this indication: %d vlan_id:%d", num_entries,
                      ind_data.connected_devices_info[0].vlan_id);

        printf("\n Printing Connected Device Info for this indication : \n");
        for(connDevCount=0; connDevCount<num_entries; connDevCount++)
        {
          printf("Device No : %d \n", connDevCount+1);
          ds_mac_addr_ntop(ind_data.connected_devices_info[connDevCount].client_mac_addr,
                           mac_addr_str);
          printf("MAC Address : %s \n", mac_addr_str);
          if(inet_ntop(AF_INET,
                       (void *)&ind_data.connected_devices_info[connDevCount].ipv4_addr,tmpIPv4,
                       INET_ADDRSTRLEN))
          {
            printf("IPv4 Address : %s \n", tmpIPv4);
          }
          memset(&tmpipv6, 0, sizeof(tmpipv6));
          memcpy(&tmpipv6.s6_addr,
                 ind_data.connected_devices_info[connDevCount].ll_ipv6_addr,
                 QCMAP_MSGR_IPV6_ADDR_LEN_V01);
          if(inet_ntop(AF_INET6,
                       (void*)&tmpipv6, ip6_addr_buf, sizeof(ip6_addr_buf)))
          {
            printf("Link Local IPv6 Address : %s\n", ip6_addr_buf);
          }

          if(is_ipv6nat_enabled && flag)
          {
            memset(&tmpipv6, 0, sizeof(tmpipv6));

            memcpy(&tmpipv6.s6_addr,
                   ind_data.connected_devices_info[connDevCount].ula_ipv6_addr,
                   QCMAP_MSGR_IPV6_ADDR_LEN_V01);

            if(inet_ntop(AF_INET6,
                        (void*)&tmpipv6, ip6_addr_buf, sizeof(ip6_addr_buf)) != NULL)
            {
              printf("ULA IPv6 Address : %s\n", ip6_addr_buf);
            }
          }

          for (int i = 0; i < QCMAP_MSGR_MAX_IPV6_ADDR_V01; i++)
          {
            memset(&allipv6[i], 0, sizeof(in6_addr));
            memset(ip6_addr_buf, 0, INET6_ADDRSTRLEN);

            memcpy(&allipv6[i].s6_addr,
                   ind_data.connected_devices_info[connDevCount].ipv6[i].addr,
                   QCMAP_MSGR_IPV6_ADDR_LEN_V01);
            if(!memcmp(&allipv6[i].s6_addr, zero_buff,QCMAP_MSGR_IPV6_ADDR_LEN_V01))
              break;
            if(inet_ntop(AF_INET6,
                         (void*)&allipv6[i], ip6_addr_buf, sizeof(ip6_addr_buf)))
            {
              printf("IPv6 Address %d: %s\n", i, ip6_addr_buf);
            }
          }

          switch (ind_data.connected_devices_info[connDevCount].device_type)
          {
            case QCMAP_MSGR_DEVICE_TYPE_PRIMARY_AP_V01:
              printf("Device Type : Primary AP\n");
              break;
            case QCMAP_MSGR_DEVICE_TYPE_GUEST_AP_V01:
              printf("Device Type :Guest AP1\n");
              break;
            case QCMAP_MSGR_DEVICE_TYPE_GUEST_AP_2_V01:
              printf("Device Type :Guest AP2\n");
              break;
            case QCMAP_MSGR_DEVICE_TYPE_USB_V01:
              printf("Device Type :USB\n");
              break;
            case QCMAP_MSGR_DEVICE_TYPE_ETHERNET_V01:
              printf("Device Type :Ethernet\n");
              break;
            default:
              printf("Device Type : Invalid\n");
              break;
          }
          printf("Host Name : %s\n",
                 ind_data.connected_devices_info[connDevCount].host_name);
          printf("rx bytes : %lu\n",
                 ind_data.connected_devices_info[connDevCount].bytes_rx);
          printf("tx bytes : %lu\n",
                 ind_data.connected_devices_info[connDevCount].bytes_tx);
          printf("Lease Expiry Time (in minutes) : %d\n",
                 ind_data.connected_devices_info[connDevCount].lease_expiry_time);
          if(ind_data.connected_devices_info[connDevCount].vlan_id != 0)
          {
            printf("VLAN ID :%d\n",ind_data.connected_devices_info[connDevCount].vlan_id);
          }
        }

        if(ind_data.num_inds_pending_valid && ind_data.num_inds_pending > 0)
        {
          QCMAP_CLI_LOG("%d follow-on indications are pending for transaction id : %d", ind_data.num_inds_pending,
                        ind_data.transaction_id);
        }
      }
      break;
    }

  case QMI_QCMAP_MSGR_CRADLE_MODE_STATUS_IND_V01:
    {
      qcmap_msgr_cradle_mode_status_ind_msg_v01 ind_data;

      qmi_error = qmi_client_message_decode(user_handle,
                           QMI_IDL_INDICATION,
                           msg_id,
                           ind_buf,
                           ind_buf_len,
                           &ind_data,
                           sizeof(qcmap_msgr_cradle_mode_status_ind_msg_v01));
      if (qmi_error != QMI_NO_ERR)
      {
        QCMAP_CLI_LOG("qcmap_msgr_qmi_qcmap_ind: "
                      "qmi_client_message_decode error %d",qmi_error);
        break;
      }

      if (ind_data.cradle_status == QCMAP_MSGR_CRADLE_CONNECTED_V01)
      {
        QCMAP_CLI_LOG("qcmap_msgr_qmi_qcmap_ind: "
                      "Mobile AP Cradle mode Connected...");
        return;
      }
      else if (ind_data.cradle_status == QCMAP_MSGR_CRADLE_DISCONNECTED_V01)
      {
        QCMAP_CLI_LOG("qcmap_msgr_qmi_qcmap_ind: "
                      "Mobile AP Cradle mode Disconnected...");
        return;
      }
      break;
    }

  case QMI_QCMAP_MSGR_WLAN_STATUS_IND_V01:
  {
     qcmap_msgr_wlan_status_ind_msg_v01 ind_data;
     int i=0;
     in_addr ip4_addr;
     char ip6_addr[INET6_ADDRSTRLEN];

     qmi_error = qmi_client_message_decode(user_handle,
                                           QMI_IDL_INDICATION,
                                           msg_id,
                                           ind_buf,
                                           ind_buf_len,
                                           &ind_data,
                                           sizeof(qcmap_msgr_wlan_status_ind_msg_v01));
      if (qmi_error != QMI_NO_ERR)
      {
        printf("qcmap_msgr_qmi_qcmap_ind: qmi_client_message_decode error %d",qmi_error);
        break;
      }

      if (ind_data.scm_ind_valid)
      {
        switch (ind_data.scm_ind)
        {
        case QCMAP_MSGR_SCM_DYNAMIC_RECONFIG_IND_V01:
          printf("QCMAP_MSGR_SCM_DYNAMIC_RECONFIG_IND_V01 SCM indication received\n");
          break;
        case QCMAP_MSGR_SCM_STATION_STATE_IND_V01:
          printf("QCMAP_MSGR_SCM_STATION_STATE_IND_V01 SCM indication received\n");
          break;
        case QCMAP_MSGR_SCM_SYS_CONTROL_IND_V01:
          printf("QCMAP_MSGR_SCM_SYS_CONTROL_IND_V01 SCM indication received\n");
          break;
        default:
          break;
        }
      }
      else
      {
        if (ind_data.wlan_status == QCMAP_MSGR_WLAN_ENABLED_V01)
        {
         printf("\n qcmap_msgr_qmi_qcmap_ind: WLAN is ENABLED...\n");
        }
        else if (ind_data.wlan_status == QCMAP_MSGR_WLAN_DISABLED_V01)
        {
         printf("\n qcmap_msgr_qmi_qcmap_ind: WLAN is DISABLED...\n");
        }
        else if (ind_data.wlan_status == QCMAP_MSGR_WLAN_SWITCH_TO_2G_V01)
        {
          printf("\n qcmap_msgr_qmi_qcmap_ind: WLAN AP/STA trying to switch to 2G...\n");
          if ( ind_data.wlan_switch_status == QCMAP_WLAN_2G_SWITCH_SUCCESS_V01 ||
               ind_data.wlan_switch_status == QCMAP_WLAN_STA_SWITCH_SUCCESS_V01 )
          {
            if (ind_data.wlan_switch_status == QCMAP_WLAN_STA_SWITCH_SUCCESS_V01)
            {
              printf("\n qcmap_msgr_qmi_qcmap_ind: WLAN STA is switched to 2G...\n");
            }
            if (ind_data.wlan_switch_status == QCMAP_WLAN_2G_SWITCH_SUCCESS_V01)
            {
              printf("\n qcmap_msgr_qmi_qcmap_ind: WLAN AP is switched to 2G...\n");
            }
          }
          else
          {
            if( ind_data.wlan_switch_status == QCMAP_WLAN_2G_SWITCH_FAIL_V01)
            {
              printf("\n qcmap_msgr_qmi_qcmap_ind: WLAN failed to switch to 2G...\n");
            }
          }
        }
        else if (ind_data.wlan_status == QCMAP_MSGR_WLAN_SWITCH_TO_5G_V01)
        {
          printf("\n qcmap_msgr_qmi_qcmap_ind: WLAN AP/STA trying to switch to 5G...\n");
          if ( ind_data.wlan_switch_status == QCMAP_WLAN_5G_SWITCH_SUCCESS_V01 ||
               ind_data.wlan_switch_status == QCMAP_WLAN_STA_SWITCH_SUCCESS_V01 )
          {
            if (ind_data.wlan_switch_status == QCMAP_WLAN_5G_SWITCH_SUCCESS_V01)
            {
             printf("\n qcmap_msgr_qmi_qcmap_ind: WLAN AP is switched to 5G...\n");
            }
            if (ind_data.wlan_switch_status == QCMAP_WLAN_STA_SWITCH_SUCCESS_V01)
            {
             printf("\n qcmap_msgr_qmi_qcmap_ind: WLAN STA is switched to 5G...\n");
            }
          }
          else
          {
           if( ind_data.wlan_switch_status == QCMAP_WLAN_5G_SWITCH_FAIL_V01)
           {
            printf("\n qcmap_msgr_qmi_qcmap_ind: WLAN failed to switch to 5G...\n");
           }
          }
        }
        else
        {
         printf("\n Invalid wlan status %d \n",ind_data.wlan_status);
        }

        if (ind_data.wlan_mode == QCMAP_MSGR_WLAN_MODE_AP_V01)
        {
         printf("qcmap_msgr_qmi_qcmap_ind: WLAN is in AP Mode...\n");
        }
        else if (ind_data.wlan_mode == QCMAP_MSGR_WLAN_MODE_AP_AP_V01)
        {
         printf("qcmap_msgr_qmi_qcmap_ind: WLAN is AP+AP Mode...\n");
        }
        else if (ind_data.wlan_mode == QCMAP_MSGR_WLAN_MODE_AP_AP_AP_V01)
        {
         printf("qcmap_msgr_qmi_qcmap_ind: WLAN is AP+AP+AP Mode...\n");
        }
        else if (ind_data.wlan_mode == QCMAP_MSGR_WLAN_MODE_AP_STA_V01)
        {
         printf("qcmap_msgr_qmi_qcmap_ind: WLAN is AP+STA Mode...\n");
         printf("STA is in %s mode...\n", (ind_data.bridge_mode ? "Bridge" : "Router"));
        }
        else if (ind_data.wlan_mode == QCMAP_MSGR_WLAN_MODE_AP_AP_STA_V01)
        {
         printf("qcmap_msgr_qmi_qcmap_ind: WLAN is AP+AP+STA Mode...\n");
         printf("STA is in %s mode...\n", (ind_data.bridge_mode ? "Bridge" : "Router"));
        }
        else if (ind_data.wlan_mode == QCMAP_MSGR_WLAN_MODE_STA_ONLY_V01)
        {
         printf("qcmap_msgr_qmi_qcmap_ind: WLAN is STA Mode...\n");
         printf("STA is in %s mode...\n", (ind_data.bridge_mode ? "Bridge" : "Router"));
        }
        else if (ind_data.wlan_mode == QCMAP_MSGR_WLAN_MODE_AP_AP_AP_AP_V01)
        {
         printf("qcmap_msgr_qmi_qcmap_ind: WLAN is AP+AP+AP+AP Mode...\n");
        }
        else
        {
         printf(" Invalid wlan mode %d ...\n",ind_data.wlan_mode);
        }

        for (i=0; i<ind_data.wlan_state_len; i++)
        {
         printf("\n WLAN State for Iface %s \n", ind_data.wlan_state[i].wlan_iface_name);
         printf("IP type %d \n",ind_data.wlan_state[i].ip_type);
         printf("Iface type %d \n",ind_data.wlan_state[i].wlan_iface_type);

         if (ind_data.wlan_state[i].wlan_iface_state == QCMAP_MSGR_WLAN_CONNECTED_V01)
         {
           printf("qcmap_msgr_qmi_qcmap_ind: Iface state is connected...\n");
         }
         else if (ind_data.wlan_state[i].wlan_iface_state == QCMAP_MSGR_WLAN_DISCONNECTED_V01)
         {
           printf("qcmap_msgr_qmi_qcmap_ind: Iface state is disconnected...\n");
         }
         else if (ind_data.wlan_state[i].wlan_iface_state == QCMAP_MSGR_WLAN_CONNECTING_V01)
         {
           printf("qcmap_msgr_qmi_qcmap_ind: Iface state is connecting...\n");
         }
         else
         {
           printf("qcmap_msgr_qmi_qcmap_ind: Incorrect Iface state %d \n",ind_data.wlan_state[i].wlan_iface_state);
         }

         ip4_addr.s_addr = ind_data.wlan_state[i].ip4_addr;
         printf("IP4 address of the iface: %s\n",inet_ntoa(ip4_addr));

         inet_ntop(AF_INET6,(void*)&ind_data.wlan_state[i].ip6_addr, ip6_addr, sizeof(ip6_addr));
         printf("IPv6 Address of the iface: %s\n",ip6_addr);
        }
      }
      break;
   }
   case QMI_QCMAP_MSGR_WLAN_STATUS_EX_IND_V01:
   {
     qcmap_msgr_wlan_status_ex_ind_msg_v01 ind_data;
     qmi_error = qmi_client_message_decode(user_handle,
                                           QMI_IDL_INDICATION,
                                           msg_id,
                                           ind_buf,
                                           ind_buf_len,
                                           &ind_data,
                                           sizeof(qcmap_msgr_wlan_status_ex_ind_msg_v01));
     if (qmi_error != QMI_NO_ERR)
     {
       printf("qcmap_msgr_qmi_qcmap_ind: qmi_client_message_decode error %d",qmi_error);
       break;
     }
     if (ind_data.module_load_status_valid)
     {
       switch (ind_data.module_load_status)
       {
         case QCMAP_MSGR_WLAN_SUCCESS_V01:
         {
           printf("WLAN Module successfully loaded");
           break;
         }

         case QCMAP_MSGR_WLAN_FAILURE_V01:
         {
           printf("WLAN Module failed to load");
           break;
         }

         default:
           break;
       }
     }
     if (ind_data.hostapd_attach_status_valid)
     {
       switch (ind_data.hostapd_attach_status)
       {
         case QCMAP_MSGR_WLAN_SUCCESS_V01:
         {
           printf("HostAPD Attach Success");
           if (ind_data.wlan_iface_name_valid)
           {
             printf(" for Iface Name: %s", ind_data.wlan_iface_name);
           }
           if (ind_data.ap_type_valid)
           {
             printf(" AP Type: %d", ind_data.ap_type);
           }
           break;
         }

         case QCMAP_MSGR_WLAN_FAILURE_V01:
         {
           printf("HostAPD Attach Failure");
           if (ind_data.wlan_iface_name_valid)
           {
             printf(" for Iface Name: %s", ind_data.wlan_iface_name);
           }
           if (ind_data.ap_type_valid)
           {
             printf(" AP Type: %d", ind_data.ap_type);
           }
           break;
         }

         default:
           break;
       }
     }
     break;
   }
   case QMI_QCMAP_MSGR_ETHERNET_MODE_STATUS_IND_V01:
    {
      qcmap_msgr_ethernet_mode_status_ind_msg_v01 ind_data;

      qmi_error = qmi_client_message_decode(user_handle,
                           QMI_IDL_INDICATION,
                           msg_id,
                           ind_buf,
                           ind_buf_len,
                           &ind_data,
                           sizeof(qcmap_msgr_ethernet_mode_status_ind_msg_v01));
      if (qmi_error != QMI_NO_ERR)
      {
        QCMAP_CLI_LOG("qcmap_msgr_qmi_qcmap_ind: "
                      "qmi_client_message_decode error %d",qmi_error);
        break;
      }

      if (ind_data.eth_status == QCMAP_MSGR_ETH_BACKHAUL_CONNECTED_V01)
      {
        QCMAP_CLI_LOG("qcmap_msgr_qmi_qcmap_ind: "
                      "Mobile AP ETHERNET Backhaul Connected...");
        return;
      }
      else if (ind_data.eth_status == QCMAP_MSGR_ETH_BACKHAUL_DISCONNECTED_V01)
      {
        QCMAP_CLI_LOG("qcmap_msgr_qmi_qcmap_ind: "
                      "Mobile AP ETHERNET Backhaul Disconnected...");
        return;
      }
      break;
    }
   case QMI_QCMAP_MSGR_BACKHAUL_STATUS_IND_V01:
     {
       qcmap_msgr_backhaul_status_ind_msg_v01 ind_data;

       qmi_error = qmi_client_message_decode(user_handle,
                           QMI_IDL_INDICATION,
                           msg_id,
                           ind_buf,
                           ind_buf_len,
                           &ind_data,
                           sizeof(qcmap_msgr_backhaul_status_ind_msg_v01));
       if (qmi_error != QMI_NO_ERR)
       {
         QCMAP_CLI_LOG("qcmap_msgr_qmi_qcmap_ind: "
                      "qmi_client_message_decode error %d",qmi_error);
         break;
       }

       if (ind_data.backhaul_type_valid == TRUE)
       {
         if (convert_backhaul_enum_to_string(ind_data.backhaul_type, buffer))
         {
           if (ind_data.backhaul_v4_status_valid == TRUE)
           {
             snprintf(command,MAX_COMMAND_STR_LEN,"IPV4 %s Backhaul %s...",buffer,(ind_data.backhaul_v4_status)?"connected":"disconneted");
             QCMAP_CLI_LOG("qcmap_msgr_qmi_qcmap_ind: %s",command);
           }

           if (ind_data.backhaul_v6_status_valid == TRUE)
           {
             memset(command, 0, MAX_COMMAND_STR_LEN);
             snprintf(command,MAX_COMMAND_STR_LEN,"IPV6 %s Backhaul %s...",buffer,(ind_data.backhaul_v6_status)?"connected":"disconneted");
             QCMAP_CLI_LOG("qcmap_msgr_qmi_qcmap_ind: %s",command);
           }

           if (ind_data.backhaul_eth_status_valid == TRUE)
           {
             memset(command, 0, MAX_COMMAND_STR_LEN);
             snprintf(command,MAX_COMMAND_STR_LEN,"ETH %s Backhaul %s...",buffer,(ind_data.backhaul_eth_status)?"connected":"disconneted");
             QCMAP_CLI_LOG("qcmap_msgr_qmi_qcmap_ind: %s",command);
           }
         }
       }
       break;
     }
   case QMI_QCMAP_MSGR_WWAN_ROAMING_STATUS_IND_V01:
    {
      qcmap_msgr_wwan_roaming_status_ind_msg_v01 ind_data;

      qmi_error = qmi_client_message_decode(user_handle,
                           QMI_IDL_INDICATION,
                           msg_id,
                           ind_buf,
                           ind_buf_len,
                           &ind_data,
                           sizeof(qcmap_msgr_wwan_roaming_status_ind_msg_v01));
      if (qmi_error != QMI_NO_ERR)
      {
        QCMAP_CLI_LOG("qcmap_msgr_qmi_qcmap_ind: "
                      "qmi_client_message_decode error %d",qmi_error);
        break;
      }

      QCMAP_CLI_LOG("qcmap_msgr_qmi_qcmap_ind Roaming status changed to %d",
                     ind_data.wwan_roaming_status);
      printf("qcmap_msgr_qmi_qcmap_ind Roaming status changed to %d",ind_data.wwan_roaming_status);
      return;
    }

    case QMI_QCMAP_MSGR_V2X_SPS_FLOW_REG_RESULT_IND_V01:
    {
      qcmap_msgr_v2x_sps_flow_reg_result_ind_msg_v01 ind_data;

      qmi_error = qmi_client_message_decode(user_handle,
                                            QMI_IDL_INDICATION,
                                            msg_id,
                                            ind_buf,
                                            ind_buf_len,
                                            &ind_data,
                                            sizeof(ind_data));
      if (qmi_error != QMI_NO_ERR)
      {
        QCMAP_CLI_LOG("qcmap_msgr_qmi_qcmap_ind: "
                      "qmi_client_message_decode error %d",qmi_error);
        break;
      }

      printf("   Received V2X_SPS_FLOW_REG_RESULT_IND \n");
      SHOW_MANDATORY_RESPONSE_TO_USER(Request ID, ind_data.reg_result.flow_id.req_id);
      SHOW_MANDATORY_RESPONSE_TO_USER(SPS ID, ind_data.reg_result.flow_id.sps_id);
      SHOW_MANDATORY_RESPONSE_TO_USER(Result, ind_data.reg_result.result);
    }
    break;

    case QMI_QCMAP_MSGR_V2X_SPS_FLOW_DEREG_RESULT_IND_V01:
    {
      qcmap_msgr_v2x_sps_flow_dereg_result_ind_msg_v01 ind_data;

      qmi_error = qmi_client_message_decode(user_handle,
                                            QMI_IDL_INDICATION,
                                            msg_id,
                                            ind_buf,
                                            ind_buf_len,
                                            &ind_data,
                                            sizeof((ind_data)));
      if (qmi_error != QMI_NO_ERR)
      {
        QCMAP_CLI_LOG("qcmap_msgr_qmi_qcmap_ind: "
                      "qmi_client_message_decode error %d",qmi_error);
        break;
      }

      printf("   Received V2X_SPS_FLOW_DEREG_RESULT_IND \n");
      SHOW_MANDATORY_RESPONSE_TO_USER(Request ID, ind_data.dereg_result.flow_id.req_id);
      SHOW_MANDATORY_RESPONSE_TO_USER(SPS ID, ind_data.dereg_result.flow_id.sps_id);
      SHOW_MANDATORY_RESPONSE_TO_USER(Result, ind_data.dereg_result.result);
    }
    break;

    case QMI_QCMAP_MSGR_V2X_SPS_FLOW_UPDATE_RESULT_IND_V01:
    {
      qcmap_msgr_v2x_sps_flow_update_result_ind_msg_v01 ind_data;

      qmi_error = qmi_client_message_decode(user_handle,
                                            QMI_IDL_INDICATION,
                                            msg_id,
                                            ind_buf,
                                            ind_buf_len,
                                            &ind_data,
                                            sizeof(ind_data));
      if (qmi_error != QMI_NO_ERR)
      {
        QCMAP_CLI_LOG("qcmap_msgr_qmi_qcmap_ind: "
                      "qmi_client_message_decode error %d",qmi_error);
        break;
      }

      printf("   Received V2X_SPS_FLOW_UPDATE_RESULT_IND \n");
      SHOW_MANDATORY_RESPONSE_TO_USER(Request ID, ind_data.update_result.flow_id.req_id);
      SHOW_MANDATORY_RESPONSE_TO_USER(SPS ID, ind_data.update_result.flow_id.sps_id);
      SHOW_MANDATORY_RESPONSE_TO_USER(Result, ind_data.update_result.result);
    }
    break;

    case QMI_QCMAP_MSGR_V2X_SERVICE_SUBSCRIBE_RESULT_IND_V01:
    {
      qcmap_msgr_v2x_service_subscribe_result_ind_msg_v01  ind_data;

      qmi_error = qmi_client_message_decode(user_handle,
                                            QMI_IDL_INDICATION,
                                            msg_id,
                                            ind_buf,
                                            ind_buf_len,
                                            &ind_data,
                                            sizeof(ind_data));
      if (qmi_error != QMI_NO_ERR)
      {
        QCMAP_CLI_LOG("qcmap_msgr_qmi_qcmap_ind: "
                      "qmi_client_message_decode error %d",qmi_error);
        break;
      }

      printf("   Received V2X_SERVICE_SUBSCRIBE_RESULT_IND \n");
      SHOW_MANDATORY_RESPONSE_TO_USER(Request ID, ind_data.req_id);
      SHOW_OPTIONAL_RESPONSE_TO_USER(Result of Wildcard Subscription, ind_data.subscribe_wildcard_result);
      SHOW_OPTIONAL_RESPONSE_LIST_TO_USER(Service Subscription Result List, ind_data.reg_result);
    }
    break;

    case QMI_QCMAP_MSGR_V2X_SEND_CONFIG_FILE_RESULT_IND_V01:
    {
      qcmap_msgr_v2x_send_config_file_result_ind_msg_v01   ind_data;

      qmi_error = qmi_client_message_decode(user_handle,
                                            QMI_IDL_INDICATION,
                                            msg_id,
                                            ind_buf,
                                            ind_buf_len,
                                            &ind_data,
                                            sizeof(ind_data));
      if (qmi_error != QMI_NO_ERR)
      {
        QCMAP_CLI_LOG("qcmap_msgr_qmi_qcmap_ind: "
                      "qmi_client_message_decode error %d",qmi_error);
        break;
      }

      printf("   Received V2X_SEND_CONFIG_FILE_RESULT_IND \n");
      SHOW_MANDATORY_RESPONSE_TO_USER(Config File Result, ind_data.result);

    }
    break;

    case QMI_QCMAP_MSGR_V2X_SPS_SCHEDULING_INFO_IND_V01:
    {
      qcmap_msgr_v2x_sps_scheduling_info_ind_msg_v01  ind_data;

      qmi_error = qmi_client_message_decode(user_handle,
                                            QMI_IDL_INDICATION,
                                            msg_id,
                                            ind_buf,
                                            ind_buf_len,
                                            &ind_data,
                                            sizeof(ind_data));
      if (qmi_error != QMI_NO_ERR)
      {
        QCMAP_CLI_LOG("qcmap_msgr_qmi_qcmap_ind: "
                      "qmi_client_message_decode error %d",qmi_error);
        break;
      }

      printf("   Received V2X_SPS_SCHEDULING_INFO_IND \n");
      SHOW_MANDATORY_RESPONSE_TO_USER(SPS ID, ind_data.info.sps_id);
      SHOW_MANDATORY_RESPONSE_TO_USER(Absolute UTC start(nanoSecs), ind_data.info.utc_time);
      SHOW_MANDATORY_RESPONSE_TO_USER(Periodicity of the grant(milliSecs), ind_data.info.periodicity);
    }
    break;

    case QMI_QCMAP_MSGR_V2X_SRC_L2_INFO_IND_V01:
    {
      qcmap_msgr_v2x_src_l2_info_ind_msg_v01  ind_data;

      qmi_error = qmi_client_message_decode(user_handle,
                                            QMI_IDL_INDICATION,
                                            msg_id,
                                            ind_buf,
                                            ind_buf_len,
                                            &ind_data,
                                            sizeof(ind_data));
      if (qmi_error != QMI_NO_ERR)
      {
        QCMAP_CLI_LOG("qcmap_msgr_qmi_qcmap_ind: "
                      "qmi_client_message_decode error %d",qmi_error);
        break;
      }

      printf("   Received V2X_SRC_L2_INFO_IND \n");
      SHOW_MANDATORY_RESPONSE_TO_USER(Source L2 ID, ind_data.src_l2_id);
    }
    break;

    case QMI_QCMAP_MSGR_V2X_CAPABILITY_INFO_IND_V01:
    {
      qcmap_msgr_v2x_capability_info_ind_msg_v01  ind_data;

      qmi_error = qmi_client_message_decode(user_handle,
                                            QMI_IDL_INDICATION,
                                            msg_id,
                                            ind_buf,
                                            ind_buf_len,
                                            &ind_data,
                                            sizeof(ind_data));
      if (qmi_error != QMI_NO_ERR)
      {
        QCMAP_CLI_LOG("qcmap_msgr_qmi_qcmap_ind: "
                      "qmi_client_message_decode error %d",qmi_error);
        break;
      }

      printf("   Received V2X_CAPABILITY_INFO_IND \n");
      SHOW_OPTIONAL_RESPONSE_TO_USER(Maximum Number of SPS Flows, ind_data.max_sps_flow_cnt);
      SHOW_OPTIONAL_RESPONSE_TO_USER(Maximum Number of Event Driven Flows, ind_data.max_event_driven_flow_cnt);
      SHOW_OPTIONAL_RESPONSE_TO_USER(WWAN Concurrency Capability, ind_data.is_concurrency_supported);
      SHOW_OPTIONAL_RESPONSE_TO_USER(Maximum Number of SPS Flows, ind_data.pppp_info); //here
      if (ind_data.pppp_info_valid)
      {
        printf("   Promixmity Service per Packet Priority Information Length=%d\n",
                ind_data.pppp_info_len);
        if (ind_data.pppp_info_len > 0)
        {
          for (int i=0; i<ind_data.pppp_info_len; i++)
          {
            printf("      Priority[%d]=%d\n", i, ind_data.pppp_info[i].priority);
            printf("      PDB[%d]=%d\n", i, ind_data.pppp_info[i].pdb);
          }
        }
      }
      SHOW_OPTIONAL_RESPONSE_TO_USER(Supported Minimum Transmission Power, ind_data.min_tx_pwr);
      SHOW_OPTIONAL_RESPONSE_TO_USER(Supported Maximum Transmission Power, ind_data.max_tx_pwr);
      SHOW_OPTIONAL_RESPONSE_LIST_TO_USER(Supported SPS Periodicity List, ind_data.supported_periodicity_list);
      if (ind_data.tx_pool_id_list_valid)
      {
        printf("   Transmission Pool ID List Length=%d\n", ind_data.tx_pool_id_list_len);
        if (ind_data.tx_pool_id_list_len > 0)
        {
          for (int i=0; i<ind_data.tx_pool_id_list_len; i++)
          {
            printf("      Pool ID[%d]=%d\n", i, ind_data.tx_pool_id_list[i].pool_id);
            printf("      Min freq[%d] in MHz=%d\n", i, ind_data.tx_pool_id_list[i].min_freq);
            printf("      Max freq[%d] in MHz=%d\n", i, ind_data.tx_pool_id_list[i].max_freq);
          }
        }
      }
    }
    break;

    case QMI_QCMAP_MSGR_MODEM_STATUS_IND_V01:
    {
      qcmap_msgr_modem_status_ind_msg_v01 ind_data;

      qmi_error = qmi_client_message_decode(user_handle,
                                            QMI_IDL_INDICATION,
                                            msg_id,
                                            ind_buf,
                                            ind_buf_len,
                                            &ind_data,
                                            sizeof(ind_data));
      if (qmi_error != QMI_NO_ERR)

      {
        QCMAP_CLI_LOG("qcmap_msgr_qmi_qcmap_ind: "
                      "qmi_client_message_decode error %d\n",qmi_error);
        break;
      }

      (ind_data.service_status == 0) ?
        printf("Received MODEM_STATUS_IND: UP\n") :
        printf("Received MODEM_STATUS_IND: DOWN\n");
    }
    break;

    case QCMAP_SERVER_STATUS_IND:
    {
      printf("QCMAP_Server status update on %s: %s\n",
        (((qcmap_server_status_t *)ind_buf)->server_type) ? "EAP" : "MDM",
        (((qcmap_server_status_t *)ind_buf)->server_status) ? "DOWN" : "UP");
    }
    break;

    case QMI_QCMAP_MSGR_MODEM_SERVICE_STATUS_IND_V01:
    {
      qcmap_msgr_modem_service_status_ind_msg_v01 ind_data;

      qmi_error = qmi_client_message_decode(user_handle,
                                            QMI_IDL_INDICATION,
                                            msg_id,
                                            ind_buf,
                                            ind_buf_len,
                                            &ind_data,
                                            sizeof(ind_data));

       if (qmi_error != QMI_NO_ERR)
       {
         QCMAP_CLI_LOG("qcmap_msgr_qmi_qcmap_ind: "
                       "qmi_client_message_decode error %d\n",qmi_error);
         break;
       }

       if(ind_data.subs_id_valid == TRUE)
       {
         QCMAP_CLI_LOG("Subs_id for MODEM_SERVICE_STATUS_IND: %d\n",ind_data.subs_id);
       }
       if (ind_data.modem_service_status_valid == TRUE)
       {
         if (ind_data.modem_service_status == 1)
         {
           QCMAP_CLI_LOG("Received MODEM_SERVICE_STATUS_IND: UP\n");
         }
         if (ind_data.modem_service_status == 0)
         {
           QCMAP_CLI_LOG("Received MODEM_SERVICE_STATUS_IND: DOWN\n");
         }
       }
    }
    break;

    case QMI_QCMAP_MSGR_MTPE_TEST_RESULT_IND_V01:
    {
      printf("Modem Throughput Test result received\n");

      qcmap_msgr_mtpe_test_result_ind_msg_v01 ind_data;

      qmi_error = qmi_client_message_decode(user_handle,
                                            QMI_IDL_INDICATION,
                                            msg_id,
                                            ind_buf,
                                            ind_buf_len,
                                            &ind_data,
                                            sizeof(ind_data));
      if (qmi_error != QMI_NO_ERR)
      {
        QCMAP_CLI_LOG("qcmap_msgr_qmi_qcmap_ind: "
                      "qmi_client_message_decode error %d\n",qmi_error);
        break;
      }

      switch(ind_data.status)
      {
        case QCMAP_MTPE_TEST_SETUP_READY_V01:

          printf("MTPE Test has successfully been configured and is ready to Start\n");

        break;

        case QCMAP_MTPE_TEST_COMPLETE_V01:

          printf("MTPE Test completed successfully!\n");
          printf("Peak rate calculated: %d(kbps)\n", ind_data.peak_rate);

        break;

        case QCMAP_MTPE_TEST_ABORTED_V01:

          printf("MTPE Test was aborted before test duration ended\n");
          if(ind_data.peak_rate_valid)
          {
            printf("Peak rate calculated: %d(kbps)\n", ind_data.peak_rate);
          }

        break;

        case QCMAP_MTPE_TEST_FAILED_V01:
          printf("MTPE Test Failed. Failure reason: 0x%x\n", ind_data.failure_reason);
        break;

        default:
          printf("Invalid test status received: 0x%x\n", ind_data.status);
        break;
      }
    }
    break;
    case QMI_QCMAP_MSGR_GLOBAL_QOS_FLOW_IND_V01:
    {
      qcmap_msgr_global_qos_flow_ind_msg_v01 ind_data;

      qmi_error = qmi_client_message_decode(user_handle,
                                            QMI_IDL_INDICATION,
                                            msg_id,
                                            ind_buf,
                                            ind_buf_len,
                                            &ind_data,
                                            sizeof(ind_data));
      if (qmi_error != QMI_NO_ERR)
      {
        QCMAP_CLI_LOG("qcmap_msgr_qmi_qcmap_ind: "
                      "qmi_client_message_decode error %d\n",qmi_error);
        break;
      }
      printf("Received GLOBAL_QOS_FLOW_IND \n");

       /* Bearer ID valid */
      if(ind_data.bearer_id_valid)
      {
        printf("Received Bearer Id %d \n",ind_data.bearer_id);
      }
      if(ind_data.tx_5g_qci_valid)
      {
        printf("Received Tx 5g qci  %d \n",ind_data.tx_5g_qci);
      }
      if(ind_data.rx_5g_qci_valid)
      {
        printf("Received Rx 5g qci  %d \n",ind_data.rx_5g_qci);
      }
    }
    break;

    case QMI_QCMAP_MSGR_DDS_RECOMMENDATION_IND_V01:
    {
      qcmap_msgr_dds_recommendation_ind_msg_v01 ind_data;

      qmi_error = qmi_client_message_decode(user_handle,
                                            QMI_IDL_INDICATION,
                                            msg_id,
                                            ind_buf,
                                            ind_buf_len,
                                            &ind_data,
                                            sizeof(ind_data));
      if (qmi_error != QMI_NO_ERR)
      {
        QCMAP_CLI_LOG("qcmap_msgr_qmi_qcmap_ind: "
                      "qmi_client_message_decode error %d\n",qmi_error);
        break;
      }

      QCMAP_CLI_LOG("Received DDS_RECOMMENDATION_IND with recommended dds subid: %d\n", ind_data.recommended_dds);
    }
    break;

    case QMI_QCMAP_MSGR_SWITCH_DDS_IND_V01:
    {
      qcmap_msgr_switch_dds_ind_msg_v01 ind_data;

      qmi_error = qmi_client_message_decode(user_handle,
                                            QMI_IDL_INDICATION,
                                            msg_id,
                                            ind_buf,
                                            ind_buf_len,
                                            &ind_data,
                                            sizeof(ind_data));
      if (qmi_error != QMI_NO_ERR)
      {
        QCMAP_CLI_LOG("qcmap_msgr_qmi_qcmap_ind: "
                      "qmi_client_message_decode error %d\n",qmi_error);
        break;
      }

      QCMAP_CLI_LOG("Received SWITCH_DDS_IND\n");

      if(ind_data.dds_switch_result == QCMAP_MSGR_DDS_SWITCH_ALLOWED_V01)
      {
        QCMAP_CLI_LOG("DDS Switch is allowed\n");
      }
      else if(ind_data.dds_switch_result == QCMAP_MSGR_DDS_SWITCH_NOT_ALLOWED_V01)
      {
        QCMAP_CLI_LOG("DDS Switch is not allowed\n");
      }
      else if(ind_data.dds_switch_result == QCMAP_MSGR_DDS_SWITCH_FAILED_V01)
      {
        QCMAP_CLI_LOG("DDS Switch Failed\n");
      }
    }
    break;

    case QMI_QCMAP_MSGR_CURRENT_DDS_IND_V01:
    {
      qcmap_msgr_current_dds_ind_msg_v01 ind_data;

      qmi_error = qmi_client_message_decode(user_handle,
                                            QMI_IDL_INDICATION,
                                            msg_id,
                                            ind_buf,
                                            ind_buf_len,
                                            &ind_data,
                                            sizeof(ind_data));
      if (qmi_error != QMI_NO_ERR)
      {
        QCMAP_CLI_LOG("qcmap_msgr_qmi_qcmap_ind: "
                      "qmi_client_message_decode error %d\n",qmi_error);
        break;
      }

      QCMAP_CLI_LOG("Received CURRENT_DDS_IND with current dds: %d\n", ind_data.dds);
    }
    break;

    case QMI_QCMAP_MSGR_MTPE_HISTORY_IND_V01:
    {
      printf("Recieved an MTPE History Indication...\n");

      qcmap_msgr_mtpe_history_ind_msg_v01 ind_data;

      qmi_error = qmi_client_message_decode(user_handle,
                                            QMI_IDL_INDICATION,
                                            msg_id,
                                            ind_buf,
                                            ind_buf_len,
                                            &ind_data,
                                            sizeof(ind_data));
      if(qmi_error != QMI_NO_ERR)
      {
        QCMAP_CLI_LOG("qcmap_msgr_qmi_qcmap_ind: "
                      "qmi_client_message_decode error %d\n",qmi_error);
        break;
      }

      if(ind_data.transaction_id_valid) {
          QCMAP_CLI_LOG("Indication received with transaction ID : %d", ind_data.transaction_id);
      } else {
          QCMAP_CLI_LOG("Indication received with invalid transaction ID");
          break;
      }


      pthread_mutex_lock(&mtpe_hist_mutex);

      if(!ProcessMTPEHistoryIndication(&ind_data,
                                      mtpe_history_entries,
                                      &len_mtpe_history,
                                      mtpe_txn_id,
                                      &mtpe_history_ready_flag)) {
         QCMAP_CLI_LOG("Failed to process an MTPE History Indication\n");
      }else {
          printf("Processed an MTPE History Indication...\n");

          // All indications from the QCMAP Server received by the client
          if(mtpe_history_ready_flag) {
            printf("Successfully retrieved requested MTPE history items:\n");
            for(int i=0; i<len_mtpe_history; i++) {
                print_mtpe_entry((mtpe_history_entry*)
                                 ((char*)mtpe_history_entries + (sizeof(mtpe_history_entry) *i)));
            }
          }
      }

      pthread_mutex_unlock(&mtpe_hist_mutex);

    }
    break;

   case QMI_QCMAP_MSGR_THROUGHPUT_STATS_IND_V01:
   {
          qcmap_msgr_throughput_stats_ind_msg_v01 ind_data;
          qmi_error = qmi_client_message_decode(user_handle,
                                                QMI_IDL_INDICATION,
                                                msg_id,
                                                ind_buf,
                                                ind_buf_len,
                                                &ind_data,
                                                sizeof(ind_data));
          if (qmi_error != QMI_NO_ERR)
          {
            QCMAP_CLI_LOG("qcmap_msgr_qmi_qcmap_ind: "
                          "qmi throughput indication error %d\n",qmi_error);
            break;
          }
          LOG_MSG_INFO1("The thermal mitigation Indication Received: %d\n", ind_data.throughput, 0, 0);
          QCMAP_CLI_LOG("The thermal mitigation Indication Received: %d\n", ind_data.throughput);

    break;
   }

    default:
      break;
  }

  return;
}

#define WWAN_TECH_TYPE(type)       ( (type == QCMAP_MSGR_MASK_TECH_PREF_ANY_V01)  ? "ANY"  : \
                                    ((type == QCMAP_MSGR_MASK_TECH_PREF_3GPP_V01) ? "3gpp" : \
                                                                                    "3gpp2"))

#define IP_FAMILY_TYPE(ip_family)  (ip_family == QCMAP_MSGR_IP_FAMILY_V4_V01)      ? "V4"    : \
                                      (ip_family == QCMAP_MSGR_IP_FAMILY_V6_V01)   ? "V6"    : \
                                      (ip_family == QCMAP_MSGR_IP_FAMILY_V4V6_V01) ? "V4V6"  : \
                                      (ip_family == QCMAP_MSGR_IP_FAMILY_ETH_V01)  ? "ETH"   : \
                                                                                     "Unknown"

#define SUBSCRIPTION_TYPE(subs_id) (subs_id == QCMAP_MSGR_DEFAULT_SUBS_V01)       ? "Default"    : \
                                      (subs_id == QCMAP_MSGR_PRIMARY_SUBS_V01)    ? "Primary"    : \
                                      (subs_id == QCMAP_MSGR_SECONDARY_SUBS_V01)  ? "Secondary"  : \
                                                                                    "Unknown"

/*===========================================================================
  FUNCTION ShowAllWWANProfiles
  ===========================================================================
  @brief
    Display's all WWAN Profile.

  @input
    default_no - Int numbere of List.
    current_profile_handle - Current Profile Handle selected.

  @return
    true - if it succeeds.
    false - if it fails.

  @dependencies
    none

  @sideefects
  None
  =========================================================================*/
bool ShowAllWWANProfiles(int *default_no,
                         profile_handle_type_v01 *current_profile_handle,
                         uint32_t *profile_number_len = NULL,
                         qcmap_net_profile_and_policy_info *wwan_profile_info = NULL)
{
  qcmap_wwan_policy_list_info wwan_policy_list;
  qmi_error_type_v01 qmi_err_num;

  if (QcMapClient->GetWWANPolicyListEx(&wwan_policy_list, &qmi_err_num))
  {
    if (wwan_policy_list.wwan_policy_len == 1)
    {
      if (wwan_policy_list.default_profile_handle_valid)
      {
        *default_no = wwan_policy_list.default_profile_handle;
        *current_profile_handle = wwan_policy_list.wwan_policy[0].profile_handle;
      }
      return wwan_policy_list.wwan_policy[0].profile_handle;
    }

    QcMapClient->GetWWANProfilePreference(current_profile_handle, &qmi_err_num);

    printf("   +---------+----------+------+------+------------+---------------------------------+\n");
    printf("   | Profile | Subs_Id  | Tech |  IP  | Profile_Id | Profile_Id |       APN Name     |\n");
    printf("   |         |          |      |      |  (3gpp)    |   (3gpp2)  |                    |\n");
    printf("   +---------+----------+------+------+------------+---------------------------------+\n");

    for (int i=0; i < wwan_policy_list.wwan_policy_len; i++)
    {
      if (wwan_policy_list.wwan_policy[i].profile_handle == wwan_policy_list.default_profile_handle)
      {
        *default_no = wwan_policy_list.wwan_policy[i].profile_handle;
        printf("   |%6d(*)|%10s|%6s|%6s|%12d|%12d|%20s|\n",
                wwan_policy_list.wwan_policy[i].profile_handle,
                SUBSCRIPTION_TYPE(wwan_policy_list.wwan_policy[i].subscription_id),
                WWAN_TECH_TYPE(wwan_policy_list.wwan_policy[i].tech_pref),
                IP_FAMILY_TYPE(wwan_policy_list.wwan_policy[i].ip_family),
                wwan_policy_list.wwan_policy[i].profile_id_3gpp,
                wwan_policy_list.wwan_policy[i].profile_id_3gpp2,
                wwan_policy_list.wwan_policy[i].apn_name);
      }
      else
      {
        printf("   |%9d|%10s|%6s|%6s|%12d|%12d|%20s|\n",
                wwan_policy_list.wwan_policy[i].profile_handle,
                SUBSCRIPTION_TYPE(wwan_policy_list.wwan_policy[i].subscription_id),
                WWAN_TECH_TYPE(wwan_policy_list.wwan_policy[i].tech_pref),
                IP_FAMILY_TYPE(wwan_policy_list.wwan_policy[i].ip_family),
                wwan_policy_list.wwan_policy[i].profile_id_3gpp,
                wwan_policy_list.wwan_policy[i].profile_id_3gpp2,
                wwan_policy_list.wwan_policy[i].apn_name);
      }
    }
    printf("   +---------+----------+------+------+------------+---------------------------------+\n");

    return true;
  }

  return false;
}

/*===========================================================================
  FUNCTION sighandler
  ===========================================================================
  @brief
  Signal Handler
  @input
  signal- signal number
  @return
  void
  @dependencies
  Under lying os to generate the signal
  @sideefects
  None
  =========================================================================*/
void sighandler(int signal)
{
  qmi_error_type_v01 qmi_err_num;
  switch (signal)
  {
    case SIGTERM:
    case SIGHUP:
    case SIGINT:
    case SIGKILL:
     if (QcMapClient)
       QcMapClient->DisableMobileAP(&qmi_err_num);

      exit(0);
      break;

    default:
      printf("Received unexpected signal %s\n", signal);
      break;
  }
}
/*===========================================================================
  FUNCTION  check_port
===========================================================================
 @brief
   Port value is validated against the range 1 - MAX_PORT_VALUE
 @input
   sport - port value
 @return
   0  - success
   -1 - failure
 @dependencies
   None
 @sideefects
   None
=========================================================================*/
int16_t
check_port (uint32 sport)
{
  if((sport > MAX_PORT_VALUE) || (sport < 1) )
  {
    printf(" port value should be between 1 - %d\n",MAX_PORT_VALUE);
    return -1;
  }
  else
    return 0;
}

/*===========================================================================
  FUNCTION  check_proto
===========================================================================
 @brief
   protocol value is validated against the range 1 - MAX_PROTO_VALUE
 @input
   sport - protocol value
 @return
   0  - success
   -1 - failure
 @dependencies
   None
 @sideefects
   None
=========================================================================*/
int16_t
check_proto (uint8 sport)
{
  if( sport > MAX_PROTO_VALUE )
  {
    printf(" port value should be between 1 - %d\n",MAX_PROTO_VALUE);
    return -1;
  }
  else
    return 0;
}

/*===========================================================================
  FUNCTION  check_tos
===========================================================================
 @brief
   Tos value is validated against the range 1 - MAX_TOS_VALUE
 @input
   tos - port value
 @return
   0  - success
   -1 - failure
 @dependencies
   None
 @sideefects
   None
=========================================================================*/
int16_t
check_tos (uint8 tos)
{
  if( tos > MAX_TOS_VALUE )
  {
    printf(" Tos value should be between 0 - %d\n",MAX_TOS_VALUE);
    return -1;
  }
  else
    return 0;
}

/*===========================================================================
  FUNCTION read_addr
  ===========================================================================
  @brief
  Read address funtion will read the address from the user
  @input
  domain - identifies ipv4 or ipv6 domain
  addr   - contains the numeric address, it's an output value.
  @return
  0  - success
  -1 - failure
  @dependencies
  It depends on inet_pton()
  @sideefects
  None
  =========================================================================*/
int read_addr(int domain,uint8 *addr)
{
  unsigned char buf[50]={0};
  char *ptr=NULL;

  bzero(buf,sizeof(buf));

  read_again:
  if ( domain == AF_INET )
  {
    QCMAP_PRINTF_TAKE_INPUT("\n Please Enter V4 Address Value =>");
  }
  else
  if(domain == AF_INET6)
  {
     QCMAP_PRINTF_TAKE_INPUT("\n Please Enter V6 Address Value =>");
  }
  else
  QCMAP_PRINTF_TAKE_INPUT("\n Please Enter Address Mask Value =>");

  if ( fgets(buf, sizeof(buf), stdin) != NULL )
  {
    ptr = strchr((char *)&buf,'\n');

    if( ptr )
      *ptr = 0;

    if ( inet_pton(domain,buf,addr) <=0 ) {
      QCMAP_PRINTF_TAKE_INPUT("\n Address not in presentation format\n");
      goto read_again;
    }
  }
  return 0;
}

/*===========================================================================
  FUNCTION read_uint8
  ===========================================================================
  @brief
  Read uint8 value from user.
  @input
  void
  @return
  int value read from user.
  @dependencies
  usr to provide input
  @sideefects
  None
  =========================================================================*/

uint8 read_uint8()
{
  char  scan_string[50];
  uint8 result;
  uint32 src_addr[4]={0};
  bzero(scan_string,sizeof(scan_string));
  fgets(scan_string, sizeof(scan_string), stdin);
  result = atoi(scan_string);
  return result;
}

/*===========================================================================
  FUNCTION read_uint32
  ===========================================================================
  @brief
  Read integer value from user.
  @input
  void
  @return
  int value read from user.
  @dependencies
  usr to provide input
  @sideefects
  None
  =========================================================================*/
int read_uint32()
{
  char  scan_string[50];
  int result;
  bzero(scan_string,sizeof(scan_string));
  uint32 src_addr[4]={0};
  fgets(scan_string, sizeof(scan_string), stdin);
  if (!VALID_NUMERIC_INPUT(scan_string))
  {
    return QCMAP_LAN_INVALID;
  }
  result = atoi(scan_string);
  return result;
}

#define GET_MAC_ADDR(scan_string,mac_addr_int) {\
    memset(scan_string,0,QCMAP_MSGR_MAX_FILE_PATH_LEN);\
    while(TRUE)\
    {\
          fflush(stdout);\
          if ( fgets(scan_string,sizeof(scan_string),stdin) != NULL )\
          {\
              scan_string[strlen(scan_string)-1]='\0'; \
              if ( ds_mac_addr_pton(scan_string,mac_addr_int))\
                break; \
          }\
          printf("Invalid address entered %s\n", scan_string);\
          printf("Enter valid address: ");\
        }\
 }

#define GET_IP_ADDR(scan_string, ip_addr) {\
    memset(scan_string,0,QCMAP_MSGR_MAX_FILE_PATH_LEN);\
    while(TRUE)\
    {\
         fflush(stdout);\
         if ( fgets(scan_string,sizeof(scan_string),stdin) != NULL )\
         {\
              scan_string[strlen(scan_string)-1]='\0'; \
              if ( (inet_aton(scan_string, &ip_addr)) && \
                   ( ip_addr.s_addr != 0 ) &&\
                   ( ip_addr.s_addr != 0xffffffff ))\
               break; \
          }\
          printf("Invalid IPv4 Address entered: %s\n", scan_string);\
     }\
 }

/*===========================================================================
  FUNCTION read_firewall_conf
  ===========================================================================
  @brief
  Read firewall configuration value from user and calls add fireall entry.
  @input
  void
  @return
  true  - success
  false - failure
  @dependencies
  usr to provide input
  @sideefects
  None
  =========================================================================*/
boolean read_firewall_conf(qcmap_msgr_firewall_conf_t *extd_firewall_add)
{
  unsigned char buf[50];
  int val,icmp_proto_val = 0;
  int protocol_entered = 0;
  memset(extd_firewall_add, 0, sizeof(qcmap_msgr_firewall_conf_t));

  while(1)
  {
    QCMAP_PRINTF_TAKE_INPUT("\nPlease input Direction [1-UL/2-DL]:");
    val = read_uint32();
    if( val == QCMAP_MSGR_UL_FIREWALL || val == QCMAP_MSGR_DL_FIREWALL )
     break;
  }
  extd_firewall_add->extd_firewall_entry.firewall_direction = val;

  while(1)
  {
    QCMAP_PRINTF_TAKE_INPUT("\nPlease input IP family [4-IPV4/6-IPV6]:");
    val = read_uint32();
    if( val == IP_V4 || val== IP_V6 )
    break;
  }
  extd_firewall_add->extd_firewall_entry.filter_spec.ip_vsn = val;

  if(extd_firewall_add->extd_firewall_entry.filter_spec.ip_vsn == IP_V4)
  {
    while(1)
    {
      printf("\nDo you want to enter IPV4 source address\n");
      QCMAP_PRINTF_TAKE_INPUT("  and subnet mask: [1-YES 0-NO]:");
      val = read_uint32();
      if( val == 1 || val == 0 )
        break;
    }
    if(val == 1)
    {
      QCMAP_PRINTF_TAKE_INPUT("\nPlease enter Src Addr [xxx.xxx.xxx.xxx]:");
      read_addr(AF_INET,(char *)&extd_firewall_add->extd_firewall_entry.filter_spec.ip_hdr.v4.src.addr.ps_s_addr);
      extd_firewall_add->extd_firewall_entry.filter_spec.ip_hdr.v4.field_mask |= IPFLTR_MASK_IP4_SRC_ADDR;
      QCMAP_PRINTF_TAKE_INPUT("\nPlease enter IPV4 src subnet mask [xxx.xxx.xxx.xxx]:");
      read_addr(AF_INET,(uint8* )&extd_firewall_add->extd_firewall_entry.filter_spec.ip_hdr.v4.src.subnet_mask.ps_s_addr);
    }

    while(1)
    {
      printf("\nDo you want to enter IPV4 destination address\n");
      QCMAP_PRINTF_TAKE_INPUT("  and subnet mask: [1-YES 0-NO]:");
      val = read_uint32();
      if( val == 1 || val == 0 )
        break;
    }
    if(val == 1)
    {
      QCMAP_PRINTF_TAKE_INPUT("\nPlease enter Dst Addr [xxx.xxx.xxx.xxx]:");
      read_addr(AF_INET,(char *)&extd_firewall_add->extd_firewall_entry.filter_spec.ip_hdr.v4.dst.addr.ps_s_addr);
      extd_firewall_add->extd_firewall_entry.filter_spec.ip_hdr.v4.field_mask |= IPFLTR_MASK_IP4_DST_ADDR;
      QCMAP_PRINTF_TAKE_INPUT("\nPlease enter IPV4 Dst subnet mask [xxx.xxx.xxx.xxx]:");
      read_addr(AF_INET,(uint8* )&extd_firewall_add->extd_firewall_entry.filter_spec.ip_hdr.v4.dst.subnet_mask.ps_s_addr);
    }

    printf("\nDo you want to enter IPV4 TOS value\n");
    QCMAP_PRINTF_TAKE_INPUT("and TOS mask: [1-YES 0-NO]:");
    if(read_uint32() == 1)
    {
      while(1)
      {
        QCMAP_PRINTF_TAKE_INPUT("\nPlease enter the TOS value [0 to 255]:");
        val = read_uint32();
        if( check_tos(val)!= -1)
          break;
      }
      extd_firewall_add->extd_firewall_entry.filter_spec.ip_hdr.v4.tos.val =val;
      while(1)
      {
        QCMAP_PRINTF_TAKE_INPUT("\nPlease enter the TOS mask [0 to 255]:");
        val = read_uint32();
        if( check_tos( val )!= -1)
          break;
      }
      extd_firewall_add->extd_firewall_entry.filter_spec.ip_hdr.v4.tos.mask = val;
      extd_firewall_add->extd_firewall_entry.filter_spec.ip_hdr.v4.field_mask |= IPFLTR_MASK_IP4_TOS;
    }
  }
  else
  {
    while(1)
    {
      QCMAP_PRINTF_TAKE_INPUT("\nDo you want to enter IPV6 source address\n"
           "and subnet mask: [1-YES 0-NO]:");
      val = read_uint32();
      if( val == 1 || val == 0 )
        break;
    }
    if(val == 1)
    {
      QCMAP_PRINTF_TAKE_INPUT("\nPlease enter Src Addr [xxxx:xxxx::xxxx:xxxx:xxxx]:");
      read_addr(AF_INET6,(uint8 *)&extd_firewall_add->extd_firewall_entry.filter_spec.ip_hdr.v6.src.addr.in6_u.u6_addr8);
      extd_firewall_add->extd_firewall_entry.filter_spec.ip_hdr.v6.field_mask |= IPFLTR_MASK_IP6_SRC_ADDR;
      QCMAP_PRINTF_TAKE_INPUT("\nPlease enter IPV6 src prefixlen:");
      extd_firewall_add->extd_firewall_entry.filter_spec.ip_hdr.v6.src.prefix_len = read_uint32();
    }

    while(1)
    {
      QCMAP_PRINTF_TAKE_INPUT("\nDo you want to enter IPV6 destination address\n"
           "and subnet mask: [1-YES 0-NO]:");
      val = read_uint32();
      if( val == 1 || val == 0 )
        break;
    }
    if(val == 1)
    {
      QCMAP_PRINTF_TAKE_INPUT("\nPlease enter Dst Addr [xxxx:xxxx::xxxx:xxxx:xxxx]:");
      read_addr(AF_INET6,(uint8 *)&extd_firewall_add->extd_firewall_entry.filter_spec.ip_hdr.v6.dst.addr.in6_u.u6_addr8);
      extd_firewall_add->extd_firewall_entry.filter_spec.ip_hdr.v6.field_mask |= IPFLTR_MASK_IP6_DST_ADDR;
      QCMAP_PRINTF_TAKE_INPUT("\nPlease enter IPV6 dst prefixlen:");
      extd_firewall_add->extd_firewall_entry.filter_spec.ip_hdr.v6.dst.prefix_len = read_uint32();
    }

    QCMAP_PRINTF_TAKE_INPUT("IPv6 NAT enable: [1-YES 0-NO]");
    if (read_uint32() == 1)
    {
      extd_firewall_add->extd_firewall_entry.filter_spec.ip_hdr.v6.ipv6_nat_enabled = 1;
    }
    else
    {
      extd_firewall_add->extd_firewall_entry.filter_spec.ip_hdr.v6.ipv6_nat_enabled = 0;
    }
    QCMAP_PRINTF_TAKE_INPUT("\nDo you want to enter IPV6 Traffic Class value\n"
           "and subnet mask: [1-YES 0-NO]:");
    if(read_uint32() == 1)
    {
      QCMAP_PRINTF_TAKE_INPUT("\nPlease enter IPV6 Class value:");
      extd_firewall_add->extd_firewall_entry.filter_spec.ip_hdr.v6.trf_cls.val=read_uint32();
      extd_firewall_add->extd_firewall_entry.filter_spec.ip_hdr.v6.field_mask |= IPFLTR_MASK_IP6_TRAFFIC_CLASS;
      QCMAP_PRINTF_TAKE_INPUT("\nPlease enter IPV6 class mask value:");
      extd_firewall_add->extd_firewall_entry.filter_spec.ip_hdr.v6.trf_cls.mask = read_uint32();
    }
  }

  QCMAP_PRINTF_TAKE_INPUT("\nDo you want to enter Protocol [1-YES 0-NO]:");
  if((protocol_entered = read_uint32()) == 1)
  {
    read_proto_again:
    QCMAP_PRINTF_TAKE_INPUT("\nPlease input IPV4 next header protocol\n");
    if( extd_firewall_add->extd_firewall_entry.filter_spec.ip_vsn == IP_V4 )
    {
      printf("TCP=%d, UDP=%d,ICMP=%d,TCP_UDP=%d,ESP=%d ",
                   PS_IPPROTO_TCP, PS_IPPROTO_UDP, PS_IPPROTO_ICMP,\
                   PS_IPPROTO_TCP_UDP, PS_IPPROTO_ESP);
      icmp_proto_val = PS_IPPROTO_ICMP;
    }
    else if (extd_firewall_add->extd_firewall_entry.filter_spec.ip_vsn == IP_V6)
    {
      printf("TCP=%d, UDP=%d,ICMP6=%d,TCP_UDP=%d,ESP=%d ",
                   PS_IPPROTO_TCP, PS_IPPROTO_UDP, PS_IPPROTO_ICMP6,\
                   PS_IPPROTO_TCP_UDP, PS_IPPROTO_ESP);
      icmp_proto_val = PS_IPPROTO_ICMP6;
    }
    fflush(stdout);
    val = read_uint32();
    if( val == PS_IPPROTO_TCP || val == PS_IPPROTO_UDP || val == icmp_proto_val\
        || val == PS_IPPROTO_ESP || val == PS_IPPROTO_TCP_UDP )
    {
      if( extd_firewall_add->extd_firewall_entry.filter_spec.ip_vsn == IP_V4 )
      {
        extd_firewall_add->extd_firewall_entry.filter_spec.ip_hdr.v4.next_hdr_prot = val;
        extd_firewall_add->extd_firewall_entry.filter_spec.ip_hdr.v4.field_mask |= IPFLTR_MASK_IP4_NEXT_HDR_PROT;
      }
      else if(extd_firewall_add->extd_firewall_entry.filter_spec.ip_vsn == IP_V6 )
      {
        extd_firewall_add->extd_firewall_entry.filter_spec.ip_hdr.v6.next_hdr_prot = val;
        extd_firewall_add->extd_firewall_entry.filter_spec.ip_hdr.v6.field_mask |= IPFLTR_MASK_IP6_NEXT_HDR_PROT;
      }
    }else
    {
      printf("Invalid selection. Please select protocol again.\n");
      goto read_proto_again;
    }
  }

  /* Return if protocol field is not selected as
   * firewall can be configured without the next header
   * protocol.
   */
  if(protocol_entered != 1)
  {
    return true;
  }

  if(val == PS_IPPROTO_TCP)
  {
    QCMAP_PRINTF_TAKE_INPUT("\nDo you want to enter Source Port and Range [1-YES 0-NO]:");
    if(read_uint32() == 1)
    {
      read_tcp_src_single_port:
      QCMAP_PRINTF_TAKE_INPUT("\nPlease input TCP source port:");
      val = read_uint32();
      if( check_port (val) == -1)
        goto read_tcp_src_single_port;
      extd_firewall_add->extd_firewall_entry.filter_spec.next_prot_hdr.tcp.src.port = val;
      extd_firewall_add->extd_firewall_entry.filter_spec.next_prot_hdr.tcp.field_mask |= IPFLTR_MASK_TCP_SRC_PORT;

      read_tcp_start_port:
      QCMAP_PRINTF_TAKE_INPUT("\nPlease enter TCP source port range:");
      val = read_uint32();
      if( check_port ((extd_firewall_add->extd_firewall_entry.filter_spec.next_prot_hdr.tcp.src.port+val)) == -1 )
        goto read_tcp_start_port;
      extd_firewall_add->extd_firewall_entry.filter_spec.next_prot_hdr.tcp.src.range = val;
    }

    QCMAP_PRINTF_TAKE_INPUT("\nDo you want to enter Destination Port and Range [1-YES 0-NO]:");
    val = read_uint32();
    if(val ==  1)
    {
      read_tcp_dst_port:
      QCMAP_PRINTF_TAKE_INPUT("\nPlease input TCP destination port:");
      val = read_uint32();
      if( check_port (val) == -1)
        goto read_tcp_dst_port;
      extd_firewall_add->extd_firewall_entry.filter_spec.next_prot_hdr.tcp.dst.port = val;
      extd_firewall_add->extd_firewall_entry.filter_spec.next_prot_hdr.tcp.field_mask |= IPFLTR_MASK_TCP_DST_PORT;

      read_tcp_dst_port_range:
      QCMAP_PRINTF_TAKE_INPUT("\nPlease enter TCP destination port range:");
      val = read_uint32();
      if( check_port ((extd_firewall_add->extd_firewall_entry.filter_spec.next_prot_hdr.tcp.dst.port + val)) == -1 )
        goto read_tcp_dst_port_range;
      extd_firewall_add->extd_firewall_entry.filter_spec.next_prot_hdr.tcp.dst.range = val;
    }
  }else if( val == PS_IPPROTO_UDP)
  {
    QCMAP_PRINTF_TAKE_INPUT("\nDo you want to enter Source Port and Range [1-YES 0-NO]:");
    val = read_uint32();
    if(val ==  1)
    {
      read_udp_src_single_port:
      QCMAP_PRINTF_TAKE_INPUT("\nPlease input udp source port:");
      val = read_uint32();
      if( check_port (val) == -1)
        goto read_udp_src_single_port;
      extd_firewall_add->extd_firewall_entry.filter_spec.next_prot_hdr.udp.src.port = val;
      extd_firewall_add->extd_firewall_entry.filter_spec.next_prot_hdr.udp.field_mask |= IPFLTR_MASK_UDP_SRC_PORT;

      read_udp_start_port:
      QCMAP_PRINTF_TAKE_INPUT("\nPlease enter udp source port range:");
      val = read_uint32();
      if( check_port ((extd_firewall_add->extd_firewall_entry.filter_spec.next_prot_hdr.udp.src.port+val)) == -1 )
        goto read_udp_start_port;
      extd_firewall_add->extd_firewall_entry.filter_spec.next_prot_hdr.udp.src.range = val;
    }
    QCMAP_PRINTF_TAKE_INPUT("\nDo you want to enter Destination Port and Range [1-YES 0-NO]:");
    val = read_uint32();
    if( val == 1)
    {
      read_udp_dst_port:
      QCMAP_PRINTF_TAKE_INPUT("\nPlease input udp destination port:");
      val = read_uint32();
      if( check_port (val) == -1)
        goto read_udp_dst_port;
      extd_firewall_add->extd_firewall_entry.filter_spec.next_prot_hdr.udp.dst.port = val;
      extd_firewall_add->extd_firewall_entry.filter_spec.next_prot_hdr.udp.field_mask |= IPFLTR_MASK_UDP_DST_PORT;

      read_udp_dst_end_port:
      QCMAP_PRINTF_TAKE_INPUT("\nPlease enter udp destination port range:");
      val = read_uint32();
      if( check_port ((extd_firewall_add->extd_firewall_entry.filter_spec.next_prot_hdr.udp.dst.port+val)) == -1 )
        goto read_udp_dst_end_port;
      extd_firewall_add->extd_firewall_entry.filter_spec.next_prot_hdr.udp.dst.range = val;
    }
  }
  else if (val == PS_IPPROTO_TCP_UDP)
  {
    QCMAP_PRINTF_TAKE_INPUT("\nDo you want to enter Source Port and Range [1-YES 0-NO]:");
    if(read_uint32() == 1)
    {
     read_tcp_udp_port_range_src_single_port:
     QCMAP_PRINTF_TAKE_INPUT("\nPlease input tcp_udp source port:");
     val = read_uint32();
     if( check_port (val) == -1)
     goto read_tcp_udp_port_range_src_single_port;
     extd_firewall_add->extd_firewall_entry.filter_spec.next_prot_hdr.tcp_udp_port_range.src.port = val;
     extd_firewall_add->extd_firewall_entry.filter_spec.next_prot_hdr.tcp_udp_port_range.field_mask |= IPFLTR_MASK_TCP_UDP_SRC_PORT;

    read_tcp_udp_port_range_start_port:
    QCMAP_PRINTF_TAKE_INPUT("\nPlease enter tcp_udp source port range:");
     val = read_uint32();
     if( check_port ((extd_firewall_add->extd_firewall_entry.filter_spec.next_prot_hdr.tcp_udp_port_range.src.port+val)) == -1 )
     goto read_tcp_udp_port_range_start_port;
     extd_firewall_add->extd_firewall_entry.filter_spec.next_prot_hdr.tcp_udp_port_range.src.range = val;
    }
    QCMAP_PRINTF_TAKE_INPUT("\nDo you want to enter Destination Port and Range [1-YES 0-NO]:");
    if(read_uint32() == 1)
    {
      read_tcp_udp_port_range_dst_port:
      QCMAP_PRINTF_TAKE_INPUT("\nPlease input tcp_udp destination port:");
      val = read_uint32();
      if( check_port (val) == -1)
        goto read_tcp_udp_port_range_dst_port;
      extd_firewall_add->extd_firewall_entry.filter_spec.next_prot_hdr.tcp_udp_port_range.dst.port = val;
      extd_firewall_add->extd_firewall_entry.filter_spec.next_prot_hdr.tcp_udp_port_range.field_mask |= IPFLTR_MASK_TCP_UDP_DST_PORT;
      read_tcp_udp_port_range_dst_end_port:
      QCMAP_PRINTF_TAKE_INPUT("\nPlease enter tcp_udp destination port range:");
      val = read_uint32();
      if( check_port ((extd_firewall_add->extd_firewall_entry.filter_spec.next_prot_hdr.tcp_udp_port_range.dst.port + val)) == -1 )
        goto read_tcp_udp_port_range_dst_end_port;
      extd_firewall_add->extd_firewall_entry.filter_spec.next_prot_hdr.tcp_udp_port_range.dst.range = val;
    }
  }else if ( val == PS_IPPROTO_ICMP || val == PS_IPPROTO_ICMP6)
  {
    QCMAP_PRINTF_TAKE_INPUT("\nDo you want to enter ICMP Type [1-YES 0-NO]:");
    if(read_uint32() == 1)
    {
      QCMAP_PRINTF_TAKE_INPUT("\nPlease enter the ICMP Type value:");
      extd_firewall_add->extd_firewall_entry.filter_spec.next_prot_hdr.icmp.type = read_uint32();
      extd_firewall_add->extd_firewall_entry.filter_spec.next_prot_hdr.icmp.field_mask |= IPFLTR_MASK_ICMP_MSG_TYPE;

      QCMAP_PRINTF_TAKE_INPUT("\nDo you want to enter ICMP code [1-YES 0-NO]:");
      if(read_uint32() == 1)
      {
        QCMAP_PRINTF_TAKE_INPUT("\nPlease enter ICMP code value:");
        extd_firewall_add->extd_firewall_entry.filter_spec.next_prot_hdr.icmp.code = read_uint32();
        extd_firewall_add->extd_firewall_entry.filter_spec.next_prot_hdr.icmp.field_mask |= IPFLTR_MASK_ICMP_MSG_CODE;
      }
    }
  }else if( val == PS_IPPROTO_ESP)
  {
    QCMAP_PRINTF_TAKE_INPUT("\nDo you want to enter ESP SPI value [1-YES 0-NO]:");
    if(read_uint32() == 1)
    {
      QCMAP_PRINTF_TAKE_INPUT("\nPlease enter the ESP SPI value:");
      extd_firewall_add->extd_firewall_entry.filter_spec.next_prot_hdr.esp.spi = read_uint32();
      extd_firewall_add->extd_firewall_entry.filter_spec.next_prot_hdr.esp.field_mask |= IPFLTR_MASK_ESP_SPI;
    }
  }
  return true;
}


/*===========================================================================
  FUNCTION readable_addr
  ===========================================================================
  @brief
    convert a numeric address into a text string suitable
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
int readable_addr(int domain,uint32_t *addr, char *str)
{
  if (inet_ntop(domain, addr, str, INET6_ADDRSTRLEN) == NULL)
  {
    printf("\n Not in presentation format \n");
    return -1;
  }

  return 0;
}

/*===========================================================================
  FUNCTION DisplayIPv4State
  ===========================================================================*/
/*!
  @brief
    Prints the state of IPv4 (enabled or disabled)

  @return
  void

  @note

  - Dependencies
  - None

  - Side Effects
  - None
*/
/*=========================================================================*/
void DisplayIPv4State()
{
  boolean ipv4_state = false;
  qmi_error_type_v01  qmi_err_num;

  if (QcMapClient->GetIPv4State(&ipv4_state, &qmi_err_num))
  {
    printf("\nIPV4 is: %s.\n",(ipv4_state)?"Enabled":"Disabled");
  }
  else
  {
    printf("\nGetIPV4State returns Error: 0x%x", qmi_err_num);
  }
}

/*===========================================================================
  FUNCTION DisplayIPv6State
  ===========================================================================
  @brief
    Prints the state of IPv6 (enabled or disabled)

  @input
    void

  @return
    void

  @dependencies
    none

  @sideefects
    None
  =========================================================================*/
void DisplayIPv6State()
{
  boolean ipv6_state;
  qmi_error_type_v01  qmi_err_num;

  memset((void *)&ipv6_state, 0, sizeof(uint8_t));

  if (QcMapClient->GetIPv6State( &ipv6_state, &qmi_err_num))
  {
    printf("\nIPV6 is: %s.\n",(ipv6_state)?"Enabled":"Disabled");
  }
  else
  {
    printf("\nGetIPV6State returns Error: 0x%x", qmi_err_num);
  }
}

/*===========================================================================
  FUNCTION DisplayWWANPolicy
  ===========================================================================
  @brief
    Displays the current WWAN network policy

  @input
    void

  @return
    void

  @dependencies
    usr to provide input

  @sideefects
    None
  =========================================================================*/
void DisplayWWANPolicy()
{
  qcmap_net_policy_info wwan_policy;
  memset(&wwan_policy,0,sizeof(qcmap_msgr_net_policy_info_v01));
  qmi_error_type_v01      qmi_err_num;

  if ( QcMapClient->GetWWANPolicyEx(&wwan_policy, &qmi_err_num))
  {
    printf("\nTech preference is :");
    switch (wwan_policy.tech_pref)
    {
      case 0:
        printf("ANY \n");
        break;

      case 1:
        printf("UMTS \n");
        break;

      case 2:
        printf("CDMA \n");
        break;

      default:
        printf("Error: invalid tech preference:- %d\n",wwan_policy.tech_pref);
        break;
    }
    printf("\n3gpp profile id is: %d.\n",
            wwan_policy.profile_id_3gpp);
    printf("\n3gpp2 pofile id is: %d.\n",
            wwan_policy.profile_id_3gpp2);
    printf("\nAPN Name is: %s.\n",
            wwan_policy.apn_name);
  }
  else
  {
    printf("\nGet WWAN policy failed Error 0x%x.\n", qmi_err_num);
  }
}


/*===========================================================================
  FUNCTION DisplayFirewall
  ===========================================================================
  @brief
    Displays the firewall configuration rules for Mobile AP

  @input
    handle_list_length - handle list length
    extd_firewall_handle_list -  firewall configuration list

  @return
    void

  @dependencies
    usr to provide input

  @sideefects
    None
  =========================================================================*/
boolean DisplayFirewall(int handle_list_len,
                        qcmap_msgr_firewall_conf_t extd_firewall_handle_list)
{
  char str[INET6_ADDRSTRLEN];
  qcmap_msgr_firewall_conf_t extd_firewall_get;
  int   index;
  int   result;
  int next_hdr_prot = 0;
  qcmap_msgr_firewall_entry_conf_t *firewall_entry;
  qmi_error_type_v01  qmi_err_num;

  if(handle_list_len > 0)
  {
    for(index =0; index < handle_list_len; index++)
    {
      memset(&extd_firewall_get, 0, sizeof(qcmap_msgr_firewall_conf_t));
      extd_firewall_get.extd_firewall_entry.filter_spec.ip_vsn =
        extd_firewall_handle_list.extd_firewall_handle_list.ip_family;
      extd_firewall_get.extd_firewall_entry.firewall_handle =
        extd_firewall_handle_list.extd_firewall_handle_list.handle_list[index];

      if(QcMapClient->GetFireWallEntry(&extd_firewall_get.extd_firewall_entry, &qmi_err_num))
      {
        firewall_entry=&extd_firewall_get.extd_firewall_entry;
        printf("\n### Start Displaying firewall configuration of handle =%d ###",
               extd_firewall_get.extd_firewall_entry.firewall_handle);
        if(firewall_entry!=NULL)
        {
          if (firewall_entry->firewall_direction == QCMAP_MSGR_UL_FIREWALL)
            printf("\nUL Firewall Rule");
          else
            printf("\nDL Firewall Rule");
          if( firewall_entry->filter_spec.ip_vsn == IP_V4 )
          {
            printf("\nIp version : IPv4");
            if( firewall_entry->filter_spec.ip_hdr.v4.field_mask & IPFLTR_MASK_IP4_SRC_ADDR)
            {
              readable_addr(AF_INET,&firewall_entry->filter_spec.ip_hdr.v4.src.addr.ps_s_addr,(char *)&str);
              printf("\nSRC Addr : %s",str);
              readable_addr(AF_INET,&firewall_entry->filter_spec.ip_hdr.v4.src.subnet_mask.ps_s_addr,(char *)&str);
              printf("\nSRC Addr Mask : %s",str);
            }
            else
              printf("\nSRC Addr : Any");

            if( firewall_entry->filter_spec.ip_hdr.v4.field_mask & IPFLTR_MASK_IP4_DST_ADDR)
            {
              readable_addr(AF_INET,&firewall_entry->filter_spec.ip_hdr.v4.dst.addr.ps_s_addr,(char *)&str);
              printf("\nDST Addr : %s",str);
              readable_addr(AF_INET,&firewall_entry->filter_spec.ip_hdr.v4.dst.subnet_mask.ps_s_addr,(char *)&str);
              printf("\nDST Addr Mask : %s",str);
            }
            else
              printf("\nDST Addr : Any");

            if(  firewall_entry->filter_spec.ip_hdr.v4.field_mask & IPFLTR_MASK_IP4_TOS)
            {
              printf("\nTos value : %x ",firewall_entry->filter_spec.ip_hdr.v4.tos.val);
              printf("\nTos Mask : %x ",firewall_entry->filter_spec.ip_hdr.v4.tos.mask);
            }
            else
              printf("\nTos value : Any");

            if( firewall_entry->filter_spec.ip_hdr.v4.field_mask & IPFLTR_MASK_IP4_NEXT_HDR_PROT )
            {
              next_hdr_prot = firewall_entry->filter_spec.ip_hdr.v4.next_hdr_prot;
            }
          }
          else
          {
            printf("\nIp version : Ipv6");
            if( firewall_entry->filter_spec.ip_hdr.v6.field_mask & IPFLTR_MASK_IP6_SRC_ADDR)
            {
              readable_addr(AF_INET6,(uint32 *)&firewall_entry->filter_spec.ip_hdr.v6.src.addr.in6_u.u6_addr32,(char *)&str);
              printf("\nSrc Addr : %s ",str);
              printf("\nSrc Prefixlen : %d ",firewall_entry->filter_spec.ip_hdr.v6.src.prefix_len);
            }
            else
              printf("\nSRC Addr : Any");

            if( firewall_entry->filter_spec.ip_hdr.v6.field_mask & IPFLTR_MASK_IP6_DST_ADDR)
            {
              readable_addr(AF_INET6,(uint32 *)&firewall_entry->filter_spec.ip_hdr.v6.dst.addr.in6_u.u6_addr32,(char *)&str);
              printf("\nDST Addr : %s ",str);
              printf("\nDST Prefixlen : %d ",firewall_entry->filter_spec.ip_hdr.v6.dst.prefix_len);
            }
            else
              printf("\nDST Addr : Any");

            if( extd_firewall_get.extd_firewall_entry.filter_spec.ip_hdr.v6.field_mask &
                IPFLTR_MASK_IP6_TRAFFIC_CLASS )
            {
              printf("\n IPV6 Traffic class value: %d",
                  extd_firewall_get.extd_firewall_entry.filter_spec.ip_hdr.v6.trf_cls.val);
              printf("\n IPV6 Traffic class mask: %d",
                  extd_firewall_get.extd_firewall_entry.filter_spec.ip_hdr.v6.trf_cls.mask);
            }

            if( firewall_entry->filter_spec.ip_hdr.v6.field_mask & IPFLTR_MASK_IP6_NEXT_HDR_PROT )
            {
              next_hdr_prot = firewall_entry->filter_spec.ip_hdr.v6.next_hdr_prot;
            }

            printf("\nIpv6 nat enabled fw entry is:%d",
                   extd_firewall_get.extd_firewall_entry.filter_spec.ip_hdr.v6.ipv6_nat_enabled);
          }
          switch(next_hdr_prot)
          {
            case PS_IPPROTO_TCP:
              printf("\nProtocol : TCP");
              if( firewall_entry->filter_spec.next_prot_hdr.tcp.field_mask & IPFLTR_MASK_TCP_SRC_PORT )
              {
                printf("\nSrc port : %d",firewall_entry->filter_spec.next_prot_hdr.tcp.src.port);
                printf("\nSrc portrange : %d",firewall_entry->filter_spec.next_prot_hdr.tcp.src.range);
              }
              if( firewall_entry->filter_spec.next_prot_hdr.tcp.field_mask & IPFLTR_MASK_TCP_DST_PORT )
              {
                printf("\nDst port : %d",firewall_entry->filter_spec.next_prot_hdr.tcp.dst.port);
                printf("\nDst portrange : %d",firewall_entry->filter_spec.next_prot_hdr.tcp.dst.range);
              }
              break;
            case PS_IPPROTO_UDP:
              printf("\nProtocol: UDP");
              if( firewall_entry->filter_spec.next_prot_hdr.udp.field_mask & IPFLTR_MASK_UDP_SRC_PORT )
              {
                printf("\nSrc port : %d",firewall_entry->filter_spec.next_prot_hdr.udp.src.port);
                printf("\nSrc portrange : %d",firewall_entry->filter_spec.next_prot_hdr.udp.src.range);
              }
              if( firewall_entry->filter_spec.next_prot_hdr.udp.field_mask & IPFLTR_MASK_UDP_DST_PORT )
              {
                printf("\nDst port : %d",firewall_entry->filter_spec.next_prot_hdr.udp.dst.port);
                printf("\nDst portrange : %d",firewall_entry->filter_spec.next_prot_hdr.udp.dst.range);
              }
              break;
            case PS_IPPROTO_TCP_UDP:
              printf("\nProtocol: TCP_UDP");
              if( firewall_entry->filter_spec.next_prot_hdr.udp.field_mask & IPFLTR_MASK_TCP_UDP_SRC_PORT )
              {
                printf("\nSrc port : %d",firewall_entry->filter_spec.next_prot_hdr.tcp_udp_port_range.src.port);
                printf("\nSrc portrange : %d",firewall_entry->filter_spec.next_prot_hdr.tcp_udp_port_range.src.range);

              }
              if( firewall_entry->filter_spec.next_prot_hdr.udp.field_mask & IPFLTR_MASK_TCP_UDP_DST_PORT )
              {
                printf("\nDst port : %d",firewall_entry->filter_spec.next_prot_hdr.tcp_udp_port_range.dst.port);
                printf("\nDst portrange : %d",firewall_entry->filter_spec.next_prot_hdr.tcp_udp_port_range.dst.range);
              }
              break;
            case PS_IPPROTO_ICMP:
              printf("\nProtocol : ICMP");
              if( firewall_entry->filter_spec.next_prot_hdr.icmp.field_mask & IPFLTR_MASK_ICMP_MSG_TYPE )
              {
                printf("\nIcmp Type: %d ",firewall_entry->filter_spec.next_prot_hdr.icmp.type);
              }
              if( firewall_entry->filter_spec.next_prot_hdr.icmp.field_mask & IPFLTR_MASK_ICMP_MSG_CODE )
              {
                printf("\nIcmp Code: %d ",firewall_entry->filter_spec.next_prot_hdr.icmp.code);
              }
              break;
            case PS_IPPROTO_ICMP6:
              printf("\nProtocol : ICMP6");
              if( firewall_entry->filter_spec.next_prot_hdr.icmp.field_mask & IPFLTR_MASK_ICMP_MSG_TYPE )
              {
                printf("\nICMPv6 Type: %d ",firewall_entry->filter_spec.next_prot_hdr.icmp.type);
              }
              if( firewall_entry->filter_spec.next_prot_hdr.icmp.field_mask & IPFLTR_MASK_ICMP_MSG_CODE )
              {
                printf("\nICMPv6 Code: %d ",firewall_entry->filter_spec.next_prot_hdr.icmp.code);
              }
              break;
            case PS_IPPROTO_ESP:
              printf("\nProtocol : ESP");
              if( firewall_entry->filter_spec.next_prot_hdr.esp.field_mask & IPFLTR_MASK_ESP_SPI )
              {
                printf("\nESP spi : %d",firewall_entry->filter_spec.next_prot_hdr.esp.spi);

              }
              break;
            default:
              printf("\nProtocol : Any");
              break;
          }
          printf("\n### End of Firewall configuration of handle =%d ###",
               extd_firewall_get.extd_firewall_entry.firewall_handle);
        }
      }
      else
      {
        printf("\nFirewall entry get failed, Error: 0x%x", qmi_err_num);
        break;
      }
     }
   }
   else
   {
      printf("\nNo Firewall Rules to Display \n" );
   }

   return true;
}

/*===========================================================================
  FUNCTION DisplayConfig
  ===========================================================================
  @brief
    Displays the current configuration of MobileAP

  @input
    void

  @return
    void

  @dependencies
    usr to provide input

@sideefects
    None
  =========================================================================*/
void DisplayConfig()
{
  qcmap_msgr_firewall_conf_t extd_firewall_get[QCMAP_MAX_FIREWALL_ENTRY];
  qcmap_msgr_firewall_conf_t extd_firewall_handle_list;
  qcmap_msgr_firewall_entry_conf_t *firewall_entry;
  qcmap_msgr_nat_enum_v01 nat_type;
  qcmap_msgr_lan_config_v01 lan_config;
  qcmap_bootup_enable_config bootup_config;
  int   handle_list_len;
  int   index;
  int   result;
  in_addr tmpIP;
  int status;
  boolean roaming,pptp,ltp,autocon,webserver_wwan_access,ltpv6,pptpv6,ipsecv6;
  uint32 dmz_ip=0;
  boolean vpn;
  struct in6_addr dmz_ipv6;
  int p_error=0;
  int i=0, num_entries=0;
  qcmap_nw_params_t qcmap_nw_params;
  char ipv6_addr_buf[INET6_ADDRSTRLEN];
  char mac_addr_str[QCMAP_MSGR_MAC_ADDR_NUM_CHARS_V01]; /*char array of mac address*/
  qcmap_msgr_snat_entry_config_v01 snat_config[QCMAP_MSGR_MAX_SNAT_ENTRIES_V01];
  qcmap_msgr_snat_v6_entry_config_v01 snat_v6_config[QCMAP_MSGR_MAX_SNAT_ENTRIES_V01];
  char str[INET6_ADDRSTRLEN];
  struct in6_addr zero_buff;
  in_addr addr;
  in_addr start,gateway,subnet_mask;
  uint32 leasetime=0;
  qmi_error_type_v01  qmi_err_num;
  uint32_t num_dhcp_entries=0;
  qcmap_msgr_dhcp_reservation_v01 dhcp_reserv_record[QCMAP_MSGR_MAX_DHCP_RESERVATION_ENTRIES_V01];
  boolean pd_mode;
  boolean ipv6_nat_disabled = false;
  boolean ipv6_nat_enabled = false;
  qcmap_msgr_n79_config_v01 n79_config;
  eth_ports_config eth_ports;
  uint64 features = 0;
  qcmap_client_feature_mode_config feature_mode_config;
  bool ret_val;
  qcmap_backhaul_status_info_type backhaul_status_info = {0};
  qcmap_backhaul_status_info_ex_type backhaul_status_info_ex = {0};
  char buffer[MAX_BACKHAUL_TYPE_LENGTH] = {0};
  qcmap_eth_config eth_config;
  memset(&eth_ports, 0, sizeof(eth_ports_config));
  memset(snat_config, 0, QCMAP_MSGR_MAX_SNAT_ENTRIES_V01*sizeof(qcmap_msgr_snat_entry_config_v01));
  memset(&zero_buff,0,sizeof(struct in6_addr));

  //firstly check if it is eth pdu mode
  ZERO_INIT_ARG(feature_mode_config);
  ret_val = QcMapClient->GetFeatureMode(&features, &feature_mode_config, &qmi_err_num);
  if ((ret_val == true) && (features & QCMAP_MSGR_MASK_ENABLE_FEATURE_ETH_PDU_V01))
  {
      printf("\n Device in ETH PDU mode.");

      QcMapClient->GetWebserverWWANAccess(&webserver_wwan_access, &qmi_err_num);
      printf("\nWebserver WWAN Access Flag: %d", webserver_wwan_access);

      QcMapClient->GetRoaming(&roaming, &qmi_err_num);
      printf("\nRoaming Mode: %s.\n",(roaming)?"Enabled":"Disabled");

      QcMapClient->GetAutoconnect(&autocon, &qmi_err_num);
      printf("\nAuto Connect Mode: %s.\n",(autocon)?"Enabled":"Disabled");

      DisplayWWANPolicy();

      if (QcMapClient->GetBackhaulStatusEx( &backhaul_status_info_ex, &qmi_err_num))
      {
        printf("\nIPV4    %s.\n",(backhaul_status_info_ex.backhaul_v4_available)?"Connected":"Disconnected");
        printf("\nIPV6    %s.\n",(backhaul_status_info_ex.backhaul_v6_available)?"Connected":"Disconnected");
        printf("\nETH PDU %s.\n",(backhaul_status_info_ex.backhaul_eth_available)?"Connected":"Disconnected");
      }
      else
      {
        printf("\n Get backhaul status failed!");
        return;
      }

      if(backhaul_status_info_ex.backhaul_v4_available)
      {
        printf("\nIPv4 NetworkConfiguration:\n");
        if (QcMapClient->GetNetworkConfiguration(
                                                 QCMAP_MSGR_IP_FAMILY_V4_V01,
                                                 &qcmap_nw_params,
                                                 &qmi_err_num))
        {
          if(qmi_err_num == QMI_ERR_NONE_V01)
          {
            qcmap_nw_params.v4_conf.public_ip.s_addr =
                              htonl(qcmap_nw_params.v4_conf.public_ip.s_addr);
            qcmap_nw_params.v4_conf.primary_dns.s_addr =
                              htonl(qcmap_nw_params.v4_conf.primary_dns.s_addr);
            qcmap_nw_params.v4_conf.secondary_dns.s_addr =
                              htonl(qcmap_nw_params.v4_conf.secondary_dns.s_addr);

            printf("Public IP: %s \n",
                   inet_ntoa(qcmap_nw_params.v4_conf.public_ip));
            printf("Primary DNS IP address: %s \n",
                   inet_ntoa(qcmap_nw_params.v4_conf.primary_dns));
            printf("Secondary DNS IP address: %s \n",
                   inet_ntoa(qcmap_nw_params.v4_conf.secondary_dns));
          }
          else
            printf("\nError in IPv4 config - 0x%x\n\n", qmi_err_num);
        }
        else
          printf("\n GetNetworkConfiguration fails for V4\n");
      }

      if(backhaul_status_info_ex.backhaul_v6_available)
      {
        printf("\nIPv6 NetworkConfiguration:\n");
        if (QcMapClient->GetNetworkConfiguration(
                                                 QCMAP_MSGR_IP_FAMILY_V6_V01,
                                                 &qcmap_nw_params,
                                                 &qmi_err_num ))
        {
          if(qmi_err_num == QMI_ERR_NONE_V01)
          {
            /* Print this if user specified IPv6 or IPv4v6 */
            if( inet_ntop(AF_INET6,&qcmap_nw_params.v6_conf.public_ip_v6,
                ipv6_addr_buf,sizeof(ipv6_addr_buf)))
            {
              printf("Public IP: %s \n",ipv6_addr_buf);
            }
            if( inet_ntop(AF_INET6,&qcmap_nw_params.v6_conf.primary_dns_v6,
                ipv6_addr_buf,sizeof(ipv6_addr_buf)))
            {
              printf("Primary DNS IP address: %s \n",ipv6_addr_buf);
            }
            if( inet_ntop(AF_INET6,&qcmap_nw_params.v6_conf.secondary_dns_v6,
                ipv6_addr_buf,sizeof(ipv6_addr_buf)))
            {
              printf("Secondary DNS IP address: %s \n\n",ipv6_addr_buf);
            }
          }
          else
            printf("\nError in IPv6 config - 0x%x\n\n", qmi_err_num);
        }
        else
          printf("\n GetNetworkConfiguration fails for V6\n");
      }

      if(backhaul_status_info_ex.backhaul_eth_available)
      {
        printf("\nETH PDU NetworkConfiguration:\n");
        if (QcMapClient->GetNetworkConfiguration(QCMAP_MSGR_IP_FAMILY_ETH_V01,&qcmap_nw_params,&qmi_err_num ))
        {
          if(qmi_err_num == QMI_ERR_NONE_V01)
          {
            printf("Vlan start:%d,vlan end:%d:\n\n",qcmap_nw_params.eth_conf.vlan_start,qcmap_nw_params.eth_conf.vlan_end);
          }
          else
          {
            printf("\nError in Eth Pdu config - 0x%x\n\n", qmi_err_num);
          }
        }
        else
        {
          printf("\n GetNetworkConfiguration fails for Eth Pdu!\n");
        }
      }
  }

  QcMapClient->GetStaticNatConfig(snat_config, &num_entries, &qmi_err_num);
  QcMapClient->GetRoaming(&roaming, &qmi_err_num);
  QcMapClient->GetDMZ(&dmz_ip, &qmi_err_num);
  QcMapClient->GetIPSECVpnPassthrough(&vpn,&qmi_err_num);
  QcMapClient->GetPPTPVpnPassthrough(&pptp,&qmi_err_num);
  QcMapClient->GetL2TPIPSECVpnPassthrough(&ltp,&qmi_err_num);
  QcMapClient->GetAutoconnect(&autocon, &qmi_err_num);
  QcMapClient->GetWebserverWWANAccess(&webserver_wwan_access, &qmi_err_num);

 // QcMapClient->GetUSBConfig(&gateway.s_addr,&start.s_addr,&subnet_mask.s_addr,&leasetime);
  memset(&extd_firewall_handle_list, 0, sizeof(qcmap_msgr_firewall_conf_t));
  handle_list_len=0;
  extd_firewall_handle_list.extd_firewall_handle_list.ip_family = IP_V4;
  if(QcMapClient->GetFireWallHandlesList(&extd_firewall_handle_list.extd_firewall_handle_list,&qmi_err_num))
  {
    handle_list_len = extd_firewall_handle_list.extd_firewall_handle_list.num_of_entries;
  }
  if(handle_list_len > 0)
  {
    for(index =0; index < handle_list_len; index++)
    {
      memset(&extd_firewall_get[index], 0, sizeof(qcmap_msgr_firewall_conf_t));
      extd_firewall_get[index].extd_firewall_entry.filter_spec.ip_vsn =
        extd_firewall_handle_list.extd_firewall_handle_list.ip_family;
      extd_firewall_get[index].extd_firewall_entry.firewall_handle =
        extd_firewall_handle_list.extd_firewall_handle_list.handle_list[index];
      QcMapClient->GetFireWallEntry(&extd_firewall_get[index].extd_firewall_entry, &qmi_err_num);
    }
  }

  memset(&qcmap_nw_params,0,sizeof(qcmap_nw_params_t));
  QcMapClient->GetNatType(&nat_type, &qmi_err_num);
  uint32 timeout_value[5];
  uint32 timeout_type;
  p_error=0;
  timeout_type = QCMAP_MSGR_NAT_TIMEOUT_GENERIC_V01;
  while(timeout_type <= QCMAP_MSGR_NAT_TIMEOUT_UDP_V01)
  {
    QcMapClient->GetNatTimeout(timeout_type,(uint32 *)&timeout_value[timeout_type], &qmi_err_num);
    timeout_type++;
  }

  memset(&lan_config,0,sizeof(qcmap_msgr_lan_config_v01));
  QcMapClient->GetLANConfig(&lan_config,&qmi_err_num);

  printf("\nSNAT configuration:\n");

  if (QcMapClient->GetStaticNatConfig(snat_config, &num_entries, &qmi_err_num))
  {
    if(num_entries > 0)
    {
      for (i=0; i<num_entries; i++)
      {
        printf("\n\nEntry %d:",i);
        tmpIP.s_addr = ntohl(snat_config[i].private_ip_addr);
        printf("\nprivate ip: %s", inet_ntoa(tmpIP));
        printf("\nprotocol: %d", snat_config[i].protocol);
        if(!IS_NON_PORT_BASED_PROTO(snat_config[i].protocol))
        {
          printf("\nprivate port: %d", snat_config[i].private_port);
          printf("\nglobal port: %d", snat_config[i].global_port);
        }
      }
    }
    else
    {
      printf("\nNo SNAT Entries Configured");
    }
  }
  else
  {
    printf("\nSNAT Entries get failed , Error: 0x%x", qmi_err_num);
  }

  printf("\nDMZ Configuration:\n");

  if (QcMapClient->GetDMZ(&dmz_ip, &qmi_err_num))
  {
    if (dmz_ip == 0)
    printf("\nNo DMZ Configured!");
    else
    {
      tmpIP.s_addr = ntohl(dmz_ip);
      printf("\ndmz ip %s",inet_ntoa(tmpIP));
    }
  }
  else
    printf("\nDMZ get fails. Error: 0x%x", qmi_err_num);

  printf("\nIPSEC Passthrough Enable Flag : %d",vpn);
  printf("\nPPTP Passthrough Enable Flag : %d",pptp);
  printf("\nL2TP Passthrough Enable Flag : %d", ltp);

  if (QcMapClient->GetIPv6NAT(&ipv6_nat_enabled,&qmi_err_num) && (ipv6_nat_enabled))
  {
    QcMapClient->GetL2TPIPSECVpnPassthrough_Ipv6(&ltpv6,&qmi_err_num);
    QcMapClient->GetPPTPVpnPassthrough_Ipv6(&pptpv6,&qmi_err_num);
    QcMapClient->GetIPSECVpnPassthrough_Ipv6(&ipsecv6,&qmi_err_num);
    QcMapClient->GetStaticNatConfig_Ipv6(snat_v6_config, &num_entries,
                                         &qmi_err_num);
    QcMapClient->GetDMZ_Ipv6(&dmz_ipv6,&qmi_err_num);
    printf("\nSNAT v6 configuration:\n");
    for (i=0; i<num_entries; i++)
    {
      printf("Entry %d:\n",i);
      inet_ntop(AF_INET6,(void*)&snat_v6_config[i].port_fwding_private_ip6_addr,
                   str, sizeof(str));
      printf("Private IP: %s\n",str);
      printf("private port: %d\n", snat_v6_config[i].private_port);
      printf("global port: %d\n", snat_v6_config[i].global_port);
      printf("protocol: %d\n", snat_v6_config[i].protocol);
    }

    printf("\nDMZ IPv6:");
    if (memcmp(&dmz_ipv6,&zero_buff,sizeof(struct in6_addr)))
    {
       inet_ntop(AF_INET6,(void*)&dmz_ipv6,
                  str, sizeof(str));
       printf("DMZ IP: %s\n",str);
    }
    printf("\nIPSEC v6 Passthrough Enable Flag : %d",ipsecv6);
    printf("\nPPTP v6 Passthrough Enable Flag : %d",pptpv6);
    printf("\nL2TP v6 Passthrough Enable Flag : %d", ltpv6);
  }

  printf("\nWebserver WWAN Access Flag: %d", webserver_wwan_access);
  printf("\nAuto Connect Mode: %s.\n",(autocon)?"Enabled":"Disabled");

  printf("\nFirewall Entries for Ipv4:");
  DisplayFirewall( handle_list_len, extd_firewall_handle_list );
  memset(&extd_firewall_handle_list, 0, sizeof(qcmap_msgr_firewall_conf_t));
  handle_list_len=0;
  extd_firewall_handle_list.extd_firewall_handle_list.ip_family = IP_V6;
  if(QcMapClient->GetFireWallHandlesList(&extd_firewall_handle_list.extd_firewall_handle_list,&qmi_err_num))
  {
    handle_list_len = extd_firewall_handle_list.extd_firewall_handle_list.num_of_entries;
  }
  if(handle_list_len > 0)
  {
    for(index =0; index < handle_list_len; index++)
    {
      memset(&extd_firewall_get[index], 0, sizeof(qcmap_msgr_firewall_conf_t));
      extd_firewall_get[index].extd_firewall_entry.filter_spec.ip_vsn =
        extd_firewall_handle_list.extd_firewall_handle_list.ip_family;
      extd_firewall_get[index].extd_firewall_entry.firewall_handle =
        extd_firewall_handle_list.extd_firewall_handle_list.handle_list[index];
      QcMapClient->GetFireWallEntry(&extd_firewall_get[index].extd_firewall_entry, &qmi_err_num);
    }
  }
  printf("\nFirewall Entries for Ipv6:");
  DisplayFirewall( handle_list_len, extd_firewall_handle_list );

  DisplayWWANPolicy();
  DisplayIPv4State();
  DisplayIPv6State();

  printf("\nCurrent Backhaul:");
  if (QcMapClient->GetBackhaulStatus( &backhaul_status_info, &qmi_err_num))
  {
    if ((backhaul_status_info.backhaul_v4_available == false) &&
        (backhaul_status_info.backhaul_v6_available == false))
    {
      printf("No Backhaul \n");
    }
    else
    {
      if (convert_backhaul_enum_to_string(backhaul_status_info.backhaul_type, buffer))
      {
        printf("%s Backhual\n", buffer);
        printf("IPV4 %s.\n",(backhaul_status_info.backhaul_v4_available)?"Connected":"Disconnected");
        printf("IPV6 %s.\n",(backhaul_status_info.backhaul_v6_available)?"Connected":"Disconnected");
      }
      else
      {
        printf("Invaild backhaul type : %d",backhaul_status_info.backhaul_type);
      }
    }
  }

  printf("\nIPv4 NetworkConfiguration:\n");
  if (QcMapClient->GetNetworkConfiguration(
                                           QCMAP_MSGR_IP_FAMILY_V4_V01,
                                           &qcmap_nw_params,
                                           &qmi_err_num))
  {
    if(qmi_err_num == QMI_ERR_NONE_V01)
    {
      qcmap_nw_params.v4_conf.public_ip.s_addr =
                        htonl(qcmap_nw_params.v4_conf.public_ip.s_addr);
      qcmap_nw_params.v4_conf.primary_dns.s_addr =
                        htonl(qcmap_nw_params.v4_conf.primary_dns.s_addr);
      qcmap_nw_params.v4_conf.secondary_dns.s_addr =
                        htonl(qcmap_nw_params.v4_conf.secondary_dns.s_addr);

      printf("Public IP: %s \n",
             inet_ntoa(qcmap_nw_params.v4_conf.public_ip));
      printf("Primary DNS IP address: %s \n",
             inet_ntoa(qcmap_nw_params.v4_conf.primary_dns));
      printf("Secondary DNS IP address: %s \n",
             inet_ntoa(qcmap_nw_params.v4_conf.secondary_dns));
    }
    else
      printf("\nError in IPv4 config - 0x%x\n\n", qmi_err_num);
  }
  else
    printf("\n GetNetworkConfiguration fails for V4\n");

  printf("\nIPv6 NetworkConfiguration:\n");
  if (QcMapClient->GetNetworkConfiguration(
                                           QCMAP_MSGR_IP_FAMILY_V6_V01,
                                           &qcmap_nw_params,
                                           &qmi_err_num ))
  {
    if(qmi_err_num == QMI_ERR_NONE_V01)
    {
      /* Print this if user specified IPv6 or IPv4v6 */
      if( inet_ntop(AF_INET6,&qcmap_nw_params.v6_conf.public_ip_v6,
          ipv6_addr_buf,sizeof(ipv6_addr_buf)))
      {
        printf("Public IP: %s \n",ipv6_addr_buf);
      }
      if( inet_ntop(AF_INET6,&qcmap_nw_params.v6_conf.primary_dns_v6,
          ipv6_addr_buf,sizeof(ipv6_addr_buf)))
      {
        printf("Primary DNS IP address: %s \n",ipv6_addr_buf);
      }
      if( inet_ntop(AF_INET6,&qcmap_nw_params.v6_conf.secondary_dns_v6,
          ipv6_addr_buf,sizeof(ipv6_addr_buf)))
      {
        printf("Secondary DNS IP address: %s \n\n",ipv6_addr_buf);
      }
    }
    else
      printf("\nError in IPv6 config - 0x%x\n\n", qmi_err_num);
  }
  else
    printf("\n GetNetworkConfiguration fails for V6\n");

  printf("Nat type configured: \n");
  if(nat_type == QCMAP_MSGR_NAT_SYMMETRIC_NAT_V01)
  {
    printf("Symmetric NAT");
  }
  else if(nat_type == QCMAP_MSGR_NAT_PORT_RESTRICTED_CONE_NAT_V01)
  {
    printf("Port Restricted Cone NAT");
  }
  else if (nat_type == QCMAP_MSGR_NAT_FULL_CONE_NAT_V01)
  {
    printf("Full Cone NAT");
  }
  else if(nat_type == QCMAP_MSGR_NAT_ADDRESS_RESTRICTED_NAT_V01)
  {
    printf("Address Restricted Cone NAT");
  }
  else
  {
    printf("Default NAT configured");
  }
  printf("\nNAT Timeout Values Configured:\n");
  timeout_type = QCMAP_MSGR_NAT_TIMEOUT_GENERIC_V01;
  while(timeout_type <= QCMAP_MSGR_NAT_TIMEOUT_UDP_V01)
  {
    if(timeout_type == QCMAP_MSGR_NAT_TIMEOUT_GENERIC_V01)
    {
      printf("GENRIC NAT Timeout: %d\n", timeout_value[timeout_type]);
    }
    else if(timeout_type == QCMAP_MSGR_NAT_TIMEOUT_ICMP_V01)
    {
      printf("ICMP NAT Timeout: %d\n", timeout_value[timeout_type]);
    }
    else if(timeout_type == QCMAP_MSGR_NAT_TIMEOUT_TCP_ESTABLISHED_V01)
    {
      printf("TCP ESTABLISHED NAT Timeout: %d\n", timeout_value[timeout_type]);
    }
    else if(timeout_type == QCMAP_MSGR_NAT_TIMEOUT_UDP_V01)
    {
      printf("UDP NAT Timeout: %d\n", timeout_value[timeout_type]);
    }
    timeout_type++;
  }

  // WLAN config
     qcmap_wlan_ex_config wlan_config;
     ZERO_INIT_ARG(wlan_config);
     printf("\nWlan Configuration :\n");
     if (QcMapClient->GetWLANConfigEx(&wlan_config, &qmi_err_num))
     {
       if (1 <= wlan_config.wlan_mode && QCMAP_MSGR_WLAN_MODE_7AP_V01 >= wlan_config.wlan_mode )
       {
         printf("\n WLAN Mode is %s.\n", wlan_mode_str[wlan_config.wlan_mode]);
       }
       else
       {
         printf("\n Unsupported WLAN Mode:- %d.\n", wlan_config.wlan_mode);
       }
     }
     else{
      printf("\n Get WLAN Config failed.\n");
    }

    if (wlan_config.wlan_mode != QCMAP_MSGR_WLAN_MODE_7AP_V01)
    {
      for (int i = 0; i < wlan_config.ap_config_len; i++)
      {
        printf("\n Guest AP %d is setup to be in '%s' mode of band\n",
              i + 1 , (wlan_config.ap_config[i].guest_ap_profile == QCMAP_MSGR_PROFILE_FULL_ACCESS_V01)?
              "Full Access" : "Internet Only Access");
      }
    }
    else
    {
      for (int i = 0; i < wlan_config.ap_config_len; i++)
      {
        printf("\n Guest AP %d is setup to be in '%s' mode of band %dGHz\n",
              i + 1 , (wlan_config.ap_config[i].guest_ap_profile == QCMAP_MSGR_PROFILE_FULL_ACCESS_V01)?
              "Full Access" : "Internet Only Access", wlan_config.ap_config[i].band);
      }
    }


  /* Print LAN Configuration. */
  printf("\n LAN Configuration:\n");
  addr.s_addr = htonl(lan_config.gw_ip);
  printf("\nGateway IP: %s\n", inet_ntoa(addr));
  addr.s_addr = htonl(lan_config.netmask);
  printf("\nNetmask : %s\n", inet_ntoa(addr));
  printf("\nDHCP Enabled: %d\n", lan_config.enable_dhcp);
  if ( lan_config.enable_dhcp == TRUE )
  {
    addr.s_addr = htonl(lan_config.dhcp_config.dhcp_start_ip);
    printf("\nDHCP Start IP: %s\n", inet_ntoa(addr));
    addr.s_addr = htonl(lan_config.dhcp_config.dhcp_end_ip);
    printf("\nDHCP End IP : %s\n", inet_ntoa(addr));
    printf("\nDHCP Lease Time (seconds) : %d\n", lan_config.dhcp_config.lease_time);
  }

   /* Print Station Configuration. */
   printf("\nStation Configuration.\n");
   if ( wlan_config.station_config.conn_type == QCMAP_MSGR_STA_CONNECTION_DYNAMIC_V01)
   printf("\n Connection Type : DYNAMIC (DHCP) \n");
   else
   {
   printf("\n Connection Type : STATIC \n");
   }
   printf("\n STATIC STA IP Configuration \n");
   addr.s_addr = htonl(wlan_config.station_config.static_ip_config.ip_addr);
   printf("\n IP Address: %s \n", inet_ntoa(addr));
   addr.s_addr = htonl(wlan_config.station_config.static_ip_config.gw_ip);
   printf("\n Gateway IP : %s \n", inet_ntoa(addr));
   addr.s_addr = htonl(wlan_config.station_config.static_ip_config.netmask);
   printf("\n Netmask: %s \n", inet_ntoa(addr));
   addr.s_addr = htonl(wlan_config.station_config.static_ip_config.dns_addr);
   printf("\n DNS Address : %s \n", inet_ntoa(addr));
   printf("\n STA is configured in %s \n",(wlan_config.station_config.ap_sta_bridge_mode?"Bridge Mode":"Router Mode"));


   if (QcMapClient->GetQCMAPBootupCfgEx(&bootup_config, &qmi_err_num))
  {
     printf("\n Mobile AP will be %s on bootup \n",((bootup_config.mobileap_enable == QCMAP_MSGR_ENABLE_ON_BOOT_V01) ?"ENABLED":"DISABLED"));
     printf("\n WLAN will be %s on bootup \n",((bootup_config.wlan_enable == QCMAP_MSGR_ENABLE_ON_BOOT_V01)?"ENABLED":"DISABLED"));
     printf("\n Calibration will be %s on bootup \n",((bootup_config.calibration_enable == QCMAP_MSGR_ENABLE_ON_BOOT_V01)?"ENABLED":"DISABLED"));
  }
   else
  {
     printf("\n  Get QCMAP Bootup Cfg Fails , Error: 0x%x \n", qmi_err_num);
  }

  memset(dhcp_reserv_record,0,QCMAP_MSGR_MAX_DHCP_RESERVATION_ENTRIES_V01*sizeof(qcmap_msgr_dhcp_reservation_v01));
  if( QcMapClient->GetDHCPReservRecords(dhcp_reserv_record, &num_entries,&qmi_err_num) )
  {
       //display each DHCP reservation records
       if ( !num_entries)
       {
           printf("\nNo DHCP Reservation Records");
       }
       else
       {
           for ( i = 0;i < num_entries; i++)
           {
             printf("\nEntry  %d:",i);
             ds_mac_addr_ntop(dhcp_reserv_record[i].client_mac_addr,mac_addr_str);
             if ( strncmp(mac_addr_str,MAC_NULL_STRING,QCMAP_MSGR_MAC_ADDR_NUM_CHARS_V01) != 0 )
             {
                 printf("\nMAC address of the client[%i]: %s",i,mac_addr_str);
             }
             tmpIP.s_addr =ntohl(dhcp_reserv_record[i].client_reserved_ip);
             printf("IP address of the client[%i]: %s",i,inet_ntoa(tmpIP));
             printf("Device Name of the client[%i]: %s",i,dhcp_reserv_record[i].client_device_name);
             printf("DHCP Reservation enabled for the client[%i]: %d",i,dhcp_reserv_record[i].enable_reservation);
           }
       }
   }
   else
   {
        printf("\nFailed to Dsiplay DHCP Reservation record. Error 0x%x.\n ", qmi_err_num);
   }

   /* Display cradle mode */
   qcmap_msgr_cradle_mode_v01 mode;
   if (QcMapClient->GetCradleMode(&mode, &qmi_err_num))
   {
     /* Only  QCMAP_MSGR_CRADLE_WAN_ROUTER_V01 is supported*/
     switch (mode)
     {
        case QCMAP_MSGR_CRADLE_DISABLED_V01:
          printf("\nMobile AP Cradle Mode is Disabled");
          break;
        case QCMAP_MSGR_CRADLE_LAN_BRIDGE_V01:
          printf("\nMobile AP Cradle Mode is LAN BRIDGE");
          break;
        case QCMAP_MSGR_CRADLE_LAN_ROUTER_V01:
          printf("\nMobile AP Cradle Mode is LAN ROUTER");
          break;
        case QCMAP_MSGR_CRADLE_WAN_BRIDGE_V01:
          printf("\nMobile AP Cradle Mode is WAN BRIDGE");
          break;
        case QCMAP_MSGR_CRADLE_WAN_ROUTER_V01:
          printf("\nMobile AP Cradle Mode is WAN ROUTER");
          break;
        default:
          printf("\nIncorrect state returned: 0x%x", mode);
          break;
     }
   }
   else
     printf("  Failed to Get Cradle Mode .Error 0x%x.\n ", qmi_err_num);

   /* Display Ethernet NIC config */

   ZERO_INIT_ARG(eth_config);
   if (QcMapClient->GetEthernetNicConfig(&eth_config, &qmi_err_num))
   {
     if (eth_config.is_eth_nics_config_valid)
     {
       DisplayEthernetNicConfig(eth_config);
     }
     switch (eth_config.mode)
     {
       case QCMAP_MSGR_ETHERNET_LAN_ROUTER_V01:
         printf("\nMobile AP Ethernet Mode is LAN ROUTER");
         break;

       case QCMAP_MSGR_ETHERNET_WAN_ROUTER_V01:
         printf("\nMobile AP Ethernet Mode is WAN ROUTER");
         break;

       case QCMAP_MSGR_ETHERNET_WAN_LAN_ROUTER_V01:
         printf("\nMobile AP Ethernet Mode is WAN_LAN ROUTER");
         if (eth_config.is_eth_ports_config_valid)
         {
           printf("\nLAN Ports");
           printf("\nPort\tVlan ID");
           printf("\n%d\t%d", eth_config.eth_ports.eth_lan_ports[0], eth_config.eth_ports.eth_lan_vlan_id);
           printf("\n%d\t%d", eth_config.eth_ports.eth_lan_ports[1], eth_config.eth_ports.eth_lan_vlan_id);
           printf("\n%d\t%d", eth_config.eth_ports.eth_lan_ports[2], eth_config.eth_ports.eth_lan_vlan_id);
           printf("\nWAN Ports\nPort\tVlan ID");
           printf("\n%d\t%d\n", eth_config.eth_ports.eth_wan_port, eth_config.eth_ports.eth_wan_vlan_id);
         }
         break;

       default:
         printf("\nIncorrect state returned: 0x%x", eth_config.mode);
         break;
     }
   }
   else
     printf("Failed to Get Ethernet Mode .Error 0x%x.\n ", qmi_err_num);

  if (QcMapClient->GetPrefixDelegationConfig(&pd_mode, &qmi_err_num))
  {
    if (pd_mode)
      printf("\nPrefix Delegation configuration is enabled.\n");
    else
      printf("\nPrefix Delegation configuration is disabled.\n");
  }
  else
    printf("\n  Failed to Get Prefix Delegation config. Error 0x%x.\n ", qmi_err_num);

  /* Display IPPT and EoGRE feature mode */
  ZERO_INIT_ARG(feature_mode_config);
  ret_val = QcMapClient->GetFeatureMode(&features, &feature_mode_config, &qmi_err_num);

  if (!ret_val)
  {
    printf("\n\nFailure in getting feature modes.");
  }
  if (features == 0)
  {
    printf("\nNo features / modes are enabled\n");
  }
  else
  {
    if (features & QCMAP_MSGR_MASK_ENABLE_FEATURE_IP_PASSTHROUGH_V01)
    {
      /* IP Passthrough feature */
      if (feature_mode_config.ip_passthrough_feature_valid)
      {
        if (feature_mode_config.ip_passthrough_feature_mode ==
            QCMAP_MSGR_IP_PASSTHROUGH_MODE_WITH_NAT_V01)
        {
          printf("\nIP Passthrough Feature is enabled : With NAT\n");
        }
        else
        {
          printf("\nIP Passthrough Feature is enabled : Without NAT\n");
        }
      }
    }

    if (features & QCMAP_MSGR_MASK_ENABLE_FEATURE_DHCP_LAN_OPTIONS_V01)
    {
      /* DHCP LAN Options feature */
      if (feature_mode_config.dhcp_lan_options_feature_valid)
      {
        if (feature_mode_config.dhcp_lan_options_feature_modes &
            QCMAP_MSGR_MASK_DHCP_VENDOR_INFORMATION_V01)
        {
          printf("\nDHCP LAN Options is enabled : DHCP Vendor Information (Options 43/60)\n");
        }
        if (feature_mode_config.dhcp_lan_options_feature_modes &
            QCMAP_MSGR_MASK_DHCP_TIMEZONE_INFORMATION_V01)
        {
          printf("\nDHCP LAN Options is enabled : DHCP Timezone Information (Options 100/101)\n");
        }
      }
    }
    if (features & QCMAP_MSGR_MASK_ENABLE_FEATURE_ETH_OVER_GRE_V01)
    {
      printf("\nEoGRE Feature is enabled\n");
    }
    else
    {
      printf("\nEoGRE Feature is not enabled\n");
    }
  }
}

boolean DisplayEoGREDSCPMarkingList
(
  qcmap_eogre_vlan_pcp_to_dscp_mapping  *vlan_to_dscp_marking_list,
  int8_t                                *len
)
{
  int16_t vlan_id;
  int8_t pcp;
  int8_t dscp;
  int i;

  if (*len == 0)
  {
    printf("\n### No entries found. ###");
    return true;
  }

  printf("\n### Start Displaying DSCP marking table ###");
  printf("\n### VLAN_ID\tPriortiy\tDSCP\n###");

  for (i = 0; i < *len; i++)
  {
    vlan_id = vlan_to_dscp_marking_list->vlan_id;
    pcp = vlan_to_dscp_marking_list->pcp;
    dscp = vlan_to_dscp_marking_list->dscp;

    printf("\n### %d\t\t %d\t\t %d\n###", vlan_id, pcp, dscp);

    vlan_to_dscp_marking_list++;
  }
  return true;
}


/* CLI Restructuring functions
------------------------------*/

void mobileApConfig(int mobileApOpt)
{
  qmi_error_type_v01 qmi_err_num;
  /* Initialize QMI Error Number. */
  qmi_err_num = QMI_ERR_NONE_V01;
  char scan_string[QCMAP_MSGR_MAX_FILE_PATH_LEN];
  qcmap_msgr_n79_config_v01 n79_config;

  /* MobileAp Configuration options */
  switch(mobileApOpt)
  {
    /* Display the current configuration of the MobileAp. */
    case 1:
      DisplayConfig();
      break;

    /* Enable/Disable MobileAp */
    case 2:
    {
      QCMAP_PRINTF_TAKE_INPUT("   Please input MobileAP State(1-Enable/0-Disable) : ");
      fgets(scan_string, sizeof(scan_string), stdin);
      if (!VALID_NUMERIC_INPUT(scan_string) || (atoi(scan_string) != 1 && atoi(scan_string) != 0))
      {
        printf("\nInvalid response. Please enter 1 or 0.\n");
        break;
      }
      if (atoi(scan_string))
      {
        if (QcMapClient->EnableMobileAP_Ext(&qmi_err_num, indication_mask))
        {
          printf("\nMobileAP Enable succeeds.");
        }
        else
        {
          printf("\nMobileAP Enable fails, Error: 0x%x", qmi_err_num);
        }
      }
      else
      {
        if (QcMapClient->DisableMobileAP(&qmi_err_num))
          printf("\nMobileAP Disable in progress.");
        else
          printf("\nMobileAP Disable request fails, Error: 0x%x", qmi_err_num);
      }
      break;
    }

    /* Get Status of MobielAp */
    case 3:
      {
        qcmap_msgr_mobile_ap_status_enum_v01 status;
        if(QcMapClient->GetMobileAPStatus(&status, &qmi_err_num))
        {
         if(status ==QCMAP_MSGR_MOBILE_AP_STATUS_CONNECTED_V01)
         {
           printf("\nMobile AP is Connected");
         }
          else if(status == QCMAP_MSGR_MOBILE_AP_STATUS_DISCONNECTED_V01)
           printf("\nMobile AP is Disconnected");
        }
        break;
      }

    /* Display's Connected Device's information */
    case 4:
      {
        int getCDIOpt = 0, array_size = 0;
        char scan_string[QCMAP_MSGR_MAX_FILE_PATH_LEN];
        array_size = sizeof(get_connected_devices_info_config_list)/sizeof(get_connected_devices_info_config_list[0]);
        for (int i = 0; i < array_size; i++)
        {
          printf("%s\n",get_connected_devices_info_config_list[i]);
        }
        fgets(scan_string, sizeof(scan_string), stdin);

        if(atoi(scan_string) == 1 || atoi(scan_string) == 2)
        {
          getCDIOpt = atoi(scan_string);
          getConnectedDevicesInfoConfig(getCDIOpt);
        }
        break;
      }

    /* Enable/Disable/Reset Packet Stats */
    case 5:
    {
      int tmp = 0;
      int validate_arr[] = { 1,0,5 };
      char validate_string[50] = "1-Enable,0-Disable,5-Reset";
      QCMAP_PRINTF_TAKE_INPUT("   Please input Packet Stats State(1-Enable/0-Disable/5-Reset) : ");
      READ_AND_VALIDATE_ARRAY_VALUE(tmp, validate_arr, validate_string);
      if (tmp == PACKET_STATS_ENABLE)
      {
        if (QcMapClient->EnablePacketStats(&qmi_err_num))
          printf("\nPacket Stats Enable succeeds.");
        else
          printf("\nPacket Stats Enable fails, Error: 0x%x", qmi_err_num);
      }
      else if (tmp == PACKET_STATS_DISABLE)
      {
        if (QcMapClient->DisablePacketStats(&qmi_err_num))
          printf("\nPacket Stats Disabled.");
        else
          printf("\nPacket Stats Disable request fails, Error: 0x%x", qmi_err_num);
      }
      else if (tmp == PACKET_STATS_RESET)
      {
        if (QcMapClient->ResetPacketStats(&qmi_err_num))
          printf("\nPacket Stats succeeded");
        else
          printf("\nPacket Stats reset request fails, Error: 0x%x", qmi_err_num);
      }
      break;
    }
     /* Get Packet Stats Status */
    case 6:
      {
        qcmap_msgr_packet_stats_status_enum_v01 status;
        if(QcMapClient->GetPacketStatsStatus(&status, &qmi_err_num))
        {
          if (status == QCMAP_MSGR_PACKET_STATS_STATUS_ENABLED_V01)
         {
           printf("\nPacket Stats is Enabled");
         }
          else if(status == QCMAP_MSGR_PACKET_STATS_STATUS_DISABLED_V01)
           printf("\nPacket Stats is Disabled");
        }
        break;
      }
     /* Will reset the device to  default factory configuration and reboot */
    case 7:
      {
        QCMAP_PRINTF_TAKE_INPUT("\n Caution this will Restore Factory configuration and");
        QCMAP_PRINTF_TAKE_INPUT("\n reboot the device (1-continue/0-exit this option):");
        fgets(scan_string, sizeof(scan_string), stdin);
        if ( atoi( scan_string ) == 1 )
        {
            if ( QcMapClient->RestoreFactoryConfig( &qmi_err_num ) )
            {
              QCMAP_PRINTF_TAKE_INPUT("\nRestore to Factory config success");
              QCMAP_PRINTF_TAKE_INPUT("\nSystem will reboot in 5 sec");
           }
          else
          {
            QCMAP_PRINTF_TAKE_INPUT("\nRestore Factory config returns Error.");
          }
        }
        break;
      }

     /* Disconnect BackHaul, disable LAN and exit application. */
    case 8:
      {
        sighandler(SIGTERM);
        exit(1);
      }
      break;

    /*Set Data Path Optimization Flag*/
    case 9:
    {
      boolean data_path_opt_status;
      int input;

      QCMAP_PRINTF_TAKE_INPUT("\n Please input 1-[enable]/0-[disable]:");
      fgets(scan_string, sizeof(scan_string), stdin);
      if (!VALID_NUMERIC_INPUT(scan_string))
      {
        printf("\nInvalid response\n");
        break;
      }
      input = atoi(scan_string);
      printf("\n Response: %d",input);
      if (input > 1 || input < 0)
      {
        printf("\nInvalid response\n");
        break;
      }
      else
      {
        data_path_opt_status = input;
        if(QcMapClient->SetDataPathOptStatus(data_path_opt_status , &qmi_err_num))
        {
           printf("\nData path opt status set successfully\n");
        }
        else
        {
           printf("\n Set data path opt  status fails, Error: 0x%x \n", qmi_err_num);
        }
      }
      break;
    }

    /* Get Data Path Optimization Flag */
    case 10:
    {
      boolean data_path_opt_status = 0;
      if (QcMapClient->GetDataPathOptStatus(&data_path_opt_status , &qmi_err_num))
      {
        if(data_path_opt_status)
          printf("\nData optimization handler enabled\n");
        else
          printf("\nData optimization handler not enabled\n");
      }
      else
      {
        printf("\n Get data path opt  status fails, Error: 0x%x \n", qmi_err_num);
      }
      break;
    }

    /*Set Device Mode */
    case 11:
    {
      qcmap_msgr_device_mode_enum_v01 input = QCMAP_MSGR_DEVICE_NONE_V01;
      QCMAP_PRINTF_TAKE_INPUT("\n 0-[NO_HW_OFFLOAD] \n 1-[Dev_L2L] \n 2-[Dev_E2E]:");
      fgets(scan_string, sizeof(scan_string), stdin);
      if (!VALID_NUMERIC_INPUT(scan_string))
      {
        printf("\nInvalid response\n");
        break;
      }
      input = (qcmap_msgr_device_mode_enum_v01)atoi(scan_string);
      printf("\n Response: %d",input);
      if (input < QCMAP_MSGR_DEVICE_NONE_V01 || input > QCMAP_MSGR_DEVICE_E2E_V01)
      {
        printf("\nInvalid response\n");
        break;
      }
      else
      {
        if(QcMapClient->SetDeviceMode(input, &qmi_err_num))
        {
           printf("\nDevice mode set successfully\n");
        }
        else
        {
           printf("\n Set Device mode fails, Error: 0x%x \n", qmi_err_num);
        }
      }
      break;
    }

    /* Get Device Mode */
    case 12:
    {
      qcmap_msgr_device_mode_enum_v01 device_mode = QCMAP_MSGR_DEVICE_NONE_V01;
      if (QcMapClient->GetDeviceMode(&device_mode , &qmi_err_num))
      {
        if(device_mode == QCMAP_MSGR_DEVICE_NONE_V01)
          printf("\n No device mode selected\n");
        else if(device_mode == QCMAP_MSGR_DEVICE_L2L_V01)
          printf("\n Device mode L2L selected\n");
        else if(device_mode == QCMAP_MSGR_DEVICE_E2E_V01)
          printf("\n Device mode E2E selected\n");
        else
          printf("\nIncorrect state returned: 0x%x\n", device_mode);
      }
      else
      {
        printf("\n Get Device mode  status fails, Error: 0x%x \n", qmi_err_num);
      }
      break;
    }

    /* Set Client Configuration */
    case 13:
    {
      qcmap_client_config client_config;
      QCMAP_PRINTF_TAKE_INPUT("   Change Client Configuration (0=> Local, 1==> Remote: ");
      fgets(scan_string, sizeof(scan_string), stdin);
      if (!VALID_NUMERIC_INPUT(scan_string))
      {
        printf("\nInvalid response\n");
        break;
      }
      client_config = (qcmap_client_config)atoi(scan_string);
      if (QcMapClient->SetClientConfig(client_config, &qmi_err_num))
      {
        printf("     Client Switched to %d successfully.\n", client_config);
      }
      else
      {
        printf("     Client Switched to %d failed, error=%d\n", client_config, qmi_err_num);
      }
    }
    break;

    /* Set n79 configuration */
    case 14:
    {
      bool configFlagSet = false;
      int temp_val = 0;
      memset(&n79_config, 0, sizeof(qcmap_msgr_n79_config_v01));
      n79_config.priority = DEFAULT_5G_PRIORITY;
      n79_config.retries = DEFAULT_QMI_RETRIES;
      n79_config.n79_hysteresis_timer = DEFAULT_N79_HYSTERESIS_TIMER;
      n79_config.wlan_hysteresis_timer = DEFAULT_WLAN_HYSTERESIS_TIMER;
      n79_config.delay_between_retries = DEFAULT_ATTEMPTS_TIMER;

      memset(scan_string, 0, QCMAP_MSGR_MAX_FILE_PATH_LEN);
      QCMAP_PRINTF_TAKE_INPUT("Do you want to enable or disable N79 1/0\n");
      fgets(scan_string, sizeof(scan_string), stdin);
      if (!VALID_NUMERIC_INPUT(scan_string))
      {
        printf("\nInvalid response\n");
        break;
      }
      configFlagSet = atoi(scan_string);
      if (configFlagSet != 1 && configFlagSet != 0)
      {
        printf("Invalid value. Please try again\n");
        break;
      }
      if (configFlagSet)
      {
        while (true)
        {
          memset(scan_string, 0, QCMAP_MSGR_MAX_FILE_PATH_LEN);
          QCMAP_PRINTF_TAKE_INPUT("Set the priority: WLAN:0 N79:1");
          fgets(scan_string, sizeof(scan_string), stdin);
          if (!VALID_NUMERIC_INPUT(scan_string))
          {
            printf("\nInvalid response\n");
            continue;
          }
          if (atoi(scan_string) != 0 && atoi(scan_string) != 1)
          {
            printf("Enter a valid priority 0 or 1");
            continue;
          }
          n79_config.priority = (atoi(scan_string)==1) ? QCMAP_MSGR_N79_PRIORITY_V01 :
                                                         QCMAP_MSGR_WLAN_PRIORITY_V01;
          break;
        }
        memset(scan_string, 0, QCMAP_MSGR_MAX_FILE_PATH_LEN);
        QCMAP_PRINTF_TAKE_INPUT("Enter the value of N79 hysteresis timer to be set. Default is 30sec");
        if((fgets(scan_string, sizeof(scan_string), stdin) != NULL))
        {
          temp_val = atoi(scan_string);
          if (temp_val == 0)
          {
            n79_config.n79_hysteresis_timer = DEFAULT_N79_HYSTERESIS_TIMER;
          }
          else
            n79_config.n79_hysteresis_timer = temp_val;
        }
        memset(scan_string, 0, QCMAP_MSGR_MAX_FILE_PATH_LEN);
        QCMAP_PRINTF_TAKE_INPUT("Enter the value of wlan 5G hysteresis timer to be set. Default is 30sec");
        if(fgets(scan_string, sizeof(scan_string), stdin) != NULL)
        {
          temp_val = atoi(scan_string);
          if (temp_val == 0)
          {
            n79_config.wlan_hysteresis_timer = DEFAULT_WLAN_HYSTERESIS_TIMER;
          }
          else
            n79_config.wlan_hysteresis_timer = temp_val;
        }
        memset(scan_string, 0, QCMAP_MSGR_MAX_FILE_PATH_LEN);
        QCMAP_PRINTF_TAKE_INPUT("Enter the value of number of re-attempts if QMI failure. Default is 3 attempts");
        if(fgets(scan_string, sizeof(scan_string), stdin) != NULL)
        {
          temp_val = atoi(scan_string);
          if (temp_val == 0)
          {
            n79_config.retries = DEFAULT_QMI_RETRIES;
          }
          else
            n79_config.retries = temp_val;
        }
        memset(scan_string, 0, QCMAP_MSGR_MAX_FILE_PATH_LEN);
        QCMAP_PRINTF_TAKE_INPUT("Enter the timer delay between two attempts to send QMI to"
               " enable or disable n79. Default is 1 sec");
        if(fgets(scan_string, sizeof(scan_string), stdin) != NULL)
        {
          temp_val = atoi(scan_string);
          if (temp_val == 0)
          {
            n79_config.delay_between_retries = DEFAULT_ATTEMPTS_TIMER;
          }
          else
            n79_config.delay_between_retries = temp_val;
        }
      }

      if(QcMapClient->SetN79Config(n79_config, configFlagSet, &qmi_err_num))
        printf("N79 config set successfully\n");
      else
        printf("N79 config not set successfully\n");

      break;
    }

    /* Retrieve the n79 configuration*/
    case 15:
    {
      boolean config_enable = false;
      memset(&n79_config, 0, sizeof(qcmap_msgr_n79_config_v01));

      if (QcMapClient->GetN79Config(&n79_config, &config_enable, &qmi_err_num))
      {
        printf("N79 config is %s in the build to register for indications\n",
                                         config_enable?"enabled":"disabled");
        printf("Higher priority is for %s\n", n79_config.priority ? "N79":"WLAN");
        printf("N79 hysteresis timer value is %d\n", n79_config.n79_hysteresis_timer);
        printf("WLAN hysteresis timer value is %d\n", n79_config.wlan_hysteresis_timer);
        printf("The number of retries for n79 if QMI command fails is %d\n", n79_config.retries);
        printf("The delay bewteen the retries for n79 %d sec\n", n79_config.delay_between_retries);
      }
      else
         printf("Unable to get the N79 config\n");
      break;
    }

    /* Register for indications with indication mask*/
    case 16:
    {
      QCMAP_PRINTF_TAKE_INPUT("\nPlease enter the indication register mask : ");
      fgets(scan_string, sizeof(scan_string), stdin);
      indication_mask = strtoul(scan_string, NULL, 16);
      if (QcMapClient->RegisterForIndications(&qmi_err_num, indication_mask))
        printf("\nSuccessfully Registered for Indications.");
      else
        printf("\nCan not register for indications, Error: %d", qmi_err_num);
      break;
    }

    /* Set Feature Mode */
    case 17:
    {
      uint64 features = 0;
      qcmap_client_feature_mode_config feature_mode_config;
      memset(&feature_mode_config, 0,  sizeof(qcmap_client_feature_mode_config));
      int feature_list_size = sizeof(feature_modes_list)/sizeof(feature_modes_list[0]);
      int feature_input = 0;

      printf("List of feature modes supported:\n");
      for (int i = 0; i < feature_list_size; i++)
      {
        printf("%s\n", feature_modes_list[i]);
      }
      QCMAP_PRINTF_TAKE_INPUT("Please enter the choice: ");
      fgets(scan_string, sizeof(scan_string), stdin);
      feature_input = atoi(scan_string);

      if (feature_input == 1)
      {
        /* IP Passthrough */
        features |= QCMAP_MSGR_MASK_ENABLE_FEATURE_IP_PASSTHROUGH_V01;

        feature_mode_config.ip_passthrough_feature_valid = true;
        printf("\nFeature modes supported for IP Passthrough: ");
        printf("\n1.IP Passthrough with NAT\n2.IP Passthrough without NAT");
        QCMAP_PRINTF_TAKE_INPUT("\nPlease enter your choice: ");
        fgets(scan_string, sizeof(scan_string), stdin);
        feature_input = atoi(scan_string);
        if (feature_input == 1)
        {
          feature_mode_config.ip_passthrough_feature_mode = QCMAP_MSGR_IP_PASSTHROUGH_MODE_WITH_NAT_V01;
        }
        else if (feature_input == 2)
        {
          feature_mode_config.ip_passthrough_feature_mode = QCMAP_MSGR_IP_PASSTHROUGH_MODE_WITHOUT_NAT_V01;
        }
        else
        {
          printf("\nIncorrect ip passthrough mode\n");
          break;
        }
      }
      else if (feature_input == 2)
      {
        /* DHCP LAN options */
        int sub_feature_input = 0;
        features |= QCMAP_MSGR_MASK_ENABLE_FEATURE_DHCP_LAN_OPTIONS_V01;
        feature_mode_config.dhcp_lan_options_feature_valid = true;

        printf("\nPlease select the DHCP LAN option:\n1. DHCP Vendor Information\n2. DHCP Timezone Information");
        QCMAP_PRINTF_TAKE_INPUT("\nPlease enter your choice: ");
        fgets(scan_string, sizeof(scan_string), stdin);
        feature_input = atoi(scan_string);

        /* DHCP Vendor Information */
        if (feature_input == 1)
        {
          QCMAP_PRINTF_TAKE_INPUT("\nMake sure you've configured the Vendor Class Identifier in the XML? 1. Continue 2. Cancel: ");
          fgets(scan_string, sizeof(scan_string), stdin);
          sub_feature_input = atoi(scan_string);
          if (sub_feature_input == 1)
          {
            feature_mode_config.dhcp_lan_options_feature_modes |= QCMAP_MSGR_MASK_DHCP_VENDOR_INFORMATION_V01;
          }
          else if (sub_feature_input == 2)
          {
            printf("\nPlease configure Vendor Class Identifier and enable again");
            break;
          }
          else
          {
            printf("\nIncorrect option\n");
            break;
          }
        }
        /* DHCP Timezone Information */
        else if (feature_input == 2)
        {
          feature_mode_config.dhcp_lan_options_feature_modes |= QCMAP_MSGR_MASK_DHCP_TIMEZONE_INFORMATION_V01;
        }
        else
        {
          printf("\nIncorrect option\n");
          break;
        }
      }
      else if (feature_input == 3)
      {
        /* EoGRE mode */
        features |= QCMAP_MSGR_MASK_ENABLE_FEATURE_ETH_OVER_GRE_V01;
      }

      else if (feature_input == 5)
      {
        /* Eth Pdu mode */
        features |= QCMAP_MSGR_MASK_ENABLE_FEATURE_ETH_PDU_V01;
        feature_mode_config.eth_pdu_device_index_valid = true;
        printf("\neth device index: ");
        printf("\n0.eth0\n1.eth1");
        QCMAP_PRINTF_TAKE_INPUT("\nPlease enter your choice: ");
        fgets(scan_string, sizeof(scan_string), stdin);
        if (!VALID_NUMERIC_INPUT(scan_string))
        {
          printf("\nInvalid response\n");
          break;
        }
        feature_input = atoi(scan_string);
        if (feature_input == 0)
        {
          feature_mode_config.eth_device_index = 0;
        }
        else if (feature_input == 1)
        {
          feature_mode_config.eth_device_index = 1;
        }
        else
        {
          printf("\nIncorrect eth device index!\n");
          break;
        }
      }

      else if (feature_input == 6)
      {
        features |= QCMAP_ENABLE_IPSEC_FEATURE;
        printf("\nPerform ipsec feature to true\n");
        feature_mode_config.ipsec_feature_valid = true;
        feature_mode_config.ipsec_feature_enable = true;
      }
      else
      {
        /* Feature choice is incorrect */
        printf("\nIncorrect feature choice\n");
        break;
      }

      if (QcMapClient->SetFeatureMode(features, &feature_mode_config, &qmi_err_num))
      {
        printf("\nSuccessfully set the feature mode \n");
        if (feature_mode_config.dhcp_lan_options_feature_modes & QCMAP_MSGR_MASK_DHCP_TIMEZONE_INFORMATION_V01)
        {
          printf("\nSetting DHCP Timezone information takes some time. Will be notified while getting configured.\n");
        }
        if (features & QCMAP_MSGR_MASK_ENABLE_FEATURE_ETH_OVER_GRE_V01)
        {
          printf("\nAuto-reboot will be triggered when setting EoGRE feature for the first time\n");
        }
        if (features & QCMAP_MSGR_MASK_ENABLE_FEATURE_ETH_PDU_V01)
        {
          printf("\nAuto-reboot will be triggered when setting Eth Pdu feature with different mode!\n");
        }
      }
      else
      {
        if (qmi_err_num == QMI_ERR_INVALID_HANDLE_V01)
        {
          printf("\nMobileAP is not enabled");
          break;
        }
        printf("\nCannot set the feature mode. Error: %d", qmi_err_num);
      }
      break;
    }

    /* Get Feature Mode */
    case 18:
    {
      uint64 features = 0;
      qcmap_client_feature_mode_config feature_mode_config;
      memset(&feature_mode_config, 0, sizeof(qcmap_client_feature_mode_config));
      bool ret_val = QcMapClient->GetFeatureMode(&features, &feature_mode_config, &qmi_err_num);
      if (!ret_val)
      {
        printf("\n\nFailure in getting feature modes.");
        break;
      }

      if (features == 0)
      {
        printf("\nNo features / modes are enabled\n");
      }
      else
      {
        if (features & QCMAP_MSGR_MASK_ENABLE_FEATURE_IP_PASSTHROUGH_V01)
        {
          printf("\nIP Passthrough Feature: Set ");
          /* IP Passthrough feature */
          if (feature_mode_config.ip_passthrough_feature_valid)
          {
            if (feature_mode_config.ip_passthrough_feature_mode ==
                QCMAP_MSGR_IP_PASSTHROUGH_MODE_WITH_NAT_V01)
            {
              printf("With NAT\n");
            }
            else
            {
              printf("Without NAT\n");
            }
          }
        }
        if (features & QCMAP_MSGR_MASK_ENABLE_FEATURE_DHCP_LAN_OPTIONS_V01)
        {
          printf("\nDHCP LAN Options: Currently enabled: ");
          /* DHCP LAN Options feature */
          if (feature_mode_config.dhcp_lan_options_feature_valid)
          {
            if (feature_mode_config.dhcp_lan_options_feature_modes &
                QCMAP_MSGR_MASK_DHCP_VENDOR_INFORMATION_V01)
            {
              printf("\nDHCP Vendor Information (Options 43/60)");
            }
            if (feature_mode_config.dhcp_lan_options_feature_modes &
                QCMAP_MSGR_MASK_DHCP_TIMEZONE_INFORMATION_V01)
            {
              printf("\nDHCP Timezone Information (Options 100/101)");
            }
            printf("\n");
          }
        }
        else
        {
          printf("\nDHCP LAN Options: Currently disabled!");
        }
        if (features & QCMAP_MSGR_MASK_ENABLE_FEATURE_ETH_OVER_GRE_V01)
        {
          printf("\nEoGRE Feature: Set ");
        }
        else
        {
          printf("\nEoGRE Feature: Not Set ");
        }

        if (features & QCMAP_MSGR_MASK_ENABLE_FEATURE_ETH_PDU_V01)
        {
          printf("\nEth Pdu Mode is enabled!");
        }
        else
        {
          printf("\nEth Pdu Mode is disabled!");
        }

        if(features & QCMAP_ENABLE_IPSEC_FEATURE)
        {
          if(feature_mode_config.ipsec_feature_valid && feature_mode_config.ipsec_feature_enable)
          {
            printf("\nIPsec Feature enabled\n");
          }
          else
          {
            printf("\nIPsec Feature NOT enabled\n");
          }
        }
      }
      break;
    }

    /* Reset Feature Mode */
    case 19:
    {
      uint64 features = 0;
      qcmap_client_feature_mode_config feature_mode_config;
      memset(&feature_mode_config, 0, sizeof(qcmap_client_feature_mode_config));
      int feature_list_size = sizeof(feature_modes_list)/sizeof(feature_modes_list[0]);
      bool ret_val = false;
      int feature_input = 0;

      printf("List of feature modes supported:\n");
      for (int i = 0; i < feature_list_size; i++)
      {
        printf("%s\n", feature_modes_list[i]);
      }
      QCMAP_PRINTF_TAKE_INPUT("Please enter the choice: ");
      fgets(scan_string, sizeof(scan_string), stdin);
      feature_input = atoi(scan_string);

      if (feature_input == 1)
      {
        /* IP Passthrough */
        features |= QCMAP_MSGR_MASK_ENABLE_FEATURE_IP_PASSTHROUGH_V01;
        feature_mode_config.ip_passthrough_feature_valid = true;
        printf("\nDefault value: IP Passthrough with NAT\n");
      }
      if (feature_input == 2)
      {
        features |= QCMAP_MSGR_MASK_ENABLE_FEATURE_DHCP_LAN_OPTIONS_V01;
        feature_mode_config.dhcp_lan_options_feature_valid = true;
        printf("\n1. Reset DHCP Vendor Information\n2. Reset DHCP Timezone Information");
        QCMAP_PRINTF_TAKE_INPUT("\nPlease enter your choice: ");
        fgets(scan_string, sizeof(scan_string), stdin);
        feature_input = atoi(scan_string);
        if (feature_input == 1)
        {
          feature_mode_config.dhcp_lan_options_feature_modes |= QCMAP_MSGR_MASK_DHCP_VENDOR_INFORMATION_V01;
        }
        else if (feature_input == 2)
        {
          feature_mode_config.dhcp_lan_options_feature_modes |= QCMAP_MSGR_MASK_DHCP_TIMEZONE_INFORMATION_V01;
        }
        else
        {
          printf("\nIncorrect Choice");
          break;
        }
      }
      if (feature_input == 3)
      {
        /* EoGRE feature */
        features |= QCMAP_MSGR_MASK_ENABLE_FEATURE_ETH_OVER_GRE_V01;
        printf("\nDefault value: Disable EoGRE\n");
        printf("EoGRE related feature yet to be implemented on OpenWRT");
        break;
      }
      if (feature_input == 5)
      {
        printf("\nEth Pdu mode will be set to disabled!\n");
        /* Eth Pdu feature */
        features |= QCMAP_MSGR_MASK_ENABLE_FEATURE_ETH_PDU_V01;
      }

      if (feature_input == 6)
      {
        features |= QCMAP_ENABLE_IPSEC_FEATURE;
        feature_mode_config.ipsec_feature_valid = true;
      }

      if (QcMapClient->ResetFeatureMode(features, &feature_mode_config, &qmi_err_num))
      {
        printf("\nSuccessfully reset the feature mode \n");
        if (features & QCMAP_MSGR_MASK_ENABLE_FEATURE_ETH_OVER_GRE_V01)
        {
          printf("\nAuto-reboot will be triggered when resetting EoGRE feature for the first time\n");
        }
        if (features & QCMAP_MSGR_MASK_ENABLE_FEATURE_ETH_PDU_V01)
        {
          printf("\nAuto-reboot will be triggered when resetting Eth Pdu feature with diffent mode!\n");
        }
      }
      else
      {
        printf("\nCannot reset the feature mode. Error: %d", qmi_err_num);
      }
      break;
    }

    /* Set Client Preference (Profile Handle, Subs_id, etc) */
    case 20:
    {
      int default_no = 1;
      profile_handle_type_v01 current_profile_handle = 0, profile_handle = 0;
      qcmap_msgr_subscription_enum_v01 subscription_id = QCMAP_MSGR_DEFAULT_SUBS_V01;
      qcmap_client_preference_config client_pref_config;
      int userOption = 0;

      ZERO_INIT_ARG(client_pref_config);

      /* Select Profile Handle */
      if (ShowAllWWANProfiles(&default_no, &current_profile_handle))
      {
        ASK_USER_FOR_INPUT_INT_PARAM("Do you want to enter Profile Preference (1-Yes, 0-No) :", userOption);
        if (userOption == 1)
        {
          ASK_USER_FOR_INPUT_INT_PARAM("   Select Profile Handle # (Default=%d, Current=%d): ", profile_handle);
          client_pref_config.client_preference |= QCMAP_CLIENT_WWAN_PROFILE_HANDLE_PREFERENCE;
          client_pref_config.profile_handle = profile_handle;
        }
      }

      /* Select Subscription Id */
      ASK_USER_FOR_INPUT_INT_PARAM("Do you want to enter Subscription Preference (1-Yes, 0-No) :", userOption);
      if (userOption == 1)
      {
        ASK_USER_FOR_INPUT_INT_PARAM("   Enter Subscription-Id (0-Default, 1-Primary, 2-Secondary): ", subscription_id);
        client_pref_config.client_preference |= QCMAP_CLIENT_WWAN_SUBSCRIPTION_PREFERENCE;
        client_pref_config.subscription_id = subscription_id;
      }

      if (QcMapClient->SetClientPreference(client_pref_config, &qmi_err_num))
      {
        printf("  Set Client Preference Success.\n");
      }
      else
      {
        printf(" Error: Unable to Set Client Preference, qmi_err_num=%d\n", qmi_err_num);
      }
      break;
    }

    /* Get Client Preference (Profile Handle, Subscription Id etc) */
    case 21:
    {
      qcmap_client_preference_config client_pref_config;
      if (QcMapClient->GetClientPreference(&client_pref_config, &qmi_err_num))
      {
        printf("  Set Client Preference Success.\n");
        if (client_pref_config.client_preference & QCMAP_CLIENT_WWAN_PROFILE_HANDLE_PREFERENCE)
        {
          printf("    Profile Handle  : %d\n", client_pref_config.profile_handle);
        }
        if (client_pref_config.client_preference & QCMAP_CLIENT_WWAN_SUBSCRIPTION_PREFERENCE)
        {
          printf("    Subscription Id : %d\n", client_pref_config.subscription_id);
        }
      }
      else
      {
        printf(" Error: Unable to Get Client Preference, qmi_err_num=%d\n", qmi_err_num);
      }
      break;
    }

    /* UnRegister for indications with indication mask*/
    case 22:
    {
      QCMAP_PRINTF_TAKE_INPUT("\nPlease enter the indication register mask : ");
      fgets(scan_string, sizeof(scan_string), stdin);
      indication_mask = strtoul(scan_string, NULL, 16);
      if (QcMapClient->UnRegisterForIndications(&qmi_err_num, indication_mask))
        printf("\nSuccessfully UnRegistered for Indications.");
      else
        printf("\nCan not unregister for indications, Error: %d", qmi_err_num);
      break;
    }

    default:
    {
      printf("Invalid response %d\n", mobileApOpt);
    }
    break;
  }
}

void lanConfig(int lanOpt)
{
  qmi_error_type_v01 qmi_err_num;
  /* Initialize QMI Error Number. */
  qmi_err_num = QMI_ERR_NONE_V01;
  char scan_string[QCMAP_MSGR_MAX_FILE_PATH_LEN] = {0};
  qcmap_msgr_sw_ip_ch_config_v01 conf;
  qcmap_msgr_dhcp_reservation_v01 dhcp_reserv_record;
  in_addr addr;
  int ix = 0;
  boolean set_sw_ip_ch_flag = false;
  qcmap_msgr_device_mode_enum_v01 device_mode = QCMAP_MSGR_DEVICE_NONE_V01;
  memset(scan_string, 0, QCMAP_MSGR_MAX_FILE_PATH_LEN);
  memset(&addr, 0, sizeof(in_addr));
  memset(&conf, 0, sizeof(qcmap_msgr_sw_ip_ch_config_v01));
  char mac_addr_str[QCMAP_MSGR_MAC_ADDR_NUM_CHARS_V01] = {0};

  /* LAN Configuration options */
  switch(lanOpt)
  {
  /* setting LAN configuration parameters */
  case 1:
  {
    int p_error=0;
    qcmap_msgr_lan_config_v01 lan_config;
    memset(&lan_config,0,sizeof(qcmap_msgr_lan_config_v01));

    in_addr addr;
    memset(scan_string, 0, QCMAP_MSGR_MAX_FILE_PATH_LEN);

    QCMAP_PRINTF_TAKE_INPUT("   Do you want to set LAN Configuration(1-Yes/0-No) : ");
    fgets(scan_string, sizeof(scan_string), stdin);
    if (atoi(scan_string) == 1)
    {
      while (TRUE)
      {
        QCMAP_PRINTF_TAKE_INPUT("   Please input Gateway IP address : ");
        if (fgets(scan_string, sizeof(scan_string), stdin) != NULL)
        {
          memset(&addr,0,sizeof(in_addr));
          scan_string[strlen(scan_string)-1]='\0';
          if ( !(inet_aton(scan_string, &addr) <= 0) )
          {
            lan_config.gw_ip = ntohl(addr.s_addr);
            break;
          }
        }
        printf("      Invalid IPv4 address %s\n", scan_string);
      }
      while (TRUE)
      {
        QCMAP_PRINTF_TAKE_INPUT("   Please input AP subnet  : ");
        if (fgets(scan_string, sizeof(scan_string), stdin) != NULL)
        {
          memset(&addr,0,sizeof(in_addr));
          scan_string[strlen(scan_string)-1]='\0';
          if ( !(inet_aton(scan_string, &addr) <= 0) )
          {
            lan_config.netmask = ntohl(addr.s_addr);
            break;
          }
        }
        printf("      Invalid IPv4 address %s\n", scan_string);
      }
      QCMAP_PRINTF_TAKE_INPUT("   Please input Enable/Disable DHCP(1-Enable/0-Disable):");
      fgets(scan_string, sizeof(scan_string), stdin);
      if ( atoi(scan_string) == 1)
      {
        lan_config.enable_dhcp = 1;
        QCMAP_PRINTF_TAKE_INPUT("   Please input DHCP Configuration\n");
        while (TRUE)
        {
          QCMAP_PRINTF_TAKE_INPUT("   Please input starting DHCPD address : ");
          if (fgets(scan_string, sizeof(scan_string), stdin) != NULL)
          {
            scan_string[strlen(scan_string)-1]='\0';
            memset(&addr,0,sizeof(in_addr));
            if ( !(inet_aton(scan_string, &addr) <= 0) )
            {
              lan_config.dhcp_config.dhcp_start_ip = ntohl(addr.s_addr);
              break;
            }
          }
          printf("      Invalid IPv4 address %s\n", scan_string);
        }
        while (TRUE)
        {
          QCMAP_PRINTF_TAKE_INPUT("   Please input ending DHCPD address : ");
          if (fgets(scan_string, sizeof(scan_string), stdin) != NULL)
          {
            scan_string[strlen(scan_string)-1]='\0';
            memset(&addr,0,sizeof(in_addr));
            if ( !(inet_aton(scan_string, &addr) <= 0) )
            {
              lan_config.dhcp_config.dhcp_end_ip = ntohl(addr.s_addr);
              // Range check
              if (lan_config.dhcp_config.dhcp_end_ip >= lan_config.dhcp_config.dhcp_start_ip) {
                 break;
              } else {
                 LOG_MSG_ERROR("SSID1 (AP Mode) DHCP Address Range provided is too short",0,0,0);
                 QCMAP_PRINTF_TAKE_INPUT(" Please enter a different DHCP End Address. \n");
              }
            }
          }
          printf("      Invalid IPv4 address %s\n", scan_string);
        }
        while(TRUE)
        {
          QCMAP_PRINTF_TAKE_INPUT("   Please input DHCP lease time in Seconds:");
          QCMAP_PRINTF_TAKE_INPUT("   Minimum DHCP lease time in 120 Seconds:");
          fgets(scan_string, sizeof(scan_string), stdin);
          lan_config.dhcp_config.lease_time = (uint32)atoi(scan_string);
          if(lan_config.dhcp_config.lease_time >= MIN_DHCP_LEASE)
          break;
        }
      }
      else
      {
        lan_config.enable_dhcp = 0;
      }
    }

    if (QcMapClient->SetLANConfig(lan_config, &qmi_err_num))
    {
      if (qmi_err_num ==  QMI_ERR_NONE_V01) {
        QCMAP_PRINTF_TAKE_INPUT("\n LAN Config Set Successfully\n");
      } else {
          if (qmi_err_num == QMI_ERR_INVALID_ARG_V01) {
            printf("SSID1 (AP Mode) DHCP Address Range provided is invalid, Minimum range is 7 \n");
            printf("Setting AP DHCP address to default values which are derived from AP Gateway Addr \n");
          } else {
            printf("\n LAN Config set fails, Error: 0x%x", qmi_err_num);
          }
      }
    }
    else if (qmi_err_num == QMI_ERR_INCOMPATIBLE_STATE_V01)
    {
      printf("\n Rejected SetLANConfig as IPPT WITHOUT NAT/ WITH NAT FCD is active Error: 0x%x ", qmi_err_num);
    }
    else
    {
      if (qmi_err_num == QMI_ERR_INVALID_HANDLE_V01)
      {
        printf("\nMobileAP is not enabled");
        break;
      }
      printf("\n LAN Config set fails, Error: 0x%x", qmi_err_num);
    }
    break;
  }

  /* Getting LAN configuration's */
  case 2:
  {
    qcmap_msgr_lan_config_v01 lan_config;
    memset(&lan_config,0,sizeof(qcmap_msgr_lan_config_v01));
    in_addr addr;
    int p_error=0;
    memset(&addr, 0 ,sizeof(in_addr));
    memset(scan_string, 0, QCMAP_MSGR_MAX_FILE_PATH_LEN);

    if (QcMapClient->GetLANConfig(&lan_config,&qmi_err_num))
    {
      /* Print AP Configuration. */
      printf("\nAP Configuration.\n");
      addr.s_addr = htonl(lan_config.gw_ip);
      printf("\nGateway IP: %s\n", inet_ntoa(addr));
      addr.s_addr = htonl(lan_config.netmask);
      printf("\nNetmask : %s\n", inet_ntoa(addr));
      printf("\nDHCP Enabled: %d\n", lan_config.enable_dhcp);
      if ( lan_config.enable_dhcp == TRUE )
      {
        addr.s_addr = htonl(lan_config.dhcp_config.dhcp_start_ip);
        printf("\nDHCP Start IP: %s\n", inet_ntoa(addr));
        addr.s_addr = htonl(lan_config.dhcp_config.dhcp_end_ip);
        printf("\nDHCP End IP : %s\n", inet_ntoa(addr));
        printf("\nDHCP Lease Time (seconds) : %d\n", lan_config.dhcp_config.lease_time);
      }
    }
    else
    {
      printf("\nGet LAN Config failed, Error:0x%x", qmi_err_num);
    }
    break;
  }

  /* Activate LAN */
  case 3:
  {
    if(QcMapClient->ActivateLAN(&qmi_err_num))
    {
      printf("\nActivated LAN\n");
    }
    else
    {
      if (qmi_err_num == QMI_ERR_INVALID_HANDLE_V01)
      {
        printf("\nMobileAP is not enabled");
        break;
      }
      printf("\nFailed to Activate LAN, Error: 0x%x", qmi_err_num);
    }
    break;
  }

  /*Add DHCP Reservation record*/
  case 4:
  {
      int newline_index;
      memset(&dhcp_reserv_record,0,sizeof(qcmap_msgr_dhcp_reservation_v01));
      int i = 0, device_type = -1;
      boolean enable = TRUE;
      dhcp_reserv_record.enable_reservation = enable;
      while(TRUE)
      {
        QCMAP_PRINTF_TAKE_INPUT("Enter the device type(0-USB/1-Other LAN clients):   ");
        fgets(scan_string,sizeof(scan_string),stdin);
        newline_index = strlen(scan_string)-1;
        scan_string[newline_index] = '\0';
        device_type = (atoi(scan_string));
        /* strcmp is used because atoi("0") and atoi(string representation of an non integral number) both gives 0 */
        if( device_type == 0 && strcmp("0", scan_string) != 0 )
          device_type = -1;
        if (!(device_type == 0 || device_type == 1))
        {
          QCMAP_PRINTF_TAKE_INPUT("\nDevice Type not Supported");
          continue;
        }
        break;
      }
      if ( device_type == 1 )
      {
        QCMAP_PRINTF_TAKE_INPUT("\nPlease input the MAC address  :");
         GET_MAC_ADDR(scan_string,mac_addr_int);
         memcpy(dhcp_reserv_record.client_mac_addr, mac_addr_int,\
                sizeof(dhcp_reserv_record.client_mac_addr));
      }
      QCMAP_PRINTF_TAKE_INPUT("\nPlease input the client reserved IP(xxx.xxx.xxx.xxx)  :");
      GET_IP_ADDR(scan_string,addr);
      dhcp_reserv_record.client_reserved_ip = ntohl(addr.s_addr);
      while(TRUE)
      {
        QCMAP_PRINTF_TAKE_INPUT("\nEnter device name.If client is not USB client,press \"ENTER\" key to skip  :");
        if ( fgets(scan_string,sizeof(scan_string),stdin) != NULL )
        {
          if (*scan_string == '\n')
          {
            if (device_type)
            {
              break;
            }
            else
            {
              QCMAP_PRINTF_TAKE_INPUT("Client name is mandatory for USB tethered client\n");
              continue;
            }
          }
          else
          {
            for ( i=0;i < strlen(scan_string)-1;i++)
            {
              dhcp_reserv_record.client_device_name[i] = scan_string[i];
            }
            dhcp_reserv_record.client_device_name[i] ='\0';
            break;
          }
        }
      }
      QCMAP_PRINTF_TAKE_INPUT("\nEnable/disable reservation for this client(1-Enable/0-Disable/Enter-skipped  :");
      fgets(scan_string,sizeof(scan_string),stdin);

      if ( *scan_string != '\n')
      {
          enable = (atoi(scan_string)) ? true:false;
          dhcp_reserv_record.enable_reservation = enable;
      }
      if( QcMapClient->AddDHCPReservRecord(&dhcp_reserv_record, &qmi_err_num) )
      {
          QCMAP_PRINTF_TAKE_INPUT("\nDHCP  Reservation Record added successfully");
      }
      else
      {
         if (qmi_err_num == QMI_ERR_INVALID_HANDLE_V01)
         {
           printf("\nMobileAP is not enabled");
           break;
         }
         QCMAP_PRINTF_TAKE_INPUT("\nFailed to add DHCP Reservation record. Error 0x%x.\n ", qmi_err_num);
      }
      break;
  }

  /* Display DHCP Reservation Record */
  case 5:
  {

    uint32_t num_entries =0, i=0;
    in_addr tmpIP;
    char mac_addr_str[QCMAP_MSGR_MAC_ADDR_NUM_CHARS_V01]; /*char array of mac address*/
    qcmap_msgr_dhcp_reservation_v01 dhcp_reserv_record[QCMAP_MSGR_MAX_DHCP_RESERVATION_ENTRIES_V01];
    memset(dhcp_reserv_record,0,\
           QCMAP_MSGR_MAX_DHCP_RESERVATION_ENTRIES_V01*sizeof(qcmap_msgr_dhcp_reservation_v01));

    if( QcMapClient->GetDHCPReservRecords(dhcp_reserv_record, &num_entries,&qmi_err_num) )
    {
       //display each DHCP reservation records
      if ( num_entries == 0 )
      {
        QCMAP_PRINTF_TAKE_INPUT("\nNo DHCP Reservation Records");
      }
      else
      {
         for ( i = 0;i < num_entries; i++)
         {
           QCMAP_PRINTF_TAKE_INPUT("\nEntry  %d:",i);
           ds_mac_addr_ntop(dhcp_reserv_record[i].client_mac_addr,mac_addr_str);
           if ( strncmp(mac_addr_str,MAC_NULL_STRING,QCMAP_MSGR_MAC_ADDR_NUM_CHARS_V01) != 0 )
           {
               QCMAP_PRINTF_TAKE_INPUT("\nMAC address of the client[%i]: %s",i,mac_addr_str);
           }
           tmpIP.s_addr =ntohl(dhcp_reserv_record[i].client_reserved_ip);
           QCMAP_PRINTF_TAKE_INPUT("\nIP address of the client[%i]: %s",i,inet_ntoa(tmpIP));
           if ( dhcp_reserv_record[i].client_device_name[0] != '\0')
           {
             QCMAP_PRINTF_TAKE_INPUT("\nDevice Name of the client[%i]: %s",i,\
                    dhcp_reserv_record[i].client_device_name);
           }
           QCMAP_PRINTF_TAKE_INPUT("\nDHCP Reservation enabled for the client[%i]: %d",i,\
                  dhcp_reserv_record[i].enable_reservation);
         }
       }
    }
    else
    {
       QCMAP_PRINTF_TAKE_INPUT("\nFailed to Display DHCP Reservation record. Error 0x%x.\n ", qmi_err_num);
    }
    break;
  }

  /*Edit DHCP Reservation record*/
  case 6:
  {
    memset(&dhcp_reserv_record,0,sizeof(qcmap_msgr_dhcp_reservation_v01));
    dhcp_reserv_record.enable_reservation = MAX_UINT8_VAL;
    uint32_t addr_to_edit =0;
    addr.s_addr =0;
    uint8 options=0;
    boolean enable;
    int i;
    QCMAP_PRINTF_TAKE_INPUT("\nPlease input the client reserved IP(xxx.xxx.xxx.xxx)  :");
    GET_IP_ADDR(scan_string,addr);
    addr_to_edit = ntohl(addr.s_addr);
    while (TRUE )
    {
       QCMAP_PRINTF_TAKE_INPUT("\nPlease enter the field to edit:  ");
       QCMAP_PRINTF_TAKE_INPUT("\n\t1. MAC Address\n\t2. IP Addr\n\t3. Device Name\n\t4. Enable/Disable\n\t:");
       fgets(scan_string,sizeof(scan_string),stdin);
       options = atoi(scan_string);
       switch (options)
       {
         case 1:
            QCMAP_PRINTF_TAKE_INPUT("Please input the MAC address  :");
            GET_MAC_ADDR(scan_string,mac_addr_int);
            memcpy(dhcp_reserv_record.client_mac_addr, mac_addr_int,\
                   sizeof(dhcp_reserv_record.client_mac_addr));
            break;

         case 2:
           addr.s_addr =0;
           QCMAP_PRINTF_TAKE_INPUT("Please input the client reserved IP(xxx.xxx.xxx.xxx)  :");
           GET_IP_ADDR(scan_string,addr);
           dhcp_reserv_record.client_reserved_ip = ntohl(addr.s_addr);
           break;

         case 3:
            while(TRUE)
            {
                QCMAP_PRINTF_TAKE_INPUT("Please input the device name  :");
                if ( fgets(scan_string,sizeof(scan_string),stdin) != NULL )
                {
                     if ( *scan_string != '\n')
                     {
                        for (i=0;i < strlen(scan_string)-1;i++)
                        {
                            dhcp_reserv_record.client_device_name[i] = scan_string[i];
                        }
                        dhcp_reserv_record.client_device_name[i] ='\0';
                       break;
                     }
                }
                QCMAP_PRINTF_TAKE_INPUT("\nInvalid Device name entered %s", scan_string);
            }
            break;

         case 4:
            QCMAP_PRINTF_TAKE_INPUT("Enable/disable reservation for this client(1-Enable/0-Disable   :");
            fgets(scan_string,sizeof(scan_string),stdin);
            enable = (atoi(scan_string)) ? true:false;
            dhcp_reserv_record.enable_reservation = enable;
            break;

        default:
          QCMAP_PRINTF_TAKE_INPUT("Invalid response %d\n",options);
          break;
       }
       QCMAP_PRINTF_TAKE_INPUT("Do you wish to Edit more fields(Enter-skipped/other character to continue):");
       fgets(scan_string,sizeof(scan_string),stdin);
       if ( *scan_string == '\n')
       {
          break;
       }
       else
       {
          continue;
       }
    }
    if( QcMapClient->EditDHCPReservRecord(&addr_to_edit,&dhcp_reserv_record,&qmi_err_num) )
    {
      QCMAP_PRINTF_TAKE_INPUT("\nDHCP  Reservation Record edited successfully");
    }
    else
   {
     if (qmi_err_num == QMI_ERR_INVALID_HANDLE_V01)
     {
       printf("\nMobileAP is not enabled");
       break;
     }
      QCMAP_PRINTF_TAKE_INPUT("\nFailed to edit DHCP Reservation record. Error 0x%x.\n ", qmi_err_num);
   }
  break;
  }

  /*Delete DHCP Reservation record*/
  case 7:
  {
     uint32_t addr_to_edit =0;
     addr.s_addr =0;
     QCMAP_PRINTF_TAKE_INPUT("Please input the client reserved IP(xxx.xxx.xxx.xxx)  :");
     GET_IP_ADDR(scan_string,addr);

     addr_to_edit = ntohl(addr.s_addr);
     if( QcMapClient->DeleteDHCPReservRecord(&addr_to_edit,&qmi_err_num) )
     {
        QCMAP_PRINTF_TAKE_INPUT("\nDHCP  Reservation Record deleted successfully");
     }
     else
     {
       QCMAP_PRINTF_TAKE_INPUT("\nFailed to delete DHCP Reservation record. Error 0x%x.\n ", qmi_err_num);
     }
     break;
  }

  /* Set/Get Gateway URL */
  case 8:
  {
   char url[QCMAP_MSGR_MAX_GATEWAY_URL_V01];
   uint32_t url_len = 0;
   uint8 options = 0;
   bzero( url, QCMAP_MSGR_MAX_GATEWAY_URL_V01);
   QCMAP_PRINTF_TAKE_INPUT("   Please input Gateway URL (1-SET URL /0-GET URL) : ");
   fgets(scan_string,sizeof(scan_string),stdin);
   if (!VALID_NUMERIC_INPUT(scan_string))
   {
      printf("\nInvalid response\n");
      break;
   }
   options = atoi(scan_string);

   if ( options == 0 )
   {
      if(!QcMapClient->GetGatewayUrl((char *)&url,&url_len,&qmi_err_num))
      {
        printf("\n Failed to get URL . Error 0x%x",qmi_err_num);
      }
      printf("\n Gateway URL configured =%s ", url);
   }
   else if ( options == 1 )
   {
      QCMAP_PRINTF_TAKE_INPUT("\n Please enter the URL:");
      fgets( scan_string, sizeof(scan_string), stdin);
      strlcpy( url, scan_string, strlen(scan_string));
      if(!QcMapClient->SetGatewayUrl((char *)&url, strlen(url), &qmi_err_num))
      {
        printf("\n Failed to set URL . Error 0x%x",qmi_err_num);
      }
      else
      {
        printf("\n Gateway URL configured \n");
      }
   }
   else
   {
     printf("\n Invalid input \n");
   }
   break;
  }
  break;
  case 9: //Add VLAN
  {
    qcmap_msgr_vlan_config_ex_v01 vlan_config_ex;

    int phy_option;
    int vlan_id;
    bool is_acclerated = false;
    bool retval;

    memset(&vlan_config_ex, 0, sizeof(vlan_config_ex));


    //tell user what options are available
    printf("   +--------------+\n");
    printf("   |              |\n");
    printf("   |    PHY       |\n");
    printf("   |              |\n");
    printf("   +--------------+\n");
    printf("   | 1. ETH       |\n");
    printf("   | 2. ECM       |\n");
    printf("   | 3. RNDIS     |\n");
    printf("   | 4. ETH-NIC2  |\n");
    printf("   +--------------+\n");

    //ask for user input
    QCMAP_PRINTF_TAKE_INPUT("   Please select PHY iface:");
    memset(scan_string, 0, QCMAP_MSGR_MAX_FILE_PATH_LEN);
    if (strlen(scan_string) == 0 && scan_string[0] == '\0')
    {
      if (fgets(scan_string, sizeof(scan_string), stdin) == NULL)
      {
        printf("error with client input: %s\n", strerror(errno));
        goto error_add_vlan;
      }
    }
    else
    {
      printf("error scan string is not empty");
      goto error_add_vlan;
    }

    phy_option = atoi(scan_string);
    switch(phy_option)
    {
      case(1): //ETH
      {
        vlan_config_ex.intf_type = QCMAP_MSGR_INTERFACE_TYPE_ETH_V01;
        break;
      }
      case(2): //ECM
      {
        vlan_config_ex.intf_type = QCMAP_MSGR_INTERFACE_TYPE_ECM_V01;
        break;
      }
      case(3): //RNDIS
      {
        vlan_config_ex.intf_type = QCMAP_MSGR_INTERFACE_TYPE_RNDIS_V01;
        break;
      }
      case(4): //ETH-NIC2
      {
        vlan_config_ex.intf_type = QCMAP_MSGR_INTERFACE_TYPE_ETH_NIC2_V01;
        break;
      }
      default:
      {
        printf("Bad option selected: %d\n", phy_option);
        goto error_add_vlan;
        break;
      }
    }

    QCMAP_PRINTF_TAKE_INPUT("\nPlease input VLAN ID(1-%d): ", MAX_VLAN_ID);
    memset(scan_string, 0, QCMAP_MSGR_MAX_FILE_PATH_LEN);
    if (strlen(scan_string) == 0 && scan_string[0] == '\0')
    {
      if (fgets(scan_string, sizeof(scan_string), stdin) == NULL)
      {
        printf("error with client input: %s\n", strerror(errno));
        goto error_add_vlan;
      }
    }
    else
    {
      printf("error scan string is not empty");
      goto error_add_vlan;
    }
    vlan_id = atoi(scan_string);
    if((vlan_id <= 0) || (vlan_id > MAX_VLAN_ID))
    {
      printf("\nInvalid value %d\n", vlan_id);
      goto error_add_vlan;
    }
    vlan_config_ex.vlan_id = (int16_t)vlan_id;

    //Skipping VLAN offload in case device mode is set
    if(QcMapClient->GetDeviceMode(&device_mode , &qmi_err_num))
    {
      if(device_mode == QCMAP_MSGR_DEVICE_NONE_V01)
      {
        printf("\nDo you want IPA offload for vlan_id=%d(1 - Enable Offload, 0 - Disable Offload)? ",
               vlan_config_ex.vlan_id);
        fflush(stdout);
        memset(scan_string, 0, QCMAP_MSGR_MAX_FILE_PATH_LEN);
       if(strlen(scan_string) == 0 && scan_string[0] == '\0')
       {
        fgets(scan_string, sizeof(scan_string) - 1, stdin);
        switch(atoi(scan_string))
        {
          case 1:
          {
            vlan_config_ex.is_accelerated = 1;
            break;
          }
          default:
          {
            vlan_config_ex.is_accelerated = 0;
            break;
          }
        }
       }
      }
    }
    if(QcMapClient->CreateVLANConfig(vlan_config_ex, &qmi_err_num, &is_acclerated))
    {
      if ((vlan_config_ex.is_accelerated == 1) && !(is_acclerated))
      {
        printf("\nVLAN Interface added without offload as offload count reached max.\n");
      }

      switch(qmi_err_num)
      {
        case(QMI_ERR_OP_IN_PROGRESS_V01):
        {
          printf("\nAuto-reboot triggered for IPA HW offload to take affect...\n");
          break;
        }
        default:
        {
          printf("\nAdd VLAN Interfaces succeeds.\n");
          break;
        }
      }
    }
    else
    {
      switch(qmi_err_num)
      {
        case(QMI_ERR_INVALID_ARG_V01):
        {
          printf("\n Add VLAN Interface fails, simultaneous Cradle and Eth not in LAN "
                 "Router mode not supported\n");
          break;
        }
        case(QMI_ERR_INVALID_HANDLE_V01):
        {
          printf("\nMobileAP is not enabled\n");
          break;
        }
        default:
        {
          printf("\n Add VLAN Interface fails, Error: 0x%x \n", qmi_err_num);
          break;
        }
      }
    }
    break;

    error_add_vlan:
      printf("\nAdd VLAN Interface fails\n");
      break;
  }
  case 10: //Get VLAN
  {
    qcmap_msgr_vlan_conf_t vlan_config;
    memset(&vlan_config, 0, sizeof(qcmap_msgr_vlan_conf_t));
    if(QcMapClient->GetVLANConfig(&vlan_config, &qmi_err_num))
    {
       if(vlan_config.vlan_config_list_len == 0)
       {
         printf("\n No VLAN Entries Configured\n");
       }
       else
       {
         printf("\nVLAN Entries Configured:\n");
         for(int i = 0; i < vlan_config.vlan_config_list_len; i++)
         {
           printf("Physical Interface: %d(%s), VLAN ID: %d, IPA Offload: %u\n",
                   vlan_config.vlan_config_list_ex[i].intf_type,
                   gIntfNames[(vlan_config.vlan_config_list_ex[i].intf_type-1) % sizeof(gIntfNames)],
                   vlan_config.vlan_config_list_ex[i].vlan_id,
                   vlan_config.vlan_config_list_ex[i].is_accelerated);
          }
       }
    }
    else
    {
      printf("\n Get VLAN Config fails, Error: 0x%x \n", qmi_err_num);
    }
    break;
  }
  case 11: //Delete VLAN
  {

    qcmap_msgr_interface_type_enum_v01 intf_type;
    int phy_option;
    int vlan_id;
    bool retval;

    //tell user what options are available
    printf("   +--------------+\n");
    printf("   |              |\n");
    printf("   |    PHY       |\n");
    printf("   |              |\n");
    printf("   +--------------+\n");
    printf("   | 1. ETH       |\n");
    printf("   | 2. ECM       |\n");
    printf("   | 3. RNDIS     |\n");
    printf("   | 4. ETH-NIC2  |\n");
    printf("   +--------------+\n");

    //ask for user input
    printf("   Please select PHY iface:");
    fflush(stdout);
    memset(scan_string, 0, QCMAP_MSGR_MAX_FILE_PATH_LEN);
    if (strlen(scan_string) == 0 && scan_string[0] == '\0')
    {
      if (fgets(scan_string, sizeof(scan_string), stdin) == NULL)
      {
        printf("error with client input: %s\n", strerror(errno));
        goto error_delete_vlan;
      }
    }
    else
    {
      printf("error scan string is not empty");
      goto error_delete_vlan;
    }

    phy_option = atoi(scan_string);
    switch(phy_option)
    {
      case(1): //ETH
      {
        intf_type = QCMAP_MSGR_INTERFACE_TYPE_ETH_V01;
        break;
      }
      case(2): //ECM
      {
        intf_type = QCMAP_MSGR_INTERFACE_TYPE_ECM_V01;
      }
      case(3): //RNDIS
      {
        intf_type = QCMAP_MSGR_INTERFACE_TYPE_RNDIS_V01;
        break;
      }
      case(4): //ETH-NIC2
      {
        intf_type = QCMAP_MSGR_INTERFACE_TYPE_ETH_NIC2_V01;
        break;
      }
      default:
      {
        printf("Bad option selected: %d\n", phy_option);
        goto error_delete_vlan;
        break;
      }
    }

    QCMAP_PRINTF_TAKE_INPUT("\nPlease input VLAN ID(1-%d): ", MAX_VLAN_ID);
    memset(scan_string, 0, QCMAP_MSGR_MAX_FILE_PATH_LEN);
    if (strlen(scan_string) == 0 && scan_string[0] == '\0')
    {
      if (fgets(scan_string, sizeof(scan_string), stdin) == NULL)
      {
        printf("error with client input: %s\n", strerror(errno));
        goto error_delete_vlan;
      }
    }
    else
    {
      printf("error scan string is not empty");
      goto error_delete_vlan;
    }
    vlan_id = atoi(scan_string);
    if((vlan_id <= 0) || (vlan_id > MAX_VLAN_ID))
    {
      printf("\nInvalid value %d\n", vlan_id);
      goto error_delete_vlan;
    }

    if(QcMapClient->DeleteVLANConfig(vlan_id, intf_type, &qmi_err_num))
    {
      printf("\n Delete VLAN Interface succeeds.\n");
    }
    else
    {
      printf("\n Delete VLAN Interface fails, Error: 0x%x \n", qmi_err_num);
    }
    break;

    error_delete_vlan:
      printf("\nDelete VLAN Interface fails\n");
      break;
  }


  case 12:
  {
    uint32_t mtu_size = STD_MTU_SIZE;
    qcmap_set_l2tp_config l2tp_enable_config;
    memset(&l2tp_enable_config,0,
                      sizeof(qcmap_set_l2tp_config));

    QCMAP_PRINTF_TAKE_INPUT("\n Please input Enable/Disable L2TP for Unmanaged Tunnels:");
    QCMAP_PRINTF_TAKE_INPUT("\n0. Disable\n1. Enable:");
    fgets(scan_string, sizeof(scan_string), stdin);
    if (!VALID_NUMERIC_INPUT(scan_string))
    {
      printf("\nInvalid response");
      break;
    }
    if (!( 0 == atoi(scan_string) || 1 == atoi(scan_string)))
    {
      printf("\nInvalid valvue %d \n",atoi(scan_string));
      break;
    }
    l2tp_enable_config.l2tp_state_enable = atoi(scan_string);

    if (l2tp_enable_config.l2tp_state_enable)
    {

      QCMAP_PRINTF_TAKE_INPUT("\n Please enter if TCP MSS to be clamped on L2TP interfaces"
           "to avoid Segmentation(1. Enable 0. Disable):");
      fgets(scan_string, sizeof(scan_string), stdin);
      if (!VALID_NUMERIC_INPUT(scan_string))
      {
        printf("\nInvalid response\n");
        break;
      }
      if (!( 0 == atoi(scan_string) || 1 == atoi(scan_string)))
      {
        printf("\nInvalid valvue %d \n",atoi(scan_string));
        break;
      }
      l2tp_enable_config.l2tp_mss_enable = atoi(scan_string);

      QCMAP_PRINTF_TAKE_INPUT("\n Please enter if MTU size to be set on l2tp interface"
             "to avoid fragmentation(1. Enable 0. Disable):");
      fgets(scan_string, sizeof(scan_string), stdin);
      if (!VALID_NUMERIC_INPUT(scan_string))
      {
        printf("\nInvalid response\n");
        break;
      }
      if (!( 0 == atoi(scan_string) || 1 == atoi(scan_string)))
      {
        printf("\nInvalid valvue %d \n",atoi(scan_string));
        break;
      }
      l2tp_enable_config.l2tp_mtu_enable = atoi(scan_string);
      if (l2tp_enable_config.l2tp_mtu_enable)
      {
        QCMAP_PRINTF_TAKE_INPUT("\n Please enter the MTU size to be set on l2tp interface");
        fgets(scan_string, sizeof(scan_string), stdin);
        if ( atoi(scan_string) < 0 )
        {
          printf("\nInvalid value for MTU size %d \n",atoi(scan_string));
          break;
        }
        l2tp_enable_config.mtu_size = atoi(scan_string);
      }
    }

    if (QcMapClient->SetUnmanagedL2TPStateEx(l2tp_enable_config,
                                            &qmi_err_num))
    {
      printf("\n Set L2TP Unmanaged state succeeds.");
    }
    else
    {
      if (qmi_err_num == QMI_ERR_NO_EFFECT_V01) {
        printf("\n L2TP Unmanaged state already set \n");
      } else {
     printf("\n Set L2TP Unmanaged state fails, Error: 0x%x \n", qmi_err_num);
      }
    }
  }
  break;
  case 13:
  {
    int temp_uint32 = 0;
    int num_sessions = 0;
    qcmap_msgr_l2tp_config_v01 l2tp_config;
    memset(&l2tp_config,0,sizeof(qcmap_msgr_l2tp_config_v01));

    QCMAP_PRINTF_TAKE_INPUT("\n Please input interface name on top of which L2TP Tunnel"
           " has to be created:");
    fgets(scan_string, sizeof(scan_string), stdin);
    if ( strlen(scan_string) == 0
         ||
         strlen( scan_string ) > QCMAP_MAX_IFACE_NAME_SIZE_V01)
    {
      printf("\nInvalid string length \n");
      break;
    }
    strlcpy(l2tp_config.local_iface,scan_string,strlen(scan_string));

    QCMAP_PRINTF_TAKE_INPUT("\n Please input local Tunnel ID:");
    fgets(scan_string, sizeof(scan_string), stdin);
    temp_uint32 = atoi(scan_string);
    if ( temp_uint32 > MAX_UINT32_VAL)
    {
      printf("\nInvalid value \n");
      break;
    }
    l2tp_config.local_tunnel_id = temp_uint32;

    QCMAP_PRINTF_TAKE_INPUT("\n Please input peer Tunnel ID:");
    fgets(scan_string, sizeof(scan_string), stdin);
    temp_uint32 = atoi(scan_string);
    if ( temp_uint32 > MAX_UINT32_VAL)
    {
      printf("\nInvalid value \n");
      break;
    }
    l2tp_config.peer_tunnel_id = temp_uint32;

    QCMAP_PRINTF_TAKE_INPUT("\n Please enter peer IP Version\n (4. v4 \n6. v6):");
    fgets(scan_string, sizeof(scan_string), stdin);
    temp_uint32 = atoi(scan_string);
    if ( temp_uint32 == 4)
    {
      l2tp_config.ip_family = QCMAP_MSGR_IP_FAMILY_V4_V01;
      printf("\nPlease input Peer IPv4 Addr [xxx.xxx.xxx.xxx]:");
      read_addr(AF_INET,(char *)&l2tp_config.peer_ipv4_addr);

    }
    else if(temp_uint32 == 6)
    {
      l2tp_config.ip_family = QCMAP_MSGR_IP_FAMILY_V6_V01;
      printf("\nPlease input Peer IPv6:");
      read_addr(AF_INET6,(char *)&l2tp_config.peer_ipv6_addr[0]);
    }
    else
    {
      printf("\nInvalid value \n");
      break;
    }

    QCMAP_PRINTF_TAKE_INPUT("\n Please input Encapsulation Protocol\n (%d. UDP \n%d. IP):",
           PS_IPPROTO_UDP, PS_IPPROTO_IP);
    fgets(scan_string, sizeof(scan_string), stdin);
    temp_uint32 = atoi(scan_string);
    if (temp_uint32 == PS_IPPROTO_IP)
    {
      l2tp_config.proto = QCMAP_MSGR_L2TP_ENCAP_IP_V01;
    }
    else if (temp_uint32 == PS_IPPROTO_UDP)
    {
      l2tp_config.proto = QCMAP_MSGR_L2TP_ENCAP_UDP_V01;
      printf("\nPlease input local UDP Port:");
      temp_uint32 = read_uint32();
      if (0 != check_port(temp_uint32))
      {
        printf("\nInvalid value \n");
        break;
      }
      l2tp_config.local_udp_port = temp_uint32;

      printf("\nPlease input peer UDP Port:");
      temp_uint32 = read_uint32();
      if (0 != check_port(temp_uint32))
      {
        printf("\nInvalid value \n");
        break;
      }
      l2tp_config.peer_udp_port = temp_uint32;
    }
    else
    {
      printf("\nInvalid protocol value entered %d \n",temp_uint32);
      break;
    }

    QCMAP_PRINTF_TAKE_INPUT("\n Please input number of session for this "
        "tunnel(Max allowed %d):",QCMAP_MSGR_L2TP_MAX_SESSION_PER_TUNNEL_V01);
    fgets(scan_string, sizeof(scan_string), stdin);
    temp_uint32 = atoi(scan_string);
    if (0 > temp_uint32
         ||
       temp_uint32 > QCMAP_MSGR_L2TP_MAX_SESSION_PER_TUNNEL_V01)
    {
      printf("\nInvalid value \n");
      break;
    }
    num_sessions = temp_uint32;
    for (int i = 0; i < num_sessions; i++)
    {
      QCMAP_PRINTF_TAKE_INPUT("\n Please input local Session ID for session %d:",i+1);
      fgets(scan_string, sizeof(scan_string), stdin);
      if (!VALID_NUMERIC_INPUT(scan_string))
      {
        printf("\nInvalid response\n");
        break;
      }
      temp_uint32 = atoi(scan_string);
      if (0 > temp_uint32 || temp_uint32 > MAX_UINT32_VAL)
      {
        printf("\nInvalid value \n");
        break;
      }
      l2tp_config.session_config[i].session_id = temp_uint32;

      QCMAP_PRINTF_TAKE_INPUT("\n Please input peer Session ID for session %d:",i+1);
      fgets(scan_string, sizeof(scan_string), stdin);
      temp_uint32 = atoi(scan_string);
      if (0 > temp_uint32 || temp_uint32 > MAX_UINT32_VAL)
      {
        printf("\nInvalid value \n");
        break;
      }
      l2tp_config.session_config[i].peer_session_id = temp_uint32;
    }

    if(QcMapClient->SetL2TPConfig(QCMAP_MSGR_L2TP_ROUTER_MODE_V01, l2tp_config, &qmi_err_num))
    {
      printf("\n Set L2TP Config succeeds.");
      break;
    }

    switch(qmi_err_num)
    {
      case(QMI_ERR_NO_EFFECT_V01):
      {
        printf("\n L2TP Config already set \n");
        break;
      }
      case(QMI_ERR_NOT_SUPPORTED_V01):
      {
        printf("\n L2TP config is not enabled, please enable L2TP config.\n");
        break;
      }
      case(QMI_ERR_INCOMPATIBLE_STATE_V01):
      {
        printf("\nL2TP config can not be enabled...\n"
               "Please map VLAN to default PDN first.\n");
        break;
      }
      default:
      {
        printf("\n Set L2TP Config fails, Error: 0x%x \n", qmi_err_num);
        break;
      }
    }
  }
  break;
  case 14:
  {
    qcmap_msgr_l2tp_conf_t l2tp_conf;
    char v6add_str[INET6_ADDRSTRLEN] = {0};
    char v4add_str[INET_ADDRSTRLEN] = {0};
    qmi_error_type_v01 qmi_err_num;
    memset(&l2tp_conf,0,sizeof(l2tp_conf));
    if (QcMapClient->GetL2TPConfig(&l2tp_conf,&qmi_err_num))
    {
      if (l2tp_conf.l2tp_mtu_config.enable)
      {
        printf("\n L2TP MTU Config is Enabled.\n");
        if (l2tp_conf.l2tp_mtu_size > 0)
        {
          printf("\n L2TP MTU size set is %d.\n",l2tp_conf.l2tp_mtu_size);
        }
      }
      else
      {
        printf("\n L2TP MTU Config is Disabled.\n");
      }

      if (l2tp_conf.l2tp_mss_config.enable)
      {
        printf("\n L2TP TCP MSS Config is Enabled.\n");
      }
      else
      {
        printf("\n L2TP TCP MSS Config is Disabled.\n");
      }

      if (l2tp_conf.l2tp_config_list_len == 0)
      {
        printf("\n NO L2TP Entries Configured\n");
      }
      else
      {
        printf("\n Current L2TP Tunnel Configuration");
        for (int i = 0; i < l2tp_conf.l2tp_config_list_len; i++)
        {
          printf("\nPhysical interface: %s",
                              l2tp_conf.l2tp_config_list[i].local_iface );
          printf("\nLocal Tunnel ID: %d",
                         l2tp_conf.l2tp_config_list[i].local_tunnel_id);
          printf("\nPeer Tunnel ID: %d",
                          l2tp_conf.l2tp_config_list[i].peer_tunnel_id);

          if (l2tp_conf.l2tp_config_list[i].ip_family ==
                                      QCMAP_MSGR_IP_FAMILY_V4_V01)
          {
            printf("\nIP Version: v4");
            inet_ntop(AF_INET,&l2tp_conf.l2tp_config_list[i].peer_ipv4_addr,
                      v4add_str,INET_ADDRSTRLEN);
             printf("\nPeer IPv4 Address: %s",v4add_str);
          }
          else if (l2tp_conf.l2tp_config_list[i].ip_family ==
                                    QCMAP_MSGR_IP_FAMILY_V6_V01)
          {
            printf("\nIP Version: v6");
            inet_ntop(AF_INET6,
                      (void *)&l2tp_conf.l2tp_config_list[i].peer_ipv6_addr,
                      v6add_str,INET6_ADDRSTRLEN);
            printf("\nPeer IPv6 Address: %s",v6add_str);
          }

          if (l2tp_conf.l2tp_config_list[i].proto ==
                QCMAP_MSGR_L2TP_ENCAP_UDP_V01)
          {
            printf("\nEncapsulation Protocol: UDP");
            if(0 == check_port(l2tp_conf.l2tp_config_list[i].local_udp_port))
            {
              printf("\nLocal UDP Port: %d",
                         l2tp_conf.l2tp_config_list[i].local_udp_port);
            }
            else
            {
              printf("\nInvalid local UDP prot received: %d",
                         l2tp_conf.l2tp_config_list[i].local_udp_port);
              break;
            }
            if(0 == check_port(l2tp_conf.l2tp_config_list[i].peer_udp_port))
            {
              printf("\nPeer UDP Port: %d",
                         l2tp_conf.l2tp_config_list[i].peer_udp_port);
            }
            else
            {
              printf("\nInvalid peer UDP prot received: %d",
                         l2tp_conf.l2tp_config_list[i].peer_udp_port);
              break;
            }
          }
          else if (l2tp_conf.l2tp_config_list[i].proto =
                  QCMAP_MSGR_L2TP_ENCAP_IP_V01)
          {
            printf("\nEncapsulation Protocol: IP");
          }
          else
          {
            printf("\nInvalid IP version received");
            break;
          }

          for (int j = 0; j < QCMAP_MSGR_L2TP_MAX_SESSION_PER_TUNNEL_V01; j++)
          {
            if (l2tp_conf.l2tp_config_list[i].session_config[j].session_id != 0)
            {
              printf("\nSession %d",j+1);
              printf("\n  Session ID: %d",
                   l2tp_conf.l2tp_config_list[i].session_config[j].session_id);
              printf("\n  Peer Session ID: %d\n",
                   l2tp_conf.l2tp_config_list[i].session_config[j].peer_session_id);
            }
          }/* End for int j */
        }/*end for int i*/
      }
    }
    else
    {
      if (qmi_err_num == QMI_ERR_NOT_SUPPORTED_V01)
      {
        printf("\n L2TP Unmanaged tunnel state is not enabled \n");
      }
      else
        printf("\n Get L2TP Config fails, Error: 0x%x \n", qmi_err_num);
    }
  }
  break;
  case 15:
  {
    qcmap_msgr_delete_l2tp_config_v01 l2tp_delete_config;
    int temp32 = 0;
    memset(&l2tp_delete_config,0,sizeof(qcmap_msgr_delete_l2tp_config_v01));

    QCMAP_PRINTF_TAKE_INPUT("\n Please input Tunnel ID to delete:");
    fgets(scan_string, sizeof(scan_string), stdin);
    temp32 = atoi(scan_string);
    if ( temp32 ==0 || temp32 > MAX_UINT32_VAL)
    {
      printf("\nInvalid value \n");
      break;
    }

    l2tp_delete_config.tunnel_id = temp32;
    if (QcMapClient->DeleteL2TPTunnelConfig(l2tp_delete_config,&qmi_err_num))
    {
      printf("\n Delete L2TP Config succeeds.");
    }
    else
    {
      if (qmi_err_num == QMI_ERR_NO_EFFECT_V01) {
        printf("\n Tunnel config is not found to delete. \n");
      } else {
      printf("\n Delete L2TP Config fails, Error: 0x%x \n", qmi_err_num);
      }
    }
  }
  break;
  case 16: //Select LAN Bridge Context
  {
    int16_t chosen_bridge = -1;
    if((chosen_bridge = ChooseLANBridge()) >= 0)
    {
      if(!QcMapClient->SelectLANBridge(chosen_bridge, &qmi_err_num))
      {
        if (qmi_err_num == QMI_ERR_INVALID_HANDLE_V01)
        {
          printf("\nMobileAP is not enabled");
          break;
        }
        printf("Failed to select LAN bridge: 0x%x", qmi_err_num);
      }
    } else {
      printf("Invalid LAN Bridge: %d", chosen_bridge);
    }
    break;
  }
  case 17: //Get Bridge-VLAN Context
  {
    qcmap_msgr_bridge_list_v01 bridge_list;
    if(QcMapClient->GetLANBridges(&bridge_list, &qmi_err_num))
    {
      if(bridge_list.num_of_bridges > 0)
      {
        printf("\n Current LAN Bridge is for VLAN ID %d \n",bridge_list.curr_bridge);
      }
      else
      {
        printf("\n No Bridges configured");
      }
    }
    else
      printf("\n Get LAN Bridges fails, Error: 0x%x \n",qmi_err_num);
  }
  break;

 /*Set Early Ethernet Mode*/
  case 18:
  {
    boolean early_eth_mode = false;
    QCMAP_PRINTF_TAKE_INPUT("   Please input EarlyEthernet Flag (1-Enable/0-Disable):");
    fgets(scan_string, sizeof(scan_string), stdin);
    if (!VALID_NUMERIC_INPUT(scan_string))
    {
      printf("\nInvalid response\n");
      break;
    }
    if (atoi(scan_string) == 0 || atoi(scan_string) == 1)
    {
      early_eth_mode = (atoi(scan_string)) ? true : false;
      if (QcMapClient->SetEarlyEthMode(early_eth_mode,&qmi_err_num))
      {
        printf("\nEarly Ethernet Mode %d config set succeeds",early_eth_mode);
      }
      else
      {
        if (qmi_err_num == QMI_ERR_NO_EFFECT_V01) {
          printf("\nEarly Ethernet Mode %d is already set",early_eth_mode);
        }
        else
          printf("\nEarly Ethernet Mode config set fails, Error: 0x%x",
               qmi_err_num);
      }
    }
    else
    {
      printf("\n   %s is invalid, please select a valid option\n",
             scan_string);
    }
  }
  break;

   /*get Early Ethernet Mode*/
  case 19:
  {
    boolean early_eth_mode = false;
    if (QcMapClient->GetEarlyEthMode(&early_eth_mode, &qmi_err_num))
    {
      printf("\nEarly Ethernet Mode: %s.\n",
                (early_eth_mode)?"Enabled":"Disabled");
    }
    else
    {
      printf("\nEarly Ethernet Mode config get fails, Error: 0x%x",
             qmi_err_num);
    }
  }
  break;

  /* Get SW IP Channel configuration parameters */
  case 20:
  {
    memset(&conf, 0, sizeof(qcmap_msgr_sw_ip_ch_config_v01));
    if (QcMapClient->GetSWIPChannelConfig(&conf, &qmi_err_num))
    {
      if (conf.if_name != NULL && conf.static_ip_addr != 0)
      {
        /* print IF name */
        printf("\nSW IP Channel Interface name: %s\n",conf.if_name);

        /* print Interface IP address */
        memset(&addr, 0, sizeof(in_addr));
        addr.s_addr = htonl(conf.static_ip_addr);
        printf("SW IP Channel Interface IP address: %s\n",inet_ntoa(addr));
      }
      else
      {
        printf(" SW IP config parameters are not present \n");
      }
    }
    else
    {
      printf("ERROR: Did not receive SW IP Channel config. Error code: 0x%x \n",qmi_err_num);
    }
  }
  break;

  /*Setup SW Channel Interface*/
  case 21:
  {
    QCMAP_PRINTF_TAKE_INPUT("   Please input logical IF(eg: mhi_swip0)");
    fgets(scan_string, sizeof(scan_string), stdin);
    if (strlen(scan_string)-1 >= QCMAP_MAX_IFACE_NAME_SIZE_V01)
    {
      printf("ERROR: Overflow..Supports only 16 char interface size\n");
      break;
    }

    for ( ix = 0; ix < strlen(scan_string)-1; ix++ )
    {
      /*interface name does not specical characters except - or _*/
     if (VALID_IF_NAME_CHAR(scan_string[ix]))
      {
        conf.if_name[ix] = scan_string[ix];
      }
      else
      {
        printf("ERROR: special char found in interface name\n");
        break;
      }
    }
    printf("The entered interface is %s\n", conf.if_name);

    QCMAP_PRINTF_TAKE_INPUT(" Do you wish to delete the configuration or Add a configuration? 1-Add/0-Del");
    fgets(scan_string, sizeof(scan_string), stdin);
    if (!VALID_NUMERIC_INPUT(scan_string))
    {
      printf("\nInvalid response\n");
      break;
    }
    if (atoi(scan_string) == 0 || atoi(scan_string)==1)
    {
      set_sw_ip_ch_flag = (atoi(scan_string)) ? true : false;
      if (set_sw_ip_ch_flag)
      {
        printf("   Please input ip address (xxx.xxx.xxx.xxx) for interface."
               "  (EX: 169.250.25.26): ");
        GET_IP_ADDR(scan_string,addr);
        conf.static_ip_addr = ntohl(addr.s_addr);
        printf("   Default netmask of 255.255.255.252 will be applied!\n");

        if (QcMapClient->SetSWIPChannelConfig(&conf, set_sw_ip_ch_flag, &qmi_err_num))
        {
          printf("SW IP channel is set up successfully");
        }
        else
        {
          printf("SW IP channel is set up failed, Error: 0x%x",
                 qmi_err_num);
        }
      }
      else
      {
        if (QcMapClient->SetSWIPChannelConfig(&conf, set_sw_ip_ch_flag, &qmi_err_num))
        {
          printf("Successfully deleted configuration parameters\n");
        }
        else
        {
          printf("Delete operation failed, Error: 0x%x",
                 qmi_err_num);
        }
      }
    }
    else
    {
      printf("\n %s is invalid, please select a valid option\n", scan_string);
    }
  }
  break;

  default:
  {
    printf("Invalid response %d\n", lanOpt);
  }
  break;
  }
}


void nat_alg_vpn_config( int natAlgOpt )
{
  qmi_error_type_v01 qmi_err_num;
  /* Initialize QMI Error Number. */
  qmi_err_num = QMI_ERR_NONE_V01;
  char scan_string[QCMAP_MSGR_MAX_FILE_PATH_LEN] = {0};
  in_addr addr;
  uint32 tmp_input=0;
  int i;
  boolean ipv6_nat_disabled = false;
  qcmap_msgr_snat_v6_entry_config_v01 snat_v6_entry;
  qcmap_msgr_snat_entry_config_v01 snat_entry;

  /* NAT/AG/VPN Configuration options */
  switch(natAlgOpt)
  {
    /* Add a static NAT entry and save XML if successful. */
    case 1:
    {
      memset(scan_string, 0, QCMAP_MSGR_MAX_FILE_PATH_LEN);
      in_addr tmp1, tmp2;

      inet_aton("0.0.0.0",&tmp1);
      inet_aton("255.255.255.255",&tmp2);
      memset(&snat_entry, 0, sizeof(qcmap_msgr_snat_entry_config_v01));
      while(TRUE)
      {
        QCMAP_PRINTF_TAKE_INPUT("Please input port_fwding_protocol (TCP=%d, UDP=%d,ICMP=%d,TCP_UDP=%d,ESP=%d): ",\
                                                PS_IPPROTO_TCP, PS_IPPROTO_UDP, PS_IPPROTO_ICMP,\
                                                PS_IPPROTO_TCP_UDP, PS_IPPROTO_ESP);
        fgets(scan_string, sizeof(scan_string), stdin);
        tmp_input = atoi(scan_string);
        if ( check_proto(tmp_input) == 0 )
        break;
      }
      snat_entry.protocol = (uint8)tmp_input;
      while (TRUE)
      {
        QCMAP_PRINTF_TAKE_INPUT("   Please input port_fwding_private_ip(xxx.xxx.xxx.xxx)   : ");
        memset(scan_string, 0, QCMAP_MSGR_MAX_FILE_PATH_LEN);
        if (fgets(scan_string, sizeof(scan_string), stdin) != NULL)
        {
            // check for owrt
           /**< fgets takes upto \n char in musl, hence setting last char to '\0' */
           scan_string[strlen(scan_string)-1]='\0';
           if ( !( inet_aton(scan_string, &addr) <= 0) &&
                ( addr.s_addr != tmp1.s_addr ) &&
                ( addr.s_addr != tmp2.s_addr ))
            break;
        }
        printf("   Invalid IPv4 address %s",scan_string);
      }
      snat_entry.private_ip_addr = ntohl(addr.s_addr);
      // for icmp and esp protocols, we don't need to enter source/dest ports
      if(!IS_NON_PORT_BASED_PROTO(snat_entry.protocol))
      {
        while (TRUE)
        {
          QCMAP_PRINTF_TAKE_INPUT("   Please input port_fwding_private_port : ");
          fgets(scan_string, sizeof(scan_string), stdin);
          tmp_input = atoi(scan_string);
          if(check_port (tmp_input) == 0 )
            break;
        }
        snat_entry.private_port = (uint16)tmp_input;
        while (TRUE)
        {
          QCMAP_PRINTF_TAKE_INPUT("   Please input port_fwding_global_port  : ");
          fgets(scan_string, sizeof(scan_string), stdin);
          tmp_input = atoi(scan_string);
          if(check_port (tmp_input) == 0 )
            break;
        }
        snat_entry.global_port = (uint16)tmp_input;
      }
    if (QcMapClient->AddStaticNatEntry(&snat_entry, &qmi_err_num))
    {
      printf("\nSNAT Entry added successfully");
    }
    else
    {
      if (qmi_err_num == QMI_ERR_INTERFACE_NOT_FOUND_V01)
        printf("\nBackhaul down, SNAT Entry added to xml file");
      else if(qmi_err_num == QMI_ERR_INVALID_HANDLE_V01)
        printf("\nMobileAP is not enabled\n");
      else
        printf("\nSNAT Entry add failed, Error: 0x%x", qmi_err_num);
    }
  }
  break;

  /* Delete a static NAT entry and save XML if successful. */
  case 2:
  {
    memset(scan_string, 0, QCMAP_MSGR_MAX_FILE_PATH_LEN);
    memset(&snat_entry, 0, sizeof(qcmap_msgr_snat_entry_config_v01));
    in_addr tmp1, tmp2;
    inet_aton("0.0.0.0",&tmp1);
    inet_aton("255.255.255.255",&tmp2);

    while(TRUE)
    {
      QCMAP_PRINTF_TAKE_INPUT("   Please input port_fwding_protocol   : ");
      fgets(scan_string, sizeof(scan_string), stdin);
      tmp_input = atoi(scan_string);
      if ( check_proto(tmp_input) == 0 )
      break;
    }
    snat_entry.protocol = (uint8)tmp_input;
    while (TRUE)
    {
      QCMAP_PRINTF_TAKE_INPUT("   Please input port_fwding_private_ip(xxx.xxx.xxx.xxx)   : ");
      memset(scan_string, 0, QCMAP_MSGR_MAX_FILE_PATH_LEN);
      if (fgets(scan_string, sizeof(scan_string), stdin) != NULL)
      {
        /**< fgets takes upto \n char in musl, hence setting last char to '\0' */
        scan_string[strlen(scan_string)-1]='\0';
        if ( !( inet_aton(scan_string, &addr) <= 0) &&
      ( addr.s_addr != tmp1.s_addr ) &&
      ( addr.s_addr != tmp2.s_addr ))
      break;
      }
      printf("   Invalid IPv4 address %s",scan_string);
    }
    snat_entry.private_ip_addr = ntohl(addr.s_addr);

// for icmp and esp protocols, we don't need to enter source/dest ports

    if(!IS_NON_PORT_BASED_PROTO(snat_entry.protocol))
    {
      while (TRUE)
      {
        QCMAP_PRINTF_TAKE_INPUT("   Please input port_fwding_private_port : ");
        fgets(scan_string, sizeof(scan_string), stdin);
        tmp_input = atoi(scan_string);
        if(check_port (tmp_input) == 0 )
          break;
      }
      snat_entry.private_port = (uint16)tmp_input;
      while (TRUE)
      {
        QCMAP_PRINTF_TAKE_INPUT("   Please input port_fwding_global_port  : ");
        fgets(scan_string, sizeof(scan_string), stdin);
        tmp_input = atoi(scan_string);
        if(check_port (tmp_input) == 0 )
          break;
      }
      snat_entry.global_port = (uint16)tmp_input;
    }

    if (QcMapClient->DeleteStaticNatEntry(&snat_entry, &qmi_err_num))
    {
      printf("\nSNAT Entry deleted successfully");
    }
    else
    {
      if (qmi_err_num == QMI_ERR_INTERFACE_NOT_FOUND_V01)
        printf("\nBackhaul down, SNAT Entry deleted from xml file.");
      else if(qmi_err_num == QMI_ERR_INVALID_HANDLE_V01)
        printf("\nMobileAP is not enabled\n");
      else
        printf("\nSNAT Entry delete failed, Error: 0x%x", qmi_err_num);
    }
  }
  break;

  /* Get SNAT config */
  case 3:
  {
    in_addr tmpIP;
    int i=0, num_entries=0;
    qcmap_msgr_snat_entry_config_v01 snat_config[QCMAP_MSGR_MAX_SNAT_ENTRIES_V01];

    memset(snat_config, 0, QCMAP_MSGR_MAX_SNAT_ENTRIES_V01*sizeof(qcmap_msgr_snat_entry_config_v01));

    if (QcMapClient->GetStaticNatConfig(snat_config, &num_entries, &qmi_err_num))
    {
      if(num_entries > 0)
      {
        for (i=0; i<num_entries; i++)
        {
          printf("\n\nEntry %d:",i);
          tmpIP.s_addr = ntohl(snat_config[i].private_ip_addr);
          printf("\nprivate ip: %s", inet_ntoa(tmpIP));
          printf("\nprotocol: %d", snat_config[i].protocol);
          if(!IS_NON_PORT_BASED_PROTO(snat_config[i].protocol))
          {
            printf("\nprivate port: %d", snat_config[i].private_port);
            printf("\nglobal port: %d", snat_config[i].global_port);
          }
        }
      }
      else
      {
        printf("\nNo SNAT Entries Configured");
      }
    }
    else
    {
      printf("\nSNAT Entries get failed  Error: 0x%x", qmi_err_num);
    }
  }
  break;

  /* Set NAT type */
  case 4:
  {
    int nat_type;
    memset(scan_string, 0, QCMAP_MSGR_MAX_FILE_PATH_LEN);

    QCMAP_PRINTF_TAKE_INPUT("Select the Type of NAT : \n"
            "0:SYMMETRIC NAT\n1: PORT RESTRICTED CONE NAT\n2: FULL CONE NAT\n"
            "3: ADDRESS RESTRICTED CONE NAT\n");
    fgets(scan_string, sizeof(scan_string), stdin);
    if (!VALID_NUMERIC_INPUT(scan_string))
    {
      printf("\nInvalid response\n");
      break;
    }
    nat_type = atoi(scan_string);
    if (nat_type < QCMAP_MSGR_NAT_SYMMETRIC_NAT_V01 ||
          nat_type > QCMAP_MSGR_NAT_ADDRESS_RESTRICTED_NAT_V01 )
    {
      printf("\nInvalid NAT Type : %d", nat_type);
      break;
    }
    if (QcMapClient->SetNatType((qcmap_msgr_nat_enum_v01)nat_type, &qmi_err_num))
    {
      if ( qmi_err_num == QMI_ERR_NO_EFFECT_V01 )
      {
        printf("\n NAT already set as desired type \n");
      }
      else
      {
        printf("\nNAT Type set successfully");
      }
    }
    else if(qmi_err_num == QMI_ERR_INVALID_HANDLE_V01)
    {
      printf("\nMobileAP is not enabled\n");
      break;
    }
    else
      printf("\nNAT Type set fail, Error: 0x%x", qmi_err_num);
    break;
  }

  /* Get NAT type */
  case 5:
  {
        qcmap_msgr_nat_enum_v01 nat_type;
        if (QcMapClient->GetNatType(&nat_type, &qmi_err_num))
        {
          switch (nat_type)
          {
            case QCMAP_MSGR_NAT_SYMMETRIC_NAT_V01:
              printf("\n Symmetric NAT \n");
              break;
            case QCMAP_MSGR_NAT_PORT_RESTRICTED_CONE_NAT_V01:
              printf("Port Restricted Cone NAT\n");
              break;
            case QCMAP_MSGR_NAT_FULL_CONE_NAT_V01:
              printf("\nFull Cone NAT\n");
              break;
            case QCMAP_MSGR_NAT_ADDRESS_RESTRICTED_NAT_V01:
              printf("Address Restricted Cone NAT\n");
              break;
            default:
              printf("Invalid NAT Type Returned: 0x%d", nat_type);
              break;
          }
        }
        else
        {
          if (qmi_err_num == QMI_ERR_INVALID_HANDLE_V01)
          {
            printf("\nMobileAP is not enabled\n");
            break;
          }
          printf("\nGet NAT type failed, Error: 0x%x", qmi_err_num);
        }
        break;
 }

  /* Set NAT Timeout. */
  case 6:
  { /* Timeout values are global, not tied to a PDN */
    qcmap_msgr_nat_timeout_enum_v01 timeout_type;
    int timeout_value = 0;
    int p_error=0;
    memset(scan_string, 0, QCMAP_MSGR_MAX_FILE_PATH_LEN);
    QCMAP_PRINTF_TAKE_INPUT("Select the Type of Timeout : \n"
           "1: GENRIC TIMEOUT\t2: ICMP TIMEOUT\n"
           "3: TCP TIMEOUT ESTABLISHED\t4: UDP TIMEOUT\t6: ICMPV6 TIMEOUT:::");
    fgets(scan_string, sizeof(scan_string), stdin);
    timeout_type = (qcmap_msgr_nat_timeout_enum_v01)atoi(scan_string);
    if ( timeout_type < QCMAP_MSGR_NAT_TIMEOUT_GENERIC_V01 ||
        timeout_type > QCMAP_MSGR_NAT_TIMEOUT_ICMPV6_V01 )
    {
      printf("\nInvalid NAT Timeout Type : %d", timeout_type);
      break;
    }
    QCMAP_PRINTF_TAKE_INPUT("\nEnter the Timeout Value (should be >= 30):::");
    READ_AND_VALIDATE_INT_VALUE(timeout_value, QCMAP_NAT_ENTRY_MIN_TIMEOUT, INT_MAX);

    if (QcMapClient->SetNatTimeout(timeout_type, (uint32)timeout_value, &qmi_err_num ))
    {
      printf("\nNAT Timeout Set Successfully\n");
    }
    else
    {
      if (qmi_err_num == QMI_ERR_INVALID_HANDLE_V01)
      {
        printf("\nMobileAP is not enabled\n");
        break;
      }
      printf("\nNAT timeout set fails, Error: 0x%x", qmi_err_num);
    }
    break;
  }

  /* Get SNAT Timeout */
  case 7:
  { /* Timeout values are global, not tied to a PDN */
    qcmap_msgr_nat_timeout_enum_v01 timeout_type;
    uint32 timeout_value = 0;
    int p_error=0;
    memset(scan_string, 0, QCMAP_MSGR_MAX_FILE_PATH_LEN);
    QCMAP_PRINTF_TAKE_INPUT("Select the Type of Timeout : \n"
           "1: GENRIC TIMEOUT\t2: ICMP TIMEOUT\n"
           "3: TCP TIMEOUT ESTABLISHED\t4: UDP TIMEOUT\t6: ICMPV6 TIMEOUT\t:::");
    fgets(scan_string, sizeof(scan_string), stdin);
    timeout_type = (qcmap_msgr_nat_timeout_enum_v01)atoi(scan_string);
    if ( timeout_type < QCMAP_MSGR_NAT_TIMEOUT_GENERIC_V01 ||
        timeout_type > QCMAP_MSGR_NAT_TIMEOUT_ICMPV6_V01 )
    {
      QCMAP_PRINTF_TAKE_INPUT("\n\nInvalid NAT Timeout Type : %d\n", timeout_type);
      break;
    }
    if (QcMapClient->GetNatTimeout(timeout_type, &timeout_value, &qmi_err_num))
    {
      QCMAP_PRINTF_TAKE_INPUT("\n\nNAT Timeout for Type %d : %d\n", timeout_type, timeout_value);
    }
    else
      QCMAP_PRINTF_TAKE_INPUT("\nNAT timeout get fails, Error: 0x%x", qmi_err_num);
    break;
  }
  /* Add a DMZ IP and save XML if successful. */
  case 8:
  {
    uint32 dmz_ip=0;
    memset(scan_string, 0, QCMAP_MSGR_MAX_FILE_PATH_LEN);
    while (TRUE)
    {
      QCMAP_PRINTF_TAKE_INPUT("   Please input DMZ IP to add(xxx.xxx.xxx.xxx) : ");
      if (fgets(scan_string, sizeof(scan_string), stdin) != NULL)
      {
        scan_string[strlen(scan_string)-1]='\0';
        if ( !(inet_aton(scan_string, &addr) <=0 ))
          break;
      }
      printf("   Invalid IPv4 address %s\n", scan_string);
    }
    dmz_ip = ntohl(addr.s_addr);
    if (QcMapClient->AddDMZ(dmz_ip, &qmi_err_num))
    {
      printf("\nDMZ IP added successfully");
    }
    else if (qmi_err_num == QMI_ERR_INVALID_HANDLE_V01)
    {
      printf("\nMobileAP is not enabled\n");
      break;
    }
    else if ( qmi_err_num == QMI_ERR_GENERAL_V01 )
    {
      printf(" DMZ is already configured. Delete the current configuration, if DMZ reconfiguration is needed \n");
    }
    else if ( qmi_err_num == QMI_ERR_INTERFACE_NOT_FOUND_V01 )
    {
      printf("\nBackhaul down. DMZ Entry deleted from xml file.");
    }
    else
      printf("\nDMZ add fails. Error: 0x%x", qmi_err_num);
  }
   break;

  /* Get DMZ IP */
  case 9:
  {
    uint32_t dmz_ip=0;
    in_addr tmpIP;
    if (QcMapClient->GetDMZ(&dmz_ip, &qmi_err_num))
    {
      if ( dmz_ip == 0 )
        printf("\nNo DMZ Configured!");
      else
      {
        tmpIP.s_addr = ntohl(dmz_ip);
        printf("\ndmz ip %s",inet_ntoa(tmpIP));
      }
    }
    else
      printf("\nDMZ get fails. Error: 0x%x", qmi_err_num);
  }
  break;

  /* Delete the current DMZ IP and save XML if successful. */
  case 10:
  {
    if (QcMapClient->DeleteDMZ(&qmi_err_num))
    {
      QCMAP_PRINTF_TAKE_INPUT("\nDMZ deleted successfully");
    }
    else
    {
      if (qmi_err_num == QMI_ERR_INTERFACE_NOT_FOUND_V01)
        printf("\nBackhaul down. DMZ deleted from xml file.");
      else if (qmi_err_num == QMI_ERR_INVALID_HANDLE_V01)
        printf("\nMobileAP is not enabled\n");
      else
        printf("\nDMZ delete fails. Error: 0x%x", qmi_err_num);
    }
  }
  break;

  /* Enable/disable the IPSEC VPN pass through. */
  case 11:
  {
    boolean enable;
    memset(scan_string, 0, QCMAP_MSGR_MAX_FILE_PATH_LEN);
    QCMAP_PRINTF_TAKE_INPUT("\n(Set IPSEC VPN Passthrough Flag: (1-Enable /0-Disable ): ");
    fgets(scan_string, sizeof(scan_string), stdin);
    if (!VALID_NUMERIC_INPUT(scan_string) || (atoi(scan_string) != 1 && atoi(scan_string) != 0))
    {
      printf("\nInvalid response. Please enter 1 or 0.\n");
      break;
    }
    enable = (atoi(scan_string)) ? true : false;

    if (QcMapClient->SetIPSECVpnPassthrough(enable, &qmi_err_num))
    {
      printf("\nIPSEC VPN passthrough set successfully");
    }
    else
    {
      if (qmi_err_num == QMI_ERR_INTERFACE_NOT_FOUND_V01)
      {
        printf("\nBackhaul down. IPSEC VPN passthrough enabled in xml file.");
      }
      else if(qmi_err_num == QMI_ERR_INVALID_HANDLE_V01)
      {
        printf("\nMobileAP is not enabled\n");
        break;
      }
      else
      {
        printf("\nIPSEC VPN passthrough set fails. Error: 0x%x", qmi_err_num);
      }
    }
  }
  break;

  /* Get IPSEC VPN Passthrough */
  case 12:
  {
    boolean flag;
    int p_error=0;

    if (QcMapClient->GetIPSECVpnPassthrough(&flag, &qmi_err_num))
    {
      printf("\nIPSEC Passthrough Enable Flag : %d", flag);
    }
    else
      printf("\nIPSEC VPN passthrough get fails. Error: 0x%x", qmi_err_num);
  }
   break;

  /* Enable/disable the PPTP VPN pass through. */
  case 13:
  {
    boolean enable;
    memset(scan_string, 0, QCMAP_MSGR_MAX_FILE_PATH_LEN);
    QCMAP_PRINTF_TAKE_INPUT("\n(Set PPTP VPN Passthrough Flag: (1-Enable /0-Disable ): ");
    fgets(scan_string, sizeof(scan_string), stdin);
    if (!VALID_NUMERIC_INPUT(scan_string) || (atoi(scan_string) != 1 && atoi(scan_string) != 0))
    {
      printf("\nInvalid response. Please enter 1 or 0.\n");
      break;
    }
    enable = (atoi(scan_string)) ? true : false;

    if (QcMapClient->SetPPTPVpnPassthrough(enable, &qmi_err_num))
    {
      printf("\nPPTP Passthrough set successfully");
    }
    else
    {
      if (qmi_err_num == QMI_ERR_INTERFACE_NOT_FOUND_V01)
      {
        printf("\nBackhaul down. PPTP VPN passthrough enabled in xml file.");
      }
      else if(qmi_err_num == QMI_ERR_INVALID_HANDLE_V01)
      {
        printf("\nMobileAP is not enabled\n");
        break;
      }
      else
      {
        printf("\nPPTP VPN passthrough set fails. Error: 0x%x", qmi_err_num);
      }
    }
  }
  break;

  /* Get PPTP passthrough */
  case 14:
  {
    boolean flag;
    int p_error=0;

    if (QcMapClient->GetPPTPVpnPassthrough(&flag, &qmi_err_num))
    {
      printf("\nPPTP Passthrough Enable Flag : %d", flag);
    }
    else
      printf("\nPPTP VPN passthrough get fails. Error: 0x%x", qmi_err_num);
  }
   break;

  /* Enable/disable the L2TP/IPSEC VPN pass through */
  case 15:
  {
    boolean enable;
    memset(scan_string, 0, QCMAP_MSGR_MAX_FILE_PATH_LEN);

    QCMAP_PRINTF_TAKE_INPUT("\n(Set L2TP VPN Passthrough Flag: (1-Enable /0-Disable ): ");
    fgets(scan_string, sizeof(scan_string), stdin);
    if (!VALID_NUMERIC_INPUT(scan_string) || (atoi(scan_string) != 1 && atoi(scan_string) != 0))
    {
      printf("\nInvalid response. Please enter 1 or 0.\n");
      break;
    }
    enable = (atoi(scan_string))? true : false;

    if (QcMapClient->SetL2TPIPSECVpnPassthrough(enable, &qmi_err_num))
    {
      printf("\nL2TP/IPSEC VPN Passthrough set successfully");
    }
    else
    {
      if (qmi_err_num == QMI_ERR_INTERFACE_NOT_FOUND_V01)
      {
        printf("\nBackhaul down. L2TP/IPSEC VPN passthrough enabled in xml file.");
      }
      else if(qmi_err_num == QMI_ERR_INVALID_HANDLE_V01)
      {
        printf("\nMobileAP is not enabled\n");
        break;
      }
      else
      {
        printf("\nL2TP/IPSEC VPN passthrough set fails. Error: 0x%x", qmi_err_num);
      }
    }
  }
    break;

  /* Get L2TP/IPSEC VPN passthrough */
  case 16:
  {
    boolean flag;
    int p_error=0;
    if (QcMapClient->GetL2TPIPSECVpnPassthrough(&flag, &qmi_err_num))
    {
      printf("\nL2TP/IPSEC Passthrough Enable Flag : %d", flag);
    }
    else
      printf("\nL2TP/IPSEC VPN passthrough get fails. Error: 0x%x", qmi_err_num);
  }
  break;

  /* Enable/Disable ALG. */
  case 17:
  {
   int alg_types = 0 , alg_type =0 ;
   memset(scan_string, 0, QCMAP_MSGR_MAX_FILE_PATH_LEN);
   //  Add more ALG in menu here
   QCMAP_PRINTF_TAKE_INPUT("Select the Type of ALG : \n"
           "1: RTSP ALG \n"
           "2: SIP ALG \n"
           "0: EXIT \n");
   fgets(scan_string, sizeof(scan_string), stdin);
   alg_type = atoi(scan_string);
   while(alg_type != 0)
   {
     // Edit if condition to support more ALGs
     if (alg_type < QCMAP_MSGR_MASK_RTSP_ALG_V01 ||
         alg_type > QCMAP_MSGR_MASK_SIP_ALG_V01)
     {
       printf("\nInvalid alg Type : 0x%x", alg_type);
       break;
     }
     if (alg_type == 1)
        alg_types = alg_types | QCMAP_MSGR_MASK_RTSP_ALG_V01;
     else if (alg_type == 2)
        alg_types = alg_types | QCMAP_MSGR_MASK_SIP_ALG_V01;
     QCMAP_PRINTF_TAKE_INPUT("Select the Type of ALG : \n"
             "1: RTSP ALG \n"
             "2: SIP ALG \n"
             "0: CONTINUE \n");
     fgets(scan_string, sizeof(scan_string), stdin);
     alg_type = atoi(scan_string);
   }
   if (alg_types == 0)
   {
     break;
   }
   QCMAP_PRINTF_TAKE_INPUT(" Please input ALG State (1-Enable/0-Disable) : ");
   fgets(scan_string, sizeof(scan_string), stdin);
   if (!VALID_NUMERIC_INPUT(scan_string) || (atoi(scan_string) != 1 && atoi(scan_string) != 0))
   {
      printf("\nInvalid response. Please enter 1 or 0.\n");
      break;
   }
   if ( atoi(scan_string) == 1 )
   {
     if ( QcMapClient->EnableAlg((qcmap_msgr_alg_type_mask_v01)alg_types,
                                  &qmi_err_num) )
       printf("\n ALGs 0x%x Enable succeeds.",alg_types);
     else
     {
       if (qmi_err_num == QMI_ERR_INTERFACE_NOT_FOUND_V01)
         printf("\nBackhaul down. ALG 0x%x enabled in xml file.",alg_types);
       else if (qmi_err_num == QMI_ERR_OP_PARTIAL_FAILURE_V01)
         printf("\n Only subset of ALGs enabled, Error:0x%x",qmi_err_num);
       else
         printf("\nALGs 0x%x Enable fails,Error:0x%x", alg_types,
                 qmi_err_num);
     }
   }
   else
   {
     if ( QcMapClient->DisableAlg((qcmap_msgr_alg_type_mask_v01)alg_types,
                                   &qmi_err_num) )
       printf("\nALG Disable Succeeds: 0x%x.", alg_types);
     else
     {
       if (qmi_err_num == QMI_ERR_INTERFACE_NOT_FOUND_V01)
         printf("\nBackhaul down. ALG 0x%x disabled in xml file.",alg_types);
       else if (qmi_err_num == QMI_ERR_OP_PARTIAL_FAILURE_V01)
         printf("\n Only subset of ALGs disabled, Error:0x%x",qmi_err_num);
       else
         printf("\nALGs 0x%x Disable fails,Error:0x%x", alg_types,
                 qmi_err_num);
     }
   }
   break;
  }

  /* Set SIP Server Information */
  case 18:
  {
   qcmap_msgr_sip_server_info_v01 sip_server_info;
   QCMAP_PRINTF_TAKE_INPUT("Please input 1-Enter PCSCF IP address 2-Enter PCSCF FQDN");
   fgets(scan_string, sizeof(scan_string), stdin);
   if (!VALID_NUMERIC_INPUT(scan_string))
   {
    printf("\nInvalid response\n");
    break;
   }
   if (atoi(scan_string)== 1)
   {
     memset(scan_string,0,QCMAP_MSGR_MAX_FILE_PATH_LEN);
     QCMAP_PRINTF_TAKE_INPUT("Please input the PCSCF IP(xxx.xxx.xxx.xxx)  :");
     memset(&addr,0,sizeof(in_addr));
     read_addr(AF_INET, (uint8 *)&addr.s_addr);

     sip_server_info.pcscf_ip_addr = addr.s_addr;
     sip_server_info.pcscf_info_type = QCMAP_MSGR_PCSCF_IP_ADDRESS_V01;
   }
   else if (atoi(scan_string) == 2)
   {
     memset(scan_string,0,QCMAP_MSGR_MAX_FILE_PATH_LEN);
     QCMAP_PRINTF_TAKE_INPUT("Please input the PCSCF FQDN:");
     if (fgets(scan_string,sizeof(scan_string),stdin) != NULL)
     {
       for (i=0;i < strlen(scan_string)-1;i++)
       {
         sip_server_info.pcscf_fqdn[i] = scan_string[i];
       }
       sip_server_info.pcscf_fqdn[i] ='\0';
     }

     sip_server_info.pcscf_info_type = QCMAP_MSGR_PCSCF_FQDN_V01;
   }
   else
   {
    printf("\nInvalid response. Please enter 1 or 2.\n");
    break;
   }

   if(QcMapClient->SetSIPServerInfo(&sip_server_info, &qmi_err_num))
   {
     printf("\n Successfully set SIP server information \n");
   }
   else if(qmi_err_num == QMI_ERR_INVALID_HANDLE_V01)
   {
     printf("\nMobileAP is not enabled\n");
     break;
   }
   else
   {
     printf("\n Failed to set SIP server information. Error:0x%x \n",
            qmi_err_num);

     break;
  }

  /* Get SIP server Information */
  case 19:
  {
   qcmap_msgr_sip_server_info_v01 default_sip_server_info;
   qcmap_msgr_sip_server_info_v01
   network_sip_server_info[QCMAP_MSGR_MAX_SIP_SERVER_ENTRIES_V01];
   qcmap_msgr_ipv6_sip_server_info_v01
   network_ipv6_sip_server_info[QCMAP_MSGR_MAX_SIP_SERVER_ENTRIES_V01];
   qcmap_msgr_ip_family_enum_v01 ip_family;
   int count_network_sip_server_info=0;
   int cnt;
   in_addr addr;
   char ipv6_addr_buf[INET6_ADDRSTRLEN];

   memset(&default_sip_server_info, 0, sizeof(default_sip_server_info));
   memset(network_sip_server_info, 0, sizeof(network_sip_server_info));
   memset(network_ipv6_sip_server_info, 0, sizeof(network_ipv6_sip_server_info));

   QCMAP_PRINTF_TAKE_INPUT("Please input IP Family for SIP server info IPV4-4 IPV6-6 : ");
   fgets(scan_string, sizeof(scan_string), stdin);
   ip_family = (qcmap_msgr_ip_family_enum_v01)atoi(scan_string);

   if (ip_family == QCMAP_MSGR_IP_FAMILY_V4_V01)
   {

     if(QcMapClient->GetSIPServerInfo(&default_sip_server_info,
                                      network_sip_server_info,
                                      &count_network_sip_server_info,
                                      &qmi_err_num))
     {
       if (default_sip_server_info.pcscf_info_type != 0)
       {
         if (default_sip_server_info.pcscf_info_type ==
                                        QCMAP_MSGR_PCSCF_IP_ADDRESS_V01)
         {
           addr.s_addr =default_sip_server_info.pcscf_ip_addr;
           printf("\nDefault PCSCF address: %s \n",inet_ntoa(addr));
         }
         else if (default_sip_server_info.pcscf_info_type ==
                                              QCMAP_MSGR_PCSCF_FQDN_V01)
         {
           printf("\n PCSCF FQDN is %s:\n", default_sip_server_info.pcscf_fqdn);
         }
       }

       if (count_network_sip_server_info > 0)
       {
         printf("\n Number of network assigned SIP server info is %d \n",
                count_network_sip_server_info);

         for (cnt=0; cnt<count_network_sip_server_info; cnt++)
         {
           if (network_sip_server_info[cnt].pcscf_info_type ==
               QCMAP_MSGR_PCSCF_IP_ADDRESS_V01)
           {
             addr.s_addr =network_sip_server_info[cnt].pcscf_ip_addr;
             printf("\%d PCSCF address: %s \n",cnt+1, inet_ntoa(addr));
           }
           else if (network_sip_server_info[cnt].pcscf_info_type ==
                    QCMAP_MSGR_PCSCF_FQDN_V01)
           {
             printf("\n %d PCSCF FQDN is %s:\n",
                    cnt+1,
                    network_sip_server_info[cnt].pcscf_fqdn);
           }
         }
       }
     }
     else
     {
       printf("\n Failed to get IPV4 SIP server info. Error:0x%x \n",
              qmi_err_num);
     }
   }

   else if (ip_family == QCMAP_MSGR_IP_FAMILY_V6_V01)
   {
     if(QcMapClient->GetV6SIPServerInfo(network_ipv6_sip_server_info,
                                        &count_network_sip_server_info,
                                        &qmi_err_num))
     {

       if (count_network_sip_server_info > 0)
       {
         printf("\n Number of network assigned SIP server info is %d \n",
                count_network_sip_server_info);

         for (cnt=0; cnt<count_network_sip_server_info; cnt++)
         {
           if (network_ipv6_sip_server_info[cnt].pcscf_info_type ==
               QCMAP_MSGR_PCSCF_IP_ADDRESS_V01)
           {
             char* ipv6_addr;
             if ( (ipv6_addr = inet_ntop(AF_INET6,
                      (in6_addr *)&network_ipv6_sip_server_info[cnt].pcscf_ipv6_addr,
                      ipv6_addr_buf,sizeof(ipv6_addr_buf))) != NULL)
             {
               printf("\%d IPV6 PCSCF address: %s \n", cnt+1, ipv6_addr);
             }
             else
             {
               printf ("Unable to get IPV6 PCSCF address");
             }
           }
           else if (network_ipv6_sip_server_info[cnt].pcscf_info_type ==
                    QCMAP_MSGR_PCSCF_FQDN_V01)
           {
             printf("\n %d PCSCF FQDN is %s:\n",
                    cnt+1,
                    network_ipv6_sip_server_info[cnt].pcscf_fqdn);
           }
         }
       }
     }
     else
     {
       printf("\n Failed to get IPV6 SIP server info. Error:0x%x \n",
              qmi_err_num);
     }
   }

   else
   {
     printf("\n Failed to get SIP server info: Invalid family\n");
   }
   break;
  }

  case 20:
  {
    int pkt_limit =0;
    while(TRUE)
    {
      QCMAP_PRINTF_TAKE_INPUT("Please enter the valid initial packet limit: ");
      fgets(scan_string, sizeof(scan_string), stdin);
      if (atoi(scan_string) < SW_PACKET_THRESHOLD)
      {
        printf("Value less than %d not allowed \n",SW_PACKET_THRESHOLD );
        continue;
      }
      break;
    }
    pkt_limit = atoi(scan_string);
    if(QcMapClient->SetInitialPacketLimit(pkt_limit, &qmi_err_num))
      printf("\n Successfully set initial packet threshold \n");
    else
      printf("\n Failed to set initial packet threshold. Error:0x%x \n",qmi_err_num);

    break;
  }
  case 21:
  {
    int pkt_limit = 0;
    if(QcMapClient->GetInitialPacketLimit(&pkt_limit, &qmi_err_num))
    {
      printf("\n The current initial packet threshold is %d \n",pkt_limit );
    }
    else
      printf("\n Failed to set initial packet threshold. Error:0x%x \n", qmi_err_num);

    break;
  }

  /* Enable/Disable the SOCKSv5 Proxy and save XML if successful. */
  case 22:
  {
    if(!(EnableDisableSOCKSv5(&qmi_err_num)))
    {
      printf("\n Failed to EnableDisable SOCKSv5. Error:0x%x\n", qmi_err_num);
    }
    break;
  }

  /* Set the SOCKSv5 Proxy Config params and save config if successful. */
  case 23:
  {
    if(!(SetSOCKSv5Config(&qmi_err_num)))
    {
      printf("\n Failed to Set SOCKSv5 Config. Error:0x%x\n", qmi_err_num);
    }
    break;
  }

  case 24:
  {
    socksv5_configuration configuration;

    memset(&configuration, 0, sizeof(socksv5_configuration));

    if(!(QcMapClient->GetSOCKSv5Config(&configuration, &qmi_err_num)))
    {
      printf("\nFailed to print SOCKSv5 config, Error: 0x%x", qmi_err_num);
    } else
    {
      //print configuration file locations
      printf("Conf File Location: %s\n", configuration.config_file_paths.conf_file);
      printf("Auth File Location: %s\n", configuration.config_file_paths.auth_file);

      //print auth method
      switch(configuration.auth_method)
      {
        case QCMAP_SOCKSV5_NO_AUTHENTICATION_V01:
        {
          printf("SOCKSv5 Authentication Method: No Authentication\n");
          break;
        }
        case QCMAP_SOCKSV5_UNAME_PASSWD_V01:
        {
          printf("SOCKSv5 Authentication Method: Username / Password\n");
          break;
        }
        default:
        {
          printf("SOCKSv5 Authentication Method: %u\n", configuration.auth_method);
          break;
        }
      }

      //print lan_iface
      printf("SOCKSv5 LAN Interface: %s\n", configuration.lan_iface);

      //print wan_ifaces with corresponding profile/service no
      for(int i = 0; i < QCMAP_MAX_NUM_BACKHAULS_V01; i++)
      {
        //if "" string then no wan_iface put into the wan_service struct
        if(strcmp(configuration.wan_service[i].wan_iface, "") != 0)
        {
          printf("SOCKSv5 WAN Interface: %s, Service/Profile#: %u\n",
          configuration.wan_service[i].wan_iface,
          configuration.wan_service[i].service_no);
        }
      }
    }
    break;
  }
     /* Enable/disable IPv6 NAT */
  case 25:
  {
    boolean ipv6_nat_status;
    qcmap_msgr_nat_enum_v01 v6_nat_type = 0;
    memset(scan_string, 0, QCMAP_MSGR_MAX_FILE_PATH_LEN);

    QCMAP_PRINTF_TAKE_INPUT("   Please input IPv6 NAT status(1-Enable/0-Disable):\n");
    fgets(scan_string, sizeof(scan_string), stdin);
    if (!VALID_NUMERIC_INPUT(scan_string) || (atoi(scan_string) != 1 && atoi(scan_string) != 0))
    {
      printf("\nInvalid response. Please enter 1 or 0.\n");
      break;
    }

    ipv6_nat_status = (atoi(scan_string)) ? true : false;
    if (ipv6_nat_status)
    {
      QCMAP_PRINTF_TAKE_INPUT("Select the Type of NAT : \n"
             "0:SYMMETRIC NAT\n");
      fgets(scan_string, sizeof(scan_string), stdin);
      v6_nat_type = atoi(scan_string);
      if ( v6_nat_type != QCMAP_MSGR_NAT_SYMMETRIC_NAT_V01)
      {
        printf("\nInvalid NAT Type : %d", v6_nat_type);
        break;
      }
    }

    if (QcMapClient->SetIPv6NAT(ipv6_nat_status,v6_nat_type,
                                &qmi_err_num))
    {
      printf("\nIPv6 nat set successfully");
    }
    else
    {
      if (qmi_err_num == QMI_ERR_INCOMPATIBLE_STATE_V01)
      {
        printf("Please tear down existing IPv6 backhaul call or"
               "Disable the firewall"
               "and retry setting IPv6 NAT");
      }
      else
      {
        printf("\nIPv6 NAT set Error: 0x%x", qmi_err_num);
      }
    }
  }
  break;

  /* Get IPv6 NAT status */
  case 26:
  {
    boolean flag = false;
    if (QcMapClient->GetIPv6NAT(&flag,&qmi_err_num))
    {
      printf("\nIPv6 NAT is %d",flag);
    }
    else
    {
      printf("\nIPv6 NAT failed");
    }
    break;
  }

  /* Add a static NAT v6 entry and save XML if successful. */
  case 27:
  {
    memset(scan_string, 0, QCMAP_MSGR_MAX_FILE_PATH_LEN);

    memset(&snat_v6_entry, 0, sizeof(qcmap_msgr_snat_v6_entry_config_v01));

    while(TRUE)
    {
      QCMAP_PRINTF_TAKE_INPUT("Please input port_fwding_protocol (TCP=%d, UDP=%d,ICMP=%d,TCP_UDP=%d,ESP=%d): ",\
                                              PS_IPPROTO_TCP, PS_IPPROTO_UDP, PS_IPPROTO_ICMP6,\
                                              PS_IPPROTO_TCP_UDP, PS_IPPROTO_ESP);
      fgets(scan_string, sizeof(scan_string), stdin);
      tmp_input = atoi(scan_string);
      if ( check_proto(tmp_input) == 0 )
      break;
    }
    snat_v6_entry.protocol = (uint8)tmp_input;
    while (TRUE)
    {
      printf("   Please input port_fwding_private_ip(xxx.xxx.xxx.xxx)   : ");
      if(read_addr(AF_INET6,(uint8 *)&snat_v6_entry.port_fwding_private_ip6_addr) == 0)
        break;
    }
    // for icmp6 and esp protocols, we don't need to enter source/dest ports
    if(!IS_NON_PORT_BASED_PROTO(snat_v6_entry.protocol))
    {
      while (TRUE)
      {
        QCMAP_PRINTF_TAKE_INPUT("   Please input port_fwding_private_port : ");
        fgets(scan_string, sizeof(scan_string), stdin);
        tmp_input = atoi(scan_string);
        if(check_port (tmp_input) == 0 )
          break;
      }
      snat_v6_entry.private_port = (uint16)tmp_input;
      while (TRUE)
      {
        QCMAP_PRINTF_TAKE_INPUT("   Please input port_fwding_global_port  : ");
        fgets(scan_string, sizeof(scan_string), stdin);
        tmp_input = atoi(scan_string);
        if(check_port (tmp_input) == 0 )
          break;
      }
      snat_v6_entry.global_port = (uint16)tmp_input;
    }

    if (QcMapClient->AddStaticNatEntry_Ipv6(&snat_v6_entry,&qmi_err_num))
    {
      printf("\nSNAT Entry added successfully");
    }
    else
    {
      printf("\nSNAT Entry faILED");
      if (qmi_err_num == QMI_ERR_INTERFACE_NOT_FOUND_V01)
      {
        printf("\nBackhaul down, SNAT v6 Entry added to xml file");
      }
      else
      {
        printf("\nSNAT Entry add failed, Error: 0x%x", qmi_err_num);
      }
    }
  }
  break;

  /* Delete a static NAT V6 entry and save XML if successful. */
  case 28:
  {
    memset(scan_string, 0, QCMAP_MSGR_MAX_FILE_PATH_LEN);
    memset(&snat_v6_entry, 0, sizeof(qcmap_msgr_snat_v6_entry_config_v01));
    in_addr tmp1, tmp2;
    inet_aton("0.0.0.0",&tmp1);
    inet_aton("255.255.255.255",&tmp2);

    while(TRUE)
    {
      QCMAP_PRINTF_TAKE_INPUT("   Please input port_fwding_protocol   : ");
      fgets(scan_string, sizeof(scan_string), stdin);
      tmp_input = atoi(scan_string);
      if ( check_proto(tmp_input) == 0 )
      break;
    }
    snat_v6_entry.protocol = (uint8)tmp_input;

    while (TRUE)
    {
      read_addr(AF_INET6,(uint8 *)&snat_v6_entry.port_fwding_private_ip6_addr);
      break;
    }

    // for icmp6 and esp protocols, we don't need to enter source/dest ports
    if(!IS_NON_PORT_BASED_PROTO(snat_v6_entry.protocol))
    {
      while (TRUE)
      {
        QCMAP_PRINTF_TAKE_INPUT("   Please input port_fwding_private_port : ");
        fgets(scan_string, sizeof(scan_string), stdin);
        tmp_input = atoi(scan_string);
        if(check_port (tmp_input) == 0 )
          break;
      }
      snat_v6_entry.private_port = (uint16)tmp_input;
      while (TRUE)
      {
        QCMAP_PRINTF_TAKE_INPUT("   Please input port_fwding_global_port  : ");
        fgets(scan_string, sizeof(scan_string), stdin);
        tmp_input = atoi(scan_string);
        if(check_port (tmp_input) == 0 )
          break;
      }
      snat_v6_entry.global_port = (uint16)tmp_input;
    }

    if (QcMapClient->DeleteStaticNatEntry_Ipv6(&snat_v6_entry,
                                               &qmi_err_num))
    {
      printf("\nSNAT Entry deleted successfully");
    }
    else
    {
      if (qmi_err_num == QMI_ERR_INTERFACE_NOT_FOUND_V01)
        printf("\nBackhaul down, SNAT V6 Entry deleted from xml file.");
      else
        printf("\nSNAT V6 Entry delete failed, Error: 0x%x", qmi_err_num);
    }
  }
  break;

  /* Get SNAT v6 config */
  case 29:
  {
    in_addr tmpIP;
    int i=0, num_entries=0;
    qcmap_msgr_snat_v6_entry_config_v01 snat_v6_config[QCMAP_MSGR_MAX_SNAT_ENTRIES_V01];
    char disp_str[INET6_ADDRSTRLEN];

    memset(snat_v6_config, 0,
           QCMAP_MSGR_MAX_SNAT_ENTRIES_V01*sizeof(qcmap_msgr_snat_v6_entry_config_v01));

    if (QcMapClient->GetStaticNatConfig_Ipv6(snat_v6_config, &num_entries,&qmi_err_num))
    {
      if(num_entries > 0)
      {
        for (i=0; i<num_entries; i++)
        {
          printf("\n\nEntry %d:",i);
          if(inet_ntop(AF_INET6,snat_v6_config[i].port_fwding_private_ip6_addr,
                        disp_str,sizeof(disp_str))!= NULL)
          {
            printf("\private V6 ip: %s",disp_str);
          }
          printf("\nprotocol: %d", snat_v6_config[i].protocol);
          if(!IS_NON_PORT_BASED_PROTO(snat_v6_config[i].protocol))
          {
            printf("\nprivate port: %d", snat_v6_config[i].private_port);
            printf("\nglobal port: %d", snat_v6_config[i].global_port);
          }
        }
      }
      else
      {
        printf("\nNo SNAT v6 Entries Configured");
      }
    }
    else
    {
      printf("\nSNAT v6 Entries get failed  Error: 0x%x", qmi_err_num);
    }
  }
  break;

  /* Add a DMZ v6 IP and save XML if successful. */
  case 30:
  {
    struct in6_addr dmz_ip;

    memset(scan_string, 0, QCMAP_MSGR_MAX_FILE_PATH_LEN);
    read_addr(AF_INET6,(uint8 *)&dmz_ip.s6_addr);

    if (QcMapClient->AddDMZ_Ipv6(dmz_ip,&qmi_err_num))
    {
      printf("\nDMZ v6 IP added successfully");
    }
    else
    {
      if ( qmi_err_num == QMI_ERR_NO_EFFECT_V01 )
      {
        printf(" DMZ v6 is already configured. Delete the current configuration, if DMZ reconfiguration is needed \n");
      }
      else if ( qmi_err_num == QMI_ERR_INTERFACE_NOT_FOUND_V01 )
      {
        printf("\nBackhaul down. DMZ v6 Entry deleted from xml file.");
      }
      else
      {
        printf("\nDMZ v6 add fails. Error: 0x%x", qmi_err_num);
      }
    }
  }
  break;

  /* Get DMZ V6 IP */
  case 31:
  {
    struct in6_addr dmz_ip;
    struct in6_addr zero_buff;
    char str[INET6_ADDRSTRLEN] = {0};

    memset(&zero_buff,0,sizeof(struct in6_addr));
    if (QcMapClient->GetDMZ_Ipv6(&dmz_ip,&qmi_err_num))
    {
      if (memcmp(&zero_buff,&dmz_ip,sizeof(struct in6_addr) == 0))
      {
        printf("\nNo DMZ v6 Configured!");
      }
      else
      {
        if(inet_ntop(AF_INET6,&dmz_ip,str,sizeof(str))!= NULL)
        {
          printf("\ndmz V6 ip: %s",str);
        }
      }
    }
    else
    {
      printf("\nDMZ get fails. Error: 0x%x", qmi_err_num);
    }
  }
  break;

  /* Delete the current DMZ IPv6 and save XML if successful. */
  case 32:
  {
    if (QcMapClient->DeleteDMZ_Ipv6(&qmi_err_num))
    {
      printf("\nDMZ deleted successfully");
    }
    else
    {
      if (qmi_err_num == QMI_ERR_INTERFACE_NOT_FOUND_V01)
        printf("\nBackhaul down. DMZ v6 deleted from xml file.");
      else
        printf("\nDMZ delete fails. Error: 0x%x", qmi_err_num);
    }
  }
  break;

   /* Enable/disable the IPSEC VPN pass through. */
  case 33:
  {
    boolean enable;
    memset(scan_string, 0, QCMAP_MSGR_MAX_FILE_PATH_LEN);
    QCMAP_PRINTF_TAKE_INPUT("\n(Set IPV6 IPSEC VPN Passthrough Flag: (1-Enable /0-Disable ): ");
    fgets(scan_string, sizeof(scan_string), stdin);
    if (!VALID_NUMERIC_INPUT(scan_string) || (atoi(scan_string) != 1 && atoi(scan_string) != 0))
    {
      printf("\nInvalid response. Please enter 1 or 0.\n");
      break;
    }
    enable = (atoi(scan_string)) ? true : false;

    if (QcMapClient->SetIPSECVpnPassthrough_Ipv6(enable,&qmi_err_num))
    {
      printf("\nIPSEC  VPN V6 passthrough set successfully");
    }
    else
    {
      if (qmi_err_num == QMI_ERR_INTERFACE_NOT_FOUND_V01)
      {
        printf("\nBackhaul down. IPSEC VPN V6 passthrough enabled in xml file.");
      }
      else if(qmi_err_num == QMI_ERR_INVALID_HANDLE_V01)
      {
        printf("\nMobileAP is not enabled\n");
        break;
      }
      else
      {
        printf("\nIPSEC VPN V6 passthrough set fails. Error: 0x%x", qmi_err_num);
      }
    }
  }
  break;

  /* Get IPv6 IPSEC VPN Passthrough */
  case 34:
  {
    boolean flag;
    int p_error=0;
    if (QcMapClient->GetIPSECVpnPassthrough_Ipv6(&flag,&qmi_err_num))
    {
      printf("\nIPSEC V6 Passthrough Enable Flag : %d", flag);
    }
    else
    {
      printf("\nIPSEC VPN V6 passthrough get fails. Error: 0x%x", qmi_err_num);
    }
  }
  break;

  /* Enable/disable the IPV6 PPTP VPN pass through. */
  case 35:
  {
    boolean enable;
    memset(scan_string, 0, QCMAP_MSGR_MAX_FILE_PATH_LEN);
    QCMAP_PRINTF_TAKE_INPUT("\n(Set IPV6 PPTP VPN Passthrough Flag: (1-Enable /0-Disable ): ");
    fgets(scan_string, sizeof(scan_string), stdin);
    if (!VALID_NUMERIC_INPUT(scan_string) || (atoi(scan_string) != 1 && atoi(scan_string) != 0))
    {
      printf("\nInvalid response. Please enter 1 or 0.\n");
      break;
    }
    enable = (atoi(scan_string)) ? true : false;

    if (QcMapClient->SetPPTPVpnPassthrough_Ipv6(enable,&qmi_err_num))
    {
      printf("\nIPv6 PPTP Passthrough set successfully");
    }
    else
    {
      if (qmi_err_num == QMI_ERR_INTERFACE_NOT_FOUND_V01)
      {
        printf("\nBackhaul down. IPv6 PPTP VPN passthrough enabled in xml file.");
      }
      else if(qmi_err_num == QMI_ERR_INVALID_HANDLE_V01)
      {
        printf("\nMobileAP is not enabled\n");
        break;
      }
      else
      {
        printf("\nIPv6 PPTP VPN passthrough set fails. Error: 0x%x", qmi_err_num);
      }
    }
  break;
  }


  /* Get IPV6 PPTP passthrough */
  case 36:
  {
    boolean flag;
    int p_error=0;

    if (QcMapClient->GetPPTPVpnPassthrough_Ipv6(&flag,&qmi_err_num))
    {
      printf("\nIPv6 PPTP Passthrough Enable Flag : %d", flag);
    }
    else
    {
      printf("\nIPv6 PPTP VPN passthrough get fails. Error: 0x%x", qmi_err_num);
    }

  }
   break;

  /* Enable/disable the IPV6 L2TP/IPSEC VPN pass through */
  case 37:
  {
    boolean enable;
    memset(scan_string, 0, QCMAP_MSGR_MAX_FILE_PATH_LEN);

    QCMAP_PRINTF_TAKE_INPUT("\n(Set IPV6 L2TP VPN Passthrough Flag: (1-Enable /0-Disable ): ");
    fgets(scan_string, sizeof(scan_string), stdin);
    if (!VALID_NUMERIC_INPUT(scan_string) || (atoi(scan_string) != 1 && atoi(scan_string) != 0))
    {
      printf("\nInvalid response. Please enter 1 or 0.\n");
      break;
    }
    enable = (atoi(scan_string))? true : false;

    if (QcMapClient->SetL2TPIPSECVpnPassthrough_Ipv6(enable,&qmi_err_num))
    {
      printf("\nIPv6 L2TP/IPSEC VPN Passthrough set successfully");
    }
    else
    {
      if (qmi_err_num == QMI_ERR_INTERFACE_NOT_FOUND_V01)
      {
        printf("\nBackhaul down. IPv6 L2TP/IPSEC VPN passthrough enabled in xml file.");
      }
      else if(qmi_err_num == QMI_ERR_INVALID_HANDLE_V01)
      {
        printf("\nMobileAP is not enabled\n");
        break;
      }
      else
      {
        printf("\nIPv6 L2TP/IPSEC VPN passthrough set fails. Error: 0x%x", qmi_err_num);
      }
    }
  }
  break;

  /* Get IPV6 L2TP/IPSEC VPN passthrough */
  case 38:
  {
    boolean flag;
    int p_error=0;

    if (QcMapClient->GetL2TPIPSECVpnPassthrough_Ipv6(&flag,&qmi_err_num))
    {
      printf("\nIPv6 L2TP/IPSEC Passthrough Enable Flag : %d", flag);
    }
    else
    {
      printf("\nIPv6 L2TP/IPSEC VPN passthrough get fails. Error: 0x%x", qmi_err_num);
    }
  }
  break;

#ifdef FEATURE_PORT_TRIGGER
  /* Add Port Trigger Entry */
  case 39:
  {
    int val, handle;
    qcmap_msgr_port_trigger_entry_conf_t add_port_trigger_entry;
    memset(&add_port_trigger_entry, 0, sizeof(add_port_trigger_entry));
    QCMAP_PRINTF_TAKE_INPUT("\nDo you want to add the port trigger entry or delete: (1-add/0-delete)");

    if (read_uint32() == 1)
    {
      QCMAP_PRINTF_TAKE_INPUT("\nPlease enter the Trigger Port Range:");
      read_trigger_start_port:
      QCMAP_PRINTF_TAKE_INPUT("\nEnter the start port: ");
      val = read_uint32();
      if ( check_port (val) == -1)
        goto read_trigger_start_port;
      add_port_trigger_entry.trigger_start_port = val;

      read_trigger_end_port:
      QCMAP_PRINTF_TAKE_INPUT("\nEnter the end port: ");
      val = read_uint32();
      if ( check_port (val) == -1)
        goto read_trigger_end_port;
      add_port_trigger_entry.trigger_end_port = val;

      QCMAP_PRINTF_TAKE_INPUT("\nPlease enter the Forward Port Range:");
      read_forward_start_port:
      QCMAP_PRINTF_TAKE_INPUT("\nEnter the start port: ");
      val = read_uint32();
      if ( check_port (val) == -1)
        goto read_forward_start_port;
      add_port_trigger_entry.forward_start_port = val;

      read_forward_end_port:
      QCMAP_PRINTF_TAKE_INPUT("\nEnter the end port: ");
      val = read_uint32();
      if ( check_port (val) == -1)
        goto read_forward_end_port;
      add_port_trigger_entry.forward_end_port = val;

      tmp_input = 0;
      while (TRUE)
      {
        printf("\nPlease input port_trigger_protocol (TCP=%d, UDP=%d): ",\
                                         PS_IPPROTO_TCP, PS_IPPROTO_UDP);
        fflush(stdout);
        tmp_input = read_uint32();
        if ( check_proto(tmp_input) == 0 )
          break;
      }
      add_port_trigger_entry.trigger_protocol = tmp_input;

      tmp_input = 0;
      while (TRUE)
      {
        printf("\nPlease input port_forward_protocol (TCP=%d, UDP=%d): ",\
                                         PS_IPPROTO_TCP, PS_IPPROTO_UDP);
        fflush(stdout);
        tmp_input = read_uint32();
        if ( check_proto(tmp_input) == 0 )
          break;
      }
      add_port_trigger_entry.forward_protocol = tmp_input;

      int timer = 0;
      memset(scan_string, 0, QCMAP_MSGR_MAX_FILE_PATH_LEN);
      QCMAP_PRINTF_TAKE_INPUT("\nPlease enter the timer value in secs: ");
      timer = read_uint32();
      add_port_trigger_entry.timer = timer;
      handle = 0;

      if (QcMapClient->AddPortTriggerEntry(add_port_trigger_entry, &handle, &qmi_err_num))
      {
        printf("\nSuccessfully added the port trigger entry with handle %d", handle);
      }
      else
      {
        printf("\nFailed to add the port trigger entry. Error: 0x%x", qmi_err_num);
      }
    }
    else
    {
      QCMAP_PRINTF_TAKE_INPUT("\nEnter the port trigger handle to be deleted:");
      handle = 0;
      handle = read_uint32();

      if (QcMapClient->DeletePortTriggerEntry(handle, &qmi_err_num))
      {
        printf("\nSuccessfully deleted the port trigger entry");
      }
      else
      {
        printf("\nFailed to delete the port trigger entry. Error: 0x%x", qmi_err_num);
      }
    }
  }
  break;

  /* Display Port Trigger Entries */
  case 40:
  {
    QCMAP_PRINTF_TAKE_INPUT("\nDo you want to see the full list of entries or just a single entry? (1-Single/0-All):");
    int tmp_input = 0, handle_list_len = 0, handle = 0;
    qcmap_msgr_port_trigger_conf_t port_trigger_list;
    memset(&port_trigger_list, 0, sizeof(qcmap_msgr_port_trigger_conf_t));
    tmp_input = read_uint32();
    if (tmp_input == 1)
    {
      QCMAP_PRINTF_TAKE_INPUT("\nEnter the handle: ");
      handle = read_uint32();
    }

    if(QcMapClient->GetPortTriggerEntry(handle, &port_trigger_list, &qmi_err_num))
    {
      handle_list_len = port_trigger_list.num_of_entries;
      for (i = 0; i < handle_list_len; i++)
      {
        printf("\nPort Trigger Handle: %d", port_trigger_list.port_trigger_entry[i].handle);
        printf("\nTrigger Port Start port:End Port is %d:%d ",
               port_trigger_list.port_trigger_entry[i].trigger_start_port,
               port_trigger_list.port_trigger_entry[i].trigger_end_port);
        printf("\nForward Port Start port:End Port is %d:%d ",
               port_trigger_list.port_trigger_entry[i].forward_start_port,
               port_trigger_list.port_trigger_entry[i].forward_end_port);

        switch (port_trigger_list.port_trigger_entry[i].trigger_protocol)
        {
          case PS_IPPROTO_TCP:
            printf("\nTrigger protcol is TCP");
            break;

          case PS_IPPROTO_UDP:
            printf("\nTrigger protcol is UDP");
            break;

          default:
            break;
        }

        switch (port_trigger_list.port_trigger_entry[i].forward_protocol)
        {
          case PS_IPPROTO_TCP:
            printf("\nForward protcol is TCP");
            break;

          case PS_IPPROTO_UDP:
            printf("\nForward protcol is UDP");
            break;

          default:
            break;
        }
        printf("\nTimer value is %d",port_trigger_list.port_trigger_entry[i].timer,0,0);
      }
    }
    else
    {
      printf("\nPort Trigger Handle list get failed, Error: 0x%x", qmi_err_num);
      break;
    }
  }
  break;

  /* Set IPV4 NAT Config */
  case 41:
  {
    boolean ipv4_nat_disable = false;
    memset(scan_string, 0, QCMAP_MSGR_MAX_FILE_PATH_LEN);

    QCMAP_PRINTF_TAKE_INPUT("Please input enable/disable IPV4 NAT disable configuration (1-Enable/0-Disable): ");
    fgets(scan_string, sizeof(scan_string), stdin);
    ipv4_nat_disable = (atoi(scan_string)) ? true : false;

    if (QcMapClient->SetV4NATConfig(ipv4_nat_disable,&qmi_err_num)
                          || qmi_err_num == QMI_ERR_NO_EFFECT_V01)
    {
      printf("\nIPV4 NAT disable configuration : %d", ipv4_nat_disable);
    }
    else
    {
      printf("\nNot able to set IPV4 NAT disable configuration. Error: 0x%x", qmi_err_num);
    }
  }
  break;

  /* Get IPV4 NAT Config */
  case 42:
  {
    boolean ipv4_nat_disable = false;
    memset(scan_string, 0, QCMAP_MSGR_MAX_FILE_PATH_LEN);

    if (QcMapClient->GetV4NATConfig(ipv4_nat_disable,&qmi_err_num))
    {
      printf("\nIPV4 NAT disable configuration : %d", ipv4_nat_disable);
    }
    else
    {
      printf("\nNot able to get current Status of IPV4 NAT disable configuration. Error: 0x%x",
                                                                                  qmi_err_num);
    }
  }
  break;

#endif
  }
}
}
/*===========================================================================
  FUNCTION EnableDisableSOCKSv5
  ===========================================================================*/
/*!
  @brief
  Enables or Disables SOCKSv5 Proxy based on user input

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
static boolean EnableDisableSOCKSv5(qmi_error_type_v01 *qmi_err_num)
{
  char scan_string[QCMAP_MSGR_MAX_FILE_PATH_LEN];

  if(NULL == QcMapClient)
  {
    LOG_MSG_INFO1("QCMAP Client object NULL", 0, 0, 0);
    return false;
  }

  memset(scan_string, 0, sizeof(scan_string));
  QCMAP_PRINTF_TAKE_INPUT("Please input SOCKSv5 Proxy State(1-Enable/0-Disable) : ");
  fgets(scan_string, sizeof(scan_string) - 1, stdin);
  if (!VALID_NUMERIC_INPUT(scan_string))
  {
    printf("\nInvalid response\n");
    return false;
  }

  switch(atoi(scan_string))
  {
    case 0:
    {
      if(QcMapClient->DisableSOCKSv5Proxy(qmi_err_num))
      {
        printf("\nDisabled SOCKSv5 Proxy", 0, 0, 0);
        return true;
      } else
      {
        LOG_MSG_INFO1("Disable SOCKSv5 Proxy fails, Error: 0x%x", *qmi_err_num, 0, 0);
      }
      break;
    }

    case 1:
    {
      if(QcMapClient->EnableSOCKSv5Proxy(qmi_err_num))
      {
        printf("\nEnabled SOCKSv5 Proxy", 0, 0, 0);
        return true;
      } else
      {
        LOG_MSG_INFO1("Enable SOCKSv5 Proxy fails, Error: 0x%x", *qmi_err_num, 0, 0);
      }
      break;
    }

    default:
    {
      printf("Invalid SOCKSv5 Proxy State response: %u\n", scan_string, 0, 0);
    }
  }

  return false;
}

/*===========================================================================
  FUNCTION SetSOCKSv5Config
  ===========================================================================*/
/*!
  @brief
  Sets SOCKSv5 Proxy config based on user input

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
static boolean SetSOCKSv5Config(qmi_error_type_v01 *qmi_err_num)
{
  char scan_string[QCMAP_MSGR_MAX_FILE_PATH_LEN];
  boolean retval = false;
  qcmap_socksv5_config_type_v01 config_type;

  if(NULL == QcMapClient)
  {
    LOG_MSG_INFO1("QCMAP Client object NULL", 0, 0, 0);
    return false;
  }

  memset(scan_string, 0, sizeof(scan_string));
  printf("1. Set SOCKSv5 Proxy Config File Path\n");
  printf("2. Set SOCKSv5 Proxy Authentication Method\n");
  printf("3. Edit LAN Interface\n");
  QCMAP_PRINTF_TAKE_INPUT("4. Add/Delete Username/Profile Association\n");
  fgets(scan_string, sizeof(scan_string) - 1, stdin);

  switch(atoi(scan_string))
  {
    case 1:
    {
      config_type = QCMAP_MSGR_SOCKSV5_SET_CONFIG_FILE_PATH_V01;
      qcmap_msgr_socksv5_config_file_paths_v01 config_file_paths;

      memset(&config_file_paths, 0, sizeof(config_file_paths));

      if(!(GetUserInputSOCKSv5ConfigFilePath(config_file_paths.conf_file,
                                             config_file_paths.auth_file)) ||
           !(retval = QcMapClient->SetSOCKSv5Config(&config_file_paths, config_type, qmi_err_num)))
      {
        LOG_MSG_INFO1("Failed to set SOCKSv5 Proxy Config File Path, Error: 0x%x",
                      *qmi_err_num, 0, 0);
      }
      break;
    }

    case 2:
    {
      unsigned char auth_method;
      config_type = QCMAP_MSGR_SOCKSV5_SET_AUTH_METHOD_V01;
      if(!(GetUserInputSOCKSv5AuthMethod(&auth_method)) ||
         !(retval = QcMapClient->SetSOCKSv5Config(&auth_method, config_type, qmi_err_num)))
      {
        LOG_MSG_INFO1("Failed to set SOCKSv5 Proxy Auth, Error: 0x%x", *qmi_err_num, 0, 0);
      }
      break;
    }

    case 3:
    {
      char lan_iface[IFNAMSIZ];
      memset(lan_iface, 0, IFNAMSIZ * sizeof(char));

      config_type = QCMAP_MSGR_SOCKSV5_EDIT_LAN_IFACE_V01;
      if(!(GetUserInputSOCKSv5LANIface(lan_iface)) ||
         !(retval = QcMapClient->SetSOCKSv5Config(lan_iface, config_type, qmi_err_num)))
      {
        LOG_MSG_INFO1("Failed to edit SOCKSv5 LAN iface, Error: 0x%x", *qmi_err_num, 0, 0);
      }
      break;
    }

    case 4:
    {
      retval = PromptUserSOCKSv5UnameAssoc(qmi_err_num);
      break;
    }

    default:
    {
      printf("Invalid set SOCKSv5 response: %u\n", scan_string, 0, 0);
      retval = false;
      break;
    }
  }

  return retval;

}

/*===========================================================================
  FUNCTION GetUserInputSOCKSv5AuthMethod
  ===========================================================================*/
/*!
  @brief
  Gets SOCKSv5 Proxy authentication method based on user input

  @params
  ptr to unsigned char to hold the authentication method

  @return
   true - valid user input
   false - otherwise

  @note

  - Dependencies
  - None

  - Side Effects
  - None
 */
/*=========================================================================*/
static boolean GetUserInputSOCKSv5AuthMethod(unsigned char *auth_method)
{
  char scan_string[QCMAP_MSGR_MAX_FILE_PATH_LEN];

  memset(scan_string, 0, sizeof(scan_string));
  QCMAP_PRINTF_TAKE_INPUT("Please Enter SOCKSv5 Proxy Authentication Method "
         "(0- No Authentication, 1 - Username/Password): ");
  fgets(scan_string, sizeof(scan_string) - 1, stdin);
  if (!VALID_NUMERIC_INPUT(scan_string))
  {
    printf("\nInvalid response\n");
    return false;
  }

  switch(atoi(scan_string))
  {
    case 0:
    {
      *auth_method = QCMAP_SOCKSV5_NO_AUTHENTICATION_V01;
      break;
    }

    case 1:
    {
      *auth_method = QCMAP_SOCKSV5_UNAME_PASSWD_V01;
      break;
    }

    default:
    {
      printf("Invalid set SOCKSv5 auth response: %s\n", scan_string);
      return false;
      break;
    }
  }

  return true;
}

/*===========================================================================
  FUNCTION GetUserInputSOCKSv5LANIface
  ===========================================================================*/
/*!
  @brief
  Gets SOCKSv5 Proxy lan iface based on user input

  @params
  ptr to hold the lan iface name

  @return
  true - valid lan iface name
  false - otherwise

  @note

  - Dependencies
  - None

  - Side Effects
  - None
 */
/*=========================================================================*/
static boolean GetUserInputSOCKSv5LANIface(char *lan_iface)
{
  char scan_string[QCMAP_MSGR_MAX_FILE_PATH_LEN];

  memset(scan_string, 0, sizeof(scan_string));
  QCMAP_PRINTF_TAKE_INPUT("Please Enter new SOCKSv5 Proxy LAN iface: ");
  fgets(scan_string, sizeof(scan_string) - 1, stdin);

  //bounds check
  if(strlen(scan_string) > IFNAMSIZ)
  {
    printf("Linux Kernel can't allow iface name size > %u\n", IFNAMSIZ);
    return false;
  }

  for(int i = 0; i < strlen(scan_string); i++)
  {
    if(strcmp(&scan_string[i], "\n") != 0)
    {
      lan_iface[i] = scan_string[i];
    }
  }

  //Can't have "" empty string inputs
  if(strcmp(lan_iface, "") == 0)
  {
    printf("Invalid LAN iface name, empty string given\n");
    return false;
  }

  return true;
}

/*===========================================================================
  FUNCTION GetUserInputSOCKSv5ConfigFilePath
  ===========================================================================*/
/*!
  @brief
  Gets SOCKSv5 Proxy config file paths based on user input

  @params
  ptr to hold qti_socksv5_conf.xml file path
  ptr to hold qti_socksv5_auth.xml file path

  @return
  true - valid user input
  false - otherwise

  @note

  - Dependencies
  - None

  - Side Effects
  - None
 */
/*=========================================================================*/
static boolean GetUserInputSOCKSv5ConfigFilePath(char* conf_file, char* auth_file)
{
  char scan_string[QCMAP_MSGR_MAX_FILE_PATH_LEN];

  memset(scan_string, 0, sizeof(scan_string));

  //get conf file path
  QCMAP_PRINTF_TAKE_INPUT("Please Enter SOCKSv5 Configuration File Path: ");
  fgets(scan_string, sizeof(scan_string) - 1, stdin);

  //bounds check
  if(strlen(scan_string) > QCMAP_MSGR_MAX_FILE_PATH_LEN)
  {
    printf("Can't allow path name size > %u\n", QCMAP_MSGR_MAX_FILE_PATH_LEN);
    return false;
  }

  for(int i = 0; i < strlen(scan_string); i++)
  {
    if(strcmp(&scan_string[i], "\n") != 0)
    {
      conf_file[i] = scan_string[i];
    }
  }

  //get auth file path
  memset(scan_string, 0, sizeof(scan_string));
  QCMAP_PRINTF_TAKE_INPUT("Please Enter SOCKSv5 Authentication File Path: ");
  fgets(scan_string, sizeof(scan_string) - 1, stdin);

  //bounds check
  if(strlen(scan_string) > QCMAP_MSGR_MAX_FILE_PATH_LEN)
  {
    printf("Can't allow path name size > %u\n", QCMAP_MSGR_MAX_FILE_PATH_LEN);
    return false;
  }

  for(int i = 0; i < strlen(scan_string); i++)
  {
    if(strcmp(&scan_string[i], "\n") != 0)
    {
      auth_file[i] = scan_string[i];
    }
  }

  //Can't have "" empty string inputs
  if((strcmp(conf_file, "") == 0) || (strcmp(auth_file, "") == 0))
  {
    printf("Invalid confguration file path, empty string given\n");
    return false;
  }

  return true;

}

/*===========================================================================
  FUNCTION PromptUserSOCKSv5UnameAssoc
  ===========================================================================*/
/*!
  @brief
  Prompts user for add/edit/delete configurations for SOCKSv5 Proxy service's
  uname wan service mapping

  @params
  ptr to uname_wan_assoc to store user input

  @return
  true - valid user input
  false - otherwise

  @note

  - Dependencies
  - None

  - Side Effects
  - None
 */
/*=========================================================================*/
static boolean PromptUserSOCKSv5UnameAssoc(qmi_error_type_v01 *qmi_err_num)
{
  qcmap_msgr_socksv5_uname_assoc_v01 uname_assoc;
  boolean retval = false;
  char scan_string[QCMAP_MSGR_MAX_FILE_PATH_LEN];
  qcmap_socksv5_config_type_v01 config_type;

  memset(&uname_assoc, 0, sizeof(qcmap_msgr_socksv5_uname_assoc_v01));
  memset(scan_string, 0, sizeof(scan_string));

  QCMAP_PRINTF_TAKE_INPUT("(0-Add/1-Delete) : ");
  fgets(scan_string, sizeof(scan_string) - 1, stdin);
  if (!VALID_NUMERIC_INPUT(scan_string))
  {
    printf("\nInvalid response\n");
    return false;
  }

  switch(atoi(scan_string))
  {
    case 0:
    {
      config_type = QCMAP_MSGR_SOCKSV5_ADD_UNAME_ASSOC_V01;
      if(!(GetUserInputSOCKSv5Uname(uname_assoc.uname)) ||
         !(GetUserInputSOCKSv5ServiceNo(&(uname_assoc.service_no))) ||
         !(retval = QcMapClient->SetSOCKSv5Config(&uname_assoc, config_type, qmi_err_num)))
      {
        LOG_MSG_INFO1("Failed to add SOCKSv5 uname/assoc, Error: 0x%x", *qmi_err_num, 0, 0);
      }
      break;
    }

    case 1:
    {
      config_type = QCMAP_MSGR_SOCKSV5_DELETE_UNAME_ASSOC_V01;
      if(!(GetUserInputSOCKSv5Uname(uname_assoc.uname)) ||
         !(retval = QcMapClient->SetSOCKSv5Config(uname_assoc.uname, config_type, qmi_err_num)))
      {
        LOG_MSG_INFO1("Failed to delete SOCKSv5 uname/assoc, Error: 0x%x",
                       *qmi_err_num, 0, 0);
      }
      break;
    }

    default:
    {
      printf("Invalid SOCKSv5 uname assoc response: %u\n", scan_string);
      retval = false;
      break;
    }
  }

  return retval;
}

/*===========================================================================
  FUNCTION GetUserInputSOCKSv5Uname
  ===========================================================================*/
/*!
  @brief
  Gets SOCKSv5 Proxy uname based on user input

  @params
  ptr to uname

  @return
  true - valid ulen and plen
  false - otherwise

  @note

  - Dependencies
  - None

  - Side Effects
  - None
 */
/*=========================================================================*/
static boolean GetUserInputSOCKSv5Uname(char* uname)
{
  char scan_string[QCMAP_SOCKSV5_MAX_UNAME_PASSWD_LEN_V01 + 1];
  /* NOTE: Having scan_string as +2, results in the original string (uname)
   * with a size of 255, being populated with characters until the
   * last 254th bit leaving the original array without termination */
  memset(uname, 0, QCMAP_SOCKSV5_MAX_UNAME_PASSWD_LEN_V01);
  memset(scan_string, 0, sizeof(scan_string));

  QCMAP_PRINTF_TAKE_INPUT("Please Enter Username: ");
  fgets(scan_string, sizeof(scan_string) - 1, stdin);

  //bounds check
  if(!(CheckSOCKSv5UnameLen(scan_string)))
  {
    return false;
  }
  for(int i = 0; i < strlen(scan_string); i++)
  {
    if(strcmp(&scan_string[i], "\n") != 0)
    {
      uname[i] = scan_string[i];
    }
  }

  //Can't have "" empty string inputs
  if(strcmp(uname, "") == 0)
  {
    printf("Invalid username empty string given\n");
    return false;
  }

  return true;
}

/*===========================================================================
  FUNCTION GetUserInputSOCKSv5ServiceNo
  ===========================================================================*/
/*!
  @brief
  Gets SOCKSv5 Proxy service no based on user input

  @params
  ptr to service no to store user input

  @return
  true - valid user input
  false - otherwise

  @note

  - Dependencies
  - None

  - Side Effects
  - None
 */
/*=========================================================================*/
static boolean GetUserInputSOCKSv5ServiceNo(unsigned int* service_no)
{
  char scan_string[QCMAP_MSGR_MAX_FILE_PATH_LEN];

  memset(scan_string, 0, sizeof(scan_string));
  QCMAP_PRINTF_TAKE_INPUT("Please Enter Service/Profile#: ");
  fgets(scan_string, sizeof(scan_string) - 1, stdin);

  //no empty string
  if(strcmp(scan_string, "\n") == 0)
  {
    printf("Can't have empty string as service/profile #\n");
    return false;
  }

  if(atoi(scan_string) >= 0)
  {
    *service_no = atoi(scan_string);
    return true;
  }

  printf("Invalid service no: %s\n", scan_string);

  return false;
}

/*===========================================================================
  FUNCTION CheckSOCKSv5UnamePasswdLen
  ===========================================================================*/
/*!
  @brief
  Checks SOCKSv5 Proxy Username and Password Len

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
static boolean CheckSOCKSv5UnameLen(char* str)
{
  //bounds check
  if((strlen(str) > QCMAP_SOCKSV5_MAX_UNAME_PASSWD_LEN_V01 + 1 &&
      str[strlen(str) - 1] == '\n') || (str[strlen(str) - 1] != '\n' &&
      strlen(str) > QCMAP_SOCKSV5_MAX_UNAME_PASSWD_LEN_V01))
  {
    printf("RFC 1929 can't allow uname len > %u\n", QCMAP_SOCKSV5_MAX_UNAME_PASSWD_LEN_V01);
    return false;
  }
  return true;
}

void wlanConfig( int wlanOpt )
{
  qmi_error_type_v01 qmi_err_num;
  /* Initialize QMI Error Number. */
  qmi_err_num = QMI_ERR_NONE_V01;
  char scan_string[QCMAP_MSGR_MAX_FILE_PATH_LEN];
  qcmap_msgr_wlan_if_info_t wlan_info_cfg;
  memset(&wlan_info_cfg, 0, sizeof(qcmap_msgr_wlan_if_info_t));
  in_addr addr;
  int ret_val = 0;

  /* WLAN configuration options */
  switch(wlanOpt)
  {
   /* Enable/Disable WLAN */
   case 1:
   {
     QCMAP_PRINTF_TAKE_INPUT("   Please input WLAN State(1-Enable/0-Disable) : ");
     fgets(scan_string, sizeof(scan_string), stdin);
     if (atoi(scan_string) == 1)
     {
       if(QcMapClient->EnableWLAN(&qmi_err_num))
       {
         printf("\nEnabled WLAN");
       }
       else
         printf("\nEnable WLAN fails, Error: 0x%x", qmi_err_num);
     }
     else if (isdigit(scan_string[0]) && strtol(scan_string, NULL, 10) == 0)
     {
       if(QcMapClient->DisableWLAN(&qmi_err_num))
       {
         QcMapClient->WhitelistWLANChannels();
         printf("\nDisabled WLAN");
       }
       else
         printf("\nDisable WLAN fails, Error: 0x%x", qmi_err_num);
     }
     else
     {
       printf("\nInvalid option");
     }
     break;
   }

   /* Activate WLAN */
   case 2:
   {
     if(QcMapClient->ActivateWLAN(&qmi_err_num))
     {
       printf("\nActivated WLAN\n");
     }
     else
       printf("\nFailed to Activate WLAN, Error: 0x%x", qmi_err_num);
     break;
   }

   /* Set WLAN config */
   case 3:
   case 16:
   {
     int p_error = 0, input_var = 0 ;
     bool primary_AP_check = false;
     int guest_ap_count = 0;
     in_addr addr;
     int validate_arr[] = {2,5,6};
     char validate_string[50] = "2GHz/5GHz/6GHz - enter the value only";
     memset(scan_string, 0, QCMAP_MSGR_MAX_FILE_PATH_LEN);
     qcmap_wlan_ex2_config wlan_ex_config;
     memset(&wlan_ex_config, 0x0, sizeof(qcmap_wlan_ex2_config));
       QCMAP_PRINTF_TAKE_INPUT("Select the Type of WLAN Mode : \n"
             "1: AP        2: AP-AP\n"
             "3: AP-STA    4: AP-AP-AP\n"
             "5: AP-AP-STA 6: STA-Only \n"
             "7: AP-AP-AP-AP:::\n"
             "11: AP-P2P    12: STA-P2P \n"
             "13: 7-AP Mode \n");

     while (TRUE ) {
         fgets(scan_string, sizeof(scan_string), stdin);
         wlan_ex_config.wlan_mode = (qcmap_msgr_wlan_mode_enum_v01)atoi(scan_string);
         if ( wlan_ex_config.wlan_mode < QCMAP_MSGR_WLAN_MODE_AP_V01 ||
              wlan_ex_config.wlan_mode > QCMAP_MSGR_WLAN_MODE_7AP_V01 ){

         QCMAP_PRINTF_TAKE_INPUT("\nPlease enter a valid WLAN Mode\t:::");
         continue;
        }
        break;
     }
   if(wlan_ex_config.wlan_mode != QCMAP_MSGR_WLAN_MODE_7AP_V01)
   {
     if (wlanOpt == 16 && wlan_ex_config.wlan_mode != QCMAP_MSGR_WLAN_MODE_STA_ONLY_V01)
     {
       QCMAP_PRINTF_TAKE_INPUT(" Do you want to set the primary AP band info.? 1-Yes/0-No\n");
       //check for primary AP band. set or not
       READ_AND_VALIDATE_INT_VALUE(input_var, 0, 1);
       if (input_var == 1)
       {
         QCMAP_PRINTF_TAKE_INPUT(" Enter the primary AP band(2GHz/5GHz/6GHz). Enter only the value. \n");
         READ_AND_VALIDATE_ARRAY_VALUE(input_var, validate_arr, validate_string);
         wlan_ex_config.primary_ap_band = input_var;
       }
     }
     if (wlanOpt == 3 || (wlanOpt == 16 && wlan_ex_config.wlan_mode != QCMAP_MSGR_WLAN_MODE_AP_V01 &&
                          wlan_ex_config.wlan_mode != QCMAP_MSGR_WLAN_MODE_AP_STA_V01 &&
                          wlan_ex_config.wlan_mode != QCMAP_MSGR_WLAN_MODE_STA_ONLY_V01))
     {
       QCMAP_PRINTF_TAKE_INPUT(" Do you want to change Guest AP 1 Access Profile 1-Yes/0-No\n");
       READ_AND_VALIDATE_INT_VALUE(input_var, 0, 1);
       guest_ap_count++;
       if (input_var == 1)
       {
         QCMAP_PRINTF_TAKE_INPUT(" Select guest AP 1 access profile 1-Full Access / 0- Internet Only \n");
         fgets(scan_string, sizeof(scan_string), stdin);
         if (!VALID_NUMERIC_INPUT(scan_string) || (atoi(scan_string) != 1 && atoi(scan_string) != 0))
         {
           printf("\nInvalid response. Please enter 1 or 0.\n");
           break;
         }
         if (atoi(scan_string) == 1)
         {
           wlan_ex_config.ap_config[QCMAP_MSGR_GUEST_AP_ONE_V01].guest_ap_profile = QCMAP_MSGR_PROFILE_FULL_ACCESS_V01;
         }
         else
         {
           wlan_ex_config.ap_config[QCMAP_MSGR_GUEST_AP_ONE_V01].guest_ap_profile = QCMAP_MSGR_PROFILE_INTERNET_ONLY_V01;
         }
       }
       if (wlanOpt == 16)
       {
         QCMAP_PRINTF_TAKE_INPUT(" Do you want to set Guest AP 1 Band 1-Yes/0-No\n");
         READ_AND_VALIDATE_INT_VALUE(input_var, 0, 1);
         if (input_var == 1)
         {
           QCMAP_PRINTF_TAKE_INPUT(" Select guest AP 1 band(2GHz/5GHz/6GHz). Enter only the value. \n");
           READ_AND_VALIDATE_ARRAY_VALUE(input_var, validate_arr, validate_string);
           wlan_ex_config.ap_config[QCMAP_MSGR_GUEST_AP_ONE_V01].band = input_var;
         }
       }
     }

     if ( (wlanOpt == 3) || (wlan_ex_config.wlan_mode == QCMAP_MSGR_WLAN_MODE_AP_AP_AP_V01) ||
                            (wlan_ex_config.wlan_mode == QCMAP_MSGR_WLAN_MODE_AP_AP_AP_AP_V01) )
     {
       guest_ap_count = guest_ap_count + 1;
       QCMAP_PRINTF_TAKE_INPUT(" Do you want to change Guest AP 2 Access Profile 1-Yes/0-No\n");
       READ_AND_VALIDATE_INT_VALUE(input_var, 0, 1);
       guest_ap_count++;
       if (input_var == 1)
       {
         QCMAP_PRINTF_TAKE_INPUT(" Select guest AP 2 access profile 1-Full Access / 0- Internet Only \n");
         fgets(scan_string, sizeof(scan_string), stdin);
         if (!VALID_NUMERIC_INPUT(scan_string) || (atoi(scan_string) != 1 && atoi(scan_string) != 0))
         {
           printf("\nInvalid response. Please enter 1 or 0.\n");
           break;
         }
         if (atoi(scan_string) == 1)
         {
           wlan_ex_config.ap_config[QCMAP_MSGR_GUEST_AP_TWO_V01].guest_ap_profile = QCMAP_MSGR_PROFILE_FULL_ACCESS_V01;
         }
         else
         {
           wlan_ex_config.ap_config[QCMAP_MSGR_GUEST_AP_TWO_V01].guest_ap_profile = QCMAP_MSGR_PROFILE_INTERNET_ONLY_V01;
         }
       }
       if (wlanOpt == 16)
       {
         QCMAP_PRINTF_TAKE_INPUT(" Do you want to set Guest AP 2 Band 1-Yes/0-No\n");
         READ_AND_VALIDATE_INT_VALUE(input_var, 0, 1);
         if (input_var == 1)
         {
           QCMAP_PRINTF_TAKE_INPUT(" Select guest AP 2 band(2GHz/5GHz/6GHz). Enter only the value. \n");
           READ_AND_VALIDATE_ARRAY_VALUE(input_var, validate_arr, validate_string);
           wlan_ex_config.ap_config[QCMAP_MSGR_GUEST_AP_TWO_V01].band =  input_var;
         }
       }
     }

     if(wlan_ex_config.wlan_mode == QCMAP_MSGR_WLAN_MODE_AP_AP_AP_AP_V01)
     {
       guest_ap_count = guest_ap_count + 1;
       QCMAP_PRINTF_TAKE_INPUT(" Do you want to change Guest AP 3 Access Profile 1-Yes/0-No\n");
       READ_AND_VALIDATE_INT_VALUE(input_var, 0, 1);
       guest_ap_count++;
       if (input_var == 1)
       {
         QCMAP_PRINTF_TAKE_INPUT(" Select guest AP 3 access profile 1-Full Access / 0- Internet Only \n");
         fgets(scan_string, sizeof(scan_string), stdin);
         if (!VALID_NUMERIC_INPUT(scan_string) || (atoi(scan_string) != 1 && atoi(scan_string) != 0))
         {
           printf("\nInvalid response. Please enter 1 or 0.\n");
           break;
         }
         if (atoi(scan_string) == 1)
         {
           wlan_ex_config.ap_config[QCMAP_MSGR_GUEST_AP_THREE_V01].guest_ap_profile = QCMAP_MSGR_PROFILE_FULL_ACCESS_V01;
         }
         else
         {
           wlan_ex_config.ap_config[QCMAP_MSGR_GUEST_AP_THREE_V01].guest_ap_profile = QCMAP_MSGR_PROFILE_INTERNET_ONLY_V01;
         }
       }

       if (wlanOpt == 16)
       {
         QCMAP_PRINTF_TAKE_INPUT(" Do you want to set Guest AP 3 Band 1-Yes/0-No\n");
         READ_AND_VALIDATE_INT_VALUE(input_var, 0, 1);
         if (input_var == 1)
         {
           QCMAP_PRINTF_TAKE_INPUT(" Select guest AP 3 band(2GHz/5GHz/6GHz). Enter only the value. \n");
           READ_AND_VALIDATE_ARRAY_VALUE(input_var, validate_arr, validate_string);
           wlan_ex_config.ap_config[QCMAP_MSGR_GUEST_AP_THREE_V01].band =  input_var;
         }
       }
     }

     if( wlanOpt == 3 || (wlanOpt == 16 && wlan_ex_config.wlan_mode == QCMAP_MSGR_WLAN_MODE_AP_STA_V01 ||
         wlan_ex_config.wlan_mode == QCMAP_MSGR_WLAN_MODE_AP_AP_STA_V01 ||
         wlan_ex_config.wlan_mode == QCMAP_MSGR_WLAN_MODE_STA_ONLY_V01) )
     {
       QCMAP_PRINTF_TAKE_INPUT("   Do you want to set Station Configuration(1-Yes/0-No) : ");
       READ_AND_VALIDATE_INT_VALUE(input_var, 0, 1);
       if (input_var == 1)
       {
         QCMAP_PRINTF_TAKE_INPUT("   Please input Connection Type, 1 for DYNAMIC/2 for STATIC:");
         fgets(scan_string, sizeof(scan_string), stdin);
         if (atoi(scan_string) == 2)
         {
           wlan_ex_config.station_config.conn_type = QCMAP_MSGR_STA_CONNECTION_STATIC_V01;

           QCMAP_PRINTF_TAKE_INPUT("   Please input a valid Static IP address:");
           memset(&addr,0,sizeof(in_addr));
           read_addr(AF_INET, (uint8 *)&addr.s_addr);
           wlan_ex_config.station_config.static_ip_config.ip_addr = addr.s_addr;
           QCMAP_PRINTF_TAKE_INPUT("   Please input a valid Gateway address:");
           memset(&addr,0,sizeof(in_addr));
           read_addr(AF_INET, (uint8 *)&addr.s_addr);
           wlan_ex_config.station_config.static_ip_config.gw_ip = addr.s_addr;
           QCMAP_PRINTF_TAKE_INPUT("   Please input a valid Netmask:");
           memset(&addr,0,sizeof(in_addr));
           read_addr(AF_INET, (uint8 *)&addr.s_addr);
           wlan_ex_config.station_config.static_ip_config.netmask = addr.s_addr;
           QCMAP_PRINTF_TAKE_INPUT("   Please input a valid DNS Address:");
           memset(&addr,0,sizeof(in_addr));
           read_addr(AF_INET, (uint8 *)&addr.s_addr);
           wlan_ex_config.station_config.static_ip_config.dns_addr = addr.s_addr;
         }
         else if (atoi(scan_string) == 1)
         {
           wlan_ex_config.station_config.conn_type = QCMAP_MSGR_STA_CONNECTION_DYNAMIC_V01;
         }
         else 
         {
          printf("\nInvalid response. Please enter 1 or 2.\n");
          break;
         }
         QCMAP_PRINTF_TAKE_INPUT(" Please Select AP+STA/AP+AP+STA Mode, 0-RouterMode / 1-BridgeMode:");
         fgets(scan_string, sizeof(scan_string), stdin);
         if (!VALID_NUMERIC_INPUT(scan_string) || atoi(scan_string) != 1 && atoi(scan_string) != 0)
         {
          printf("\nInvalid response. Please enter 1 or 0.\n");
          break;
         }
         wlan_ex_config.station_config.ap_sta_bridge_mode = (atoi(scan_string) ? 1:0);
         if (wlanOpt == 16)
         {
           QCMAP_PRINTF_TAKE_INPUT(" Do you want to set Station Band 1-Yes/0-No\n");
           READ_AND_VALIDATE_INT_VALUE(input_var, 0, 1);
           if (input_var == 1)
           {
             QCMAP_PRINTF_TAKE_INPUT(" Select Station band(2GHz/5GHz/6GHz). Enter only the value. \n");
             READ_AND_VALIDATE_ARRAY_VALUE(input_var, validate_arr, validate_string);
             wlan_ex_config.station_band =  input_var;
           }
         }
       }
     }
    }
    else
    {
      printf("7AP configuration\n");
      QCMAP_PRINTF_TAKE_INPUT(" Do you want to Configure in 2GHz.? 1-Yes/0-No\n");
      READ_AND_VALIDATE_INT_VALUE(input_var, 0, 1);
      if (input_var)
      {
        /* 2GHz Radio Configuration */
        QCMAP_PRINTF_TAKE_INPUT(" Do you want to set the primary AP band info in 2GHz.? 1-Yes/0-No\n");
        //check for primary AP band. set or not
        READ_AND_VALIDATE_INT_VALUE(input_var, 0, 1);
        if (input_var == 1)
        {
          primary_AP_check = true;
          printf("Primary AP is configured in 2GHz Radio successfully\n");
        }
        QCMAP_PRINTF_TAKE_INPUT(" Do you want to set the Guest AP band info in 2GHz? 1-Yes/0-No\n");
        READ_AND_VALIDATE_INT_VALUE(input_var, 0, 1);
        if (input_var == 1)
        {
          if ( wlan_ex_config.primary_ap_band == 2)
          {
            printf(" Maximum number of Guest AP available in 2GHz is 6\n How many Guest AP Access Profile Do you want to configure?\n");
            READ_AND_VALIDATE_INT_VALUE(input_var, 1, 6);
            guest_ap_count = input_var;
            wlan_ex_config.guestap_count_2g = input_var;
          }
          else
          {
            printf(" Maximum number of Guest AP available in 2GHz is 7\n How many Guest AP Access Profile Do you want to configure?\n");
            READ_AND_VALIDATE_INT_VALUE(input_var, 1, 7);
            guest_ap_count = input_var;
            wlan_ex_config.guestap_count_2g = input_var;
          }
          for ( int i = 0; i < input_var; i++)
          {
            printf(" Select guest AP %d access profile 1-Full Access / 0- Internet Only \n", i + 1);
            fgets(scan_string, sizeof(scan_string), stdin);
            if (!VALID_NUMERIC_INPUT(scan_string) || (atoi(scan_string) != 1 && atoi(scan_string) != 0))
            {
              printf("\nInvalid response. Please enter 1 or 0.\n");
              break;
            }
            if (atoi(scan_string) == 1)
            {
              wlan_ex_config.ap_config[i].guest_ap_profile = QCMAP_MSGR_PROFILE_FULL_ACCESS_V01;
            }
            else
            {
              wlan_ex_config.ap_config[i].guest_ap_profile = QCMAP_MSGR_PROFILE_INTERNET_ONLY_V01;
            }
            wlan_ex_config.ap_config[i].band =  2;
          }
        }
      }

      QCMAP_PRINTF_TAKE_INPUT(" Do you want to Configure in 5GHz.? 1-Yes/0-No\n");
      READ_AND_VALIDATE_INT_VALUE(input_var, 0, 1);
      if (input_var)
      {
        /* 5GHz Radio Configuration */
        //check for primary AP band. set or not
        if (primary_AP_check != true)
        {
          QCMAP_PRINTF_TAKE_INPUT(" Do you want to set the primary AP band info in 5GHz.? 1-Yes/0-No\n");
          READ_AND_VALIDATE_INT_VALUE(input_var, 0, 1);
          if (input_var == 1)
          {
            wlan_ex_config.primary_ap_band = 5;
            primary_AP_check = true;
            printf("Primary AP is configured in 5GHz Radio successfully\n");
          }
        }
        QCMAP_PRINTF_TAKE_INPUT(" Do you want to set the Guest AP band info in 5GHz? 1-Yes/0-No\n");
        READ_AND_VALIDATE_INT_VALUE(input_var, 0, 1);
        if (input_var == 1)
        {
          if ( wlan_ex_config.primary_ap_band == 5)
          {
            printf(" Maximum number of Guest AP available in 5GHz is 6\n How many Guest AP Access Profile Do you want to configure?\n");
            READ_AND_VALIDATE_INT_VALUE(input_var, 1, 6);
            wlan_ex_config.guestap_count_5g = input_var;
          }
          else
          {
            printf(" Maximum number of Guest AP available in 5GHz is 7\n How many Guest AP Access Profile Do you want to configure?\n");
            READ_AND_VALIDATE_INT_VALUE(input_var, 1, 7);
            wlan_ex_config.guestap_count_5g = input_var;
          }
          for ( int i = guest_ap_count; i < (guest_ap_count + input_var); i++)
          {
            printf(" Select guest AP %d access profile 1-Full Access / 0- Internet Only \n", i + 1);
            fgets(scan_string, sizeof(scan_string), stdin);
            if (!VALID_NUMERIC_INPUT(scan_string) || (atoi(scan_string) != 1 && atoi(scan_string) != 0))
            {
              printf("\nInvalid response. Please enter 1 or 0.\n");
              break;
            }
            if (atoi(scan_string) == 1)
            {
              wlan_ex_config.ap_config[i].guest_ap_profile = QCMAP_MSGR_PROFILE_FULL_ACCESS_V01;
            }
            else
            {
              wlan_ex_config.ap_config[i].guest_ap_profile = QCMAP_MSGR_PROFILE_INTERNET_ONLY_V01;
            }
            wlan_ex_config.ap_config[i].band =  5;
          }
          /* Update the total number of guest ap configures */
          guest_ap_count = guest_ap_count + input_var;
        }
      }

      QCMAP_PRINTF_TAKE_INPUT(" Do you want to Configure in 6GHz.? 1-Yes/0-No\n");
      READ_AND_VALIDATE_INT_VALUE(input_var, 0, 1);
      if (input_var)
      {
        /* 6GHz Radio Configuration */
        //check for primary AP band. set or not
        while(primary_AP_check != true)
        {
          QCMAP_PRINTF_TAKE_INPUT(" Do you want to set the primary AP band info in 6GHz.? 1-Yes/0-No\n");
          READ_AND_VALIDATE_INT_VALUE(input_var, 0, 1);
          if (input_var == 1)
          {
            wlan_ex_config.primary_ap_band = 6;
            primary_AP_check = true;
            printf("Primary AP is configured in 6GHz Radio successfully\n");
          }
          else
          {
            QCMAP_PRINTF_TAKE_INPUT(" Primary Ap is not Configured, Enter the primary AP band(2GHz/5GHz/6GHz). Enter only the value. \n");
            fgets(scan_string, sizeof(scan_string), stdin);
            wlan_ex_config.primary_ap_band = atoi(scan_string);
            primary_AP_check = true;
          }
        }
        QCMAP_PRINTF_TAKE_INPUT(" Do you want to set the Guest AP band info in 6GHz? 1-Yes/0-No\n");
        READ_AND_VALIDATE_INT_VALUE(input_var, 0, 1);
        if (input_var == 1)
        {
          if ( wlan_ex_config.primary_ap_band == 6)
          {
            printf(" Maximum number of Guest AP available in 6GHz is 6\n How many Guest AP Access Profile Do you want to configure?\n");
            READ_AND_VALIDATE_INT_VALUE(input_var, 1, 6);
            wlan_ex_config.guestap_count_6g = input_var;
          }
          else
          {
            printf(" Maximum number of Guest AP available in 6GHz is 7\n How many Guest AP Access Profile Do you want to configure?\n");
            READ_AND_VALIDATE_INT_VALUE(input_var, 1, 7);
            wlan_ex_config.guestap_count_6g = input_var;
          }
          for ( int i = guest_ap_count; i < (guest_ap_count + input_var); i++)
          {
            printf(" Select guest AP %d access profile 1-Full Access / 0- Internet Only \n", i + 1);
            fgets(scan_string, sizeof(scan_string), stdin);
            if (!VALID_NUMERIC_INPUT(scan_string) || (atoi(scan_string) != 1 && atoi(scan_string) != 0))
            {
              printf("\nInvalid response. Please enter 1 or 0.\n");
              break;
            }
            if (atoi(scan_string) == 1)
            {
              wlan_ex_config.ap_config[i].guest_ap_profile = QCMAP_MSGR_PROFILE_FULL_ACCESS_V01;
            }
            else
            {
              wlan_ex_config.ap_config[i].guest_ap_profile = QCMAP_MSGR_PROFILE_INTERNET_ONLY_V01;
            }
            wlan_ex_config.ap_config[i].band =  6;
          }
          /* Update the total number of guest ap configures */
          guest_ap_count = guest_ap_count + input_var;
        }
      }
    }
    if(guest_ap_count <= QCMAP_MSGR_MAX_GUEST_AP_COUNT)
    {
      wlan_ex_config.ap_config_len = (guest_ap_count+1);
    }
    else
    {
      printf("Invalid Guest AP count");
      break;
    }
    if (QcMapClient->SetWLANConfigEx2(wlan_ex_config,&qmi_err_num))
    {
      if (qmi_err_num ==  QMI_ERR_NONE_V01)
      {
        printf("\n WLAN Config Set Successfully\n");
      }
      else
      {
        printf("\nWLAN Config set fails, Error: 0x%x", qmi_err_num);
      }
    }
    else
    {
      if(qmi_err_num ==  QMI_ERR_INVALID_HANDLE_V01)
      {
        printf("\nMobileAP is not enabled\n");
        break;
      }
      printf("\nWLAN Config set fails, Error: 0x%x", qmi_err_num);
    }
   break;
  }


   /* Get WLAN Config */
   case 4:
   case 17:
   {
     qcmap_wlan_ex_config wlan_config;
     in_addr addr;

     ZERO_INIT_ARG(wlan_config);
     ZERO_INIT_ARG(addr);
     if (QcMapClient->GetWLANConfigEx(&wlan_config, &qmi_err_num))
     {
       if (1 <= wlan_config.wlan_mode && QCMAP_MSGR_WLAN_MODE_7AP_V01 >= wlan_config.wlan_mode )
       {
         printf("\nConfigured WLAN Mode is %s.\n", wlan_mode_str[wlan_config.wlan_mode]);
       }
       else
       {
         printf("\nUnsupported WLAN Mode:- %d.\n", wlan_config.wlan_mode);
         break;
       }
     }
     else{
       if (qmi_err_num == QMI_ERR_NONE_V01)
       {
         printf("\n WLAN Config is NOT SET");
       }
       else
       {
         printf("\nGet WLAN Config failed");
       }
      break;
    }

       if (wlanOpt == 17)
       {
         printf("\n Primary AP Band is %dGHz\n", wlan_config.primary_ap_band);
         // Guest AP 1 Access/Band profile
         printf("\n Guest AP 1 is setup to be in '%s' mode of Band %dGHz \n",
               (wlan_config.ap_config[QCMAP_MSGR_GUEST_AP_ONE_V01].guest_ap_profile == QCMAP_MSGR_PROFILE_FULL_ACCESS_V01)?
               "Full Access" : "Internet Only Access", wlan_config.ap_config[QCMAP_MSGR_GUEST_AP_ONE_V01].band);
         // Guest AP 2 Access/Band profile
         printf("\n Guest AP 2 is setup to be in '%s' mode of Band %dGHz\n",
               (wlan_config.ap_config[QCMAP_MSGR_GUEST_AP_TWO_V01].guest_ap_profile == QCMAP_MSGR_PROFILE_FULL_ACCESS_V01)?
               "Full Access" : "Internet Only Access", wlan_config.ap_config[QCMAP_MSGR_GUEST_AP_TWO_V01].band);
         // Guest AP 3 Access/Band profile
         printf("\n Guest AP 3 is setup to be in '%s' mode of band %dGHz\n",
               (wlan_config.ap_config[QCMAP_MSGR_GUEST_AP_THREE_V01].guest_ap_profile == QCMAP_MSGR_PROFILE_FULL_ACCESS_V01)?
               "Full Access" : "Internet Only Access", wlan_config.ap_config[QCMAP_MSGR_GUEST_AP_THREE_V01].band);
         //Station Band.
         printf("\n Station is set with Band %dGHz", wlan_config.station_band);
       }
       else
       {
         // Guest AP 1 Access profile
         printf("\n Guest AP 1 is setup to be in '%s' mode \n",
               (wlan_config.guest_profile.guest_ap_profile[QCMAP_MSGR_GUEST_AP_ONE_V01] == QCMAP_MSGR_PROFILE_FULL_ACCESS_V01)?
               "Full Access" : "Internet Only Access");

         // Guest AP 2 Access profile
         printf("\n Guest AP 2 is setup to be in '%s' mode \n",
               (wlan_config.guest_profile.guest_ap_profile[QCMAP_MSGR_GUEST_AP_TWO_V01] == QCMAP_MSGR_PROFILE_FULL_ACCESS_V01)?
               "Full Access" : "Internet Only Access");

         // Guest AP 3 Access profile
         printf("\n Guest AP 3 is setup to be in '%s' mode \n",
               (wlan_config.guest_profile.guest_ap_profile[QCMAP_MSGR_GUEST_AP_THREE_V01] == QCMAP_MSGR_PROFILE_FULL_ACCESS_V01)?
               "Full Access" : "Internet Only Access");
       }

       /* Print Station Configuration. */
       if (wlan_config.wlan_mode != QCMAP_MSGR_WLAN_MODE_7AP_V01)
       {
       printf("\nStation Configuration.\n");
       if ( wlan_config.station_config.conn_type == QCMAP_MSGR_STA_CONNECTION_DYNAMIC_V01)
         printf("\nConnection Type : DYNAMIC (DHCP)\n");
       else
       {
         printf("\nConnection Type : STATIC\n");
       }
       printf("\n STATIC STA IP Configuration\n");
       addr.s_addr = htonl(wlan_config.station_config.static_ip_config.ip_addr);
       printf("\nIP Address: %s\n", inet_ntoa(addr));
       addr.s_addr = htonl(wlan_config.station_config.static_ip_config.gw_ip);
       printf("\nGateway IP : %s\n", inet_ntoa(addr));
       addr.s_addr = htonl(wlan_config.station_config.static_ip_config.netmask);
       printf("\nNetmask: %s\n", inet_ntoa(addr));
       addr.s_addr = htonl(wlan_config.station_config.static_ip_config.dns_addr);
       printf("\nDNS Address : %s\n", inet_ntoa(addr));
       printf("\nSTA is configured in %s \n",(wlan_config.station_config.ap_sta_bridge_mode?"Bridge Mode":"Router Mode"));

     }
     break;
   }

   /* Get WLAN Status */
   case 5:
   {

     qcmap_msgr_wlan_mode_enum_v01 wlan_mode;
     if (QcMapClient->GetWLANStatus(&wlan_mode, &qmi_err_num))
     {
       if (1 <= wlan_mode && QCMAP_MSGR_WLAN_MODE_7AP_V01 >= wlan_mode )
       {
         printf("\nConfigured WLAN Mode is %s.\n", wlan_mode_str[wlan_mode]);
       }
       else
       {
         printf("\nUnsupported WLAN Mode:- %d.\n", wlan_mode);
       }
     }
     else
       printf("\nGet WLAN Status failed, Error: 0x%x", qmi_err_num);
     break;
   }

   /* Set MobileAP/WLAN Bootup Config */
   case 6:
   {
     qcmap_bootup_enable_config bootup_config;
     memset(&bootup_config, 0x0, sizeof(bootup_config));

     QCMAP_PRINTF_TAKE_INPUT(" Do you want to change MobileAP bootup configuration (1- Change /0- Do not Change) : ");
     fgets(scan_string, sizeof(scan_string), stdin);
     if (!VALID_NUMERIC_INPUT(scan_string) || atoi(scan_string) != 1 && atoi(scan_string) != 0)
     {
       printf("\nInvalid response. Please enter 1 or 0.\n");
       break;
     }
     if (atoi(scan_string)) {
        QCMAP_PRINTF_TAKE_INPUT(" Do you want to enable MobileAP on bootup (1-Enable/0-Disable) : ");
        fgets(scan_string, sizeof(scan_string), stdin);
        if (!VALID_NUMERIC_INPUT(scan_string) || atoi(scan_string) != 1 && atoi(scan_string) != 0)
        {
          printf("\nInvalid response. Please enter 1 or 0.\n");
          break;
        }
        if (atoi(scan_string)) {
           bootup_config.mobileap_enable = QCMAP_MSGR_ENABLE_ON_BOOT_V01;
        } else {
           bootup_config.mobileap_enable = QCMAP_MSGR_DISABLE_ON_BOOT_V01;
        }
     } else {
        bootup_config.mobileap_enable = QCMAP_MSGR_BOOTUP_FLAG_MIN_ENUM_VAL_V01;
     }

     QCMAP_PRINTF_TAKE_INPUT(" Do you want to change WLAN bootup configuration (1- Change /0- Do not Change) : ");
     fgets(scan_string, sizeof(scan_string), stdin);
     if (!VALID_NUMERIC_INPUT(scan_string) || atoi(scan_string) != 1 && atoi(scan_string) != 0)
     {
       printf("\nInvalid response. Please enter 1 or 0.\n");
       break;
     }
      if (atoi(scan_string)) {
        QCMAP_PRINTF_TAKE_INPUT(" Do you want to enable WLAN on bootup (1-Enable/0-Disable) : ");
        fgets(scan_string, sizeof(scan_string), stdin);
        if (!VALID_NUMERIC_INPUT(scan_string) || atoi(scan_string) != 1 && atoi(scan_string) != 0)
        {
          printf("\nInvalid response. Please enter 1 or 0.\n");
          break;
        }
        if (atoi(scan_string)) {
           bootup_config.wlan_enable = QCMAP_MSGR_ENABLE_ON_BOOT_V01;
        } else {
           bootup_config.wlan_enable = QCMAP_MSGR_DISABLE_ON_BOOT_V01;
        }
     }
     else {
        bootup_config.wlan_enable = QCMAP_MSGR_BOOTUP_FLAG_MIN_ENUM_VAL_V01;
     }

     if (QcMapClient->SetQCMAPBootupCfgEx(bootup_config, &qmi_err_num)){
         printf("\n QCMAP Bootup configuration set successfully set \n");
     } else {
         printf("\n  Set QCMAP Bootup Cfg Fails , Error: 0x%x \n", qmi_err_num);
     }
     break;
   }

   /* Get MobileAP/WLAN Bootup config */
   case 7:
   {
     qcmap_bootup_enable_config bootup_config;
     if (QcMapClient->GetQCMAPBootupCfgEx(&bootup_config, &qmi_err_num)){
         printf("\n Mobile AP will be %s on bootup \n",((bootup_config.mobileap_enable == QCMAP_MSGR_ENABLE_ON_BOOT_V01) ?"ENABLED":"DISABLED"));
         printf("\n WLAN will be %s on bootup \n",((bootup_config.wlan_enable == QCMAP_MSGR_ENABLE_ON_BOOT_V01)?"ENABLED":"DISABLED"));
     } else {
         printf("\n  Get QCMAP Bootup Cfg Fails , Error: 0x%x \n", qmi_err_num);
     }
     break;
   }

   /* Get station mode status. */
   case 8:
   {

     qcmap_msgr_station_mode_status_enum_v01 status;
     if (QcMapClient->GetStationModeStatus(&status, &qmi_err_num))
     {
       if (status == QCMAP_MSGR_STATION_MODE_CONNECTED_V01)
         printf("\nMobile AP Station Mode is Connected");
       else if(status == QCMAP_MSGR_STATION_MODE_DISCONNECTED_V01)
         printf("\nMobile AP Station Mode is Disconnected");
       else
         printf("\nIncorrect state returned: 0x%x", status);
     }
     else
       printf("  Failed to Get Station Mode Status .Error 0x%x.\n ", qmi_err_num);
   }
   break;

   /* Activate Hostapd with the new config. */
   case 9:
   {

     qcmap_msgr_activate_hostapd_ap_enum_v01 ap_type;
     qcmap_msgr_activate_hostapd_action_enum_v01 action_type;

     memset(scan_string, 0, QCMAP_MSGR_MAX_FILE_PATH_LEN);
     QCMAP_PRINTF_TAKE_INPUT("Select the AP Type for which settings need to be activated : \n"
               "1: Primary AP   2: Guest AP\n"
               "3: Guest 2 AP  4. Guest 3 AP \n5: All AP's\t:::");
     fgets(scan_string, sizeof(scan_string), stdin);

     ap_type = (qcmap_msgr_activate_hostapd_ap_enum_v01)atoi(scan_string);
     if ( ap_type < QCMAP_MSGR_PRIMARY_AP_V01 ||
          ap_type > QCMAP_MSGR_ALL_AP_V01 )
     {
       printf("\n\nInvalid AP Type : %d\n", ap_type);
       break;
     }

     memset(scan_string, 0, QCMAP_MSGR_MAX_FILE_PATH_LEN);
     QCMAP_PRINTF_TAKE_INPUT("Select the Action Type for hostapd which need to be activated : \n"
            "1: Start\t2: Stop\n"
            "3: Restart\t:::");
     fgets(scan_string, sizeof(scan_string), stdin);

     action_type = (qcmap_msgr_activate_hostapd_action_enum_v01)atoi(scan_string);
     if ( action_type < QCMAP_MSGR_HOSTAPD_START_V01 ||
          action_type > QCMAP_MSGR_HOSTAPD_RESTART_V01 )
     {
       printf("\n\nInvalid Action Type : %d\n", action_type);
       break;
     }

     if(QcMapClient->ActivateHostapdConfig(ap_type, action_type, &qmi_err_num))
     {
       printf("\nActivated Hostapd with the new config\n");
     }
     else
       printf("\nFailed to Activate Hostapd, Error: 0x%x", qmi_err_num);
     break;
   }

   /* Activate Supplicant with the new config. */
   case 10:
   {

     if(QcMapClient->ActivateSupplicantConfig(&qmi_err_num))
     {
       printf("\nActivated Supplicant with the new config\n");
     }
     else
       printf("\nFailed to Activate Supplicant, Error: 0x%x", qmi_err_num);
     break;
   }

   /* Get WLAN IF information. */
   case 11:
   {
     if (QcMapClient->GetActiveWlanIfInfo(&wlan_info_cfg, &qmi_err_num))
     {
       if (wlan_info_cfg.wlan_if_info_len > 0)
       {
         printf("Total IF active %d\n\n\n",wlan_info_cfg.wlan_if_info_len );
         printf("   +---------+---------+-------------+-----------+\n");
         printf("   |         |         |             |           |\n");
         printf("   | IF Name | AP type |  Card Type  |   State   |\n");
         printf("   |         |         |             |           |\n");
         printf("   +---------+---------+-------------+-----------+\n");

         for (int i = 0; i < wlan_info_cfg.wlan_if_info_len; i++)
         {
           if (wlan_info_cfg.wlan_if_info[i].state)
           {
              printf("   |%9s|%9s|%13s|%11s|\n", wlan_info_cfg.wlan_if_info[i].if_name,
                WLAN_AP_TYPE(wlan_info_cfg.wlan_if_info[i].wlan_ap_type),
                    WLAN_CARD_TYPE(wlan_info_cfg.wlan_if_info[i].wlan_dev_type),
                        WLAN_STATE_TYPE(wlan_info_cfg.wlan_if_info[i].state));

              printf("   +---------+---------+-------------+-----------+\n");
           }
         }
       }
       else
       {
         printf("No IF is found active\n");
       }
     }
     else
     {
       if(qmi_err_num ==  QMI_ERR_INVALID_HANDLE_V01)
       {
         printf("\nMobileAP is not enabled\n");
         break;
       }
       printf("Failed to get active wlan information, Error: 0x%x\r\n", qmi_err_num);
     }
     break;
   }

   /*Set Always on WLAN*/
   case 12:
   {
     boolean always_on_wlan_state = false;
     QCMAP_PRINTF_TAKE_INPUT("Please input Always on WLAN Flag (1-Enable/2-Disable):");
     fgets(scan_string, sizeof(scan_string), stdin);
     if (atoi(scan_string) == 2 || atoi(scan_string) == 1)
     {
       always_on_wlan_state = (atoi(scan_string) == 1) ? true : false;
       if(QcMapClient->SetAlwaysOnWLAN(always_on_wlan_state,&qmi_err_num))
         printf("\nSet Always on WLAN Succeds\n");
       else
         printf("\nFailed to Set Always on WLAN, Error: 0x%x", qmi_err_num);
     }
     else
     {
       printf("\n Invalid option : %d\n", atoi(scan_string));
     }
     break;
   }

   /*Get Always on WLAN*/
   case 13:
   {
     boolean always_on_wlan_status = false;
     if(QcMapClient->GetAlwaysOnWLAN(&always_on_wlan_status,&qmi_err_num))
     {
       printf("\nAlways on WLAN: %s.",
                (always_on_wlan_status)?"Enabled":"Disabled");
     }
     else
        printf("\nFailed to Get Always on WLAN, Error: 0x%x", qmi_err_num);
     break;
   }
   break;

   /* Set Peer to Peer Role */
   case 14:
   {
     int newline_index;
     int input;
     qcmap_p2p_config         p2p_config;

     memset(&p2p_config, 0, sizeof(qcmap_p2p_config));

     QCMAP_PRINTF_TAKE_INPUT("\n Please input Peer to Peer Status (1-Enable/0-Disable) \n");
     fgets(scan_string, sizeof(scan_string), stdin);
     newline_index = strlen(scan_string)-1;
     scan_string[newline_index] = '\0';
     if (atoi(scan_string) == 1)
     {
       QCMAP_PRINTF_TAKE_INPUT("\n Please input Peer to Peer Role (1-P2P-GO /2-P2P-CLI) \n");
       fgets(scan_string, sizeof(scan_string), stdin);
       if (atoi(scan_string) == 1)
       {
         LOG_MSG_INFO1("Enabling Peer-to-Peer Mode Group Owner", 0, 0, 0);
         p2p_config.p2p_status = TRUE;
         p2p_config.p2p_role_valid = TRUE;
         p2p_config.p2p_role = QCMAP_P2P_ROLE_GO_V01;
         if(QcMapClient->set_p2p_role(p2p_config, &qmi_err_num))
         {
           printf("\n Set peer-to-peer role P2P-GO succeeds \n");
         }
         else
         {
           printf("\n Set Peer-to-peer role P2P-GO failed   0x%x \n", qmi_err_num);
         }
       }
       else if(atoi(scan_string) == 2)
       {
         LOG_MSG_INFO1("Enabling Peer-to-Peer Mode Client", 0, 0, 0);
         p2p_config.p2p_status = TRUE;
         p2p_config.p2p_role_valid = TRUE;
         p2p_config.p2p_role = QCMAP_P2P_ROLE_CLI_V01;
         if(QcMapClient->set_p2p_role(p2p_config, &qmi_err_num))
         {
           printf("\n Set peer-to-peer role P2P-CLI succeeds \n");
         }
         else
         {
           printf("\n Set Peer-to-peer mode P2P-CLI failed   0x%x \n", qmi_err_num);
         }
      }
      else
      {
        printf("\n Invalid option : %d Please input a valid option \n", atoi(scan_string));
        break;
      }
    }
    else if( (atoi(scan_string) == 0) && (0 == strcmp(scan_string, "0")) )
    {
      if(QcMapClient->set_p2p_role(p2p_config, &qmi_err_num))
      {
        printf("\n Disable Peer-to-peer mode success");
      }
      else
      {
        printf("\n Disable Peer-to-peer mode failed");
      }
    }
    else
    {
      printf("\n Invalid option : %d Please input a valid option \n", atoi(scan_string));
    }
   }
   break;

   /* Get Peer to Peer Role */
   case 15:
   {
     qcmap_p2p_config p2p_config;

     memset(&p2p_config, 0, sizeof(qcmap_p2p_config));
     if(QcMapClient->get_p2p_role(&p2p_config, &qmi_err_num))
     {
       if(P2P_ROLE_ENABLE == p2p_config.p2p_status)
       {
         printf("\n peer-to-peer mode is enabled \n");
         if(QCMAP_P2P_ROLE_GO_V01 == p2p_config.p2p_role)
         {
           printf("\n peer-to-peer mode set is P2P-Group-Owner \n");
         }
         else if(QCMAP_P2P_ROLE_CLI_V01 == p2p_config.p2p_role)
         {
           printf("\n peer-to-peer mode set is  P2P-Client \n");
         }
       }
       else if(P2P_ROLE_DISABLE == p2p_config.p2p_status)
       {
         printf("\n Peer-to-peer mode is not enabled \n");
       }
     }
     else
     {
         printf("\n Get Peer-to-peer mode failed qmi_err_num: 0x%x,  \
                 \n p2p_status: %d p2p_role_valid: %d p2p_role: %d \n",
                  qmi_err_num, p2p_config.p2p_role_valid, p2p_config.p2p_role);
     }
   }
   break;

   /* CoEX Channel Avoidance */
   case 18:
   {
     boolean coex_state = false;

     QCMAP_PRINTF_TAKE_INPUT("Please input CoEX Channel Avoidance State (1-Enable/2-Disable):");
     fgets(scan_string, sizeof(scan_string), stdin);

     if (atoi(scan_string) == 2 || atoi(scan_string) == 1)
     {
       coex_state = (atoi(scan_string) == 1) ? true : false;

       if (QcMapClient->ConfigureCoEXChannelAvoidance(coex_state, &qmi_err_num))
         printf("\nSetting CoEX Channel Avoidance state is successful\n");
       else
         printf("\nSetting CoEX Channel Avoidance state failed with error[0x%x]\n", qmi_err_num);
     }
     else
     {
       printf("\nInvalid option : %d\n", atoi(scan_string));
     }

     break;
   }
   /* Set EZMesh Config */
   case 19:
   {
     memset(scan_string, 0, QCMAP_MSGR_MAX_FILE_PATH_LEN);
     qcmap_ezmesh_config ezmesh_config;
     qcmap_msgr_ezmesh_mode_enum_v01    enable;
     bool new_config = 0;
     uint8 min_ezmesh_ap = MIN_R1_EZMESH_AP;
     uint8 max_ezmesh_ap = MAX_R1_EZMESH_AP;
     memset(&ezmesh_config, 0, sizeof(qcmap_ezmesh_config));
     QCMAP_PRINTF_TAKE_INPUT("   Please input EZMesh State(1-Enable/0-Disable) : ");
     fgets(scan_string, sizeof(scan_string), stdin);
     if (atoi(scan_string) == 1)
     {
       enable = QCMAP_MSGR_EZMESH_ENABLE_V01;
       while(TRUE)
       {
         /* If configuring EZMesh for the first time
         * or User doesn't want to use the existing conf.*/
         QCMAP_PRINTF_TAKE_INPUT("\nDo you want to enable ezmesh with a new config (1-Yes / 0-No): ");
         fgets(scan_string, sizeof(scan_string), stdin);
         if (!VALID_NUMERIC_INPUT(scan_string))
         {
           printf("\nInvalid response\n");
           continue;
         }
         new_config = atoi(scan_string);
         if (!(new_config == 0 || new_config == 1))
         {
           printf("\n Provide correct input (0/1)");
           continue;
         }
         break;
       }
       if (new_config == 1)// Give a new config
       {
         QCMAP_PRINTF_TAKE_INPUT("   Please input EZMesh Capability(1-R1/2-R2/3-R3/4-R4) : ");
         fgets(scan_string, sizeof(scan_string), stdin);
         if (atoi(scan_string) == 1)
         {
           ezmesh_config.capability = QCMAP_MSGR_EZMESH_R1_capability_V01;
         }
         else if (atoi(scan_string) == 2)
         {
           ezmesh_config.capability = QCMAP_MSGR_EZMESH_R2_capability_V01;
         }
         else if (atoi(scan_string) == 3)
         {
           ezmesh_config.capability = QCMAP_MSGR_EZMESH_R3_capability_V01;
         }
         else if (atoi(scan_string) == 4)
         {
           ezmesh_config.capability = QCMAP_MSGR_EZMESH_R4_capability_V01;
         }
         else
         {
           printf("Invalid input : %d",atoi(scan_string));
           break;
         }

         if (IS_EZMESH_CAPABILITY_R2_R3_R4(ezmesh_config.capability))
         {
           while(TRUE)
           {
             QCMAP_PRINTF_TAKE_INPUT("   Do you want to enable ezmesh traffic separation/vlan mapping (1-Yes / 0-No): ");
             fgets(scan_string, sizeof(scan_string), stdin);
             if (!VALID_NUMERIC_INPUT(scan_string))
             {
               printf("\nInvalid response\n");
               continue;
             }
             if (!(atoi(scan_string) == 0 || atoi(scan_string) == 1))
             {
               printf("\n Provide correct input (0/1)");
               continue;
             }
             ezmesh_config.is_ezmesh_r2_cfg_valid = atoi(scan_string);
             break;
           }
           min_ezmesh_ap = MIN_R2_EZMESH_AP;
           max_ezmesh_ap = QCMAP_MSGR_MAX_EZMESH_AP_COUNT_V01;
         }

         if(ezmesh_config.is_ezmesh_r2_cfg_valid == 0)
         {
           min_ezmesh_ap = MIN_R1_EZMESH_AP;
           max_ezmesh_ap = MAX_R1_EZMESH_AP;
           printf("   How many AP's are configuring (Max AP Supported -12 for R1/R2/R3/R4) : ");
           READ_AND_VALIDATE_INT_VALUE(ezmesh_config.ezmesh_cfg.ap_config_len,min_ezmesh_ap,max_ezmesh_ap);
           ezmesh_config.is_ezmesh_cfg_valid = true;
           for (int i=0; i < ezmesh_config.ezmesh_cfg.ap_config_len; i++)
           {
             QCMAP_PRINTF_TAKE_INPUT(" Enter the  AP%d band(2GHz/5GHz). Enter only the value. \n",i+1);
             fgets(scan_string, sizeof(scan_string), stdin);
             ezmesh_config.ezmesh_cfg.ap_config[i].band = atoi(scan_string);
             QCMAP_PRINTF_TAKE_INPUT("Select EZMesh AP type : 1:Primary FH AP  5:Add FH AP  6:Add FH AP 2  7:Add FH AP 3  \n"
                    "8:BH AP(R1)  10:SM AP\n");
             fgets(scan_string, sizeof(scan_string), stdin);
             if ((atoi(scan_string) >= QCMAP_MSGR_PRIMARY_FRONTHAUL_AP_V01) &&
                 (atoi(scan_string) != QCMAP_MSGR_GUEST_FRONTHAUL_AP_V01) &&
                 (atoi(scan_string) != QCMAP_MSGR_GUEST_FRONTHAUL_AP_2_V01) &&
                 (atoi(scan_string) != QCMAP_MSGR_GUEST_FRONTHAUL_AP_3_V01) &&
                 (atoi(scan_string) != QCMAP_MSGR_BACKHAUL_AP_R2_V01) &&
                 (atoi(scan_string) <= QCMAP_MSGR_SMART_MONITOR_AP_V01))
             {
               ezmesh_config.ezmesh_cfg.ap_config[i].ap_type = atoi(scan_string);
             }
             else
             {
               printf("Invalid input : %d",atoi(scan_string));
               break;
             }
           }
         }
         else
         {
           min_ezmesh_ap = MIN_R2_EZMESH_AP;
           max_ezmesh_ap = QCMAP_MAX_EZMESH_AP_COUNT;
           printf("   How many AP's are configuring (Max AP Supported -12 for R1, 14 for R2/R3/R4) : ");
           READ_AND_VALIDATE_INT_VALUE(ezmesh_config.ezmesh_cfg.ap_config_len,min_ezmesh_ap,max_ezmesh_ap);
           ezmesh_config.is_ezmesh_cfg_valid = true;
           for (int i=0; i < ezmesh_config.ezmesh_cfg.ap_config_len; i++)
           {
             QCMAP_PRINTF_TAKE_INPUT(" Enter the  AP%d band(2GHz/5GHz). Enter only the value. \n",i+1);
             fgets(scan_string, sizeof(scan_string), stdin);
             ezmesh_config.ezmesh_cfg.ap_config[i].band = atoi(scan_string);
             QCMAP_PRINTF_TAKE_INPUT("Select EZMesh AP type : 1:Primary FH AP  2:Guest FH AP  3:Guest FH AP 2  4:Guest FH AP 3  \n"
               "5:Add FH AP  6:Add FH AP 2  7:Add FH AP 3  8:BH AP(R1)  9:BH AP(R2)  10:SM AP\n");
             fgets(scan_string, sizeof(scan_string), stdin);
             if ((atoi(scan_string) >= QCMAP_MSGR_PRIMARY_FRONTHAUL_AP_V01) &&
                 (atoi(scan_string) <= QCMAP_MSGR_SMART_MONITOR_AP_V01))
             {
               ezmesh_config.ezmesh_cfg.ap_config[i].ap_type = atoi(scan_string);
               /*  Profile is Internet Only for Guest FH AP and Full Access for others*/
               if ((atoi(scan_string) > QCMAP_MSGR_PRIMARY_FRONTHAUL_AP_V01) &&
                   (atoi(scan_string) < QCMAP_MSGR_ADDITIONAL_FH_AP_1_V01))
               {
                 ezmesh_config.ezmesh_cfg.ap_config[i].guest_ap_profile = QCMAP_MSGR_PROFILE_INTERNET_ONLY_V01;
               }
             }
             else
             {
               printf("Invalid input : %d",atoi(scan_string));
               break;
             }
           }
         }

         if (IS_EZMESH_CAPABILITY_R2_R3_R4(ezmesh_config.capability) &&
             ezmesh_config.is_ezmesh_r2_cfg_valid)
         {
           printf("   Enter Number of vlan mapping, Max allowed is 4 and Min allowed is 2 : ");
           READ_AND_VALIDATE_INT_VALUE(ezmesh_config.ezmesh_r2_cfg.mapping_len,MIN_R2_EZMESH_VLAN_MAPPING,QCMAP_MSGR_MAX_EZMESH_VLAN_MAPPING_V01);
           for (int i=0; i < ezmesh_config.ezmesh_r2_cfg.mapping_len;i++)
           {
             printf("Select EZMesh AP type for Mapping %d: \n"
                  " 1:Primary FH AP  2:Guest FH AP \n"
                  " 3:Guest FH AP 2  4:Guest FH AP 3  \n",i+1);
             READ_AND_VALIDATE_INT_VALUE(ezmesh_config.ezmesh_r2_cfg.mapping[i].fh_ap_type,
             QCMAP_MSGR_PRIMARY_FRONTHAUL_AP_V01,QCMAP_MSGR_GUEST_FRONTHAUL_AP_3_V01);
             printf(" Enter the vlan id[1-%d] for mapping %d \n",MAX_VLAN_ID,i+1);
             READ_AND_VALIDATE_INT_VALUE(ezmesh_config.ezmesh_r2_cfg.mapping[i].vlan_id,1,MAX_VLAN_ID);
           }
         }

        if (IS_EZMESH_CAPABILITY_R3_R4(ezmesh_config.capability) &&
            ezmesh_config.is_ezmesh_r2_cfg_valid)
        {
          QCMAP_PRINTF_TAKE_INPUT("  Please input EZMesh Service Prioritization State(1-Enable/0-Disable) : ");
          ZERO_INIT_ARG(scan_string);
          fgets(scan_string, sizeof(scan_string), stdin);
          if (!VALID_NUMERIC_INPUT(scan_string))
          {
            printf("\nInvalid response\n");
            break;
          }
          if ((atoi(scan_string) == 1) || (atoi(scan_string) == 0))
          {
            ezmesh_config.is_service_prioritization_state_valid = true;
            ezmesh_config.service_prioritization_state = atoi(scan_string);
          }
        }
       }
     }
     else if (strtol(scan_string, NULL, 10) == 0)
     {
       while(TRUE)
       {
         /* If configuring EZMesh for the first time
         * or User doesn't want to use the existing conf.*/
         QCMAP_PRINTF_TAKE_INPUT("\nDo you want to Clean up Ezmesh config (1-Yes / 0-No): ");
         fgets(scan_string, sizeof(scan_string), stdin);
         if (!VALID_NUMERIC_INPUT(scan_string))
         {
          printf("\nInvalid response\n");
          continue;
         }
         new_config = atoi(scan_string);
         if (!(new_config == 0 || new_config == 1))
         {
           printf("\n Provide correct input (0/1)");
           continue;
         }
         break;
       }
       enable = QCMAP_MSGR_EZMESH_DISABLE_V01;
     }
     if(QcMapClient->SetEZMeshConfig(enable,new_config,ezmesh_config, &qmi_err_num))
     {
       printf("\n Set EZMesh config is successful \n");
     }
     else
     {
       if (qmi_err_num == QMI_ERR_INVALID_ARG_V01)
       {
         printf("No existing config prevails for EZMesh config: 0x%x", qmi_err_num);
       }
       else if (qmi_err_num == QMI_ERR_INVALID_HANDLE_V01)
       {
         printf("\nMobileAP is not enabled\n");
         break;
       }
         printf("\n SetEZMesh config failed qmi_err_num: 0x%x, \n", qmi_err_num);
     }
     break;
   }

   /* Get EZMesh Config */
   case 20:
   {
     memset(scan_string, 0, QCMAP_MSGR_MAX_FILE_PATH_LEN);
     qcmap_ezmesh_config ezmesh_config;
     qcmap_msgr_ezmesh_status_enum_v01     enable;
     memset(&ezmesh_config, 0, sizeof(qcmap_ezmesh_config));
     if(QcMapClient->GetEZMeshConfig(&enable, &ezmesh_config, &qmi_err_num))
     {
       printf("\n EZMesh is %s\n", EZMESH_STATE_TYPE(enable));
       printf("Capability is %s\n", EZMESH_CAPABILITY_TYPE(ezmesh_config.capability));
       if (ezmesh_config.is_service_prioritization_state_valid &&
           IS_EZMESH_CAPABILITY_R3_R4(ezmesh_config.capability))
       {
         printf("Service Priority state %d\n", ezmesh_config.service_prioritization_state);
       }
       if (ezmesh_config.is_ezmesh_cfg_valid && ezmesh_config.ezmesh_cfg.ap_config_len)
       {
         printf("Total AP count %d\n\n\n",ezmesh_config.ezmesh_cfg.ap_config_len );
         printf("   +------+---------+-----------------+-----------+\n");
         printf("   |      |         |                 |           |\n");
         printf("   | Band | Profile |  FH/BH/SM Type  |  Ap type  |\n");
         printf("   |      |         |                 |           |\n");
         printf("   +------+---------+-----------------+-----------+\n");

         for (int i = 0; i < ezmesh_config.ezmesh_cfg.ap_config_len; i++)
         {
              printf("   |%6d|%9s|%17s|%11s|\n", ezmesh_config.ezmesh_cfg.ap_config[i].band,
                WLAN_AP_PROFILE(ezmesh_config.ezmesh_cfg.ap_config[i].guest_ap_profile),
                    EZMESH_AP_TYPE(ezmesh_config.ezmesh_cfg.ap_config[i].ap_type),
                        WLAN_AP_TYPE(ezmesh_config.ezmesh_cfg.ap_config[i].wlan_ap_type));
              printf("   +------+---------+-----------------+-----------+\n");

         }
         printf("\nVLAN Mapping enable state %d\n", ezmesh_config.is_ezmesh_r2_cfg_valid);
         if (ezmesh_config.is_ezmesh_r2_cfg_valid &&
             IS_EZMESH_CAPABILITY_R2_R3_R4(ezmesh_config.capability))
         {
           printf("Total Mapping %d\n\n\n",ezmesh_config.ezmesh_r2_cfg.mapping_len );
           printf("   +-----------------+-----------+\n");
           printf("   |                 |           |\n");
           printf("   |  FH/BH/SM Type  |  Vlan ID  |\n");
           printf("   |                 |           |\n");
           printf("   +-----------------+-----------+\n");

           for (int i = 0; i < ezmesh_config.ezmesh_r2_cfg.mapping_len; i++)
           {
             printf("   |%17s|%11d|\n",
                    EZMESH_FH_AP_TYPE(ezmesh_config.ezmesh_r2_cfg.mapping[i].fh_ap_type),
                    ezmesh_config.ezmesh_r2_cfg.mapping[i].vlan_id);
             printf("   +-----------------+-----------+\n");

           }
         }
       }
       else
       {
         printf("No AP is found \n");
       }
     }
     else
     {
       if(qmi_err_num == QMI_ERR_INVALID_HANDLE_V01)
       {
         printf("\nMobileAP is not enabled\n");
         break;
       }
       printf("\n GetEZMesh config failed qmi_err_num: 0x%x, \n",
                qmi_err_num);
     }
     break;
   }

   /*Activate Hostapd Restart for EZMesh with the new config */
   case 21:
   {
     qcmap_msgr_activate_hostapd_ap_enum_v01 ap_type = QCMAP_MSGR_ACTIVATE_HOSTAPD_AP_ENUM_MIN_ENUM_VAL_V01;
     qcmap_hostapd_ap_config_list ap_list;
     bool invalid_input = false;
     ZERO_INIT_ARG(ap_list)
     memset(scan_string, 0, QCMAP_MSGR_MAX_FILE_PATH_LEN);
     printf(" Select the number of ap for activate hostapd : ");
     READ_AND_VALIDATE_INT_VALUE(ap_list.list_len,1,QCMAP_MSGR_MAX_EZMESH_AP_COUNT_V01);
     printf("Select the AP Type for which settings need to be activated : \n"
               "1: Primary AP      2: Guest AP\n"
               "3: Guest 2 AP      4. Guest 3 AP \n5: All AP's\n"
               "6: Guest 4 AP      7: Guest 5 AP\n"
               "8: Guest 6 AP      9: Guest 7 AP\n"
               "10: Guest 8 AP    11: Guest 9 AP\n"
               "12: Guest 10 AP   13: Guest 11 AP\n"
               "14: Guest 12 AP   15: Guest 13 AP\t:::\n");
#if 0
               "16: Guest 14 AP   17: Guest 15 AP\n"
               "18: Guest 16 AP   19: Guest 17 AP\n"
               "20: Guest 18 AP   21: Guest 19 AP\n"
               "22: Guest 20 AP\t:::\n");
#endif

     if (ap_list.list_len == 1)
     {
       memset(scan_string, 0, QCMAP_MSGR_MAX_FILE_PATH_LEN);
       QCMAP_PRINTF_TAKE_INPUT("Select AP type \n");
       fgets(scan_string, sizeof(scan_string), stdin);
       ap_list.ap_type[0] = (qcmap_msgr_activate_hostapd_ap_enum_v01)atoi(scan_string);
       if ( ap_list.ap_type[0] < QCMAP_MSGR_PRIMARY_AP_V01 ||
            ap_list.ap_type[0] > QCMAP_MSGR_GUEST_AP_13_V01 )
       {
         printf("\n\nInvalid AP Type : %d\n", ap_type);
         invalid_input = true;
         break;
       }
     }

     for (int i=0; i < ap_list.list_len && ap_list.list_len > 1; i++)
     {
       QCMAP_PRINTF_TAKE_INPUT("Select AP type for %d\n",i+1);
       fgets(scan_string, sizeof(scan_string), stdin);
       ap_list.ap_type[i] = atoi(scan_string);
       if ( ap_list.ap_type[i] < QCMAP_MSGR_PRIMARY_AP_V01 ||
            ap_list.ap_type[i] == QCMAP_MSGR_ALL_AP_V01 ||
            ap_list.ap_type[i] > QCMAP_MSGR_GUEST_AP_13_V01)
       {
         printf("\n\nInvalid AP Type : %d\n", ap_type);
         invalid_input = true;
         break;
       }
     }
     if (invalid_input)
       break;
     if(QcMapClient->ActivateHostapdConfig(ap_type, QCMAP_MSGR_HOSTAPD_RESTART_V01, &qmi_err_num,false,&ap_list))
     {
       printf("\nActivated Hostapd with the new config\n");
     }
     else
     {
       if(qmi_err_num == QMI_ERR_INVALID_HANDLE_V01)
       {
         printf("\nMobileAP is not enabled\n");
         break;
       }
       printf("\nFailed to Activate Hostapd, Error: 0x%x", qmi_err_num);
     }
     break;
   }

   /* Set EZMesh Service Prioritization state */
   case 22:
   {
     memset(scan_string, 0, QCMAP_MSGR_MAX_FILE_PATH_LEN);
     qcmap_ezmesh_config ezmesh_config;
     memset(&ezmesh_config, 0, sizeof(qcmap_ezmesh_config));
     QCMAP_PRINTF_TAKE_INPUT("  Please input EZMesh Service Prioritization State(1-Enable/0-Disable) : ");
     ZERO_INIT_ARG(scan_string);
     fgets(scan_string, sizeof(scan_string), stdin);
     if (!VALID_NUMERIC_INPUT(scan_string) || atoi(scan_string) != 1 && atoi(scan_string) != 0)
     {
       printf("\nInvalid response. Please enter 1 or 0.\n");
       break;
     }
     if (atoi(scan_string) == 1)
     {
       ezmesh_config.service_prioritization_state = atoi(scan_string);
     }

     if(QcMapClient->SetEZMeshServicePriority(ezmesh_config, &qmi_err_num))
     {
       printf("\n Set EZMesh Service Prioritization state is successful \n");
     }
     else if(qmi_err_num == QMI_ERR_INVALID_HANDLE_V01)
     {
       printf("\nMobileAP is not enabled\n");
       break;
     }
     else
     {
       printf("\n Set EZMesh Service Prioritization state failed qmi_err_num: 0x%x, \n",
              qmi_err_num);
     }
     break;
   }

  case 23:
  {
     qcmap_wlan_ex3_config_t wlan_config_ex3;
     memset(&wlan_config_ex3, 0, sizeof(wlan_config_ex3));
     int input_var;
     in_addr addr;

     /*Set WLAN ConfigEx3 */
     QCMAP_PRINTF_TAKE_INPUT("Do you want to set MLD config 0/1: ");
     READ_AND_VALIDATE_INT_VALUE(input_var, 0, 1);
     if(input_var == 1)
     {
       //Take input for MLD config
       QCMAP_PRINTF_TAKE_INPUT("Do you want to configure AP in MLD 0/1 : ");
       READ_AND_VALIDATE_INT_VALUE(input_var, 0, 1);
       if(input_var == 1)
       {
          QCMAP_PRINTF_TAKE_INPUT("Enter number of MLD-AP you want configure 0-7: ");
          READ_AND_VALIDATE_INT_VALUE(input_var, 1, 7);
          wlan_config_ex3.mld_ap_config_len = input_var;
          for (int i=0; i<wlan_config_ex3.mld_ap_config_len; i++)
          {
             QCMAP_PRINTF_TAKE_INPUT("Enter number of link in MLD-AP%d ",i+1);
             READ_AND_VALIDATE_INT_VALUE(input_var, 1, 3);
             wlan_config_ex3.mld_ap_config[i].no_of_mld_link = input_var;
             for(int j=0; j<wlan_config_ex3.mld_ap_config[i].no_of_mld_link; j++)
             {
                QCMAP_PRINTF_TAKE_INPUT("Enter Link:%d band: ",j+1);
                fgets(scan_string, sizeof(scan_string), stdin);
                wlan_config_ex3.mld_ap_config[i].band[j] = atoi(scan_string);
             }
             QCMAP_PRINTF_TAKE_INPUT("Enter MLD-AP%d access profile 1-Internet/0-Full access " ,i+1);
             READ_AND_VALIDATE_INT_VALUE(input_var, 0, 1);
             wlan_config_ex3.mld_ap_config[i].accessprofile = input_var;
          }
       }
       QCMAP_PRINTF_TAKE_INPUT("Do you want to configure MLD-STA 0/1: ");
       READ_AND_VALIDATE_INT_VALUE(input_var, 0, 1);
       if(input_var == 1)
       {
          QCMAP_PRINTF_TAKE_INPUT("Enter number of link in MLD-STA 1-3: ");
          READ_AND_VALIDATE_INT_VALUE(input_var, 1, 3);
          wlan_config_ex3.mld_sta_config.no_of_mld_link = input_var;
          for(int j=0; j<wlan_config_ex3.mld_sta_config.no_of_mld_link; j++)
          {
             QCMAP_PRINTF_TAKE_INPUT("Enter link%d band: ", j+1);
             fgets(scan_string, sizeof(scan_string), stdin);
             wlan_config_ex3.mld_sta_config.band[j] = atoi(scan_string);
          }
          QCMAP_PRINTF_TAKE_INPUT("Select bridge or router mode 1-Router/2-Bridge  ");
          fgets(scan_string, sizeof(scan_string), stdin);
          wlan_config_ex3.mld_sta_config.ap_sta_bridge_mode = atoi(scan_string);
          QCMAP_PRINTF_TAKE_INPUT("Select STA mode 1-DYNAMIC/2-STATIC ");
          fgets(scan_string, sizeof(scan_string), stdin);
          wlan_config_ex3.mld_sta_config.conn_type = atoi(scan_string);
          if(wlan_config_ex3.mld_sta_config.conn_type == QCMAP_MSGR_STA_CONNECTION_STATIC_V01)
          {
              QCMAP_PRINTF_TAKE_INPUT("   Please input a valid Static IP address:  ");
              memset(&addr,0,sizeof(in_addr));
              read_addr(AF_INET, (uint8 *)&addr.s_addr);
              wlan_config_ex3.mld_sta_config.static_ip_config.ip_addr = addr.s_addr;
              QCMAP_PRINTF_TAKE_INPUT("   Please input a valid Gateway address:  ");
              memset(&addr,0,sizeof(in_addr));
              read_addr(AF_INET, (uint8 *)&addr.s_addr);
              wlan_config_ex3.mld_sta_config.static_ip_config.gw_ip = addr.s_addr;
              QCMAP_PRINTF_TAKE_INPUT("   Please input a valid Netmask:  ");
              memset(&addr,0,sizeof(in_addr));
              read_addr(AF_INET, (uint8 *)&addr.s_addr);
              wlan_config_ex3.mld_sta_config.static_ip_config.netmask = addr.s_addr;
              QCMAP_PRINTF_TAKE_INPUT("   Please input a valid DNS Address: ");
              memset(&addr,0,sizeof(in_addr));
              read_addr(AF_INET, (uint8 *)&addr.s_addr);
              wlan_config_ex3.mld_sta_config.static_ip_config.dns_addr = addr.s_addr;
          }
          wlan_config_ex3.is_mld_sta_configured = true;
       }
     }
     QCMAP_PRINTF_TAKE_INPUT("Do you want to set Non-MLD config 0/1: ");
     READ_AND_VALIDATE_INT_VALUE(input_var, 0, 1);
     if(input_var == 1)
     {
        //Take input for Non-MLD config
        QCMAP_PRINTF_TAKE_INPUT("Do you want to configure AP 0/1:  ");
        READ_AND_VALIDATE_INT_VALUE(input_var, 0, 1);
        if(input_var == 1)
        {
           QCMAP_PRINTF_TAKE_INPUT("Enter number of AP you want configure: [1-21] ");
           READ_AND_VALIDATE_INT_VALUE(input_var, 1, 21);
           wlan_config_ex3.ap_config_len = input_var;
           for (int i=0; i<wlan_config_ex3.ap_config_len; i++)
           {
              QCMAP_PRINTF_TAKE_INPUT("Enter AP%d band ",i+1);
              fgets(scan_string, sizeof(scan_string), stdin);
              wlan_config_ex3.ap_config[i].band = atoi(scan_string);
              QCMAP_PRINTF_TAKE_INPUT("Enter AP%d access profile 1-Internet/0-Full access: ",i+1);
              READ_AND_VALIDATE_INT_VALUE(input_var, 0, 1);
              wlan_config_ex3.ap_config[i].accessprofile = input_var;
           }
        }
        QCMAP_PRINTF_TAKE_INPUT("Do you want to configure STA 0/1: ");
        READ_AND_VALIDATE_INT_VALUE(input_var, 0, 1);
        if(input_var == 1)
        {
           QCMAP_PRINTF_TAKE_INPUT("Enter STA band: ");
           fgets(scan_string, sizeof(scan_string), stdin);
           wlan_config_ex3.station_config.band = atoi(scan_string);
           QCMAP_PRINTF_TAKE_INPUT("Select bridge or router mode 1-Router/2-Bridge  ");
           fgets(scan_string, sizeof(scan_string), stdin);
           wlan_config_ex3.station_config.ap_sta_bridge_mode = atoi(scan_string);
           QCMAP_PRINTF_TAKE_INPUT("Select STA mode 1-DYNAMIC/2-STATIC  ");
           fgets(scan_string, sizeof(scan_string), stdin);
           wlan_config_ex3.station_config.conn_type = atoi(scan_string);
           if(wlan_config_ex3.station_config.conn_type == QCMAP_MSGR_STA_CONNECTION_STATIC_V01)
           {
               QCMAP_PRINTF_TAKE_INPUT("   Please input a valid Static IP address: ");
               memset(&addr,0,sizeof(in_addr));
               read_addr(AF_INET, (uint8 *)&addr.s_addr);
               wlan_config_ex3.station_config.static_ip_config.ip_addr = addr.s_addr;
               QCMAP_PRINTF_TAKE_INPUT("   Please input a valid Gateway address: ");
               memset(&addr,0,sizeof(in_addr));
               read_addr(AF_INET, (uint8 *)&addr.s_addr);
               wlan_config_ex3.station_config.static_ip_config.gw_ip = addr.s_addr;
               QCMAP_PRINTF_TAKE_INPUT("   Please input a valid Netmask:  ");
               memset(&addr,0,sizeof(in_addr));
               read_addr(AF_INET, (uint8 *)&addr.s_addr);
               wlan_config_ex3.station_config.static_ip_config.netmask = addr.s_addr;
               QCMAP_PRINTF_TAKE_INPUT("   Please input a valid DNS Address:");
               memset(&addr,0,sizeof(in_addr));
               read_addr(AF_INET, (uint8 *)&addr.s_addr);
               wlan_config_ex3.station_config.static_ip_config.dns_addr = addr.s_addr;
           }
           wlan_config_ex3.is_sta_configured = true;
        }
     }
    // set WLAN config Ex3.
    if (QcMapClient->SetWLANConfigEx3(wlan_config_ex3,&qmi_err_num))
     {
      if (qmi_err_num ==  QMI_ERR_NONE_V01)
      {
        printf("\n WLAN Config Set Successfully\n");
      }
      else
      {
        printf("\nWLAN Config set fails, Error: 0x%x", qmi_err_num);
      }
    }
    else
    {
      if (qmi_err_num == QMI_ERR_INVALID_HANDLE_V01)
      {
        printf("\nMobileAP is not enabled");
        break;
      }
      printf("\nWLAN Config set fails, Error: 0x%x", qmi_err_num);
    }
   break;
  }
  case 24:
  {
     qcmap_wlan_ex3_config_t wlan_config;
     in_addr addr;

     ZERO_INIT_ARG(wlan_config);
     ZERO_INIT_ARG(addr);
     if (QcMapClient->GetWLANConfigEx3(&wlan_config, &qmi_err_num))
     {
       if(wlan_config.ap_config_len > 0 || wlan_config.is_sta_configured)
       {
          printf("\nWLAN MODE: %s",wlan_mode_str[wlan_config.wlan_mode]);
          if(wlan_config.ap_config_len > 0)
          {
            printf("\n========== NON-MLD WLAN INFO BEGIN ===================\n");
            for(int i=0; i<wlan_config.ap_config_len; i++)
            {
              printf("\nAP %d \nband:%d \naccessprofile:%s",
                                      i+1,
                                      wlan_config.ap_config[i].band,
                                      (wlan_config.ap_config[i].accessprofile == QCMAP_MSGR_PROFILE_FULL_ACCESS_V01 ?
                                      "Full Access": "Internet Only"));
            }
          }
          if(wlan_config.is_sta_configured)
          {
             if ( wlan_config.station_config.conn_type == QCMAP_MSGR_STA_CONNECTION_DYNAMIC_V01)
            {
                printf("\nConnection Type : DYNAMIC (DHCP)\n");
            }
             else
            {
               printf("\nConnection Type : STATIC\n");
               printf("\n STATIC STA IP Configuration\n");
               addr.s_addr = htonl(wlan_config.station_config.static_ip_config.ip_addr);
               printf("\nIP Address: %s\n", inet_ntoa(addr));
               addr.s_addr = htonl(wlan_config.station_config.static_ip_config.gw_ip);
               printf("\nGateway IP : %s\n", inet_ntoa(addr));
               addr.s_addr = htonl(wlan_config.station_config.static_ip_config.netmask);
               printf("\nNetmask: %s\n", inet_ntoa(addr));
               addr.s_addr = htonl(wlan_config.station_config.static_ip_config.dns_addr);
               printf("\nDNS Address : %s\n", inet_ntoa(addr));
               printf("\nSTA is configured in %s \n",(wlan_config.station_config.ap_sta_bridge_mode?"Bridge Mode":"Router Mode"));
            }
          }
          printf("\n============     WLAN INFO END         ==================\n");
       }

       // Get MLD Configuration.
       if(wlan_config.mld_ap_config_len > 0 || wlan_config.is_mld_sta_configured)
       {
          printf("\n========== MLD WLAN INFO BEGIN ===================\n");
          printf("\nMLD-WLAN MODE: %s",wlan_mode_str[wlan_config.mld_wlan_mode]);
          if(wlan_config.mld_ap_config_len > 0)
          {
           for(int i=0; i<wlan_config.mld_ap_config_len; i++)
           {
               printf("\nMLD AP %d \nToatal Link:%d \naccessprofile:%s",
                                     i+1,
                                     wlan_config.mld_ap_config[i].no_of_mld_link,
                                     (wlan_config.mld_ap_config[i].accessprofile == QCMAP_MSGR_PROFILE_FULL_ACCESS_V01 ?
                                      "Full Access": "Internet Only"));
           }
          }
          if(wlan_config.is_mld_sta_configured)
          {
            if ( wlan_config.mld_sta_config.conn_type == QCMAP_MSGR_STA_CONNECTION_DYNAMIC_V01)
           {
               printf("\nConnection Type : DYNAMIC (DHCP)\n");
           }
            else
           {
              printf("\nConnection Type : STATIC\n");
              printf("\n STATIC STA IP Configuration\n");
              addr.s_addr = htonl(wlan_config.mld_sta_config.static_ip_config.ip_addr);
              printf("\nIP Address: %s\n", inet_ntoa(addr));
              addr.s_addr = htonl(wlan_config.mld_sta_config.static_ip_config.gw_ip);
              printf("\nGateway IP : %s\n", inet_ntoa(addr));
              addr.s_addr = htonl(wlan_config.mld_sta_config.static_ip_config.netmask);
              printf("\nNetmask: %s\n", inet_ntoa(addr));
              addr.s_addr = htonl(wlan_config.mld_sta_config.static_ip_config.dns_addr);
              printf("\nDNS Address : %s\n", inet_ntoa(addr));
              printf("\nSTA is configured in %s \n",(wlan_config.mld_sta_config.ap_sta_bridge_mode?"Bridge Mode":"Router Mode"));
           }
          }
          printf("\n========== MLD WLAN INFO END ===================\n");
       }
     }
     else
     {
       if (qmi_err_num == QMI_ERR_NONE_V01)
       {
         printf("\n WLAN Config is NOT SET");
       }
       else if (qmi_err_num == QMI_ERR_INVALID_HANDLE_V01)
       {
        printf("\nMobileAP is not enabled");
       }
       else
       {
         printf("\nGet WLAN Config failed");
       }
      break;
     }
    break;
  }

   default:
   {
     printf("Invalid response %d\n", wlanOpt);
     break;
   }
  }
}

void firewallConfig( int firewallOpt )
{
  qmi_error_type_v01 qmi_err_num;
  /* Initialize QMI Error Number. */
  qmi_err_num = QMI_ERR_NONE_V01;
  char scan_string[QCMAP_MSGR_MAX_FILE_PATH_LEN];

  /* Firewall Configuration options */
  switch(firewallOpt)
  {
  /* Add Firewall Entry */
  case 1:
  {
    qcmap_msgr_firewall_conf_t     extd_firewall_add;

    /*Default rule is to accept the packets */
    QCMAP_PRINTF_TAKE_INPUT("Please Enter Firewall entry:\n");
      if ( read_firewall_conf( &extd_firewall_add ) )
      {
        if ( QcMapClient->AddFireWallEntry(&extd_firewall_add,&qmi_err_num )== true)
        {
          printf("\n Add Firewall Entry success.");
        }
        else if (qmi_err_num == QMI_ERR_INTERFACE_NOT_FOUND_V01)
        {
          printf("\nBackhaul down. Saved firwall entry in configuration file.");
        }
        else if (qmi_err_num == QMI_ERR_INVALID_HANDLE_V01)
        {
          printf("\nMobileAP is not enabled\n");
          break;
        }
        else
          printf("\nAdd Firewall Entry failed, Error: 0x%x", qmi_err_num);
      }
  }
  break;

  /* Set Firewall Config */
  case 2:
  {
    boolean enable_firewall, pkts_allowed = false;

    QCMAP_PRINTF_TAKE_INPUT("   Please input Firewall State (1-Enable/0-Disable) : ");
    fgets(scan_string, sizeof(scan_string), stdin);
    if (!VALID_NUMERIC_INPUT(scan_string) || atoi(scan_string) != 1 && atoi(scan_string) != 0)
    {
      printf("\nInvalid response. Please enter 1 or 0.\n");
      break;
    }
    enable_firewall = atoi(scan_string);
    if (enable_firewall)
    {
      QCMAP_PRINTF_TAKE_INPUT("   Please input Packets Allowed Setting(1-ACCEPT/0-Drop) : ");
      fgets(scan_string, sizeof(scan_string), stdin);
      pkts_allowed = atoi(scan_string);
    }
    if(QcMapClient->SetFirewall(enable_firewall, pkts_allowed, &qmi_err_num))
    {
      printf("Set Firewall state to:%d success\n", enable_firewall);
    }
    else
    {
      if(qmi_err_num == QMI_ERR_INVALID_HANDLE_V01)
      {
        printf("\nMobileAP is not enabled\n");
        break;
      }
      printf("Set Firewall Error: 0x%x", qmi_err_num);
    }
    break;
  }

  /* Get Firewall Config */
  case 3:
  {
    boolean enable_firewall, pkts_allowed;
    if (QcMapClient->GetFirewall(&enable_firewall, &pkts_allowed,&qmi_err_num))
    {
      if(enable_firewall)
      {
        printf("Firewall is Enabled\n");
        if(pkts_allowed)
        {
          printf("Firewall is configured to ACCEPT packets\n");
        }
        else
        {
          printf("Firewall is configured to DROP packets\n");
        }
      }
      else
        printf("Firewall is Disabled \n");
    }
    else
     printf("Get Firewall configuration failed,Error 0x%x", qmi_err_num);
    break;
  }

  /* Display Firewalls */
  case 4:
  {
    qcmap_msgr_firewall_conf_t extd_firewall_handle_list;
    int   handle_list_len;
    int   result;

    memset(&extd_firewall_handle_list, 0, sizeof(qcmap_msgr_firewall_conf_t));
    handle_list_len=0;
    while(1)
    {
      QCMAP_PRINTF_TAKE_INPUT("\n Please input IP family type [4-IPv4 6-IPv6]:");
      fgets(scan_string, sizeof(scan_string), stdin);
      result = atoi(scan_string);
      if(result == IP_V4 || result == IP_V6 )
      break;
    }

    if(result == IP_V4)
    {
        extd_firewall_handle_list.extd_firewall_handle_list.ip_family = IP_V4;
    }
     else if(result == IP_V6)
    {
     extd_firewall_handle_list.extd_firewall_handle_list.ip_family = IP_V6;
    }
    if(QcMapClient->GetFireWallHandlesList(&extd_firewall_handle_list.extd_firewall_handle_list, &qmi_err_num))
    {
      handle_list_len = extd_firewall_handle_list.extd_firewall_handle_list.num_of_entries;
    }
    else
    {
      printf("\nFirewall Handle list get failed, Error: 0x%x", qmi_err_num);
      break;
    }
    if( DisplayFirewall( handle_list_len, extd_firewall_handle_list ) )
    {
      printf("\n Firewall Display success \n");
    }
    break;
  }

  /* Delete FireWall Entry */
  case 5:
  {
    int handle=0;
    memset(scan_string, 0, QCMAP_MSGR_MAX_FILE_PATH_LEN);

    QCMAP_PRINTF_TAKE_INPUT(" Please Enter the Firewall Handle : ");
    fgets(scan_string, sizeof(scan_string), stdin);
    if (!VALID_NUMERIC_INPUT(scan_string))
    {
      printf("\nInvalid response\n");
      break;
    }
    handle= atoi(scan_string);
    if(handle <0)
      printf("\n Entered Handle is invalid \n");

    if (!QcMapClient->DeleteFireWallEntry(handle, &qmi_err_num))
    {
      if (qmi_err_num == QMI_ERR_INTERFACE_NOT_FOUND_V01)
      {
        printf("\nBackhaul down. firewall entry deleted from configuration file.");
      }
      else if (qmi_err_num == QMI_ERR_INVALID_HANDLE_V01)
      {
        printf("\nInvalid firewall handle.");
      }
      else
        printf("\nDelete firewall Fails, Error: 0x%x", qmi_err_num);
    }
    else
      printf("\nDelete firewall Successfully");
  }
  break;

  /* Set HW Filter State
   * Every time we enable HW Filter State, the previous config is reset with
   * the new config. If we do not enable, then the default is disabled state
   * where we send the disable state for each of the sub-configs (mac filter state,
   * ip segment state and iface filter state) to IPA.
   */
  case 6:
  {
    int num_of_clients = 0;
    int num_ip_seg = 0;
    in_addr start_ip, end_ip, gateway_ip, netmask_ip;
    qcmap_msgr_config_state_enum_v01   state = QCMAP_MSGR_CONFIG_STATE_ENUM_MAX_ENUM_VAL_V01;
    qcmap_msgr_config_state_enum_v01   mac_flt_state = QCMAP_MSGR_CONFIG_STATE_ENUM_MAX_ENUM_VAL_V01;
    qcmap_msgr_config_state_enum_v01   ip_seg_flt_state = QCMAP_MSGR_CONFIG_STATE_ENUM_MAX_ENUM_VAL_V01;
    qcmap_msgr_config_state_enum_v01   if_name_flt_state = QCMAP_MSGR_CONFIG_STATE_ENUM_MAX_ENUM_VAL_V01;
    qcmap_hw_filter_config             hw_filter_config;
    qcmap_msgr_lan_config_v01          lan_config;
    memset(&hw_filter_config, 0, sizeof(qcmap_hw_filter_config));
    memset(&lan_config,0,sizeof(qcmap_msgr_lan_config_v01));
    memset(&gateway_ip, 0 ,sizeof(in_addr));
    memset(&netmask_ip, 0 ,sizeof(in_addr));
    QCMAP_PRINTF_TAKE_INPUT("\nEnter HW Filter State (0-Disable/1-Enable): ");
    fgets(scan_string, sizeof(scan_string), stdin);
    if (!VALID_NUMERIC_INPUT(scan_string))
    {
      printf("\nInvalid response\n");
      break;
    }
    state = (qcmap_msgr_config_state_enum_v01)atoi(scan_string);
    
    if(state != QCMAP_MSGR_CONFIG_ENABLE_V01 && state != QCMAP_MSGR_CONFIG_DISABLE_V01)
    {
      printf("\nIncorrect State Entered %d", state);
      break;
    }

    if(state == QCMAP_MSGR_CONFIG_ENABLE_V01)
    {
      QCMAP_PRINTF_TAKE_INPUT("\nEnter MAC Filtering State (0-Disable/1-Enable): ");
      fgets(scan_string, sizeof(scan_string), stdin);
      mac_flt_state = (qcmap_msgr_config_state_enum_v01)atoi(scan_string);
      hw_filter_config.mac_flt_state = mac_flt_state;
      if(mac_flt_state == QCMAP_MSGR_CONFIG_ENABLE_V01)
      {
        while(TRUE)
        {
          QCMAP_PRINTF_TAKE_INPUT("\nMax number of clients can be entered is %d", QCMAP_MAX_HW_MAC_FILTER_CLIENTS_V01);
          QCMAP_PRINTF_TAKE_INPUT("\nEnter Number of Clients for Hardware MAC Filtering State: ");
          fgets(scan_string, sizeof(scan_string), stdin);
          hw_filter_config.num_of_clients = atoi(scan_string);
          if(hw_filter_config.num_of_clients > QCMAP_MAX_HW_MAC_FILTER_CLIENTS_V01 ||
             hw_filter_config.num_of_clients < 1)
          {
            QCMAP_PRINTF_TAKE_INPUT("\nEntered invalid number of clients: %d\n", hw_filter_config.num_of_clients);
          }
          else
            break;
        }
        for(int i = 0; i < hw_filter_config.num_of_clients ; i++)
        {
          printf("\nEnter MAC Address of Client %d: ", i+1);
          GET_MAC_ADDR(scan_string, hw_filter_config.client_list[i].hw_filtering_mac_addr);
        }
      }
      QCMAP_PRINTF_TAKE_INPUT("\nEnter IP Segment Filtering State (0-Disable/1-Enable): ");
      fgets(scan_string, sizeof(scan_string), stdin);
      ip_seg_flt_state = (qcmap_msgr_config_state_enum_v01)atoi(scan_string);
      hw_filter_config.ip_segment_filter_state = ip_seg_flt_state;
      if(ip_seg_flt_state != QCMAP_MSGR_CONFIG_ENABLE_V01 && QCMAP_MSGR_CONFIG_DISABLE_V01)
      {
        QCMAP_PRINTF_TAKE_INPUT("\nInvalid State Entered");
        break;
      }
      if(ip_seg_flt_state == QCMAP_MSGR_CONFIG_ENABLE_V01)
      {
        if (QcMapClient->GetLANConfig(&lan_config,&qmi_err_num))
        {
          gateway_ip.s_addr = htonl(lan_config.gw_ip);
          netmask_ip.s_addr = htonl(lan_config.netmask);
        }
        while(TRUE)
        {
          QCMAP_PRINTF_TAKE_INPUT("\nMax Num of IP Segments that can be filtered is %d", QCMAP_MAX_IPV4_SEGMENT_FILTERING_V01);
          QCMAP_PRINTF_TAKE_INPUT("\nEnter No. of IP Segments to be Filtered: ");
          fgets(scan_string, sizeof(scan_string), stdin);
          hw_filter_config.num_of_ip_segments = (qcmap_msgr_config_state_enum_v01)atoi(scan_string);
          num_ip_seg = hw_filter_config.num_of_ip_segments;
          if(num_ip_seg <= 0 || num_ip_seg > QCMAP_MAX_IPV4_SEGMENT_FILTERING_V01)
          {
            printf("\nEntered invalid no. of ip segments %d", num_ip_seg);
          }
          else
            break;
        }
        while(TRUE)
        {
          for(int i=0; i<num_ip_seg; i++)
          {
            QCMAP_PRINTF_TAKE_INPUT("\nEnter IP Segment %d Start IP: ", i+1);
            if(fgets(scan_string, sizeof(scan_string), stdin) != NULL)
            {
              memset(&start_ip,0,sizeof(in_addr));
              scan_string[strlen(scan_string)-1]='\0';
              if(!(inet_aton(scan_string, &start_ip) <= 0))
              {
                hw_filter_config.ip_segment_filter_list[i].ip_segment_start = ntohl(start_ip.s_addr);
              }
              else
              {
                QCMAP_PRINTF_TAKE_INPUT("\nIncorrect IPv4 address entered");
                i--;
                continue;
              }
            }
            else
            {
              printf("\nNULL IPv4 address entered");
              fflush(stdout);
              i--;
              continue;
            }
            QCMAP_PRINTF_TAKE_INPUT("\nEnter IP Segment %d End IP: ", i+1);
            if(fgets(scan_string, sizeof(scan_string), stdin) != NULL)
            {
              memset(&end_ip,0,sizeof(in_addr));
              scan_string[strlen(scan_string)-1]='\0';
              if(!(inet_aton(scan_string, &end_ip) <= 0))
              {
                hw_filter_config.ip_segment_filter_list[i].ip_segment_end = ntohl(end_ip.s_addr);
              }
              else
              {
                QCMAP_PRINTF_TAKE_INPUT("\nIncorrect IPv4 address entered");
                i--;
                continue;
              }
            }
            else
            {
              printf("\nNULL IPv4 address entered");
              fflush(stdout);
              i--;
              continue;
            }
            if(hw_filter_config.ip_segment_filter_list[i].ip_segment_end <
               hw_filter_config.ip_segment_filter_list[i].ip_segment_start)
            {
              QCMAP_PRINTF_TAKE_INPUT("\nIncorrect IP Segment Range entered");
              i--;
              continue;
            }
            if(((start_ip.s_addr & netmask_ip.s_addr) != (gateway_ip.s_addr & netmask_ip.s_addr)) ||
               ((end_ip.s_addr & netmask_ip.s_addr) != (gateway_ip.s_addr & netmask_ip.s_addr)))
            {
              QCMAP_PRINTF_TAKE_INPUT("\nIP Segments entered are NOT in LAN subnet. Please enter again\n");
              i--;
              continue;
            }
          }
          break;
        }
      }
      QCMAP_PRINTF_TAKE_INPUT("\nEnter the Iface Name Filter State: (0-Disable/1-Enable): ");
      fgets(scan_string, sizeof(scan_string), stdin);
      if_name_flt_state = (qcmap_msgr_config_state_enum_v01)atoi(scan_string);
      if(if_name_flt_state != QCMAP_MSGR_CONFIG_DISABLE_V01 && if_name_flt_state != QCMAP_MSGR_CONFIG_ENABLE_V01)
      {
        QCMAP_PRINTF_TAKE_INPUT("\nIncorrect State entered %d", if_name_flt_state);
        break;
      }
      if(if_name_flt_state == QCMAP_MSGR_CONFIG_ENABLE_V01)
      {
        hw_filter_config.iface_filter_state = if_name_flt_state;
        while(TRUE)
        {
          QCMAP_PRINTF_TAKE_INPUT("\n Max No. of Ifaces that can be filtered is %d", std::min(QCMAP_MAX_IFACE_FILTERING_V01,
                  IPA_MAX_IFACE_FILTERING));
          QCMAP_PRINTF_TAKE_INPUT("\nEnter the no. of ifaces to be filtered: ");
          fgets(scan_string, sizeof(scan_string), stdin);
          if(atoi(scan_string) <= 0 || atoi(scan_string) >
             std::min(QCMAP_MAX_IFACE_FILTERING_V01, IPA_MAX_IFACE_FILTERING))
          {
            QCMAP_PRINTF_TAKE_INPUT("\nIncorrect No. of Ifaces Entered: %d", atoi(scan_string));
          }
          else
          {
            hw_filter_config.num_of_iface = atoi(scan_string);
            for(int i=0; i<hw_filter_config.num_of_iface; i++)
            {
              QCMAP_PRINTF_TAKE_INPUT("\nEnter the iface name %d: ", i+1);
              fgets(scan_string, sizeof(scan_string), stdin);
              if(scan_string != NULL)
              {
                scan_string[strlen(scan_string)-1]='\0';
                hw_filter_config.if_name_filter_list[i].if_name_len = QCMAP_MAX_IFACE_NAME_SIZE_V01;
                memcpy(&hw_filter_config.if_name_filter_list[i].if_name, scan_string, QCMAP_MAX_IFACE_NAME_SIZE_V01);
              }
              else
              {
                printf("\nEmpty iface entered\n");
                break;
              }
            }
            break;
          }
        }
      }
    }
    if(QcMapClient->SetHWMACFilteringState(state, hw_filter_config, &qmi_err_num))
    {
      printf("\nSet Hardware Filtering State successful\n");
    }
    else
    {
      if(qmi_err_num == QMI_ERR_INVALID_HANDLE_V01)
      {
        printf("\nMobileAP is not enabled\n");
        break;
      }
      printf("\nSet Hardware Filtering State Error: 0x%x \n", qmi_err_num);
    }
  }
  break;

  /* Get HW Filter State */
  case 7:
  {
    qcmap_msgr_config_state_enum_v01   status;
    qcmap_hw_filter_config hw_filter_config;
    int num_of_clients = 0;
    int j = 0;
    char mac_addr_string[QCMAP_MSGR_MAC_ADDR_NUM_CHARS_V01] = {0};
    in_addr start_ip, end_ip;
    memset(&hw_filter_config, 0, sizeof(qcmap_hw_filter_config));

    if (QcMapClient->GetHWMACFilteringState(&status, &hw_filter_config, &qmi_err_num))
    {
      if(status == QCMAP_MSGR_CONFIG_ENABLE_V01)
      {
        if(hw_filter_config.mac_flt_state == QCMAP_MSGR_CONFIG_ENABLE_V01)
        {
          if(hw_filter_config.num_of_clients == 0)
          {
            printf("\nNo Client with Hardware Filtering Enabled \n");
          }
          else
          {
            printf("\nHardware MAC Filtering ENABLED with %d client(s) \n", hw_filter_config.num_of_clients);
            for(j = 0; j < hw_filter_config.num_of_clients; j++)
            {
              printf("\nClient No. MAC Address is %d: ", j+1);
              ds_mac_addr_ntop(hw_filter_config.client_list[j].hw_filtering_mac_addr, mac_addr_string);
              if(strncmp(mac_addr_string, MAC_NULL_STRING, QCMAP_MSGR_MAC_ADDR_NUM_CHARS_V01) != 0)
              {
                printf("\n%s\n",mac_addr_string);
              }
            }
          }
        }
        if(hw_filter_config.ip_segment_filter_state == QCMAP_MSGR_CONFIG_ENABLE_V01)
        {
          if(hw_filter_config.num_of_ip_segments == 0)
          {
            printf("\nNO IP Segments Configured for HW Filtering");
          }
          else
          {
            printf("\n %d IP Segments Configured for HW Filtering", hw_filter_config.num_of_ip_segments);
          }
          for(int k=0; k<hw_filter_config.num_of_ip_segments; k++)
          {
            start_ip.s_addr = htonl(hw_filter_config.ip_segment_filter_list[k].ip_segment_start);
            end_ip.s_addr = htonl(hw_filter_config.ip_segment_filter_list[k].ip_segment_end);
            printf("\nIP Segment %d", k+1);
            printf("\n Start IP: %s", inet_ntoa(start_ip));
            printf("\n End IP: %s", inet_ntoa(end_ip));
          }
        }
        if(hw_filter_config.iface_filter_state == QCMAP_MSGR_CONFIG_ENABLE_V01)
        {
          if(hw_filter_config.num_of_iface == 0)
          {
            printf("\nNO Iface Names Configured for HW Filtering");
          }
          else
          {
            printf("\n %d Ifaces have been configured for HW Filtering", hw_filter_config.num_of_iface);
            for(int k=0; k<hw_filter_config.num_of_iface; k++)
            {
              printf("\nIface %d is: %s", k+1, hw_filter_config.if_name_filter_list[k].if_name);
            }
          }
        }
      }
      else if(status == QCMAP_MSGR_CONFIG_DISABLE_V01)
      {
        printf("\n HW Filtering Feature is NOT ENABLED\n");
      }
    }
    else
    {
      printf("\nGet HW Filtering Feature State failed,Error 0x%x", qmi_err_num);
    }
  }
  break;

  default :
  {
    printf("Invalid response %d\n", firewallOpt);
  }
  break;
  }
}

/*===========================================================================
  FUNCTION dns_list_to_string
  ===========================================================================
/*!
  @brief
  Converts array of dns name strings to one string,
  where each name is seprated by a ' ' character

  @parameters
    char                           *dns_string
    uint32_t                        buf_size
    dns_seach_info                 *dns_list
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
 void dns_list_to_string_for_get_pdn
(
  char                           *dns_string,
  uint32_t                        buf_size,
  dns_seach_info                 *dns_list,
  uint32_t                        list_len
)
{
  if(dns_list!=NULL)
  {
    dns_string[0]='\0';
    for(int i=0; i<list_len; i++)
    {
        strlcat(dns_string, dns_list[i].dns_search_name, buf_size);
        if(i != list_len-1)
        {
            strlcat(dns_string, " ", buf_size);
        }
    }
  }
}

void backhaulCommConfig( int backhaulCommOpt )
{
  qmi_error_type_v01 qmi_err_num;
  /* Initialize QMI Error Number. */
  qmi_err_num = QMI_ERR_NONE_V01;
  char scan_string[QCMAP_MSGR_MAX_FILE_PATH_LEN];
  uint32 tmp_input=0;

  /* BackhualCOMMON configuration options */
  switch(backhaulCommOpt)
  {
    /* Get WWAN Network Configuration */
    case 1:
    {
      qcmap_msgr_ip_family_enum_v01 ip_family;
      qcmap_nw_params_t qcmap_nw_params;
      char ip6_addr_buf[INET6_ADDRSTRLEN];
      char ipv4_addr[INET_ADDRSTRLEN]={0};

      memset(&qcmap_nw_params,0,sizeof(qcmap_nw_params_t));

      QCMAP_PRINTF_TAKE_INPUT("\nPlease input IP Family 4:IPv4 6:IPv6 12:ETH :- ");
      ip_family = read_uint32();

      if ( QcMapClient->GetNetworkConfiguration(
                                   ip_family, &qcmap_nw_params, &qmi_err_num ))
      {
        if (ip_family == QCMAP_MSGR_IP_FAMILY_V4_V01)
        {
          if (qmi_err_num == QMI_ERR_NONE_V01)
          {
            printf("\n IPv4 configuration \n");
            qcmap_nw_params.v4_conf.public_ip.s_addr =
                        htonl(qcmap_nw_params.v4_conf.public_ip.s_addr);
            qcmap_nw_params.v4_conf.primary_dns.s_addr =
                        htonl(qcmap_nw_params.v4_conf.primary_dns.s_addr);
            qcmap_nw_params.v4_conf.secondary_dns.s_addr =
                        htonl(qcmap_nw_params.v4_conf.secondary_dns.s_addr);

            if (inet_ntoa(qcmap_nw_params.v4_conf.public_ip) != NULL)
            {
              printf("\nPublic IP for WWAN: %s \n",
                     inet_ntoa(qcmap_nw_params.v4_conf.public_ip));
            }
            else
            {
              printf ("Unable to get Public IP for WWAN");
            }

            char* primary_dns_ip;
            char* secondary_dns_ip;
            if ((primary_dns_ip = inet_ntoa(qcmap_nw_params.v4_conf.primary_dns)) != NULL)
            {
              printf("Primary DNS IP address: %s \n", primary_dns_ip);
            }
            else
            {
              printf ("Unable to get Primary DNS IP address");
            }

            if ((secondary_dns_ip = inet_ntoa(qcmap_nw_params.v4_conf.secondary_dns)) !=  NULL)
            {
              printf("Secondary DNS IP address: %s \n", secondary_dns_ip);
            }
            else
            {
              printf ("Unable to get Secondary DNS IP address");
            }
          }
          else
            printf("\nError in IPv4 config - 0x%x\n\n", qmi_err_num);
        }

        if (ip_family == QCMAP_MSGR_IP_FAMILY_V6_V01)
        {
          if (qmi_err_num == QMI_ERR_NONE_V01)
          {
            char* ipv6_addr;
            printf("\n IPv6 configuration \n");
            if ((ipv6_addr = inet_ntop(AF_INET6,
                     &qcmap_nw_params.v6_conf.public_ip_v6,
                     ip6_addr_buf,sizeof(ip6_addr_buf)))!= NULL)
            {
              printf("\nPublic IP for WWAN: %s \n", ipv6_addr);
            }
            else
            {
              printf ("Unable to get Public IP for WWAN");
            }
            if ((ipv6_addr = inet_ntop(AF_INET6,
                              &qcmap_nw_params.v6_conf.primary_dns_v6,
                              ip6_addr_buf,sizeof(ip6_addr_buf))) != NULL)
            {
              printf("Primary DNS IP address: %s \n", ipv6_addr);
            }
            else
            {
              printf ("Unable to get Primary DNS IP address");
            }

            if ((ipv6_addr = inet_ntop(AF_INET6,
                              &qcmap_nw_params.v6_conf.secondary_dns_v6,
                              ip6_addr_buf,sizeof(ip6_addr_buf))) != NULL)
            {
              printf("Secondary DNS IP address: %s \n", ipv6_addr);
            }
            else
            {
              printf ("Unable to get Secondary DNS IP address");
            }
          }
          else
            printf("\nError in IPv6 config - 0x%x\n\n", qmi_err_num);
        }
        if (ip_family == QCMAP_MSGR_IP_FAMILY_ETH_V01)
        {
          if (qmi_err_num == QMI_ERR_NONE_V01)
          {
            printf("\n Eth Pdu configuration \n");
            printf ("Eth Pdu vlan_start:%d,vlan_end:%d.",
                     qcmap_nw_params.eth_conf.vlan_start,
                     qcmap_nw_params.eth_conf.vlan_end);
          }
          else
          {
            printf("\nError in Eth Pdu config - 0x%x\n\n", qmi_err_num);
          }
        }
      }
      else
        printf("\nGet WWAN Network Config failed,0x%x", qmi_err_num);
    }
    break;

    /* Configure Backhaul Active Priority */
    /* Set Backhaul priority*/
    case 2:
    {
      int i = 1;
      backhaul_pref_t backhaul_pref_req;
      bzero(&backhaul_pref_req,sizeof(backhaul_pref_t));
      while (i <= MAX_BACKHAUL_SUPPORTED)
      {
        bzero(&scan_string,sizeof(scan_string));
        printf("Enter backhaul (1. WWAN or 2. USB Cradle or 3. WLAN"
               " or 4. Ethernet or 5. BT-WAN) which has Priority %d :", i);
        fflush(stdout);
        tmp_input = read_uint32();
        switch (i)
        {
          case 1:
          {
            backhaul_pref_req.first = tmp_input;
            printf(" first priority %d\n",backhaul_pref_req.first);
          }
          break;
          case 2:
          {
            backhaul_pref_req.second = tmp_input;
            printf(" second priority %d\n",backhaul_pref_req.second);
          }
          break;
          case 3:
          {
            backhaul_pref_req.third = tmp_input;
            printf(" thrid priority %d\n",backhaul_pref_req.third);
          }
          break;
          case 4:
          {
            backhaul_pref_req.fourth = tmp_input;
            printf(" fourth priority %d\n",backhaul_pref_req.fourth);
          }
          break;
          case 5:
          {
            backhaul_pref_req.fifth= tmp_input;
            printf(" fifth priority %d\n",backhaul_pref_req.fifth);
          }
          break;
          default:
            printf("Wrong value Entered!!, Please enter ( wwan or cradle "
              "or Ethernet or wlan) \n");
            i--;
          break;
        }
        i++;
      }

      if (QcMapClient->SetActiveBackhaulPref(&backhaul_pref_req, &qmi_err_num))
      {
        printf("\nBackhaul set config succeeds. Please reboot the device for config to take effect");
      }
      else if(qmi_err_num == QMI_ERR_INVALID_HANDLE_V01)
      {
        printf("\nMobileAP is not enabled\n");
        break;
      }
      else
      {
        printf("\nBackhaul set config fails, Error: 0x%x", qmi_err_num);
      }
     }
     break;

     /* Get Backhual Priority */
     case 3:
     {
       backhaul_pref_t backhaul_pref_resp;
       if (QcMapClient->GetBackhaulPref(&backhaul_pref_resp, &qmi_err_num))
       {
         if (backhaul_pref_resp.first == QCMAP_MSGR_WWAN_BACKHAUL_V01)
           printf("First Priority backhaul is WWAN\n");
         else if (backhaul_pref_resp.first == QCMAP_MSGR_USB_CRADLE_BACKHAUL_V01)
           printf("First Priority backhaul is USB Cradle\n");
         else if (backhaul_pref_resp.first == QCMAP_MSGR_WLAN_BACKHAUL_V01)
           printf("First Priority backhaul is WLAN\n");
         else if (backhaul_pref_resp.first == QCMAP_MSGR_ETHERNET_BACKHAUL_V01)
           printf("First Priority backhaul is Ethernet\n");
         else if (backhaul_pref_resp.first == QCMAP_MSGR_BT_BACKHAUL_V01)
           printf("First Priority backhaul is BT WAN\n");

         if (backhaul_pref_resp.second == QCMAP_MSGR_WWAN_BACKHAUL_V01)
           printf("Second Priority backhaul is WWAN\n");
         else if (backhaul_pref_resp.second == QCMAP_MSGR_USB_CRADLE_BACKHAUL_V01)
           printf("Second Priority backhaul is USB Cradle\n");
         else if (backhaul_pref_resp.second == QCMAP_MSGR_WLAN_BACKHAUL_V01)
           printf("Second Priority backhaul is WLAN\n");
         else if (backhaul_pref_resp.second == QCMAP_MSGR_ETHERNET_BACKHAUL_V01)
           printf("Second Priority backhaul is Ethernet\n");
         else if (backhaul_pref_resp.second == QCMAP_MSGR_BT_BACKHAUL_V01)
           printf("Second Priority backhaul is BT WAN\n");

         if (backhaul_pref_resp.third == QCMAP_MSGR_WWAN_BACKHAUL_V01)
           printf("Third Priority backhaul is WWAN\n");
         else if (backhaul_pref_resp.third == QCMAP_MSGR_USB_CRADLE_BACKHAUL_V01)
           printf("Third Priority backhaul is USB Cradle\n");
         else if (backhaul_pref_resp.third == QCMAP_MSGR_WLAN_BACKHAUL_V01)
           printf("Third Priority backhaul is WLAN\n");
         else if (backhaul_pref_resp.third == QCMAP_MSGR_ETHERNET_BACKHAUL_V01)
           printf("Third Priority backhaul is Ethernet\n");
         else if (backhaul_pref_resp.third == QCMAP_MSGR_BT_BACKHAUL_V01)
           printf("Third Priority backhaul is BT WAN\n");

         if (backhaul_pref_resp.fourth == QCMAP_MSGR_WWAN_BACKHAUL_V01)
           printf("Fourth Priority backhaul is WWAN\n");
         else if (backhaul_pref_resp.fourth == QCMAP_MSGR_USB_CRADLE_BACKHAUL_V01)
           printf("Fourth Priority backhaul is USB Cradle\n");
         else if (backhaul_pref_resp.fourth == QCMAP_MSGR_WLAN_BACKHAUL_V01)
           printf("Fourth Priority backhaul is WLAN\n");
         else if (backhaul_pref_resp.fourth == QCMAP_MSGR_ETHERNET_BACKHAUL_V01)
           printf("Fourth Priority backhaul is Ethernet\n");
         else if (backhaul_pref_resp.fourth == QCMAP_MSGR_BT_BACKHAUL_V01)
           printf("Fourth Priority backhaul is BT WAN\n");

         if (backhaul_pref_resp.fifth == QCMAP_MSGR_WWAN_BACKHAUL_V01)
           printf("Fifth Priority backhaul is WWAN\n");
         else if (backhaul_pref_resp.fifth == QCMAP_MSGR_USB_CRADLE_BACKHAUL_V01)
           printf("Fifth Priority backhaul is USB Cradle\n");
         else if (backhaul_pref_resp.fifth == QCMAP_MSGR_WLAN_BACKHAUL_V01)
           printf("Fifth Priority backhaul is WLAN\n");
         else if (backhaul_pref_resp.fifth == QCMAP_MSGR_ETHERNET_BACKHAUL_V01)
           printf("Fifth Priority backhaul is Ethernet\n");
         else if (backhaul_pref_resp.fifth == QCMAP_MSGR_BT_BACKHAUL_V01)
           printf("Fifth Priority backhaul is BT WAN\n");
       }
       else
         printf("\nGet Backhaul Priority fails, Error: 0x%x", qmi_err_num);
    }
       break;

     /* Get data bitrates */
     case 4:
     {
       qcmap_msgr_data_bitrate_v01 data_rate;
       memset(&data_rate, 0, sizeof(qcmap_msgr_data_bitrate_v01));

      if ( QcMapClient->GetDataRate(&data_rate, &qmi_err_num))
      {
        printf("\n Data Rates:");
        printf("\n Current tx: %lu", data_rate.tx_rate);
        printf("\n Current rx: %lu", data_rate.rx_rate);
        printf("\n Max tx: %lu", data_rate.max_tx_rate);
        printf("\n Max rx: %lu\n", data_rate.max_rx_rate);
      }
      else
      {
        printf("\n  Get Data Bitrate Fails , Error: 0x%x \n", qmi_err_num);
      }
      break;
    }


    /* Enable/Disable IPV4. */
    case 5:
    {
      QCMAP_PRINTF_TAKE_INPUT("   Please input IPV4 State (1-Enable/0-Disable) : ");
      int enable = read_uint32();
      if (enable == 1)
      {
        if (QcMapClient->EnableIPV4(&qmi_err_num))
          printf("\nIPV4 Enable succeeds.");
        else
          printf("\nIPV4 Enable fails, Error: 0x%x", qmi_err_num);
      }
      else if (enable == 0)
      {
        if (QcMapClient->DisableIPV4(&qmi_err_num))
          printf("\nIPV4 Disable in progress.");
        else
          printf("\nIPV4 Disable request fails, Error: 0x%x", qmi_err_num);
      }
      else
      {
        printf("\nInvalid response. Please enter 1 or 0.\n");
      }
      break;
    }

    /* Enable/Disable IPV6. */
    case 6:
    {
      QCMAP_PRINTF_TAKE_INPUT("   Please input IPV6 State (1-Enable/0-Disable) : ");
      int enable = read_uint32();
      if (enable == 1)
      {
        if (QcMapClient->EnableIPV6(&qmi_err_num))
          printf("\nIPV6 Enable succeeds.");
        else
          printf("\nIPV6 Enable fails, Error: 0x%x", qmi_err_num);
      }
      else if (enable == 0)
      {
        if (QcMapClient->DisableIPV6(&qmi_err_num))
          printf("\nIPV6 Disable in progress.");
        else
          printf("\nIPV6 Disable request fails, Error: 0x%x", qmi_err_num);
      }
      else
      {
        printf("\nInvalid response. Please enter 1 or 0.\n");
      }
      break;
    }

    /* Get ipv4 state .     */
    case 7:
    {
      boolean ipv4_state;
      memset((void *)&ipv4_state, 0, sizeof(uint8_t));

      if (QcMapClient->GetIPv4State( &ipv4_state, &qmi_err_num))
      {
         printf("\nIPV4 is: %s.\n",(ipv4_state)?"Enabled":"Disabled");
      }
      else
      {
        printf("\nGetIPV4State returns Error: 0x%x", qmi_err_num);
      }
      break;
    }

    /* Get ipv6 state .     */
    case 8:
    {
      DisplayIPv6State();
      break;
    }
    /* Get Current Backhaul Status*/
    case 9:
    {
      qcmap_backhaul_status_info_ex_type backhaul_status_info_ex;
      char buffer[MAX_BACKHAUL_TYPE_LENGTH];
      memset(&backhaul_status_info_ex, 0, sizeof(qcmap_backhaul_status_info_ex_type));
      memset(buffer,0,MAX_BACKHAUL_TYPE_LENGTH);

      if (QcMapClient->GetBackhaulStatusEx( &backhaul_status_info_ex, &qmi_err_num))
      {
       if ((backhaul_status_info_ex.backhaul_v4_available == false) &&
           (backhaul_status_info_ex.backhaul_v6_available == false) &&
           (backhaul_status_info_ex.backhaul_eth_available == false))
       {
         printf("No Backhaul \n");
       }
       else
       {
         if (convert_backhaul_enum_to_string(backhaul_status_info_ex.backhaul_type, buffer))
         {
           printf("%s Backhual\n", buffer);
           printf("IPV4 %s.\n",(backhaul_status_info_ex.backhaul_v4_available)?"Connected":"Disconnected");
           printf("IPV6 %s.\n",(backhaul_status_info_ex.backhaul_v6_available)?"Connected":"Disconnected");
           printf("ETH PDU %s.\n",(backhaul_status_info_ex.backhaul_eth_available)?"Connected":"Disconnected");
         }
         else
         {
           printf("Invaild backhaul type : %d",backhaul_status_info_ex.backhaul_type);
         }
       }
      }
      else
      {
        printf("\nGetBackhaulStatus returns Error: 0x%x", qmi_err_num);
      }
      break;
    }
    break;

    /*Get Data Bearer Tech */
    case 10:
    {
      qcmap_msgr_data_bearer_tech_type_enum_v01 bearer_tech;
      if (!QcMapClient->GetBearerTech(&bearer_tech, &qmi_err_num))
      {
        printf("Error getting Bearer Tech 0x%x.\n ", qmi_err_num);
        break;
      }
      printf("Current Bearer Tech :%d \n",bearer_tech);
      break;
    }

    /*Get All Connected PDN's*/
    case 11:
    {
      uint8                           num_intf;
      qcmap_connected_wwan_info      *wwan_info;
      char                            addr_str[INET6_ADDRSTRLEN];
      uint32_t max_dns_string_len = QCMAP_DOMAIN_NAME_MAX_V01*QCMAP_MAX_NUM_DNS_SEARCH_LIST;
      char dns_str[max_dns_string_len] ={0};

      wwan_info = (qcmap_connected_wwan_info *) malloc(sizeof(qcmap_connected_wwan_info) * QCMAP_MAX_NUM_BACKHAULS_V01);
      if (wwan_info == NULL)
      {
        printf("   No Memory Left!!!\n");
        break;
      }

      if (!QcMapClient->GetAllConnectedPDNEx(&num_intf, wwan_info, &qmi_err_num))
      {
        printf("Error getting Connected PDN's 0x%x.\n ", qmi_err_num);
        free(wwan_info);
        break;
      }

      printf("Num of Connected PDN's = %d\n", num_intf);
      for (uint8 i=0; i<num_intf; i++)
      {
        printf("Profile Handle          : %d\n", wwan_info[i].profile_handle);
        printf("Subscription Id         : %d\n", wwan_info[i].subscription_id);
        printf("Q6 3gpp Profile Index   : %d\n", wwan_info[i].profile_index_3gpp);
        printf("Q6 3gpp2 Profile Index  : %d\n", wwan_info[i].profile_index_3gpp2);
        printf("Bearer Tech             : %d\n", wwan_info[i].bearer_tech);
        printf("Interface Name          : %s\n", wwan_info[i].iface_name);
        if((wwan_info[i].ip_family == QCMAP_MSGR_IP_FAMILY_V4_V01)||
           (wwan_info[i].ip_family == QCMAP_MSGR_IP_FAMILY_V6_V01) ||
           (wwan_info[i].ip_family == QCMAP_MSGR_IP_FAMILY_V4V6_V01))
        {
          CONVERT_IPV4_NETWORK_ADDRESS_TO_STRING(wwan_info[i].v4_addr, addr_str);
          printf("Public IPv4 Address     : %s\n", addr_str);
          CONVERT_IPV4_NETWORK_ADDRESS_TO_STRING(wwan_info[i].v4_gw_addr, addr_str);
          printf("Gateway IPv4 Address     : %s\n", addr_str);
          CONVERT_IPV4_NETWORK_ADDRESS_TO_STRING(wwan_info[i].v4_subnet_mask, addr_str);
          printf("Public IPv4 subnet mask  : %s\n", addr_str);
          CONVERT_IPV4_NETWORK_ADDRESS_TO_STRING(wwan_info[i].v4_pri_dns_addr, addr_str);
          printf("Pri DNS IPv4 Address    : %s\n", addr_str);
          CONVERT_IPV4_NETWORK_ADDRESS_TO_STRING(wwan_info[i].v4_sec_dns_addr, addr_str);
          printf("Sec DNS IPv4 Address    : %s\n", addr_str);
          CONVERT_IPV6_NETWORK_ADDRESS_TO_STRING(wwan_info[i].v6_addr, addr_str);
          printf("Public IPv6 Address     : %s\n", addr_str);
          CONVERT_IPV6_NETWORK_ADDRESS_TO_STRING(wwan_info[i].v6_gw_addr, addr_str);
          printf("Gateway IPv6 Address     : %s\n", addr_str);
          CONVERT_IPV6_NETWORK_ADDRESS_TO_STRING(wwan_info[i].v6_pri_dns_addr, addr_str);
          printf("Pri DNS IPv6 Address    : %s\n", addr_str);
          printf("MTU IPv4                : %d\n", wwan_info[i].ipv4_mtu);
          printf("MTU IPv6                : %d\n", wwan_info[i].ipv6_mtu);
          printf("IPV6 prefix length      : %d\n", wwan_info[i].ipv6_pref_len);
          /* Only print DNS Search List for default PDN*/
          if (i == 0)
          {
            CONVERT_IPV6_NETWORK_ADDRESS_TO_STRING(wwan_info[i].v6_sec_dns_addr, addr_str);
            printf("Sec DNS IPv6 Address    : %s\n", addr_str);
            if (wwan_info->dns_search_list_len != 0)
            {
              dns_list_to_string_for_get_pdn(dns_str, max_dns_string_len,
                                              wwan_info->dns_search_list,
                                              wwan_info->dns_search_list_len);
              printf("DNS Search List         : %s\n", dns_str);
            }
            printf("\n");
          }
          else
          {
            CONVERT_IPV6_NETWORK_ADDRESS_TO_STRING(wwan_info[i].v6_sec_dns_addr, addr_str);
            printf("Sec DNS IPv6 Address    : %s\n\n", addr_str);
          }
        }

        if(wwan_info[i].ip_family == QCMAP_MSGR_IP_FAMILY_ETH_V01)
        {
          printf("PDN Vlan Mapping Id Start      : %d\n", wwan_info[i].vlan_start);
          printf("PDN Vlan Mapping Id End        : %d\n", wwan_info[i].vlan_end);
        }
      }

      free(wwan_info);
      break;
    }

    default:
    {
      printf("Invalid response %d\n", backhaulCommOpt);
    }
    break;
  }
}

int16_t ChooseLANBridge(void)
{
  qcmap_msgr_bridge_list_v01 bridge_list;
  qmi_error_type_v01 qmi_err_num;
  char scan_string[QCMAP_MSGR_MAX_FILE_PATH_LEN] = {0};
  int16_t option = 0;
  profile_handle_type_v01 profile_handle = 0;

  qcmap_msgr_vlan_conf_t br_lan_list;
  memset(&br_lan_list, 0, sizeof(qcmap_msgr_vlan_conf_t));

  if (QcMapClient->GetVLANConfig(&br_lan_list, &qmi_err_num))
  {
    if (br_lan_list.vlan_config_list_len > 0)
    {
      //tell user what options are available
      printf("   +--------+\n");
      printf("   |        |\n");
      printf("   | br-lan |\n");
      printf("   |        |\n");
      printf("   +--------+\n");

      printf("   |%8d|\n",DEFAULT_BRIDGE_ID);
      for (int i = 0; i < br_lan_list.vlan_config_list_len; i++)
      {
        printf("   |%8d|\n", br_lan_list.vlan_config_list_ex[i].vlan_id);
      }
      printf("   +--------+\n\n");

      //ask for user input
      QCMAP_PRINTF_TAKE_INPUT("   Select br-lan#: ");
      if(fgets(scan_string, sizeof(scan_string), stdin) == NULL)
      {
        printf("error with client input: %s\n", strerror(errno));
        return -1;
      }
      option = atoi(scan_string);

      //validate user input
      if (option == DEFAULT_BRIDGE_ID)
      {
        printf ("Selected br-lan# = (%d)\n", option);
        return option;
      }

      for (int i = 0; i < br_lan_list.vlan_config_list_len; i++)
      {
        if(option == br_lan_list.vlan_config_list_ex[i].vlan_id)
        {
          printf ("Selected br-lan# = (%d)\n", option);
          return option;
        }
      }
    }
  }
  else
  {
    LOG_MSG_ERROR("Failed to getVlanConfig , Error: 0x%x", qmi_err_num,0,0);
  }

  printf("Error with choosing LAN Bridge\n");
  return -1;
}

profile_handle_type_v01 ChooseWWANProfileHandle()
{
  char scan_string[QCMAP_MSGR_MAX_FILE_PATH_LEN];
  int option;
  profile_handle_type_v01 profile_handle = 0, current_profile_handle = 0;
  int default_no = 1;

  if (ShowAllWWANProfiles(&default_no, &current_profile_handle))
  {
    QCMAP_PRINTF_TAKE_INPUT("   Select Profile Handle # (Default=%d, Current=%d): ", default_no, current_profile_handle);

    fgets(scan_string, sizeof(scan_string), stdin);
    profile_handle = atoi(scan_string);
    printf ("Profile=(%d)\n", profile_handle);
  }

  return profile_handle;
}

void QoSFlowIndConfig( int qosFlowOpt )
{
  qcmap_msgr_ip_family_enum_v01 ip_family = QCMAP_MSGR_IP_FAMILY_V4V6_V01;
  boolean                       subscribe = false;
  qmi_error_type_v01            qmi_err_num;
  char                          scan_string[QCMAP_MSGR_MAX_FILE_PATH_LEN];

  switch(qosFlowOpt)
  {
    case 1:
       {
         QCMAP_PRINTF_TAKE_INPUT ("\nPlease input IP Family type [4:IPv4 6:IPv6 10:IPv4_IPv6]:");
         if (fgets(scan_string, sizeof(scan_string), stdin) == NULL)
           break;
         ip_family = (qcmap_msgr_ip_family_enum_v01)atoi(scan_string);
         if (!QcMapClient->GetGlobalQoSFlowInfo(ip_family, &qmi_err_num))
         {
           printf("\nFailed to query Global QoS Flow Indications, error code: %d", qmi_err_num);
           if (qmi_err_num == QMI_ERR_OP_IN_PROGRESS_V01)
           {
             printf("\nAnother QoS flow request is in progress");
           }
           return;
         }
         printf("\nSuccessfully queried Global QoS Flow Indications, error code: %d", qmi_err_num);
       }
       break;
  }
  return;
}
void backhaulWWANUpdateConfig( int backhaulWWANUpdateOpt )
{
  qmi_error_type_v01 qmi_err_num;
  char scan_string[QCMAP_MSGR_MAX_FILE_PATH_LEN] = {0};
  qcmap_net_policy_info net_policy;
  qcmap_msgr_update_profile_enum_v01 update_req = 0;
  profile_handle_type_v01 current_profile_handle = 0;

  /* Initialize QMI Error Number. */
  qmi_err_num = QMI_ERR_NONE_V01;
  memset(&net_policy,0,sizeof(qcmap_msgr_net_policy_info_v01));

  /* BackhaulWWAN Update configuration options */
  switch(backhaulWWANUpdateOpt)
  {
    /* Update Tech Pref */
    case 1:
      {
        ASK_USER_FOR_INPUT_INT_PARAM("0 -> Any, 1 -> 3gpp, 2 -> 3gpp2 (select one):", net_policy.tech_pref);
        if ( net_policy.tech_pref != 0 && net_policy.tech_pref != 1 &&
            net_policy.tech_pref != 2)
        {
          printf ("\n Invalid tech preference\n");
          return;
        }
        update_req = QCMAP_MSGR_UPDATE_TECH_TYPE_V01;
      }
      break;

    /* Update 3GPP (V4) profile # */
    case 2:
      {
        ASK_USER_FOR_INPUT_INT_PARAM("   Please enter UMTS Profile Number : ", net_policy.profile_id_3gpp);
        update_req = QCMAP_MSGR_UPDATE_V4_3GPP_PROFILE_INDEX_V01;
      }
      break;

    /* Update 3GPP2 (V4) profile # */
    case 3:
      {
        ASK_USER_FOR_INPUT_INT_PARAM("   Please enter CDMA Profile Number : ", net_policy.profile_id_3gpp2);
        update_req = QCMAP_MSGR_UPDATE_V4_3GPP2_PROFILE_INDEX_V01;
      }
      break;

#if 0  /* Not being used */
    /* Update 3GPP (V6) profile # */
    case 4:
      {
        ASK_USER_FOR_INPUT_INT_PARAM("   Please enter UMTS Profile Number : ", net_policy.profile_id_3gpp);
        update_req = QCMAP_MSGR_UPDATE_V6_3GPP_PROFILE_INDEX_V01;
      }
      break;

    /* Update 3GPP2 (V6) profile # */
    case 5:
      {
        ASK_USER_FOR_INPUT_INT_PARAM("   Please enter CDMA Profile Number : ", net_policy.profile_id_3gpp2);
        update_req = QCMAP_MSGR_UPDATE_V6_3GPP2_PROFILE_INDEX_V01;
      }
      break;
#endif

    /* Update All profile # */
    case 4:
      {
        ASK_USER_FOR_INPUT_INT_PARAM("   Please enter UMTS Profile Number : ", net_policy.profile_id_3gpp);
        ASK_USER_FOR_INPUT_INT_PARAM("   Please enter CDMA Profile Number : ", net_policy.profile_id_3gpp2);

        update_req = QCMAP_MSGR_UPDATE_ALL_PROFILE_INDEX_V01;
      }
      break;

    /* Set concurrent profile as default */
    case 5:
      {
        printf("Switching this current profile as default\n");
        update_req = QCMAP_MSGR_SET_DEFAULT_PROFILE_V01;
      }
      break;

    /* Update subscription id */
    case 6:
      {
        ASK_USER_FOR_INPUT_INT_PARAM("   Enter Subscription Id (0-Default, 1-Primary, 2-Secondary): ", net_policy.subscription_id);
        update_req = QCMAP_MSGR_UPDATE_SUBSCRIPTION_ID_V01;
      }
      break;

    /* Update APN Name */
    case 7:
      {
        QCMAP_PRINTF_TAKE_INPUT("Enter the APN Name for the profile to be updated  :");
        if ( fgets(scan_string,sizeof(scan_string),stdin) != NULL )
        {
          strlcpy(net_policy.apn_name, scan_string, sizeof(scan_string));
          net_policy.apn_name[strlen(net_policy.apn_name)-1] = '\0';
          update_req = QCMAP_MSGR_UPDATE_APN_NAME_V01;
          break;
        }
      }

    default:
      printf("Invalid option %d\n", backhaulWWANUpdateOpt);
      break;
  }

  if (update_req && QcMapClient->UpdateWWANPolicyEx(update_req, net_policy, &qmi_err_num))
  {
    printf("  Update WWAN policy succeeds.\n. ");
  }
  else
  {
    printf("  Failed to Update WWAN policy. Error 0x%x.\n ", qmi_err_num);
  }
}


void backhaulWWANConfig( int backhaulWWANOpt )
{
  qmi_error_type_v01 qmi_err_num;
  /* Initialize QMI Error Number. */
  qmi_err_num = QMI_ERR_NONE_V01;
  char scan_string[QCMAP_MSGR_MAX_FILE_PATH_LEN] = {0};
  profile_handle_type_v01 profile_handle = 0;
  int array_size = 0, qosFlowOpt = 0;

  /* BackhaulWWAN configuration options */
  switch(backhaulWWANOpt)
  {
  /* Connect/Disconnect Backhaul */
  case 1:
  {
  qcmap_msgr_wwan_call_type_v01 call_type;
  QCMAP_PRINTF_TAKE_INPUT("   Please input Backhaul State(1-Connect/0-Disconnect) : ");
  fgets(scan_string, sizeof(scan_string), stdin);
  if(!isdigit(scan_string[0]) || ((atoi(scan_string) != 0) && (atoi(scan_string) != 1)))
  {
    printf("\nInvalid Input Entered");
    break;
  }
  if (atoi(scan_string))
  {
    memset(scan_string, 0, QCMAP_MSGR_MAX_FILE_PATH_LEN);

    QCMAP_PRINTF_TAKE_INPUT("\n   Please input Call Type (1-IPV4; 2-IPV6; 3-IPV4V6; 4-V2X; 5-ETH) : ");

    fgets(scan_string, sizeof(scan_string), stdin);
    call_type = (qcmap_msgr_wwan_call_type_v01)atoi(scan_string);

    if ( call_type < QCMAP_MSGR_WWAN_CALL_TYPE_V4_V01 ||
         call_type > QCMAP_MSGR_WWAN_CALL_TYPE_ETH_V01 )
    {
      printf("\nInvalid Call Type : %d", call_type);
      break;
    }
    if ( QcMapClient->ConnectBackHaul(call_type, &qmi_err_num))
    {
      if(qmi_err_num != QMI_ERR_NONE_V01)
      {
        printf("\nConnectBackHaul skipped. Error: 0x%x", qmi_err_num);
      }
      else
      {
        printf("\nConnectBackHaul succeeds.");
      }
    }
    else
    {
      if(qmi_err_num == QMI_ERR_OP_DEVICE_UNSUPPORTED_V01)
        printf("\nConnectBackhaul fails, call type %d is not supported", call_type);
      else
        printf("\nConnectBackHaul fails, Error: 0x%x", qmi_err_num);
    }
  }
  else
  {
    memset(scan_string, 0, QCMAP_MSGR_MAX_FILE_PATH_LEN);

    QCMAP_PRINTF_TAKE_INPUT("\n   Please input Call Type (1-IPV4; 2-IPV6; 3-IPV4V6; 4-V2X; 5-ETH) : ");

    fgets(scan_string, sizeof(scan_string), stdin);
    call_type = (qcmap_msgr_wwan_call_type_v01)atoi(scan_string);

    if ( call_type < QCMAP_MSGR_WWAN_CALL_TYPE_V4_V01 ||
         call_type > QCMAP_MSGR_WWAN_CALL_TYPE_ETH_V01 )
    {
      printf("\nInvalid Call Type : %d", call_type);
      break;
    }
    if ( QcMapClient->DisconnectBackHaul(call_type, &qmi_err_num))
    {
      if (qmi_err_num != QMI_ERR_NONE_V01)
      {
        printf("\nDisconnectBackhaul skipped. Error: 0x%x", qmi_err_num);
      }
      else
      {
        printf("\nDisconnectBackHaul succeeds.");
      }
    }
    else
    {
      printf("\nDisconnect BackHaul fails, Error: 0x%x", qmi_err_num);
    }

  }
  break;
  }

  /* Get WAN Status */
  case 2:
  {
    qcmap_msgr_wwan_status_enum_v01 v4_status, v6_status,eth_status;
    if( QcMapClient->GetWWANStatusEx(&v4_status, &v6_status,&eth_status,&qmi_err_num))
    {
      switch(v4_status)
      {
        case QCMAP_MSGR_WWAN_STATUS_CONNECTING_V01:
          printf(" IPV4 WWAN is Connecting \n");
          break;

        case QCMAP_MSGR_WWAN_STATUS_CONNECTED_V01:
          printf(" IPV4 WWAN is Connected \n");
          break;

        case QCMAP_MSGR_WWAN_STATUS_DISCONNECTING_V01:
          printf(" IPV4 WWAN is Disconnecting \n");
          break;

        case QCMAP_MSGR_WWAN_STATUS_DISCONNECTED_V01:
          printf(" IPV4 WWAN is Disconnected \n");
          break;

        default:
          printf(" IPV4 WWAN status is unknown \n");
          break;
      }

      switch(v6_status)
      {
        case QCMAP_MSGR_WWAN_STATUS_IPV6_CONNECTING_V01:
          printf(" IPV6 WWAN is Connecting \n");
          break;

        case QCMAP_MSGR_WWAN_STATUS_IPV6_CONNECTED_V01:
          printf(" IPV6 WWAN is Connected \n");
          break;

        case QCMAP_MSGR_WWAN_STATUS_IPV6_DISCONNECTING_V01:
          printf(" IPV6 WWAN is Disconnecting \n");
          break;

        case QCMAP_MSGR_WWAN_STATUS_IPV6_DISCONNECTED_V01:
          printf(" IPV6 WWAN is Disconnected \n");
          break;

        default:
          printf(" IPV6 WWAN status is unknown \n");
          break;
      }

      switch(eth_status)
      {
        case QCMAP_MSGR_WWAN_STATUS_ETH_CONNECTING_V01:
          printf(" ETH WWAN is Connecting \n");
          break;

        case QCMAP_MSGR_WWAN_STATUS_ETH_CONNECTED_V01:
          printf(" ETH WWAN is Connected \n");
          break;

        case QCMAP_MSGR_WWAN_STATUS_ETH_DISCONNECTING_V01:
          printf(" ETH WWAN is Disconnecting \n");
          break;

        case QCMAP_MSGR_WWAN_STATUS_ETH_DISCONNECTED_V01:
          printf(" ETH WWAN is Disconnected \n");
          break;

        default:
          printf(" ETH WWAN status is unknown \n");
          break;
      }
    }
    else
    {
      printf("\nWWAN status get fails, Error: 0x%x", qmi_err_num);
    }
  }
  break;

  /* Get WWAN Statistics */
  case 3:
  {
  qcmap_msgr_ip_family_enum_v01 ip_family;
  qcmap_msgr_wwan_statistics_type_v01 wwan_stats;


  QCMAP_PRINTF_TAKE_INPUT("Please input IP Family 4==>IPV4, 6==>IPV6, 10==>IPV4_IPV6, 12==>ETH: ");

  fgets(scan_string, sizeof(scan_string), stdin);
  ip_family = (qcmap_msgr_ip_family_enum_v01)atoi(scan_string);

  memset((void *)&wwan_stats, 0, sizeof(qcmap_msgr_wwan_statistics_type_v01));
  if ( QcMapClient->GetWWANStatistics(ip_family, &wwan_stats, &qmi_err_num) )
  {
   printf("\nWWAN Stats Fetched.\n");
   printf("\nbytes_rx: %lu",wwan_stats.bytes_rx);
   printf("\nbytes_tx: %lu",wwan_stats.bytes_tx);
   printf("\npkts_rx: %lu",wwan_stats.pkts_rx);
   printf("\npkts_tx: %lu",wwan_stats.pkts_tx);
   printf("\npkts_dropped_rx: %lu",wwan_stats.pkts_dropped_rx);
   printf("\npkts_dropped_tx: %lu",wwan_stats.pkts_dropped_tx);
  }
  else
   printf("\nGet WWAN Stats Fails, Error: 0x%x", qmi_err_num);
  }
  break;


  /* RESET WWAN Statistics */
  case 4:
  {
  qcmap_msgr_ip_family_enum_v01 ip_family;

  QCMAP_PRINTF_TAKE_INPUT("Please input IP Family 4==>IPV4, 6==>IPV6, 10==>IPV4_IPV6: ");
  fgets(scan_string, sizeof(scan_string), stdin);
  ip_family = (qcmap_msgr_ip_family_enum_v01)atoi(scan_string);

  if ( QcMapClient->ResetWWANStatistics(ip_family, &qmi_err_num))
  {
     printf("WWAN Stats Reset for IP Family: %d \n", ip_family);
  }
  else
   printf("\nReset WWAN Stats failed, Error: 0x%x", qmi_err_num);
  }
  break;

  /* Set WWAN Webserver Access Flag */
  case 5:
  {
  boolean enable;
  memset(scan_string, 0, QCMAP_MSGR_MAX_FILE_PATH_LEN);

  QCMAP_PRINTF_TAKE_INPUT("   Please input Webserver WWAN access(1-Enable/0-Disable) : ");
  fgets(scan_string, sizeof(scan_string), stdin);
  if (!VALID_NUMERIC_INPUT(scan_string) || (atoi(scan_string) != 1 && atoi(scan_string) != 0))
  {
    printf("\nInvalid response. Please enter 1 or 0.\n");
    break;
  }
  enable = (atoi(scan_string))? true : false;

  if (QcMapClient->SetWebserverWWANAccess(enable, &qmi_err_num))
  {
    printf("\nWebserver WWAN access set successfully");
  }
  else
  {
    if (qmi_err_num == QMI_ERR_INTERFACE_NOT_FOUND_V01)
      printf("\nBackhaul down.webserver wwan access enabled in xml file.");
    else
      printf("\nset webserver wwan access fails. Error: 0x%x", qmi_err_num);
  }
  }
  break;

  /* Enable/disable the Webserver WWAN access and save XML if successful. */
  case 6:
  {
  boolean flag;
  int p_error=0;

  if (QcMapClient->GetWebserverWWANAccess(&flag, &qmi_err_num))
  {
   printf("\nWebserver WWAN Access Enable Flag : %d", flag);
  }
  else
   printf("\nWebserver WWAN Access fails. Error: 0x%x", qmi_err_num);
  }
  break;


  /* set WWAN Profile */
  case 7:
  {
  qcmap_msgr_ip_family_enum_v01 ip_family ;
  qcmap_msgr_net_policy_info_v01 net_policy;
  memset(&net_policy,0,sizeof(qcmap_msgr_net_policy_info_v01));

  QCMAP_PRINTF_TAKE_INPUT("Please select Technology (0-ANY, 1-UMTS, 2-CDMA) : ");
  fgets(scan_string, sizeof(scan_string), stdin);
  if (!VALID_NUMERIC_INPUT(scan_string))
  {
    printf("\nInvalid response\n");
    break;
  }
  net_policy.tech_pref = atoi(scan_string);
  if ( net_policy.tech_pref != 0 && net_policy.tech_pref != 1 &&
      net_policy.tech_pref != 2)
  {
    printf ("\n Invalid tech preference\n");
    break;
  }
  QCMAP_PRINTF_TAKE_INPUT("Please input IP Family IPV4-4 IPV6-6 IPV4V6-10 ETH-12: ");
  fgets(scan_string, sizeof(scan_string), stdin);
  ip_family = (qcmap_msgr_ip_family_enum_v01)atoi(scan_string);
  if ( ip_family == QCMAP_MSGR_IP_FAMILY_V4_V01 )
  {
   QCMAP_PRINTF_TAKE_INPUT("   Please enter UMTS Profile Number : ");
   fgets(scan_string, sizeof(scan_string), stdin);
   net_policy.v4_profile_id_3gpp = atoi(scan_string);
   QCMAP_PRINTF_TAKE_INPUT("   Please enter CDMA Profile Number : ");
   fgets(scan_string, sizeof(scan_string), stdin);
   net_policy.v4_profile_id_3gpp2 = atoi(scan_string);
  }
  else if ( ip_family == QCMAP_MSGR_IP_FAMILY_V6_V01 )
  {
    QCMAP_PRINTF_TAKE_INPUT("   Please enter UMTS Profile Number : ");
    fgets(scan_string, sizeof(scan_string), stdin);
    net_policy.v6_profile_id_3gpp = atoi(scan_string);
    QCMAP_PRINTF_TAKE_INPUT("   Please enter CDMA Profile Number : ");
    fgets(scan_string, sizeof(scan_string), stdin);
    net_policy.v6_profile_id_3gpp2 = atoi(scan_string);
  }
  else if ( ip_family == QCMAP_MSGR_IP_FAMILY_V4V6_V01 )
  {
    QCMAP_PRINTF_TAKE_INPUT("Please enter V4 UMTS Profile Number : ");
    fgets(scan_string, sizeof(scan_string), stdin);
    net_policy.v4_profile_id_3gpp = atoi(scan_string);
    QCMAP_PRINTF_TAKE_INPUT("   Please enter V4 CDMA Profile Number : ");
    fgets(scan_string, sizeof(scan_string), stdin);
    net_policy.v4_profile_id_3gpp2 = atoi(scan_string);
    QCMAP_PRINTF_TAKE_INPUT("Please enter V6 UMTS Profile Number : ");
    fgets(scan_string, sizeof(scan_string), stdin);
    net_policy.v6_profile_id_3gpp = atoi(scan_string);
    QCMAP_PRINTF_TAKE_INPUT("   Please enter V6 CDMA Profile Number : ");
    fgets(scan_string, sizeof(scan_string), stdin);
    net_policy.v6_profile_id_3gpp2 = atoi(scan_string);
  }
  else if ( ip_family == QCMAP_MSGR_IP_FAMILY_ETH_V01 )
  {
    QCMAP_PRINTF_TAKE_INPUT("Please enter ETH UMTS Profile Number : ");
    fgets(scan_string, sizeof(scan_string), stdin);
    net_policy.eth_profile_id_3gpp = atoi(scan_string);
  }
  else
  {
    printf("\nUnsupported ip mode:- %d.\n", ip_family);
    break;
  }
  net_policy.ip_family = ip_family;
  if (QcMapClient->SetWWANPolicy(net_policy, &qmi_err_num))
    printf("  Set Wwan policy succeeds.\n. ");
  else
  {
    printf("  Failed to Set WWAN policy. Error 0x%x.\n ", qmi_err_num);
    if (qmi_err_num == QMI_ERR_DEVICE_IN_USE_V01)
      printf("    Data-call is active for this profile. Disconnect Backhaul and try again.\n");
    else if (qmi_err_num == QMI_ERR_INVALID_HANDLE_V01)
      printf("    Profile already exists, delete this profile and try again.\n");
    else
      printf("    Unknown error.\n");
  }
  break;
  }

  /* get WWAN policy    */
  case 8:
  {
  DisplayWWANPolicy();
  break;
  }

  /* Set Prefix Delegation Mode */
  case 9:
  {
  QCMAP_PRINTF_TAKE_INPUT("   Enable(1)/Disable(0) Prefix Delegation mode:");
  fgets(scan_string, sizeof(scan_string), stdin);
  if (!VALID_NUMERIC_INPUT(scan_string))
  {
    printf("\nInvalid response\n");
    break;
  }

  if (atoi(scan_string) >= 0 && atoi(scan_string) <= 1)
  {
   if (QcMapClient->SetPrefixDelegationConfig(atoi(scan_string), &qmi_err_num))
   {
      printf("\nMobile AP Prefix Delegation Config has been set\n");
   }
   else
   {
     if (qmi_err_num == QMI_ERR_DEVICE_IN_USE_V01)
       printf("\nIPv6 WAN call is connected, config saved and will take affect after v6 call is restarted.\n");
     else
       printf("\nFailed to Set Prefix Delegation Config: Error 0x%x.\n ", qmi_err_num);
   }
  }
  else
   printf("\n   %s is invalid, please select a valid option\n", scan_string);

  break;
  }

  /* Get Prefix Delegation Config */
  case 10:
  {
  boolean pd_mode;

  if (QcMapClient->GetPrefixDelegationConfig(&pd_mode, &qmi_err_num))
  {
   if (pd_mode)
     printf("\n   Prefix Delegation Config: mode is enabled.\n");
   else
     printf("\n   Prefix Delegation Config: mode is disabled.\n");
  }
  else
   printf("  Failed to Get Prefix Delegation config. Error 0x%x.\n ", qmi_err_num);
  break;
  }


  /* Get Prefix Delegation Status */
  case 11:
  {
  boolean pd_mode;

  if (QcMapClient->GetPrefixDelegationStatus(&pd_mode, &qmi_err_num))
  {
   if (pd_mode)
     printf("   Prefix Delegation is enabled.\n");
   else
     printf("   Prefix Delegation is disabled.\n");
  }
  else
   printf("  Failed to Get Prefix Delegation mode. Error 0x%x.\n ", qmi_err_num);
  break;
  }

  /* Enable/Disable TinyProxy */
  case 12:
  {
  QCMAP_PRINTF_TAKE_INPUT("Please input TinyProxy State(1-Enable/0-Disable) : ");
  fgets(scan_string, sizeof(scan_string), stdin);
  if (atoi(scan_string))
  {
  if(QcMapClient->EnableTinyProxy(&qmi_err_num))
  {
    printf("\nEnabled TinyProxy");
  }
  else
  {
    printf("\nEnable TinyProxy fails, Error: 0x%x", qmi_err_num);
  }
  }
  else
  {
  if(QcMapClient->DisableTinyProxy(&qmi_err_num))
  {
    printf("\nDisabled TinyProxy");
  }
  else
    printf("\nDisable TinyProxy fails, Error: 0x%x", qmi_err_num);
  }

  break;
  }

  /* Get TinyProxy Status */
  case 13:
  {
  qcmap_msgr_tiny_proxy_mode_enum_v01  status;
  if(QcMapClient->GetTinyProxyStatus(&status, &qmi_err_num))
  {
   if(status == QCMAP_MSGR_TINY_PROXY_MODE_UP_V01)
   {
     printf("\nTinyProxy  enabled");
   }
   else if(status == QCMAP_MSGR_TINY_PROXY_MODE_DOWN_V01)
     printf("\nTinyProxy  is disabled");
   else
     printf("\nTinyProxy  mode is not set");
  }
  else
  {
   printf("\nGetTinyProxyStatus returns Error: 0x%x", qmi_err_num);
  }
  break;
  }

  /* Set IP Passthrough Config */
  case 14:
  {
    qcmap_msgr_ip_passthrough_config_v01 ip_passthrough_config;
    int i = 0, new_config, ip_pass_config_type = 0, device_type = 0;
    qcmap_msgr_ip_passthrough_mode_enum_v01 enable_state;
    memset(&ip_passthrough_config, 0, sizeof(qcmap_client_ip_passthrough_config));

    QCMAP_PRINTF_TAKE_INPUT("\n(Set IP Passthrough Flag: (1-Enable /0-Disable ): ");
    if (fgets(scan_string, sizeof(scan_string), stdin) != NULL)
    {
      enable_state = atoi(scan_string);
      if (!isdigit(scan_string[0]) || !(enable_state == 0 || enable_state == 1))
      {
        printf("\n Provide correct input (0/1)");
        break; /* continue */
      }
    }
    else
    {
      printf("Did not recognize input");
      break;
    }

    if(enable_state == QCMAP_MSGR_IP_PASSTHROUGH_MODE_UP_V01)
    {
      while(TRUE)
      {
        /* If configuring IP Passthrough for the first time
         * or User doesn't want to use the existing conf
         */
        QCMAP_PRINTF_TAKE_INPUT("\nDo you want to enable passthough with a new config (1-Yes / 0-No ): ");
        if (fgets(scan_string, sizeof(scan_string), stdin) != NULL)
        {
          new_config = atoi(scan_string);
          if (!(new_config == 0 || new_config == 1))
          {
            printf("\n Provide correct input (0/1)");
            continue;
          }
          break;
        }
        else
        {
          printf("Did not recognize input");
          break;
        }
      }
      if (new_config == 0)// Use existing config
      {
        if (QcMapClient->SetIPPassthroughConfig(enable_state, false, NULL, &qmi_err_num))
        {
          printf("Set IP Passthrough status successful\n");
        }
        else
        {
          if (qmi_err_num == QMI_ERR_INVALID_ARG_V01)
          {
            printf("No existing config prevails for IP Passthrough: 0x%x", qmi_err_num);
          }
          printf("Set IP Passthrough status Error: 0x%x", qmi_err_num);
        }
      }
      else if (new_config == 1)// Give a new config
      {
        QCMAP_PRINTF_TAKE_INPUT("\n1.Enter specific details for IP Passthrough device"
               "\n2.First Connected Device (Optional MAC Address) "
               "\n\nEnter your choice: ");
        fgets(scan_string, sizeof(scan_string), stdin);
        ip_pass_config_type = atoi(scan_string);
        if (ip_pass_config_type == 1)
        {
          QCMAP_PRINTF_TAKE_INPUT("\nPlease enter the device type (0-USB / 1-ETH / 2-WiFi / 3-ETH NIC2) : ");
          fgets(scan_string, sizeof(scan_string), stdin);
          if (!VALID_NUMERIC_INPUT(scan_string))
          {
            printf("\nInvalid response\n");
            break;
          }
          device_type = atoi(scan_string);
          if (!(device_type >= 0 || device_type <= 3))
          {
            printf("\nDevice Type not Supported");
            break; /*continue;*/
          }
          if (device_type == 0)
          {
            /* device type USB */
            ip_passthrough_config.device_type = QCMAP_MSGR_DEVICE_TYPE_USB_V01;
            printf("\nEnter device detail for reservertion\n");
            while(TRUE)
            {
              QCMAP_PRINTF_TAKE_INPUT("Enter hostname of the client:");
              if (fgets(scan_string,sizeof(scan_string),stdin) != NULL)
              {
                if (*scan_string == '\n')
                {
                  printf("Client name is mandatory for USB client\n");
                  continue;
                }
                else
                {
                  for (i=0;i < strlen(scan_string)-1;i++)
                  {
                    ip_passthrough_config.client_device_name[i] = scan_string[i];
                  }
                  ip_passthrough_config.client_device_name[i] ='\0';
                  break;
                }
              }
            }
          }
          else if (device_type == 1 || device_type == 3)
          {
            /* device type ETHERNET */
            QCMAP_PRINTF_TAKE_INPUT("\nPlease input the MAC address : ");
            device_type == 1 ? (ip_passthrough_config.device_type = QCMAP_MSGR_DEVICE_TYPE_ETHERNET_V01) :
                               (ip_passthrough_config.device_type = QCMAP_MSGR_DEVICE_TYPE_ETHERNET_NIC2_V01);
            GET_MAC_ADDR(scan_string,ip_passthrough_config.mac_addr);
          }
          else if (device_type == 2)
          {
            /* device type WIFI */
            QCMAP_PRINTF_TAKE_INPUT("\nPlease input the MAC address : ");
            ip_passthrough_config.device_type = QCMAP_MSGR_DEVICE_TYPE_ANY_AP_V01;
            GET_MAC_ADDR(scan_string, ip_passthrough_config.mac_addr);
          }
        }
        else if (ip_pass_config_type == 2)
        {
          printf("\nIP Passthrough first device mode");
          ip_passthrough_config.device_type = QCMAP_MSGR_DEVICE_TYPE_ANY_V01;
        }
        else
        {
          printf("\nInvalid option");
          break;
        }
        if (QcMapClient->SetIPPassthroughConfig(enable_state, true, &ip_passthrough_config,
                                                &qmi_err_num))
        {
          printf("Set IP Passthrough status successful\n");
        }
        else
        {
          printf("Set IP Passthrough status Error: 0x%x", qmi_err_num);
        }
      }
    }
    else
    {
      if (QcMapClient->SetIPPassthroughConfig(enable_state, false, NULL, &qmi_err_num))
      {
        printf("Set IP Passthrough status successful\n");
      }
      else
      {
        if(qmi_err_num == QMI_ERR_INVALID_HANDLE_V01)
        {
          printf("\nMobileAP is not enabled\n");
          break;
        }
        printf("\nSet IP Passthrough status Error: 0x%x", qmi_err_num);
      }
    }
  }
  break;

  /* Get IP Passthrough Configuration */
  case 15:
  {
    int i = 0;
    qcmap_msgr_ip_passthrough_config_v01 ip_passthrough_config;
    qcmap_msgr_ip_passthrough_mode_enum_v01 enable_state;
    char mac_addr_str[QCMAP_MSGR_MAC_ADDR_NUM_CHARS_V01] = {0}; /*char array of mac addr*/
    memset(&ip_passthrough_config, 0, sizeof(qcmap_client_ip_passthrough_config));
    if (QcMapClient->GetIPPassthroughConfig(&enable_state, &ip_passthrough_config, &qmi_err_num))
    {
      /* enable state */
      if(enable_state)
      {
        printf("\nIP Passthrough Flag is SET");
      }
      else
      {
        printf("\nIP Passthrough is NOT SET");
      }
      /* device type */
      if (ip_passthrough_config.device_type== QCMAP_MSGR_DEVICE_TYPE_USB_V01)
      {
        printf("\nPassthrough Device: USB");
        /* device name */
        if (ip_passthrough_config.client_device_name[0] != '\0')
        {
          printf("\nDevice Name of the client: %s", ip_passthrough_config.client_device_name);
        }
      }
      else if (ip_passthrough_config.device_type == QCMAP_MSGR_DEVICE_TYPE_ETHERNET_V01)
      {
        printf("\nPassthrough Device: Ethernet");
      }
      else if (ip_passthrough_config.device_type == QCMAP_MSGR_DEVICE_TYPE_ETHERNET_NIC2_V01)
      {
        printf("\nPassthrough Device: Ethernet-NIC2");
      }
      else if (ip_passthrough_config.device_type == QCMAP_MSGR_DEVICE_TYPE_ANY_AP_V01)
      {
        printf("\nPassthrough Device: WiFi");
      }
      else if (ip_passthrough_config.device_type == QCMAP_MSGR_DEVICE_TYPE_ANY_V01)
      {
        printf("\nIP Passthrough is set for first connected device (Optional MAC Address Configuration)");
      }
      else
      {
        printf("\nDevice Type Not Set");
      }
      /* mac address */
      if (ip_passthrough_config.device_type == QCMAP_MSGR_DEVICE_TYPE_ETHERNET_V01 ||
          ip_passthrough_config.device_type == QCMAP_MSGR_DEVICE_TYPE_ETHERNET_NIC2_V01 ||
          ip_passthrough_config.device_type == QCMAP_MSGR_DEVICE_TYPE_ANY_AP_V01)
      {
        ds_mac_addr_ntop(ip_passthrough_config.mac_addr, mac_addr_str);
        if (strncmp(mac_addr_str,MAC_NULL_STRING,QCMAP_MSGR_MAC_ADDR_NUM_CHARS_V01) != 0)
        {
            printf("\nMAC address of the client: %s", mac_addr_str);
        }
      }
    }
    else
    {
      printf("\nGet IP Passthrough status failed,Error 0x%x", qmi_err_num);
    }
  }
  break;

  /* Get IP Passthrough State */
  case 16:
  {
  boolean active_state;
  if (QcMapClient->GetIPPassthroughState(&active_state,&qmi_err_num))
  {
   if(active_state)
   {
     printf("\nIP Passthrough is ACTIVATED");
   }
   else
     printf("\nIP Passthrough is NOT ACTIVATED");
  }
  else
   printf("Get IP Passthrough state  failed,Error 0x%x", qmi_err_num);
  }
  break;

  /* Set AutoConnect Config */
  case 17:
  {
  boolean enable;
  QCMAP_PRINTF_TAKE_INPUT("   Please input Autoconnect Mode Flag (1-Enable/0-Disable) : ");
  fgets(scan_string, sizeof(scan_string), stdin);
  if (!VALID_NUMERIC_INPUT(scan_string) || (atoi(scan_string) != 1 && atoi(scan_string) != 0))
  {
    printf("\nInvalid response. Please enter 1 or 0.\n");
    break;
  }
  enable = (atoi(scan_string)) ? true : false;
  if ( QcMapClient->SetAutoconnect(enable, &qmi_err_num))
  {
   printf("\nAuto Connect config set succeeds.");
  }
  else
   printf("\nAuto Connect config set fails, Error: 0x%x", qmi_err_num);
   }
  break;

  /* Get AutoConnect Config */
  case 18:
  {
  boolean enable;
  int p_error=0;
  if ( QcMapClient->GetAutoconnect(&enable, &qmi_err_num))
  {
   printf("\nAuto Connect Mode: %s.",(enable)?"Enabled":"Disabled");
  }
  else
   printf("\nAuto Connect config get fails, Error: 0x%x", qmi_err_num);
  }
  break;

  /* Set Roaming */
  case 19:
  {
   boolean enable;
   QCMAP_PRINTF_TAKE_INPUT("   Please input Roaming Mode Flag (1-Enable/0-Disable) : ");
   fgets(scan_string, sizeof(scan_string), stdin);
   if (!VALID_NUMERIC_INPUT(scan_string) || (atoi(scan_string) != 1 && atoi(scan_string) != 0))
   {
     printf("\nInvalid response. Please enter 1 or 0.\n");
     break;
   }
   enable = (atoi(scan_string)) ? true : false;
   if ( QcMapClient->SetRoaming(enable, &qmi_err_num))
   {
     printf("\nRoaming set config succeeds.");
   }
   else
     printf("\nRoaming set config fails, Error: 0x%x", qmi_err_num);
  }
  break;

  /* Get Roaming */
  case 20:
  {
  boolean enable;
  int p_error=0;
  if ( QcMapClient->GetRoaming(&enable, &qmi_err_num))
  {
    printf("\nRoaming Mode: %s.",(enable)? "Enabled" : "Disabled");
  }
  else
    printf("\nRoaming  get fails. Error: 0x%x", qmi_err_num);
  }
  break;

  /* Enable/Disable DDNS */
  case 21:
  {
    QCMAP_PRINTF_TAKE_INPUT("   Please input Dynamic DNS State(1-Enable/0-Disable) : ");
    fgets(scan_string, sizeof(scan_string), stdin);
    if (!VALID_NUMERIC_INPUT(scan_string) || (atoi(scan_string) != 1 && atoi(scan_string) != 0))
    {
      printf("\nInvalid response. Please enter 1 or 0.\n");
      break;
    }
    if (atoi(scan_string))
    {
      if(QcMapClient->EnableDDNS(&qmi_err_num))
      {
        printf("\nEnabled DDNS");
      }
      else
        printf("\nEnable DDNS fails, Error: 0x%x", qmi_err_num);
    }
    else
    {
      if(QcMapClient->DisableDDNS(&qmi_err_num))
      {
        printf("\nDisabled DDNS");
      }
      else
        printf("\nDisable DDNS fails, Error: 0x%x", qmi_err_num);
    }
    break;
  }

  /* Set DDNS Config */
  case 22:
  {
   qcmap_msgr_get_dynamic_dns_config_resp_msg_v01 ddns_server_supported;
   qcmap_msgr_set_dynamic_dns_config_req_msg_v01 ddns_set_config;
   uint32_t value = 0;
   bzero(&ddns_server_supported, sizeof(ddns_server_supported));
   bzero(&ddns_set_config, sizeof(ddns_set_config));
   QCMAP_PRINTF_TAKE_INPUT("\nPlease Find the supported Dynamic server: ");
   if(!QcMapClient->GetDDNSConfig(&ddns_server_supported,&qmi_err_num))
   {
     printf("Error getting the supported ddns server 0x%x.\n ", qmi_err_num);
     break;
   }

   if( ddns_server_supported.ddns_config_len ==0 )
   {
     printf("No ddns server configured 0x%x.\n ", qmi_err_num);
     break;
   }
   printf("\nSupported Server : %s ", ddns_server_supported.ddns_config[0].server_url);
   memcpy( ddns_set_config.ddns_server, ddns_server_supported.ddns_config[0].server_url,
   QCMAP_MSGR_DDNS_URL_LENGTH_V01);
   QCMAP_PRINTF_TAKE_INPUT("\nPlease enter the login ID :");
   fgets(scan_string,sizeof(scan_string),stdin);
   strlcpy( (char *)&ddns_set_config.login, scan_string, strlen(scan_string));
   QCMAP_PRINTF_TAKE_INPUT("\nPlease enter the password :");
   fgets(scan_string,sizeof(scan_string),stdin);
   strlcpy( (char *)&ddns_set_config.password, scan_string, strlen(scan_string));
   QCMAP_PRINTF_TAKE_INPUT("\nPlease enter the Hostname:");
   fgets(scan_string,sizeof(scan_string),stdin);
   strlcpy( (char *)&ddns_set_config.hostname, scan_string, strlen(scan_string));
   QCMAP_PRINTF_TAKE_INPUT("\nPlease enter the timeout :");
   fgets(scan_string, sizeof(scan_string), stdin);
   ddns_set_config.timeout = atoi(scan_string);
   if(!QcMapClient->SetDDNSConfig(&ddns_set_config,&qmi_err_num))
   {
     printf("Error getting the supported ddns server 0x%x.\n ", qmi_err_num);
     break;
   }
   break;
  }

  /* Get DDNS Config */
  case 23:
  {
   qcmap_msgr_get_dynamic_dns_config_resp_msg_v01 ddns_server_supported;
   uint32_t value = 0;
   bzero(&ddns_server_supported, sizeof(ddns_server_supported));
   printf(" Please Find the supported Dynamic Dns server: \n");
   if(!QcMapClient->GetDDNSConfig(&ddns_server_supported,&qmi_err_num))
   {
     printf("Error getting the supported ddns server 0x%x.\n ", qmi_err_num);
     break;
   }
   if( ddns_server_supported.ddns_config_len ==0 )
   {
     printf("No ddns server configured 0x%x.\n ", qmi_err_num);
     break;
   }
   for(int i =0 ; i < ddns_server_supported.ddns_config_len; i++)
   {
     printf("%d. %s \n",i,ddns_server_supported.ddns_config[i].server_url);
   }
   printf("Configured hostname :%s \n",ddns_server_supported.hostname);
   printf("Please enter the timeout :%d \n",ddns_server_supported.timeout);

   if( ddns_server_supported.enable )
     printf("DDNS enabled \n");
   else
     printf("DDNS Disabled \n");

   break;
  }

  /* Switch Profile */
  case 24:
  {
    boolean is_def_pdn = false;
    uint32_t profile_handle = 0;
    profile_handle = ChooseWWANProfileHandle();
    if(QcMapClient->SetWWANProfileHandlePreference(profile_handle, &qmi_err_num))
    {
      printf("Profile-handle updated.");
    }
    else
    {
      printf("Profile-handle Fails!!!");
    }
    break;
  }

  /* Create Profile */
  case 25:
  {
    qcmap_net_policy_info net_policy;
    profile_handle_type_v01 profile_handle;
    memset(&net_policy, 0, sizeof(net_policy));

    QCMAP_PRINTF_TAKE_INPUT("Please select Technology (0-ANY, 1-UMTS, 2-CDMA) : ");
    fgets(scan_string, sizeof(scan_string), stdin);
    if (!VALID_NUMERIC_INPUT(scan_string))
    {
      printf("\nInvalid response\n");
      break;
    }
    net_policy.tech_pref = atoi(scan_string);
    if ( net_policy.tech_pref != 0 && net_policy.tech_pref != 1 &&
        net_policy.tech_pref != 2)
    {
      printf ("\n Invalid tech preference\n");
      break;
    }

    QCMAP_PRINTF_TAKE_INPUT("   Please enter Subscription Id (0-Default, 1-Primary, 2-Secondary): ");
    fgets(scan_string, sizeof(scan_string), stdin);
    net_policy.subscription_id = atoi(scan_string);
    QCMAP_PRINTF_TAKE_INPUT("Please enter UMTS Profile Number : ");
    fgets(scan_string, sizeof(scan_string), stdin);
    net_policy.profile_id_3gpp = atoi(scan_string);
    QCMAP_PRINTF_TAKE_INPUT("   Please enter CDMA Profile Number : ");
    fgets(scan_string, sizeof(scan_string), stdin);
    net_policy.profile_id_3gpp2 = atoi(scan_string);
    QCMAP_PRINTF_TAKE_INPUT("   Please enter the APN name on which the call needs to be brought up: ");
    fgets(scan_string,sizeof(scan_string),stdin);
    strlcpy(net_policy.apn_name, scan_string, strlen(scan_string));

    QCMAP_PRINTF_TAKE_INPUT("Please select ip family (4-V4, 6-V6, 10-V4V6,12-ETH) : ");
    fgets(scan_string, sizeof(scan_string), stdin);
    net_policy.ip_family = atoi(scan_string);
    if ( net_policy.ip_family != QCMAP_MSGR_IP_FAMILY_V4_V01 &&
         net_policy.ip_family != QCMAP_MSGR_IP_FAMILY_V6_V01 &&
         net_policy.ip_family != QCMAP_MSGR_IP_FAMILY_V4V6_V01 &&
         net_policy.ip_family != QCMAP_MSGR_IP_FAMILY_ETH_V01)
    {
      printf ("\n Invalid ip familiy\n");
      break;
    }

    if (QcMapClient->CreateWWANPolicyEx(net_policy, &profile_handle, &qmi_err_num))
    {
      printf("  Create WWAN policy succeeds, profile_handle=%d\n. ", profile_handle);
    }
    else
    {
      if (qmi_err_num == QMI_ERR_NO_FREE_PROFILE_V01)
        printf("  Max Profiles reached, Error 0x%x\n", qmi_err_num);
      else if (qmi_err_num == QMI_ERR_INVALID_PROFILE_V01)
        printf ("  Invalid/Duplicate Profile request, Error 0x%x", qmi_err_num);
      else
        printf("  Failed to Create WWAN policy. Error 0x%x\n ", qmi_err_num);
    }
    break;
  }

  /* Update Profile */
  case 26:
  {
    int array_size;
    int backhaulWWANUpdateOpt;
    array_size = sizeof(backhaul_wwan_update_configuration_list)/sizeof(backhaul_wwan_update_configuration_list[0]);
    for (int i=0; i<array_size; i++)
    {
      printf("%s\n",backhaul_wwan_update_configuration_list[i]);
    }

    QCMAP_PRINTF_TAKE_INPUT("Please select an option : ");
    fgets(scan_string, sizeof(scan_string), stdin);
    backhaulWWANUpdateOpt = atoi(scan_string);
    backhaulWWANUpdateConfig( backhaulWWANUpdateOpt );
    break;
  }

  /* Delete Profile */
  case 27:
  {
    qmi_err_num = QMI_ERR_NONE_V01;
    if(QcMapClient->DeleteWWANPolicy(&qmi_err_num))
    {
      printf("Delete Policy succeeds");
    }
    else
    {
      printf("Delete Policy Fails!!! Error 0x%x.\n", qmi_err_num);
      if (qmi_err_num == QMI_ERR_DEVICE_IN_USE_V01)
        printf("    Data-call is active for this profile. Disconnect Backhaul and try again.\n");
      else if (qmi_err_num == QMI_ERR_INVALID_HANDLE_V01)
        printf("    Profile is default or doesn't exists.\n");
      else
        printf("    Unknown error.\n");
    }
    break;
  }

  /* Add/Delete PDN to VLAN Mapping */
  case 28:
  {
    int16_t chosen_bridge = -1;
    memset(scan_string, 0, QCMAP_MSGR_MAX_FILE_PATH_LEN);
    QCMAP_PRINTF_TAKE_INPUT("Please input (1-Add/0-Delete) PDN to VLAN Mapping: ");
    fgets(scan_string, sizeof(scan_string), stdin);
    if (!VALID_NUMERIC_INPUT(scan_string))
    {
      printf("\nInvalid response\n");
      break;
    }
    switch(atoi(scan_string))
    {
      case(0): //Delete case
      {
        printf("Please select VLAN/Bridge:\n");
        fflush(stdout);
        if((chosen_bridge = ChooseLANBridge()) >= 0)
        {
          if(!QcMapClient->SelectLANBridge(chosen_bridge, &qmi_err_num))
          {
            printf("Failed to select LAN bridge: 0x%x", qmi_err_num);
            return;
          }
        } else {
          printf("Invalid LAN Bridge: %d", chosen_bridge);
          return;
        }
        printf("Please select PDN to disassociate from VLAN/Bridge %d:\n", chosen_bridge);
        profile_handle = ChooseWWANProfileHandle();

        if(!QcMapClient->DeletePDNToVLANMapping(chosen_bridge, profile_handle, &qmi_err_num))
        {
          printf("Failed to Delete VLAN/Bridge %d from profile_handle %d: 0x%x\n", chosen_bridge,
                 profile_handle, qmi_err_num);
          return;
        }
        printf("Succesfully Deleted VLAN/Bridge %d from profile_handle %d\n", chosen_bridge,
                profile_handle);
        break;
      }
      case(1): //Add case
      {
        QCMAP_PRINTF_TAKE_INPUT("Please select VLAN/Bridge:\n");
        if((chosen_bridge = ChooseLANBridge()) >= 0)
        {
          if(!QcMapClient->SelectLANBridge(chosen_bridge, &qmi_err_num))
          {
            printf("Failed to select LAN bridge: 0x%x", qmi_err_num);
            return;
          }
        } else {
          printf("Invalid LAN Bridge: %d", chosen_bridge);
          return;
        }
        QCMAP_PRINTF_TAKE_INPUT("Please select PDN to map to VLAN/Bridge %d:\n", chosen_bridge);
        profile_handle = ChooseWWANProfileHandle();

        if(!QcMapClient->AddPDNToVLANMapping(chosen_bridge, profile_handle, &qmi_err_num))
        {
          printf("Failed to Add VLAN/Bridge %d to profile_handle %d: 0x%x\n", chosen_bridge,
                 profile_handle, qmi_err_num);
          return;
        }
        printf("Succesfully Added VLAN/Bridge %d to profile_handle %d\n", chosen_bridge,
                profile_handle);
        break;
      }
      default:
      {
        printf("Invalid input: %s\n", scan_string);
        return;
        break;
      }
    }
    break;
  }

  /* Get All PDN to VLAN Mappings */
  case 29:
  {
    int num_entries = 0;
    qcmap_msgr_pdn_to_vlan_mapping_ex_v01 mappings_ex[QCMAP_MAX_NUM_BACKHAULS_V01];
    ZERO_INIT_ARG(mappings_ex);

    if(QcMapClient->GetPDNtoVLANMappingsEx(mappings_ex, &num_entries, &qmi_err_num))
    {
      /*mappings_ex check*/
      if (mappings_ex == NULL) 
      {
        printf("\nmappings_ex is null");
        break;
      }

      printf("|PDN|VLAN-ID|\n");
      printf("-------------\n");
      for (int i=0; i<num_entries; i++)
      {
        for(int j = 0; j < mappings_ex[i].vlan_id_len; j++)
        {
          printf("  %1d%6d\n",mappings_ex[i].profile_handle, mappings_ex[i].vlan_id[j]);
        }
      }
    }
    else
    {
      printf("No current mappings");
    }
    break;
  }
  /* Set PMIP mode configuration */
  case 30:
  {
    int  pmip_mode_enabled = 0;
    int pmip_mode_type = 0;
    int is_pmip_debug_mode=0;
    int pmip_tunnel_mode = 0;
    int pmipv4_work_mode = 0;
    int pmip_mobile_node_identifier_type = 0;
    int modify_service_selection_string = 0;
    struct ps_in_addr lma_v4_ip = {0};
    struct ps_in_addr dmnp_prefix = {0};
    int dmnp_prefix_len = 0;
    struct ps_in6_addr lma_v6_ip = {0};
    struct in_addr addr = {0};

    qmi_error_type_v01 qmi_err_num;
    /* Initialize QMI Error Number. */
    qmi_err_num = QMI_ERR_NONE_V01;
    char scan_string[QCMAP_MSGR_MAX_FILE_PATH_LEN];
    char id_string[MAX_ID_STRING_LEN] = {0};
    qcmap_msgr_set_pmip_mode_req_msg_v01  set_pmip_mode_req_msg;


    memset( (void*)&set_pmip_mode_req_msg, 0x0, sizeof(qcmap_msgr_set_pmip_mode_req_msg_v01));
    QCMAP_PRINTF_TAKE_INPUT("  Please input PMIP mode(1-Enable/0-Disable) : \n");
    fgets(scan_string, sizeof(scan_string), stdin);
    if (!VALID_NUMERIC_INPUT(scan_string))
    {
      printf("\nInvalid response\n");
      break;
    }
    pmip_mode_enabled = atoi(scan_string);
    if (pmip_mode_enabled == 1)
    {
      set_pmip_mode_req_msg.enable_pmip_mode = pmip_mode_enabled;
      printf(" Please input pmip Logging mode : 0-Normal Mode/1-Debug Mode\n");
      QCMAP_PRINTF_TAKE_INPUT ("In debug mode Logs will to written in File. Enter choice:");
      fgets(scan_string, sizeof(scan_string), stdin);
      if (!VALID_NUMERIC_INPUT(scan_string))
      {
        printf("\nInvalid response\n");
        break;
      }
      if((atoi(scan_string) != 0) && (atoi(scan_string) != 1))
      {
         printf ("\nInvalid input for debug enable %d. Expecting 0/1\n",atoi(scan_string));
         break;
      }
      is_pmip_debug_mode = atoi(scan_string);
      set_pmip_mode_req_msg.enable_pmip_debug_mode= is_pmip_debug_mode;

      printf ("\nPlease enter LMA ip Address type 0-v4, 1-v6\n");
      printf ("If type v4 then gre Tunnel over v4 address,If v6 then\n");
      QCMAP_PRINTF_TAKE_INPUT (" gre tunnel over v6 adress for pmip mode.Enter Choice:");


      fgets(scan_string, sizeof(scan_string), stdin);
      if(atoi(scan_string) == 0)
      {
        printf("\nPlease enter LMA V4 Address : [x:x:x:x]:");
        read_addr(AF_INET,(uint8 *)&lma_v4_ip.ps_s_addr);
        set_pmip_mode_req_msg.lma_ipv4_addr_valid = true;
        set_pmip_mode_req_msg.lma_ipv4_addr = lma_v4_ip.ps_s_addr;
        addr.s_addr= lma_v4_ip.ps_s_addr;
        printf("  \nEnabling PMIP mode with LMA v4 IP=%s... \n",inet_ntoa(addr));
      }
      else
      {
        printf("\nPlease enter LMA V6 Address : [xxxx:xxxx::xxxx:xxxx:xxxx]:");
        read_addr(AF_INET6,(uint8 *)&lma_v6_ip.in6_u.u6_addr8);
        set_pmip_mode_req_msg.lma_ipv6_addr_valid = true;
        memcpy(set_pmip_mode_req_msg.lma_ipv6_addr, &lma_v6_ip, sizeof( struct ps_in6_addr));
      }

      QCMAP_PRINTF_TAKE_INPUT("  Please input PMIP Mode: 0-v4/1-v6/2-v4v6\n");
      fgets(scan_string, sizeof(scan_string), stdin);
      if (!VALID_NUMERIC_INPUT(scan_string))
      {  
        printf("\nInvalid response\n");
        break;
      }
      pmip_mode_type = atoi(scan_string);
      if (pmip_mode_type == 0)
      {
        set_pmip_mode_req_msg.pmip_mode_type = QCMAP_MSGR_IP_FAMILY_V4_V01;
      }
      else if (pmip_mode_type == 1)
      {
        set_pmip_mode_req_msg.pmip_mode_type = QCMAP_MSGR_IP_FAMILY_V6_V01;
      }
      else if (pmip_mode_type == 2)
      {
        set_pmip_mode_req_msg.pmip_mode_type = QCMAP_MSGR_IP_FAMILY_V4V6_V01;
      }
      else
      {
        printf("\nInvalid pmip mode . setting to v6 pmip moed");
        set_pmip_mode_req_msg.pmip_mode_type = QCMAP_MSGR_IP_FAMILY_V6_V01;
        pmip_mode_type = 1;
      }
      if ((pmip_mode_type == 0) || (pmip_mode_type == 1) || (pmip_mode_type == 2))
      {
        QCMAP_PRINTF_TAKE_INPUT("\nPlease Enter PMIP Mobile node identifier type used in PBU[\n1-Mobile Node Identifier String\n2-MAC address\n]:");
        fgets(scan_string, sizeof(scan_string), stdin);
        pmip_mobile_node_identifier_type = atoi(scan_string);
        if (pmip_mobile_node_identifier_type == 1)
        {
          QCMAP_PRINTF_TAKE_INPUT("\nPlease Enter Mobile node identifier string:\n");
          fgets(id_string, sizeof(id_string), stdin);
          id_string[strlen(id_string)-1] = '\0';
          set_pmip_mode_req_msg.pmip_mobile_node_identifier_type_valid = true;
          set_pmip_mode_req_msg.pmip_mobile_node_identifier_type = QCMAP_MSGR_PMIP_MOBILE_NODE_IDENTIFIER_STRING_V01;

          memset(set_pmip_mode_req_msg.pmip_mn_id_string,0,QCMAP_MSGR_PMIP_MN_ID_STRING_LENGTH_V01+ 1);
          set_pmip_mode_req_msg.pmip_mn_id_string_valid = true;
          memcpy(set_pmip_mode_req_msg.pmip_mn_id_string,id_string,strlen(id_string));
        }
        else if (pmip_mobile_node_identifier_type == 2)
        {
          set_pmip_mode_req_msg.pmip_mobile_node_identifier_type_valid = true;
          set_pmip_mode_req_msg.pmip_mobile_node_identifier_type = QCMAP_MSGR_PMIP_MOBILE_NODE_IDENTIFIER_MAC_V01;
        }
        else
        {
          printf("\nInvalid Identifier type - %d\n", pmip_mobile_node_identifier_type);
          break;
        }
        QCMAP_PRINTF_TAKE_INPUT("\nAdd Service Selection String in PBU [1-Yes/0-NO]:");
        fgets(scan_string, sizeof(scan_string), stdin);
        if (!VALID_NUMERIC_INPUT(scan_string))
        {
          printf("\nInvalid response\n");
          break;
        }
        modify_service_selection_string = atoi(scan_string);
        if (modify_service_selection_string == 0)
        {
          printf("\nService Selection string will not be added in PBU if server has invalid string:\n");
        }
        else if (modify_service_selection_string == 1)
        {
          QCMAP_PRINTF_TAKE_INPUT("\nPlease Enter Service Selection string :\n");
          fgets(id_string, sizeof(id_string), stdin);
          id_string[strlen(id_string)-1] = '\0';
          memset(set_pmip_mode_req_msg.pmip_service_selection_string,0,QCMAP_MSGR_PMIP_SERVICE_SELECTION_STRING_LENGTH_V01 + 1);
          set_pmip_mode_req_msg.pmip_service_selection_string_valid= true;
          memcpy(set_pmip_mode_req_msg.pmip_service_selection_string,id_string,strlen(id_string));
        }
        else
        {
          printf("\nInvalid Option - %d\n", modify_service_selection_string);
          break;
        }
      }

      //If pmip type is v4 ask more info
      if((pmip_mode_type == 0) || (pmip_mode_type == 2))
      {
        QCMAP_PRINTF_TAKE_INPUT("\nPlease Enter Pmipv4 working mode [0-CPE Mode/1-Secondary Router mode]:");
        fgets(scan_string, sizeof(scan_string), stdin);
        if (!VALID_NUMERIC_INPUT(scan_string))
        {
          printf("\nInvalid response\n");
          break;
        }
        pmipv4_work_mode = atoi(scan_string);
        if (pmipv4_work_mode == 0)
        {
          printf("  Pmip v4 CPE Mode... \n");
          set_pmip_mode_req_msg.pmipv4_mode_type_valid = true;
          set_pmip_mode_req_msg.pmipv4_mode_type = QCMAP_MSGR_PMIPV4_MODE_CPE_V01;
        }
        else if (pmipv4_work_mode == 1)
        {
          printf("  Pmip v4 Secondary Router Mode... \n");
          set_pmip_mode_req_msg.pmipv4_mode_type_valid = true;
          set_pmip_mode_req_msg.pmipv4_mode_type = QCMAP_MSGR_PMIPV4_MODE_SECONDARY_ROUTER_V01;
          printf("\nPlease enter DMNP PREFIX : [x:x:x:x]:");
          read_addr(AF_INET,(uint8 *)&dmnp_prefix.ps_s_addr);
          set_pmip_mode_req_msg.pmipv4_sec_router_param_valid= true;
          set_pmip_mode_req_msg.pmipv4_sec_router_param.dmnp_prefix= dmnp_prefix.ps_s_addr;
          addr.s_addr= dmnp_prefix.ps_s_addr;
          printf(" DMNP prefix=%s... \n",inet_ntoa(addr));

          QCMAP_PRINTF_TAKE_INPUT("\nPlease Enter DMNP Prefix length:");
          fgets(scan_string, sizeof(scan_string), stdin);
          dmnp_prefix_len = atoi(scan_string);
          set_pmip_mode_req_msg.pmipv4_sec_router_param.prefix_len = dmnp_prefix_len;
        }
        else
        {
          printf("\nInvalid option - %d\n", pmipv4_work_mode);
          break;
        }
      }
      if (QcMapClient->SetPMIPMode(&set_pmip_mode_req_msg, &qmi_err_num))
        printf("SetPMIPMode to Enable succeeded \n");
      else
        printf("SetPMIPMode fails , Error: 0x%x\n", qmi_err_num);
    }
    else if (pmip_mode_enabled == 0)
    {
      printf("   Disabling PMIP mode ... \n");
      set_pmip_mode_req_msg.enable_pmip_mode = pmip_mode_enabled;
      if (QcMapClient->SetPMIPMode(&set_pmip_mode_req_msg, &qmi_err_num))
        printf("SetPMIPMode to Disable succeeded \n");
      else
        printf("SetPMIPMode fails, Error: 0x%x\n", qmi_err_num);
    }
    else
    {
      printf("\nInvalid option - %d\n", pmip_mode_enabled);
    }
    break;
  }

  /* Get PMIP mode configuration */
  case 31:
  {
    int  pmip_mode_enabled = 0;
    int pmip_mode_type = 0;
    struct ps_in_addr lma_v4_ip;
    struct ps_in6_addr lma_v6_ip;
    qmi_error_type_v01 qmi_err_num;
    /* Initialize QMI Error Number. */
    qmi_err_num = QMI_ERR_NONE_V01;
    char scan_string[QCMAP_MSGR_MAX_FILE_PATH_LEN];
    qcmap_msgr_get_pmip_mode_resp_msg_v01  get_pmip_mode_resp_msg;
    memset( (void*)&get_pmip_mode_resp_msg, 0x0, sizeof(qcmap_msgr_get_pmip_mode_resp_msg_v01));
    printf("  Getting PMIP mode: ");
    if (QcMapClient->GetPMIPMode(&get_pmip_mode_resp_msg, &qmi_err_num))
    {
      char  ip6_addr_str[INET6_ADDRSTRLEN];
      char  ip4_addr_str[INET_ADDRSTRLEN];
      printf("GetPMIPmode succeeds.\n---- Status: %d----\n",get_pmip_mode_resp_msg.pmip_mode);
      printf("---- pmip_mode_type: %d----\n",get_pmip_mode_resp_msg.pmip_mode_type);
      if (get_pmip_mode_resp_msg.pmip_mode_type == QCMAP_MSGR_IP_FAMILY_V4_V01)
      {
        printf ("Pmip is in v4 Mode\n");
      }
      else if (get_pmip_mode_resp_msg.pmip_mode_type == QCMAP_MSGR_IP_FAMILY_V6_V01)
      {
        printf ("Pmip is in v6 Mode\n");
      }
      else if (get_pmip_mode_resp_msg.pmip_mode_type == QCMAP_MSGR_IP_FAMILY_V4V6_V01)
      {
        printf ("Pmip is in V4V6 Mode\n");
      }
      else
      {
        printf ("Pmip is in Invalid Mode\n");
      }
      if(get_pmip_mode_resp_msg.lma_ipv6_addr_valid == 1)
      {
        inet_ntop(AF_INET6, (uint8_t*)get_pmip_mode_resp_msg.lma_ipv6_addr,
                  ip6_addr_str, INET6_ADDRSTRLEN);
        printf("\nTunnel Mode is v6\n");
        printf("---- LMA V6 Addres: %s----\n",ip6_addr_str);
      }
      else if(get_pmip_mode_resp_msg.lma_ipv4_addr_valid == 1)
      {
        inet_ntop(AF_INET, (void *)&get_pmip_mode_resp_msg.lma_ipv4_addr,
                  ip4_addr_str, INET_ADDRSTRLEN);
        printf("\nTunnel Mode is v4\n");
        printf("---- LMA V4 Addres: %s----\n",ip4_addr_str);
      }
      else
      {
        printf("\nEither Tunnel Mode is Invalid Or Tunnel Ip not provided\n");
      }
      if (get_pmip_mode_resp_msg.pmip_mobile_node_identifier_type_valid == true)
      {
        if (get_pmip_mode_resp_msg.pmip_mobile_node_identifier_type == QCMAP_MSGR_PMIP_MOBILE_NODE_IDENTIFIER_STRING_V01)
        {
          printf ("\nPmip Mobile node Identifier type is String.\n");
          if (get_pmip_mode_resp_msg.pmip_mn_id_string_valid)
            printf ("\nPmip Mobile node Identifier type is string %s\n",get_pmip_mode_resp_msg.pmip_mn_id_string);
          else
            printf ("\nget_pmip_mode_resp_msg.pmip_mn_id_string_valid flag is false\n");
        }
        else if (get_pmip_mode_resp_msg.pmip_mobile_node_identifier_type == QCMAP_MSGR_PMIP_MOBILE_NODE_IDENTIFIER_MAC_V01)
        {
          printf ("\nPmip Mobile node Identifier type is MAC address.\n");
        }
        else
        {
          printf("\nInvalid pmip_mobile_node_identifier_type %d\n",get_pmip_mode_resp_msg.pmip_mobile_node_identifier_type);
        }
      }
      else
      {
        printf("\npmip_mobile_node_identifier_type not provided by server\n");
      }

      if (get_pmip_mode_resp_msg.pmip_service_selection_string_valid == true)
      {
        printf ("\nPmip Service Selection string '%s'\n",get_pmip_mode_resp_msg.pmip_service_selection_string);
      }
      else
      {
        printf ("\nPmip Service Selection string not provided by server\n");
      }

      if (get_pmip_mode_resp_msg.pmip_mode_type == QCMAP_MSGR_IP_FAMILY_V4_V01 ||
          get_pmip_mode_resp_msg.pmip_mode_type == QCMAP_MSGR_IP_FAMILY_V4V6_V01)
      {
        if (get_pmip_mode_resp_msg.pmipv4_mode_type == QCMAP_MSGR_PMIPV4_MODE_CPE_V01)
        {
          printf ("\nPmip v4 is in CPE Mode\n");
        }
        else if (get_pmip_mode_resp_msg.pmipv4_mode_type == QCMAP_MSGR_PMIPV4_MODE_SECONDARY_ROUTER_V01)
        {
          printf ("\nPmip v4 is in Secondary Router Mode\n");
          inet_ntop(AF_INET, (void *)&get_pmip_mode_resp_msg.pmipv4_sec_router_param.dmnp_prefix,
                    ip4_addr_str, INET_ADDRSTRLEN);
          printf("\n---- Secondary Router Mode Prefix: %s----\n",ip4_addr_str);
          printf ("\nSecondary Router Mode prefix Length %d\n",
                  get_pmip_mode_resp_msg.pmipv4_sec_router_param.prefix_len);
        }
        else
        {
          printf ("\nPmip v4  mode getting failed mode type %d \n",get_pmip_mode_resp_msg.pmipv4_mode_type);
        }
      }
    }
    else
      printf("GetPMIPmode fails, Error: 0x%x\n", qmi_err_num);
    break;
  }

  /* Get WWAN Roaming status*/
  case 32:
  {
    uint8_t roam_status = 0;
    if (!QcMapClient->GetWWANRoamStatus(&roam_status, &qmi_err_num))
    {
      printf("Error getting the Roaming status 0x%x.\n ", qmi_err_num);
      break;
    }
    printf("WWAN Roaming status :%d \n",roam_status);
    break;
  }

  /*Get current profile handle*/
  case 33:
  {
    profile_handle_type_v01 current_profile_handle;
    if (!QcMapClient->GetWWANProfilePreference(&current_profile_handle, &qmi_err_num))
    {
      printf("Error getting current profile handle 0x%x.\n ", qmi_err_num);
      break;
    }
    printf("Current Profile Handle :%d \n",current_profile_handle);
    break;
  }

  /*EoGRE Configuration Options*/
  case 34:
  {
    int array_size = 0;
    int eogre_option = 0;
    array_size = sizeof(eogre_config_list)/sizeof(eogre_config_list[0]);
    printf("\n");
    for (int i=0; i<array_size; i++)
    {
      printf("%s\n",eogre_config_list[i]);
    }
    if(fgets(scan_string, sizeof(scan_string), stdin) == NULL)
      break;

    eogre_option = atoi(scan_string);

    eogreTunnelConfig(eogre_option);
  }
  break;

  /* Set IP Passthrough SW Path Filters */
  case 35:
  {
    int type_of_filter = 0, port_no = 0, operation = 0, index = 0;
    qcmap_msgr_sw_path_filters_conf_t filter_config;
    qmi_error_type_v01 qmi_err_num = QMI_ERR_NONE_V01;
    struct in_addr addr;
    int protocol = 0;

    ZERO_INIT_ARG(filter_config);

    filter_config.filter_type = QCMAP_MSGR_IP_PT_SW_PATH_PORT_PROTOCOL_IP_FILTER_V01;

    printf("\n\n------------------------------------------------------------------------"
           "\nSoftware Path Filter Configuration"
           "\n------------------------------------------------------------------------");

    /* Description */
    printf("\nTraffic destined to the public gateway IP of the LAN gateway interface"
           "\n(bridge) will take Software Path irrespecitve of destination port and"
           "\nprotocol."
           "\nTraffic destined to the public gateway IP of the bridge with configured"
           "\ndestination port range and protocol will be consumed on the bridge"
           "\n------------------------------------------------------------------------");

    /* Print currently configured filters */
    if (!QcMapClient->GetIPPassthroughSoftwarePathFilters(&filter_config, &qmi_err_num))
    {
      printf("\nError in fetching the currently configured filters. error: 0x%x", qmi_err_num);
      break;
    }
    if (!filter_config.num_of_filters)
    {
      printf("\nNo filters are configured");
    }
    else
    {
      printf("\n\nCurrently %d filters are configured: ", filter_config.num_of_filters);

      if (filter_config.public_gateway_ip)
      {
        addr.s_addr = filter_config.public_gateway_ip;
        printf("\n\nPublic Gateway IP: %s", inet_ntoa(addr));
      }
      else
      {
        printf("\n\nPublic Gateway IP is not set, as IP Passthrough is not enabled yet");
      }

      for (uint8_t i = 0; i < filter_config.num_of_filters; i++)
      {
        printf("\n[%d]: ", i+1);
        if (filter_config.filters[i].port_range.start_port == filter_config.filters[i].port_range.end_port)
        {
          printf("Port: %d, ", filter_config.filters[i].port_range.start_port);
        }
        else
        {
          printf("Port Range: %d-%d, ", filter_config.filters[i].port_range.start_port,
                 filter_config.filters[i].port_range.end_port);
        }
        printf("Protocol: ");
        if (filter_config.filters[i].protocol == QCMAP_MSGR_PROTO_TCP_V01)
        {
          printf("TCP");
        }
        else if (filter_config.filters[i].protocol == QCMAP_MSGR_PROTO_UDP_V01)
        {
          printf("UDP");
        }
        else
        {
          printf("Error in printing protocol");
        }
      }
    }
    /* Operation to be performed */
    QCMAP_PRINTF_TAKE_INPUT("\n\nWhat operation do you want to perform? 1: Add, 2: Delete, 3: Exit: ");
    memset(scan_string, 0, QCMAP_MSGR_MAX_FILE_PATH_LEN);
    if (strlen(scan_string) == 0 && scan_string[0] == '\0')
    {
      fgets(scan_string, sizeof(scan_string), stdin);
    }
    else
    {
      printf("\nerror scan string is not empty");
      break;
    }
    operation = atoi(scan_string);
    if ((operation != 1) && (operation != 2) && (operation != 3))
    {
      printf("\nInvalid Input");
      break;
    }

    if (operation == 1)
    {
      if (filter_config.num_of_filters == QCMAP_CM_MAX_SW_PATH_FILTERS)
      {
        printf("\nMaximum number of filters are already configured.", 0, 0, 0);
        break;
      }

      index = filter_config.num_of_filters;
      filter_config.num_of_filters = filter_config.num_of_filters + 1;

      printf("\nPlease provide the port range: ");

      printf("\nStart Port: ");
      READ_AND_VALIDATE_INT_VALUE(port_no, 1, 0xfffe);
      filter_config.filters[index].port_range.start_port = port_no;

      printf("\nEnd Port (must be equal to or greater than Start Port): ");
      READ_AND_VALIDATE_INT_VALUE(port_no, filter_config.filters[index].port_range.start_port, 0xfffe);
      filter_config.filters[index].port_range.end_port = port_no;

      printf("\nPlease provide the protocol (1: TCP, 2: UDP): ");
      READ_AND_VALIDATE_INT_VALUE(protocol, 1, 2);
      filter_config.filters[index].protocol = protocol;
    }
    else if (operation == 2)
    {
      int entry_no = 0;
      qcmap_msgr_sw_path_filters_conf_t temp_filter_config;

      if (!filter_config.num_of_filters)
      {
        printf("\nNothing to delete as no filters are configured");
        break;
      }

      printf("\nPlease provide the index number of the filter to delete:  ");
      READ_AND_VALIDATE_INT_VALUE(entry_no, 1, filter_config.num_of_filters);

      ZERO_INIT_ARG(temp_filter_config);
      temp_filter_config.filter_type = QCMAP_MSGR_IP_PT_SW_PATH_PORT_PROTOCOL_IP_FILTER_V01;
      temp_filter_config.num_of_filters = filter_config.num_of_filters - 1;
      index = 0;

      for (uint8_t i = 0; i < filter_config.num_of_filters; i++)
      {
        if (i == (entry_no - 1))
        {
          continue;
        }
        temp_filter_config.filters[index].port_range.start_port = filter_config.filters[i].port_range.start_port;
        temp_filter_config.filters[index].port_range.end_port = filter_config.filters[i].port_range.end_port;
        temp_filter_config.filters[index].protocol = filter_config.filters[i].protocol;
        index++;
      }

      filter_config = temp_filter_config;
    }
    else
    {
      printf("\nSkipping this menu");
      break;
    }

    if (QcMapClient->SetIPPassthroughSoftwarePathFilters(&filter_config, &qmi_err_num) && (qmi_err_num == QMI_ERR_NONE_V01))
    {
      printf("\nSuccessfully configured ip passthrough software path filters");
    }
    else
    {
      printf("\nError in configuring the ip passthrough software path filters, error: 0x%x", qmi_err_num);
    }
  }
  break;

  /* Get IP Passthrough SW Path Filters */
  case 36:
  {
    int type_of_filter = 0;
    qcmap_msgr_sw_path_filters_conf_t filter_config;
    qmi_error_type_v01 qmi_err_num;
    struct in_addr addr;

    ZERO_INIT_ARG(filter_config);

    filter_config.filter_type = QCMAP_MSGR_IP_PT_SW_PATH_PORT_PROTOCOL_IP_FILTER_V01;

    printf("\n-----------------------------------------------------------"
           "\nSoftware Path Filters"
           "\n-----------------------------------------------------------");

    if (!QcMapClient->GetIPPassthroughSoftwarePathFilters(&filter_config, &qmi_err_num))
    {
      printf("\nError in fetching the currently configured filters. error: 0x%x", qmi_err_num);
      break;
    }
    if (!filter_config.num_of_filters)
    {
      printf("\nNo filters are configured");
    }
    else
    {
      printf("\n\nCurrently %d filters are configured: ", filter_config.num_of_filters);
      if (filter_config.public_gateway_ip)
      {
        addr.s_addr = filter_config.public_gateway_ip;
        printf("\n\nPublic Gateway IP: %s", inet_ntoa(addr));
      }
      else
      {
        printf("\n\nPublic Gateway IP is not set, as IP Passthrough is not enabled yet");
      }
      for (uint8_t i = 0; i < filter_config.num_of_filters; i++)
      {
        printf("\n\n[%d]. ", i+1);
        if (filter_config.filters[i].port_range.start_port == filter_config.filters[i].port_range.end_port)
        {
          printf("Port: %d, ", filter_config.filters[i].port_range.start_port);
        }
        else
        {
          printf("Port Range: %d-%d, ", filter_config.filters[i].port_range.start_port,
                 filter_config.filters[i].port_range.end_port);
        }
        printf("Protocol: ");
        if (filter_config.filters[i].protocol == QCMAP_MSGR_PROTO_TCP_V01)
        {
          printf("TCP");
        }
        else if (filter_config.filters[i].protocol == QCMAP_MSGR_PROTO_UDP_V01)
        {
          printf("UDP");
        }
        else
        {
          printf("Error in printing protocol");
        }
      }
      printf("\n");
    }
  }
  break;

  /* Set DHCPv6 DNS proxy config */
  case 37:
  {
    int opt;
    qcmap_msgr_config_state_enum_v01 dhcpv6_dns_state;

    ASK_USER_FOR_INPUT_INT_PARAM("Enter the state to set DHCPv6 DNS(1-Enable/0-Disable): ", opt);
    if (!VALID_NUMERIC_INPUT(scan_string) || (atoi(scan_string) != 1 && atoi(scan_string) != 0))
    {
      printf("\nInvalid response. Please enter 1 or 0.\n");
      break;
    }

    if(opt == 0)
      dhcpv6_dns_state = QCMAP_MSGR_CONFIG_DISABLE_V01;
    else if (opt == 1)
      dhcpv6_dns_state = QCMAP_MSGR_CONFIG_ENABLE_V01;
    else
    {
      printf("Unrecognised option: %d\n", opt);
      break;
    }

    QcMapClient->SetDhcpv6DNSConfig(dhcpv6_dns_state, &qmi_err_num);

    if(qmi_err_num == QMI_ERR_NONE_V01)
    {
      printf("DHCPv6 DNS proxy successfully %s.\n",
        (dhcpv6_dns_state == QCMAP_MSGR_CONFIG_ENABLE_V01)?("enabled"):("disabled"));
    }
    else if(qmi_err_num == QMI_ERR_NO_EFFECT_V01)
    {
      printf("DHCPv6 DNS proxy is already %s.\n",
        (dhcpv6_dns_state == QCMAP_MSGR_CONFIG_ENABLE_V01)?("enabled"):("disabled"));
    }
    else if(qmi_err_num == QMI_ERR_INVALID_HANDLE_V01)
    {
      printf("\nMobileAP is not enabled\n");
      break;
    }
    else
    {
      printf("Error occurred: 0x%x\n", qmi_err_num);
    }

    break;
  }

  /* Get DHCPv6 DNS proxy state */
  case 38:
  {
    qcmap_msgr_config_state_enum_v01 dhcpv6_dns_state;

    QcMapClient->GetDhcpv6DNSConfig(&dhcpv6_dns_state, &qmi_err_num);

    if(qmi_err_num == QMI_ERR_NONE_V01)
    {
      printf("DHCPv6 DNS proxy is %s.\n",
        (dhcpv6_dns_state == QCMAP_MSGR_CONFIG_ENABLE_V01)?("enabled"):("disabled"));
    }
    else
    {
      printf("Error occured: 0x%x\n", qmi_err_num);
    }
    break;
  }

  /* Set/Reset IPv6 External Router Mode */
  case 39:
  {
    int enable = 0;
    qmi_error_type_v01 qmi_err_num = QMI_ERR_NONE_V01;
    qcmap_ipv6_ext_router_mode_config config;

    printf("\nIPv6 External Router Mode: (1: Enable, 2: Disable, 0: Cancel): ");
    READ_AND_VALIDATE_INT_VALUE(enable, 0, 2);
    if (enable == 0)
    {
      break;
    }
    else
    {
      ZERO_INIT_ARG(config);
      if (enable == 1)
      {
        config.enable = true;
        if (QcMapClient->SetIPv6ExtRouterMode(&config, &qmi_err_num))
        {
          printf("\nSuccessfully enabled IPv6 External Router Mode\n");
        }
        else
        {
          printf("\nError in enabling IPv6 External Router Mode: 0x%x\n", qmi_err_num);
        }
      }
      else
      {
        if (QcMapClient->SetIPv6ExtRouterMode(&config, &qmi_err_num))
        {
          printf("\nSuccessfully disabled IPv6 External Router Mode\n");
        }
        else
        {
          printf("\nError in disabling IPv6 External Router Mode: 0x%x\n", qmi_err_num);
        }
      }
    }
    break;
  }

  /* Get IPv6 External Router Mode */
  case 40:
  {
    boolean enable = false;
    boolean delegate_all = false;
    qmi_error_type_v01 qmi_err_num = QMI_ERR_NONE_V01;
    qcmap_ipv6_ext_router_mode_config config;
    ZERO_INIT_ARG(config);

    // call API
    if (QcMapClient->GetIPv6ExtRouterMode(&config, &qmi_err_num))
    {
      printf("\nIPv6 External Router Mode is ");
      config.enable ? printf("enabled.\n") : printf("disabled.\n");
    }
    else
    {
      printf("\nError in getting IPv6 External Router Mode: 0x%x", qmi_err_num);
    }
    break;
  }

  /* Not Supported */
  case 41:
  {
    break;
  }

  /* Enable/Get QoS indications */
  case 42:
  {
    array_size = sizeof(backhaul_wwan_qos_flow_ind_list)/sizeof(backhaul_wwan_qos_flow_ind_list[0]);
    for (int i=0; i<array_size; i++)
    {
      printf("%s\n",backhaul_wwan_qos_flow_ind_list[i]);
    }
    if (fgets(scan_string, sizeof(scan_string), stdin) == NULL)
       break;
    fflush(stdout);
    qosFlowOpt = atoi(scan_string);
    QoSFlowIndConfig( qosFlowOpt );
    break;
  }

  /* Configure DDS Recommendation */
  case 43:
  {
    int enable_dds_recommendation, dds_recomm_type;
    qcmap_msgr_dds_recommendation_enum_type_v01 dds_recommendation_type;

    QCMAP_PRINTF_TAKE_INPUT("\nEnable DDS Recommendation (0: Disable, 1: Enable): ");
    READ_AND_VALIDATE_INT_VALUE(enable_dds_recommendation, 0, 1);

    QcMapClient->ConfigureDDSRecommendation(enable_dds_recommendation, &qmi_err_num);
    if(qmi_err_num == QMI_ERR_NONE_V01)
    {
      printf("DDS Recommendation is configured\n");
    }
    else if(qmi_err_num == QMI_ERR_NO_EFFECT_V01)
    {
      printf("DDS Recommendation is already configured\n");
    }
    else
    {
      printf("Error occured: 0x%x\n", qmi_err_num);
    }
    break;
  }

  /* Switch DDS */
  case 44:
  {
    int switch_dds;
    qcmap_msgr_subscription_enum_v01 subs_id;

    QcMapClient->GetCurrentDDS(&subs_id, &qmi_err_num);
    if(qmi_err_num == QMI_ERR_NONE_V01)
    {
      printf("Current DDS Subscription ID: %d\n", subs_id);
    }
     else
    {
      printf("Error occured: 0x%x\n", qmi_err_num);
    }

    QCMAP_PRINTF_TAKE_INPUT("\nSwitch DDS based on Recommended DDS? (1: Primary Sub, 2: Secondary Sub): ");
    READ_AND_VALIDATE_INT_VALUE(switch_dds, 1, 2);
    if(switch_dds == subs_id)
    {
      QCMAP_CLI_LOG("Same sub given as current DDS, ignoring DDS Recommendation\n");
    }
    else
    {
      QcMapClient->SwitchDDS(switch_dds, &qmi_err_num);
      if(qmi_err_num == QMI_ERR_NONE_V01)
      {
        printf("DDS Switch req sent\n");
      }
      else
      {
        printf("Error occured: 0x%x\n", qmi_err_num);
      }
    }
    break;
  }

  /* Get Current DDS */
  case 45:
  {
    qcmap_msgr_subscription_enum_v01 subs_id;
    QcMapClient->GetCurrentDDS(&subs_id, &qmi_err_num);
    if(qmi_err_num == QMI_ERR_NONE_V01)
    {
      printf("Current DDS Subscription ID: %d\n", subs_id);
    }
     else
    {
      printf("Error occured: 0x%x\n", qmi_err_num);
    }
    break;
  }

  default :
  {
    printf("Invalid response %d\n", backhaulWWANOpt);
  }
  break;
  }
}

bool ReadMACsecConfig(qcmap_msgr_macsec_nic_config_v01 &macsec_config)
{
  int mac_mode=0;
  char scan_string[QCMAP_MSGR_MAX_FILE_PATH_LEN];
  int iface_name = -1;

  memset(scan_string, 0, QCMAP_MSGR_MAX_FILE_PATH_LEN);
  QCMAP_PRINTF_TAKE_INPUT("   On Which NIC interface you want to configure macsec (eg: eth0 (NIC-1), eth1 (NIC-2) ):");
  fgets(scan_string, sizeof(scan_string), stdin);
  strlcpy(macsec_config.eth_nic_iface_name, scan_string,strlen(scan_string));

  ASK_USER_FOR_INPUT_INT_PARAM("Please input MACsec config action (0-Disable/1-Enable/2-Restart):", macsec_config.state);

  if(macsec_config.state)
  {
    ASK_USER_FOR_INPUT_INT_PARAM("Enter the MACcse config  mode ( 1-Suppliant, 2-authenticator):",mac_mode);
    if ((mac_mode == QCMAP_MSGR_MACSEC_MODE_SUPPLICANT_V01) ||
        (mac_mode == QCMAP_MSGR_MACSEC_MODE_AUTHENTICATOR_V01))
    {
      macsec_config.macsec_mode = mac_mode;
    }
    else
    {
      printf("Invalid Macsec mode %d\n",mac_mode);
      return false;
    }
  }
  return true;
}


void tetheringConfig( int tetheOpt )
{
  qmi_error_type_v01 qmi_err_num;
  /* Initialize QMI Error Number. */
  qmi_err_num = QMI_ERR_NONE_V01;
  char scan_string[QCMAP_MSGR_MAX_FILE_PATH_LEN] = {0};
  int ix = 0;
  qcmap_eth_config eth_config;

  int new_config = 0, new_nic_config = 0,new_macsec_config = 0,mac_mode = 0,mac_nic_update = 0;
  boolean ret = false;
  boolean macsec_ret = false;

  /* Tethering Config Options */
  switch(tetheOpt)
  {
  /* Set Cradle Mode */
  case 1:
  {
  /* Set Cradle Mode */
  /* Only  QCMAP_MSGR_CRADLE_WAN_ROUTER_V01 is supported*/
  printf("   Possible Cradle Modes\n");
  printf("   Disabled = 0\n");
  printf("   LAN Bridge = 1\n");
  printf("   LAN Router = 2\n");
  printf("   WAN Bridge = 3\n");
  printf("   WAN Router = 4\n");
  printf("   Please input cradle mode (0-4):");
  fgets(scan_string, sizeof(scan_string), stdin);
  if (!VALID_NUMERIC_INPUT(scan_string))
  {
    printf("\nInvalid response\n");
    break;
  }

  if (atoi(scan_string) >= 0 && atoi(scan_string) <= 4)
  {
   if (QcMapClient->SetCradleMode(atoi(scan_string), &qmi_err_num))
   {
      printf("\nMobile AP Cradle Mode has been set\n");
   }
   else
   {
     switch(qmi_err_num)
     {
       case(QMI_ERR_INCOMPATIBLE_STATE_V01):
       {
         printf("\nCrade mode could not be set...\n"
                "Please delete all VLANs first.\n");
         break;
       }
       default:
       {
         printf("\nFailed to Set Cradle Mode: Error 0x%x.\n ", qmi_err_num);
         break;
       }
     }
   }

  }
  else
   printf("\n   %s is invalid, please select a valid option\n", scan_string);

  break;
  }

  /* Get Cradle Mode */
  case 2:
  {
  /* Get Cradle Mode/Status */
  qcmap_msgr_cradle_mode_v01 mode;
  if (QcMapClient->GetCradleMode(&mode, &qmi_err_num))
  {
   /* Only  QCMAP_MSGR_CRADLE_WAN_ROUTER_V01 is supported*/
   switch (mode)
   {
      case QCMAP_MSGR_CRADLE_DISABLED_V01:
        printf("\nMobile AP Cradle Mode is Disabled");
        break;
      case QCMAP_MSGR_CRADLE_LAN_BRIDGE_V01:
        printf("\nMobile AP Cradle Mode is LAN BRIDGE");
        break;
      case QCMAP_MSGR_CRADLE_LAN_ROUTER_V01:
        printf("\nMobile AP Cradle Mode is LAN ROUTER");
        break;
      case QCMAP_MSGR_CRADLE_WAN_BRIDGE_V01:
        printf("\nMobile AP Cradle Mode is WAN BRIDGE");
        break;
      case QCMAP_MSGR_CRADLE_WAN_ROUTER_V01:
        printf("\nMobile AP Cradle Mode is WAN ROUTER");
        break;
      default:
        printf("\nIncorrect state returned: 0x%x", mode);
        break;
   }
  }
  else
   printf("  Failed to Get Cradle Mode .Error 0x%x.\n ", qmi_err_num);
  break;
  }

  /* Set Ethernet mode. */
  case 3:
  {
    ZERO_INIT_ARG(eth_config);
    /* If configuring ETH NIC for the first time
     * or User doesn't want to use the existing conf
     */
    int wan_nic_count = 0;
    ASK_USER_FOR_INPUT_INT_PARAM("Do you want add new ETH NIC config (1-Yes / 0-No ): ", new_nic_config);
    if (new_nic_config != 0 && new_nic_config != 1)
    {
      printf("Invalid input:%d. Please choose correct option", new_nic_config);
      break;
    }

    if (new_nic_config)
    {
      printf("\nHow many ETH NIC's you want to configure (Max NIC supported-%d):", QCMAP_MAX_ETH_NIC_SUPPORT);
      fflush(stdout);
      READ_AND_VALIDATE_INT_VALUE(eth_config.no_of_nics, 1, QCMAP_MAX_ETH_NIC_SUPPORT);

      for (int i = 0; i < eth_config.no_of_nics; i++)
      {
        printf("\nEnter NIC-%d interface name (example eth0,eth1 etc, please be careful of case):", i+1);
        fflush(stdout);

        memset(scan_string, 0, QCMAP_MSGR_MAX_FILE_PATH_LEN);
        if (strlen(scan_string) == 0 && scan_string[0] == '\0')
        {
          fgets(scan_string, sizeof(scan_string), stdin);
        }
        else
        {
          printf("\nerror scan string is not empty");
          break;
        }
        if (strlen(scan_string)-1 >= QCMAP_MAX_IFACE_NAME_SIZE_V01)
        {
          printf("ERROR: Overflow..Supports only 16 char interface size\n");
          fflush(stdout);
          return;
        }
        int ix = 0;
        for (ix=0;ix < strlen(scan_string)-1;ix++)
        {
          if (VALID_IF_NAME_CHAR(scan_string[ix]))
        {
            eth_config.eth_nic_config[i].eth_iface_name[ix] = scan_string[ix];
        }
          else
        {
            printf("\nERROR: special char found in interface name\n");
            fflush(stdout);
            return;
          }
        }
        eth_config.eth_nic_config[i].eth_iface_name[ix] = '\0';
        strlcpy(eth_config.macsec_nic_config[i].eth_nic_iface_name, scan_string,strlen(scan_string));

        int tmp = -1;
        printf("\nEnter NIC-%d connectivity type (0-LAN/1-WAN):",i+1);
        READ_AND_VALIDATE_INT_VALUE(tmp, 0, 1);
        eth_config.eth_nic_config[i].eth_nic_type = tmp;
        if (tmp == 1)
        {
          wan_nic_count++;
        }
      }
      if (wan_nic_count > 1)
      {
        printf("\nmultiple NICs(%d) configured for WAN connectivity -- Not supported", wan_nic_count);
        fflush(stdout);
        return;
      }
      eth_config.is_eth_nics_config_valid = true;
    }

    /* Get ETH NIC configuration */
    if (!new_nic_config)
    {
      if (QcMapClient->GetEthernetNicConfig(&eth_config, &qmi_err_num))
      {
        DisplayEthernetNicConfig(eth_config);
        eth_config.is_eth_nics_config_valid = false;
        eth_config.is_macsec_nic_config_valid = false;
      }
      printf("\n");
    }

    memset(scan_string, 0, QCMAP_MSGR_MAX_FILE_PATH_LEN);
    /* Setting MACSec configuration from user*/
    ASK_USER_FOR_INPUT_INT_PARAM("Do you want set new MACsec config (1-Yes / 0-No): ", new_macsec_config);
    if (new_macsec_config == 1)
    {
      memset(scan_string, 0, QCMAP_MSGR_MAX_FILE_PATH_LEN);
      ASK_USER_FOR_INPUT_INT_PARAM("How many macsec NICs you want to configure (eg: 1,2 max:2 ): ",eth_config.no_of_macsec_nics);
      for (int in = 0; in < eth_config.no_of_macsec_nics; in++)
      {
        printf("   Please input MACsec config:%d\n", in+1);
        if (!ReadMACsecConfig(eth_config.macsec_nic_config[in]))
        {
          printf("Macsec Configuration is Invalid  ");
          break;
        }
        eth_config.is_macsec_nic_config_valid = true;
      }
    }
    printf("   Possible Ethernet Modes\n");
    printf("   LAN Router = 0\n");
    printf("   WAN Router = 1\n");
    printf("   WAN_LAN Router = 2\n");
    QCMAP_PRINTF_TAKE_INPUT("   Please input Ethernet mode (0-2):");
    fgets(scan_string, sizeof(scan_string), stdin);
    if (!VALID_NUMERIC_INPUT(scan_string))
    {
      printf("\nInvalid response\n");
      break;
    }
    int tmp = 0;
    bool read_user_config = false;
    if (atoi(scan_string) >= 0 && atoi(scan_string) < 3)
    {
      eth_config.mode = atoi(scan_string);
      if (eth_config.mode == QCMAP_MSGR_ETHERNET_LAN_ROUTER_V01 ||
          eth_config.mode == QCMAP_MSGR_ETHERNET_WAN_ROUTER_V01)
      {
        read_user_config = true;
      }
      if (eth_config.mode == QCMAP_MSGR_ETHERNET_WAN_LAN_ROUTER_V01)
      {
        ASK_USER_FOR_INPUT_INT_PARAM("Do you want to enable WAN_LAN mode with a new config (1-Yes / 0-No ): ",
                                       new_config);

        if (new_config != 0 && new_config != 1)
        {
          printf("Invalid input:%d. Please choose correct option", new_config);
          break;
        }
        if (new_config)
        {
          printf("\n   Is WAN_LAN mode enabling on (1-QCA8337 / 2-NTN3 ): ");
          READ_AND_VALIDATE_INT_VALUE(tmp, 1, 2);
          if (tmp == 2)
          {
            read_user_config = true;
          }
          else
          {
            tmp = 0;
            eth_config.is_eth_ports_config_valid = true;
            printf("Please input 3 LAN port numbers(1-4)\n");
            for (int i = 0; i < MAX_ETH_LAN_PORTS;)
            {
              memset(scan_string, 0, QCMAP_MSGR_MAX_FILE_PATH_LEN);
              fgets(scan_string, sizeof(scan_string), stdin);
              tmp = atoi(scan_string);
              if (tmp > 0 && tmp < 5)
              {
                eth_config.eth_ports.eth_lan_ports[i] = tmp;
                i++;
              }
              else
              {
                printf("\nInvalid LAN port. Please input valid LAN port(1-4) \n");
                continue;
              }
            }
            printf("Please input VLAN id for LAN ports (1-%d) \n", MAX_VLAN_ID);
            while(1)
            {
              fgets(scan_string, sizeof(scan_string), stdin);
              tmp = atoi(scan_string);
              if (tmp <= 0 || tmp > MAX_VLAN_ID)
              {
                printf("\nInvalid VLAN id. Please input valid vlan id (1-%d)\n",MAX_VLAN_ID);
                continue;
              }
              eth_config.eth_ports.eth_lan_vlan_id = tmp;
              break;
            }
            printf("Please input WAN port (1-4) \n");
            while(1)
            {
              fgets(scan_string, sizeof(scan_string), stdin);
              tmp = atoi(scan_string);
              if (tmp <= 0 || tmp > 4)
              {
                printf("\nInvalid WAN port. Please input valid WAN port(1-4)\n");
                continue;
              }
              eth_config.eth_ports.eth_wan_port = tmp;
              break;
            }
            printf("Please input VLAN id for WAN port (1-%d) \n", MAX_VLAN_ID);
            while(1)
            {
              fgets(scan_string, sizeof(scan_string), stdin);
              tmp = atoi(scan_string);
              if (tmp <= 0 || tmp > MAX_VLAN_ID)
              {
                printf("\nInvalid VLAN id. Please input valid vlan id(1-%d)\n",MAX_VLAN_ID);
                continue;
              }
              eth_config.eth_ports.eth_wan_vlan_id = tmp;
              break;
            }
            if (eth_config.eth_ports.eth_wan_vlan_id == eth_config.eth_ports.eth_lan_vlan_id)
            {
              printf("\n Used same VLAN id (%d) for lan and wan ports..try again!!!",
                      eth_config.eth_ports.eth_lan_vlan_id);
              break;
            }
            else if (eth_config.eth_ports.eth_wan_port == eth_config.eth_ports.eth_lan_ports[0] ||
                     eth_config.eth_ports.eth_wan_port == eth_config.eth_ports.eth_lan_ports[1] ||
                     eth_config.eth_ports.eth_wan_port == eth_config.eth_ports.eth_lan_ports[2])
            {
              printf("Used same port for lan and wan ports..try again!!!");
              break;
            }
          }
        }
      }

      memset(scan_string, 0, QCMAP_MSGR_MAX_FILE_PATH_LEN);
      if (!new_nic_config && read_user_config)
      {
        ASK_USER_FOR_INPUT_INT_PARAM("Do want to change NIC connectivity type (1-Yes/0-No):", tmp);
        if (tmp != 0 && tmp != 1)
        {
          printf("Invalid Input:%d", tmp);
          break;
        }
        if (tmp)
        {
          printf("\n   Please configure all NIC connectivity types");

          for (int i = 0; i < eth_config.no_of_nics; i++)
          {
            printf("\n   Input NIC (%d-%s) connectivity type (0-LAN/1-WAN):",
                    i+1, eth_config.eth_nic_config[i].eth_iface_name);
            READ_AND_VALIDATE_INT_VALUE(tmp, 0, 1);
            eth_config.eth_nic_config[i].eth_nic_type = tmp;
          }
          eth_config.is_eth_nics_config_valid = true;
        }
        wan_nic_count = 0;

        for (int i = 0; i < eth_config.no_of_nics; i++)
        {
          if (eth_config.eth_nic_config[i].eth_nic_type == QCMAP_MSGR_ETHERNET_WAN_TYPE_V01)
          {
            printf("\n   WAN connectivity NIC - %s", eth_config.eth_nic_config[i].eth_iface_name);
            wan_nic_count++;
          }
        }
        if (eth_config.mode == QCMAP_MSGR_ETHERNET_WAN_ROUTER_V01 ||
            eth_config.mode == QCMAP_MSGR_ETHERNET_WAN_LAN_ROUTER_V01)

        {
          if (wan_nic_count > 1)
          {
            printf("\nmultiple NICs(%d) configured for WAN connectivity -- Not supported", wan_nic_count);
            return ;
          }
          if (wan_nic_count == 0)
          {
            printf("\nNo NIC configured for WAN connectivity in WAN_LAN mode -- Invalid Config");
            return ;
          }
        }
        if (eth_config.mode == QCMAP_MSGR_ETHERNET_LAN_ROUTER_V01)

        {
          if (wan_nic_count > 0)
          {
            printf("\nNIC configured as WAN connectivity type in LAN+LAN mode -- Invalid config");
            return;
          }
        }
      }

      if(eth_config.no_of_nics <= 0 || eth_config.no_of_nics > QCMAP_MAX_ETH_NIC_SUPPORT)
      {
        LOG_MSG_ERROR("Number of NICs not in range", 0, 0, 0);
        return;
      }

      ret = QcMapClient->SetEthernetNicConfig(eth_config, &qmi_err_num);
      if (ret)
      {
         printf("\nETH NIC('s) Config/Mobile AP Ethernet has been set\n");
      } else {
      switch(qmi_err_num)
      {
        case(QMI_ERR_INCOMPATIBLE_STATE_V01):
        {
          printf("\nEth backhaul can not be enabled...\n"
                 "Please delete all VLANs first.\n");
          break;
        }
        case(QMI_ERR_INVALID_HANDLE_V01):
        {
          printf("\nMobileAP is not enabled");
          break;
        }
        default:
        {
          printf("\nFailed to Set Ethernet Mode: Error 0x%x.\n ",
                 qmi_err_num);
          break;
        }
      }
    }
  }
  else
   printf("\n   %s is invalid, please select a valid option\n",
          scan_string);

  break;
  }


  /* Get Ethernet mode. */
  case 4:
  {
    ZERO_INIT_ARG(eth_config);
    if (QcMapClient->GetEthernetNicConfig(&eth_config, &qmi_err_num))
    {
        printf("   ETH NIC valid:%d, count:%d\n", eth_config.is_eth_nics_config_valid, eth_config.no_of_nics);
        fflush(stdout);
      if (eth_config.is_eth_nics_config_valid)
      {
        DisplayEthernetNicConfig(eth_config);
      }
      switch (eth_config.mode)
      {
        case QCMAP_MSGR_ETHERNET_LAN_ROUTER_V01:
          printf("\nMobile AP Ethernet Mode is LAN ROUTER");
          break;
        case QCMAP_MSGR_ETHERNET_WAN_ROUTER_V01:
          printf("\nMobile AP Ethernet Mode is WAN ROUTER");
          break;
        case QCMAP_MSGR_ETHERNET_WAN_LAN_ROUTER_V01:
          printf("\nMobile AP Ethernet Mode is WAN_LAN ROUTER");
          if (eth_config.is_eth_ports_config_valid)
          {
            printf("\nLAN Ports");
            printf("\nPort\tVlan ID");
            printf("\n%d\t%d", eth_config.eth_ports.eth_lan_ports[0], eth_config.eth_ports.eth_lan_vlan_id);
            printf("\n%d\t%d", eth_config.eth_ports.eth_lan_ports[1], eth_config.eth_ports.eth_lan_vlan_id);
            printf("\n%d\t%d", eth_config.eth_ports.eth_lan_ports[2], eth_config.eth_ports.eth_lan_vlan_id);
            printf("\nWAN Ports\nPort\tVlan ID");
            printf("\n%d\t%d\n", eth_config.eth_ports.eth_wan_port, eth_config.eth_ports.eth_wan_vlan_id);
          }
          break;
        default:
          printf("\nIncorrect state returned: 0x%x", eth_config.mode);
        break;
      }
    }
    else
      printf("  Failed to Get Ethernet Mode .Error 0x%x.\n ",
                qmi_err_num);
      break;
  }

  /* Get BT Tethering Status. */
  case 5:

    qcmap_msgr_bt_tethering_status_enum_v01 bt_teth_status;
    qcmap_bt_tethering_mode_enum_v01 bt_teth_mode;
    if (QcMapClient->GetBTTetheringStatus(&bt_teth_status, &qmi_err_num, &bt_teth_mode))
    {
      if (bt_teth_status == QCMAP_MSGR_BT_TETHERING_MODE_UP_V01)
      {
        printf("\n BT Tethering is UP in");
        printf("Mode: %s\n", ((bt_teth_mode == QCMAP_MSGR_BT_MODE_WAN_V01) ? "WAN" : "LAN"));
      }
      else
        printf("BT Tethering is DOWN\n");
    }
    else
      printf("  Failed to Get BT Tethering Status .Error 0x%x.\n ",
          qmi_err_num);
    break;

  /*Set Dun Dongle Mode*/
  case 6:
  {
    boolean dun_dongle_mode_state = false;
    QCMAP_PRINTF_TAKE_INPUT("   Please input Dundonglemode Flag (1-Enable/0-Disable):");
    fgets(scan_string, sizeof(scan_string), stdin);
    if (!VALID_NUMERIC_INPUT(scan_string))
    {
      printf("\nInvalid response\n");
      break;
    }
    if (atoi(scan_string) == 0 || atoi(scan_string) == 1)
    {
      dun_dongle_mode_state = (atoi(scan_string)) ? true : false;
      if (QcMapClient->SetDunDongleMode(dun_dongle_mode_state, &qmi_err_num))
      {
        printf("\nDUN Dongle Mode config set succeeds.");
      }
      else
      {
        printf("\nDUN Dongle Mode config set fails, Error: 0x%x",
               qmi_err_num);
      }
    }
    else
    {
      printf("\n   %s is invalid, please select a valid option\n",
             scan_string);
    }
    break;
  }

  /*get Dun Dongle Mode*/
  case 7:
  {
    boolean dun_dongle_mode_status = false;
    if (QcMapClient->GetDunDongleMode(&dun_dongle_mode_status, &qmi_err_num))
    {
      printf("\nDUN Dongle Mode: %s.",
                (dun_dongle_mode_status)?"Enabled":"Disabled");
    }
    else
    {
      printf("\nDUN Dongle Mode config get fails, Error: 0x%x",
             qmi_err_num);
    }
    break;
  }

  default :
    printf("Invalid response %d\n", tetheOpt);
    break;
  }
}

void mediaServiceConfig(int medServOpt)
{

  qmi_error_type_v01 qmi_err_num;
  /* Initialize QMI Error Number. */
  qmi_err_num = QMI_ERR_NONE_V01;
  in_addr addr;
  char scan_string[QCMAP_MSGR_MAX_FILE_PATH_LEN] = {0};
  memset(scan_string, 0, QCMAP_MSGR_MAX_FILE_PATH_LEN);
  int input_var = 0 ;

  /* MediaService Config Options */
  switch(medServOpt)
  {
    /* Enable/Disable UPNP */
    case 1:
    {
      QCMAP_PRINTF_TAKE_INPUT("   Please input UPnP State (1-Enable/0-Disable) : ");
      READ_AND_VALIDATE_INT_VALUE(input_var, 0, 1);
      if (input_var == 1)
      {
      if (QcMapClient->EnableUPNP(&qmi_err_num))
        printf("\nUPNP Enable succeeds.");
      else
        printf("\nUPNP Enable fails, Error: 0x%x", qmi_err_num);
      }
      else
      {
        if (QcMapClient->DisableUPNP(&qmi_err_num))
          printf("\nUPNP Disabled.");
        else
          printf("\nUPNP Disable request fails, Error: 0x%x", qmi_err_num);
      }
    break;
    }

    /* Get UpNp status */
    case 2:
    {
      qcmap_msgr_upnp_mode_enum_v01 upnp_state;

      if(QcMapClient->GetUPNPStatus( &upnp_state, &qmi_err_num))
      {
        if (upnp_state == QCMAP_MSGR_UPNP_MODE_UP_V01)
        {
          printf("\nUPnP is enabled");
        }
        else
        {
          printf("\nUPnP is disabled");
        }
      }
      else
      {
        printf("\nGetUPNPStatus returns Error: 0x%x", qmi_err_num);
      }

    break;
    }

    /* Set UPnP notify interval */
    case 3:
    {
      int upnp_notify_int = 0;

      QCMAP_PRINTF_TAKE_INPUT("\nPlease input UPnP notify interval in seconds (%d-%d):",
      MIN_NOTIFY_INTERVAL, MAX_NOTIFY_INTERVAL);
      READ_AND_VALIDATE_INT_VALUE(upnp_notify_int, MIN_NOTIFY_INTERVAL, MAX_NOTIFY_INTERVAL);
      printf("\nUPnP notify interval set!\n");

      qcmap_msgr_upnp_mode_enum_v01 upnp_state;

      if(QcMapClient->SetUPNPNotifyInterval(upnp_notify_int, &qmi_err_num))
      {
        if (QcMapClient->GetUPNPStatus( &upnp_state, &qmi_err_num))
        {
          if (upnp_state == QCMAP_MSGR_UPNP_MODE_UP_V01)
          {
            printf("\nThis change will not take effect until restart.\n");
            QCMAP_PRINTF_TAKE_INPUT("   Do you want to restart now? (1-yes/0-no) : ");

            memset(scan_string, 0, QCMAP_MSGR_MAX_FILE_PATH_LEN);
            if (strlen(scan_string) == 0 && scan_string[0] == '\0')
            {
              fgets(scan_string, sizeof(scan_string), stdin);
            }
            else
            {
              printf("\nerror scan string is not empty");
              break;
            }
            if (atoi(scan_string) == 1)
            {
              if (QcMapClient->DisableUPNP(&qmi_err_num))
              {
                printf("\nUPNP has been stopped.");
                if (QcMapClient->EnableUPNP(&qmi_err_num))
                  printf("\nUPNP Restart succeeds.");
                else
                  printf("\nUPNP Restart fails, Error: 0x%x", qmi_err_num);
              }
              else
                printf("\nUPNP Restart fails, Error: 0x%x", qmi_err_num);
            }
          }
        }
        else
          printf("\nGetUPNPStatus returns Error: 0x%x", qmi_err_num);
      }
      else if(qmi_err_num == QMI_ERR_INVALID_HANDLE_V01)
      {
        printf("\nMobileAP is not enabled\n");
        break;
      }
      else
      {
        printf("\nUPnP notify interval returns Error: 0x%x", qmi_err_num);
      }
    break;
    }
    /* Get UPnP notify interval */
    case 4:
    {
    int upnp_notify_int = 0;

    if(QcMapClient->GetUPNPNotifyInterval(&upnp_notify_int, &qmi_err_num))
    {
      printf("\nCurrent UPnP notify interval: %d\n", upnp_notify_int);
    }
    else
    {
      printf("\nUPnP notify interval returns Error: 0x%x", qmi_err_num);
    }

    break;
    }

    /* Set UPNPPinhole State */
    case 5:
    {
      boolean enable_firewall, pkts_allowed = false;
      int upnp_pinhole_allow = 0;

      QcMapClient->GetFirewall(&enable_firewall, &pkts_allowed,&qmi_err_num);
      if(enable_firewall)
      {
        QCMAP_PRINTF_TAKE_INPUT("   Please input UPNP Pinhole Allow State (1-Enable/0-Disable) : ");
        READ_AND_VALIDATE_INT_VALUE(upnp_pinhole_allow, 0, 1)
        if (QcMapClient->SetUPNPState(enable_firewall,upnp_pinhole_allow, &qmi_err_num))
        {
          printf("Set UPNP state success\n");
        }
        else if(qmi_err_num == QMI_ERR_INVALID_HANDLE_V01)
        {
          printf("\nMobileAP is not enabled\n");
          break;
        }
        else
        {
          printf("Set UPNP state Error: 0x%x", qmi_err_num);
        }
      }
      else
        printf(" Firewall is DISABLED. UPNP NOT ALLOWED " );
      break;
      }

    /* Get UPNPPinhole State */
    case 6:
    {
      boolean upnp_pinhole_flag;
      if (QcMapClient->GetUPNPState(&upnp_pinhole_flag,&qmi_err_num))
      {
        if(upnp_pinhole_flag)
          printf("UPNP Pinhole is allowed\n");
        else
          printf("UPNP Pinhole is NOT allowed \n");
      }
      else
        printf("Get UPNP Pinhole configuration failed,Error 0x%x", qmi_err_num);
    break;
    }

    /* Enable/Disable DLNA */
    case 7:
      QCMAP_PRINTF_TAKE_INPUT("   Please input DLNA State (1-Enable/0-Disable) : ");
      fgets(scan_string, sizeof(scan_string), stdin);
      if (atoi(scan_string))
      {
        if (QcMapClient->EnableDLNA(&qmi_err_num))
          printf("\nDLNA Enable succeeds.");
        else
          printf("\nDLNA Enable fails, Error: 0x%x", qmi_err_num);
      }
      else
      {
        if (QcMapClient->DisableDLNA(&qmi_err_num))
          printf("\nDLNA Disabled.");
        else
          printf("\nDLNA Disable request fails, Error: 0x%x", qmi_err_num);
      }
    break;

    /* Set DLNA notify interval */
    case 8:
    {
      int dlna_notify_int;
      qcmap_msgr_dlna_mode_enum_v01 dlna_state;

      QCMAP_PRINTF_TAKE_INPUT("   Please input DLNA notify interval in seconds (%d-%d):",
      MIN_NOTIFY_INTERVAL, MAX_NOTIFY_INTERVAL);
      fgets(scan_string, sizeof(scan_string), stdin);
      dlna_notify_int = atoi(scan_string);
      if (dlna_notify_int >= MIN_NOTIFY_INTERVAL && dlna_notify_int <= MAX_NOTIFY_INTERVAL)
      {
        if(QcMapClient->SetDLNANotifyInterval(dlna_notify_int, &qmi_err_num))
        {
          printf("\nDLNA notify interval set!\n");
          if(QcMapClient->GetDLNAStatus( &dlna_state, &qmi_err_num))
          {
            if (dlna_state == QCMAP_MSGR_DLNA_MODE_UP_V01)
            {
              printf("\nThis change will not take effect until restart.\n");
              QCMAP_PRINTF_TAKE_INPUT("   Do you want to restart now? (1-yes/0-no) : ");
              fgets(scan_string, sizeof(scan_string), stdin);
              if (atoi(scan_string) == 1)
              {
                if (QcMapClient->DisableDLNA(&qmi_err_num))
                {
                  printf("\nDLNA has been stopped.");
                  if (QcMapClient->EnableDLNA(&qmi_err_num))
                    printf("\nDLNA Restart succeeds.");
                  else
                    printf("\nDLNA Restart fails, Error: 0x%x", qmi_err_num);
                }
                else
                  printf("\nDLNA Restart fails, Error: 0x%x", qmi_err_num);
              }
            }
          }
          else
            printf("\nGetDLNAStatus returns Error: 0x%x", qmi_err_num);
        }
        else
        {
          printf("\nDLNA notify interval returns Error: 0x%x", qmi_err_num);
        }
      }
      else
        printf("      Invalid DLNA notify interval, must be in range %d-%d: %s",
        MIN_NOTIFY_INTERVAL, MAX_NOTIFY_INTERVAL, scan_string);
    break;
    }
    /* Get DLNA notify interval */
    case 9:
    {
      int dlna_notify_int = 0;

      if(QcMapClient->GetDLNANotifyInterval(&dlna_notify_int, &qmi_err_num))
      {
        printf("\nCurrent DLNA notify interval: %d\n", dlna_notify_int);
      }
      else
      {
        printf("\nDLNA notify interval returns Error: 0x%x", qmi_err_num);
      }
    break;
    }

    /* get DLNA status */
    case 10:
    {
      qcmap_msgr_dlna_mode_enum_v01 dlna_state;
      if(QcMapClient->GetDLNAStatus( &dlna_state, &qmi_err_num))
      {
        if (dlna_state == QCMAP_MSGR_DLNA_MODE_UP_V01)
        {
          printf("\nDLNA is enabled");
        }
        else
        {
          printf("\nDLNA is disabled");
        }
      }
      else
      {
      printf("\nGetDLNAStatus returns Error: 0x%x", qmi_err_num);
      }
    break;
    }

    /* set DLNA media directory */
    case 11:
    {
      char media_dir_get[QCMAP_MSGR_MAX_DLNA_DIR_LEN_V01] = "";
      char media_dir_set[QCMAP_MSGR_MAX_DLNA_DIR_LEN_V01] = "";
      char *ptr;

      if(QcMapClient->GetDLNAMediaDir( media_dir_get, &qmi_err_num))
      {
        printf("\nCurrent DLNA Media Dir('s):");
        printf("\n%s\n", media_dir_get);
        QCMAP_PRINTF_TAKE_INPUT("   Do you wish to keep these Directories?(1-YES/0-NO) : ");

        fgets(scan_string, sizeof(scan_string), stdin);
        int enable = (atoi(scan_string)) ? true : false;

        if (enable)
        {
          strlcpy(media_dir_set, media_dir_get, sizeof(media_dir_set));

          //replace all newlines with ','
          ptr = strchr(media_dir_set, '\n');
          while (ptr != NULL)
          {
            media_dir_set[ptr-media_dir_set] = ',';
            ptr = strchr(ptr+1, '\n');
          }
          strlcat(media_dir_set, ",", sizeof(media_dir_set));
        }
      }
      QCMAP_PRINTF_TAKE_INPUT("   Please input a valid Media Directory (\",\" to seperate multiple):");
      if (fgets(scan_string, sizeof(scan_string), stdin) != NULL)
      {
        strlcat(media_dir_set, scan_string, sizeof(media_dir_set));
        if(QcMapClient->SetDLNAMediaDir( media_dir_set, &qmi_err_num))
        {
          printf("\nDLNA Media Dir added!\n");
        }
        else
        {
          if (qmi_err_num != QMI_ERR_NO_EFFECT_V01)
          {
          printf("\nSetDLNAMediaDir returns Error: 0x%x", qmi_err_num);
          }
          else
          {
          printf("\nSetDLNAMediaDir succeeds but restart failed");
          }
        }
      }
      else
        printf("      Invalid Media Directory: %s", scan_string);
    break;
    }

    /* Get DLNA media directory */
    case 12:
    {
      char media_dir[QCMAP_MSGR_MAX_DLNA_DIR_LEN_V01] = "";
      if(QcMapClient->GetDLNAMediaDir( media_dir, &qmi_err_num))
      {
        printf("\nCurrent DLNA Media Dir('s):");
        printf("\n%s\n", media_dir);
      }
      else
      {
        printf("\nGetDLNAMediaDir returns Error: 0x%x", qmi_err_num);
      }
    break;
    }

    /* Set DLNA Whitelisting. */
    case 13:
    {
      boolean dlna_whitelist_ip_flag = false;
      QCMAP_PRINTF_TAKE_INPUT("   Please Set DLNA Whitelisting State (1-Enable/0-Disable) : ");
      fgets(scan_string, sizeof(scan_string), stdin);
      dlna_whitelist_ip_flag = atoi(scan_string);
      if (QcMapClient->SetDLNAWhitelisting(dlna_whitelist_ip_flag,&qmi_err_num))
      {
        printf("Set DLNA Whitelisting status successful\n");
      }
      else
      {
        printf("Set DLNA Whitelisting status Error: 0x%x", qmi_err_num);
      }
    break;
    }

    /* Get DLNA Whitelisting */
    case 14:
    {
      boolean dlna_whitelist_ip_flag = false;
      if (QcMapClient->GetDLNAWhitelisting(&dlna_whitelist_ip_flag, &qmi_err_num))
      {
        if(dlna_whitelist_ip_flag == 1)
          printf("DLNA Whitelisting is Enabled\n");
        else
          printf("DLNA Whitelisting is Disabled \n");
      }
      else
        printf("Get DLNA Whitelisting configuration failed,Error 0x%x", qmi_err_num);
    break;
    }

    /* Add DLNAWhitelistingIP */
    case 15:
    {
      uint32 dlna_whitelist_ip;
      memset(scan_string, 0, QCMAP_MSGR_MAX_FILE_PATH_LEN);
      while (TRUE)
      {
        QCMAP_PRINTF_TAKE_INPUT("   Please input DLNA whitelist IP to add(xxx.xxx.xxx.xxx) : ");
        if (fgets(scan_string, sizeof(scan_string), stdin) != NULL)
        {
          scan_string[strlen(scan_string)-1]='\0';
          if ( !(inet_aton(scan_string, &addr) <=0 ))
            break;
        }
        printf("   Invalid IPv4 address %d\n", scan_string);
      }
      dlna_whitelist_ip = ntohl(addr.s_addr);

      if (QcMapClient->AddDLNAWhitelistIP(dlna_whitelist_ip, &qmi_err_num))
      {
        printf("\nDLNA Whitelisting IP added successfully");
      }
      else if ( qmi_err_num == QMI_ERR_NO_EFFECT_V01 )
      {
        printf(" DLNA Whitelisting IP is already present. \n");
      }
      else
        printf("\nDLNA Whitelisting IP add fails. Error: 0x%x", qmi_err_num);
    break;
    }

    /* Delete DLNAWhitelistingIP */
    case 16:
    {
      uint32 dlna_whitelist_ip=0;
      memset(scan_string, 0, QCMAP_MSGR_MAX_FILE_PATH_LEN);
      memset(&dlna_whitelist_ip, 0, sizeof(uint32));

      while (TRUE)
      {
        QCMAP_PRINTF_TAKE_INPUT("   Please input DLNA whitelist IP to delete(xxx.xxx.xxx.xxx) : ");
        if (fgets(scan_string, sizeof(scan_string), stdin) != NULL)
        {
          scan_string[strlen(scan_string)-1]='\0';
          if ( !(inet_aton(scan_string, &addr) <=0 ))
          break;
        }
        printf("   Invalid IPv4 address %d\n", scan_string);
      }
      dlna_whitelist_ip = ntohl(addr.s_addr);

      if (QcMapClient->DeleteDLNAWhitelistIP(dlna_whitelist_ip, &qmi_err_num))
      {
        printf("\nDLNA Whitelisting IP deleted successfully");
      }
      else
        printf("\nDLNA Whitelisting IP add fails. Error: 0x%x", qmi_err_num);
    break;
    }

    /* Enable / Disable M-DNS */
    case 17:
    {
      QCMAP_PRINTF_TAKE_INPUT("   Please input M-DNS State (1-Enable/0-Disable) : ");
      int input_var = 0 ;
      boolean same_mdns_state;
      READ_AND_VALIDATE_INT_VALUE(input_var, 0, 1);
      if (input_var == 1)
      {
      if (QcMapClient->EnableMDNS(&qmi_err_num))
      {
        printf("\n M-DNS Enable succeeds.");
      }
      else
      {
        if (qmi_err_num == QMI_ERR_NO_EFFECT_V01)
        {
          printf("\n M-DNS Already Enabled \n");
        }
        else
        {
          printf("\n M-DNS Enable fails, Error: 0x%x \n", qmi_err_num);
        }
      }
    }
    else
    {
      if (QcMapClient->DisableMDNS(&qmi_err_num))
      {
        printf("\n M-DNS Disable in progress.");
      }
      else
      {
        if (qmi_err_num == QMI_ERR_NO_EFFECT_V01)
        {
          printf("\n M-DNS Already Disabled \n");
        }
        else
        {
          printf("\n M-DNS Disable fails, Error: 0x%x \n", qmi_err_num);
        }
      }
    }
    break;
    }


    /* Get MDNS status. */
    case 18:
    {
      qcmap_msgr_mdns_mode_enum_v01 mdns_state;
      if(QcMapClient->GetMDNSStatus( &mdns_state, &qmi_err_num))
      {
        if (mdns_state == QCMAP_MSGR_MDNS_MODE_UP_V01)
          printf("\nMDNS is enabled");
        else
          printf("\nMDNS is disabled");
      }
      else
        printf("\nGetMDNSStatus returns Error: 0x%x", qmi_err_num);
      break;
    }

    default :
    {
      printf("Invalid response %d\n", medServOpt);
    }
    break;
  }
}

void gsbConfig(int gsbOpt)
{
  qmi_error_type_v01 qmi_err_num;
  /* Initialize QMI Error Number. */
  qmi_err_num = QMI_ERR_NONE_V01;
  char scan_string[QCMAP_MSGR_MAX_FILE_PATH_LEN];
  qcmap_msgr_gsb_config_v01 conf;
  uint8 num_of_if = 0;
  qcmap_msgr_gsb_config_v01 conf_arr[QCMAP_MSGR_MAX_IF_SUPPORTED_V01];
  char if_name[QCMAP_MAX_IFACE_NAME_SIZE_V01];
  int ix = 0;
  boolean canSet = true;
  in_addr addr;
  memset(scan_string, 0, QCMAP_MSGR_MAX_FILE_PATH_LEN);
  memset(&addr, 0, sizeof(in_addr));
  memset(&conf, 0, sizeof(conf));

  /* GSB Configuration options */
  switch(gsbOpt)
  {
  /* Set GSB Config*/
  case 1:
  {
    qcmap_msgr_gsb_config_v01 conf;
    memset(&conf, 0, sizeof(conf));

    QCMAP_PRINTF_TAKE_INPUT("Enter IF name (example wlan0 etc, please be careful of case): \n");
    fgets(scan_string, sizeof(scan_string), stdin);
    if (strlen(scan_string)-1 >= QCMAP_MAX_IFACE_NAME_SIZE_V01)
    {
      printf("ERROR: Overflow..Supports only 16 char interface size\n");
      break;
    }

    for ( ix=0;ix < strlen(scan_string)-1;ix++)
    {
      if (VALID_IF_NAME_CHAR(scan_string[ix]))
      {
        conf.if_name[ix] = scan_string[ix];
      }
      else
      {
        printf("ERROR: special char found in interface name\n");
        return;
      }
    }
    printf("you added %s interface\n", conf.if_name);
    QCMAP_PRINTF_TAKE_INPUT(" Add bandwidth requirement for IF (Max 900 Mbps)\n");
    while(TRUE)
    {
      memset(scan_string, 0, QCMAP_MSGR_MAX_FILE_PATH_LEN);
      fgets(scan_string, sizeof(scan_string), stdin);
      conf.bw_reqd_in_mb = atoi(scan_string);
      if (conf.bw_reqd_in_mb > 0  && conf.bw_reqd_in_mb <= 900 ) {
        printf("BW added  %d\n", conf.bw_reqd_in_mb);
        break;
      }
      else {
        printf("Please add valid bw requirement\n");
      }
    }

    QCMAP_PRINTF_TAKE_INPUT(" Add high watermark value(max 600)\n");
    while(TRUE)
    {
      fgets(scan_string, sizeof(scan_string), stdin);
      conf.if_high_watermark = atoi(scan_string);
      if (conf.if_high_watermark > 0 && conf.if_high_watermark <=600) {
        printf("high wm added  %d\n", conf.if_high_watermark);
        break;
      }
      else {
        printf("Please add valid high wm requirement\n");
      }
    }

    QCMAP_PRINTF_TAKE_INPUT(" Add low watermark value\n");
    while(TRUE)
    {
      memset(scan_string, 0, QCMAP_MSGR_MAX_FILE_PATH_LEN);
      fgets(scan_string, sizeof(scan_string), stdin);
      if ((conf.if_low_watermark = atoi(scan_string)) > 0) {
        printf("low wm added  %d\n", conf.if_low_watermark);
        break;
      }
      else {
        printf("Please add valid low wm requirement\n");
      }
    }

    QCMAP_PRINTF_TAKE_INPUT(" Specify IF type(1-WLAN-AP, 2-WLAN-STA,3-ETH)\n");
    while(TRUE)
    {
      memset(scan_string, 0, QCMAP_MSGR_MAX_FILE_PATH_LEN);
      fgets(scan_string, sizeof(scan_string), stdin);
      conf.if_type = (qcmap_msgr_gsb_interface_type_enum_v01)atoi(scan_string);
      if ( conf.if_type < QCMAP_MSGR_INTERFACE_TYPE_WLAN_AP_V01 ||
           conf.if_type > QCMAP_MSGR_INTERFACE_TYPE_ETHERNET_V01)
      {
        printf("Please enter a valid IF type\n");
        continue;
      }
      else if (conf.if_type == QCMAP_MSGR_INTERFACE_TYPE_WLAN_AP_V01 ||
               conf.if_type == QCMAP_MSGR_INTERFACE_TYPE_WLAN_STA_V01)
      {
        printf("WLAN IF type is dynamically supported on reference platform.No GSB config required\n");
        canSet = false;
      }
      break;
    }

    /*ap ip for QCMAP is managed by QCMAP server.*/
    while (0)
    {
      QCMAP_PRINTF_TAKE_INPUT(" Specify ap_ip\n");
      if (fgets(scan_string, sizeof(scan_string), stdin) != NULL)
      {
        scan_string[strlen(scan_string)-1]='\0';
        if ( !(inet_aton(scan_string, &addr) <= 0) )
        {
          conf.ap_ip = ntohl(addr.s_addr);
          printf("conf Ip set 0x%X, %s\n", conf.ap_ip, inet_ntoa(addr));
          break;
        }
      }
      printf("Invalid IPv4 address %s\n", scan_string);
    }

    if(canSet && QcMapClient->SetGSBConfig(&conf, &qmi_err_num))
    {
      if (qmi_err_num ==  QMI_ERR_NONE_V01) {
        printf("GSB Config Set Successfully\n");
      }
    }
    else if(qmi_err_num == QMI_ERR_INVALID_HANDLE_V01)
    {
      printf("\nMobileAP is not enabled\n");
      break;
    }
    else
    {
      printf("GSB Config set fails, Error: 0x%x\n", qmi_err_num);
    }
  }
  break;

  /* Enable/Disable GSB*/
  case 2:
  {
    memset(scan_string, 0, QCMAP_MSGR_MAX_FILE_PATH_LEN);
    QCMAP_PRINTF_TAKE_INPUT("Please input GSB State(1-Enable/0-Disable) : ");
    fgets(scan_string, sizeof(scan_string), stdin);
    if (!VALID_NUMERIC_INPUT(scan_string) || (atoi(scan_string) != 1 && atoi(scan_string) != 0))
    {
      printf("\nInvalid response. Please enter 1 or 0.\n");
      break;
    }
    if (atoi(scan_string))
    {
      if(QcMapClient->EnableGSB(&qmi_err_num))
      {
       printf("Enabled GSB\n");
      }
      else
       printf("Enable GSB fails, Error: 0x%x\n", qmi_err_num);
    }
    else
    {
      if(QcMapClient->DisableGSB(&qmi_err_num))
      {
       printf("Disabled GSB\n");
      }
      else
       printf("Disable GSB fails, Error: 0x%x\n", qmi_err_num);
    }
  }
  break;

  /* Get GSB Config */
  case 3:
  {
    qcmap_msgr_gsb_config_v01 conf_arr[QCMAP_MSGR_MAX_IF_SUPPORTED_V01];
    memset(conf_arr, 0, sizeof(qcmap_msgr_gsb_config_v01)*QCMAP_MSGR_MAX_IF_SUPPORTED_V01);
    memset(scan_string, 0, QCMAP_MSGR_MAX_FILE_PATH_LEN);
    if (QcMapClient->GetGSBConfig(conf_arr, &num_of_if, &qmi_err_num))
    {
      if (num_of_if > 0)
      {
        printf("Total IF Configured %d\n\n\n",num_of_if );
        for (int i = 0; i < num_of_if; i++)
        {
          printf("IF Name %s\n", conf_arr[i].if_name);
          switch (conf_arr[i].if_type)
          {
          case QCMAP_MSGR_INTERFACE_TYPE_WLAN_AP_V01:
            printf("WLAN device type(AP mode)\n");
            break;
          case QCMAP_MSGR_INTERFACE_TYPE_WLAN_STA_V01:
            printf("WLAN device type(STA mode)\n");
            break;
          case QCMAP_MSGR_INTERFACE_TYPE_ETHERNET_V01:
            printf("ETH device type\n");
            break;
          default:
            printf("UNKNOWN device type\n");
            break;
          }
          printf("BW_reqd :%d\n", conf_arr[i].bw_reqd_in_mb);
          printf("low_wm :%d\n", conf_arr[i].if_low_watermark);
          printf("high_wm :%d\n", conf_arr[i].if_high_watermark);
          printf("\n\n");
        }
      }
      else
      {
        printf("No IF is configured to work with GSB\n");
      }

    }
    else
    {
      printf("Get GSB Config failed, Error:0x%x\n", qmi_err_num);
    }
  }
  break;

  /* Delete GSB Config*/
  case 4:
  {
    memset(if_name, 0, QCMAP_MAX_IFACE_NAME_SIZE_V01);
    memset(scan_string, 0, QCMAP_MSGR_MAX_FILE_PATH_LEN);

    QCMAP_PRINTF_TAKE_INPUT("Enter IF name whose config need to be deleted (example wlan0 etc, please be careful of case):\n");
    fgets(scan_string, sizeof(scan_string), stdin);
    if (strlen(scan_string)-1 >= QCMAP_MAX_IFACE_NAME_SIZE_V01)
    {
      printf("ERROR: Overflow..Supports only 16 char interface size\n");
      break;
    }
    for ( ix=0;ix < strlen(scan_string)-1;ix++)
    {
      if (VALID_IF_NAME_CHAR(scan_string[ix]))
      {
        if_name[ix] = scan_string[ix];
      }
      else
      {
        printf("ERROR: Unwanted Charaters in interface name\n");
        return;
      }
    }

    printf("The IF you want to delete is %s\n", if_name);

    if((if_name != NULL) && QcMapClient->DeleteGSBConfig(if_name, &qmi_err_num))
    {
      printf("Deleted GSB Conifg\n");
    }
    else
      printf("Delete GSB Config fails, Error: 0x%x\n", qmi_err_num);
  }
  break;

  default :
  {
    printf("Invalid response %d\n", gsbOpt);
  }
  break;
  }
}

void v2xServiceConfig(int v2xOpt)
{
  char                   scan_string[QCMAP_MSGR_MAX_FILE_PATH_LEN];
  int                    userInput;
  boolean                ret_val = false;
  qcmap_v2x_request      v2x_request;
  qcmap_v2x_response     v2x_response;
  qmi_error_type_v01     qmi_err_num;
  qcmap_msgr_v2x_request_type_enum_v01  v2x_request_type = QCMAP_MSGR_V2X_INVALID_V01;

  switch(v2xOpt)
  {
    /* V2X_SPS_FLOW_REG_REQ_V01 */
    case 1:
    {
      qcmap_msgr_v2x_sps_flow_reg_req_msg_v01   req_msg;
      qcmap_msgr_v2x_sps_flow_reg_resp_msg_v01  resp_msg;

      ZERO_INIT_ARG(req_msg);
      ZERO_INIT_ARG(resp_msg);

      v2x_request_type = QCMAP_MSGR_V2X_SPS_FLOW_REG_V01;

      ASK_USER_FOR_MANDATORY_PARAM(Request Id,  uint32_t, req_msg.req_id);
      ASK_USER_FOR_MANDATORY_PARAM(Priority,    (1-8, 8=highest), req_msg.priority);
      ASK_USER_FOR_MANDATORY_PARAM(Periodicity, uint32_t(milliSecs),  req_msg.periodicity);
      ASK_USER_FOR_MANDATORY_PARAM(Msg Size,    uint32_t(bytes), req_msg.msg_size);

      ASK_USER_FOR_OPTIONAL_PARAM (Service ID,  uint32_t, req_msg.service_id);
      ASK_USER_FOR_OPTIONAL_PARAM (SPS Port,    uint16_t, req_msg.sps_port);
      ASK_USER_FOR_OPTIONAL_PARAM (Event Driven Port,  uint16_t, req_msg.evt_driven_port);
      ASK_USER_FOR_OPTIONAL_PARAM (Protocol,  (TCP=1, UDP=2, TCP_UDP=3), req_msg.protocol);
      ASK_USER_FOR_OPTIONAL_PARAM (Peak Transmission Power, int32_t, req_msg.peak_tx_power);
      ASK_USER_FOR_OPTIONAL_PARAM (MCS Index, uint8_t, req_msg.mcs_index);
      ASK_USER_FOR_OPTIONAL_PARAM (TX Pkt Retx Setting, (AUTO=0, ON=1, OFF=2),  req_msg.retx_setting);
      ASK_USER_FOR_OPTIONAL_PARAM (Transmission Pool ID, uint8_t, req_msg.tx_pool_id);

      memcpy(&(v2x_request.req_msg.v2x_sps_flow_reg_req_msg), &req_msg, sizeof(req_msg));
      ret_val = QcMapClient->ProcessCV2XRequest(v2x_request_type, v2x_request, &v2x_response, &qmi_err_num);
      if (ret_val == false)
      {
        printf ("   Command failed, qmi_err_num=%d\n", qmi_err_num);
      }
      else
      {
        /* Empty Response */
        printf ("   Command success\n");
      }
    }
    break;

    /* V2X_SPS_FLOW_DEREG_REQ */
    case 2:
    {
      qcmap_msgr_v2x_sps_flow_dereg_req_msg_v01    req_msg;
      qcmap_msgr_v2x_sps_flow_dereg_resp_msg_v01   resp_msg;

      ZERO_INIT_ARG(req_msg);
      ZERO_INIT_ARG(resp_msg);

      v2x_request_type = QCMAP_MSGR_V2X_SPS_FLOW_DEREG_V01;

      ASK_USER_FOR_MANDATORY_PARAM(Request Id,  uint32_t, req_msg.flow_id.req_id);
      ASK_USER_FOR_MANDATORY_PARAM(SPS ID,      uint8_t,  req_msg.flow_id.sps_id);

      memcpy(&(v2x_request.req_msg.v2x_sps_flow_dereg_req_msg), &req_msg, sizeof(req_msg));
      ret_val = QcMapClient->ProcessCV2XRequest(v2x_request_type, v2x_request, &v2x_response, &qmi_err_num);
      if (ret_val == false)
      {
        printf ("   Command failed, qmi_err_num=%d\n", qmi_err_num);
      }
      else
      {
        /* Empty Response */
        printf ("   Command success\n");
      }

    }
    break;

    /* V2X_SPS_FLOW_UPDATE_REQ */
    case 3:
    {
      qcmap_msgr_v2x_sps_flow_update_req_msg_v01    req_msg;
      qcmap_msgr_v2x_sps_flow_update_resp_msg_v01   resp_msg;

      ZERO_INIT_ARG(req_msg);
      ZERO_INIT_ARG(resp_msg);

      v2x_request_type = QCMAP_MSGR_V2X_SPS_FLOW_UPDATE_V01;

      ASK_USER_FOR_MANDATORY_PARAM(Request Id,  uint32_t, req_msg.flow_id.req_id);
      ASK_USER_FOR_MANDATORY_PARAM(SPS ID,      uint8_t,  req_msg.flow_id.sps_id);

      ASK_USER_FOR_OPTIONAL_PARAM(Periodicity, uint32_t(milliSecs),  req_msg.periodicity);
      ASK_USER_FOR_OPTIONAL_PARAM(Msg Size,    uint32_t(bytes), req_msg.msg_size);
      ASK_USER_FOR_OPTIONAL_PARAM(Peak Transmission Power, int32_t, req_msg.peak_tx_power);
      ASK_USER_FOR_OPTIONAL_PARAM(MCS Index,   uint8_t, req_msg.mcs_index);
      ASK_USER_FOR_OPTIONAL_PARAM(TX Pkt Retx Setting, (AUTO=0, ON=1, OFF=2),  req_msg.retx_setting);
      ASK_USER_FOR_OPTIONAL_PARAM(Transmission Pool ID, uint8_t, req_msg.tx_pool_id);

      memcpy(&(v2x_request.req_msg.v2x_sps_flow_update_req_msg), &req_msg, sizeof(req_msg));
      ret_val = QcMapClient->ProcessCV2XRequest(v2x_request_type, v2x_request, &v2x_response, &qmi_err_num);
      if (ret_val == false)
      {
        printf ("   Command failed, qmi_err_num=%d\n", qmi_err_num);
      }
      else
      {
        /* Empty Response */
        printf ("   Command success\n");
      }
    }
    break;

    /* V2X_SPS_FLOW_GET_INFO_REQ */
    case 4:
    {
      qcmap_msgr_v2x_sps_flow_get_info_req_msg_v01   req_msg;
      qcmap_msgr_v2x_sps_flow_get_info_resp_msg_v01  resp_msg;

      ZERO_INIT_ARG(req_msg);
      ZERO_INIT_ARG(resp_msg);

      v2x_request_type = QCMAP_MSGR_V2X_SPS_FLOW_GET_INFO_V01;

      ASK_USER_FOR_MANDATORY_PARAM(Request Id,  uint32_t, req_msg.flow_id.req_id);
      ASK_USER_FOR_MANDATORY_PARAM(SPS ID,      uint8_t,  req_msg.flow_id.sps_id);

      memcpy(&(v2x_request.req_msg.v2x_sps_flow_get_info_req_msg), &req_msg, sizeof(req_msg));
      ret_val = QcMapClient->ProcessCV2XRequest(v2x_request_type, v2x_request, &v2x_response, &qmi_err_num);
      if (ret_val == false)
      {
        printf ("   Command failed, qmi_err_num=%d\n", qmi_err_num);
      }
      else
      {
        printf ("   Command success\n");

        memcpy(&resp_msg, &(v2x_response.resp_msg.v2x_sps_flow_get_info_resp_msg), sizeof(resp_msg));
        /* Show Response to User */
        SHOW_OPTIONAL_RESPONSE_TO_USER(Priority,    resp_msg.priority);
        SHOW_OPTIONAL_RESPONSE_TO_USER(Periodicity, resp_msg.priority);
        SHOW_OPTIONAL_RESPONSE_TO_USER(Msg Size,    resp_msg.msg_size);
        SHOW_OPTIONAL_RESPONSE_TO_USER(Service ID,  resp_msg.service_id);
        SHOW_OPTIONAL_RESPONSE_TO_USER(SPS Port,    resp_msg.sps_port);
        SHOW_OPTIONAL_RESPONSE_TO_USER(Event Driven Port, resp_msg.evt_driven_port);
        SHOW_OPTIONAL_RESPONSE_TO_USER(Protocol (TCP=1, UDP=2, TCP_UDP=3), resp_msg.protocol);
        SHOW_OPTIONAL_RESPONSE_TO_USER(Peak Transmission Power, resp_msg.peak_tx_power);
        SHOW_OPTIONAL_RESPONSE_TO_USER(MCS Index,   resp_msg.mcs_index);
        SHOW_OPTIONAL_RESPONSE_TO_USER(TX Pkt Retx Setting (AUTO=0, ON=1, OFF=2), resp_msg.retx_setting);
        SHOW_OPTIONAL_RESPONSE_TO_USER(Transmission Pool ID, resp_msg.tx_pool_id);
      }
    }
    break;

    /* V2X_NON_SPS_FLOW_REG_REQ */
    case 5:
    {
      qcmap_msgr_v2x_non_sps_flow_reg_req_msg_v01   req_msg;
      qcmap_msgr_v2x_non_sps_flow_reg_resp_msg_v01  resp_msg;

      ZERO_INIT_ARG(req_msg);
      ZERO_INIT_ARG(resp_msg);

      v2x_request_type = QCMAP_MSGR_V2X_NON_SPS_FLOW_REG_V01;

      ASK_USER_FOR_MANDATORY_PARAM(Request Id,  uint32_t, req_msg.req_id);
      QCMAP_PRINTF_TAKE_INPUT ( "   Do you want to enter Non-SPS Flow Reg Info "
               " (1-Yes, 0-No) : ");
      if (fgets(scan_string, sizeof(scan_string), stdin) != NULL)
      {
        userInput = atoi(scan_string);
        if (userInput == 1)
        {
          /* Optional but ask as Mandatory */
          INDENT_TEXT; ASK_USER_FOR_MANDATORY_PARAM(Service ID,  uint32_t, req_msg.non_sps_flow.reg_info.service_id);
          INDENT_TEXT; ASK_USER_FOR_MANDATORY_PARAM(Port,        uint16_t, req_msg.non_sps_flow.reg_info.port);
          INDENT_TEXT; ASK_USER_FOR_MANDATORY_PARAM(Protocol,  (TCP=1, UDP=2, TCP_UDP=3), req_msg.non_sps_flow.protocol);
        }
      }
      ASK_USER_FOR_OPTIONAL_PARAM (Peak Transmission Power, int32_t, req_msg.peak_tx_power);
      ASK_USER_FOR_OPTIONAL_PARAM (MCS Index, uint8_t, req_msg.mcs_index);
      ASK_USER_FOR_OPTIONAL_PARAM (TX Pkt Retx Setting, (AUTO=0, ON=1, OFF=2),  req_msg.retx_setting);
      ASK_USER_FOR_OPTIONAL_PARAM (Transmission Pool ID, uint8_t, req_msg.tx_pool_id);

      memcpy(&(v2x_request.req_msg.v2x_non_sps_flow_reg_req_msg), &req_msg, sizeof(req_msg));
      ret_val = QcMapClient->ProcessCV2XRequest(v2x_request_type, v2x_request, &v2x_response, &qmi_err_num);
      if (ret_val == false)
      {
        printf ("   Command failed, qmi_err_num=%d\n", qmi_err_num);
      }
      else
      {
        printf ("   Command success\n");

        memcpy(&resp_msg, &(v2x_response.resp_msg.v2x_non_sps_flow_reg_resp_msg), sizeof(resp_msg));
        /* Show Response to User */
        if (resp_msg.result_valid)
        {
          SHOW_MANDATORY_RESPONSE_TO_USER(Service ID, resp_msg.result.service_id);
          SHOW_MANDATORY_RESPONSE_TO_USER(Result, resp_msg.result.result);
        }
      }
    }
    break;

    /* V2X_NON_SPS_FLOW_DEREG_REQ */
    case 6:
    {
      qcmap_msgr_v2x_non_sps_flow_dereg_req_msg_v01   req_msg;
      qcmap_msgr_v2x_non_sps_flow_dereg_resp_msg_v01  resp_msg;

      ZERO_INIT_ARG(req_msg);
      ZERO_INIT_ARG(resp_msg);

      v2x_request_type = QCMAP_MSGR_V2X_NON_SPS_FLOW_DEREG_V01;

      ASK_USER_FOR_MANDATORY_PARAM(Request Id,  uint32_t, req_msg.req_id);
      QCMAP_PRINTF_TAKE_INPUT("   Do you want to enter DeReg Non-SPS Flow Info: (1=Yes, 0=No) : ");
      if (fgets(scan_string, sizeof(scan_string), stdin) != NULL)
      {
        userInput = atoi(scan_string);
        if (userInput == 1)
        {
          QCMAP_PRINTF_TAKE_INPUT("   How many DeReg Non-SPS Flow, you want to add? :");
          if (fgets(scan_string, sizeof(scan_string), stdin) != NULL)
          {
            int non_sps_flow_len = 0;
            non_sps_flow_len = atoi(scan_string);
            if (non_sps_flow_len > 0)
            {
              for (int i=0; i < non_sps_flow_len; i++)
              {
                INDENT_TEXT; ASK_USER_FOR_MANDATORY_PARAM(Service ID, uint32_t, req_msg.non_sps_flow_info[i].service_id);
                INDENT_TEXT; ASK_USER_FOR_MANDATORY_PARAM(Port,       uint16_t, req_msg.non_sps_flow_info[i].port);
              }
              req_msg.non_sps_flow_info_valid = true;
              req_msg.non_sps_flow_info_len   = non_sps_flow_len;
            }
          }
        }
      }

      memcpy(&(v2x_request.req_msg.v2x_non_sps_flow_dereg_req_msg), &req_msg, sizeof(req_msg));
      ret_val = QcMapClient->ProcessCV2XRequest(v2x_request_type, v2x_request, &v2x_response, &qmi_err_num);
      if (ret_val == false)
      {
        printf ("   Command failed, qmi_err_num=%d\n", qmi_err_num);
      }
      else
      {
        printf ("   Command success\n");

        /* Show Response to User */
        memcpy(&resp_msg, &(v2x_response.resp_msg.v2x_non_sps_flow_dereg_resp_msg), sizeof(resp_msg));
        if (resp_msg.dereg_result_valid)
        {
          printf("   Non-SPS Flow Dereg Result Length=%d\n", resp_msg.dereg_result_len);
          for (int i=0; i<resp_msg.dereg_result_len; i++)
          {
            INDENT_TEXT; printf("      Service ID[%d]=%d", i, resp_msg.dereg_result[i].service_id);
            INDENT_TEXT; printf("          Result[%d]=%d", i, resp_msg.dereg_result[i].result);
          }
        }
      }
    }
    break;

    /* V2X_SERVICE_SUBSCRIBE_REQ */
    case 7:
    {
      qcmap_msgr_v2x_service_subscribe_req_msg_v01    req_msg;
      qcmap_msgr_v2x_service_subscribe_resp_msg_v01   resp_msg;

      ZERO_INIT_ARG(req_msg);
      ZERO_INIT_ARG(resp_msg);

      v2x_request_type = QCMAP_MSGR_V2X_SERVICE_SUBSCRIBE_V01;

      ASK_USER_FOR_MANDATORY_PARAM(Request ID, uint32_t, req_msg.service_info.req_id);
      ASK_USER_FOR_MANDATORY_PARAM(Action, (\n     SUBS_ADD=0, SUBS_REMOVE=1,
                                   SUBS_ADD_WILDCARD=2, SUBS_REMOVE_WILDCARD=3),
                                   req_msg.service_info.action);
      ASK_USER_FOR_OPTIONAL_PARAM_LIST(Service ID, uint32_t, req_msg.service_id_list);
      ASK_USER_FOR_OPTIONAL_PARAM(Port, uint16_t, req_msg.port);

      memcpy(&(v2x_request.req_msg.v2x_service_subscribe_req_msg), &req_msg, sizeof(req_msg));
      ret_val = QcMapClient->ProcessCV2XRequest(v2x_request_type, v2x_request, &v2x_response, &qmi_err_num);
      if (ret_val == false)
      {
        printf ("   Command failed, qmi_err_num=%d\n", qmi_err_num);
      }
      else
      {
        /* Empty Response */
        printf ("   Command success\n");
      }
    }
    break;

    /* V2X_GET_SERVICE_SUBSCRIPTION_INFO_REQ */
    case 8:
    {
      qcmap_msgr_v2x_service_get_subscribe_list_req_msg_v01    req_msg;
      qcmap_msgr_v2x_service_get_subscribe_list_resp_msg_v01   resp_msg;

      ZERO_INIT_ARG(req_msg);
      ZERO_INIT_ARG(resp_msg);

      v2x_request_type = QCMAP_MSGR_V2X_GET_SERVICE_SUBSCRIPTION_INFO_V01;

      memcpy(&(v2x_request.req_msg.v2x_service_get_subscribe_list_req_msg), &req_msg, sizeof(req_msg));
      ret_val = QcMapClient->ProcessCV2XRequest(v2x_request_type, v2x_request, &v2x_response, &qmi_err_num);
      if (ret_val == false)
      {
        printf ("   Command failed, qmi_err_num=%d\n", qmi_err_num);
      }
      else
      {
        printf ("   Command success\n");

        memcpy(&resp_msg, &(v2x_response.resp_msg.v2x_service_get_subscribe_list_resp_msg), sizeof(resp_msg));
        /* Show Response to User */
        SHOW_OPTIONAL_RESPONSE_TO_USER(Wildcard Enabled, resp_msg.wildcard_enabled);
        if (resp_msg.service_id_valid)
        {
          printf ("   Service ID List Len = %d\n", resp_msg.service_id_len);
          for (int i=0; i < resp_msg.service_id_len; i++)
          {
            printf("      Service_ID[%d]=%d\n", i, resp_msg.service_id[i]);
          }
        }
        SHOW_OPTIONAL_RESPONSE_TO_USER(Destination Port, resp_msg.dest_port);
      }
    }
    break;

    /* V2X_SEND_CONFIG_FILE_REQ */
    case 9:
    {
      qcmap_msgr_v2x_send_config_file_req_msg_v01      req_msg;
      qcmap_msgr_v2x_send_config_file_resp_msg_v01     resp_msg;

      ZERO_INIT_ARG(req_msg);
      ZERO_INIT_ARG(resp_msg);

      v2x_request_type = QCMAP_MSGR_V2X_SEND_CONFIG_FILE_V01;

      QCMAP_PRINTF_TAKE_INPUT("   Enter Cfg Filename (with path) : ");
      if (fgets(scan_string, sizeof(scan_string), stdin) != NULL)
      {
        strlcpy((char *)req_msg.cfg_file_path, scan_string, sizeof(req_msg.cfg_file_path));
        req_msg.cfg_file_path_len = strlen((char *) req_msg.cfg_file_path);

        ret_val = QcMapClient->ProcessCV2XRequest(v2x_request_type, v2x_request, &v2x_response, &qmi_err_num);
        if (ret_val == false)
        {
          printf ("   Command failed, qmi_err_num=%d\n", qmi_err_num);
        }
        else
        {
          /* Empty Response */
          printf ("   Command success\n");
        }
      }
    }
    break;

    /* V2X_UPDATE_SRC_L2_INFO_REQ */
    case 10:
    {
      qcmap_msgr_v2x_update_src_l2_info_req_msg_v01      req_msg;
      qcmap_msgr_v2x_update_src_l2_info_resp_msg_v01     resp_msg;

      ZERO_INIT_ARG(req_msg);
      ZERO_INIT_ARG(resp_msg);

      v2x_request_type = QCMAP_MSGR_V2X_UPDATE_SRC_L2_INFO_V01;

      /* Empty Request */
      ret_val = QcMapClient->ProcessCV2XRequest(v2x_request_type, v2x_request, &v2x_response, &qmi_err_num);
      if (ret_val == false)
      {
        printf ("   Command failed, qmi_err_num=%d\n", qmi_err_num);
      }
      else
      {
        /* Empty Response */
        printf ("   Command success\n");
      }
    }
    break;

    /* V2X_TUNNEL_MODE_INFO_REQ */
    case 11:
    {
      qcmap_msgr_v2x_tunnel_mode_info_req_msg_v01      req_msg;
      qcmap_msgr_v2x_tunnel_mode_info_resp_msg_v01     resp_msg;

      ZERO_INIT_ARG(req_msg);
      ZERO_INIT_ARG(resp_msg);

      v2x_request_type = QCMAP_MSGR_V2X_TUNNEL_MODE_INFO_V01;

      ASK_USER_FOR_OPTIONAL_PARAM_LIST(Malicious Src L2, uint32_t, req_msg.malicious_src_l2_id_list);
      QCMAP_PRINTF_TAKE_INPUT("   Do you want to enter Trusted Remote UE Src L2: (1=Yes, 0=No) : ");
      if (fgets(scan_string, sizeof(scan_string), stdin) != NULL)
      {
        userInput = atoi(scan_string);
        if (userInput == 1)
        {
          QCMAP_PRINTF_TAKE_INPUT("      How many Trusted Remote UE Src L2, you want to add? :");
          if (fgets(scan_string, sizeof(scan_string), stdin) != NULL)
          {
            int list_len = 0;
            list_len = atoi(scan_string);
            if (list_len > 0)
            {
              for (int i=0; i < list_len; i++)
              {
                INDENT_TEXT; ASK_USER_FOR_MANDATORY_PARAM(Source L2 ID, uint32_t,
                                                          req_msg.trusted_l2_info_ex[i].src_l2_id);
                INDENT_TEXT; ASK_USER_FOR_MANDATORY_PARAM(Time Uncertainty, float(milliSecs),
                                                          req_msg.trusted_l2_info_ex[i].time_uncertainty);
                INDENT_TEXT; ASK_USER_FOR_MANDATORY_PARAM(Position Confidence Level, uint16_t(0-127),
                                                          req_msg.trusted_l2_info_ex[i].time_uncertainty);
                INDENT_TEXT; ASK_USER_FOR_MANDATORY_PARAM(Propagation Delay, uint32_t,
                                                          req_msg.trusted_l2_info_ex[i].propagation_delay);
              }
              req_msg.trusted_l2_info_ex_valid = true;
              req_msg.trusted_l2_info_ex_len   = list_len;
            }
          }
        }
      }

      memcpy(&(v2x_request.req_msg.v2x_tunnel_mode_info_req_msg), &req_msg, sizeof(req_msg));
      ret_val = QcMapClient->ProcessCV2XRequest(v2x_request_type, v2x_request, &v2x_response, &qmi_err_num);
      if (ret_val == false)
      {
        printf ("   Command failed, qmi_err_num=%d\n", qmi_err_num);
      }
      else
      {
        /* Empty Response */
        printf ("   Command success\n");
      }
    }
    break;

    /* V2X_GET_CAPABILITY_INFO_REQ */
    case 12:
    {
      qcmap_msgr_v2x_get_capability_info_req_msg_v01      req_msg;
      qcmap_msgr_v2x_get_capability_info_resp_msg_v01     resp_msg;

      ZERO_INIT_ARG(req_msg);
      ZERO_INIT_ARG(resp_msg);

      v2x_request_type = QCMAP_MSGR_V2X_GET_CAPABILITY_INFO_V01;

      //Request is empty
      ret_val = QcMapClient->ProcessCV2XRequest(v2x_request_type, v2x_request, &v2x_response, &qmi_err_num);
      if (ret_val == false)
      {
        printf ("   Command failed, qmi_err_num=%d\n", qmi_err_num);
      }
      else
      {
        printf ("   Command success\n");

        memcpy(&resp_msg, &(v2x_response.resp_msg.v2x_get_capability_info_resp_msg), sizeof(resp_msg));
        /* Show Response to User */
        SHOW_OPTIONAL_RESPONSE_TO_USER(Maximum Number of SPS Flows, resp_msg.max_sps_flow_cnt);
        SHOW_OPTIONAL_RESPONSE_TO_USER(Maximum Number of Event Driven Flows, resp_msg.max_event_driven_flow_cnt);
        SHOW_OPTIONAL_RESPONSE_TO_USER(WWAN Concurrency Capability, resp_msg.is_concurrency_supported);
        SHOW_OPTIONAL_RESPONSE_TO_USER(Maximum Number of SPS Flows, resp_msg.pppp_info); //here
        if (resp_msg.pppp_info_valid)
        {
          printf("   Promixmity Service per Packet Priority Information Length=%d\n",
                  resp_msg.pppp_info_len);
          if (resp_msg.pppp_info_len > 0)
          {
            for (int i=0; i<resp_msg.pppp_info_len; i++)
            {
              printf("      Priority[%d]=%d\n", i, resp_msg.pppp_info[i].priority);
              printf("      PDB[%d]=%d\n", i, resp_msg.pppp_info[i].pdb);
            }
          }
        }
        SHOW_OPTIONAL_RESPONSE_TO_USER(Supported Minimum Transmission Power, resp_msg.min_tx_pwr);
        SHOW_OPTIONAL_RESPONSE_TO_USER(Supported Maximum Transmission Power, resp_msg.max_tx_pwr);
        SHOW_OPTIONAL_RESPONSE_LIST_TO_USER(Supported SPS Periodicity List, resp_msg.supported_periodicity_list);
        if (resp_msg.tx_pool_id_list_valid)
        {
          printf("   Transmission Pool ID List Length=%d\n", resp_msg.tx_pool_id_list_len);
          if (resp_msg.tx_pool_id_list_len > 0)
          {
            for (int i=0; i<resp_msg.tx_pool_id_list_len; i++)
            {
              printf("      Pool ID[%d]=%d\n", i, resp_msg.tx_pool_id_list[i].pool_id);
              printf("      Min freq[%d] in MHz=%d\n", i, resp_msg.tx_pool_id_list[i].min_freq);
              printf("      Max freq[%d] in MHz=%d\n", i, resp_msg.tx_pool_id_list[i].max_freq);
            }
          }
        }
      }
    }
    break;

    default:
    {
      printf("Invalid request %d\n", v2xOpt);
    }
    break;
  }
}


void v2xServiceIndication
(
 qmi_client_type user_handle,    /* QMI user handle       */
 unsigned int    msg_id,         /* Indicator message ID  */
 void           *ind_buf,        /* Raw indication data   */
 unsigned int    ind_buf_len,    /* Raw data length       */
 void           *ind_cb_data     /* User call back handle */
)
{
  qmi_client_error_type qmi_error;

  switch(msg_id)
  {
    case QMI_QCMAP_MSGR_V2X_SPS_FLOW_REG_RESULT_IND_V01:
    {
      qcmap_msgr_v2x_sps_flow_reg_result_ind_msg_v01    ind_msg;

      ZERO_INIT_ARG(ind_msg);
      qmi_error = qmi_client_message_decode(user_handle,
                                            QMI_IDL_INDICATION,
                                            msg_id,
                                            ind_buf,
                                            ind_buf_len,
                                            &ind_msg,
                                            sizeof(ind_msg));
      if (qmi_error != QMI_NO_ERR)
      {
        QCMAP_CLI_LOG("v2xServiceIndication: qmi_client_message_decode error %d\n",qmi_error);
        break;
      }

      SHOW_INDICATION_MSG_TO_USER(V2X_SPS_FLOW_REG_RESULT_IND);
      SHOW_MANDATORY_RESPONSE_TO_USER(Request ID, ind_msg.reg_result.flow_id.req_id);
      SHOW_MANDATORY_RESPONSE_TO_USER(SPS ID, ind_msg.reg_result.flow_id.sps_id);
      SHOW_MANDATORY_RESPONSE_TO_USER(Result, ind_msg.reg_result.result);
    }
    break;

    case QMI_QCMAP_MSGR_V2X_SPS_FLOW_DEREG_RESULT_IND_V01:
    {
      qcmap_msgr_v2x_sps_flow_dereg_result_ind_msg_v01   ind_msg;

      ZERO_INIT_ARG(ind_msg);
      qmi_error = qmi_client_message_decode(user_handle,
                                            QMI_IDL_INDICATION,
                                            msg_id,
                                            ind_buf,
                                            ind_buf_len,
                                            &ind_msg,
                                            sizeof(ind_msg));
      if (qmi_error != QMI_NO_ERR)
      {
        QCMAP_CLI_LOG("v2xServiceIndication: qmi_client_message_decode error %d\n",qmi_error);
        break;
      }

      SHOW_INDICATION_MSG_TO_USER(V2X_SPS_FLOW_DEREG_RESULT_IND);
      SHOW_MANDATORY_RESPONSE_TO_USER(Request ID, ind_msg.dereg_result.flow_id.req_id);
      SHOW_MANDATORY_RESPONSE_TO_USER(SPS ID, ind_msg.dereg_result.flow_id.sps_id);
      SHOW_MANDATORY_RESPONSE_TO_USER(Result, ind_msg.dereg_result.result);
    }
    break;

    case QMI_QCMAP_MSGR_V2X_SPS_FLOW_UPDATE_RESULT_IND_V01:
    {
      qcmap_msgr_v2x_sps_flow_update_result_ind_msg_v01   ind_msg;

      ZERO_INIT_ARG(ind_msg);
      qmi_error = qmi_client_message_decode(user_handle,
                                            QMI_IDL_INDICATION,
                                            msg_id,
                                            ind_buf,
                                            ind_buf_len,
                                            &ind_msg,
                                            sizeof(ind_msg));
      if (qmi_error != QMI_NO_ERR)
      {
        QCMAP_CLI_LOG("v2xServiceIndication: qmi_client_message_decode error %d\n",qmi_error);
        break;
      }

      SHOW_INDICATION_MSG_TO_USER(V2X_SPS_FLOW_UPDATE_RESULT_IND);
      SHOW_MANDATORY_RESPONSE_TO_USER(Request ID, ind_msg.update_result.flow_id.req_id);
      SHOW_MANDATORY_RESPONSE_TO_USER(SPS ID, ind_msg.update_result.flow_id.sps_id);
      SHOW_MANDATORY_RESPONSE_TO_USER(Result, ind_msg.update_result.result);
    }
    break;

    case QMI_QCMAP_MSGR_V2X_SERVICE_SUBSCRIBE_RESULT_IND_V01:
    {
      qcmap_msgr_v2x_service_subscribe_result_ind_msg_v01   ind_msg;

      ZERO_INIT_ARG(ind_msg);
      qmi_error = qmi_client_message_decode(user_handle,
                                            QMI_IDL_INDICATION,
                                            msg_id,
                                            ind_buf,
                                            ind_buf_len,
                                            &ind_msg,
                                            sizeof(ind_msg));
      if (qmi_error != QMI_NO_ERR)
      {
        QCMAP_CLI_LOG("v2xServiceIndication: qmi_client_message_decode error %d\n",qmi_error);
        break;
      }

      SHOW_INDICATION_MSG_TO_USER(V2X_SERVICE_SUBSCRIBE_RESULT_IND);
      SHOW_MANDATORY_RESPONSE_TO_USER(Request ID, ind_msg.req_id);
      SHOW_OPTIONAL_RESPONSE_TO_USER(Result of Wildcard Subscription, ind_msg.subscribe_wildcard_result);
      if (ind_msg.reg_result_valid)
      {
        printf("   Service Subscription Result List Length=%d\n", ind_msg.reg_result_len);
        for (int i=0; i<ind_msg.reg_result_len; i++)
        {
          INDENT_TEXT; SHOW_MANDATORY_RESPONSE_TO_USER(Service ID, ind_msg.reg_result[i].service_id);
          INDENT_TEXT; SHOW_MANDATORY_RESPONSE_TO_USER(Result, ind_msg.reg_result[i].result);
        }
      }

    }
    break;

    case QMI_QCMAP_MSGR_V2X_SEND_CONFIG_FILE_RESULT_IND_V01:
    {
      qcmap_msgr_v2x_send_config_file_result_ind_msg_v01   ind_msg;

      ZERO_INIT_ARG(ind_msg);

      //2 Suren - look into this.
      SHOW_INDICATION_MSG_TO_USER(V2X_SEND_CONFIG_FILE_RESULT_IND);
    }
    break;

    case QMI_QCMAP_MSGR_V2X_SPS_SCHEDULING_INFO_IND_V01:
    {
      qcmap_msgr_v2x_sps_scheduling_info_ind_msg_v01    ind_msg;

      ZERO_INIT_ARG(ind_msg);
      qmi_error = qmi_client_message_decode(user_handle,
                                            QMI_IDL_INDICATION,
                                            msg_id,
                                            ind_buf,
                                            ind_buf_len,
                                            &ind_msg,
                                            sizeof(ind_msg));
      if (qmi_error != QMI_NO_ERR)
      {
        QCMAP_CLI_LOG("v2xServiceIndication: qmi_client_message_decode error %d\n",qmi_error);
        break;
      }

      SHOW_INDICATION_MSG_TO_USER(V2X_SPS_SCHEDULING_INFO_IND);
      SHOW_MANDATORY_RESPONSE_TO_USER(SPS ID, ind_msg.info.sps_id);
      SHOW_MANDATORY_RESPONSE_TO_USER(UTC start time, ind_msg.info.utc_time);
      SHOW_MANDATORY_RESPONSE_TO_USER(Periodicity(milliSecs), ind_msg.info.periodicity);
    }
    break;

    case QMI_QCMAP_MSGR_V2X_SRC_L2_INFO_IND_V01:
    {
      qcmap_msgr_v2x_src_l2_info_ind_msg_v01  ind_msg;

      ZERO_INIT_ARG(ind_msg);
      qmi_error = qmi_client_message_decode(user_handle,
                                            QMI_IDL_INDICATION,
                                            msg_id,
                                            ind_buf,
                                            ind_buf_len,
                                            &ind_msg,
                                            sizeof(ind_msg));
      if (qmi_error != QMI_NO_ERR)
      {
        QCMAP_CLI_LOG("v2xServiceIndication: qmi_client_message_decode error %d\n",qmi_error);
        break;
      }

      SHOW_INDICATION_MSG_TO_USER(V2X_SRC_L2_INFO_IND);
      SHOW_MANDATORY_RESPONSE_TO_USER(Source L2 ID, ind_msg.src_l2_id);
    }
    break;

    case QMI_QCMAP_MSGR_V2X_CAPABILITY_INFO_IND_V01:
    {
      qcmap_msgr_v2x_capability_info_ind_msg_v01 ind_msg;

      ZERO_INIT_ARG(ind_msg);
      qmi_error = qmi_client_message_decode(user_handle,
                                            QMI_IDL_INDICATION,
                                            msg_id,
                                            ind_buf,
                                            ind_buf_len,
                                            &ind_msg,
                                            sizeof(ind_msg));
      if (qmi_error != QMI_NO_ERR)
      {
        QCMAP_CLI_LOG("v2xServiceIndication: qmi_client_message_decode error %d\n",qmi_error);
        break;
      }

      SHOW_INDICATION_MSG_TO_USER(V2X_CAPABILITY_INFO_IND);
      SHOW_OPTIONAL_RESPONSE_TO_USER(Maximum Number of SPS Flows, ind_msg.max_sps_flow_cnt);
      SHOW_OPTIONAL_RESPONSE_TO_USER(Maximum Number of Event Driven Flows, ind_msg.max_event_driven_flow_cnt);
      SHOW_OPTIONAL_RESPONSE_TO_USER(WWAN Concurrency Capability, ind_msg.is_concurrency_supported);
      SHOW_OPTIONAL_RESPONSE_TO_USER(Maximum Number of SPS Flows, ind_msg.pppp_info); //here
      if (ind_msg.pppp_info_valid)
      {
        printf("   Promixmity Service per Packet Priority Information Length=%d\n",
                ind_msg.pppp_info_len);
        if (ind_msg.pppp_info_len > 0)
        {
          for (int i=0; i<ind_msg.pppp_info_len; i++)
          {
            printf("      Priority[%d]=%d\n", i, ind_msg.pppp_info[i].priority);
            printf("      PDB[%d]=%d\n", i, ind_msg.pppp_info[i].pdb);
          }
        }
      }
      SHOW_OPTIONAL_RESPONSE_TO_USER(Supported Minimum Transmission Power, ind_msg.min_tx_pwr);
      SHOW_OPTIONAL_RESPONSE_TO_USER(Supported Maximum Transmission Power, ind_msg.max_tx_pwr);
      if (ind_msg.supported_periodicity_list_valid)
      {
        printf("   Supported SPS Periodicity List Length=%d\n", ind_msg.supported_periodicity_list_len);
        if (ind_msg.supported_periodicity_list_len > 0)
        {
          for (int i=0; i<ind_msg.supported_periodicity_list_len; i++)
          {
            printf("      Supported SPS Periodicity[%d]=%d\n", ind_msg.supported_periodicity_list[i]);
          }
        }
      }
      if (ind_msg.tx_pool_id_list_valid)
      {
        printf("   Transmission Pool ID List Length=%d\n", ind_msg.tx_pool_id_list_len);
        if (ind_msg.tx_pool_id_list_len > 0)
        {
          for (int i=0; i<ind_msg.tx_pool_id_list_len; i++)
          {
            printf("      Pool ID[%d]=%d\n", i, ind_msg.tx_pool_id_list[i].pool_id);
            printf("      Min freq[%d] in MHz=%d\n", i, ind_msg.tx_pool_id_list[i].min_freq);
            printf("      Max freq[%d] in MHz=%d\n", i, ind_msg.tx_pool_id_list[i].max_freq);
          }
        }
      }
    }
    break;

    default:
      printf("Unknown V2X indication msgId=%d is recevied\n", msg_id);
      break;
  }
}

void mtpeConfig(int mtpeOpt)
{
  qmi_error_type_v01 qmi_err_num = QMI_ERR_NONE_V01;

  char scan_string[QCMAP_MSGR_MAX_FILE_PATH_LEN];

  switch(mtpeOpt)
  {
    /* Set MTPE Config*/
    case 1:
    {
      qcmap_mtpe_config_data config_data;
      memset(&config_data, 0, sizeof(qcmap_mtpe_config_data));

      uint16 port = 0;
      uint16 port_range = 0;
      uint16 num_parallel_streams = 0;
      bool using_url = false;
      bool using_payload = false;

      QCMAP_PRINTF_TAKE_INPUT("Please input throughput estimation test type:\n\
          1:Uplink\n\
          2:Downlink\n\
          3:Ping \n");

      while(TRUE)
      {
        memset(scan_string, 0, QCMAP_MSGR_MAX_FILE_PATH_LEN);
        fgets(scan_string, sizeof(scan_string), stdin);
        config_data.test_type = atoi(scan_string);

        if (config_data.test_type == QCMAP_MTPE_TEST_TYPE_UPLINK_V01 ||
            config_data.test_type == QCMAP_MTPE_TEST_TYPE_DOWNLINK_V01 ||
            config_data.test_type == QCMAP_MTPE_TEST_TYPE_PING_V01) 
        {
          break;
        } else {
          printf("\nPlease input throughput estimation test type:\n\
              1:Uplink\n\
              2:Downlink\n\
              3:Ping \n");
          fflush(stdout);
        }
      }


      printf("\nPlease input test duration in seconds :- \n");
      while(TRUE)
      {
        memset(scan_string, 0, QCMAP_MSGR_MAX_FILE_PATH_LEN);
        fgets(scan_string, sizeof(scan_string), stdin);
        config_data.duration = atoi(scan_string);
        if (config_data.duration >= 0) {
          break;
        } else {
          printf("\nPlease input test duration in seconds :- \n");
        }
      }
      if (config_data.test_type != QCMAP_MTPE_TEST_TYPE_PING_V01)
      {

      printf("\nPlease input Protocol 1:UDP 2:TCP :- \n");
      fflush(stdout);
      while(TRUE)
      {
        memset(scan_string, 0, QCMAP_MSGR_MAX_FILE_PATH_LEN);
        fgets(scan_string, sizeof(scan_string), stdin);
        int val = atoi(scan_string);
        if (val == 1) {
          config_data.protocol = QCMAP_MSGR_PROTO_UDP_V01;
          break;
        } else if (val == 2) {
          config_data.protocol = QCMAP_MSGR_PROTO_TCP_V01;
          break;
        } else {
          printf("\nPlease input Protocol 1:UDP 2:TCP :- \n");
        }
      }
	  	}
      printf("\nPlease input IP Family 4:IPv4 6:IPv6 :- \n");
      while(TRUE)
      {
        memset(scan_string, 0, QCMAP_MSGR_MAX_FILE_PATH_LEN);
        fgets(scan_string, sizeof(scan_string), stdin);
        config_data.ip_family = atoi(scan_string);
        if (config_data.ip_family == QCMAP_MSGR_IP_FAMILY_V4_V01 ||
            config_data.ip_family == QCMAP_MSGR_IP_FAMILY_V6_V01)
        {
          break;
        } else {
          printf("\nPlease input IP Family 4:IPv4 6:IPv6 :- \n");
        }
      }

      printf("\nPlease input client's public IP address\\n");
      if(config_data.ip_family == QCMAP_MSGR_IP_FAMILY_V4_V01)
      {
        if(config_data.test_type == QCMAP_MTPE_TEST_TYPE_DOWNLINK_V01)
        {
          //Client is considered the destination in DL or Ping Test scenarios
          read_addr(AF_INET, ((uint8 *)&config_data.dst_ipv4_addr));
        }
        else
        {
          read_addr(AF_INET, ((uint8 *)&config_data.src_ipv4_addr));
        }
      }
      else
      {
        if(config_data.test_type == QCMAP_MTPE_TEST_TYPE_DOWNLINK_V01)
        {
          //Client is considered the destination in DL scenarios
          read_addr(AF_INET6, config_data.dst_ipv6_addr.addr);
          config_data.dst_ipv6_addr_valid = true;
        }
        else
        {
          read_addr(AF_INET6, config_data.src_ipv6_addr.addr);
          config_data.src_ipv6_addr_valid = true;
        }
      }

      printf("\nUse IP address or URL for Server: 0:IP Address 1:URL :- \n");
      memset(scan_string, 0, QCMAP_MSGR_MAX_FILE_PATH_LEN);
      fgets(scan_string, sizeof(scan_string), stdin);
      if (!VALID_NUMERIC_INPUT(scan_string) || (atoi(scan_string) != 1 && atoi(scan_string) != 0))
      {
        printf("\nInvalid response. Please enter 1 or 0.\n");
        break;
      }
      using_url = atoi(scan_string);

      if (using_url == 1) {
        char *ptr=NULL;
        printf("\nPlease input test server's URL\n");

        memset(config_data.server_url, 0, QCMAP_MSGR_MAX_GATEWAY_URL_V01);
        config_data.server_url_valid = true;
        fgets(config_data.server_url, sizeof(config_data.server_url), stdin);

        ptr = strchr(config_data.server_url,'\n');
        if( ptr )
          *ptr = 0;
      } else {
        printf("\nPlease input test server's public IP address\n");
        if(config_data.ip_family == QCMAP_MSGR_IP_FAMILY_V4_V01)
        {
          if(config_data.test_type == QCMAP_MTPE_TEST_TYPE_DOWNLINK_V01)
          {
            //Server is considered the source in DL scenarios
            read_addr(AF_INET, ((uint8 *)&config_data.src_ipv4_addr));
          }
          else
          {
            read_addr(AF_INET, ((uint8 *)&config_data.dst_ipv4_addr));
          }
        }
        else
        {
          if(config_data.test_type == QCMAP_MTPE_TEST_TYPE_DOWNLINK_V01)
          {
            //Server is considered the source in DL scenarios
            read_addr(AF_INET6, config_data.src_ipv6_addr.addr);
            config_data.src_ipv6_addr_valid = true;
          }
          else
          {
            read_addr(AF_INET6, config_data.dst_ipv6_addr.addr);
            config_data.dst_ipv6_addr_valid = true;
          }
        }
      }
      fflush(stdout);
      if (config_data.test_type != QCMAP_MTPE_TEST_TYPE_PING_V01)
      {
        printf("\nPlease input client's port :- \n");

        while(TRUE)
        {
          memset(scan_string, 0, QCMAP_MSGR_MAX_FILE_PATH_LEN);
          fgets(scan_string, sizeof(scan_string), stdin);
          port = atoi(scan_string);

          if (port != 0) {
            if(config_data.test_type == QCMAP_MTPE_TEST_TYPE_DOWNLINK_V01)
            {
              config_data.dst_port = port;
            }
            else
            {
              config_data.src_port = port;
            }
            break;
          } else {
            printf("\nPlease input client's port :- \n");
          }
        }

        printf("\nPlease input server's port :- \n");
        fflush(stdout);
        while(TRUE)
        {
          memset(scan_string, 0, QCMAP_MSGR_MAX_FILE_PATH_LEN);
          fgets(scan_string, sizeof(scan_string), stdin);
          port = atoi(scan_string);

          if (port > 0) {
            if(config_data.test_type == QCMAP_MTPE_TEST_TYPE_DOWNLINK_V01)
            {
              config_data.src_port = port;
            }
            else
            {
              config_data.dst_port = port;
            }
            break;
          } else {
            printf("\nPlease input server's port :- \n");
            fflush(stdout);
          }

        }

        printf("\nProvide optional packet payload? 1:Yes 0:No :- \n");  //Optional Payload

        memset(scan_string, 0, QCMAP_MSGR_MAX_FILE_PATH_LEN);
        fgets(scan_string, sizeof(scan_string), stdin);
        using_payload = atoi(scan_string);
        if (using_payload) {
          config_data.payload_valid = true;
          char * ptr = NULL;

          if((config_data.test_type == QCMAP_MTPE_TEST_TYPE_UPLINK_V01) ||
              (config_data.test_type == QCMAP_MTPE_TEST_TYPE_PING_V01))
          {
            char payload_string[QCMAP_MSGR_MTPE_PACKET_PAYLOAD_MAX_LEN_V01];
            memset(payload_string, 0, QCMAP_MSGR_MTPE_PACKET_PAYLOAD_MAX_LEN_V01);

            printf("Provide packet payload: \n");
            fgets(payload_string, sizeof(payload_string), stdin);

            ptr = strchr(payload_string,'\n');
            if( ptr )
              *ptr = 0;

            config_data.payload_len = strlen(payload_string);
            memcpy(config_data.payload.payload, payload_string, config_data.payload_len);

            LOG_MSG_INFO1("Paylod string: %s\n",payload_string,0,0);
          }
          else if(config_data.test_type == QCMAP_MTPE_TEST_TYPE_DOWNLINK_V01)
          {

            qcmap_mtpe_downlink_payload dl_data;
            memset(&dl_data, 0, sizeof(dl_data));

            dl_data.duration = config_data.duration;
            dl_data.dl_code = MTPE_DL_TRIGGER_CODE;

            //bandwidth
            printf("Provide bandwidth (kbps): \n");
            memset(scan_string, 0, QCMAP_MSGR_MAX_FILE_PATH_LEN);
            fgets(scan_string, sizeof(scan_string), stdin);
            ptr = strchr(scan_string,'\n');
            if( ptr )
              *ptr = 0;
            dl_data.bandwidth = atoi(scan_string);

            //MTU
            printf("Provide MTU: \n");
            memset(scan_string, 0, QCMAP_MSGR_MAX_FILE_PATH_LEN);
            fgets(scan_string, sizeof(scan_string), stdin);
            ptr = strchr(scan_string,'\n');
            if( ptr )
              *ptr = 0;

            dl_data.mtu = atoi(scan_string);

            memcpy(config_data.payload.payload, &dl_data, sizeof(dl_data));
            config_data.payload_len = sizeof(qcmap_mtpe_downlink_payload);

            LOG_MSG_INFO1("Payload_len: %d\n",config_data.payload_len,0,0);
            LOG_MSG_INFO1("BW: %d MTU: %d\n", dl_data.bandwidth, dl_data.mtu,0);
          }
        }

        if (config_data.protocol == QCMAP_MSGR_PROTO_TCP_V01)
        {
          printf("\nPlease input port range [0,10]:- \n");
          fflush(stdout);
          while(TRUE)
          {
            memset(scan_string, 0, QCMAP_MSGR_MAX_FILE_PATH_LEN);
            fgets(scan_string, sizeof(scan_string), stdin);
            port_range = atoi(scan_string);

            if (port_range >= 0) {
              config_data.dst_port_range = port_range;
              break;
            } else {
              printf("\nPlease input port range [0,10]:- \n");
              fflush(stdout);
            }
          }

          if (config_data.test_type == QCMAP_MTPE_TEST_TYPE_UPLINK_V01)
          {
            printf("\nPlease parallel streams [1,4]:- \n");
            fflush(stdout);
            while(TRUE)
            {
              memset(scan_string, 0, QCMAP_MSGR_MAX_FILE_PATH_LEN);
              fgets(scan_string, sizeof(scan_string), stdin);
              num_parallel_streams = atoi(scan_string);

              if (num_parallel_streams > 0) {
                config_data.num_parallel_streams= num_parallel_streams;
                break;
              } else {
                printf("\nPlease parallel streams [1,4]:- \n");
                fflush(stdout);
              }
            }
          }
        }
      }

      if(QcMapClient->ConfigureMTPE(&config_data, &qmi_err_num))
      {
        printf("Modem throughput estimation configuration sent.\n");
      }
      else
      {
        printf("Failed to send Modem throughput estimation configuration, Error:0x%x\n", qmi_err_num);
      }

    }
    break;

    /* Start MTPE Test*/
    case 2:
    {
      if(QcMapClient->StartMTPE(&qmi_err_num))
      {
        printf("Successfully started Modem Throughput Estimation test\n");
      }
      else
      {
        printf("Failed to start the Modem Throughput Estimation test, Error:0x%x\n", qmi_err_num);
      }
    }
    break;

    /* Stop MTPE Test*/
    case 3:
    {
      if(QcMapClient->StopMTPE(&qmi_err_num))
      {
        printf("Successfully stoped Modem Throughput Estimation test\n");
      }
      else
      {
        printf("Failed to stop the Modem Throughput Estimation test, Error:0x%x\n", qmi_err_num);
      }
    }
    break;

    /* Get MTPE Test Config Info */
    case 4:
    {
      qcmap_msgr_ip_family_enum_v01 ip_family;
      qcmap_nw_params_t             ip_info;
      uint32                        bandwidth;
      uint32                        mtu;

      printf("\nPlease input IP Family 4:IPv4 6:IPv6 :- \n");
      while(TRUE)
      {
        memset(scan_string, 0, QCMAP_MSGR_MAX_FILE_PATH_LEN);
        fgets(scan_string, sizeof(scan_string), stdin);
        ip_family = atoi(scan_string);
        if (ip_family == 4 || ip_family == 6) {
          break;
        } else {
          printf("\nPlease input IP Family 4:IPv4 6:IPv6 :- \n");
        }
      }

      printf("\nPlease input client's public IP address\n");
      if(ip_family == QCMAP_MSGR_IP_FAMILY_V4_V01)
      {
        read_addr(AF_INET, (uint8 *)&ip_info.v4_conf.public_ip.s_addr);
      }
      else
      {
        read_addr(AF_INET6, ip_info.v6_conf.public_ip_v6.s6_addr);
      }

      if(QcMapClient->GetMTPETestInfo(ip_family, ip_info, &bandwidth, &mtu, &qmi_err_num))
      {
        printf("Configured Bandwidth(kbps): %d \n Supported MTU: %d\n", bandwidth, mtu);
      }
      else
      {
        printf("Failed to retreive Test Configuration information, Error:0x%x\n", qmi_err_num);
      }
    }
    break;

    /* Teardown MTPE Config*/
    case 5:
    {
      if(QcMapClient->TeardownMTPE(&qmi_err_num))
      {
        printf("Successfully tore down Modem Throughput Estimation configuration\n");
      }
      else
      {
        printf("Failed to tear down Modem Throughput Estimation configuration, Error: 0x%x\n", qmi_err_num);
      }
    }
    break;

    /* Get MTPE History */
    case 6: {
        int num_history_items = 0;
        printf("\nEnter the number of history items to retrieve: ");
        READ_AND_VALIDATE_INT_VALUE(num_history_items, 0, 100);

        if (!QcMapClient->GetMTPEHistory((uint32)num_history_items,
                                         mtpe_history_entries,
                                         &mtpe_history_ready_flag,
                                         &len_mtpe_history,
                                         &mtpe_txn_id,
                                         &qmi_err_num)) {
          printf("\nRetrieving requested MTPE history items failed, Error: 0x%x\n", qmi_err_num);

        }else{

          pthread_mutex_lock(&mtpe_hist_mutex);

          if(num_history_items < len_mtpe_history) {
            printf("Only retrieved %d MTPE history items (expected %d)\n", len_mtpe_history, num_history_items);
          }

          // All requested data could be served in one single response packet
          if(mtpe_history_ready_flag) {
            printf("Successfully retrieved requested MTPE history items:\n");
            for(int i=0; i<len_mtpe_history; i++) {
                print_mtpe_entry((mtpe_history_entry*)
                                 ((char*)mtpe_history_entries + (sizeof(mtpe_history_entry) *i)));
            }
          }else{
            // Requested data will be served by indications from the QCMAP Server
            printf("Waiting on further indications...\n");
          }

          pthread_mutex_unlock(&mtpe_hist_mutex);
        }

    }
    break;

    default:
      printf("Invalid option: %d\n", mtpeOpt);
    break;
  }


  return;
}

/* Eth over GRE Tunnel Config */
void eogreTunnelConfig(int eogre_option)
{
  qmi_error_type_v01 qmi_err_num;
  /* Initialize QMI Error Number. */
  qmi_err_num = QMI_ERR_NONE_V01;
  char scan_string[QCMAP_MSGR_MAX_FILE_PATH_LEN];
  profile_handle_type_v01 profile_handle = 0;

  /* EoGRE tunnel configuration options */
  switch(eogre_option)
  {
    /*Set EoGRE Interface*/
    case 1:
    {
      qcmap_msgr_ip_family_enum_v01 ip_family;
      qcmap_eogre_param eogre_end_point;
      memset(&eogre_end_point,0,sizeof(qcmap_eogre_param));

      QCMAP_PRINTF_TAKE_INPUT("Please input IP Family 4==>IPV4, 6==>IPV6: ");
      fgets(scan_string, sizeof(scan_string), stdin);
      ip_family = (qcmap_msgr_ip_family_enum_v01)atoi(scan_string);

      if(ip_family == QCMAP_MSGR_IP_FAMILY_V4_V01)
      {
        eogre_end_point.ip_family = QCMAP_MSGR_IP_FAMILY_V4_V01;
        printf("\nPlease input EoGRE destination IPv4 Addr:");
        read_addr(AF_INET,(char *)&eogre_end_point.ipv4_gretap_dst_addr);
      }
      else if(ip_family == QCMAP_MSGR_IP_FAMILY_V6_V01)
      {
        eogre_end_point.ip_family = QCMAP_MSGR_IP_FAMILY_V6_V01;
        printf("\nPlease input EoGRE destination IPv6 Addr:");
        read_addr(AF_INET6,(char *)&eogre_end_point.ipv6_gretap_dst_addr[0]);
      }
      else
      {
        printf("\nInvalid value \n");
        break;
      }

      if(QcMapClient->SetEoGREInterfaceConfig(eogre_end_point, &qmi_err_num))
      {
        printf("\n Set EoGRE end point succeeds");
      }
      else
      {
        printf("\n Set EoGRE end point fails, Error: 0x%x \n",
            qmi_err_num);
      }
      break;
    }

    /*Get EoGRE Interface*/
    case 2:
    {
      qcmap_eogre_param eogre_end_point;
      char v6add_str[INET6_ADDRSTRLEN]   = {0};
      char v4add_str[INET_ADDRSTRLEN]    = {0};
      memset(&eogre_end_point,0,sizeof(qcmap_eogre_param));

      if(QcMapClient->GetEoGREInterfaceConfig(&eogre_end_point, &qmi_err_num))
      {
        if(eogre_end_point.ip_family == QCMAP_MSGR_IP_FAMILY_V4_V01)
        {
          printf("\nIP Version: v4");
          inet_ntop(AF_INET,
                    &eogre_end_point.ipv4_gretap_dst_addr,
                    v4add_str,
                    INET_ADDRSTRLEN);
          printf("\nEoGRE Dst IPv4 Address: %s",v4add_str);
        }
        else if(eogre_end_point.ip_family == QCMAP_MSGR_IP_FAMILY_V6_V01)
        {
          printf("\nIP Version: v6");
          inet_ntop(AF_INET6,
                    (void *)&eogre_end_point.ipv6_gretap_dst_addr[0],
                    v6add_str,
                    INET6_ADDRSTRLEN);
          printf("\nEoGRE Dst IPv6 Address: %s",v6add_str);
        }
        else if (eogre_end_point.ip_family == 0)
        {
          printf("\nEoGRE Dst IPv4/v6 address not set");
        }
      }
      else
      {
         printf("\n Get EoGRE end point fails, Error: 0x%x \n",
            qmi_err_num);
      }
      break;
    }

    /*Set EoGRE DSCP Marking*/
    case 3:
    {
      int16_t vlan_id;
      int8_t  pcp;
      int8_t  dscp;

      qcmap_eogre_vlan_pcp_to_dscp_mapping dscp_marking_config;
      memset(&dscp_marking_config,0,sizeof(qcmap_eogre_vlan_pcp_to_dscp_mapping));

      QCMAP_PRINTF_TAKE_INPUT("Please input VLAN ID [%d-%d]: ",MIN_VLAN_ID,MAX_VLAN_ID);
      fgets(scan_string, sizeof(scan_string), stdin);
      vlan_id = atoi(scan_string);

      QCMAP_PRINTF_TAKE_INPUT("Please input PCP [0-7]: ");
      fgets(scan_string, sizeof(scan_string), stdin);
      if (!VALID_NUMERIC_INPUT(scan_string))
      {
        printf("\nInvalid response\n");
        break;
      }
      pcp = atoi(scan_string);

      QCMAP_PRINTF_TAKE_INPUT("Please input DSCP Marking [0-63]: ");
      fgets(scan_string, sizeof(scan_string), stdin);
      if (!VALID_NUMERIC_INPUT(scan_string))
      {
        printf("\nInvalid response\n");
        break;
      }
      dscp = atoi(scan_string);

      dscp_marking_config.vlan_id = vlan_id;
      dscp_marking_config.pcp = pcp;
      dscp_marking_config.dscp = dscp;

      if(QcMapClient->AddEoGREDSCPMarking(dscp_marking_config, &qmi_err_num))
      {
        printf("\n Add EoGRE DSCP Marking succeeds");
      }
      else
      {
        printf("\n Add EoGRE DSCP Marking fails, Error: 0x%x \n",
               qmi_err_num);
      }
      break;
    }

    /*Get EoGRE DSCP Marking*/
    case 4:
    {
      //Size of eogre_dscp_mapping_length set to 20 for testing
      qcmap_eogre_vlan_pcp_to_dscp_mapping map_vlan_pcp_to_dscp[20];
      int8_t length;
      int eogre_dscp_mapping_length = sizeof(map_vlan_pcp_to_dscp)/sizeof(map_vlan_pcp_to_dscp[0]);

      memset(map_vlan_pcp_to_dscp, 0, (sizeof(qcmap_eogre_vlan_pcp_to_dscp_mapping) * 20));

      if(QcMapClient->GetEoGREDSCPMarking(map_vlan_pcp_to_dscp, eogre_dscp_mapping_length, &length, &qmi_err_num))
      {
        printf("\n Get EoGRE DSCP Marking succeeds");
        if( DisplayEoGREDSCPMarkingList(map_vlan_pcp_to_dscp, &length ) == true)
        {
          printf("\n List VLAN and PCP to DSCP marking success \n");
        }
      }
      else
      {
        printf("\n Get EoGRE DSCP Marking fails, Error: 0x%x \n",
            qmi_err_num);
      }
      break;
    }

    /*Delete EoGRE DSCP Marking*/
    case 5:
    {
      int16_t vlan_id;
      int8_t  pcp;

      QCMAP_PRINTF_TAKE_INPUT("Please input VLAN ID: ");
      fgets(scan_string, sizeof(scan_string), stdin);
      vlan_id = atoi(scan_string);

      QCMAP_PRINTF_TAKE_INPUT("Please input PCP: ");
      fgets(scan_string, sizeof(scan_string), stdin);
      pcp = atoi(scan_string);

      if(QcMapClient->DeleteEoGREDSCPMarking(vlan_id, pcp, &qmi_err_num))
      {
        printf("\n Delete EoGRE DSCP Marking succeeds");
      }
      else
      {
        printf("\n Delete EoGRE DSCP Marking fails, Error: 0x%x \n",
            qmi_err_num);
      }
      break;
    }

    default:
      printf("Unknown EoGRE option");
      break;
  }

  return;
}

void getConnectedDevicesInfoConfig(int getCDIOpt)
{
  qmi_error_type_v01 qmi_err_num = QMI_ERR_NONE_V01;

  /* Get Connected Devices Info configuration options */
  switch(getCDIOpt)
  {
    case 1:
    {
      char tmpIPv4[INET_ADDRSTRLEN] = {0};
      in6_addr allipv6[QCMAP_MSGR_MAX_IPV6_ADDR_V01];
      in6_addr tmpipv6;
      boolean flag = false;
      uint8 zero_buff[QCMAP_MSGR_IPV6_ADDR_LEN_V01] = {0};
      char ip6_addr_buf[INET6_ADDRSTRLEN] = {0};
      int connDevCount=0, num_entries=0;
      char mac_addr_str[QCMAP_MSGR_MAC_ADDR_NUM_CHARS_V01] = {0}; /*char array of mac address*/
      qcmap_msgr_connected_device_info_v01
            connected_devices[QCMAP_MSGR_MAX_CONNECTED_DEVICES_V01];
      memset(connected_devices,
               0,
               (QCMAP_MSGR_MAX_CONNECTED_DEVICES_V01
               *sizeof(qcmap_msgr_connected_device_info_v01)));
      ZERO_INIT_ARG(tmpIPv4);

      is_ipv6nat_enabled = QcMapClient->GetIPv6NAT(&flag,&qmi_err_num);

      if (QcMapClient->GetConnectedDevicesInfo(connected_devices, &num_entries, &qmi_err_num))
      {
        if(num_entries != 0)
        {
          printf("\n Printing Connected Device Info for this Device \n");
          printf("\n----------------------------------------------\n");
          // Displaying the information in appropriate fashion
          for (connDevCount=0; connDevCount<num_entries; connDevCount++)
          {
            printf("Device No : %d \n",connDevCount+1);
            ds_mac_addr_ntop(connected_devices[connDevCount].client_mac_addr,
                               mac_addr_str);
            printf("MAC Address : %s \n",mac_addr_str);
            if(tmpIPv4 && inet_ntop(AF_INET,
                         (void *)&connected_devices[connDevCount].ipv4_addr,tmpIPv4,
                           INET_ADDRSTRLEN))
            {
              printf("IPv4 Address : %s \n",tmpIPv4);
            }
            ZERO_INIT_ARG(tmpipv6);
            memcpy(&tmpipv6.s6_addr,
                     connected_devices[connDevCount].ll_ipv6_addr,
                     QCMAP_MSGR_IPV6_ADDR_LEN_V01);
            if(inet_ntop(AF_INET6, (void*)&tmpipv6, ip6_addr_buf, sizeof(ip6_addr_buf)))
            {
              printf("Link Local IPv6 Address : %s\n",ip6_addr_buf);
            }

            if (is_ipv6nat_enabled && flag)
            {
              ZERO_INIT_ARG(tmpipv6);

              memcpy(&tmpipv6.s6_addr,
                       connected_devices[connDevCount].ula_ipv6_addr,
                       QCMAP_MSGR_IPV6_ADDR_LEN_V01);

              if(inet_ntop(AF_INET6, (void*)&tmpipv6, ip6_addr_buf, sizeof(ip6_addr_buf)) != NULL)
              {
                printf("ULA IPv6 Address : %s\n",ip6_addr_buf);
              }
            }

            for (int i = 0; i < QCMAP_MSGR_MAX_IPV6_ADDR_V01; i++)
            {
              memset(&allipv6[i], 0, sizeof(in6_addr));
              memset(ip6_addr_buf, 0, INET6_ADDRSTRLEN);

              memcpy(&allipv6[i].s6_addr,
                      connected_devices[connDevCount].ipv6[i].addr,
                       QCMAP_MSGR_IPV6_ADDR_LEN_V01);
              if (!memcmp(&allipv6[i].s6_addr, zero_buff,QCMAP_MSGR_IPV6_ADDR_LEN_V01))
                break;
              if(inet_ntop(AF_INET6,
                            (void*)&allipv6[i], ip6_addr_buf, sizeof(ip6_addr_buf)))
              {
                printf("IPv6 Address %d: %s\n",i, ip6_addr_buf);
              }
            }
            switch (connected_devices[connDevCount].device_type)
            {
              case QCMAP_MSGR_DEVICE_TYPE_PRIMARY_AP_V01:
                printf("Device Type : Primary AP\n");
                break;
              case QCMAP_MSGR_DEVICE_TYPE_GUEST_AP_V01:
                printf("Device Type :Guest AP1\n");
                break;
              case QCMAP_MSGR_DEVICE_TYPE_GUEST_AP_2_V01:
                printf("Device Type :Guest AP2\n");
                break;
              case QCMAP_MSGR_DEVICE_TYPE_GUEST_AP_3_V01:
                printf("Device Type :Guest AP3\n");
                break;
              case QCMAP_MSGR_DEVICE_TYPE_USB_V01:
                printf("Device Type :USB\n");
                break;
              case QCMAP_MSGR_DEVICE_TYPE_ETHERNET_V01:
                printf("Device Type :Ethernet\n");
                break;

              case QCMAP_MSGR_DEVICE_TYPE_ETHERNET_NIC2_V01:
                printf("Device Type :Ethernet-NIC2\n");
                break;

              case QCMAP_MSGR_DEVICE_TYPE_ANY_AP_V01:
                printf("Device Type :AP\n");
                break;

              case QCMAP_MSGR_WLAN_IFACE_MLD_AP:
                printf("Device Type :MLD-AP\n");
                break;

              default:
                printf("Device Type : Invalid\n");
                break;
            }
            printf("Host Name : %s\n",
                   connected_devices[connDevCount].host_name);
            printf("rx bytes : %lu\n",
                   connected_devices[connDevCount].bytes_rx);
            printf("tx bytes : %lu\n",
                   connected_devices[connDevCount].bytes_tx);
            printf("Lease Expiry Time (in minutes) : %d\n",
                   connected_devices[connDevCount].lease_expiry_time);
            if(connected_devices[connDevCount].vlan_id != 0)
            {
              printf("VLAN ID :%d\n",connected_devices[connDevCount].vlan_id);
            }
            printf("\n----------------------------------------------\n");
          }
        }
        else
        {
          printf("\n No Connected Device to this Access Point \n");
          printf("\n----------------------------------------------\n");
        }
      }
      else
      {
        printf("\n Error in fetching Connected Device info. Error: 0x%x\n",
               qmi_err_num);
        printf("\n----------------------------------------------\n");
      }
      break;
    }
    case 2:
    {
      char tmpIPv4[INET_ADDRSTRLEN] = {0};
      in6_addr allipv6[QCMAP_MSGR_MAX_IPV6_ADDR_V01];
      in6_addr tmpipv6;
      boolean flag = false;
      uint8 zero_buff[QCMAP_MSGR_IPV6_ADDR_LEN_V01] = {0};
      char ip6_addr_buf[INET6_ADDRSTRLEN] = {0};
      int connDevCount=0, num_entries=0, trans_id=0;
      char mac_addr_str[QCMAP_MSGR_MAC_ADDR_NUM_CHARS_V01] = {0}; /*char array of mac address*/
      qcmap_msgr_connected_device_info_v01
            connected_devices[QCMAP_MSGR_MAX_CONNECTED_DEVICES_EX_V01];
      memset(connected_devices,
               0,
               (QCMAP_MSGR_MAX_CONNECTED_DEVICES_EX_V01
               *sizeof(qcmap_msgr_connected_device_info_v01)));
      memset(tmpIPv4,0,INET_ADDRSTRLEN);

      // Parse the IPv6 data coming from CDI once for every Get CDI call and use for each client connected
      is_ipv6nat_enabled = QcMapClient->GetIPv6NAT(&flag,&qmi_err_num);

      if (QcMapClient->GetConnectedDevicesInfo_Ex(&trans_id,connected_devices,
                                                 &num_entries, &qmi_err_num))
      {
        if(trans_id)
        {
           printf("\nFragmentation is enabled");
           printf("\nConnected Devices Info with transaction id %d will be printed in follow on indications",
                  trans_id);
        }
        else
        {
          if(num_entries != 0)
          {
            printf("\n Printing Connected Device Info for this Device \n");
            printf("\n----------------------------------------------\n");
            for (connDevCount=0; connDevCount<num_entries; connDevCount++)
            {
              printf("Device No : %d \n",connDevCount+1);
              ds_mac_addr_ntop(connected_devices[connDevCount].client_mac_addr, mac_addr_str);
              printf("MAC Address : %s \n",mac_addr_str);
              if(inet_ntop(AF_INET,
                           (void *)&connected_devices[connDevCount].ipv4_addr,tmpIPv4,
                           INET_ADDRSTRLEN))
              {
                printf("IPv4 Address : %s \n",tmpIPv4);
              }
              ZERO_INIT_ARG(tmpipv6);
              memcpy(&tmpipv6.s6_addr,
                     connected_devices[connDevCount].ll_ipv6_addr,
                     QCMAP_MSGR_IPV6_ADDR_LEN_V01);
              if(inet_ntop(AF_INET6, (void*)&tmpipv6, ip6_addr_buf, sizeof(ip6_addr_buf)))
              {
                printf("Link Local IPv6 Address : %s\n",ip6_addr_buf);
              }

              if (is_ipv6nat_enabled && flag)
              {
                memset(&tmpipv6, 0, sizeof(tmpipv6));

                memcpy(&tmpipv6.s6_addr,
                       connected_devices[connDevCount].ula_ipv6_addr,
                       QCMAP_MSGR_IPV6_ADDR_LEN_V01);

                if(inet_ntop(AF_INET6,
                             (void*)&tmpipv6, ip6_addr_buf, sizeof(ip6_addr_buf)) != NULL)
                {
                  printf("ULA IPv6 Address : %s\n",ip6_addr_buf);
                }
              }

              for (int i = 0; i < QCMAP_MSGR_MAX_IPV6_ADDR_V01; i++)
              {
                memset(&allipv6[i], 0, sizeof(in6_addr));
                memset(ip6_addr_buf, 0, INET6_ADDRSTRLEN);

                memcpy(&allipv6[i].s6_addr,
                      connected_devices[connDevCount].ipv6[i].addr,
                       QCMAP_MSGR_IPV6_ADDR_LEN_V01);
                if (!memcmp(&allipv6[i].s6_addr, zero_buff,QCMAP_MSGR_IPV6_ADDR_LEN_V01))
                  break;
                if(inet_ntop(AF_INET6,
                            (void*)&allipv6[i], ip6_addr_buf, sizeof(ip6_addr_buf)))
                {
                  printf("IPv6 Address %d: %s\n",i, ip6_addr_buf);
                }
              }
              switch (connected_devices[connDevCount].device_type)
              {
                 case QCMAP_MSGR_DEVICE_TYPE_PRIMARY_AP_V01:
                   printf("Device Type : Primary AP\n");
                   break;
                case QCMAP_MSGR_DEVICE_TYPE_GUEST_AP_V01:
                  printf("Device Type :Guest AP1\n");
                 break;
                case QCMAP_MSGR_DEVICE_TYPE_GUEST_AP_2_V01:
                  printf("Device Type :Guest AP2\n");
                  break;
                case QCMAP_MSGR_DEVICE_TYPE_GUEST_AP_3_V01:
                  printf("Device Type :Guest AP3\n");
                  break;
                case QCMAP_MSGR_DEVICE_TYPE_USB_V01:
                  printf("Device Type :USB\n");
                  break;
                case QCMAP_MSGR_DEVICE_TYPE_ETHERNET_V01:
                  printf("Device Type :Ethernet\n");
                  break;

                case QCMAP_MSGR_DEVICE_TYPE_ETHERNET_NIC2_V01:
                  printf("Device Type :Ethernet-NIC2\n");
                  break;

                default:
                  printf("Device Type : Invalid\n");
                  break;
              }
              printf("Host Name : %s\n",
                     connected_devices[connDevCount].host_name);
              printf("rx bytes : %lu\n",
                     connected_devices[connDevCount].bytes_rx);
              printf("tx bytes : %lu\n",
                     connected_devices[connDevCount].bytes_tx);
              printf("Lease Expiry Time (in minutes) : %d\n",
                     connected_devices[connDevCount].lease_expiry_time);
              if(connected_devices[connDevCount].vlan_id != 0)
              {
                printf("VLAN ID :%d\n",connected_devices[connDevCount].vlan_id);
              }
              printf("\n----------------------------------------------\n");
            }
          }
          else
          {
            printf("\n No Connected Device to this Access Point \n");
            printf("\n----------------------------------------------\n");
          }
        }
      }
      else
      {
         printf("\n Error in fetching Connected Device info. Error: 0x%x\n",
                 qmi_err_num);
         printf("\n----------------------------------------------\n");
      }
      break;
    }
    default:
    {
       printf("Invalid Request %d\n", getCDIOpt);
    }
  }
}

static boolean setHostToHostConfig(qcmap_ipsec_config_t *ipsec_config)
{
  char scan_string[QCMAP_MSGR_MAX_FILE_PATH_LEN];
  in_addr addr;
  in6_addr ipv6_addr;

  memset(scan_string, 0, QCMAP_MSGR_MAX_FILE_PATH_LEN);

  /*Read in IKE tunnel info*/
  /*Read in profile id*/
  QCMAP_PRINTF_TAKE_INPUT("   Please input the profile id:");
  fgets(scan_string, sizeof(scan_string), stdin);
  (*ipsec_config).profile_id = atoi(scan_string);

  /*Read in IKE_identifier*/
  QCMAP_PRINTF_TAKE_INPUT("   Please input the IKE_identifier of IPsec tunnel:");
  fgets(scan_string, sizeof(scan_string), stdin);
  scan_string[strlen(scan_string)-1]='\0';
  strlcpy((*ipsec_config).ike_cfg.ike_identifier,scan_string, sizeof(scan_string));

  /*Read in tunnel type*/
  QCMAP_PRINTF_TAKE_INPUT("   Please input the tunnel type(1 for IPv4, 2 for IPv6):");
  fgets(scan_string, sizeof(scan_string), stdin);
  (*ipsec_config).ike_cfg.tunnel_type = atoi(scan_string);

  /*Read in auth_type*/
  QCMAP_PRINTF_TAKE_INPUT("   Please input the authentication type(1 for PSK, 2 for X509):");
  fgets(scan_string, sizeof(scan_string), stdin);
  (*ipsec_config).ike_cfg.auth_type = atoi(scan_string);

  /*Read in remote_ep_addr*/
  if ((*ipsec_config).ike_cfg.tunnel_type == QCMAP_IPSEC_V4_ESP_TUNNEL_MODE_TUNNEL_TYPE) /*IPv4*/
  {
    while (TRUE)
    {
      QCMAP_PRINTF_TAKE_INPUT("   Please input Remote Gateway IPv4 address : ");
      if (fgets(scan_string, sizeof(scan_string), stdin) != NULL)
      {
        memset(&addr,0,sizeof(in_addr));
        scan_string[strlen(scan_string)-1]='\0';
        if ( !(inet_aton(scan_string, &addr) <= 0) )
        {
          strlcpy((*ipsec_config).ike_cfg.remote_ep_addr, scan_string, sizeof(scan_string));
          break;
        }
      }
      printf("      Invalid IPv4 address %s\n", scan_string);
    }

  }else if((*ipsec_config).ike_cfg.tunnel_type == QCMAP_IPSEC_V6_ESP_TUNNEL_MODE_TUNNEL_TYPE){
        /*IPv6*/
    while (TRUE) {
      QCMAP_PRINTF_TAKE_INPUT("   Please input Remote Gateway IPv6 address : ");
      if (fgets(scan_string, sizeof(scan_string), stdin) != NULL)
      {
        memset(&ipv6_addr, 0, sizeof(ipv6_addr));
        scan_string[strlen(scan_string)-1]='\0';
        if (inet_pton(AF_INET6, scan_string, &(ipv6_addr.s6_addr)) == 1)
        {
          strlcpy((*ipsec_config).ike_cfg.remote_ep_addr, scan_string, sizeof(scan_string));
          break;
        }
        printf("      Invalid IPv6 address %s\n", scan_string);
      }
    }

  }else{
    (*ipsec_config).ike_cfg.tunnel_type == 0;

  }


  if ((*ipsec_config).ike_cfg.auth_type == QCMAP_IPSEC_PSK_AUTHENTICATION_TYPE || (*ipsec_config).ike_cfg.auth_type == QCMAP_IPSEC_X509_AUTHENTICATION_TYPE) {
    /*Read in local_identifier*/
    QCMAP_PRINTF_TAKE_INPUT("   Please input the local identifier:");
    fgets(scan_string, sizeof(scan_string), stdin);
    scan_string[strlen(scan_string)-1]='\0';
    strlcpy((*ipsec_config).ike_cfg.local_identifier, scan_string, sizeof(scan_string));

    /*Read in remote_identifier*/
    QCMAP_PRINTF_TAKE_INPUT("   Please input the remote identifier:");
    fgets(scan_string, sizeof(scan_string), stdin);
    scan_string[strlen(scan_string)-1]='\0';
    strlcpy((*ipsec_config).ike_cfg.remote_identifier, scan_string, sizeof(scan_string));
  }else{
    (*ipsec_config).ike_cfg.auth_type == 0;
  }


  /*tunnel IP*/
  //QCMAP_PRINTF_TAKE_INPUT("   Please input the tunnel IP:");
  //fgets(scan_string, sizeof(scan_string), stdin);
  //*ipsec_config.ike_cfg.tunnel_ip = scan_string;

  /*Rekey time*/
  QCMAP_PRINTF_TAKE_INPUT("   Please input rekey time for IKE tunnel:");
  fgets(scan_string, sizeof(scan_string), stdin);
  (*ipsec_config).ike_cfg.rekey_interval_hrs = atoi(scan_string);

  return true;

}

static boolean setSiteToSiteWithoutNATConfig(qcmap_ipsec_config_t *ipsec_config)
{
  char scan_string[QCMAP_MSGR_MAX_FILE_PATH_LEN];
  in_addr addr;

  memset(scan_string, 0, QCMAP_MSGR_MAX_FILE_PATH_LEN);
  memset(&addr, 0, sizeof(in_addr));

  /*Child SA*/
  /*Read in child entires*/
  QCMAP_PRINTF_TAKE_INPUT("   Please input number of Child SAs : ");
  fgets(scan_string, sizeof(scan_string), stdin);
  (*ipsec_config).child_entries = atoi(scan_string);

  for (int i = 0; i < (*ipsec_config).child_entries; i++) {
    printf("\n-----------------Child %d------------------\n",i+1);

    /*Read in child_identifier*/
    QCMAP_PRINTF_TAKE_INPUT("   Please input the CHILD_identifier of IPsec tunnel:");
    fgets(scan_string, sizeof(scan_string), stdin);
    scan_string[strlen(scan_string)-1]='\0';
    strlcpy((*ipsec_config).child_cfg[i].child_identifier,scan_string, sizeof(scan_string));

    /*Read in port_id*/
    QCMAP_PRINTF_TAKE_INPUT("   Please input port id(0 for no port_id) : ");
    fgets(scan_string, sizeof(scan_string), stdin);
    (*ipsec_config).child_cfg[i].port_id = atoi(scan_string);


    /*Port range and Protocol_type*/
    if ((*ipsec_config).child_cfg[i].port_id != 0) {
      /*Port range*/
      QCMAP_PRINTF_TAKE_INPUT("   Do you want to input end_port_id:(1.yes 0.no) ");
      fgets(scan_string, sizeof(scan_string), stdin);
      if (atoi(scan_string) == 1) {
        (*ipsec_config).child_cfg[i].port_range = true;
        QCMAP_PRINTF_TAKE_INPUT("   Please input end_port_id: ");
        fgets(scan_string, sizeof(scan_string), stdin);
        (*ipsec_config).child_cfg[i].end_port_id = atoi(scan_string);
      }else{
        (*ipsec_config).child_cfg[i].port_range = false;
        (*ipsec_config).child_cfg[i].end_port_id = 0;
      }

      /*Protocol_type*/
      QCMAP_PRINTF_TAKE_INPUT("   Please input protocol type:(1.TCP 2.UDP) ");
      fgets(scan_string, sizeof(scan_string), stdin);
      (*ipsec_config).child_cfg[i].protocol_type = atoi(scan_string);
    }

    /*Local subnets*/
    QCMAP_PRINTF_TAKE_INPUT("   Please input Local Address : ");
    fgets(scan_string, sizeof(scan_string), stdin);
    scan_string[strlen(scan_string)-1]='\0';
    strlcpy((*ipsec_config).child_cfg[i].local_addr,scan_string, sizeof(scan_string));

    /*Remote subnets*/
    QCMAP_PRINTF_TAKE_INPUT("   Please input Remote Address : ");
    fgets(scan_string, sizeof(scan_string), stdin);
    scan_string[strlen(scan_string)-1]='\0';
    strlcpy((*ipsec_config).child_cfg[i].remote_addr,scan_string, sizeof(scan_string));

    /*Read in hw_offload*/
    QCMAP_PRINTF_TAKE_INPUT("   Do you want to enable hw_offload:(1.YES 0.NO) ");
    fgets(scan_string, sizeof(scan_string), stdin);
    (*ipsec_config).child_cfg[i].hw_offload=atoi(scan_string);

    /*Read in trap_action*/
    QCMAP_PRINTF_TAKE_INPUT("   Do you want to set start action as trap:(1.YES 0.NO) ");
    fgets(scan_string, sizeof(scan_string), stdin);
    (*ipsec_config).child_cfg[i].trap_action=atoi(scan_string);

    /*Rekey_interval_hrs*/
    QCMAP_PRINTF_TAKE_INPUT("   Please input rekey time for Child tunnel:");
    fgets(scan_string, sizeof(scan_string), stdin);
    (*ipsec_config).child_cfg[i].rekey_interval_hrs = atoi(scan_string);

  }
  return true;

}

void ipsecConfig(int ipsecOpt)
{
  char scan_string[QCMAP_MSGR_MAX_FILE_PATH_LEN];
  qmi_error_type_v01 qmi_err_num;

  /* Initialize QMI Error Number. */
  qmi_err_num = QMI_ERR_NONE_V01;
  memset(scan_string, 0, QCMAP_MSGR_MAX_FILE_PATH_LEN);

  /* IPsec Configuration options */
  switch(ipsecOpt)
  {
    /*Set IPsec tunnel*/
    case 1:
    {
      qcmap_ipsec_config_t ipsec_config;
      int topology_list_size = sizeof(IPSEC_topology_list)/sizeof(IPSEC_topology_list[0]);
      int topology_input = 0;

      memset(&ipsec_config,0,sizeof(qcmap_ipsec_config_t));

      printf("List of IPsec topology supported:\n");
      for (int i = 0; i < topology_list_size; i++)
      {
        printf("%s\n", IPSEC_topology_list[i]);
      }
      QCMAP_PRINTF_TAKE_INPUT("Please enter the choice: ");
      fgets(scan_string, sizeof(scan_string), stdin);
      topology_input = atoi(scan_string);
      printf("topology choice: %d\n",topology_input);

      switch (topology_input)
      {
        /* IPsec Host-to-Host */
        case 1:
        {
          ipsec_config.topology = QCMAP_IPSEC_HOST_TO_HOST_TOPOLOGY;

          setHostToHostConfig(&ipsec_config);

          /*Need child info for host-to-host*/
          ipsec_config.child_entries = 1;
          QCMAP_PRINTF_TAKE_INPUT("   Please input the CHILD_identifier of IPsec tunnel:");
          fgets(scan_string, sizeof(scan_string), stdin);
          scan_string[strlen(scan_string)-1]='\0';
          strlcpy(ipsec_config.child_cfg[0].child_identifier,scan_string, sizeof(scan_string));

          QCMAP_PRINTF_TAKE_INPUT("   Do you want to enable hw_offload:(1.YES 0.NO) ");
          fgets(scan_string, sizeof(scan_string), stdin);
          ipsec_config.child_cfg[0].hw_offload=atoi(scan_string);

          QCMAP_PRINTF_TAKE_INPUT("   Do you want to set start action as trap:(1.YES 0.NO) ");
          fgets(scan_string, sizeof(scan_string), stdin);
          ipsec_config.child_cfg[0].trap_action=atoi(scan_string);

          QCMAP_PRINTF_TAKE_INPUT("   Please input rekey time for Child tunnel:");
          fgets(scan_string, sizeof(scan_string), stdin);
          ipsec_config.child_cfg[0].rekey_interval_hrs = atoi(scan_string);

          if (QcMapClient->SetIPsecTunnelInfo(&ipsec_config, &qmi_err_num))
          {
            printf("\n SetIPsecConfig successful\n");
          }
          else if(qmi_err_num == QMI_ERR_INVALID_HANDLE_V01)
          {
            printf("\nMobileAP is not enabled\n");
            break;
          }
          else
          {
            printf("\n SetIPsecConfig set fails\n");
          }
          break;
        }

        /* IPsec Site_to_Site without NAT */
        case 2:
        {
          ipsec_config.topology = QCMAP_IPSEC_SITE_TO_SITE_WITHOUT_NAT_TOPOLOGY;

          setHostToHostConfig(&ipsec_config);
          setSiteToSiteWithoutNATConfig(&ipsec_config);

          if (QcMapClient->SetIPsecTunnelInfo(&ipsec_config, &qmi_err_num))
          {
            printf("\n SetIPsecConfig successful\n");
          }
          else if(qmi_err_num == QMI_ERR_INVALID_HANDLE_V01)
          {
            printf("\nMobileAP is not enabled\n");
            break;
          }
          else
          {
            printf("\n SetIPsecConfig set fails\n");
          }

          break;
        }

        /* IPsec Host-to-Host and IPsec Site_to_Site without NAT */
        case 3:
        {
          ipsec_config.topology = QCMAP_IPSEC_HOST_TO_HOST_AND_SITE_TO_SITE_WITHOUT_NAT_TOPOLOGY;

          setHostToHostConfig(&ipsec_config);
          setSiteToSiteWithoutNATConfig(&ipsec_config);

          /*Need child info for host-to-host*/
          ipsec_config.child_entries = ipsec_config.child_entries+1;
          QCMAP_PRINTF_TAKE_INPUT("   Please input the CHILD_identifier of IPsec Host-to-Host tunnel:");
          fgets(scan_string, sizeof(scan_string), stdin);
          scan_string[strlen(scan_string)-1]='\0';
          strlcpy(ipsec_config.child_cfg[ipsec_config.child_entries-1].child_identifier,scan_string, sizeof(scan_string));

          QCMAP_PRINTF_TAKE_INPUT("   Do you want to enable hw_offload:(1.YES 0.NO) ");
          fgets(scan_string, sizeof(scan_string), stdin);
          ipsec_config.child_cfg[ipsec_config.child_entries-1].hw_offload=atoi(scan_string);

          QCMAP_PRINTF_TAKE_INPUT("   Do you want to set start action as trap:(1.YES 0.NO) ");
          fgets(scan_string, sizeof(scan_string), stdin);
          ipsec_config.child_cfg[ipsec_config.child_entries-1].trap_action=atoi(scan_string);

          QCMAP_PRINTF_TAKE_INPUT("   Please input rekey time for Child tunnel:");
          fgets(scan_string, sizeof(scan_string), stdin);
          ipsec_config.child_cfg[ipsec_config.child_entries-1].rekey_interval_hrs = atoi(scan_string);

          if (QcMapClient->SetIPsecTunnelInfo(&ipsec_config, &qmi_err_num))
          {
            printf("\n SetIPsecConfig successful\n");
          }
          else if(qmi_err_num == QMI_ERR_INVALID_HANDLE_V01)
          {
            printf("\nMobileAP is not enabled\n");
            break;
          }
          else
          {
            printf("\n SetIPsecConfig set fails\n");
          }
          break;
        }

        default:
        {
          ipsec_config.topology = 0;
          break;
        }
      }
      break;
    }


    /*Activate IPsec tunnel*/
    case 2:
    {
      char ike_identifier[QCMAP_MAX_STRING_LEN]={0};
      char child_identifier[QCMAP_MAX_STRING_LEN]={0};

      QCMAP_PRINTF_TAKE_INPUT("Please input ike_indentifier:\n");
      fgets(scan_string, sizeof(scan_string), stdin);
      scan_string[strlen(scan_string)-1]='\0';
      strlcpy(ike_identifier, scan_string, sizeof(scan_string));

      QCMAP_PRINTF_TAKE_INPUT("Do you want to activate specific child? (1.YES, 2.NO)\n");
      fgets(scan_string, sizeof(scan_string), stdin);
      if (atoi(scan_string)==1) {
      QCMAP_PRINTF_TAKE_INPUT("Please input child_indentifier:\n");
      fgets(scan_string, sizeof(scan_string), stdin);
      scan_string[strlen(scan_string)-1]='\0';
      strlcpy(child_identifier, scan_string, sizeof(scan_string));
      }else if(atoi(scan_string)!=2){
        printf("Invalid input");
        break;
      }

      if (QcMapClient->ActivateIPsecTunnelInfo(ike_identifier, child_identifier, &qmi_err_num)){
        printf("\n Check IPsec tunnel status info to see if IPsec tunnel established \n");
      }
      else{
        printf("\n ActivateIPsecConfig fails\n");
        break;
      }
      break;
    }

    /*Delete IPsec tunnel*/
    case 3:
    {
      char ike_identifier[QCMAP_MAX_STRING_LEN]={0};
      char child_identifier[QCMAP_MAX_STRING_LEN]={0};

      QCMAP_PRINTF_TAKE_INPUT("Please input ike_indentifier:\n");
      fgets(scan_string, sizeof(scan_string), stdin);
      scan_string[strlen(scan_string)-1]='\0';
      strlcpy(ike_identifier, scan_string, sizeof(scan_string));

      QCMAP_PRINTF_TAKE_INPUT("Do you want to delete specific child? (1.YES, 2.NO)\n");
      fgets(scan_string, sizeof(scan_string), stdin);
      if (atoi(scan_string)==1)
      {
        QCMAP_PRINTF_TAKE_INPUT("Please input child_indentifier:\n");
        fgets(scan_string, sizeof(scan_string), stdin);
        scan_string[strlen(scan_string)-1]='\0';
        strlcpy(child_identifier, scan_string, sizeof(scan_string));
      }else if(atoi(scan_string)!=2)
      {
        printf("Invalid input");
        break;
      }

      if (QcMapClient->DeleteIPsecTunnel(ike_identifier, child_identifier, &qmi_err_num))
      {
        printf("\n DeleteIPsecConfig successful\n");
      }
      else
      {
        printf("\n DeleteIPsecConfig fails\n");
        break;
      }
      break;
    }

    /*Get IPsec tunnel*/
    case 4:
    {
      char ike_identifier[QCMAP_MAX_STRING_LEN]={0};
      char child_identifier[QCMAP_MAX_STRING_LEN]={0};
      char result[QCMAP_MAX_STRING_LEN]={0};
      char command[QCMAP_MAX_STRING_LEN]={0};
      qcmap_ipsec_config_t ipsec_config;
      /*for finding child_identifier*/
      uint8_t flag=0;

      memset(&ipsec_config,0,sizeof(qcmap_ipsec_config_t));

      QCMAP_PRINTF_TAKE_INPUT("Please input ike_indentifier:\n");
      fgets(scan_string, sizeof(scan_string), stdin);
      scan_string[strlen(scan_string)-1]='\0';
      strlcpy(ike_identifier, scan_string, sizeof(scan_string));

      QCMAP_PRINTF_TAKE_INPUT("Do you want to get specific child? (1.YES, 2.NO)\n");
      fgets(scan_string, sizeof(scan_string), stdin);
      if (atoi(scan_string)==1)
      {
        QCMAP_PRINTF_TAKE_INPUT("Please input child_indentifier:\n");
        fgets(scan_string, sizeof(scan_string), stdin);
        scan_string[strlen(scan_string)-1]='\0';
        strlcpy(child_identifier, scan_string, sizeof(scan_string));
      }else if(atoi(scan_string)!=2)
      {
        printf("Invalid input");
        break;
      }

      if (QcMapClient->GetIPsecTunnelInfo(ike_identifier, child_identifier, &ipsec_config, &qmi_err_num))
      {
        printf("\n GetIPsecConfig successful\n");
      }
      else
      {
        printf("\n GetIPsecConfig fails\n");
        break;
      }

      /* Print IPsec Configuration. */
      printf("\n IPsec Configuration:\n");
      printf("\nProfile id: %d\n", ipsec_config.profile_id);
      if (ipsec_config.topology == QCMAP_IPSEC_HOST_TO_HOST_TOPOLOGY) {
        printf("Topology: QCMAP_IPSEC_HOST_TO_HOST_TOPOLOGY\n");
      }else if(ipsec_config.topology == QCMAP_IPSEC_SITE_TO_SITE_WITHOUT_NAT_TOPOLOGY){
        printf("Topology: QCMAP_IPSEC_SITE_TO_SITE_WITHOUT_NAT_TOPOLOGY\n");
      }else if(ipsec_config.topology == QCMAP_IPSEC_HOST_TO_HOST_AND_SITE_TO_SITE_WITHOUT_NAT_TOPOLOGY){
        printf("Topology: QCMAP_IPSEC_HOST_TO_HOST_AND_SITE_TO_SITE_WITHOUT_NAT_TOPOLOGY\n");
      }else{
        printf("QCMAP_IPSEC_INVALID_TOPOLOGY");
      }

      printf("\n--------IKE--------\n");
      printf("IKE identifier: %s\n", ike_identifier);

      /*Get enable option*/
      snprintf(command, QCMAP_MAX_STRING_LEN, " %s ipsec.%s.enabled", UCI_GET_COMMAND, ike_identifier);
      if (ExecuteSystemCmd((const char *)command, result, sizeof(result))) {
        printf("Enabled: %d\n",atoi(result));
      }else{
        printf("Failed to get enable status\n");
        LOG_MSG_ERROR("Failed to get enable status ",0,0,0);
      }

      if (ipsec_config.ike_cfg.tunnel_type == QCMAP_IPSEC_V4_ESP_TUNNEL_MODE_TUNNEL_TYPE) {
        printf("Tunnel type: QCMAP_IPSEC_V4_ESP_TUNNEL_MODE_TUNNEL_TYPE\n");
      }else if(ipsec_config.ike_cfg.tunnel_type == QCMAP_IPSEC_V6_ESP_TUNNEL_MODE_TUNNEL_TYPE){
        printf("Tunnel type: QCMAP_IPSEC_V6_ESP_TUNNEL_MODE_TUNNEL_TYPE\n");
      }else{
        printf("QCMAP_IPSEC_INVALID_TUNNEL_TYPE");
      }
      printf("Remote EP address: %s\n", ipsec_config.ike_cfg.remote_ep_addr);
      printf("Authentication Type: %d (1.PSK,2.X509)\n", ipsec_config.ike_cfg.auth_type);
      if (ipsec_config.ike_cfg.auth_type == QCMAP_IPSEC_X509_AUTHENTICATION_TYPE) {
        printf("Local identifier: %s\n", ipsec_config.ike_cfg.local_identifier);
        printf("Remote identifier: %s\n", ipsec_config.ike_cfg.remote_identifier);
      }
      printf("IKE rekey time: %d\n", ipsec_config.ike_cfg.rekey_interval_hrs);
      printf("Child entries: %d\n", ipsec_config.child_entries);


      if (child_identifier[0] != '\0') {
        for (int i = 0; i < ipsec_config.child_entries; i++) {
          /*Get specific child*/
          if (strcmp(ipsec_config.child_cfg[i].child_identifier, child_identifier)==0)
          {
            printf("\n--------Child %s ----------\n", child_identifier);
            printf("Child Identifier: %s\n", ipsec_config.child_cfg[i].child_identifier);
            /*Get enable option*/
            snprintf(command, QCMAP_MAX_STRING_LEN, " %s ipsec.%s.enabled", UCI_GET_COMMAND, child_identifier);
            if (ExecuteSystemCmd((const char *)command, result, sizeof(result))) {
              printf("Enabled: %d\n",atoi(result));
            }else{
              printf("Failed to get enable status\n");
              LOG_MSG_ERROR("Failed to get enable status ",0,0,0);
            }
            printf("Child hw_offload option: %d\n", ipsec_config.child_cfg[i].hw_offload);
            printf("Child trap_action option: %d\n", ipsec_config.child_cfg[i].trap_action);
            printf("Child rekey time: %d\n", ipsec_config.child_cfg[i].rekey_interval_hrs);
            if (strlen(ipsec_config.child_cfg[i].local_addr)!=0) {
              printf("Child local subnet: %s\n", ipsec_config.child_cfg[i].local_addr);
              printf("Child remote subnet: %s\n", ipsec_config.child_cfg[i].remote_addr);
            }
            break;
          }
        }
        break;
      }

      for (int i = 0; i < ipsec_config.child_entries; i++) {
        printf("\n--------Child %d ----------\n", i+1);
        printf("Child Identifier: %s\n", ipsec_config.child_cfg[i].child_identifier);
        /*Get enable option*/
        snprintf(command, QCMAP_MAX_STRING_LEN, " %s ipsec.%s.enabled", UCI_GET_COMMAND, ipsec_config.child_cfg[i].child_identifier);
        if (ExecuteSystemCmd((const char *)command, result, sizeof(result))) {
          printf("Enabled: %d\n",atoi(result));
        }else{
          printf("Failed to get enable status\n");
          LOG_MSG_ERROR("Failed to get enable status ",0,0,0);
        }
        printf("Child hw_offload option: %d\n", ipsec_config.child_cfg[i].hw_offload);
        printf("Child trap_action option: %d\n", ipsec_config.child_cfg[i].trap_action);
        printf("Child rekey time: %d\n", ipsec_config.child_cfg[i].rekey_interval_hrs);
        if (strlen(ipsec_config.child_cfg[i].local_addr)!=0) {
          printf("Child local subnet: %s\n", ipsec_config.child_cfg[i].local_addr);
          printf("Child remote subnet: %s\n", ipsec_config.child_cfg[i].remote_addr);
        }
      }
      break;
    }

    /*Get IPsec tunnel state*/
    case 5:
    {
      char ike_identifier[QCMAP_MAX_STRING_LEN]={0};
      char child_identifier[QCMAP_MAX_STRING_LEN]={0};
      char result[QCMAP_MAX_STRING_LEN]={0};
      char command[QCMAP_MAX_STRING_LEN]={0};

      qcmap_ipsec_tunnel_state_info_t state_info;
      /*for finding child_identifier*/
      uint8_t flag=0;

      memset(&state_info,0,sizeof(qcmap_ipsec_tunnel_state_info_t));

      QCMAP_PRINTF_TAKE_INPUT("Please input ike_indentifier:\n");
      fgets(scan_string, sizeof(scan_string), stdin);
      scan_string[strlen(scan_string)-1]='\0';
      strlcpy(ike_identifier, scan_string, sizeof(scan_string));

      QCMAP_PRINTF_TAKE_INPUT("Do you want to get specific child? (1.YES, 2.NO)\n");
      fgets(scan_string, sizeof(scan_string), stdin);
      if (atoi(scan_string)==1) {
        QCMAP_PRINTF_TAKE_INPUT("Please input child_indentifier:\n");
        fgets(scan_string, sizeof(scan_string), stdin);
        scan_string[strlen(scan_string)-1]='\0';
        strlcpy(child_identifier, scan_string, sizeof(scan_string));
      }else if(atoi(scan_string)!=2){
        printf("Invalid input");
        break;
      }

      if (QcMapClient->GetIPsecTunnelStateInfo(ike_identifier, child_identifier, &state_info, &qmi_err_num))
      {
        printf("\n GetIPsecConfig successful\n");
      }
      else
      {
        printf("\n GetIPsecConfig fails\n");
        break;
      }


      /* Print IPsec Status. */
      printf("--------------IPSEC Tunnel Status----------\n");
      if (child_identifier[0] != '\0') {
      /*If child_identifier is provided*/
        for (int i = 0; i < state_info.tunnel_entries; i++) {
          if (strcmp(state_info.tunnel_status[i].child_identifier, child_identifier)==0) {
            printf("\n--------Child %s ----------\n", child_identifier);
            printf("Child Identifier: %s\n", state_info.tunnel_status[i].child_identifier);
            /*Get enable option*/
            printf("Enabled: %d\n", state_info.tunnel_status[i].enable_status);

            if (state_info.tunnel_status[i].state==0) {
              printf("IPsec tunnel state: DISCONNECTED\n");
            } else if (state_info.tunnel_status[i].state==2) {
              printf("IPsec tunnel state: CONNECTED\n");
            }else if (state_info.tunnel_status[i].state==1) {
              printf("IPsec tunnel state: CONNECTING\n");
            }
            break;
          }
        }
        break;
      }

      for (int i = 0; i < state_info.tunnel_entries; i++) {
        printf("\n--------Child %d ----------\n", i+1);
        printf("Child Identifier: %s\n", state_info.tunnel_status[i].child_identifier);
        /*Get enable option*/
        printf("Enabled: %d\n", state_info.tunnel_status[i].enable_status);

        if (state_info.tunnel_status[i].state == QCMAP_IPSEC_TUNNEL_DISCONNECTED) {
          printf("IPsec tunnel state: DISCONNECTED\n");
        } else if (state_info.tunnel_status[i].state == QCMAP_IPSEC_TUNNEL_CONNECTED) {
          printf("IPsec tunnel state: CONNECTED\n");
        }else if (state_info.tunnel_status[i].state == QCMAP_IPSEC_TUNNEL_INPROGRESS) {
          printf("IPsec tunnel state: CONNECTING\n");
        }
      }
    }

    default:
    {
      LOG_MSG_ERROR("Invalid Option", 0, 0, 0);
      break;
    }
  }
}


/*===========================================================================
  FUNCTION main
  ===========================================================================
  @brief
    main funcion

  @input
    argc
    argv

  @return
    0 - success
    exit - fail

  @dependencies
    usr to provide input

  @sideefects
    None
  =========================================================================*/
int main(int argc, char **argv)
{
  int opt = 0, mobileApOpt = 0, lanOpt = 0, natAlgOpt = 0, wlanOpt = 0, firewallOpt = 0,
  backhaulOpt = 0, backhaulCommOpt = 0, backhaulWWANOpt = 0, tetheOpt = 0, medServOpt = 0,
  gsbOpt = 0, mtpeOpt = 0, ipsecOpt = 0, p_error;
  char scan_string[QCMAP_MSGR_MAX_FILE_PATH_LEN];
  uint8 mac_addr_int[QCMAP_MSGR_MAC_ADDR_LEN_V01]; /*byte array of mac address*/
  qmi_error_type_v01  qmi_err_num;
  int array_size = 0;
  int result = -1;
  /* Register the sighandlers, so the app may be shutdown with a
     kill command.*/
  signal(SIGTERM, sighandler);
  signal(SIGINT, sighandler);
  signal(SIGKILL, sighandler);
  signal(SIGHUP, sighandler);

#ifdef FEATURE_EXTERNAL_AP
  QcMapClient = new QCMAP_Client( qcmap_msgr_qmi_qcmap_ind, QCMAP_FUSION_ARCH_V01);
#else
  QcMapClient = new QCMAP_Client( qcmap_msgr_qmi_qcmap_ind );
#endif

  if (QcMapClient->IsReady() == false)
  {
    printf("\nCouldn't setup QcMapClient..exiting");
    sighandler(SIGTERM);
    exit(1);
  }

  while (TRUE)
  {

    /* Display menu of options. */
    printf("\nPlease select an option to test from the items listed below.\n\n");
    array_size = sizeof(options_list)/sizeof(options_list[0]);
    for (int i=0; i<array_size; i++)
    {
      printf("%s\n",options_list[i]);
    }
    printf("Option > ");
    fflush(stdout);

    /* Initialize QMI Error Number. */
    qmi_err_num = QMI_ERR_NONE_V01;

    /* Read the option from the standard input. */
    if (fgets(scan_string, sizeof(scan_string), stdin) == NULL)
      continue;

    /* Convert the option to an integer, and switch on the option entered. */
    opt = atoi(scan_string);

    /* Display submenu's according to main menu option selected above. */
    switch (opt)
    {
      /* A. MobileAP CONFIGURATION SUBMENU */
      case 1:
        array_size = sizeof(mobileAp_configuration_list)/sizeof(mobileAp_configuration_list[0]);
        printf("\n");
        for (int i=0; i<array_size; i++)
        {
          printf("%s\n",mobileAp_configuration_list[i]);
        }
        fflush(stdout);
        if(fgets(scan_string, sizeof(scan_string), stdin) == NULL)
          continue;

        mobileApOpt = atoi(scan_string);

        /* mobileApConfig() for mobileApConfig options */
        mobileApConfig(mobileApOpt);
      break;

      /* B. LAN CONFIGURATION SUBMENU */
      case 2:
        array_size = sizeof(lan_configuration_list)/sizeof(lan_configuration_list[0]);
        for (int i=0; i<array_size; i++)
        {
          printf("%s\n",lan_configuration_list[i]);
        }
        fflush(stdout);
        if (fgets(scan_string, sizeof(scan_string), stdin) == NULL)
          continue;

          lanOpt = atoi(scan_string);

        /* lanConfig() for LAN options */
        lanConfig(lanOpt);
      break;


      /* C. NAT/ALG/VPN CONFIGURATION SUBMENU */
      case 3:
        array_size = sizeof(nat_alg_vpn_configuration_list)/sizeof(nat_alg_vpn_configuration_list[0]);
        for (int i=0; i<array_size; i++)
        {
          printf("%s\n",nat_alg_vpn_configuration_list[i]);
        }
        fflush(stdout);
        if (fgets(scan_string, sizeof(scan_string), stdin) == NULL)
          continue;

        natAlgOpt = atoi(scan_string);

        /* nat_alg_vpn() for respective options */
        nat_alg_vpn_config(natAlgOpt);
      break;


      /* D. WLAN CONFIGURATION SUBMENU */
      case 4:
       array_size = sizeof(wlan_configuration_list)/sizeof(wlan_configuration_list[0]);
       for (int i=0; i<array_size; i++)
       {
         printf("%s\n",wlan_configuration_list[i]);
       }
       fflush(stdout);
       if (fgets(scan_string, sizeof(scan_string), stdin) == NULL)
         continue;

       wlanOpt = atoi(scan_string);

         /* WLAN Config options*/
         wlanConfig( wlanOpt );
      break;


      /* E. FIREWALL CONFIGURATION SUBMENU */
      case 5:
        array_size = sizeof(firewall_configuration_list)/sizeof(firewall_configuration_list[0]);
        for (int i=0; i<array_size; i++)
        {
          printf("%s\n",firewall_configuration_list[i]);
        }
        fflush(stdout);
        if (fgets(scan_string, sizeof(scan_string), stdin) == NULL)
          continue;

        firewallOpt = atoi(scan_string);

        /*Firewall Config options */
        firewallConfig( firewallOpt );
        break;


       /* F. BACKHAUL CONFIGURATION SUBMENU */
      case 6:
        array_size = sizeof(backhaul_configuration_list)/sizeof(backhaul_configuration_list[0]);
        for (int i=0; i<array_size; i++)
        {
          printf("%s\n",backhaul_configuration_list[i]);
        }
        fflush(stdout);
        if (fgets(scan_string, sizeof(scan_string), stdin) == NULL)
          continue;

        backhaulOpt = atoi(scan_string);

        /* Backhual configuration options */
        switch(backhaulOpt)
        {
          /* $ BackhualCOMMON Config SUBMENU */
          case 1:
          {
            array_size = sizeof(backhaul_common_configuration_list)/sizeof(backhaul_common_configuration_list[0]);
            for (int i=0; i<array_size; i++)
            {
              printf("%s\n",backhaul_common_configuration_list[i]);
            }
            fflush(stdout);
            if (fgets(scan_string, sizeof(scan_string), stdin) == NULL)
              continue;

            backhaulCommOpt = atoi(scan_string);
            backhaulCommConfig( backhaulCommOpt );
          }
          break;

          /* $ BackhaulWWAN Config SUBMENU */
          case 2:
          {
            array_size = sizeof(backhaul_wwan_configuration_list)/sizeof(backhaul_wwan_configuration_list[0]);
            for (int i=0; i<array_size; i++)
            {
              printf("%s\n",backhaul_wwan_configuration_list[i]);
            }
            fflush(stdout);
            if (fgets(scan_string, sizeof(scan_string), stdin) == NULL)
              continue;

            backhaulWWANOpt = atoi(scan_string);
            backhaulWWANConfig( backhaulWWANOpt );
          }
          break;

          default :
          {
           printf("Invalid response %d\n", backhaulOpt);
          }
          break;
        }
        break;


        /* G. TETHERING CONFIGURATION SUBMENU */
      case 7:
        array_size = sizeof(tethering_configuration_list)/sizeof(tethering_configuration_list[0]);
        for (int i=0; i<array_size; i++)
        {
          printf("%s\n",tethering_configuration_list[i]);
        }
        fflush(stdout);
        if (fgets(scan_string, sizeof(scan_string), stdin) == NULL)
          continue;

        tetheOpt = atoi(scan_string);

        tetheringConfig(tetheOpt);
        break;

         /* H. MEDIA SERVICE CONFIGURATION SUBMENU */
      case 8:
        array_size = sizeof(media_service_configuration_list)/sizeof(media_service_configuration_list[0]);
        for (int i=0; i<array_size; i++)
        {
          printf("%s\n",media_service_configuration_list[i]);
        }
        fflush(stdout);
        if (fgets(scan_string, sizeof(scan_string), stdin) == NULL)
          continue;

        medServOpt = atoi(scan_string);

        mediaServiceConfig(medServOpt);
      break;

       /* I. GENERIC SOFTWARE BRIDGE(GSB) SUBMENU */
      case 9:
        array_size = sizeof(software_bridge_config_list)/sizeof(software_bridge_config_list[0]);
        for (int i=0; i<array_size; i++)
        {
          printf("%s\n",software_bridge_config_list[i]);
        }
        fflush(stdout);
        if (fgets(scan_string, sizeof(scan_string), stdin) == NULL)
          continue;

        gsbOpt = atoi(scan_string);

        gsbConfig(gsbOpt);
        break;

       /* 10. V2X Service Config Submenu */
#if 0 /* Disable CV2X feature */
      case 10:
      {
        int v2xOpt;

        array_size = sizeof(v2x_config_list)/sizeof(v2x_config_list[0]);
        for (int i=0; i<array_size; i++)
        {
          printf("%s\n",v2x_config_list[i]);
        }

        if (fgets(scan_string, sizeof(scan_string), stdin) == NULL)
          continue;

        v2xOpt = atoi(scan_string);

        v2xServiceConfig(v2xOpt);
      }
      break;
#endif

      case 11: //mtpe_configuration_list
      {
        array_size = sizeof(mtpe_configuration_list)/sizeof(mtpe_configuration_list[0]);
        for (int i=0; i<array_size; i++)
        {
          printf("%s\n",mtpe_configuration_list[i]);
        }
        fflush(stdout);
        if (fgets(scan_string, sizeof(scan_string), stdin) == NULL)
          continue;

        mtpeOpt = atoi(scan_string);


        if(mtpe_history_entries == NULL) {
          // note: dyn. memory must be freed on QCMAP exit (SIGINT, etc...)
          mtpe_history_entries = malloc(sizeof(mtpe_history_entry) * 100);
        }

        mtpeConfig(mtpeOpt);
        break;
      }
      break;

      /* CLI Debug Options */
      case CLI_DEBUG_OPTION:
      {
        printf(" Supported Dev Test Options for QCMAP CLI \n");
        printf(" 501 ------------- EnableSTAOnlyMode \n");
        printf(" 502 ------------- DisableSTAOnlyMode \n");
        printf(" 503 ------------- RegisterWLANStatusIND \n");
      }
      break;

    /* 12. IPsec Config Submenu */
      case 12:
      {
        array_size = sizeof(IPSEC_configuration_list)/sizeof(IPSEC_configuration_list[0]);
        for (int i=0; i<array_size; i++)
        {
          printf("%s\n",IPSEC_configuration_list[i]);
        }
        fflush(stdout);
        if (fgets(scan_string, sizeof(scan_string), stdin) == NULL)
          continue;

        ipsecOpt = atoi(scan_string);

        ipsecConfig(ipsecOpt);
        break;
      }

#if 0
      /* Enable STA Only Mode */
      case ENABLE_STA_ONLY_DEBUG_MODE:
      {
        if (!QcMapClient->EnableSTAMode(&qmi_err_num))
        {
          printf("STA-Only Mode couldnt be enabled. ERR: 0x%x.\n ", qmi_err_num);
        }
        else
        {
          printf("STA-Only Mode Enabled Successufully.\n ");
        }
        break;
      }

        /* Disable STA Only Mode */
      case DISABLE_STA_ONLY_DEBUG_MODE:
      {
        if (!QcMapClient->DisableSTAMode(&qmi_err_num))
        {
          printf("STA-Only Mode couldnt be Disabled. ERR: 0x%x.\n ", qmi_err_num);
        }
        else
        {
          printf("STA-Only Mode Disabled Successufully.\n ");
        }
        break;
      }

        /* Register for WLAN Status IND */
      case REGISTER_FOR_WLAN_STATUS_IND:
      {
        QCMAP_PRINTF_TAKE_INPUT("Register/De-register for WLAN Status (1-Regsiter/0-De-register): ");
        fgets(scan_string, sizeof(scan_string), stdin);
        if (!QcMapClient->RegisterForWLANStatusIND(&qmi_err_num, atoi(scan_string)))
        {
          printf("Registeration failed for WLAN Status IND.ERR 0x%x Input %d\n",
                   qmi_err_num, atoi(scan_string));
        }
        else
        {
          printf("Registration Successufully for WLAN Status IND.\n ");
        }
        break;
      }
#endif

      default :
      {
        printf("Invalid response %d\n", opt);
      }
      break;
    }
  }
  return 0;
}
