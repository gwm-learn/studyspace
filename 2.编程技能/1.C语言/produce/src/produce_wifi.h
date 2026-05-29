#ifndef __PRODUCE_WIFI_H__
#define __PRODUCE_WIFI_H__

typedef struct countryIE {
    char countryA2[3];
    char countryName[24];
} COUNTRY_IE_ELEMENT;

#define SIOCIWFIRSTPRIV             0x8BE0
#define RT_PRIV_IOCTL               (SIOCIWFIRSTPRIV + 0x0E)

#define RT_OID_SYNC_RT61            0x0D010750
#define RT_OID_WSC_QUERY_STATUS     ((RT_OID_SYNC_RT61 + 0x01) & 0xffff)

extern COUNTRY_IE_ELEMENT conutry_ie_array[];
extern int conutry_count;
extern char wifi_config_file[2][64];

void config_to_system_wireless();

void produce_countrycode(int param_flag, char* param);
void produce_ssid_2g(int param_flag, char* param);
void produce_ssidpsk_2g(int param_flag, char* param);
void produce_wpspin_2g(int param_flag, char* param);
void produce_ssid_5g(int param_flag, char* param);
void produce_ssidpsk_5g(int param_flag, char* param);
void produce_wps_status(int param_flag, char* param);

#endif
