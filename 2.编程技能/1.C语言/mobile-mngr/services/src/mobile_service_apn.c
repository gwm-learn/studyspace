#include "mobile_module_util.h"
#include "mobile_service_modem.h"
#include "mobile_service_dial.h"
#include "mobile_service_state_machine.h"
#include "mobile_service_apn.h"

/*
 * 文件名称：mobile_service_apn.c
 * 功能描述：
 *     负责管理apn
 *
 * 作者：gaoweiming
 */

// 内部状态变量
static int g_apn_module_initialized = 0;
static auto_apn_context_t g_auto_apn_ctx = {0};

/**
 * 初始化APN服务模块
 * 封装APN模块的初始化操作，提供标准的初始化框架
 *
 * @return 成功返回0，失败返回错误码
 */
int mobile_init_apn_service(void) {
    int ret = 0;
    
    if (g_apn_module_initialized) {
        MOBILE_INFO("apn service module already initialized\n");
        return 0;
    }
    
    ret = mobile_init_basic_apn();
    ret |= mobile_init_multi_apn();
    
    g_apn_module_initialized = 1;
    if (ret == 0) {
        mobile_network_basic_interface_config();
        mobile_network_multi_interface_settings();
        mobile_fw_config_basic_settings();
        mobile_fw_config_multi_setting();
        MOBILE_INFO("apn service module initialized successfully\n");
    } else {
        MOBILE_ERROR("apn service module initialized fail\n");
    }
    return ret;
}

/**
 * 清理APN服务模块资源
 * 封装APN模块的清理操作
 *
 * @return 无返回值
 */
void mobile_deinit_apn_service(void) {
    if (!g_apn_module_initialized) {
        return;
    }
    
    memset(&g_module_desc.basic_config, '\0', sizeof(g_module_desc.basic_config));
    mobile_free_whole_comm_list((void**)&g_module_desc.mutil_config);
    
    g_apn_module_initialized = 0;
    MOBILE_INFO("apn service module deinitialized\n");
}

/**
 * 初始化自动APN配置
 * 根据MCCMNC自动配置APN参数，支持运营商自动识别
 * 优先级：APN MCCMNC > SIM MCCMNC
 *
 * @return 成功返回0，失败返回错误码
 */
int mobile_init_auto_apn(void) {
    int ret = 0;
    char *mccmnc_to_use = NULL;

    mobile_get_apn_mccmnc();
    mobile_get_sim_mccmnc();

    // 确定要使用的MCCMNC，优先级：APN MCCMNC > SIM MCCMNC
    if (strlen(g_module_desc.apn_mccmnc) > 0) {
        mccmnc_to_use = g_module_desc.apn_mccmnc;
    } else if (strlen(g_module_desc.sim_mccmnc) > 0) {
        mccmnc_to_use = g_module_desc.sim_mccmnc;
    }

    // 如果没有可用的MCCMNC，直接返回成功
    if (mccmnc_to_use == NULL) {
        MOBILE_WARN("No valid MCCMNC available for auto APN configuration\n");
        return 0;
    }

    // 设置PLMN并初始化自动APN列表
    strncpy(g_module_desc.plmn, mccmnc_to_use, sizeof(g_module_desc.plmn));
    ret = mobile_init_auto_apn_list(mccmnc_to_use, &g_auto_apn_ctx.list);
    if (ret != 0) {
        MOBILE_ERROR("Failed to initialize auto APN list with MCCMNC [%s], ret=%d\n", mccmnc_to_use, ret);
        return ret;
    }

    // 打印自动APN列表信息
    mobile_print_auto_apn_list(&g_auto_apn_ctx.list);
    
    MOBILE_INFO("Auto APN initialization completed successfully with MCCMNC [%s]\n", mccmnc_to_use);
    return 0;
}

/**
 * 初始化基本APN配置
 * 从UCI配置文件中读取基本APN配置参数并初始化全局配置结构体
 * 包括网络类型、认证信息、PIN码设置、网络功能等配置项
 *
 * @return 成功返回0，失败返回错误码
 */
int mobile_init_basic_apn(void) {
    int ret = 0;

    ret = mobile_uci_get_int("mobile.@basic[0].enable", &g_module_desc.basic_config.enable);
    ret |= mobile_uci_get_int("mobile.@basic[0].manualApnEnable", &g_module_desc.basic_config.manual_apn_enable);
    ret |= mobile_uci_get_int("mobile.@basic[0].mtu", &g_module_desc.basic_config.mtu);
    ret |= mobile_uci_get_int("mobile.@basic[0].bridgeEnable", &g_module_desc.basic_config.bridge_enable);
    ret |= mobile_uci_get_int("mobile.@basic[0].natEnable", &g_module_desc.basic_config.nat_enable);
    //ret |= mobile_uci_get_int("mobile.@basic[0].busType", &g_module_desc.basic_config.bus_type);
    ret |= mobile_uci_get_int("mobile.@basic[0].delegate", &g_module_desc.basic_config.delegate);
    ret |= mobile_uci_get("mobile.@basic[0].netType",    g_module_desc.basic_config.net_type);
    ret |= mobile_uci_get("mobile.@basic[0].netSubType", g_module_desc.basic_config.net_sub_type);
    ret |= mobile_uci_get("mobile.@basic[0].authType",   g_module_desc.basic_config.auth_type);
    ret |= mobile_uci_get("mobile.@basic[0].userName",   g_module_desc.basic_config.username);
    ret |= mobile_uci_get("mobile.@basic[0].password",   g_module_desc.basic_config.password);
    ret |= mobile_uci_get("mobile.@basic[0].dialNumber", g_module_desc.basic_config.dial_number);
    ret |= mobile_uci_get("mobile.@basic[0].apn",        g_module_desc.basic_config.apn);
    ret |= mobile_uci_get("mobile.@basic[0].ipVersion",  g_module_desc.basic_config.ip_version);
    if (strcasecmp(g_module_desc.basic_config.ip_version, "IPv4") == 0) {
        strcpy(g_module_desc.basic_config.ip_ver_num, "1");
    } else if (strcasecmp(g_module_desc.basic_config.ip_version, "IPv6") == 0) {
        strcpy(g_module_desc.basic_config.ip_ver_num, "2");
    } else {
        strcpy(g_module_desc.basic_config.ip_ver_num, "3");
    }

    if (ret != 0) {
        MOBILE_WARN("Some UCI configuration parameters failed to load\n");
    }
    
    MOBILE_INFO("Basic APN configuration loaded: enable=%d, APN=%s, auth=%s, user=%s, password=%s, dial=%s\n",
                g_module_desc.basic_config.enable, g_module_desc.basic_config.apn,
                g_module_desc.basic_config.auth_type, g_module_desc.basic_config.username,
                g_module_desc.basic_config.password, g_module_desc.basic_config.dial_number);
    return 0;
}

/**
 * 初始化单个多APN配置
 * 从UCI配置文件中读取多APN配置参数并初始化全局配置结构体
 * 包括网络类型、认证信息、PIN码设置、网络功能等配置项
 *
 */
static void mobile_init_multi_apn_single(module_mutil_config_t *node, char *section) {
    int ret = 0;

    if (node == NULL || section == NULL) {
        MOBILE_ERROR("node or section is null\n");
        return;
    }

    ret = mobile_uci_get_option_int("mobile", section, "enable", &node->enable);
    ret |= mobile_uci_get_option_int("mobile", section, "delegate", &node->delegate);
    ret |= mobile_uci_get_option_int("mobile", section, "vlan", &node->vlanid);
    ret |= mobile_uci_get_option_int("mobile", section, "vlan", &node->cid);
    ret |= mobile_uci_get_option_int("mobile", section, "bridgeEnable", &node->bridge_enable);
    ret |= mobile_uci_get_option_int("mobile", section, "natEnable", &node->nat_enable);
    ret |= mobile_uci_get_option("mobile", section, "interface", node->iface);
    ret |= mobile_uci_get_option("mobile", section, "authType",  node->auth_type);
    ret |= mobile_uci_get_option("mobile", section, "userName",  node->username);
    ret |= mobile_uci_get_option("mobile", section, "password",  node->password);
    ret |= mobile_uci_get_option("mobile", section, "apn",       node->apn);
    ret |= mobile_uci_get_option("mobile", section, "dialNumber",node->dial_number);
    ret |= mobile_uci_get_option("mobile", section, "ipVersion", node->ip_version);
    
    if (ret != 0) {
        MOBILE_WARN("Some UCI configuration parameters failed to load for section %s\n", section);
    }
}

/**
 * 初始化多APN配置
 * 从UCI配置文件中读取多APN配置参数并初始化全局配置结构体
 * 包括网络类型、认证信息、PIN码设置、网络功能等配置项
 *
 * @return 成功返回0，失败返回错误码
 */
int mobile_init_multi_apn(void) {
    char section[64 + 1] = {0};
    module_mutil_config_t *new_node = NULL;
    int config_count = 0;

    // 清理现有的多APN配置列表
    mobile_free_whole_comm_list((void**)&g_module_desc.mutil_config);
    
    // 遍历所有可能的APN配置项
    for (int i = 0, j = 0; i < MAX_MULTAPNS_COUNT; i++) {
        // 在公共列表尾部分配新节点
        new_node = (module_mutil_config_t*)mobile_malloc_in_comm_list_tail(
            (void**)&g_module_desc.mutil_config, sizeof(module_mutil_config_t));
        if (new_node) {
            // 初始化新节点
            memset(new_node, 0, sizeof(module_mutil_config_t));
            // 构建配置段名称
            snprintf(section, sizeof(section), "@mapn[%d]", j);
            mobile_init_multi_apn_single(new_node, section);
            // 记录配置信息
            MOBILE_INFO("Multi APN index[%d]: cid=%d, enable=%d, iface=%s, apn=%s, auth=%s, user=%s, password=%s, dial=%s\n", j, 
                new_node->cid, new_node->enable, new_node->iface, new_node->apn, new_node->auth_type, new_node->username, new_node->password, new_node->dial_number);
            j++;
            config_count++;
        } else {
            MOBILE_ERROR("Failed to allocate memory for multi APN configuration node %d\n", i);
            return -1;
        }
    }
    
    MOBILE_INFO("Multi APN initialization completed, loaded %d configurations\n", config_count);
    return 0;
}

/**
 * 配置移动网络基本接口参数
 * 根据提供的APN配置参数更新网络接口配置，支持IPv4/IPv6、认证方式、桥接模式等设置
 * 主要功能包括：
 * - 初始化wan5g网络接口配置
 * - 配置认证信息（用户名、密码、认证方式）
 * - 设置APN参数（iaapn和apn）
 * - 配置MTU、IP类型、网络模式等参数
 * - 支持路由模式和桥接模式切换
 * - 配置NAT和代理设置
 *
 * @param ipver      IP版本类型 ("ipv4", "ipv6", "ipv4v6")
 * @param apn        APN接入点名称
 * @param username   认证用户名
 * @param password   认证密码
 * @param dialnumber 拨号号码
 * @param authmethod 认证方式 ("none", "pap", "chap", "pap-chap")
 * @param cfg        模块配置结构体指针，包含MTU、桥接、NAT、代理等配置
 */
void mobile_network_basic_interface_config(void) {
    char wan_iface[64] = {0};
    apn_config config_t;
    int mtu_str[8] = {0};
    char disable[8] = {0};

    mobile_uci_get_option("network", MOBILE_BASICWAN_NAME, NULL, wan_iface);
    if (strcmp(wan_iface, "interface") != 0) {
        mobile_uci_set_option("network", MOBILE_BASICWAN_NAME, NULL, "interface");
        mobile_uci_set_option("network", MOBILE_BASICWAN_NAME, "disabled", "0");
        mobile_uci_set_option("network", MOBILE_BASICWAN_NAME, "device", "ccmni");
        mobile_uci_set_option("network", MOBILE_BASICWAN_NAME, "roamingtype", "3");
        mobile_uci_set_option("network", MOBILE_BASICWAN_NAME, "proto", "ql_datacall");
        mobile_uci_set_option("network", MOBILE_BASICWAN_NAME, "auto_conf", "0");
        mobile_uci_set_option("network", MOBILE_BASICWAN_NAME, "auto", "0");
        mobile_uci_set_option("network", MOBILE_BASICWAN_NAME, "cid", "1");
        mobile_uci_set_option("network", MOBILE_BASICWAN_NAME, "defaultroute", "0");
    }

    mobile_uci_get_option("network", MOBILE_BASICWAN_NAME, "disabled", disable);
    if((atoi(disable) == 1) && (g_module_desc.basic_config.enable == 1)) {
        mobile_uci_set_option("network", MOBILE_BASICWAN_NAME, "disabled", "");
    } else if ((atoi(disable) == 0) && (g_module_desc.basic_config.enable == 0)) {
        mobile_uci_set_option("network", MOBILE_BASICWAN_NAME, "disabled", "1");
    }

    mobile_uci_get_option("network", MOBILE_BASICWAN_NAME, "auth", config_t.auth_buf);
    if (strcmp(config_t.auth_buf, g_module_desc.basic_config.auth_type)) {
        mobile_uci_set_option("network", MOBILE_BASICWAN_NAME, "auth", g_module_desc.basic_config.auth_type);
    }

    mobile_uci_get_option("network", MOBILE_BASICWAN_NAME, "username", config_t.username_buf);
    if (strcmp(config_t.username_buf, g_module_desc.basic_config.username)) {
        mobile_uci_set_option("network", MOBILE_BASICWAN_NAME, "username", g_module_desc.basic_config.username);
    }

    mobile_uci_get_option("network", MOBILE_BASICWAN_NAME, "password", config_t.password_buf);
    if (strcmp(config_t.password_buf, g_module_desc.basic_config.password)) {
        mobile_uci_set_option("network", MOBILE_BASICWAN_NAME, "password", g_module_desc.basic_config.password);
    }

    mobile_uci_get_option("network", MOBILE_BASICWAN_NAME, "iaapn", config_t.iaapn_buf);
    if (strcmp(config_t.iaapn_buf, g_module_desc.basic_config.apn)) {
        mobile_uci_set_option("network", MOBILE_BASICWAN_NAME, "iaapn", g_module_desc.basic_config.apn);
    }

    mobile_uci_get_option("network", MOBILE_BASICWAN_NAME, "apn", config_t.apn_buf);
    if (strcmp(config_t.apn_buf, g_module_desc.basic_config.apn)) {
        mobile_uci_set_option("network", MOBILE_BASICWAN_NAME, "apn", g_module_desc.basic_config.apn);
    }

    sprintf(mtu_str, "%d", g_module_desc.basic_config.mtu);
    mobile_uci_get_option("network", MOBILE_BASICWAN_NAME, "mtu", config_t.mtu_buf);
    if (strcmp(config_t.mtu_buf, mtu_str)) {
        mobile_uci_set_option("network", MOBILE_BASICWAN_NAME, "mtu", mtu_str);
    }

    mobile_uci_get_option("network", MOBILE_BASICWAN_NAME, "iptype", config_t.iptype_buf);
    if (strcmp(config_t.iptype_buf, g_module_desc.basic_config.ip_ver_num)) {
        mobile_uci_set_option("network", MOBILE_BASICWAN_NAME, "iptype", g_module_desc.basic_config.ip_ver_num);
    }

    //Mode: 1 => Routing 2 => Bridge
    if (!g_module_desc.basic_config.bridge_enable) {
        mobile_uci_set_option("network", MOBILE_BASICWAN_NAME, "mode", "1");
    } else {
        mobile_uci_set_option("network", MOBILE_BASICWAN_NAME, "mode", "2");
    }
    if (g_module_desc.basic_config.nat_enable) {
        mobile_uci_set_option("network", MOBILE_BASICWAN_NAME, "natEnable", "1");
    } else {
        mobile_uci_set_option("network", MOBILE_BASICWAN_NAME, "natEnable", "0");
    }
    if (!g_module_desc.basic_config.delegate) {
        mobile_uci_set_option("network", MOBILE_BASICWAN_NAME, "delegate", "0");
    } else {
        mobile_uci_set_option("network", MOBILE_BASICWAN_NAME, "delegate", "1");
    }
}

/**
 * 配置单个多APN接口参数
 */
static void mobile_network_single_multi_interface_config(module_mutil_config_t *current_config) {
    char mtu[8] = {0};
    char apn[64] = {0};
    char ipver[8] = {0};
    char cmd_str[256] = {0};
    char wan_iface[64] = {0};
    char CID[8] = {0};
    char vlan[8] = {0};
    char mulitapn_default_route[8] = {0};
    apn_config config_t;

    mobile_uci_get_option("network", "wan5g", "iaapn", apn);
    mobile_uci_get_option("network", "wan5g", "mtu", mtu);

    sprintf(cmd_str, "network.%s", current_config->iface);
    mobile_uci_get_option("network", current_config->iface, NULL, wan_iface);

    if (strcmp(wan_iface, "interface") != 0) {
        mobile_uci_set_option("network", current_config->iface, NULL, "interface");
        mobile_uci_set_option("network", current_config->iface, "device", "ccmni");
        mobile_uci_set_option("network", current_config->iface, "proto", "ql_datacall");
        mobile_uci_set_option("network", current_config->iface, "auto_conf", "0");
        mobile_uci_set_option("network", current_config->iface, "auto", "0");
        mobile_uci_set_option("network", current_config->iface, "defaultroute", "0");
        mobile_uci_set_option("network", current_config->iface, "delegate", "0");
        mobile_uci_set_option("network", current_config->iface, "roamingtype", "3");
        mobile_uci_set_option("network", current_config->iface, "retry", "5");
        mobile_uci_set_option("network", current_config->iface, "ql_conf", "1");
        mobile_uci_set_option("network", current_config->iface, "auth", "AUTO");
        mobile_uci_set_option("network", current_config->iface, "username", "any");
        mobile_uci_set_option("network", current_config->iface, "password", "any");
        mobile_uci_set_option("network", current_config->iface, "apn", "internet");
        mobile_uci_set_option("network", current_config->iface, "vlan", "0");
        mobile_uci_set_option("network", current_config->iface, "iptype", "3");
        mobile_uci_set_option("network", current_config->iface, "iaapn", "internet");
        mobile_uci_set_option("network", current_config->iface, "mtu", "1500");
        mobile_uci_set_option("network", current_config->iface, "cid", "0");
    }

    if (current_config->apn == NULL || strlen(current_config->apn) == 0 ||
        current_config->iface == NULL || strlen(current_config->iface) == 0) {
        current_config->enable = 0;
    }

    mobile_uci_get_option("network", current_config->iface, "auth", config_t.auth_buf);
    if (strcmp(config_t.auth_buf, current_config->auth_type)) {
        mobile_uci_set_option("network", current_config->iface, "auth", current_config->auth_type);
    }

    mobile_uci_get_option("network", current_config->iface, "username", config_t.username_buf);
    if (strcmp(config_t.username_buf, current_config->username)) {
        mobile_uci_set_option("network", current_config->iface, "username", current_config->username);
    }

    mobile_uci_get_option("network", current_config->iface, "password", config_t.password_buf);
    if (strcmp(config_t.password_buf, current_config->password)) {
        mobile_uci_set_option("network", current_config->iface, "password", current_config->password);
    }

    mobile_uci_get_option("network", current_config->iface, "apn", config_t.apn_buf);
    if (strcmp(config_t.apn_buf, current_config->apn)) {
        mobile_uci_set_option("network", current_config->iface, "apn", current_config->apn);
    }

    mobile_uci_get_option("network", current_config->iface, "vlan", config_t.vlan_buf);
    if (atoi(config_t.vlan_buf) != current_config->vlanid) {
        sprintf(vlan, "%d", current_config->vlanid);
        mobile_uci_set_option("network", current_config->iface, "vlan", vlan);
    }

    if (strcasecmp(current_config->ip_version, "IPv4") == 0) {
        strcpy(ipver, "1");
    } else if (strcasecmp(current_config->ip_version, "IPv6") == 0) {
        strcpy(ipver, "2");
    } else {
        strcpy(ipver, "3");
    }

    mobile_uci_get_option("network", current_config->iface, "iptype", config_t.iptype_buf);
    if (strcmp(config_t.iptype_buf, ipver)) {
        mobile_uci_set_option("network", current_config->iface, "iptype", ipver);
    }

    mobile_uci_get_option("network", current_config->iface, "iaapn", config_t.iaapn_buf);
    if (strcmp(config_t.iaapn_buf, apn)) {
        mobile_uci_set_option("network", current_config->iface, "iaapn", apn);
    }

    mobile_uci_get_option("network", current_config->iface, "mtu", config_t.mtu_buf);
    if (strcmp(config_t.mtu_buf, mtu)) {
        mobile_uci_set_option("network", current_config->iface, "mtu", mtu);
    }

    sprintf(CID, "%d", current_config->cid);
    mobile_uci_get_option("network", current_config->iface, "cid", config_t.cid_buf);
    if (strcmp(config_t.cid_buf, CID)) {
        mobile_uci_set_option("network", current_config->iface, "cid", CID);
    }

    if (current_config->enable) {
        mobile_uci_set_option("network", current_config->iface, "disabled", "0");
    } else {
        mobile_uci_set_option("network", current_config->iface, "disabled", "1");
    }

    if (!current_config->bridge_enable) {
        mobile_uci_set_option("network", current_config->iface, "mode", "1");
    } else {
        mobile_uci_set_option("network", current_config->iface, "mode", "2");
    }

    if (current_config->nat_enable) {
        mobile_uci_set_option("network", current_config->iface, "natEnable", "1");
    } else {
        mobile_uci_set_option("network", current_config->iface, "natEnable", "0");
    }

    if (!current_config->delegate) {
        mobile_uci_set_option("network", current_config->iface, "delegate", "0");
    } else {
        mobile_uci_set_option("network", current_config->iface, "delegate", "1");
    }
}

/**
 * 配置移动网络多接口参数
 */
void mobile_network_multi_interface_settings(void) {
    module_mutil_config_t *current_config = g_module_desc.mutil_config;
    
    while (current_config != NULL) {
        mobile_network_single_multi_interface_config(current_config);
        current_config = current_config->next;
    }
}

/**
 * 配置防火墙设置
 * 根据基本APN配置的启用状态和NAT设置，管理wan5g接口在防火墙区域中的配置
 * 主要功能：
 * - 当基本APN启用且NAT启用时，将wan5g接口添加到防火墙WAN区域
 * - 当基本APN禁用或NAT禁用时，从防火墙WAN区域移除wan5g接口
 *
 */
void mobile_fw_config_basic_settings(void) {
    MOBILE_INFO("Firewall basic config: interface=%s, enable=%d, nat_enable=%d\n", MOBILE_BASICWAN_NAME, g_module_desc.basic_config.enable, g_module_desc.basic_config.nat_enable);
    if (g_module_desc.basic_config.enable == 1 && g_module_desc.basic_config.nat_enable== 1) {
        mobile_uci_del_list("firewall", "@zone[1]", "network", MOBILE_BASICWAN_NAME);
        mobile_uci_add_list("firewall", "@zone[1]", "network", MOBILE_BASICWAN_NAME);
        MOBILE_INFO("Added wan5g to firewall WAN zone\n");
    } else {
        mobile_uci_del_list("firewall", "@zone[1]", "network", MOBILE_BASICWAN_NAME);
        MOBILE_INFO("Removed wan5g from firewall WAN zone\n");
    }

    MOBILE_INFO("Basic APN firewall configuration completed\n");
}

/**
 * 配置防火墙设置
 * 配置单个多apn
 *
 */
static void mobile_fw_config_single_multi_setting(module_mutil_config_t *current_config) {
    MOBILE_INFO("Firewall multi config: interface=%s, enable=%d, nat_enable=%d\n", current_config->iface, current_config->enable, current_config->nat_enable);
    if (current_config->enable == 1 && current_config->nat_enable == 1) {
        mobile_uci_del_list("firewall", "@zone[1]", "network", current_config->iface);
        mobile_uci_add_list("firewall", "@zone[1]", "network", current_config->iface);
        MOBILE_INFO("Added %s to firewall WAN zone\n", current_config->iface);
    } else {
        mobile_uci_del_list("firewall", "@zone[1]", "network", current_config->iface);
        MOBILE_INFO("Removed %s from firewall WAN zone\n", current_config->iface);
    }

    MOBILE_INFO("Multi APN firewall configuration completed\n");
}

/**
 * 配置防火墙设置
 * 根据多APN配置的启用状态和NAT设置，管理wan5gxx接口在防火墙区域中的配置
 *
 */
void mobile_fw_config_multi_setting(void) {
    module_mutil_config_t *current_config = g_module_desc.mutil_config;
    
    while (current_config != NULL) {
        mobile_fw_config_single_multi_setting(current_config);
        current_config = current_config->next;
    }
}

/**
 * 检查basic配置更新
 *
 */
void mobile_check_update_basic_config(void) {
    if (access(DEFAULT_MOBILEBASICAPN_CONFIG_UPDATE, F_OK) != 0) {
        return;
    }
    MOBILE_DEBUG("************* check file %s *************\n", DEFAULT_MOBILEBASICAPN_CONFIG_UPDATE);
    mobile_init_basic_apn();
    mobile_network_basic_interface_config();
    mobile_fw_config_basic_settings();
    mobile_network_dedial_ex(MOBILE_BASICWAN_NAME, 0, 0);
    mobile_enable_state_machine();
    mobile_update_wan_status(DIALD_NONE);
    mobile_update_state_machine_status(MOBILE_STATE_NONE);
    unlink(DEFAULT_MOBILEBASICAPN_CONFIG_UPDATE);
}

/**
 * 更新单个多APN配置
 *
 * @param current_config 多APN配置指针
 * @param section 配置段名称
 */
static void mobile_update_single_multi_config(module_mutil_config_t *current_config, const char *section) {
    if (current_config == NULL || section == NULL) {
        return;
    }
    
    mobile_init_multi_apn_single(current_config, section);
    mobile_network_single_multi_interface_config(current_config);
    mobile_fw_config_single_multi_setting(current_config);
    mobile_network_dial_multi_single(current_config);
}

/**
 * 处理单个多APN配置更新
 *
 * @param index 配置索引
 * @return 是否处理了更新
 */
static bool mobile_process_single_multi_update(int index) {
    char section[16] = {0};
    char interface[16] = {0};
    char check_file[128] = {0};

    snprintf(section, sizeof(section), "@mapn[%d]", index);
    snprintf(interface, sizeof(interface), "%s0%d", MOBILE_BASICWAN_NAME, index + 1);
    snprintf(check_file, sizeof(check_file), "%s_%d", DEFAULT_MOBILEMULTIAPN_CONFIG_UPDATE, index);
    
    if (access(check_file, F_OK) != 0) {
        return false;
    }

    MOBILE_DEBUG("************* check file %s, interface %s *************\n", check_file, interface);
    module_mutil_config_t *current_config = g_module_desc.mutil_config;
    while (current_config != NULL) {
        MOBILE_DEBUG("current config: %s %s %s %s\n", current_config->iface, current_config->apn, current_config->username, current_config->password);
        if (strcmp(interface, current_config->iface) == 0) {
            mobile_update_single_multi_config(current_config, section);
            break;
        }
        current_config = current_config->next;
    }
    
    unlink(check_file);
    return true;
}

/**
 * 检查multi配置更新
 *
 */
void mobile_check_update_multi_config(void) {
    for (int i = 0; i < MAX_MULTAPNS_COUNT; i++) {
        mobile_process_single_multi_update(i);
    }
}

/**
 * 检查配置更新
 *
 */
void mobile_check_update_apn(void) {
    mobile_check_update_basic_config();
    mobile_check_update_multi_config();
}

/**
 * 获取主apn使能情况
 *
 * @return apn使能
 */
int mobile_get_basic_apn_status(void) {
    MOBILE_INFO("basic apn status:%s\n", g_module_desc.basic_config.enable ? "enable" : "disable");
    return g_module_desc.basic_config.enable;
}

/**
 * 设置APN MCCMNC为数字格式
 * 发送AT命令设置运营商信息显示格式为数字格式（MCCMNC）
 * 用于后续获取运营商信息的格式准备
 *
 * @return 成功返回true，失败返回false
 */
bool mobile_set_apn_mccmnc_num(void) {
    char result[256] = {0};
    char command[64] = {0};

    snprintf(command, sizeof(command), "at+cops=3,2");
    if (mobile_at_cmd((const char*)command, result, sizeof(result)) == 0) {
        MOBILE_DEBUG("result:%s\n", result);
        if (strcasestr(result, "OK")) {
            return true;
        }
    }

    return false;
}

/**
 * 获取APN MCCMNC值
 * 通过AT命令查询当前网络的运营商信息，解析并提取MCCMNC值
 * 首先设置显示格式为数字格式，然后查询运营商信息
 * 解析响应中的MCCMNC值并保存到全局配置中
 *
 * @return 成功返回true，失败返回false
 */
bool mobile_get_apn_mccmnc(void) {
    char result[256] = {0};
    char command[64] = {0};

    memset(g_module_desc.apn_mccmnc, 0, sizeof(g_module_desc.apn_mccmnc));

    // 设置MCCMNC显示格式为数字格式
    if (!mobile_set_apn_mccmnc_num()) {
        return false;
    }

    // 查询运营商信息
    snprintf(command, sizeof(command), "at+cops?");
    if (mobile_at_cmd((const char*)command, result, sizeof(result)) != 0) {
        return false;
    }

    // 检查响应是否包含OK
    if (!strcasestr(result, "OK")) {
        return false;
    }

    // 查找MCCMNC模式
    char *p = strstr(result, ",\"");
    if (p == NULL) {
        MOBILE_DEBUG("MCCMNC pattern not found in response\n");
        return false;
    }

    p += 2; // 跳过逗号和引号

    // 查找结束引号
    char *p1 = strstr(p, "\"");
    if (p1 == NULL) {
        MOBILE_DEBUG("MCCMNC closing quote not found\n");
        return false;
    }

    // 提取MCCMNC值
    *p1 = '\0';
    
    // 安全拷贝并确保字符串终止
    strncpy(g_module_desc.apn_mccmnc, p, sizeof(g_module_desc.apn_mccmnc) - 1);
    g_module_desc.apn_mccmnc[sizeof(g_module_desc.apn_mccmnc) - 1] = '\0';

    // 检查是否获取到有效的MCCMNC值
    if (strlen(p) == 0) {
        MOBILE_DEBUG("get empty apn mccmnc\n");
        return false;
    }

    mobile_write_buff_to_file(PROVIDER_MCCMNC_FILE, g_module_desc.apn_mccmnc, strlen(g_module_desc.apn_mccmnc));
    MOBILE_DEBUG("get apn mccmnc success [%s]\n", g_module_desc.apn_mccmnc);
    
    return true;
}

/**
 * 设置APN MCCMNC为字符串格式
 * 发送AT命令设置运营商信息显示格式为字符串格式（运营商名称）
 * 用于恢复默认的运营商信息显示格式
 *
 * @return 成功返回true，失败返回false
 */
bool mobile_set_apn_mccmnc_str(void) {
    char result[256] = {0};
    char command[64] = {0};

    snprintf(command, sizeof(command), "at+cops=3,0");
    if (mobile_at_cmd((const char*)command, result, sizeof(result)) == 0) {
        if (strcasestr(result, "OK")) {
            return true;
        }
    }

    MOBILE_DEBUG("set apn mccmnc str fail\n");
    return false;
}

/**
 * 获取SIM卡MCCMNC值
 * 根据IMSI值在预定义的运营商列表中查找匹配的MCCMNC
 * 支持完整长度（≥6位）和短长度（<6位）的匹配
 * 对于4位短码会自动补零处理
 * 如果找不到精确匹配，则使用IMSI的前5位作为默认值
 *
 * @return 成功返回true，失败返回false
 */
bool mobile_get_sim_mccmnc(void) {

    memset(g_module_desc.sim_mccmnc, 0, sizeof(g_module_desc.sim_mccmnc));

    if (g_module_desc.imsi[0] == 0) {
        mobile_init_imsi();
        if (g_module_desc.imsi[0] == 0) {
            MOBILE_ERROR("get imsi fail\n");
            return false;
        }
    }

    provider_table_t* tables = mobile_apn_tables();

    // 优先匹配完整长度（≥6），其次短长度（<6）
    for (int is_short = 0; is_short <= 1; is_short++)
    {
        for (size_t t = 0; t < mobile_apn_tables_count(); t++)
        {
            provider_t *plist = tables[t].providers;
            for (size_t i = 0; i < tables[t].count; i++)
            {
                const size_t len = strlen(plist[i].mccmnc);
                if (plist[i].apn[0] == '\0') continue;
                if ((is_short && len >= 6) || (!is_short && len < 6)) continue;

                char processed_mccmnc[8] = {0};
                if (len == 4) { // 处理短码补零逻辑
                    snprintf(processed_mccmnc, sizeof(processed_mccmnc),
                            "%.3s0%c", plist[i].mccmnc, plist[i].mccmnc[3]);
                } else {
                    strncpy(processed_mccmnc, plist[i].mccmnc, sizeof(processed_mccmnc)-1);
                }

                const size_t cmp_len = strlen(processed_mccmnc);
                if (strncmp(g_module_desc.imsi, processed_mccmnc, cmp_len) == 0)
                {
                    strncpy(g_module_desc.sim_mccmnc, processed_mccmnc, sizeof(g_module_desc.sim_mccmnc)-1);
                    mobile_write_buff_to_file(SIM_MCCMNC_FILE, g_module_desc.sim_mccmnc, strlen(g_module_desc.sim_mccmnc));
                    MOBILE_DEBUG("Match: IMSI=%s, MCCMNC=%s (Processed: %s)\n", g_module_desc.imsi, plist[i].mccmnc, processed_mccmnc);
                    return true;
                }
            }
        }
    }

    // 无匹配时的默认处理
    if (g_module_desc.imsi[0]) {
        strncpy(g_module_desc.sim_mccmnc, g_module_desc.imsi, 5);
        g_module_desc.sim_mccmnc[5] = '\0';
        MOBILE_DEBUG("No match, use IMSI prefix: %s\n", g_module_desc.sim_mccmnc);
        return true;
    }

    return false;
}

/**
 * 执行AT命令设置APN配置
 *
 * @param apn APN接入点名称
 * @param username 认证用户名
 * @param password 认证密码
 */
static void mobile_execute_apn_command(const char *apn, const char *username, const char *password) {
    char cmd_buf[512] = {0};
    char result[256] = {0};
    
    // 构建AT命令
    snprintf(cmd_buf, sizeof(cmd_buf), "'AT+EIAAPN=\"%s\",%d,\"%s\",\"%s\",%d,\"%s\",\"%s\"'",
             apn, 0, g_module_desc.basic_config.ip_ver_num, g_module_desc.basic_config.ip_ver_num, 0, username, password);
    MOBILE_DEBUG("APN command: %s\n", cmd_buf);
    
    // 执行AT命令
    if (mobile_at_cmd(cmd_buf, result, sizeof(result)) == 0) {
        MOBILE_INFO("APN configuration successful: %s\n", result);
    } else {
        MOBILE_ERROR("APN configuration failed\n");
    }
}

/**
 * 同步apn到uci配置
 *
 */
static void mobile_sync_auto_apn_config(void) {
    if (g_module_desc.basic_config.enable) {
        mobile_uci_set_option("mobile", "@basic[0]", "apn", g_auto_apn_ctx.current->provider->apn);
        mobile_uci_set_option("mobile", "@basic[0]", "userName", g_auto_apn_ctx.current->provider->username);
        mobile_uci_set_option("mobile", "@basic[0]", "password", g_auto_apn_ctx.current->provider->password);
        mobile_uci_set_option("mobile", "@basic[0]", "dialNumber", g_auto_apn_ctx.current->provider->number);
        mobile_uci_set_option("network", MOBILE_BASICWAN_NAME, "apn", g_auto_apn_ctx.current->provider->apn);
        mobile_uci_set_option("network", MOBILE_BASICWAN_NAME, "username", g_auto_apn_ctx.current->provider->username);
        mobile_uci_set_option("network", MOBILE_BASICWAN_NAME, "password", g_auto_apn_ctx.current->provider->password);
        MOBILE_DEBUG("update auto apn to uci config file: %s\n", g_auto_apn_ctx.current->provider->apn);
        sprintf(g_module_desc.basic_config.username, "%s", g_auto_apn_ctx.current->provider->username);
        sprintf(g_module_desc.basic_config.password, "%s", g_auto_apn_ctx.current->provider->password);
        sprintf(g_module_desc.basic_config.dial_number, "%s", g_auto_apn_ctx.current->provider->number);
        sprintf(g_module_desc.basic_config.apn, "%s", g_auto_apn_ctx.current->provider->apn);
    }
}

/**
 * 函数1：初始化APN配置
 *
 * 功能说明：
 * 1. 重置所有auto_provider_list中的状态为0（未使用）
 * 2. 如果是手动模式，使用基础配置中的参数
 * 3. 如果是自动模式，使用列表中的第一项，状态置为2（已使用未验证）
 * 4. 执行AT命令设置APN
 * 5. 无返回值
 */
void mobile_init_apn_in_modem(void) {
    const char *apn = NULL;
    const char *username = NULL;
    const char *password = NULL;
    
    // 重置重试计数器
    g_auto_apn_ctx.retry_count = 0;
    g_auto_apn_ctx.current = NULL;
    
    // 检查手动APN设置是否启用
    if (g_module_desc.basic_config.manual_apn_enable == 1) {
        // 手动设置模式：使用basic_config中的参数
        MOBILE_INFO("Manual APN configuration mode\n");
        apn = g_module_desc.basic_config.apn;
        username = g_module_desc.basic_config.username;
        password = g_module_desc.basic_config.password;
    } else {
        // 自动设置模式
        MOBILE_INFO("Auto APN configuration mode\n");
        
        // 检查自动APN列表是否已初始化
        if (g_auto_apn_ctx.list == NULL) {
            MOBILE_ERROR("Auto provider list not initialized\n");
            return;
        }
        
        // 重置所有APN配置状态为未使用
        auto_provider_t *current = g_auto_apn_ctx.list;
        while (current != NULL) {
            current->status = APN_STATUS_UNUSED;
            current = current->next;
        }
        
        // 使用列表中的第一项
        if (g_auto_apn_ctx.list != NULL) {
            g_auto_apn_ctx.current = g_auto_apn_ctx.list;
            g_auto_apn_ctx.current->status = APN_STATUS_USED_UNVERIFIED;
            apn = g_auto_apn_ctx.current->provider->apn;
            username = g_auto_apn_ctx.current->provider->username;
            password = g_auto_apn_ctx.current->provider->password;
            MOBILE_INFO("Auto APN init: using first APN: %s, status set to UNVERIFIED\n",
                       g_auto_apn_ctx.current->provider->apn);
        } else {
            MOBILE_ERROR("No APN configuration available in auto list\n");
            return;
        }
    }
    
    // 执行AT命令设置APN
    mobile_execute_apn_command(apn, username, password);
}

/**
 * 函数2：检查自动APN配置
 *
 * 功能说明：
 * 1. 重复调用逻辑，维护调用计数器（每2秒计数一次）
 * 2. 超过15次则标记当前项为-1（已设置但无效）
 * 3. 使用下一项，状态置为2（已使用未验证）
 * 4. 执行AT命令设置新的APN
 * 5. 无返回值
 */
void mobile_check_auto_apn(void) {
    static unsigned int s_last_apn_check_time = 0;
    unsigned int now_time = mobile_get_uptime_in_ms();
    const char *apn = NULL;
    const char *username = NULL;
    const char *password = NULL;
    
    // 如果是手动模式，直接返回
    if (g_module_desc.basic_config.manual_apn_enable == 1) {
        MOBILE_INFO("Manual APN configuration mode, skip auto APN check\n");
        return;
    }
    
    // 检查自动APN列表是否已初始化
    if (g_auto_apn_ctx.list == NULL) {
        MOBILE_ERROR("Auto provider list not initialized\n");
        return;
    }
    
    // 时间计数：只有超过2秒才会增加重试计数器
    if ((now_time - s_last_apn_check_time) < 2000) {
        MOBILE_DEBUG("APN check too frequent, skip retry count increase\n");
        return; // 时间间隔不足2秒，直接返回
    }
    
    s_last_apn_check_time = now_time;
    
    // 增加重试计数器
    g_auto_apn_ctx.retry_count++;
    MOBILE_INFO("APN retry count: %d\n", g_auto_apn_ctx.retry_count);
    
    // 如果重试次数超过15次，标记当前项为无效
    if (g_auto_apn_ctx.retry_count > 15 && g_auto_apn_ctx.current != NULL) {
        MOBILE_WARN("APN retry count exceeded 15, marking current APN as invalid: %s\n",
                   g_auto_apn_ctx.current->provider->apn);
        g_auto_apn_ctx.current->status = APN_STATUS_USED_INVALID;
        g_auto_apn_ctx.current = NULL;
        g_auto_apn_ctx.retry_count = 0; // 重置计数器
    }
    
    // 如果当前没有可用的provider，重新初始化列表状态
    if (g_auto_apn_ctx.current == NULL) {
        MOBILE_INFO("No current provider, resetting APN list status\n");
        // 重置所有APN配置状态为未使用
        auto_provider_t *current = g_auto_apn_ctx.list;
        while (current != NULL) {
            current->status = APN_STATUS_UNUSED;
            current = current->next;
        }
        // 使用列表中的第一项
        g_auto_apn_ctx.current = g_auto_apn_ctx.list;
        if (g_auto_apn_ctx.current != NULL) {
            g_auto_apn_ctx.current->status = APN_STATUS_USED_UNVERIFIED;
            apn = g_auto_apn_ctx.current->provider->apn;
            username = g_auto_apn_ctx.current->provider->username;
            password = g_auto_apn_ctx.current->provider->password;
            MOBILE_INFO("Auto APN check: using first APN: %s, status set to UNVERIFIED\n",
                       g_auto_apn_ctx.current->provider->apn);
            // 打印当前自动APN配置信息
            mobile_print_auto_provider_info(g_auto_apn_ctx.current);
        } else {
            return;
        }
    } else {
        // 查找下一个可用的APN配置
        auto_provider_t *next_provider = NULL;
        
        // 从当前项的下一个开始查找
        next_provider = g_auto_apn_ctx.current->next;
        
        // 遍历列表，找到第一个未使用或未验证的项
        while (next_provider != NULL) {
            if (next_provider->status == APN_STATUS_UNUSED ||
                next_provider->status == APN_STATUS_USED_UNVERIFIED) {
                break;
            }
            next_provider = next_provider->next;
        }
        
        // 如果没找到，从头开始查找
        if (next_provider == NULL) {
            next_provider = g_auto_apn_ctx.list;
            while (next_provider != NULL) {
                if (next_provider->status == APN_STATUS_UNUSED ||
                    next_provider->status == APN_STATUS_USED_UNVERIFIED) {
                    break;
                }
                next_provider = next_provider->next;
            }
        }
        
        // 如果找到了可用的配置项
        if (next_provider != NULL) {
            g_auto_apn_ctx.current = next_provider;
            g_auto_apn_ctx.current->status = APN_STATUS_USED_UNVERIFIED;
            apn = g_auto_apn_ctx.current->provider->apn;
            username = g_auto_apn_ctx.current->provider->username;
            password = g_auto_apn_ctx.current->provider->password;
            MOBILE_INFO("Auto APN check: using next APN: %s, status set to UNVERIFIED\n",
                       g_auto_apn_ctx.current->provider->apn);
            // 打印当前自动APN配置信息
            mobile_print_auto_provider_info(g_auto_apn_ctx.current);
        } else {
            // 所有配置项都已尝试过且无效
            MOBILE_ERROR("All APN configurations in auto list have been tried and are invalid\n");
            return;
        }
    }
    
    // 执行AT命令设置APN
    mobile_execute_apn_command(apn, username, password);
}

/**
 * 函数3：同步自动APN配置
 *
 * 功能说明：
 * 1. 拨号成功后调用，标记当前使用的项为1（已设置且有效）
 * 2. 设置UCI配置文件
 *
 * @return 无返回值
 */
void mobile_sync_auto_apn(void) {
    // 重置重试计数器
    g_auto_apn_ctx.retry_count = 0;
    
    // 如果是手动模式或没有当前使用的provider，直接返回
    if (g_module_desc.basic_config.manual_apn_enable == 1 ||
        g_auto_apn_ctx.current == NULL) {
        return;
    }

    // 标记当前使用的APN配置为有效
    g_auto_apn_ctx.current->status = APN_STATUS_USED_VALID;
    mobile_sync_auto_apn_config();
    
    // 打印当前自动APN配置的所有元素信息
    mobile_print_auto_provider_info(g_auto_apn_ctx.current);
}

/**
 * 打印自动APN提供者信息
 * 打印当前自动APN配置的所有元素信息
 *
 * @param provider 自动APN提供者指针
 * @return 无返回值
 */
void mobile_print_auto_provider_info(auto_provider_t *provider) {
    if (provider == NULL) {
        MOBILE_INFO("Auto Provider: NULL\n");
        return;
    }
    
    MOBILE_INFO("=== Current Auto APN Provider Information ===\n");
    
    if (provider->provider != NULL) {
        MOBILE_INFO("Provider Details:\n");
        MOBILE_INFO("  APN: %s\n", provider->provider->apn);
        MOBILE_INFO("  Username: %s\n", provider->provider->username);
        MOBILE_INFO("  Password: %s\n", provider->provider->password);
        MOBILE_INFO("  MCCMNC: %s\n", provider->provider->mccmnc);
        MOBILE_INFO("  Number: %s\n", provider->provider->number);
        MOBILE_INFO("  Status: %s\n", mobile_get_apn_status_str(provider->status));
    } else {
        MOBILE_INFO("Provider Details: NULL\n");
    }
}

/**
 * 打印APN配置信息
 *
 * @return 无返回值
 */
void mobile_print_apn_info(void) {
    module_mutil_config_t *current_config = g_module_desc.mutil_config;
    int config_count = 0;

    // 基础配置信息
    MOBILE_INFO("=== Basic APN Configuration Information ===\n");
    MOBILE_INFO("Basic Config - Enable: %d\n", g_module_desc.basic_config.enable);
    MOBILE_INFO("Basic Config - APN: %s\n", g_module_desc.basic_config.apn);
    MOBILE_INFO("Basic Config - Username: %s\n", g_module_desc.basic_config.username);
    MOBILE_INFO("Basic Config - Password: %s\n", g_module_desc.basic_config.password);
    MOBILE_INFO("Basic Config - PIN: %s\n", g_module_desc.basic_config.pin_number);
    MOBILE_INFO("Basic Config - Net Type: %s\n", g_module_desc.basic_config.net_type);
    MOBILE_INFO("\n");

    if (current_config == NULL) {
        MOBILE_INFO("No multi APN configurations available\n");
        return;
    }
    MOBILE_INFO("=== Multi APN Configuration Information ===\n");

    while (current_config != NULL) {
        MOBILE_INFO("Multi APN Config[%d]:\n", config_count);
        MOBILE_INFO("  Enable: %d\n", current_config->enable);
        MOBILE_INFO("  APN: %s\n", current_config->apn);
        MOBILE_INFO("  Username: %s\n", current_config->username);
        MOBILE_INFO("  Password: %s\n", current_config->password);
        MOBILE_INFO("  Dial Number: %s\n", current_config->dial_number);
        MOBILE_INFO("  IP Version: %s\n", current_config->ip_version);
        MOBILE_INFO("  Active: %d\n", current_config->active);
        MOBILE_INFO("  Delegate: %d\n", current_config->delegate);
        MOBILE_INFO("  VLAN ID: %d\n", current_config->vlanid);
        MOBILE_INFO("  CID: %d\n", current_config->cid);
        MOBILE_INFO("  Bridge Enable: %d\n", current_config->bridge_enable);
        MOBILE_INFO("  NAT Enable: %d\n", current_config->nat_enable);
        MOBILE_INFO("  Interface: %s\n", current_config->iface);
        MOBILE_INFO("  Auth Type: %s\n", current_config->auth_type);
        MOBILE_INFO("  Next Config Address: %p\n", current_config->next);
        current_config = current_config->next;
        config_count++;
    }
    MOBILE_INFO("\n");
}
