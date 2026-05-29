/*
 *      Operation routines for produce access
 *
 *      Authors: broadlink gaoweiming
 *
 *      v 2.00 2026/5/12
 *
 */

#include "produce_led.h"
#include "produce_util.h"
#include "produce_wifi.h"
#include "produce_mac.h"
#include "produce_eth.h"
#include "produce_cwmp.h"
#include "produce_system.h"

typedef struct _produce_t {
    const char* key;
    void (*func)(int, char*);
    bool is_show;
    const char* help_info;
} produce_t;

produce_t produce_table[] = {
    {"restoredefault",       produce_restore_defaults,          true,   "restoredefault ----- reset current setting to default."},
    {"showAllFactoryParams", produce_show_all_factory_params,   true,   "showAllFactoryParams ----- show all stored factory parameters."},
    {"resetflag",            produce_reset_flag,                true,   "resetflag [0|1] ----- reset flag."},
    {"configcheck",          produce_config_check,              true,   "configcheck ----- check restoredefault is ok."},
    {"configwrite",          produce_config_write,              true,   "configwrite ----- write config to flash."},
    {"macrw",                produce_macrw,                     true,   "macrw [macaddr] ----- get or set macaddr, e.g:produce macrw 021018555888"},
    {"showallocedwanmac",    produce_show_alloc_wan_mac,        true,   "showallocedwanmac ----- show alloc wan mac."},
    {"addallocedwanmac",     produce_add_alloc_wan_mac,         true,   "addallocedwanmac ----- add alloc wan mac."},
    {"allocwanmac",          produce_alloc_wan_mac,             true,   "allocwanmac ----- alloc wan mac."},
    {"freewanmac",           produce_free_wan_mac,              true,   "freewanmac ----- free wan mac."},
    {"freeallwanmac",        produce_free_all_wan_mac,          true,   "freeallwanmac ----- free all wan mac."},
    {"mactype",              produce_mactype,                   true,   "mactype [type] ----- get mactypelist or get mac by type."},
    {"countrycode",          produce_countrycode,               true,   "countrycode [xx | help] ----- help will show country list."},
    {"sn",                   produce_sn,                        true,   "sn [serialnum] ----- get or set serial number."},
    {"fotaUrl",              produce_fota_url,                  true,   "fotaUrl [url] ----- get or set onlineUpgrade url."},
    {"adminPasswd",          produce_admin_passwd,              true,   "adminPasswd [password] --- get or set password of admin."},
    {"encryptPass",          produce_encrypt_pass,              true,   "encryptPass ----- Algorithm encryption"},
    {"productClass",         produce_product_class,             true,   "productClass [name] --- get or set product class name."},
    {"wpspin_2g",            produce_wpspin_2g,                 true,   "wpspin_2g [pincode] ----- get or set wps pincode."},
    {"ssid_2g",              produce_ssid_2g,                   true,   "ssid_2g [ssid] ----- get or set ssid of 2.4G ."},
    {"ssidpsk_2g",           produce_ssidpsk_2g,                true,   "ssidpsk_2g [psk] ----- get or set password of 2.4G."},
    {"ssid_5g",              produce_ssid_5g,                   true,   "ssid_5g [ssid] ----- get or set ssid of 5.8G."},
    {"ssidpsk_5g",           produce_ssidpsk_5g,                true,   "ssidpsk_5g [psk] ----- get or set password of 5.8G."},
    {"wps_status",           produce_wps_status,                true,   "wps_status [5g/2g]----- get wps status."},
    {"swVersion",            produce_software_version,          true,   "swVersion ----- show the sw version."},
    {"hwVersion",            produce_hardware_version,          true,   "hwVersion ----- show the hw version."},
    {"eth",                  produce_eth,                       true,   "eth portX ----- test eth interface, wan X is 0, lan X is 1 2 3 ."},
    {"cwmp",                 produce_cwmp,                      true,   "cwmp [manufacturer | product_class | hardware_version | software_version | oui | serial_number] --- show cwmp params value."},
    {"factoryMode",          produce_factory_mode,              true,   "factoryMode ----- operate factory mode\n        fm=1 ----- enter factory mode.\n        fm=0 ----- outof factory mode."},
    {"allledon",             produce_all_led_on,                true,   "allledon ----- trun on all leds."},
    {"allledoff",            produce_all_led_off,               true,   "allledoff ----- trun off all leds."},
    {"allledblink",          produce_all_led_blink,             true,   "allledblink  ----- blink all leds."},
    {"allledlist",           produce_all_led_list,              true,   "allledlist ----- list all leds."},
    {"ledon",                produce_led_on,                    true,   "ledon LedName ----- lighton one spec led, you can find the from allledlist command."},
    {"ledoff",               produce_led_off,                   true,   "ledoff LedName ----- lightoff one spec led, you can find the from allledlist command."},
    {"ledblink",             produce_led_blink,                 true,   "ledblink LedName ----- blink one spec led, you can find the from allledlist command."},
    {NULL,                   NULL,                              true,   NULL},
};

static int compare_produce_table(const void* a, const void* b) {
    return strcmp(((produce_t*)a)->key, ((produce_t*)b)->key);
}

static void (*binary_search_produce_table(const char* key))(int, char*) {
    int low = 0, high = sizeof(produce_table) / sizeof(produce_table[0]) - 2;
    while (low <= high) {
        int mid = (low + high) / 2;
        int cmp = strcmp(produce_table[mid].key, key);
        if (cmp == 0)
            return produce_table[mid].func;
        else if (cmp < 0)
            low = mid + 1;
        else
            high = mid - 1;
    }
    return NULL;
}

static void show_help(void) {
    int offset = 0;
    size_t buffer_size = 0;
    static char* help_buffer = NULL;

    buffer_size += snprintf(NULL, 0, "Usage: produce cmd\noption:\ncmd:\n");
    for (int i = 0; produce_table[i].help_info != NULL; i++) {
        if (produce_table[i].is_show == false)
            continue;
        buffer_size += snprintf(NULL, 0, "    %s\n", produce_table[i].help_info);
    }
    buffer_size += snprintf(NULL, 0, "\n");

    help_buffer = (char*)malloc(buffer_size + 1);
    if (help_buffer == NULL) {
        perror("Failed to allocate memory for help buffer");
        exit(EXIT_FAILURE);
    }

    offset += snprintf(help_buffer + offset, buffer_size - offset, "Usage: produce cmd\noption:\ncmd:\n");
    for (int i = 0; produce_table[i].help_info != NULL; i++) {
        if (produce_table[i].is_show == false)
            continue;
        offset += snprintf(help_buffer + offset, buffer_size - offset, "    %s\n", produce_table[i].help_info);
    }
    offset += snprintf(help_buffer + offset, buffer_size - offset, "\n");

    fputs(help_buffer, stdout);
    free(help_buffer);
}

static void exec_func(int argc, char** argv) {
    qsort(produce_table, sizeof(produce_table) / sizeof(produce_table[0]) - 1, sizeof(produce_t), compare_produce_table);
    void (*func)(int, char*) = binary_search_produce_table(argv[1]);

    if (func != NULL) {
        func(argc, (argc == PARAM_FLAG_HAS_SUBCOMMAND) ? argv[2] : NULL);
    } else if (strcmp(argv[1], "echo") == 0) {
        if (argc > 2) {
            for (int i = 2; i < argc; i++) {
                printf("%s", argv[i]);
                if (i + 1 < argc) printf(" ");
            }
            printf("\n");
        }
    } else {
        show_help();
        exit(1);
    }
}

int main(int argc, char** argv) {
    if (argc == 1 || argc > 3) {
        show_help();
        return 0;
    }

    exec_func(argc, argv);
    return 0;
}
