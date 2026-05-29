#include "produce_wifi.h"
#include "produce_util.h"

COUNTRY_IE_ELEMENT conutry_ie_array[] = {
    {"AL", "ALBANIA"}, {"DZ", "ALGERIA"}, {"AR", "ARGENTINA"},
    {"AM", "ARMENIA"}, {"AU", "AUSTRALIA"}, {"AT", "AUSTRIA"},
    {"AZ", "AZERBAIJAN"}, {"BH", "BAHRAIN"}, {"BY", "BELARUS"},
    {"BE", "BELGIUM"}, {"BZ", "BELIZE"}, {"BO", "BOLIVIA"},
    {"BR", "BRAZIL"}, {"BN", "BRUNEI"}, {"BG", "BULGARIA"},
    {"CA", "CANADA"}, {"CL", "CHILE"}, {"CN", "CHINA"},
    {"CO", "COLOMBIA"}, {"CR", "COSTA RICA"}, {"HR", "CROATIA"},
    {"CY", "CYPRUS"}, {"CZ", "CZECH REPUBLIC"}, {"DK", "DENMARK"},
    {"DO", "DOMINICAN REPUBLIC"}, {"EC", "ECUADOR"}, {"EG", "EGYPT"},
    {"SV", "EL SALVADOR"}, {"EE", "ESTONIA"}, {"FI", "FINLAND"},
    {"FR", "FRANCE"}, {"GE", "GEORGIA"}, {"DE", "GERMANY"},
    {"GR", "GREECE"}, {"GT", "GUATEMALA"}, {"HN", "HONDURAS"},
    {"HK", "HONG KONG"}, {"HU", "HUNGARY"}, {"IS", "ICELAND"},
    {"IN", "INDIA"}, {"ID", "INDONESIA"}, {"IR", "IRAN"},
    {"IE", "IRELAND"}, {"IL", "ISRAEL"}, {"IT", "ITALY"},
    {"JP", "JAPAN"}, {"JO", "JORDAN"}, {"KZ", "KAZAKHSTAN"},
    {"KR", "NORTH KOREA"}, {"KP", "KOREA REPUBLIC"},
    {"KW", "KUWAIT"}, {"LV", "LATVIA"}, {"LB", "LEBANON"},
    {"LI", "LIECHTENSTEIN"}, {"LT", "LITHUANIA"}, {"LU", "LUXEMBOURG"},
    {"MO", "CHINA MACAU"}, {"MK", "MACEDONIA"}, {"MY", "MALAYSIA"},
    {"MX", "MEXICO"}, {"MC", "MONACO"}, {"MA", "MOROCCO"},
    {"NL", "NETHERLANDS"}, {"NZ", "NEW ZEALAND"}, {"NO", "NORWAY"},
    {"OM", "OMAN"}, {"PK", "PAKISTAN"}, {"PA", "PANAMA"},
    {"PE", "PERU"}, {"PH", "PHILIPPINES"}, {"PL", "POLAND"},
    {"PT", "PORTUGAL"}, {"PR", "PUERTO RICO"}, {"QA", "QATAR"},
    {"RA", "ROMANIA"}, {"RU", "RUSSIAN"}, {"SA", "SAUDI ARABIA"},
    {"SG", "SINGAPORE"}, {"SK", "SLOVAKIA"}, {"SI", "SLOVENIA"},
    {"ZA", "SOUTH AFRICA"}, {"ES", "SPAIN"}, {"SE", "SWEDEN"},
    {"CH", "SWITZERLAND"}, {"SY", "SYRIAN ARAB REPUBLIC"},
    {"TW", "TAIWAN"}, {"TH", "THAILAND"}, {"TT", "TRINIDAD AND TOBAGO"},
    {"TN", "TUNISIA"}, {"TR", "TURKEY"}, {"UA", "UKRAINE"},
    {"AE", "UNITED ARAB EMIRATES"}, {"GB", "UNITED KINGDOM"},
    {"US", "UNITED STATES"}, {"UY", "URUGUAY"}, {"UZ", "UZBEKISTAN"},
    {"VE", "VENEZUELA"}, {"VN", "VIET NAM"}, {"YE", "YEMEN"},
    {"ZW", "ZIMBABWE"},
};
int conutry_count = sizeof(conutry_ie_array) / sizeof(conutry_ie_array[0]);

char wifi_config_file[2][64] = {
    "/etc/wireless/mediatek/mt7981.dbdc.b1.dat",
    "/etc/wireless/mediatek/mt7981.dbdc.b0.dat"
};

static void write_country_code(char* country_code) {
    int i = 0;
    char find = 0;

    for (i = 0; i < conutry_count; i++) {
        if (strcmp(country_code, conutry_ie_array[i].countryA2) == 0) {
            find = 1;
            set_produce_param(PRODUCE_COUNTRYCODE_KEY, NULL, country_code);
            break;
        }
    }

    if (find == 0) {
        if (MATCH_UNSET_STR(country_code)) {
            find = 1;
            set_produce_param(PRODUCE_COUNTRYCODE_KEY, NULL, country_code);
        }
    }

    if (find == 0) {
        printf("country code is error\n");
    } else {
        printf("!success!\n");
    }
}

void config_to_system_wireless() {
    char buf_exc[512] = {0};
    char buf_ori[256] = {0};

    memset(buf_exc, 0x0, sizeof(buf_exc));
    memset(buf_ori, 0x0, sizeof(buf_ori));
    get_produce_param(PRODUCE_2GSSIDNAME_KEY, NULL, buf_ori);
    if (buf_ori[0] != 0) {
        snprintf(buf_exc, sizeof(buf_exc) - 1, "datconf -f %s set SSID1 %s",
                 wifi_config_file[WIFI_RADIO_2G_INDEX], buf_ori);
        system(buf_exc);
    }

    memset(buf_exc, 0x0, sizeof(buf_exc));
    memset(buf_ori, 0x0, sizeof(buf_ori));
    get_produce_param(PRODUCE_5GSSIDNAME_KEY, NULL, buf_ori);
    if (buf_ori[0] != 0) {
        snprintf(buf_exc, sizeof(buf_exc) - 1, "datconf -f %s set SSID1 %s",
                 wifi_config_file[WIFI_RADIO_5G_INDEX], buf_ori);
        system(buf_exc);
    }

    memset(buf_exc, 0x0, sizeof(buf_exc));
    memset(buf_ori, 0x0, sizeof(buf_ori));
    get_produce_param(PRODUCE_2GSSIDPASS_KEY, NULL, buf_ori);
    if (buf_ori[0] != 0) {
        snprintf(buf_exc, sizeof(buf_exc) - 1, "datconf -f %s set WPAPSK1 %s",
                 wifi_config_file[WIFI_RADIO_2G_INDEX], buf_ori);
        system(buf_exc);
    }

    memset(buf_exc, 0x0, sizeof(buf_exc));
    memset(buf_ori, 0x0, sizeof(buf_ori));
    get_produce_param(PRODUCE_5GSSIDPASS_KEY, NULL, buf_ori);
    if (buf_ori[0] != 0) {
        snprintf(buf_exc, sizeof(buf_exc) - 1, "datconf -f %s set WPAPSK1 %s",
                 wifi_config_file[WIFI_RADIO_5G_INDEX], buf_ori);
        system(buf_exc);
    }
}

static void get_wps_status(char *interface){
    int socket_id;
    struct iwreq wrq;
    int data = 0;
    socket_id = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_id < 0) {
        perror("socket() failed");
        return socket_id;
    }

    snprintf(wrq.ifr_name, sizeof(wrq.ifr_name), "%s", interface);
    wrq.u.data.length = sizeof(data);
    wrq.u.data.pointer = (caddr_t) &data;
    wrq.u.data.flags = RT_OID_WSC_QUERY_STATUS;
    if( ioctl(socket_id, RT_PRIV_IOCTL, &wrq) == -1)
    {
        fprintf(stderr, "%s: ioctl fail\n", __func__);
    }
    close(socket_id);

    switch(data) {
        case 0:
            printf("Not used\n");
            break;
        case 1:
            printf("Idle\n");
            break;
        case 2:
            printf("WSC Fail\n");
            break;
        case 3:
            printf("Start WSC Process\n");
            break;
        case 4:
            printf("Received EAPOL-Start\n");
            break;
        case 5:
            printf("Sending EAP-Req(ID)\n");
            break;
        case 6:
            printf("Receive EAP-Rsp(ID)\n");
            break;
        case 7:
            printf("Receive EAP-Req with wrong WSC SMI Vendor Id\n");
            break;
        case 8:
            printf("Receive EAPReq with wrong WSC Vendor Type\n");
            break;
        case 9:
            printf("Sending EAP-Req(WSC_START)\n");
            break;
        case 10:
            printf("Send M1\n");
            break;
        case 11:
            printf("Received M1\n");
            break;
        case 12:
            printf("Send M2\n");
            break;
        case 13:
            printf("Received M2\n");
            break;
        case 14:
            printf("Received M2D\n");
            break;
        case 15:
            printf("Send M3\n");
            break;
        case 16:
            printf("Received M3\n");
            break;
        case 17:
            printf("Send M4\n");
            break;
        case 18:
            printf("Received M4\n");
            break;
        case 19:
            printf("Send M5\n");
            break;
        case 20:
            printf("Received M5\n");
            break;
        case 21:
            printf("Send M6\n");
            break;
        case 22:
            printf("Received M6\n");
            break;
        case 23:
            printf("Send M7\n");
            break;
        case 24:
            printf("Received M7\n");
            break;
        case 25:
            printf("Send M8\n");
            break;
        case 26:
            printf("Received M8\n");
            break;
        case 27:
            printf("Processing EAP Response (ACK)\n");
            break;
        case 28:
            printf("Processing EAP Request (Done)\n");
            break;
        case 29:
            printf("Processing EAP Response (Done)\n");
            break;
        case 30:
            printf("WSC Fail\n");
            break;
        case 31:
            printf("WSC Fail\n");
            break;
        case 32:
            printf("WSC Fail\n");
            break;
        case 33:
            printf("WSC Fail\n");
            break;
        case 34:
            printf("Success\n");
            break;
        case 35:
            printf("SCAN AP\n");
            break;
        case 36:
            printf("EAPOL START SENT\n");
            break;
        case 37:
            printf("WSC_EAP_RSP_DONE_SENT\n");
            break;
        case 38:
            printf("WAIT PINCODE\n");
            break;
        case 39:
            printf("WSC_START_ASSOC\n");
            break;
        case 0x101:
            printf("PBC:TOO MANY AP\n");
            break;
        case 0x102:
            printf("PBC:NO AP\n");
            break;
        case 0x103:
            printf("EAP_FAIL_RECEIVED\n");
            break;
        case 0x104:
            printf("EAP_NONCE_MISMATCH\n");
            break;
        case 0x105:
            printf("EAP_INVALID_DATA\n");
            break;
        case 0x106:
            printf("PASSWORD_MISMATCH\n");
            break;
        case 0x107:
            printf("EAP_REQ_WRONG_SMI\n");
            break;
        case 0x108:
            printf("EAP_REQ_WRONG_VENDOR_TYPE\n");
            break;
        case 0x109:
            printf("Overlap\n");
            break;
        default:
            printf("Unknown\n");
            break;
    }
}

void produce_countrycode(int param_flag, char* param) {
    if (param_flag != PARAM_FLAG_HAS_SUBCOMMAND) {
        get_produce_param(PRODUCE_COUNTRYCODE_KEY, NULL, NULL);
    } else if (strstr(param, "help") != NULL) {
        printf("\neg:produce countrycode CN\n");
        for (int i = 0; i < conutry_count; i++) {
            printf("%s  %s\n", conutry_ie_array[i].countryA2, conutry_ie_array[i].countryName);
        }
    } else {
        write_country_code(param);
    }
}

void produce_ssid_2g(int param_flag, char* param) {
    if (param_flag == PARAM_FLAG_HAS_SUBCOMMAND) {
        if (strlen(param) < 32 && set_produce_param(PRODUCE_2GSSIDNAME_KEY, NULL, param) == 0) {
            printf("!success!\n");
        } else {
            printf("!fail!\n");
        }
    } else {
        get_produce_param(PRODUCE_2GSSIDNAME_KEY, NULL, NULL);
    }
}

void produce_ssidpsk_2g(int param_flag, char* param) {
    if (param_flag == PARAM_FLAG_HAS_SUBCOMMAND) {
        if (((strlen(param) < 32 && strlen(param) >= 8) || MATCH_UNSET_STR(param)) && set_produce_param(PRODUCE_2GSSIDPASS_KEY, NULL, param) == 0) {
            printf("!success!\n");
        } else {
            printf("!fail!\n");
        }
    } else {
        get_produce_param(PRODUCE_2GSSIDPASS_KEY, NULL, NULL);
    }
}

void produce_wpspin_2g(int param_flag, char* param) {
    int checksum = 0;
    char wps_pin[16] = {0};

    if (param_flag == PARAM_FLAG_HAS_SUBCOMMAND) {
        strcpy(wps_pin, param);
        checksum = wps_pin_validate_checksum(atol(wps_pin));
        if (checksum == 0) {
            printf("!fail!\n");
            return;
        }

        if ((strlen(param) == 8 || MATCH_UNSET_STR(param)) && set_produce_param(PRODUCE_2GWPSPINCO_KEY, NULL, param) == 0) {
            printf("!success!\n");
        } else {
            printf("!fail!\n");
        }
    } else {
        get_produce_param(PRODUCE_2GWPSPINCO_KEY, NULL, NULL);
    }
}

void produce_ssid_5g(int param_flag, char* param) {
    if (param_flag == PARAM_FLAG_HAS_SUBCOMMAND) {
        if (strlen(param) < 32 && set_produce_param(PRODUCE_5GSSIDNAME_KEY, NULL, param) == 0) {
            printf("!success!\n");
        } else {
            printf("!fail!\n");
        }
    } else {
        get_produce_param(PRODUCE_5GSSIDNAME_KEY, NULL, NULL);
    }
}

void produce_ssidpsk_5g(int param_flag, char* param) {
    if (param_flag == PARAM_FLAG_HAS_SUBCOMMAND) {
        if (((strlen(param) < 32 && strlen(param) >= 8) || MATCH_UNSET_STR(param)) && set_produce_param(PRODUCE_5GSSIDPASS_KEY, NULL, param) == 0) {
            printf("!success!\n");
        } else {
            printf("!fail!\n");
        }
    } else {
        get_produce_param(PRODUCE_5GSSIDPASS_KEY, NULL, NULL);
    }
}

void produce_wps_status(int param_flag, char* param) {
    if (param_flag == PARAM_FLAG_HAS_SUBCOMMAND) {
        if (!strcmp(param, "2g")) {
            get_wps_status("ra0");
        } else if (!strcmp(param, "5g")) {
            get_wps_status("rax0");
        } else {
            printf("!fail!\n");
        }
    } else {
        printf("!fail!\n");
    }
}