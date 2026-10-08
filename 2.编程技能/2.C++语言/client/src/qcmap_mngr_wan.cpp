#include "qcmap_mngr_wan.h"
#include "qcmap_mngr_info.h"
#include "qcmap_mngr_util.h"
#include "qcmap_mngr_config.h"

void qcmap_get_interface_info(char* inf, inf_status_t* wandeviceinfo) {
    bool read_end = false;
    FILE* fd = NULL;
    char line[128] = {0};
    char* ptr = NULL;
    char cmd[128] = {0};

    sprintf(cmd, "ubus call network.interface.%s status | sed 's/\"//g'", inf);
    if ((fd = popen(cmd, "r")) != NULL) {
        while (fgets(line, sizeof(line) - 1, fd)) {
            if (read_end) {
                continue;
            }
            if (strstr(line, "up: true") != NULL) {
                wandeviceinfo->status = 1;
                continue;
            }
            if (strstr(line, "uptime:") != NULL) {
                sscanf(line, "%*s %s", wandeviceinfo->uptime);
                if ((ptr = strstr(wandeviceinfo->uptime, ",")) != NULL)
                    *ptr = '\0';
                continue;
            }
            if (strstr(line, "l3_device:") != NULL) {
                sscanf(line, "%*s %s", wandeviceinfo->l3_device);
                if ((ptr = strstr(wandeviceinfo->l3_device, ",")) != NULL)
                    *ptr = '\0';
                continue;
            }
            if (strstr(line, "proto:") != NULL) {
                sscanf(line, "%*s %s", wandeviceinfo->proto);
                if ((ptr = strstr(wandeviceinfo->proto, ",")) != NULL)
                    *ptr = '\0';
                continue;
            }
            if (strstr(line, "device:") != NULL) {
                sscanf(line, "%*s %s", wandeviceinfo->device);
                if ((ptr = strstr(wandeviceinfo->device, ",")) != NULL)
                    *ptr = '\0';
                continue;
            }
            if (strstr(line, "ipv4-address:") != NULL) {
                while (fgets(line, sizeof(line) - 1, fd)) {
                    if (strstr(line, "],") == NULL) {
                        if (strstr(line, "address:") != NULL) {
                            sscanf(line, "%*s %s", wandeviceinfo->ipv4_address);
                            if ((ptr = strstr(wandeviceinfo->ipv4_address, ",")) != NULL)
                                *ptr = '\0';
                            //break;
                        }
                        if (strstr(line, "mask:") != NULL) {
                            sscanf(line, "%*s %s", wandeviceinfo->mask);
                            if ((ptr = strstr(wandeviceinfo->mask, ",")) != NULL)
                                *ptr = '\0';
                            break;
                        }
                    } else
                        break;
                }
                continue;
            }
            if (strstr(line, "ipv6-address:") != NULL) {
                while (fgets(line, sizeof(line) - 1, fd)) {
                    if (strstr(line, "],") == NULL) {
                        if (strstr(line, "address:") != NULL) {
                            sscanf(line, "%*s %s", wandeviceinfo->ipv6_address);
                            if ((ptr = strstr(wandeviceinfo->ipv6_address, ",")) != NULL)
                                *ptr = '\0';
                            //break;
                        }
                        if (strstr(line, "mask:") != NULL) {
                            sscanf(line, "%*s %s", wandeviceinfo->ipv6_mask);
                            if ((ptr = strstr(wandeviceinfo->ipv6_mask, ",")) != NULL)
                                *ptr = '\0';
                            break;
                        }
                    } else
                        break;
                }
                continue;
            }
            if (strstr(line, "ipv6-prefix:") != NULL) {
                while (fgets(line, sizeof(line) - 1, fd)) {
                    if (strstr(line, "],") == NULL) {
                        if (strstr(line, "address:") != NULL) {
                            sscanf(line, "%*s %s", wandeviceinfo->ipv6_prefix_address);
                            if ((ptr = strstr(wandeviceinfo->ipv6_prefix_address, ",")) != NULL)
                                *ptr = '\0';
                            //break;
                        }
                        if (strstr(line, "mask:") != NULL) {
                            sscanf(line, "%*s %s", wandeviceinfo->ipv6_prefix_mask);
                            if ((ptr = strstr(wandeviceinfo->ipv6_prefix_mask, ",")) != NULL)
                                *ptr = '\0';
                            break;
                        }
                    } else
                        break;
                }
                continue;
            }
            if (strstr(line, "route:") != NULL) {
                char source[16] = {0};
                while (fgets(line, sizeof(line) - 1, fd)) {
                    if (strstr(line, "],") == NULL) {
                        if (strstr(line, "nexthop:") != NULL) {
                            sscanf(line, "%*s %s", wandeviceinfo->nexthop);
                            if ((ptr = strstr(wandeviceinfo->nexthop, ",")) != NULL)
                                *ptr = '\0';
                        } else if (strstr(line, "source:") != NULL) {
                            sscanf(line, "%*s %s", source);
                            if ((ptr = strstr(source, "/")) != NULL)
                                *ptr = '\0';
                            if (!strcmp(wandeviceinfo->ipv4_address, source))
                                break;
                        }
                    } else
                        break;
                }
                continue;
            }
            if (strstr(line, "dns-server:") != NULL) {
                char dnsip[512] = {0};
                char dnsbuf[512] = {0};
                while (fgets(line, sizeof(line) - 1, fd)) {
                    memset(dnsip, 0, sizeof(dnsip));
                    memset(dnsbuf, 0, sizeof(dnsbuf));
                    if (strstr(line, "],") == NULL) {
                        ptr = strrchr(line, '\t');
                        if (ptr)
                            snprintf(dnsip, sizeof(dnsip), "%s", ptr + 1);
                        else
                            snprintf(dnsip, sizeof(dnsip), "%s", line);
                        if ((ptr = strstr(dnsip, "\n")) != NULL)
                            *ptr = '\0';
                        if ((ptr = strstr(dnsip, ",")) != NULL)
                            *ptr = '\0';
                        if (strcmp(dnsip, "") != 0 && strlen(dnsip) >= 7) {
                            if (wandeviceinfo->dns[0] == '\0') {
                                snprintf(wandeviceinfo->dns, sizeof(wandeviceinfo->dns), "%s", dnsip);
                            } else {
                                snprintf(dnsbuf, sizeof(dnsbuf), "%s,%s", wandeviceinfo->dns, dnsip);
                                snprintf(wandeviceinfo->dns, sizeof(wandeviceinfo->dns), "%s", dnsbuf);
                            }
                        }
                    } else
                        break;
                }
                continue;
            }
            if (strstr(line, "inactive: {") != NULL) {    //we don't check the info behind the 'inactive'
                read_end = true;
                continue;
            }
        }
        pclose(fd);
    }
}

void qcmap_wan_uptime_update(void) {
    char *endptr;
    unsigned long uptime = 0;
    inf_status_t inf_status;
    char connect_time_str[16] = {0};
    char wan_name[16] = {0};
    char wan_name_v6[16] = {0};

    for (int i = 1; i <= QCMAP_MNGR_DEFAULT_PROFILE_NUM; i++) {
        if (!qcmap_wan5g_list[i - 1].enable)
            continue;

        if (i == 1) {
            sprintf(wan_name, "%s", QCMAP_MNGR_DEFAULT_WAN_NAME);
            sprintf(wan_name_v6, "%s_v6", QCMAP_MNGR_DEFAULT_WAN_NAME);
        } else {
            sprintf(wan_name, "%s%d", QCMAP_MNGR_DEFAULT_WAN_NAME, i);
            sprintf(wan_name_v6, "%s%d_v6", QCMAP_MNGR_DEFAULT_WAN_NAME, i);
        }

        if (qcmap_wan5g_list[i - 1].ip_family == 6) {
            qcmap_get_interface_info(wan_name_v6, &inf_status);
        } else {
            qcmap_get_interface_info(wan_name, &inf_status);
        }

        uptime = strtoul(inf_status.uptime, &endptr, 10);

        sprintf(connect_time_str, "%d:%02d:%02d", uptime / 3600, (uptime % 3600 / 60), uptime % 60);
        write_buf_to_file(WAN_5G_CONNECT_TIME_FILE, connect_time_str, strlen(connect_time_str));
        return;
    }
    write_buf_to_file(WAN_5G_CONNECT_TIME_FILE, "0:00:00", strlen("0:00:00"));
}

void qcmap_wan_status_update(bool is_first_call) {
    const char *wanstatus = DIALD_DISCONNECTED;
    inf_status_t inf_status;
    inf_status_t inf_status_v6;
    char wan_name[16] = {0};
    char wan_name_v6[16] = {0};

    // 局部静态变量记录3个IPv4接口和3个IPv6接口状态
    static bool v4_interface_status[3] = {false, false, false};
    static bool v6_interface_status[3] = {false, false, false};

    glb_module_desc.wanipv4addr = 0;
    memset(glb_module_desc.wan_addr6, 0, sizeof(glb_module_desc.wan_addr6));

    // 如果是第一次调用，重置状态记录
    if (is_first_call) {
        memset(v4_interface_status, 0, sizeof(v4_interface_status));
        memset(v6_interface_status, 0, sizeof(v6_interface_status));
        PRINT_DEBUG("First call: reset interface status tracking\n");
    }

    // 遍历所有3个接口
    for (int i = 1; i <= QCMAP_MNGR_DEFAULT_PROFILE_NUM; i++) {
        if (!qcmap_wan5g_list[i - 1].enable) {
            v4_interface_status[i - 1] = false;
            v6_interface_status[i - 1] = false;
            continue;
        }

        if (i == 1) {
            sprintf(wan_name, "%s", QCMAP_MNGR_DEFAULT_WAN_NAME);
            sprintf(wan_name_v6, "%s_v6", QCMAP_MNGR_DEFAULT_WAN_NAME);
        } else {
            sprintf(wan_name, "%s%d", QCMAP_MNGR_DEFAULT_WAN_NAME, i);
            sprintf(wan_name_v6, "%s%d_v6", QCMAP_MNGR_DEFAULT_WAN_NAME, i);
        }

        bool current_v4_status = false;
        bool current_v6_status = false;
        uint32_t temp_ipv4addr = 0;
        char temp_addr6[128] = {0};

        // 检查IPv4接口状态
        if (qcmap_wan5g_list[i - 1].ip_family == 4 || qcmap_wan5g_list[i - 1].ip_family == 10) {
            qcmap_get_interface_info(wan_name, &inf_status);
            read_interface_info_v4(inf_status.l3_device, NULL, &temp_ipv4addr, NULL);
            current_v4_status = (temp_ipv4addr != 0x0 && temp_ipv4addr != 0xffffffff);
            
            // 更新全局变量为第一个连接的接口
            if (current_v4_status && glb_module_desc.wanipv4addr == 0) {
                glb_module_desc.wanipv4addr = temp_ipv4addr;
            }
        }

        // 检查IPv6接口状态
        if (qcmap_wan5g_list[i - 1].ip_family == 6 || qcmap_wan5g_list[i - 1].ip_family == 10) {
            qcmap_get_interface_info(wan_name_v6, &inf_status_v6);
            read_interface_info_v6(inf_status_v6.l3_device, NULL, temp_addr6, NULL);
            current_v6_status = (strlen(temp_addr6) > 0);
            
            // 更新全局变量为第一个连接的接口
            if (current_v6_status && strlen(glb_module_desc.wan_addr6) == 0) {
                strncpy(glb_module_desc.wan_addr6, temp_addr6, sizeof(glb_module_desc.wan_addr6) - 1);
            }
        }

        // 检查IPv4接口状态变化
        if (i - 1 < 3 && current_v4_status != v4_interface_status[i - 1]) {
            PRINT_DEBUG("IPv4 interface %d status changed: %s -> %s\n",
                       i,
                       v4_interface_status[i - 1] ? "CONNECTED" : "DISCONNECTED",
                       current_v4_status ? "CONNECTED" : "DISCONNECTED");
            if (i == 1 && current_v4_status == true && v4_interface_status[i - 1] == false) {
                qcmap_send_cwmp_notify();
            }
            v4_interface_status[i - 1] = current_v4_status;
        }

        // 检查IPv6接口状态变化
        if (i - 1 < 3 && current_v6_status != v6_interface_status[i - 1]) {
            PRINT_DEBUG("IPv6 interface %d status changed: %s -> %s\n",
                       i,
                       v6_interface_status[i - 1] ? "CONNECTED" : "DISCONNECTED",
                       current_v6_status ? "CONNECTED" : "DISCONNECTED");
            v6_interface_status[i - 1] = current_v6_status;
        }

        // 如果任一接口连接，则设置连接状态
        if (current_v4_status || current_v6_status) {
            wanstatus = DIALD_CONNECTED;
        }
    }

    glb_module_desc.wanstatus = wanstatus;
    write_buf_to_file(WAN_STATUS_FILE, wanstatus, strlen(wanstatus));
}

void qcmap_timer_check_wan(void) {
    static unsigned int s_last_check_time = 0;
    unsigned int now_time = qcmap_get_uptime_in_ms();

    if (POS_DIFF_VAL(now_time, s_last_check_time) < MOD_STATUS_UPDATE_INTERVAL) {
        return;
    }

    s_last_check_time = now_time;

    qcmap_wan_status_update(false);
    qcmap_wan_uptime_update();
}

bool qcmap_get_ip_passthrough_feature(void) {
    bool ret = false;
    uint64 features = 0;
    qmi_error_type_v01 qmi_err_num = QMI_ERR_NONE_V01;
    qcmap_client_feature_mode_config feature_mode_config;

    memset(&feature_mode_config, 0, sizeof(qcmap_client_feature_mode_config));
    bool ret_val = qcmap_client->GetFeatureMode(&features, &feature_mode_config, &qmi_err_num);
    if (!ret_val) {
        PRINT_DEBUG("Failure in getting feature modes\n");
        return ret;
    }

    if (features == 0) {
        PRINT_DEBUG("No features / modes are enabled\n");
        return ret;
    }

    if (features & QCMAP_MSGR_MASK_ENABLE_FEATURE_IP_PASSTHROUGH_V01) {
        if (feature_mode_config.ip_passthrough_feature_valid) {
            if (feature_mode_config.ip_passthrough_feature_mode == QCMAP_MSGR_IP_PASSTHROUGH_MODE_WITH_NAT_V01) {
                PRINT_DEBUG("IP Passthrough Feature: Set With NAT\n");
            } else {
                PRINT_DEBUG("IP Passthrough Feature: Set Without NAT\n");
                ret = true;
            }
        } else {
            PRINT_DEBUG("IP Passthrough Feature disabled\n");
        }
    } else {
        PRINT_DEBUG("no IP Passthrough Feature found\n");
    }

    return ret;
}

bool qcmap_set_ip_passthrough_feature(bool state) {
    bool ret = false;
    uint64 features = 0;
    qmi_error_type_v01 qmi_err_num = QMI_ERR_NONE_V01;
    qcmap_client_feature_mode_config feature_mode_config;

    memset(&feature_mode_config, 0,  sizeof(qcmap_client_feature_mode_config));

    features |= QCMAP_MSGR_MASK_ENABLE_FEATURE_IP_PASSTHROUGH_V01;
    feature_mode_config.ip_passthrough_feature_valid = state;
    if (feature_mode_config.ip_passthrough_feature_valid) {
        //feature_mode_config.ip_passthrough_feature_mode = QCMAP_MSGR_IP_PASSTHROUGH_MODE_WITH_NAT_V01;
        feature_mode_config.ip_passthrough_feature_mode = QCMAP_MSGR_IP_PASSTHROUGH_MODE_WITHOUT_NAT_V01;
    }

    if (qcmap_client->SetFeatureMode(features, &feature_mode_config, &qmi_err_num)) {
        PRINT_DEBUG("Successfully set the ip passthrough feature mode state [%d]\n", state);
        ret = true;
    } else {
        PRINT_DEBUG("Cannot set the ip passthrough feature mode state [%d]. Error: %d\n", state, qmi_err_num);
    }

    return ret;
}

bool qcmap_get_ip_passthrough_config(void) {
    bool ret = false;
    qmi_error_type_v01 qmi_err_num = QMI_ERR_NONE_V01;
    qcmap_msgr_ip_passthrough_config_v01 ip_passthrough_config;
    qcmap_msgr_ip_passthrough_mode_enum_v01 enable_state;
    char mac_addr_str[QCMAP_MSGR_MAC_ADDR_NUM_CHARS_V01] = {0};

    memset(&ip_passthrough_config, 0, sizeof(qcmap_client_ip_passthrough_config));
    if (qcmap_client->GetIPPassthroughConfig(&enable_state, &ip_passthrough_config, &qmi_err_num))
    {
        /* enable state */
        if(enable_state && ip_passthrough_config.device_type == QCMAP_MSGR_DEVICE_TYPE_ANY_V01) {
            PRINT_DEBUG("get IP Passthrough config right\n");
            PRINT_DEBUG("get IP Passthrough config Flag is SET\n");
            PRINT_DEBUG("get IP Passthrough config is set for first connected device (Optional MAC Address Configuration)\n");
            ret = true;
        } else {
            if (!enable_state) {
                PRINT_DEBUG("get IP Passthrough config is NOT SET\n");
                ret = false;
            }
            if (ip_passthrough_config.device_type != QCMAP_MSGR_DEVICE_TYPE_ANY_V01) {
                PRINT_DEBUG("get IP Passthrough config set mode not first connected device\n");
                ret = false;
            }
        }
    } else {
        PRINT_DEBUG("Get IP Passthrough config failed,Error 0x%x\n", qmi_err_num);
        ret = false;
    }

    return ret;
}

void qcmap_set_ip_passthrough_config(bool state) {
    qmi_error_type_v01 qmi_err_num = QMI_ERR_NONE_V01;
    qcmap_msgr_ip_passthrough_config_v01 ip_passthrough_config;
    qcmap_msgr_ip_passthrough_mode_enum_v01 enable_state = (qcmap_msgr_ip_passthrough_mode_enum_v01)state;
    memset(&ip_passthrough_config, 0, sizeof(qcmap_client_ip_passthrough_config));

    if (enable_state) {
        ip_passthrough_config.device_type = QCMAP_MSGR_DEVICE_TYPE_ANY_V01;
        if (qcmap_client->SetIPPassthroughConfig(enable_state, true, &ip_passthrough_config, &qmi_err_num)) {
            PRINT_DEBUG("Set IP Passthrough config enable successful\n");
        } else {
            PRINT_DEBUG("Set IP Passthrough config enable Error: 0x%x", qmi_err_num);
        }
    } else {
        if (qcmap_client->SetIPPassthroughConfig(enable_state, false, NULL, &qmi_err_num)) {
            PRINT_DEBUG("Set IP Passthrough config disable successful\n");
        } else {
            PRINT_DEBUG("Set IP Passthrough config disable Error: 0x%x\n", qmi_err_num);
            if(qmi_err_num == QMI_ERR_INVALID_HANDLE_V01) {
                PRINT_DEBUG("MobileAP is not enabled\n");
            }
        }
    }
}

bool qcmap_get_ip_passthrough_state(void) {
    boolean active_state;
    qmi_error_type_v01 qmi_err_num = QMI_ERR_NONE_V01;

    if (qcmap_client->GetIPPassthroughState(&active_state,&qmi_err_num)) {
        if(active_state) {
            PRINT_DEBUG("get IP Passthrough state is ACTIVATED\n");
            return true;
        } else {
            PRINT_DEBUG("get IP Passthrough state is NOT ACTIVATED\n");
            return false;
        }
    } else {
        PRINT_DEBUG("Get IP Passthrough state failed,Error 0x%x\n", qmi_err_num);
        return true; //获取失败则当作使能
    }
}

int qcmap_check_apply_bridge(void) {
    int bridge_enable = 0;
    int bridge_enable_old = 0;
    char result[64] = {0};
    char str_tmp[64] = {0};
    int update_bridge = 0;

    if (access(QCMAP_MNGR_NETWORK_WAN5G_BRIDGE, F_OK) == 0) {
        if (qcmap_get_uci_value(QCMAP_MNGR_MOBILE_CONFIG_FILE, QCMAP_MNGR_DEFAULT_WAN_NAME, QCMAP_MNGR_CONFIG_BRIDGEENABLE, result, sizeof(result))) {
            bridge_enable = atoi(result);
            bridge_enable_old = qcmap_get_bridge_enable(0);
            if (bridge_enable_old != bridge_enable) {
                PRINT_DEBUG("bridge_enable [%d] change to [%d]\n", bridge_enable_old, bridge_enable);
                qcmap_set_bridge_enable(0, bridge_enable);
                qcmap_init_cfun(true);
                update_bridge = 1;
            }
        }
        unlink(QCMAP_MNGR_NETWORK_WAN5G_BRIDGE);
    }

    return update_bridge;
}

void qcmap_init_bridge_firewall(void) {
    char current_value[16] = {0};
    bool bridge_enable = qcmap_get_bridge_enable(0);
    
    // 检查并设置 @zone[0] 的 masq 值
    if (qcmap_get_uci_value("firewall", "@zone[0]", "masq", current_value, sizeof(current_value))) {
        if (bridge_enable) {
            if (strcmp(current_value, "0") != 0) {
                qcmap_set_uci_value("firewall", "@zone[0]", "masq", "0");
            }
        } else {
            if (strcmp(current_value, "1") != 0) {
                qcmap_set_uci_value("firewall", "@zone[0]", "masq", "1");
            }
        }
    } else {
        // 如果读取失败，直接设置
        if (bridge_enable) {
            qcmap_set_uci_value("firewall", "@zone[0]", "masq", "0");
        } else {
            qcmap_set_uci_value("firewall", "@zone[0]", "masq", "1");
        }
    }
    
    // 检查并设置 @zone[1] 的 masq 值
    memset(current_value, 0, sizeof(current_value));
    if (qcmap_get_uci_value("firewall", "@zone[1]", "masq", current_value, sizeof(current_value))) {
        if (bridge_enable) {
            if (strcmp(current_value, "0") != 0) {
                qcmap_set_uci_value("firewall", "@zone[1]", "masq", "0");
            }
        } else {
            if (strcmp(current_value, "1") != 0) {
                qcmap_set_uci_value("firewall", "@zone[1]", "masq", "1");
            }
        }
    } else {
        // 如果读取失败，直接设置
        if (bridge_enable) {
            qcmap_set_uci_value("firewall", "@zone[1]", "masq", "0");
        } else {
            qcmap_set_uci_value("firewall", "@zone[1]", "masq", "1");
        }
    }
    
    qcmap_set_update_firewall(true);
}

void qcmap_init_bridge(void) {
    bool bridge_enable = qcmap_get_bridge_enable(0);

    qcmap_get_ip_passthrough_state();
    qcmap_set_ip_passthrough_feature(bridge_enable);
    qcmap_set_ip_passthrough_config(bridge_enable);
    qcmap_get_ip_passthrough_feature();
    qcmap_get_ip_passthrough_config();
    qcmap_init_bridge_firewall();
}