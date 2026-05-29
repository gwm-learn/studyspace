#include "produce_system.h"
#include "produce_wifi.h"
#include "produce_util.h"

static int encrypt_pass(const char* password, char* encryptPass, int maxLen) {
    int ret = -1;
    char cmd_buf[128] = {0};

    if (!IS_EMPTY_STRING(password)) {
        //get username first, encrypt password need it
        char username[64] = {0};
        system("uci -q get system.auth.username > " TMP_PASS_FILE);
        read_file_to_buf(TMP_PASS_FILE, username, sizeof(username) - 1);
        if (username[0] == 0)
            strcpy(username, DEFAULT_USERNAME);
        snprintf(cmd_buf, sizeof(cmd_buf) - 1, FORMAT_PASS_CMD, password, username, username);
        system(cmd_buf);
        read_file_to_buf(TMP_PASS_FILE, encryptPass, maxLen);
        if (!IS_EMPTY_STRING(encryptPass)) {
            ret = 0;
        }
        //printf("%s %d cmd:%s encryptPass:%s\n", __func__, __LINE__, cmd_buf, encryptPass);
    }

    return ret;
}

static int config_check_mac() {
    int ret = 0;
    char buf_cmp[256] = {0};
    char buf_ori[256] = {0};
    char fmac_str[24] = {0};

    get_produce_param(PRODUCE_MACADDR_KEY, NULL, fmac_str);
    conver_mac_addr_to_sp1(fmac_str, buf_ori);
    read_file_to_buf(BR_MAC_VERIFY_FILE, buf_cmp, sizeof(buf_cmp) - 1);
    if (buf_ori[0] != 0 && buf_cmp[0] != 0 && strcasecmp(buf_ori, buf_cmp)) {
        printf("check mac failed: %s - %s\n", buf_ori, buf_cmp);
        ret = 1;
    }

    return ret;
}

static int config_check_password() {
    int ret = 0;
    char buf_cmp[256] = {0};
    char buf_ori[256] = {0};
    char pass_str[32] = {0};

    memset(buf_cmp, 0x0, sizeof(buf_cmp));
    memset(buf_ori, 0x0, sizeof(buf_ori));
    memset(pass_str, 0x0, sizeof(pass_str));
    get_produce_param(PRODUCE_ADMINPASS_KEY, NULL, pass_str);
    if (encrypt_pass(pass_str, buf_ori, sizeof(buf_ori) - 1) == 0) {
        system("uci get system.auth.password > " TMP_PASS_FILE);
        read_file_to_buf(TMP_PASS_FILE, buf_cmp, sizeof(buf_cmp) - 1);
        if (buf_ori[0] != 0 && strcasecmp(buf_ori, buf_cmp)) {
            printf("check adminpassword failed: %s - %s\n", buf_ori, buf_cmp);
            ret = 1;
        }
    }

    return ret;
}

static int config_check_wireless() {
    int ret = 0;
    char buf_cmp[256] = {0};
    char buf_ori[256] = {0};
    char buf_cmd[256] = {0};

    memset(buf_cmp, 0x0, sizeof(buf_cmp));
    memset(buf_ori, 0x0, sizeof(buf_ori));

    get_produce_param(PRODUCE_2GSSIDNAME_KEY, NULL, buf_ori);
    if (buf_ori[0] != 0) {
        snprintf(buf_cmd, sizeof(buf_cmd) - 1, "datconf -f %s get SSID1 > %s",
                 wifi_config_file[WIFI_RADIO_2G_INDEX], CONFIG_CHECK_TMP_FILE);
        system(buf_cmd);
        read_file_to_buf(CONFIG_CHECK_TMP_FILE, buf_cmp, sizeof(buf_cmp) - 1);
        if (strcasecmp(buf_ori, buf_cmp)) {
            printf("check 2g ssid failed: %s - %s\n", buf_ori, buf_cmp);
            ret = 1;
        }
    } else {
        ret = 1;
    }

    get_produce_param(PRODUCE_5GSSIDNAME_KEY, NULL, buf_ori);
    if (buf_ori[0] != 0) {
        snprintf(buf_cmd, sizeof(buf_cmd) - 1, "datconf -f %s get SSID1 > %s",
                 wifi_config_file[WIFI_RADIO_5G_INDEX], CONFIG_CHECK_TMP_FILE);
        system(buf_cmd);
        read_file_to_buf(CONFIG_CHECK_TMP_FILE, buf_cmp, sizeof(buf_cmp) - 1);
        if (strcasecmp(buf_ori, buf_cmp)) {
            printf("check 5g ssid failed: %s - %s\n", buf_ori, buf_cmp);
            ret = 1;
        }
    } else {
        ret = 1;
    }

    return ret;
}

static void config_to_system_password() {
    int changed = 0;
    char buf_exc[256] = {0};
    char buf_ori[256] = {0};
    char pass_str[32] = {0};
    char username[64] = {0};

    get_produce_param(PRODUCE_ADMINPASS_KEY, NULL, pass_str);
    memset(buf_exc, 0x0, sizeof(buf_exc));
    memset(buf_ori, 0x0, sizeof(buf_ori));
    if (encrypt_pass(pass_str, buf_ori, sizeof(buf_ori) - 1) == 0) {
        strcpy(buf_exc, "uci set system.auth.password=");
        strcat(buf_exc, buf_ori);
        system(buf_exc);
        changed = 1;
    }

    system("uci -q get system.auth.username > "TMP_PASS_FILE);
    read_file_to_buf(TMP_PASS_FILE, username, sizeof(username) - 1);
    if (username[0] == 0)
        strcpy(username, DEFAULT_USERNAME);
    if (pass_str[0]) {
        //bl_sync_account root 'nE7n$8q%5m'
        memset(buf_exc, 0x0, sizeof(buf_exc));
        sprintf(buf_exc, "bl_sync_account '%s' '%s'", username, pass_str);
        system(buf_exc);
    } else {
        memset(buf_exc, 0x0, sizeof(buf_exc));
        sprintf(buf_exc, "bl_sync_account '%s' '%s'", username, "admin");
        system(buf_exc);
    }

    if (changed) {
        system("uci commit system");
        system("sync");
    }
}

void produce_restore_defaults(int param_flag, char* param) {
    if (IS_FACTORY_MODE()) {
        printf("restoredefault but now is in factorymode, please excute 'produce factoryMode fm=0' to exit factorymode.\n");
        return;
    }

    if (!IS_RESTOREDEFAULT_ING()) {
        int fd = creat(RESTOREDEFAULT_ING_FILE, 0666);
        if (fd > 0) close(fd);

        printf("restoredefault...\n");
        system("firstboot -y");
        system("sync");
        sleep(1);
        system("reboot");
    }
}

void produce_config_check(int param_flag, char* param) {
    int ret = 0;

    ret |= config_check_mac();
    ret |= config_check_password();
    ret |= config_check_wireless();

    if (ret == 0) {
        printf("!success!\n");
    } else {
        printf("!fail!\n");
    }
}

void produce_reset_flag(int param_flag, char* param) {
    char str_val[16] = {0};

    if (param_flag == PARAM_FLAG_HAS_SUBCOMMAND) {
        strncpy(str_val, param, sizeof(str_val) - 1);
        if (param[0] == '0' && param[0] == '\0') {
            str_val[0] = 0;
        }
        set_produce_param(PRODUCE_RESETFLAG_KEY, NULL, str_val);
    } else {
        get_produce_param(PRODUCE_RESETFLAG_KEY, NULL, str_val);
        if (str_val[0] == 0) {
            str_val[0] = '0';
        }
        printf("%s", str_val);
        fflush(stdout);
    }
}

void produce_config_write(int param_flag, char* param) {
    config_to_system_password();
    config_to_system_wireless();
}

void produce_sn(int param_flag, char* param) {
    if (param_flag == PARAM_FLAG_HAS_SUBCOMMAND) {
        if (set_produce_param(PRODUCE_SERIALN_KEY, NULL, param) == 0) {
            printf("!success!\n");
        } else {
            printf("!fail!\n");
        }
    } else {
        get_produce_param(PRODUCE_SERIALN_KEY, NULL, NULL);
    }
}

void produce_fota_url(int param_flag, char* param) {
    if (param_flag == PARAM_FLAG_HAS_SUBCOMMAND) {
        if (set_produce_param(PRODUCE_FOTAURL_KEY, NULL, param) == 0) {
            printf("!success!\n");
        } else {
            printf("!fail!\n");
        }
    } else {
        get_produce_param(PRODUCE_FOTAURL_KEY, NULL, NULL);
    }
}

void produce_admin_passwd(int param_flag, char* param) {
    if (param_flag == PARAM_FLAG_HAS_SUBCOMMAND) {
        if (set_produce_param(PRODUCE_ADMINPASS_KEY, NULL, param) == 0) {
            printf("!success!\n");
        } else {
            printf("!fail!\n");
        }
    } else {
        get_produce_param(PRODUCE_ADMINPASS_KEY, NULL, NULL);
    }
}

void produce_product_class(int param_flag, char* param) {
    if (param_flag == PARAM_FLAG_HAS_SUBCOMMAND) {
        if (set_produce_param(PRODUCE_PRODUCTCLASS_KEY, NULL, param) == 0) {
            printf("!success!\n");
        } else {
            printf("!fail!\n");
        }
    } else {
        get_produce_param(PRODUCE_PRODUCTCLASS_KEY, NULL, NULL);
    }
}

void produce_software_version(int param_flag, char* param) {
    printf("!%s!\n", CUSTOMER_SW_VER);
}

void produce_hardware_version(int param_flag, char* param) {
    printf("!%s!\n", CUSTOMER_HW_VER);
}

void produce_factory_mode(int param_flag, char* param) {
    if (param_flag == PARAM_FLAG_HAS_SUBCOMMAND) {
        if (strstr(param, "fm=1") != NULL) {
            system("echo 1 > " FACTORY_MODE_FILE);
            system("echo 1 > " LED_FACTORY_MODE_FILE);
            printf("in factory mode\n");
        } else if (strstr(param, "fm=0") != NULL) {
            unlink(LED_FACTORY_MODE_FILE);
            unlink(FACTORY_MODE_FILE);
            printf("out factory mode\n");
        } else {
            printf("!INPUT ERROR!\n");
        }
    } else {
        if (IS_FACTORY_MODE()) {
            printf("in factory mode\n");
        } else {
            printf("out factory mode\n");
        }
    }
}

void produce_encrypt_pass(int param_flag, char* param) {
    char buf_ori[132] = {};
    if (param_flag == PARAM_FLAG_HAS_SUBCOMMAND) {
        encrypt_pass(param, buf_ori, sizeof(buf_ori) - 1);
        printf("%s", buf_ori);
    }
}

void produce_show_all_factory_params(int param_flag, char* param) {
    system(CMD_READ_PRODUCE_TOOL " | grep bl");    //all storage params name have same string prefix 'bl_'
}
