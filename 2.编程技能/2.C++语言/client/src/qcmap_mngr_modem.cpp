#include "qcmap_mngr_modem.h"
#include "qcmap_mngr_config.h"
#include "qcmap_mngr_led.h"
#include "qcmap_mngr_apn.h"
#include "qcmap_mngr_lock.h"
#include "qcmap_mngr_info.h"

#define WRITE_SIM_STATUS(status) write_buf_to_file(SIM_STATUS_FILE, status, strlen(status))
#define WRITE_PINLOCK_STATUS(status) write_buf_to_file(PINLOCK_STATUS_FILE, status, strlen(status))

static int modem_status = MODEM_NONE;

void qcmap_release_modem(void) {
    int config_index = 1;
    qmi_error_type_v01 qmi_err_num = QMI_ERR_NONE_V01;

    modem_status = MODEM_NONE;
    if (qcmap_client) {
        for (int i = 1; i <= g_max_apn_num; i++) {
            if (i > 1 && i <= g_apn_num)
                continue;
            i == 1 ? config_index = i - 1 : config_index = i - g_apn_num;
            if (qcmap_profile_connected(i, config_index))
                qcmap_disable_profile(i, config_index);
        }

        if (qcmap_client->DisableMobileAP(&qmi_err_num))
            PRINT_DEBUG("mobile disable success\n");
        else
            PRINT_DEBUG("mobile disable fail, error: 0x%x\n", qmi_err_num);
    }
}

int qcmap_get_ims_status(void) {
    char result[QCMAP_MAX_STRING_LEN] = {0};
    char command[QCMAP_MAX_STRING_LEN] = {0};

    memset(result, 0, sizeof(result));
    memset(command, 0, sizeof(command));
    snprintf(command, QCMAP_MAX_STRING_LEN, "%s %s?", QCMAP_MNGR_AT_MNGR_COMMAND, QCMAP_MNGR_AT_COMMAND_CAVIMS);
    if (execute_cmd((const char*)command, result, sizeof(result))) {
        if (strcasestr(result, "+CAVIMS: 1")) {
            return 1;
        } else if (strcasestr(result, "+CAVIMS: 0")) {
            return 0;
        } else {
            return 1;
        }
    }
}

void qcmap_set_ims_status(int status) {
    char result[QCMAP_MAX_STRING_LEN] = {0};
    char command[QCMAP_MAX_STRING_LEN] = {0};

    memset(result, 0, sizeof(result));
    memset(command, 0, sizeof(command));
    snprintf(command, QCMAP_MAX_STRING_LEN, "%s %s=%d", QCMAP_MNGR_AT_MNGR_COMMAND, QCMAP_MNGR_AT_COMMAND_CAVIMS, status);
    if (execute_cmd((const char*)command, result, sizeof(result))) {
        PRINT_DEBUG("result:[%s]\n", result);
        if (strcasestr(result, "OK")) {
            PRINT_DEBUG("%s success\n", command);
        } else {
            PRINT_DEBUG("%s fail\n", command);
        }
    }
}

void qcmap_init_ims(int status) {
    int ims_status = 0;//default 0

    ims_status = qcmap_get_ims_status();
    if (ims_status != status) {
        PRINT_DEBUG("ims status old:[%d] new:[%d]\n", ims_status, status);
        qcmap_set_ims_status(status);
    } else {
        PRINT_DEBUG("ims already disabled\n");
    }
}

//同步profile信息到modem侧
void qcmap_sync_profile_with_modem(void) {
    int config_index = 1;

    for (int i = 1; i <= g_max_apn_num; i++) {
        if (i > 1 && i <= g_apn_num)
            continue;
        if ((i == 1 && work_type == WORK_TYPE_MULTI) || (i > 1 && work_type == WORK_TYPE_BASIC))
            continue;
        i == 1 ? config_index = i - 1 : config_index = i - g_apn_num;
        if (qcmap_wan5g_list[config_index].enable) {
            RETRY_UNTIL_SUCCESS(qcmap_sync_apn_info_with_modem, RETRY_TIMES, SLEEP_TIME, i, config_index);
            if (i <= g_max_apn_num) {
                RETRY_UNTIL_SUCCESS(qcmap_sync_apn_auth_with_modem, RETRY_TIMES, SLEEP_TIME, i, config_index);
            } else {
                PRINT_DEBUG("profile [%d] config [%d] apn [%s] can not set auth\n", i, config_index, qcmap_wan5g_list[config_index].apn_name);
            }
        }
    }
}

//同步注网类型到modem侧
bool qcmap_sync_nettype_with_modem(void) {
    char nettype[16] = {0};
    char param[16] = {0};
    char result[QCMAP_MAX_STRING_LEN] = {0};
    char command[QCMAP_MAX_STRING_LEN] = {0};

    if (!qcmap_get_nettype_with_modem(nettype)) {
        PRINT_DEBUG("get nettype in modem fail\n");
        return false;
    }

    if (!strcmp(qcmap_wan5g_list[0].nettype, "AUTO")) {
        sprintf(param, "%s", "10");
    } else if (!strcmp(qcmap_wan5g_list[0].nettype, "5G")) {
        if (!strcmp(qcmap_wan5g_list[0].subnettype, "AUTO")) {
            sprintf(param, "%s", "17,6");
        } else if (!strcmp(qcmap_wan5g_list[0].subnettype, "SA")) {
            sprintf(param, "%s", "14");
        } else if (!strcmp(qcmap_wan5g_list[0].subnettype, "NSA")) {
            sprintf(param, "%s", "17,6");
        } else {
            PRINT_DEBUG("nettype [%s] subnettype [%s] not suppport\n", qcmap_wan5g_list[0].nettype, qcmap_wan5g_list[0].subnettype);
            return true;
        }
        PRINT_DEBUG("setting nettype [%s] subnettype [%s]\n", qcmap_wan5g_list[0].nettype, qcmap_wan5g_list[0].subnettype);
    } else {
        if (!strcmp(qcmap_wan5g_list[0].nettype, "4G")) {
            sprintf(param, "%s", "3");
        } else if (!strcmp(qcmap_wan5g_list[0].nettype, "3G")) {
            sprintf(param, "%s", "2");
        } else {
            PRINT_DEBUG("nettype [%s] not suppport\n", qcmap_wan5g_list[0].nettype);
            return true;
        }
        PRINT_DEBUG("setting nettype [%s]\n", qcmap_wan5g_list[0].nettype);
    }

    // 保存nettype_num参数
    strncpy(qcmap_wan5g_list[0].nettype_num, param, sizeof(qcmap_wan5g_list[0].nettype_num) - 1);
    qcmap_wan5g_list[0].nettype_num[sizeof(qcmap_wan5g_list[0].nettype_num) - 1] = '\0';
    PRINT_DEBUG("saved nettype_num: [%s]\n", qcmap_wan5g_list[0].nettype_num);

    if (!strcmp(nettype, param)) {
        PRINT_DEBUG("modem nettype [%s] is same with config nettype [%s], not need to setting\n", nettype, param);
        return true;
    }

    snprintf(command, QCMAP_MAX_STRING_LEN, "%s \'%s=%s\'", QCMAP_MNGR_AT_MNGR_COMMAND, QCMAP_MNGR_AT_COMMAND_GTRAT, param);
    if (execute_cmd((const char*)command, result, sizeof(result))) {
        PRINT_DEBUG("result:[%s]\n", result);
        if (strcasestr(result, "OK")) {
            PRINT_DEBUG("set nettype success, move to next step\n");
            return true;
        }
    }

    PRINT_DEBUG("set nettype fail, move to next step\n");
    return false;
}

bool qcmap_get_nettype_with_modem(char *value) {
    char param[16] = {0};
    char result[QCMAP_MAX_STRING_LEN] = {0};
    char command[QCMAP_MAX_STRING_LEN] = {0};
    bool success = false;

    snprintf(command, QCMAP_MAX_STRING_LEN, "%s -i 2 %s?", QCMAP_MNGR_AT_MNGR_COMMAND, QCMAP_MNGR_AT_COMMAND_GTRAT);
    if (execute_cmd((const char*)command, result, sizeof(result))) {
        PRINT_DEBUG("result:[%s]\n", result);
        
        // 解析 +GTRAT: 后面的值
        char *gtrat_line = strstr(result, "+GTRAT:");
        if (gtrat_line != NULL) {
            // 跳过 "+GTRAT: " 前缀
            char *value_start = gtrat_line + strlen("+GTRAT:");
            // 去除前导空格
            while (*value_start == ' ') value_start++;
            
            // 找到行尾（换行符或字符串结束）
            char *value_end = value_start;
            while (*value_end != '\0' && *value_end != '\r' && *value_end != '\n') {
                value_end++;
            }
            
            // 计算值的长度
            size_t value_len = value_end - value_start;
            if (value_len > 0) {
                // 确保不超过目标缓冲区大小
                if (value_len >= QCMAP_MAX_STRING_LEN) {
                    value_len = QCMAP_MAX_STRING_LEN - 1;
                }
                strncpy(value, value_start, value_len);
                value[value_len] = '\0';
                PRINT_DEBUG("Extracted nettype value: [%s]\n", value);
                success = true;
            } else {
                strcpy(value, "");
                PRINT_DEBUG("No nettype value found\n");
            }
        } else {
            strcpy(value, "");
            PRINT_DEBUG("GTRAT response not found\n");
        }
    } else {
        strcpy(value, "");
        PRINT_DEBUG("Execute command failed\n");
    }

    return success;
}

int qcmap_check_registration(void) {
    modem_network_t regstatus = NETWORK_NONE;
    char result[QCMAP_MAX_STRING_LEN] = {0};
    char command[QCMAP_MAX_STRING_LEN] = {0};

    memset(result, 0, sizeof(result));
    memset(command, 0, sizeof(command));
    snprintf(command, QCMAP_MAX_STRING_LEN, "%s -i 5 %s?", QCMAP_MNGR_AT_MNGR_COMMAND, QCMAP_MNGR_AT_COMMAND_C5GREG);
    if (execute_cmd((const char*)command, result, sizeof(result))) {
        if (strcasestr(result, "OK") && (strstr(result, ",1") || strstr(result, ",5"))) {
            regstatus = NETWORK_REGISTERED;
            goto func_end;
        } else if (strcasestr(result, "Receive timed out")) {
            regstatus = NETWORK_TIMEOUT;
        } else {
            PRINT_DEBUG("network is not registered, check again\n");
        }
    }

    memset(result, 0, sizeof(result));
    memset(command, 0, sizeof(command));
    snprintf(command, QCMAP_MAX_STRING_LEN, "%s -i 5 %s?", QCMAP_MNGR_AT_MNGR_COMMAND, QCMAP_MNGR_AT_COMMAND_CEREG);
    if (execute_cmd((const char*)command, result, sizeof(result))) {
        if (strcasestr(result, "OK") && (strstr(result, ",1") || strstr(result, ",5"))) {
            regstatus = NETWORK_REGISTERED;
            goto func_end;
        } else if (strcasestr(result, "Receive timed out")) {
            regstatus = NETWORK_TIMEOUT;
        } else {
            PRINT_DEBUG("network is not registered, check again\n");
            PRINT_DEBUG("result:[%s]\n", result);
        }
    }

    memset(result, 0, sizeof(result));
    memset(command, 0, sizeof(command));
    snprintf(command, QCMAP_MAX_STRING_LEN, "%s -i 5 %s?", QCMAP_MNGR_AT_MNGR_COMMAND, QCMAP_MNGR_AT_COMMAND_CREG);
    if (execute_cmd((const char*)command, result, sizeof(result))) {
        if (strcasestr(result, "OK") && (strstr(result, ",1") || strstr(result, ",5"))) {
            regstatus = NETWORK_REGISTERED;
            goto func_end;
        } else if (strcasestr(result, "Receive timed out")) {
            regstatus = NETWORK_TIMEOUT;
        } else {
            PRINT_DEBUG("network is not registered, check again\n");
            PRINT_DEBUG("result:[%s]\n", result);
        }
    }

func_end:
    write_buf_to_file(SIM_STATUS_FILE, (regstatus == NETWORK_REGISTERED) ? "Registered" : "Not Registered", strlen((regstatus == NETWORK_REGISTERED) ? "Registered" : "Not Registered"));

    return regstatus;
}

void qcmap_release_interface(void) {
    FILE *fp = fopen("/proc/net/dev", "r");
    if (!fp) {
        PRINT_DEBUG("Error opening /proc/net/dev\n");
        return;
    }

    char line[256];
    // 跳过前两行标题
    fgets(line, sizeof(line), fp);
    fgets(line, sizeof(line), fp);

    char interfaces[20][IFNAMSIZ];
    int count = 0;

    while (fgets(line, sizeof(line), fp) && count < 20) {
        // 移除行首空白字符
        char *name = line;
        while (*name != '\0' && isspace((unsigned char)*name)) {
            name++;
        }

        // 查找接口名结束位置
        char *colon = strchr(name, ':');
        if (!colon) continue;

        // 截断接口名
        *colon = '\0';
        
        // 检查是否是rmnet_data接口
        if (strncmp(name, "rmnet_data", 10) == 0) {
            strncpy(interfaces[count], name, IFNAMSIZ - 1);
            interfaces[count][IFNAMSIZ - 1] = '\0';
            count++;
        }
    }
    fclose(fp);

    // 关闭所有找到的接口
    for (int i = 0; i < count; i++) {
        char cmd[64];
        snprintf(cmd, sizeof(cmd), "ifconfig %s down", interfaces[i]);
        int ret = system(cmd);
        if (ret != 0) {
            PRINT_DEBUG("Failed to down interface: %s\n", interfaces[i]);
        }
    }
}

void qcmap_mark_signal_file(void) {
    PRINT_DEBUG("write file [%s]\n", QCMAP_MNGR_DIAL_END_FROM_SIGNAL);
    write_buf_to_file(QCMAP_MNGR_DIAL_END_FROM_SIGNAL, "1", sizeof("1"));
    system_ex("sync", 0);
}

char *qcmap_signal_num_to_str(int signal) {
    switch(signal) {
        case SIGUSR1:
            return "SIGUSR1";
        case SIGTERM:
            return "SIGTERM";
        case SIGHUP:
            return "SIGHUP";
        case SIGINT:
            return "SIGINT";
        case SIGKILL:
            return "SIGKILL";
        default:
            return "UNKONW";
    }
}

void qcmap_signal_handler(int signal) {
    static bool signal_mark = false;
    qmi_error_type_v01 qmi_err_num = QMI_ERR_NONE_V01;
    PRINT_DEBUG("recv signal %s\n", qcmap_signal_num_to_str(signal));

    if (signal == SIGUSR1 || signal == SIGTERM || signal == SIGHUP || signal == SIGINT || signal == SIGKILL) {
        if (!signal_mark) {
            qcmap_release_util();
            qcmap_release_modem();
            qcmap_release_led();
            qcmap_release_config();
            qcmap_release_apn();
            qcmap_release_info();
            qcmap_release_client();
            qcmap_release_interface();
            signal_mark = true;
        }
        if (signal == SIGUSR1) {
            qcmap_mark_signal_file();
            PRINT_DEBUG("------------------------>>> qcmap_mngr end ------------------------>>>\n");
            return;
        } else {
            write_buf_to_file(QCMAP_BOOT_FILE, "1", sizeof("1"));
            PRINT_DEBUG("------------------------>>> qcmap_mngr end ------------------------>>>\n");
            exit(0);
        }
    } else {
        PRINT_DEBUG("Received unexpected signal %s\n", signal);
        return;
    }
}

int qcmap_check_at(void) {
    char result[QCMAP_MAX_STRING_LEN] = {0};
    char command[QCMAP_MAX_STRING_LEN] = {0};

    snprintf(command, QCMAP_MAX_STRING_LEN, "%s %s", QCMAP_MNGR_AT_MNGR_COMMAND, QCMAP_MNGR_AT_COMMAND_AT);
    if (execute_cmd((const char*)command, result, sizeof(result))) {
        if (strcasestr(result, "OK")) {
            return AT_READY;
        }
        if (strcasestr(result, "Receive timed out")) {
            return AT_TIMEOUT;
        }
    }
    PRINT_DEBUG("at is not ready, check again\n");

    return AT_NONE;
}

static inline bool contains_ignore_case(const char *str, const char *substr) {
    return strcasestr(str, substr) != NULL;
}

int qcmap_check_sim(void) {
    char result[QCMAP_MAX_STRING_LEN] = {0};
    char command[QCMAP_MAX_STRING_LEN];
    int sim_status = SIM_NONE;

    snprintf(command, sizeof(command), "%s -i 2 %s?", QCMAP_MNGR_AT_MNGR_COMMAND, QCMAP_MNGR_AT_COMMAND_CPIN);
    
    if (!execute_cmd(command, result, sizeof(result))) {
        goto exit;
    }

    if (contains_ignore_case(result, "READY")) {
        pinlock_config.pincodeset = 0;
        WRITE_SIM_STATUS("Sim Ready");
        return SIM_READY;
    }

    if (contains_ignore_case(result, "Receive timed out")) {
        sim_status = SIM_TIMEOUT;
        goto exit;
    }

    if (contains_ignore_case(result, "failure") || contains_ignore_case(result, "not inserted")) {
        WRITE_SIM_STATUS("No Sim");
        goto exit;
    }

    if (!contains_ignore_case(result, "PIN") && !contains_ignore_case(result, "SIM")) {
        WRITE_SIM_STATUS("No Sim");
        goto exit;
    }

    if (contains_ignore_case(result, "ERROR")) {
        WRITE_SIM_STATUS("No Sim");
        goto exit;
    }

    if (contains_ignore_case(result, "PH-NET")) {
        WRITE_SIM_STATUS("MCCMNC Lock");
        goto exit;
    }

    if (contains_ignore_case(result, "PUK")) {
        WRITE_SIM_STATUS("Sim Puk");
        goto exit;
    }

    WRITE_SIM_STATUS("Sim Pin");

    /* PIN Auto Unlock Handling */
    if (pinlock_config.pinAutoUnlock && !pinlock_config.pincodeset) {
        pinlock_config.pincodeset = 1;
        WRITE_PINLOCK_STATUS("pin lock");

        if (strlen(pinlock_config.pinNumber) > 3) {
            memset(result, 0, sizeof(result));
            snprintf(command, sizeof(command), "%s \'%s=\"%s\"\'", QCMAP_MNGR_AT_MNGR_COMMAND, QCMAP_MNGR_AT_COMMAND_CPIN, pinlock_config.pinNumber);
            if (execute_cmd(command, result, sizeof(result))) {
                PRINT_DEBUG("pin locked, go to unlock\n");
                if (contains_ignore_case(result, "ERROR")) {
                    PRINT_DEBUG("pin unlock fail\n");
                    WRITE_SIM_STATUS("PinError");
                } else {
                    PRINT_DEBUG("pin unlock success\n");
                    WRITE_SIM_STATUS("Sim Ready");
                    sim_status = SIM_READY;
                }
            }
        }
    }

exit:
    if (sim_status != SIM_READY) {
        PRINT_DEBUG("sim is not ready, check again\n");
        PRINT_DEBUG("result:[%s]\n", result);
    }
    return sim_status;
}

void qcmap_print_modem_status(int index) {
    switch (index) {
        case MODEM_NONE: {
            PRINT_DEBUG("modem none\n");
            break;
        }
        case MODEM_AT_READY: {
            PRINT_DEBUG("modem at ready\n");
            break;
        }
        case MODEM_SIM_READY: {
            PRINT_DEBUG("modem sim ready\n");
            break;
        }
        case MODEM_END: {
            PRINT_DEBUG("modem end\n");
            break;
        }
        default: {
            PRINT_DEBUG("modem unknown\n");
            break;
        }
    }
}

//检查模组状态，at，cpin，并作对应动作，用状态机方式实现
void qcmap_timer_check_modem(void) {
    static int at_count = 0;
    static int sim_count = 0;
    static int network_count = 0;
    static unsigned int s_last_modem_check_time = 0;
    unsigned int now_time = qcmap_get_uptime_in_ms();

    if (POS_DIFF_VAL(now_time, s_last_modem_check_time) < MOD_STATUS_UPDATE_INTERVAL) {
        return;
    }

    s_last_modem_check_time = now_time;

    if (qcmap_get_state_machine_running_status()) {
        return;
    }

    switch (modem_status) {
        case MODEM_NONE: {
            at_count++;
            int at_status = qcmap_check_at();
            
            if (at_status == AT_TIMEOUT) {
                // 超时情况下等待下次调用再重试
                break;
            }
            
            if (at_status == AT_READY) {
                modem_status = MODEM_AT_READY;
                at_count = 0; // 重置计数器
            } else {
                if (at_count > 2) {
                    qcmap_enable_state_machine();
                    qcmap_reset_signal();
                    at_count = 0; // 重置计数器
                }
                qcmap_release_led();
            }
            break;
        }
        case MODEM_AT_READY: {
            sim_count++;
            int sim_status = qcmap_check_sim();
            
            if (sim_status == SIM_TIMEOUT) {
                // 超时情况下等待下次调用再重试
                break;
            }
            
            if (sim_status == SIM_READY) {
                modem_status = MODEM_SIM_READY;
                sim_count = 0; // 重置计数器
            } else {
                if (sim_count > 2) {
                    qcmap_enable_state_machine();
                    qcmap_reset_signal();
                    sim_count = 0; // 重置计数器
                }
                qcmap_release_led();
            }
            break;
        }
        case MODEM_SIM_READY: {
            network_count++;
            int network_status = qcmap_check_registration();
            
            if (network_status == NETWORK_TIMEOUT) {
                // 超时情况下等待下次调用再重试
                break;
            }
            
            if (network_status == NETWORK_REGISTERED) {
                modem_status = MODEM_END;
                network_count = 0; // 重置计数器
            } else {
                if (network_count > 2) {
                    qcmap_enable_state_machine();
                    qcmap_reset_signal();
                    network_count = 0; // 重置计数器
                }
                qcmap_release_led();
            }
            break;
        }
        case MODEM_END: {
            // 完成所有检查，重置状态
            modem_status = MODEM_NONE;
            break;
        }
        default: {
            // 未知状态，重置为初始状态
            modem_status = MODEM_NONE;
            break;
        }
    }
}