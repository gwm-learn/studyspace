#ifndef __PRODUCE_UTIL_H__
#define __PRODUCE_UTIL_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/wireless.h>
#include <sys/types.h>

#define FACTORY_MODE_FILE           "/var/factoryMode"
#define IS_FACTORY_MODE()           (access(FACTORY_MODE_FILE, F_OK) == 0)
#define RESTOREDEFAULT_ING_FILE     "/var/restoredefaulting"
#define IS_RESTOREDEFAULT_ING()     (access(RESTOREDEFAULT_ING_FILE, F_OK) == 0)
#define LED_FACTORY_MODE_FILE       "/var/ledFactoryMode"

#define PLAN_MACs_FILE              "/var/.c.a.m"
#define USES_MACs_FILE              "/var/.c.a.m.u"
#define CONFIG_CHECK_TMP_FILE       "/var/produce.configcheck.tmp"
#define BR_MAC_VERIFY_FILE          "/sys/class/net/br-lan/address"
#define TMP_PASS_FILE               "/tmp/_mm_pass"
#define MIN_SIZE_OF_MACs_FILE       (16 * 17)

#define DEFAULT_USERNAME            "admin"
#define FORMAT_PASS_CMD             "gopass --cipher md5 --password %s " ME_GOAHEAD_REALM " %s %s > " TMP_PASS_FILE

#define WIFI_RADIO_5G_INDEX         0
#define WIFI_RADIO_2G_INDEX         1

#define PARAM_FLAG_HAS_SUBCOMMAND   3

#define IS_EMPTY_STRING(s)          ((s == NULL) || (s[0] == 0))
#define MATCH_UNSET_STR(v)          ((v[0] == '\0') || (v[0] == '\"' && v[1] == '\"' && v[2] == '\0') || (v[0] == '\'' && v[1] == '\'' && v[2] == '\0'))

#ifdef CUS_PARAMS_MANUFACTURER
#define CUSTOMER_MANUFACTURER CUS_PARAMS_MANUFACTURER
#else
#define CUSTOMER_MANUFACTURER "Broadlink"
#endif
#ifdef CUS_PARAMS_MODULENAME
#define CUSTOMER_PRODUCT_CLASS CUS_PARAMS_MODULENAME
#else
#define CUSTOMER_PRODUCT_CLASS "M2000"
#endif
#ifdef CUS_PARAMS_HARDVERSION
#define CUSTOMER_HW_VER CUS_PARAMS_HARDVERSION
#else
#define CUSTOMER_HW_VER "V1.0"
#endif
#ifdef CUS_PARAMS_SOFTVERSION
#define CUSTOMER_SW_VER CUS_PARAMS_SOFTVERSION
#else
#define CUSTOMER_SW_VER "V1.01.01"
#endif

#define CMD_READ_PRODUCE_TOOL "fw_printparams"
#define CMD_WRITE_PRODUCE_TOOL "fw_setparams"
#define PRODUCE_TMP_FILE_FORMAT "/var/pp_%s"
#define PRODUCE_KEY_VALUE_FORMAT "%s="

#define PRODUCE_MACADDR_KEY "bl_uni_macaddr"
#define PRODUCE_LISENCE_KEY "bl_uni_lisence"
#define PRODUCE_SERIALN_KEY "bl_uni_serialn"
#define PRODUCE_2GSSIDNAME_KEY "bl_2gssid_name"
#define PRODUCE_2GSSIDPASS_KEY "bl_2gssid_pass"
#define PRODUCE_5GSSIDNAME_KEY "bl_5gssid_name"
#define PRODUCE_5GSSIDPASS_KEY "bl_5gssid_pass"
#define PRODUCE_2GWPSPINCO_KEY "bl_2gwps_pin"
#define PRODUCE_ADMINPASS_KEY "bl_admin_pass"
#define PRODUCE_PRODUCTCLASS_KEY "bl_product_class"
#define PRODUCE_COUNTRYCODE_KEY "bl_coun_code"
#define PRODUCE_FOTAURL_KEY "bl_uni_fotaurl"
#define PRODUCE_RESETFLAG_KEY "bl_reset_flag"

int get_produce_param(char* key, char* tmpFileName, char* value);
int set_produce_param(char* key, char* tmpFileName, char* value);
int write_buf_to_file(const char* fileName, char* buf, unsigned int iLen);
int read_file_to_buf(const char* fileName, char* buf, unsigned int iMaxLen);
char* read_file_to_alloc_buf(const char* fullfilepath);
int is_valid_mac(char* mac);
void change_to_lowercase(char* mac);
void change_to_upcase(char* mac);
void do_next_hex_char(char* o_ch);
void conver_mac_addr_to_sp1(char* baseMacAddrStr, char* finaMacAddrStr);
int wps_pin_validate_checksum(unsigned long int PIN);
int hex_char_to_int(char c);
char int_to_hex_char(int n);

#endif
