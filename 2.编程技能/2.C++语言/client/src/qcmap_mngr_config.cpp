#include "qcmap_mngr_config.h"
#include "qcmap_mngr_apn.h"
#include "qcmap_mngr_wan.h"
#include "qcmap_mngr_info.h"

work_type_t work_type = WORK_TYPE_NONE;
bool update_firewall = false;
bool update_default_route = false;
bool has_default_route = false;
int default_route_check_count = 0;
void qcmap_release_config(void) {
    update_firewall = false;
    update_default_route = false;
    has_default_route = false;
    default_route_check_count = 0;
}

bool qcmap_get_update_firewall(void) {
    return update_firewall;
}

void qcmap_set_update_firewall(bool update) {
    update_firewall = update;
}

//qcmap中的ip无法设置生效即该值无效，通过at+cgdcont可以进行设置
bool qcmap_show_all_profile(void) {
    qcmap_wwan_policy_list_info wwan_policy_list;
    qmi_error_type_v01 qmi_err_num;

    if (qcmap_client->GetWWANPolicyListEx(&wwan_policy_list, &qmi_err_num)) {
        PRINT_DEBUG("   +---------+----------+------+------+------------+---------------------------------+\n");
        PRINT_DEBUG("   | Profile | Subs_Id  | Tech |  IP  | Profile_Id | Profile_Id |       APN Name     |\n");
        PRINT_DEBUG("   |         |          |      |      |  (3gpp)    |   (3gpp2)  |                    |\n");
        PRINT_DEBUG("   +---------+----------+------+------+------------+---------------------------------+\n");

        for (int i = 0; i < wwan_policy_list.wwan_policy_len; i++) {
            if (wwan_policy_list.wwan_policy[i].profile_handle == wwan_policy_list.default_profile_handle) {
                PRINT_DEBUG("   |%6d(*)|%10s|%6s|%6s|%12d|%12d|%20s|\n", wwan_policy_list.wwan_policy[i].profile_handle, SUBSCRIPTION_TYPE(wwan_policy_list.wwan_policy[i].subscription_id), WWAN_TECH_TYPE(wwan_policy_list.wwan_policy[i].tech_pref), IP_FAMILY_TYPE(wwan_policy_list.wwan_policy[i].ip_family), wwan_policy_list.wwan_policy[i].profile_id_3gpp,
                            wwan_policy_list.wwan_policy[i].profile_id_3gpp2, wwan_policy_list.wwan_policy[i].apn_name);
            } else {
                PRINT_DEBUG("   |%9d|%10s|%6s|%6s|%12d|%12d|%20s|\n", wwan_policy_list.wwan_policy[i].profile_handle, SUBSCRIPTION_TYPE(wwan_policy_list.wwan_policy[i].subscription_id), WWAN_TECH_TYPE(wwan_policy_list.wwan_policy[i].tech_pref), IP_FAMILY_TYPE(wwan_policy_list.wwan_policy[i].ip_family), wwan_policy_list.wwan_policy[i].profile_id_3gpp,
                            wwan_policy_list.wwan_policy[i].profile_id_3gpp2, wwan_policy_list.wwan_policy[i].apn_name);
            }
        }
        PRINT_DEBUG("   +---------+----------+------+------+------------+---------------------------------+\n");

        return true;
    } else {
        PRINT_DEBUG("show profile fail\n");
    }

    return false;
}

void qcmap_show_profile(int profile_index) {
    qmi_error_type_v01 qmi_err_num;
    qcmap_wwan_policy_list_info wwan_policy_list;

    if (qcmap_client->GetWWANPolicyListEx(&wwan_policy_list, &qmi_err_num)) {
        PRINT_DEBUG("   +---------+----------+------+------+------------+---------------------------------+\n");
        PRINT_DEBUG("   | Profile | Subs_Id  | Tech |  IP  | Profile_Id | Profile_Id |       APN Name     |\n");
        PRINT_DEBUG("   |         |          |      |      |  (3gpp)    |   (3gpp2)  |                    |\n");
        PRINT_DEBUG("   +---------+----------+------+------+------------+---------------------------------+\n");
        for (int i = 0; i < wwan_policy_list.wwan_policy_len; i++) {
            if (profile_index == i + 1) {
                PRINT_DEBUG("   |%9d|%10s|%6s|%6s|%12d|%12d|%20s|\n", wwan_policy_list.wwan_policy[i].profile_handle, SUBSCRIPTION_TYPE(wwan_policy_list.wwan_policy[i].subscription_id), WWAN_TECH_TYPE(wwan_policy_list.wwan_policy[i].tech_pref), IP_FAMILY_TYPE(wwan_policy_list.wwan_policy[i].ip_family), wwan_policy_list.wwan_policy[i].profile_id_3gpp,
                            wwan_policy_list.wwan_policy[i].profile_id_3gpp2, wwan_policy_list.wwan_policy[i].apn_name);
                PRINT_DEBUG("   +---------+----------+------+------+------------+---------------------------------+\n");
                return true;
            }
        }
    } else {
        PRINT_DEBUG("show profile info fail\n");
    }

    return false;
}

bool qcmap_get_profile_info(qcmap_net_profile_and_policy_info *profile_info, int profile_index) {
    qcmap_wwan_policy_list_info wwan_policy_list;
    qmi_error_type_v01 qmi_err_num;

    if (qcmap_client->GetWWANPolicyListEx(&wwan_policy_list, &qmi_err_num)) {
        for (int i = 0; i < wwan_policy_list.wwan_policy_len; i++) {
            if (profile_index == i + 1) {
                memcpy(profile_info, &(wwan_policy_list.wwan_policy[i]), sizeof(qcmap_net_profile_and_policy_info));
                return true;
            }
        }
    } else {
        PRINT_DEBUG("get profile info fail\n");
    }

    return false;
}

bool qcmap_get_profile_ip_family(int *ip_family, int config_index) {
    FILE* fd = NULL;
    char line[128] = {0};
    char cmd[128] = {0};
    char temp_line[128] = {0};
    char ip_family_str[16] = {0};
    bool ret = false;
    int current_index = 0;

    // 构造AT命令，例如：at+cgdcont?
    sprintf(cmd, "%s %s?", QCMAP_MNGR_AT_MNGR_COMMAND, QCMAP_MNGR_AT_COMMAND_CGDCONT);
    if ((fd = popen(cmd, "r")) == NULL) {
        PRINT_DEBUG("run cmd %s fail\n", cmd);
        return ret;
    }

    while (fgets(line, sizeof(line) - 1, fd)) {
        if (!(sscanf(line, "+CGDCONT: %d", &current_index) == 1 && current_index == config_index + 1))
            continue;

        memset(temp_line, 0, sizeof(temp_line));
        strncpy(temp_line, line, sizeof(temp_line));
        temp_line[sizeof(temp_line) - 1] = '\0';
        char *token = strtok(temp_line, ",");
        int field = 0;
        while (token != NULL) {
            if (field == 1) {
                char *start = token;
                while (*start == ' ' || *start == '"') start++;
                char *end = start + strlen(start) - 1;
                while (end >= start && (*end == ' ' || *end == '"')) end--;
                if (end >= start) {
                    size_t len = end - start + 1;
                    strncpy(ip_family_str, start, len);
                    ip_family_str[len] = '\0';
                    if (!strcmp(ip_family_str, "IP")) {
                        *ip_family = 4;
                    } else if (!strcmp(ip_family_str, "IPV6")) {
                        *ip_family = 6;
                    } else {
                        *ip_family = 10;
                    }
                    ret = true;
                } else {
                    ret = false;
                }
                break;
            }
            token = strtok(NULL, ",");
            field++;
        }
        break;
    }
    pclose(fd);
    return ret;
}

int qcmap_auth_str_to_int(char *auth) {
    if (auth == nullptr) {
        return AUTH_TYPE_NONE;
    }

    if (!strcmp("NONE", auth)) {
        return AUTH_TYPE_NONE;
    } else if (!strcmp("PAP", auth)) {
        return AUTH_TYPE_PAP;
    } else if (!strcmp("CHAP", auth)) {
        return AUTH_TYPE_CHAP;
    } else if (!strcmp("PAP+CHAP", auth)) {
        return AUTH_TYPE_PAP_CHAP;
    } else {
        return AUTH_TYPE_NONE;
    }
}

void qcmap_get_wan5g_config(void) {
    char result[64] = {0};
    char wan_name[16] = {0};

    memset(&qcmap_wan5g_list, 0, sizeof(qcmap_wan5g_list));

    for (int i = 1; i <= QCMAP_MNGR_DEFAULT_PROFILE_NUM; i++) {
        if (i == 1)
            sprintf(wan_name, "%s", QCMAP_MNGR_DEFAULT_WAN_NAME);
        else
            sprintf(wan_name, "%s%d", QCMAP_MNGR_DEFAULT_WAN_NAME, i);

        memset(result, 0, sizeof(result));
        if (qcmap_get_uci_value(QCMAP_MNGR_DEFAULT_CONFIG_FILE, wan_name, QCMAP_MNGR_CONFIG_DISABLE, result, sizeof(result))) {
            qcmap_wan5g_list[i - 1].enable = !atoi(result);
        }

        memset(result, 0, sizeof(result));
        if (qcmap_get_uci_value(QCMAP_MNGR_DEFAULT_CONFIG_FILE, wan_name, QCMAP_MNGR_CONFIG_PROFILE, result, sizeof(result))) {
            qcmap_wan5g_list[i - 1].profile_index = atoi(result);
        }

        memset(result, 0, sizeof(result));
        if (qcmap_get_uci_value(QCMAP_MNGR_MOBILE_CONFIG_FILE, wan_name, QCMAP_MNGR_CONFIG_IPTYPE, result, sizeof(result))) {
            qcmap_wan5g_list[i - 1].ip_family = atoi(result);
        }

        memset(result, 0, sizeof(result));
        if (qcmap_get_uci_value(QCMAP_MNGR_MOBILE_CONFIG_FILE, wan_name, QCMAP_MNGR_CONFIG_APNNAME, result, sizeof(result))) {
            sprintf(qcmap_wan5g_list[i - 1].apn_name, "%s", result);
        }

        memset(result, 0, sizeof(result));
        if (qcmap_get_uci_value(QCMAP_MNGR_MOBILE_CONFIG_FILE, wan_name, QCMAP_MNGR_CONFIG_USERNAME, result, sizeof(result))) {
            sprintf(qcmap_wan5g_list[i - 1].username, "%s", result);
        }

        memset(result, 0, sizeof(result));
        if (qcmap_get_uci_value(QCMAP_MNGR_MOBILE_CONFIG_FILE, wan_name, QCMAP_MNGR_CONFIG_PASSWORD, result, sizeof(result))) {
            sprintf(qcmap_wan5g_list[i - 1].password, "%s", result);
        }

        memset(result, 0, sizeof(result));
        if (qcmap_get_uci_value(QCMAP_MNGR_MOBILE_CONFIG_FILE, wan_name, QCMAP_MNGR_CONFIG_AUTH, result, sizeof(result))) {
            qcmap_wan5g_list[i - 1].auth = qcmap_auth_str_to_int(result);
        }

        memset(result, 0, sizeof(result));
        if (qcmap_get_uci_value(QCMAP_MNGR_MOBILE_CONFIG_FILE, wan_name, QCMAP_MNGR_CONFIG_DEFAULTROUTE, result, sizeof(result))) {
            qcmap_wan5g_list[i - 1].defaultroute = atoi(result);
        }

        if (i == 1) {
            memset(result, 0, sizeof(result));
            if (qcmap_get_uci_value(QCMAP_MNGR_MOBILE_CONFIG_FILE, wan_name, QCMAP_MNGR_CONFIG_MANUALAPN, result, sizeof(result))) {
                qcmap_wan5g_list[i - 1].manaul_apn = atoi(result);
            }

            memset(result, 0, sizeof(result));
            if (qcmap_get_uci_value(QCMAP_MNGR_MOBILE_CONFIG_FILE, wan_name, QCMAP_MNGR_CONFIG_NETTYPE, result, sizeof(result))) {
                sprintf(qcmap_wan5g_list[i - 1].nettype, "%s", result);
            }

            memset(result, 0, sizeof(result));
            if (qcmap_get_uci_value(QCMAP_MNGR_MOBILE_CONFIG_FILE, wan_name, QCMAP_MNGR_CONFIG_SUBNETTYPE, result, sizeof(result))) {
                sprintf(qcmap_wan5g_list[i - 1].subnettype, "%s", result);
            }

            memset(result, 0, sizeof(result));
            if (qcmap_get_uci_value(QCMAP_MNGR_MOBILE_CONFIG_FILE, wan_name, QCMAP_MNGR_CONFIG_NATENABLE, result, sizeof(result))) {
                qcmap_wan5g_list[i - 1].nat_enable = atoi(result);
            }

            memset(result, 0, sizeof(result));
            if (qcmap_get_uci_value(QCMAP_MNGR_MOBILE_CONFIG_FILE, wan_name, QCMAP_MNGR_CONFIG_BRIDGEENABLE, result, sizeof(result))) {
                qcmap_wan5g_list[i - 1].bridge_enable = atoi(result);
            }

            memset(result, 0, sizeof(result));
            if (qcmap_get_uci_value(QCMAP_MNGR_DEFAULT_CONFIG_FILE, wan_name, QCMAP_MNGR_CONFIG_LOCKMTU, result, sizeof(result))) {
                qcmap_wan5g_list[i - 1].lockmtu = atoi(result);
            }
        }

        qcmap_wan5g_list[i - 1].update = false;
        
        // 统一打印配置信息
        PRINT_DEBUG("WAN5G[%d]: %d, %d, %d(%s), %s, %s, %s, %d, %d, %d, %s, %s, %d, %d, %d\n",
                   i - 1,
                   qcmap_wan5g_list[i - 1].enable,
                   qcmap_wan5g_list[i - 1].profile_index,
                   qcmap_wan5g_list[i - 1].ip_family,
                   IP_FAMILY_TYPE_STR(qcmap_wan5g_list[i - 1].ip_family),
                   qcmap_wan5g_list[i - 1].apn_name,
                   qcmap_wan5g_list[i - 1].username,
                   qcmap_wan5g_list[i - 1].password,
                   qcmap_wan5g_list[i - 1].auth,
                   qcmap_wan5g_list[i - 1].defaultroute,
                   qcmap_wan5g_list[i - 1].manaul_apn,
                   qcmap_wan5g_list[i - 1].nettype,
                   qcmap_wan5g_list[i - 1].subnettype,
                   qcmap_wan5g_list[i - 1].nat_enable,
                   qcmap_wan5g_list[i - 1].bridge_enable,
                   qcmap_wan5g_list[i - 1].lockmtu);
    }
}

int qcmap_get_call_type(int config_index) {
    switch (qcmap_wan5g_list[config_index].ip_family) {
        case 4:
            return 1;
        case 6:
            return 2;
        case 10:
            return 3;
        default :
            return 3;
    }
}

void qcmap_check_dial_connect(int profile_index, int config_index) {
    unsigned int curr_ms;
    char file_name_v4[64] = {0};
    char file_name_v6[64] = {0};
    unsigned int mark_ms = qcmap_get_uptime_in_ms();

    sprintf(file_name_v4, "%s_%d", QCMAP_MNGR_DIAL_CONNECT_IPV4, profile_index);
    sprintf(file_name_v6, "%s_%d", QCMAP_MNGR_DIAL_CONNECT_IPV6, profile_index);
    do {
        switch (qcmap_wan5g_list[config_index].ip_family) {
            case 4:
                if (access(file_name_v4, F_OK) == 0) {
                    unlink(file_name_v4);
                    PRINT_DEBUG("profile [%d] ipv4 check dial connect success\n", profile_index);
                    return;
                }
                break;
            case 6:
                if (access(file_name_v6, F_OK) == 0) {
                    unlink(file_name_v6);
                    PRINT_DEBUG("profile [%d] ipv6 check dial connect success\n", profile_index);
                    return;
                }
                break;
            case 10:
                if (access(file_name_v4, F_OK) == 0 && access(file_name_v6, F_OK) == 0) {
                    unlink(file_name_v4);
                    unlink(file_name_v6);
                    PRINT_DEBUG("profile [%d] ipv4v6 check dial connect success\n", profile_index);
                    return;
                }
                break;
            default:
                break;
        }
        usleep(0.5 * 1000 * 1000);
        curr_ms = qcmap_get_uptime_in_ms();
    } while (curr_ms - mark_ms < QCMAP_MNGR_DIAL_TIME);
}

void qcmap_check_dial_disconnect(int profile_index, int config_index) {
    unsigned int curr_ms;
    char file_name_v4[64] = {0};
    char file_name_v6[64] = {0};
    unsigned int mark_ms = qcmap_get_uptime_in_ms();

    sprintf(file_name_v4, "%s_%d", QCMAP_MNGR_DIAL_DISCONNECT_IPV4, profile_index);
    sprintf(file_name_v6, "%s_%d", QCMAP_MNGR_DIAL_DISCONNECT_IPV6, profile_index);
    do {
        switch (qcmap_wan5g_list[config_index].ip_family) {
            case 4:
                if (access(file_name_v4, F_OK) == 0) {
                    unlink(file_name_v4);
                    PRINT_DEBUG("profile [%d] ipv4 check dial disconnect success\n", profile_index);
                    return;
                }
                break;
            case 6:
                if (access(file_name_v6, F_OK) == 0) {
                    unlink(file_name_v6);
                    PRINT_DEBUG("profile [%d] ipv6 check dial disconnect success\n", profile_index);
                    return;
                }
                break;
            case 10:
                if (access(file_name_v4, F_OK) == 0 && access(file_name_v6, F_OK) == 0) {
                    unlink(file_name_v4);
                    unlink(file_name_v6);
                    PRINT_DEBUG("profile [%d] ipv4v6 check dial disconnect success\n", profile_index);
                    return;
                }
                break;
            default:
                break;
        }
        usleep(0.5 * 1000 * 1000);
        curr_ms = qcmap_get_uptime_in_ms();
    } while (curr_ms - mark_ms < QCMAP_MNGR_DIAL_TIME);
}

void qcmap_update_interface(int config_index) {
    FILE *fp; 
    bool update = true;
    bool update_v6 = true;
    char path[64] = {0}; 
    char line[64] = {0}; 
    char wan_name[16] = {0};
    char wan_name_v6[16] = {0};
    char command[32] = {0};
    char command_v6[32] = {0};

    if (config_index == 0) {
        sprintf(wan_name, "%s", QCMAP_MNGR_DEFAULT_WAN_NAME);
        sprintf(wan_name_v6, "%s_v6", QCMAP_MNGR_DEFAULT_WAN_NAME);
    } else {
        sprintf(wan_name, "%s%d", QCMAP_MNGR_DEFAULT_WAN_NAME, config_index + 1);
        sprintf(wan_name_v6, "%s%d_v6", QCMAP_MNGR_DEFAULT_WAN_NAME, config_index + 1);
    }

    sprintf(command, "ifup %s", wan_name);
    sprintf(command_v6, "ifup %s", wan_name_v6);

    fp = popen("ubus list network.interface.*  | awk -F. '{print $3}'", "r"); 
    if (fp == NULL) { 
        PRINT_DEBUG("Failed to run command\n"); 
        return; 
    }

    PRINT_DEBUG("wanname : [%s] wanname_v6 : [%s]\n", wan_name, wan_name_v6);

    while (fgets(line, sizeof(line), fp) != NULL) { 
        if (strstr(line, QCMAP_MNGR_DEFAULT_WAN_NAME) == NULL)
            continue;

        char *newline = strchr(line, '\n'); 
        if (newline != NULL) { 
            *newline = '\0'; 
        }
        PRINT_DEBUG("interface : [%s]\n", line);

        switch (qcmap_wan5g_list[config_index].ip_family) {
            case 4:
                if (!strcmp(line, wan_name)) { 
                    update = false;
                }
                break;
            case 6:
                if (!strcmp(line, wan_name_v6)) { 
                    update_v6 = false;
                }
                break;
            case 10:
                if (!strcmp(line, wan_name)) { 
                    update = false;
                }
                if (!strcmp(line, wan_name_v6)) { 
                    update_v6 = false;
                }
                break;
            default:
                break;
        }
    }

    if (update)
        system_ex(command, 0);
    if (update_v6)
        system_ex(command_v6, 0);

    pclose(fp); 
}

void qcmap_update_profile_index(int profile_index, int config_index) {
    char value[8] = {0};
    char wan_name[16] = {0};
    char wan_name_v6[16] = {0};

    if ((profile_index == 1 && work_type == WORK_TYPE_MULTI) || (profile_index > 1 && work_type == WORK_TYPE_BASIC))
        return;

    if (config_index == 0) {
        sprintf(wan_name, "%s", QCMAP_MNGR_DEFAULT_WAN_NAME);
        sprintf(wan_name_v6, "%s_v6", QCMAP_MNGR_DEFAULT_WAN_NAME);
    } else {
        sprintf(wan_name, "%s%d", QCMAP_MNGR_DEFAULT_WAN_NAME, config_index + 1);
        sprintf(wan_name_v6, "%s%d_v6", QCMAP_MNGR_DEFAULT_WAN_NAME, config_index + 1);
    }

    qcmap_wan5g_list[config_index].profile_index = profile_index;
    sprintf(value, "%d", qcmap_wan5g_list[config_index].profile_index);
    qcmap_set_uci_value(QCMAP_MNGR_DEFAULT_CONFIG_FILE, wan_name, QCMAP_MNGR_CONFIG_PROFILE, value);
    qcmap_set_uci_value(QCMAP_MNGR_DEFAULT_CONFIG_FILE, wan_name_v6, QCMAP_MNGR_CONFIG_PROFILE, value);
}

bool qcmap_enable_profile(int profile_index, int config_index) {
    int call_type = 3;

    if ((profile_index == 1 && work_type == WORK_TYPE_MULTI) || (profile_index > 1 && work_type == WORK_TYPE_BASIC))
        return;

    qmi_error_type_v01 qmi_err_num = QMI_ERR_NONE_V01;

    //切换到该profile
    if (!qcmap_client->SetWWANProfileHandlePreference(profile_index, &qmi_err_num)) {
        PRINT_DEBUG("change to profile [%d] config [%d] fail\n", profile_index, config_index);
        return false;
    }

    PRINT_DEBUG("change to profile [%d] config [%d] success\n", profile_index, config_index);
    call_type = qcmap_get_call_type(config_index);
    if (qcmap_client->ConnectBackHaul(call_type, &qmi_err_num)) {
        if (qmi_err_num != QMI_ERR_NONE_V01) {
            PRINT_DEBUG("profile [%d] config [%d] connect skipped. error: 0x%x\n", profile_index, config_index, qmi_err_num);
            if (profile_index == 1 && !qcmap_wan5g_list[0].manaul_apn) {
                qcmap_update_auto_apn_status(AUTO_APN_INVALID);
                RETRY_UNTIL_SUCCESS(qcmap_update_apn_name, RETRY_TIMES, SLEEP_TIME, profile_index, config_index);
                RETRY_UNTIL_SUCCESS(qcmap_sync_apn_info_with_modem, RETRY_TIMES, SLEEP_TIME, profile_index, config_index);
                if (profile_index <= g_max_apn_num) {
                    RETRY_UNTIL_SUCCESS(qcmap_sync_apn_auth_with_modem, RETRY_TIMES, SLEEP_TIME, profile_index, config_index);
                } else {
                    PRINT_DEBUG("profile [%d] config [%d] apn [%s] can not set auth\n", profile_index, config_index, qcmap_wan5g_list[config_index].apn_name);
                }
            }
        } else {
            qcmap_check_dial_connect(profile_index, config_index);
            PRINT_DEBUG("profile [%d] config [%d] connect success\n", profile_index, config_index);
            qcmap_wan5g_list[config_index].call_type = call_type;
            if (profile_index == 1 && !qcmap_wan5g_list[0].manaul_apn) {
                qcmap_update_auto_apn_status(AUTO_APN_VALID);
            }
            qcmap_update_interface(config_index);
            return true;
        }
    } else {
        if (qmi_err_num == QMI_ERR_OP_DEVICE_UNSUPPORTED_V01)
            PRINT_DEBUG("profile [%d] config [%d] connect fail, call type %d is not supported\n", profile_index, config_index, call_type);
        else
            PRINT_DEBUG("profile [%d] config [%d] connect fail error: 0x%x\n", profile_index, config_index, qmi_err_num);
        if (profile_index == 1 && !qcmap_wan5g_list[0].manaul_apn) {
            qcmap_update_auto_apn_status(AUTO_APN_INVALID);
            RETRY_UNTIL_SUCCESS(qcmap_update_apn_name, RETRY_TIMES, SLEEP_TIME, profile_index, config_index);
            RETRY_UNTIL_SUCCESS(qcmap_sync_apn_info_with_modem, RETRY_TIMES, SLEEP_TIME, profile_index, config_index);
            if (profile_index <= g_max_apn_num) {
                RETRY_UNTIL_SUCCESS(qcmap_sync_apn_auth_with_modem, RETRY_TIMES, SLEEP_TIME, profile_index, config_index);
            } else {
                PRINT_DEBUG("profile [%d] config [%d] apn [%s] can not set auth\n", profile_index, config_index, qcmap_wan5g_list[config_index].apn_name);
            }
        }
    }

    return false;
}

bool qcmap_disable_profile(int profile_index, int config_index) {
    qmi_error_type_v01 qmi_err_num = QMI_ERR_NONE_V01;

    //切换到该profile
    if (!qcmap_client->SetWWANProfileHandlePreference(profile_index, &qmi_err_num)) {
        PRINT_DEBUG("change to profile [%d] config [%d] fail\n", profile_index, config_index);
        return false;
    }

    PRINT_DEBUG("change to profile [%d] config [%d] success\n", profile_index, config_index);

    if (qcmap_client->DisconnectBackHaul(qcmap_wan5g_list[config_index].call_type, &qmi_err_num)) {
        if (qmi_err_num != QMI_ERR_NONE_V01)
            PRINT_DEBUG("profile [%d] config [%d] disconnect skipped. Error: 0x%x\n", profile_index, config_index, qmi_err_num);
        else {
            qcmap_check_dial_disconnect(profile_index, config_index);
            PRINT_DEBUG("profile [%d] config [%d] disconnect success\n", profile_index, config_index);
            return true;
        }
    } else {
        PRINT_DEBUG("profile [%d] config [%d] disconnect fail, error: 0x%x\n", profile_index, config_index, qmi_err_num);
    }
    return false;
}

void qcmap_dial_profile(void) {
    int config_index = 1;

    for (int i = 1; i <= g_max_apn_num; i++) {
        if (i > 1 && i <= g_apn_num)
            continue;
        if ((i == 1 && work_type == WORK_TYPE_MULTI) || (i > 1 && work_type == WORK_TYPE_BASIC))
            continue;
        i == 1 ? config_index = i - 1 : config_index = i - g_apn_num;
        if (qcmap_wan5g_list[config_index].enable) 
            qcmap_enable_profile(i, config_index);
        qcmap_wan5g_list[config_index].update = false;
        qcmap_wan5g_list[config_index].dial_lose_count_v4 = 0;
        qcmap_wan5g_list[config_index].dial_lose_count_v6 = 0;
    }
    PRINT_DEBUG("dial profile finish\n");
}

void qcmap_disdial_profile(void) {
    int config_index = 1;

    for (int i = 1; i <= g_max_apn_num; i++) {
        if (i > 1 && i <= g_apn_num)
            continue;
        i == 1 ? config_index = i - 1 : config_index = i - g_apn_num;
        if (qcmap_wan5g_list[config_index].enable)
            qcmap_disable_profile(i, config_index);
        qcmap_wan5g_list[config_index].update = false;
        qcmap_wan5g_list[config_index].dial_lose_count_v4 = 0;
        qcmap_wan5g_list[config_index].dial_lose_count_v6 = 0;
    }
    PRINT_DEBUG("disdial profile finish\n");
}

//同步network配置到profile
void qcmap_update_profile_config(void) {
    int config_index = 1;

    for (int i = 1; i <= g_max_apn_num; i++) {
        if (i > 1 && i <= g_apn_num)
            continue;
        if ((i == 1 && work_type == WORK_TYPE_MULTI) || (i > 1 && work_type == WORK_TYPE_BASIC))
            continue;
        i == 1 ? config_index = i - 1 : config_index = i - g_apn_num;
        if (qcmap_wan5g_list[config_index].enable) {
            RETRY_UNTIL_SUCCESS(qcmap_update_apn_name, RETRY_TIMES, SLEEP_TIME, i, config_index);
        }
        qcmap_update_profile_index(i, config_index);
    }
}

void qcmap_get_profile_status(qcmap_msgr_wwan_status_enum_v01 *v4_status, qcmap_msgr_wwan_status_enum_v01 *v6_status, int profile_index) {
    qmi_error_type_v01 qmi_err_num = QMI_ERR_NONE_V01;
    qcmap_msgr_wwan_status_enum_v01 eth_status;

    //切换到该profile
    if (!qcmap_client->SetWWANProfileHandlePreference(profile_index, &qmi_err_num)) {
        PRINT_DEBUG("change to profile [%d] fail\n", profile_index);
        return;
    }

    if(!qcmap_client->GetWWANStatusEx(v4_status, v6_status, &eth_status, &qmi_err_num))
        PRINT_DEBUG("profile [%d] WWAN status get fails, Error: 0x%x\n", profile_index, qmi_err_num);
}

//检查 profile 是否掉网，重新拨号 //auto apn list 轮询是否能拨号，如果轮询完还不能拨号则不重拨
void qcmap_check_profile_update(void) {
    bool redial = false;
    int config_index = 1;
    qmi_error_type_v01 qmi_err_num = QMI_ERR_NONE_V01;
    qcmap_msgr_wwan_status_enum_v01 v4_status, v6_status;

    for (int i = 1; i <= g_max_apn_num; i++) {
        if (i > 1 && i <= g_apn_num)
            continue;

        i == 1 ? config_index = i - 1 : config_index = i - g_apn_num;

        if (!qcmap_wan5g_list[config_index].enable)
            return;

        if (i == 1 && !qcmap_wan5g_list[0].manaul_apn && auto_provider_list.auto_provider[auto_provider_list.provider_use_index].status == AUTO_APN_INVALID) {
            break;
        }

        qcmap_get_profile_status(&v4_status, &v6_status, i);
        if ((qcmap_wan5g_list[config_index].ip_family == 4 || qcmap_wan5g_list[config_index].ip_family == 10) && v4_status == QCMAP_MSGR_WWAN_STATUS_DISCONNECTED_V01) {
            //redial = true;
            qcmap_wan5g_list[config_index].dial_lose_count_v4++;
            PRINT_DEBUG("dial lose count v4 index[%d] count[%d]\n", config_index, qcmap_wan5g_list[config_index].dial_lose_count_v4);
        } else if (qcmap_wan5g_list[config_index].ip_family == 6 && v6_status == QCMAP_MSGR_WWAN_STATUS_IPV6_DISCONNECTED_V01) {
            qcmap_wan5g_list[config_index].dial_lose_count_v6++;
            PRINT_DEBUG("dial lose count v6 index[%d] count[%d]\n", config_index, qcmap_wan5g_list[config_index].dial_lose_count_v6);
            //redial = true;
        }

        if (qcmap_wan5g_list[config_index].dial_lose_count_v4 >= 4 || qcmap_wan5g_list[config_index].dial_lose_count_v6 >= 4) {
            qcmap_wan5g_list[config_index].dial_lose_count_v4 = 0;
            qcmap_wan5g_list[config_index].dial_lose_count_v6 = 0;
            redial = true;
        }

        if (redial) {
            PRINT_DEBUG("profile [%d] config [%d] disconnected, redial\n", i, config_index);
            qcmap_enable_profile(i, config_index);
        }
    }
}

bool qcmap_profile_connected(int profile_index, int config_index) {
    bool connected = true;
    qcmap_msgr_wwan_status_enum_v01 v4_status, v6_status;

    qcmap_get_profile_status(&v4_status, &v6_status, profile_index);
    if ((qcmap_wan5g_list[config_index].ip_family == 4 || qcmap_wan5g_list[config_index].ip_family == 10) && v4_status == QCMAP_MSGR_WWAN_STATUS_DISCONNECTED_V01) {
        connected = false;
    } else if (qcmap_wan5g_list[config_index].ip_family == 6 && v6_status == QCMAP_MSGR_WWAN_STATUS_IPV6_DISCONNECTED_V01) {
        connected = false;
    }

    return connected;
}

//检查 network 配置更新 /var/mobile/qcmap_mngr_network_wan5g_x
int qcmap_check_config_update(void) {
    char result[64] = {0};
    char str_tmp[64] = {0};
    char wan_name[16] = {0};
    qcmap_wan5g_config_t wan5g_config_t;
    int update_flag = 0;

    for (int i = 1; i <= QCMAP_MNGR_DEFAULT_PROFILE_NUM; i++) {
        memset(str_tmp, 0, sizeof(str_tmp));
        sprintf(str_tmp, "%s_%d", QCMAP_MNGR_NETWORK_WAN5G_UPDATE, i);
        if (access(str_tmp, F_OK) == 0) {
            PRINT_DEBUG("check file %s\n", str_tmp);
            memset(&wan5g_config_t, 0, sizeof(wan5g_config_t));

            if (i == 1)
                sprintf(wan_name, "%s", QCMAP_MNGR_DEFAULT_WAN_NAME);
            else
                sprintf(wan_name, "%s%d", QCMAP_MNGR_DEFAULT_WAN_NAME, i);

            memset(result, 0, sizeof(result));
            if (qcmap_get_uci_value(QCMAP_MNGR_DEFAULT_CONFIG_FILE, wan_name, QCMAP_MNGR_CONFIG_DISABLE, result, sizeof(result))) {
                wan5g_config_t.enable = !atoi(result);
                if (qcmap_wan5g_list[i - 1].enable != wan5g_config_t.enable) {
                    PRINT_DEBUG("config [%d] enable [%d] change to [%d]\n", i - 1, qcmap_wan5g_list[i - 1].enable, wan5g_config_t.enable);
                    qcmap_wan5g_list[i - 1].enable = wan5g_config_t.enable;
                    qcmap_wan5g_list[i - 1].update = true;
                    update_flag |= 1;
                }
            }
            memset(result, 0, sizeof(result));
            if (qcmap_get_uci_value(QCMAP_MNGR_DEFAULT_CONFIG_FILE, wan_name, QCMAP_MNGR_CONFIG_PROFILE, result, sizeof(result))) {
                wan5g_config_t.profile_index = atoi(result);
                if (qcmap_wan5g_list[i - 1].profile_index != wan5g_config_t.profile_index) {
                    PRINT_DEBUG("config [%d] profile_index [%d] change to [%d]\n", i - 1, qcmap_wan5g_list[i - 1].profile_index, wan5g_config_t.profile_index);
                    qcmap_wan5g_list[i - 1].profile_index = wan5g_config_t.profile_index;
                    qcmap_wan5g_list[i - 1].update = true;
                    update_flag |= 1;
                }
            }
            memset(result, 0, sizeof(result));
            if (qcmap_get_uci_value(QCMAP_MNGR_MOBILE_CONFIG_FILE, wan_name, QCMAP_MNGR_CONFIG_IPTYPE, result, sizeof(result))) {
                wan5g_config_t.ip_family = atoi(result);
                if (qcmap_wan5g_list[i - 1].ip_family != wan5g_config_t.ip_family) {
                    PRINT_DEBUG("config [%d] ip_family [%d] change to [%d]\n", i - 1, qcmap_wan5g_list[i - 1].ip_family, wan5g_config_t.ip_family);
                    qcmap_wan5g_list[i - 1].ip_family = wan5g_config_t.ip_family;
                    qcmap_wan5g_list[i - 1].update = true;
                    update_flag |= 1;
                }
            }
            memset(result, 0, sizeof(result));
            if (qcmap_get_uci_value(QCMAP_MNGR_MOBILE_CONFIG_FILE, wan_name, QCMAP_MNGR_CONFIG_APNNAME, result, sizeof(result))) {
                sprintf(wan5g_config_t.apn_name, "%s", result);
                if (strcmp(qcmap_wan5g_list[i - 1].apn_name, wan5g_config_t.apn_name)) {
                    PRINT_DEBUG("config [%d] apn_name [%s] change to [%s]\n", i - 1, qcmap_wan5g_list[i - 1].apn_name, wan5g_config_t.apn_name);
                    sprintf(qcmap_wan5g_list[i - 1].apn_name, "%s", wan5g_config_t.apn_name);
                    qcmap_wan5g_list[i - 1].update = true;
                    update_flag |= 1;
                }
            }
            memset(result, 0, sizeof(result));
            if (qcmap_get_uci_value(QCMAP_MNGR_DEFAULT_CONFIG_FILE, wan_name, QCMAP_MNGR_CONFIG_LOCKMTU, result, sizeof(result))) {
                wan5g_config_t.lockmtu = atoi(result);
                if (qcmap_wan5g_list[i - 1].lockmtu != wan5g_config_t.lockmtu) {
                    PRINT_DEBUG("config [%d] lockmtu [%d] change to [%d]\n", i - 1, qcmap_wan5g_list[i - 1].lockmtu, wan5g_config_t.lockmtu);
                    qcmap_wan5g_list[i - 1].lockmtu = wan5g_config_t.lockmtu;
                    qcmap_wan5g_list[i - 1].update = true;
                    update_flag |= 1;
                }
            }
            memset(result, 0, sizeof(result));
            if (qcmap_get_uci_value(QCMAP_MNGR_MOBILE_CONFIG_FILE, wan_name, QCMAP_MNGR_CONFIG_USERNAME, result, sizeof(result))) {
                sprintf(wan5g_config_t.username, "%s", result);
                if (strcmp(qcmap_wan5g_list[i - 1].username, wan5g_config_t.username)) {
                    PRINT_DEBUG("config [%d] username [%s] change to [%s]\n", i - 1, qcmap_wan5g_list[i - 1].username, wan5g_config_t.username);
                    sprintf(qcmap_wan5g_list[i - 1].username, "%s", wan5g_config_t.username);
                    qcmap_wan5g_list[i - 1].update = true;
                    update_flag |= 1;
                }
            }
            memset(result, 0, sizeof(result));
            if (qcmap_get_uci_value(QCMAP_MNGR_MOBILE_CONFIG_FILE, wan_name, QCMAP_MNGR_CONFIG_PASSWORD, result, sizeof(result))) {
                sprintf(wan5g_config_t.password, "%s", result);
                if (strcmp(qcmap_wan5g_list[i - 1].password, wan5g_config_t.password)) {
                    PRINT_DEBUG("config [%d] password [%s] change to [%s]\n", i - 1, qcmap_wan5g_list[i - 1].password, wan5g_config_t.password);
                    sprintf(qcmap_wan5g_list[i - 1].password, "%s", wan5g_config_t.password);
                    qcmap_wan5g_list[i - 1].update = true;
                    update_flag |= 1;
                }
            }
            memset(result, 0, sizeof(result));
            if (qcmap_get_uci_value(QCMAP_MNGR_MOBILE_CONFIG_FILE, wan_name, QCMAP_MNGR_CONFIG_AUTH, result, sizeof(result))) {
                wan5g_config_t.auth = qcmap_auth_str_to_int(result);
                if (qcmap_wan5g_list[i - 1].auth != wan5g_config_t.auth) {
                    PRINT_DEBUG("config [%d] auth [%d] change to [%d]\n", i - 1, qcmap_wan5g_list[i - 1].auth, wan5g_config_t.auth);
                    qcmap_wan5g_list[i - 1].auth = wan5g_config_t.auth;
                    qcmap_wan5g_list[i - 1].update = true;
                    update_flag |= 1;
                }
            }
            memset(result, 0, sizeof(result));
            if (qcmap_get_uci_value(QCMAP_MNGR_MOBILE_CONFIG_FILE, wan_name, QCMAP_MNGR_CONFIG_DEFAULTROUTE, result, sizeof(result))) {
                wan5g_config_t.defaultroute = atoi(result);
                if (qcmap_wan5g_list[i - 1].defaultroute != wan5g_config_t.defaultroute) {
                    PRINT_DEBUG("config [%d] defaultroute [%d] change to [%d]\n", i - 1, qcmap_wan5g_list[i - 1].defaultroute, wan5g_config_t.defaultroute);
                    qcmap_wan5g_list[i - 1].defaultroute = wan5g_config_t.defaultroute;
                    update_default_route = true;
                    update_flag |= 1;
                }
            }
            if (i == 1) {
                memset(result, 0, sizeof(result));
                if (qcmap_get_uci_value(QCMAP_MNGR_MOBILE_CONFIG_FILE, wan_name, QCMAP_MNGR_CONFIG_MANUALAPN, result, sizeof(result))) {
                    wan5g_config_t.manaul_apn = atoi(result);
                    if (qcmap_wan5g_list[i - 1].manaul_apn != wan5g_config_t.manaul_apn) {
                        PRINT_DEBUG("config [%d] manaul_apn [%d] change to [%d]\n", i - 1, qcmap_wan5g_list[i - 1].manaul_apn, wan5g_config_t.manaul_apn);
                        qcmap_wan5g_list[i - 1].manaul_apn = wan5g_config_t.manaul_apn;
                        qcmap_wan5g_list[i - 1].update = true;
                        update_flag |= 1;
                    }
                }
                memset(result, 0, sizeof(result));
                if (qcmap_get_uci_value(QCMAP_MNGR_MOBILE_CONFIG_FILE, wan_name, QCMAP_MNGR_CONFIG_NETTYPE, result, sizeof(result))) {
                    sprintf(wan5g_config_t.nettype, "%s", result);
                    if (strcmp(qcmap_wan5g_list[i - 1].nettype, wan5g_config_t.nettype)) {
                        PRINT_DEBUG("config [%d] nettype [%s] change to [%s]\n", i - 1, qcmap_wan5g_list[i - 1].nettype, wan5g_config_t.nettype);
                        sprintf(qcmap_wan5g_list[i - 1].nettype, "%s", wan5g_config_t.nettype);
                        qcmap_wan5g_list[i - 1].update = true;
                        update_flag |= 1;
                        qcmap_enable_state_machine();
                        qcmap_reset_signal();
                    }
                }
                memset(result, 0, sizeof(result));
                if (qcmap_get_uci_value(QCMAP_MNGR_MOBILE_CONFIG_FILE, wan_name, QCMAP_MNGR_CONFIG_SUBNETTYPE, result, sizeof(result))) {
                    sprintf(wan5g_config_t.subnettype, "%s", result);
                    if (strcmp(qcmap_wan5g_list[i - 1].subnettype, wan5g_config_t.subnettype)) {
                        PRINT_DEBUG("config [%d] subnettype [%s] change to [%s]\n", i - 1, qcmap_wan5g_list[i - 1].subnettype, wan5g_config_t.subnettype);
                        sprintf(qcmap_wan5g_list[i - 1].subnettype, "%s", wan5g_config_t.subnettype);
                        qcmap_wan5g_list[i - 1].update = true;
                        update_flag |= 1;
                        qcmap_enable_state_machine();
                        qcmap_reset_signal();
                    }
                }
            }
            unlink(str_tmp);
        }
    }

    return update_flag;
}

int qcmap_find_zone_index(char *zone_name) {
    if (!zone_name) 
    {
        PRINT_DEBUG("error, zone name is null\n");
        return -1;
    }

    FILE *fp = popen("/etc/data/uci_ex.sh show firewall", "r"); // 执行uci命令
    if (!fp) return -1;

    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    int found_index = -1;

    // 逐行读取uci输出
    while ((read = getline(&line, &len, fp)) != -1) {
        // 去除行尾换行符
        if (line[read-1] == '\n') line[read-1] = '\0';

        // 解析键值对
        char *equal_pos = strchr(line, '=');
        if (!equal_pos) continue;

        // 提取键和值
        *equal_pos = '\0';
        char *key = line;
        char *value = equal_pos + 1;

        // 检查是否为zone的name字段
        int index;
        if (sscanf(key, "firewall.@zone[%d].name", &index) != 1) continue;

        // 提取引号内的值
        char *start_quote = strchr(value, '\'');
        if (!start_quote) continue;
        char *end_quote = strchr(start_quote+1, '\'');
        if (!end_quote) continue;

        // 构造字符串进行比较
        char actual_value[end_quote - start_quote];
        strncpy(actual_value, start_quote+1, end_quote - start_quote -1);
        actual_value[end_quote - start_quote -1] = '\0';

        if (strcmp(actual_value, zone_name) == 0) {
            found_index = index;
            break;
        }
    }

    // 清理资源
    free(line);
    pclose(fp);
    return found_index;
}

int qcmap_apply_nat(int index) {
    char uci_section[16] = {0};
    char uci_path[32] = {0};
    char interface_list[128] = {0};
    int ret = 0;

    if (index >= 0) {
        sprintf(uci_section, "@zone[%d]", index);
        sprintf(uci_path, "firewall.%s.network", uci_section);
    }

    // 获取firewall.@zone[%d].network中的接口
    qcmap_get_uci_value("firewall", uci_section, "network", interface_list, sizeof(interface_list));
    PRINT_DEBUG("nat interface list [%s]\n", interface_list);

    // 判断nat_enable
    PRINT_DEBUG("nat_enable=%d\n", qcmap_wan5g_list[0].nat_enable);
    if (qcmap_wan5g_list[0].nat_enable == 0) {
        // 如果为0，判断interface_list是否为空，不为空则清除掉(qcmap_uci_dellist)，返回1，为空返回0
        PRINT_DEBUG("NAT disenabled, processing interface pairs\n");
        if (strlen(interface_list) > 0) {
            // 清除所有可能的接口
            const char* interfaces[] = {
                QCMAP_MNGR_DEFAULT_WAN_NAME,
                QCMAP_MNGR_DEFAULT_WAN_NAME"2",
                QCMAP_MNGR_DEFAULT_WAN_NAME"3",
                QCMAP_MNGR_DEFAULT_WAN_NAME"_v6",
                QCMAP_MNGR_DEFAULT_WAN_NAME"2_v6",
                QCMAP_MNGR_DEFAULT_WAN_NAME"3_v6"
            };
            
            for (int i = 0; i < sizeof(interfaces)/sizeof(interfaces[0]); i++) {
                if (strstr(interface_list, interfaces[i]) != NULL) {
                    qcmap_uci_dellist(uci_path, interfaces[i]);
                    ret = 1;
                }
            }
            if (ret) {
                PRINT_DEBUG("%s nat to reload disable\n", uci_path);
            }
        }
        return ret;
    } else {
        // 如果为1，判断对应接口是否在接口列表中
        PRINT_DEBUG("NAT enabled, processing interface pairs\n");
        const char* interface_pairs[][2] = {
            {QCMAP_MNGR_DEFAULT_WAN_NAME, QCMAP_MNGR_DEFAULT_WAN_NAME"_v6"},
            {QCMAP_MNGR_DEFAULT_WAN_NAME"2", QCMAP_MNGR_DEFAULT_WAN_NAME"2_v6"},
            {QCMAP_MNGR_DEFAULT_WAN_NAME"3", QCMAP_MNGR_DEFAULT_WAN_NAME"3_v6"}
        };
        
        PRINT_DEBUG("checking %zu interface pairs\n", sizeof(interface_pairs)/sizeof(interface_pairs[0]));
        for (int i = 0; i < sizeof(interface_pairs)/sizeof(interface_pairs[0]); i++) {
            const char* interface_v4 = interface_pairs[i][0];
            const char* interface_v6 = interface_pairs[i][1];

            // 检查接口是否在列表中
            bool in_list_v4 = (strstr(interface_list, interface_v4) != NULL);
            bool in_list_v6 = (strstr(interface_list, interface_v6) != NULL);

            // 检查接口是否enable
            bool enable = qcmap_wan5g_list[i].enable;
            
            if (in_list_v4) {
                // 在列表中。如果接口enable，不做操作；如果disable，删除列表中这一项
                if (!enable) {
                    PRINT_DEBUG("removing v4 interface %s (disabled)\n", interface_v4);
                    qcmap_uci_dellist(uci_path, interface_v4);
                    ret = 1;
                } else {
                    PRINT_DEBUG("v4 interface %s already in list and enabled, no action\n", interface_v4);
                }
            } else {
                // 不在列表中。如果enable，添加接口到列表中；如果disable，不做操作
                if (enable) {
                    PRINT_DEBUG("adding v4 interface %s (enabled)\n", interface_v4);
                    qcmap_uci_addlist(uci_path, interface_v4);
                    ret = 1;
                } else {
                    PRINT_DEBUG("v4 interface %s not in list but disabled, no action\n", interface_v4);
                }
            }
            
            if (in_list_v6) {
                // 在列表中。如果接口enable，不做操作；如果disable，删除列表中这一项
                if (!enable) {
                    PRINT_DEBUG("removing v6 interface %s (disabled)\n", interface_v6);
                    qcmap_uci_dellist(uci_path, interface_v6);
                    ret = 1;
                } else {
                    PRINT_DEBUG("v6 interface %s already in list and enabled, no action\n", interface_v6);
                }
            } else {
                // 不在列表中。如果enable，添加接口到列表中；如果disable，不做操作
                if (enable) {
                    PRINT_DEBUG("adding v6 interface %s (enabled)\n", interface_v6);
                    qcmap_uci_addlist(uci_path, interface_v6);
                    ret = 1;
                } else {
                    PRINT_DEBUG("v6 interface %s not in list but disabled, no action\n", interface_v6);
                }
            }
        }
        
        if (ret) {
            PRINT_DEBUG("%s nat to reload enable\n", uci_path);
        } else {
            PRINT_DEBUG("no changes needed for NAT enable\n");
        }
        return ret;
    }

    return 0;
}

void qcmap_init_nat(void) {
    int ret = 0;
    char *zone_wan_all = "wan_all";

    ret |= qcmap_apply_nat(qcmap_find_zone_index(zone_wan_all));

    if (ret)
        qcmap_set_update_firewall(true);
}

int qcmap_check_apply_nat(void) {
    int ret = 0;
    int nat_enable = 0;
    char result[64] = {0};
    char str_tmp[64] = {0};
    char *zone_wan_all = "wan_all";
    int update_nat = 0;

    if (access(QCMAP_MNGR_NETWORK_WAN5G_NAT, F_OK) == 0) {
        if (qcmap_get_uci_value(QCMAP_MNGR_MOBILE_CONFIG_FILE, QCMAP_MNGR_DEFAULT_WAN_NAME, QCMAP_MNGR_CONFIG_NATENABLE, result, sizeof(result))) {
            nat_enable = atoi(result);
            if (qcmap_wan5g_list[0].nat_enable != nat_enable) {
                PRINT_DEBUG("nat_enable [%d] change to [%d]\n", qcmap_wan5g_list[0].nat_enable, nat_enable);
                qcmap_wan5g_list[0].nat_enable = nat_enable;
                ret |= qcmap_apply_nat(qcmap_find_zone_index(zone_wan_all));
                update_nat = 1;
            }
        }
        unlink(QCMAP_MNGR_NETWORK_WAN5G_NAT);
    }

    if (ret) {
        system_ex("conntrack -D --proto icmp", 0);
        qcmap_set_update_firewall(true);
    }

    return update_nat;
}

// 设置默认路由项，1：如果只有一项为1，且该项所处接口已使能，设置该接口为默认路由。 2：如果多项为1，根据已使能并为1的顺序，设置第一个接口为1
void qcmap_init_default_route(void) {
    // 初始化时重置检测计数
    default_route_check_count = 0;
    update_default_route = true;

    qcmap_check_and_set_default_route(true);
}

// 更新默认路由项，用于定时器检查
void qcmap_update_default_route(void) {
    char gateway[32] = {0};

    if (!update_default_route) {
        if (!has_default_route) {
            default_route_check_count = 0;
            return;
        }
        get_default_route(gateway);
        if (strlen(gateway) <= 0) {
            PRINT_DEBUG("check gateway is null, go to set default gateway\n");
        } else {
            default_route_check_count = 0;
            return;
        }
    }

    // 检查是否超过最大检测次数
    if (default_route_check_count >= 20) {
        PRINT_DEBUG("default route check count exceeded 20, reset\n");
        update_default_route = false;
        default_route_check_count = 0;
        qcmap_init_cfun(true);
        qcmap_enable_state_machine(); 
        qcmap_reset_signal();
        qcmap_update_state_machine_status(STATE_MACHINE_NONE);
        return;
    }

    // 增加检测计数
    default_route_check_count++;

    qcmap_check_and_set_default_route(false);
}

// 更新dns链接，用于定时器检查
void qcmap_update_dns_link(void) {
    const char *tmp_resolv = "/tmp/resolv.conf";
    const char *target = "/tmp/resolv.conf.d/resolv.conf.auto";
    struct stat st;

    // 检查 /tmp/resolv.conf
    if (lstat(tmp_resolv, &st) == 0) {
        if (!S_ISLNK(st.st_mode)) {
            PRINT_DEBUG("/tmp/resolv.conf not link, relink\n");
            unlink(tmp_resolv);
            symlink(target, tmp_resolv);
        }
    } else {
        // 文件不存在，创建链接
        PRINT_DEBUG("/tmp/resolv.conf not exist, relink %s\n", target);
        symlink(target, tmp_resolv);
    }
}

// 检查并设置默认路由的公共函数
bool qcmap_check_and_set_default_route(bool is_init) {
    bool update = false;
    char wan_name[16] = {0};
    char command[256] = {0};
    char gateway[32] = {0};
    char gateway_old[32] = {0};
    inf_status_t inf_status;
    bool route_update = false;
    bool has_default_route_local = false;

    // 遍历所有WAN接口配置
    for (int i = 1; i <= QCMAP_MNGR_DEFAULT_PROFILE_NUM; i++) {
        // 构造接口名称
        if (i == 1) {
            if(qcmap_get_bridge_enable(0)) {
                continue;
            }
            sprintf(wan_name, "%s", QCMAP_MNGR_DEFAULT_WAN_NAME);
        } else {
            sprintf(wan_name, "%s%d", QCMAP_MNGR_DEFAULT_WAN_NAME, i);
        }

        qcmap_wan5g_config_t *current_config = &qcmap_wan5g_list[i - 1];
        // 检查接口是否启用且配置为默认路由
        if (current_config->enable && current_config->defaultroute) {
            has_default_route_local = true;
            // 记录调试信息
            if (is_init)
                PRINT_DEBUG("init check interface [%s] status\n", wan_name);
            else
                PRINT_DEBUG("update check interface [%s] status, check count: %d\n", wan_name, default_route_check_count);
            // 获取接口状态信息
            memset(&inf_status, 0, sizeof(inf_status));
            qcmap_get_interface_info(wan_name, &inf_status);
            // 检查接口是否up,等待下次检查，直至20次
            if (!inf_status.status) {
                update_default_route = true;
                PRINT_DEBUG("interface [%s] not up yet\n", wan_name);
                return false;
            }
            // 记录接口状态
            PRINT_DEBUG("%s check interface [%s] status up\n", is_init ? "init" : "update", inf_status.l3_device);

            // 获取接口的默认路由网关地址
            PRINT_DEBUG("device [%s] address [%s] nexthope [%s]\n", inf_status.l3_device, inf_status.ipv4_address, inf_status.nexthop);
            if (strlen(inf_status.ipv4_address) > 0 && strlen(inf_status.nexthop) > 0 && strcmp(inf_status.nexthop, "0.0.0.0") != 0) {
                sprintf(gateway, "%s", inf_status.nexthop);
            } else {
                get_ip_from_route(inf_status.l3_device, inf_status.ipv4_address, gateway);
            }
            if (strlen(inf_status.l3_device) > 0 && strlen(gateway) > 0 && qcmap_check_ipv4_address(gateway)) {
                get_default_route(gateway_old);// 获取当前系统默认路由
                PRINT_DEBUG("%s old gateway [%s] new gateway [%s]\n", is_init ? "init" : "update", gateway_old, gateway);
                if(strlen(gateway_old) > 0 && strcmp(gateway, gateway_old) != 0) {// 如果网关不同，更新默认路由
                    system_ex("route del default gw 0.0.0.0", 0);
                    route_update = true;
                } else if (strlen(gateway_old) <= 0){
                    route_update = true;
                }
                if (route_update) {
                    memset(command, 0, sizeof(command));
                    sprintf(command, "ip route add default via %s dev %s proto static src %s metric 256", gateway, inf_status.l3_device, inf_status.ipv4_address);
                    system_ex(command, 0);
                }
            }
            // 更新状态
            update_default_route = false;
            if (!is_init) {
                default_route_check_count = 0;
            }

            return true;
        }
    }

    has_default_route = has_default_route_local;

    // 没有找到合适的接口
    if (has_default_route) {
        update_default_route = true;
    } else {
        update_default_route = false;
    }

    return false;
}

//处理更新的配置 -- 同步到profile并做对应动作
void qcmap_apply_config_update(void) {
    bool update = false;
    int nat_config = 0;
    int ip_family = 0;
    int config_index = 1;
    char *username = nullptr, *password = nullptr;
    qcmap_wan5g_config_t config;
    qcmap_net_profile_and_policy_info profile_info;

    // if (qcmap_get_state_machine_running_status()) {
    //     return;
    // }

    for (int i = 1; i <= g_max_apn_num; i++) {
        if (i > 1 && i <= g_apn_num)
            continue;

        i == 1 ? config_index = i - 1 : config_index = i - g_apn_num;
        qcmap_wan5g_config_t *current_config = &qcmap_wan5g_list[config_index];
        if (!current_config->update)
            continue;

        PRINT_DEBUG("Processing profile [%d] config [%d]\n", i, config_index);
        memset(&config, 0, sizeof(config));
        memset(&profile_info, 0, sizeof(profile_info));
        if (!qcmap_get_state_machine_running_status()) {
            if (!qcmap_get_profile_info(&profile_info, i) || !qcmap_get_apn_auth_info(config_index, &config) || !qcmap_get_profile_ip_family(&ip_family, config_index)) {
                PRINT_DEBUG("Failed to get profile [%d] config [%d] info\n", i, config_index);
                continue;
            }
        }

        PRINT_DEBUG("Applying changes for profile [%d] config [%d]\n", i, config_index);
        if (!qcmap_get_state_machine_running_status()) {
            nat_config = 0;
            if (qcmap_profile_connected(i, config_index)) {
                qcmap_disable_profile(i, config_index);
            } else {
                nat_config = 1;
            }
        }

        // 自动APN处理逻辑
        provider_t *auto_apn = nullptr;
        bool is_auto_apn = (i == 1 && !current_config->manaul_apn);
        if (is_auto_apn) {
            auto_apn = qcmap_get_auto_apn();
        }

        // 统一APN名称处理
        const char *target_apn = is_auto_apn && auto_apn ? auto_apn->apn : current_config->apn_name;
        if (strcmp(profile_info.apn_name, target_apn) != 0 || ip_family != current_config->ip_family) {
            if (!qcmap_get_state_machine_running_status()) {
                RETRY_UNTIL_SUCCESS(qcmap_update_apn_name, RETRY_TIMES, SLEEP_TIME, i, config_index);
            }
            RETRY_UNTIL_SUCCESS(qcmap_sync_apn_info_with_modem, RETRY_TIMES, SLEEP_TIME, i, config_index);
        }

        // 统一认证信息处理
        bool auth_changed = false;
        if (is_auto_apn && auto_apn) {
            username = auto_apn->username;
            password = auto_apn->password;
        } else {
            username = current_config->username;
            password = current_config->password;
        }

        auth_changed =(config.auth != current_config->auth ||
            strcmp(config.username, username) != 0 ||
            strcmp(config.password, password) != 0);

        if (auth_changed) {
            if (i <= g_max_apn_num) {
                RETRY_UNTIL_SUCCESS(qcmap_sync_apn_auth_with_modem, RETRY_TIMES, SLEEP_TIME, i, config_index);
            } else {
                PRINT_DEBUG("profile [%d] config [%d] apn [%s] can not set auth\n", i, config_index, qcmap_wan5g_list[config_index].apn_name);
            }
        }

        // 重新启用配置 todo
        if (!qcmap_get_state_machine_running_status()) {
            if (current_config->enable) {
                qcmap_enable_profile(i, config_index);
            }

            if (nat_config) {
                qcmap_init_nat();
            }
        }

        current_config->update = false;
    }
}

void qcmap_init_firewall(void) {
    if (qcmap_get_update_firewall()) {
        qcmap_set_update_firewall(false);
        system_ex("/etc/init.d/firewall restart", 0);
    }
}

//检查各种配置更新情况并同步信息后重新拨号
int qcmap_timer_check_config(void) {
    int ret = 0;
    //检查 network 配置更新
    if (qcmap_check_config_update()) {
        //处理更新的配置 -- 同步到profile并做对应动作
        qcmap_apply_config_update();
        ret |= 1;
    }

    //检查bridge更新
    if (qcmap_check_apply_bridge()) {
        ret |= 1;
    }

    if (qcmap_check_apply_nat()) {
        ret |= 1;
    }

    qcmap_init_firewall();

    return ret;
}

void qcmap_timer_check_profile(void) {
    static unsigned int s_last_check_time = 0;
    unsigned int now_time = qcmap_get_uptime_in_ms();

    if (POS_DIFF_VAL(now_time, s_last_check_time) < MOD_STATUS_UPDATE_INTERVAL) {
        return;
    }

    s_last_check_time = now_time;

    //检查 profile 是否掉网，重新拨号
    qcmap_check_profile_update();
}