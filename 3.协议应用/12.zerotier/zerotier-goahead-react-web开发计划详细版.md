# ZeroTier 支持详细开发计划（goahead + react-web，替代 luci-app-zerotier）

> 参考：MTK7981 `r47602`（wireguard）的功能与函数划分模式
> 适配：X75 现有 goahead/react-web 代码基础设施
> 本计划为代码级细化，含函数签名、JSON 字段、UCI 读写、NAT 脚本全文

---

## 〇、关键前置结论（探索确认的事实）

### 0.1 X75 代码基础设施与 r47602 的差异（必须适配，否则编译失败）

| 功能 | r47602 (MTK7981) | X75 实际可用 | 结论 |
|---|---|---|---|
| JSON 解析 | `json_tokener_parse` | `convert_string_to_json(char*)` | 用 X75 的 |
| JSON 取值 | `json_object_object_get_ex` + `json_object_get_string` | `get_strval_from_json` / `get_intval_from_json` / `get_boolval_from_json` | 标量用 X75 的，数组仍用 json-c |
| JSON 响应 | `resposeweb(wp, ...)` | `web_back(wp, errcode, msg, time, json_info)` | 用 X75 的 |
| 字符串分割 | `str_split_iter`（r47602 新增） | `split_str` / 标准库 `strtok_r` | 用 `strtok_r`（免新增） |
| 防火墙辅助 | 直接写 zone+rule | `uci_set_one_vpn_firewall_info(section, add)` 已有 | 可选复用 |
| ACTION 枚举 | `cmd_type_t`（utility.h） | 有 `NO_ACTION`，无 ZEROTIER_SET | 用 `NO_ACTION`（exec 内自调 system） |

### 0.2 X75 关键文件已确认

- 后端：`broadlink/package/goahead/src/src/platform/wrt/{sub_vpn.c, sub_vpn.h, sub_login.c, uci_utility.h}`、`cmd.c`、`parse_data.h`、`utility.h`
- 前端：`broadlink/package/react-web/src/web/src/views/settings/vpn/{index.js, component/}`、`api/settings.js`、`public/locales/{cn,en,tw}/`
- 包分发：`broadlink/package/goahead/files/`（现有 `goahead.init`，NAT 脚本加这里）
- 构建配置：`broadlink/target/X75-ODU-*/target.config`

---

## 一、目标与范围

在 X75 新增 ZeroTier VPN：react-web 网页配置 → goahead 写 zerotier UCI + 防火墙 → 复用 zerotier 主包 + 自带 NAT 脚本。**完全替代 luci-app-zerotier**。

### 总体架构

```
react-web (views/settings/vpn/component/zerotier.js)
        │ GET /acfun/getZerotierInfo  POST /acfun/zerotierSettings
        ▼
goahead (cmd.c 分发 → sub_vpn.c)
        ├─ get_zerotier_info / ZerotierGet_back        (读 UCI → JSON)
        ├─ ZerotierSet_parse / set_zerotier            (JSON → 写 UCI)
        └─ ZerotierSet_exec:
             ├─ 写 /etc/config/zerotier (sample_config)
             ├─ 写 /etc/config/firewall (rule + include)
             ├─ system("/etc/init.d/zerotier restart/stop")
             └─ system("/etc/init.d/firewall restart")
                    │
                    ▼
             /usr/share/zerotier/firewall.include
                    │
                    ▼
             /etc/init.d/zerotier-nat (等 zt 网卡 + iptables/nft)
```

---

## 二、后端详细设计（goahead）

### 2.1 `sub_vpn.h` 新增（`#endif` 前，仿 r47602 第 604-607、617-649 行）

```c
/* ============ ZeroTier ============ */
#define UCI_CONFIG_ZEROTIER       "/etc/config/zerotier"
#define ZEROTIER_INTERFACE        "sample_config"      /* zerotier 的 named section */

/* JSON 键名（前端字段名） */
#define KEY_ZT_ENABLED            "enabled"
#define KEY_ZT_JOIN               "join"
#define KEY_ZT_NAT                "nat"
#define KEY_ZT_PORT               "port"
#define KEY_ZT_SECRET             "secret"
#define KEY_ZT_CONFIG_PATH        "config_path"
#define KEY_ZT_COPY_CONFIG_PATH   "copy_config_path"
#define KEY_ZT_LOCAL_CONF         "local_conf"

typedef struct {
    char enabled[4];          /* "0"/"1" */
    char join[256];           /* 空格分隔的多网络 ID，对应 UCI list */
    char nat[4];              /* "0"/"1" */
    char port[16];            /* 默认 9993 */
    char secret[128];         /* identity.secret，留空自动生成 */
    char config_path[64];
    char copy_config_path[4];
    char local_conf[64];
} zerotier_t;

char* ZerotierGet_parse(void* data, int* errcode);
void  ZerotierGet_back(Webs* wp, void* data, int* errcode);
char* ZerotierSet_parse(void* data, int* errcode);
char* ZerotierSet_exec(void* data, int* errcode);
```

### 2.2 `sub_vpn.c` 新增函数（文件末尾追加）

**全局结构体**（仿 r47602 `g_wireguard_server`）：`zerotier_t g_zerotier;`

**① `get_zerotier_info`（static，读 UCI → 结构体）**

```c
static void get_zerotier_info(zerotier_t *info) {
    char str_tmp[COMMENT_LEN_256] = {0};
    memset(info, 0, sizeof(zerotier_t));

    get_uci_option_value(UCI_CONFIG_ZEROTIER, ZEROTIER_INTERFACE, KEY_ZT_ENABLED, str_tmp, sizeof(str_tmp));
    DO_SAFE_DARRAY_COPY(info->enabled, str_tmp);
    if (info->enabled[0] == 0) strcpy(info->enabled, "0");

    get_uci_option_value(UCI_CONFIG_ZEROTIER, ZEROTIER_INTERFACE, KEY_ZT_NAT, str_tmp, sizeof(str_tmp));
    DO_SAFE_DARRAY_COPY(info->nat, str_tmp);
    if (info->nat[0] == 0) strcpy(info->nat, "0");

    get_uci_option_value(UCI_CONFIG_ZEROTIER, ZEROTIER_INTERFACE, KEY_ZT_PORT, str_tmp, sizeof(str_tmp));
    DO_SAFE_DARRAY_COPY(info->port, str_tmp);
    if (info->port[0] == 0) strcpy(info->port, "9993");

    get_uci_option_value(UCI_CONFIG_ZEROTIER, ZEROTIER_INTERFACE, KEY_ZT_SECRET, str_tmp, sizeof(str_tmp));
    DO_SAFE_DARRAY_COPY(info->secret, str_tmp);

    get_uci_option_value(UCI_CONFIG_ZEROTIER, ZEROTIER_INTERFACE, KEY_ZT_CONFIG_PATH, str_tmp, sizeof(str_tmp));
    DO_SAFE_DARRAY_COPY(info->config_path, str_tmp);

    get_uci_option_value(UCI_CONFIG_ZEROTIER, ZEROTIER_INTERFACE, KEY_ZT_COPY_CONFIG_PATH, str_tmp, sizeof(str_tmp));
    DO_SAFE_DARRAY_COPY(info->copy_config_path, str_tmp);

    get_uci_option_value(UCI_CONFIG_ZEROTIER, ZEROTIER_INTERFACE, KEY_ZT_LOCAL_CONF, str_tmp, sizeof(str_tmp));
    DO_SAFE_DARRAY_COPY(info->local_conf, str_tmp);

    /* join 是 list：uci_get_option_str 自动空格拼接 */
    memset(str_tmp, 0, sizeof(str_tmp));
    uci_get_option_str(UCI_CONFIG_ZEROTIER "." ZEROTIER_INTERFACE "." KEY_ZT_JOIN, str_tmp, sizeof(str_tmp));
    DO_SAFE_DARRAY_COPY(info->join, str_tmp);
}
```

**② `ZerotierGet_back`（读 + JSON 响应）**

```c
char* ZerotierGet_parse(void* data, int* errcode) { *errcode = SUC_CODE; return NULL; }

void ZerotierGet_back(Webs* wp, void* data, int* errcode) {
    zerotier_t info;
    struct json_object *info_json = NULL, *join_arr = NULL;
    char *token = NULL, *saveptr = NULL;

    memset(&info, 0, sizeof(info));
    get_zerotier_info(&info);

    *errcode = SUC_CODE;
    info_json = json_object_new_object();
    join_arr  = json_object_new_array();

    json_object_object_add(info_json, KEY_ZT_ENABLED,          json_object_new_boolean(atoi(info.enabled)));
    json_object_object_add(info_json, KEY_ZT_NAT,              json_object_new_boolean(atoi(info.nat)));
    json_object_object_add(info_json, KEY_ZT_PORT,             json_object_new_string(info.port));
    json_object_object_add(info_json, KEY_ZT_SECRET,           json_object_new_string(info.secret));
    json_object_object_add(info_json, KEY_ZT_CONFIG_PATH,      json_object_new_string(info.config_path));
    json_object_object_add(info_json, KEY_ZT_COPY_CONFIG_PATH, json_object_new_boolean(atoi(info.copy_config_path)));
    json_object_object_add(info_json, KEY_ZT_LOCAL_CONF,       json_object_new_string(info.local_conf));

    /* join：空格分隔串 → JSON 数组 */
    token = strtok_r(info.join, " \t,", &saveptr);
    while (token) {
        json_object_array_add(join_arr, json_object_new_string(token));
        token = strtok_r(NULL, " \t,", &saveptr);
    }
    json_object_object_add(info_json, KEY_ZT_JOIN, join_arr);

    web_back(wp, *errcode, NULL, 0, info_json);
    websDone(wp);
    return;
}
```

**③ `ZerotierSet_parse`（JSON → 全局结构体）**

```c
char* ZerotierSet_parse(void* data, int* errcode) {
    void *reqjson = NULL;
    json_object *joinArr = NULL, *item = NULL;
    int i, len;
    const char *ip = NULL;

    *errcode = SUC_CODE;
    reqjson = convert_string_to_json((char*)data);
    if (NULL == reqjson) { *errcode = TRANSMIT_PARA_ERR; return NULL; }

    memset(&g_zerotier, 0, sizeof(g_zerotier));

    snprintf(g_zerotier.enabled, sizeof(g_zerotier.enabled), "%d",
             get_boolval_from_json(reqjson, KEY_ZT_ENABLED));
    snprintf(g_zerotier.nat, sizeof(g_zerotier.nat), "%d",
             get_boolval_from_json(reqjson, KEY_ZT_NAT));
    DO_SAFE_DARRAY_COPY(g_zerotier.port, get_strval_from_json(reqjson, KEY_ZT_PORT));
    DO_SAFE_DARRAY_COPY(g_zerotier.secret, get_strval_from_json(reqjson, KEY_ZT_SECRET));
    DO_SAFE_DARRAY_COPY(g_zerotier.config_path, get_strval_from_json(reqjson, KEY_ZT_CONFIG_PATH));
    snprintf(g_zerotier.copy_config_path, sizeof(g_zerotier.copy_config_path), "%d",
             get_boolval_from_json(reqjson, KEY_ZT_COPY_CONFIG_PATH));
    DO_SAFE_DARRAY_COPY(g_zerotier.local_conf, get_strval_from_json(reqjson, KEY_ZT_LOCAL_CONF));

    /* join：JSON 数组 → 空格拼接（json-c 原生，仿 r47602 第 415-430 行） */
    if (json_object_object_get_ex((json_object*)reqjson, KEY_ZT_JOIN, &joinArr)
        && json_object_get_type(joinArr) == json_type_array) {
        len = json_object_array_length(joinArr);
        for (i = 0; i < len; i++) {
            item = json_object_array_get_idx(joinArr, i);
            ip = json_object_get_string(item);
            if (strlen(g_zerotier.join) + strlen(ip) + 2 >= sizeof(g_zerotier.join)) break;
            if (i > 0)
                strncat(g_zerotier.join, " ", sizeof(g_zerotier.join) - strlen(g_zerotier.join) - 1);
            strncat(g_zerotier.join, ip ? ip : "",
                    sizeof(g_zerotier.join) - strlen(g_zerotier.join) - 1);
        }
    }

    json_object_put(reqjson);
    return NULL;
}
```

**④ `set_zerotier`（static，写 `/etc/config/zerotier`）**

```c
static void set_zerotier(void) {
    char *saveptr = NULL, *token = NULL;

    /* 先删旧 join list，避免残留 */
    uci_del_section_option(UCI_CONFIG_ZEROTIER, ZEROTIER_INTERFACE, KEY_ZT_JOIN);

    /* 注意：zerotier 直接用 enabled，非 wireguard 的 disabled 反义 */
    set_uci_option_value(UCI_CONFIG_ZEROTIER, ZEROTIER_INTERFACE, KEY_ZT_ENABLED,
                         atoi(g_zerotier.enabled) == 1 ? "1" : "0");
    set_uci_option_value(UCI_CONFIG_ZEROTIER, ZEROTIER_INTERFACE, KEY_ZT_NAT,
                         atoi(g_zerotier.nat) == 1 ? "1" : "0");

    if (g_zerotier.port[0])
        set_uci_option_value(UCI_CONFIG_ZEROTIER, ZEROTIER_INTERFACE, KEY_ZT_PORT, g_zerotier.port);

    /* secret 空则删除，让 zerotier 包首次启动自动生成 */
    if (g_zerotier.secret[0])
        set_uci_option_value(UCI_CONFIG_ZEROTIER, ZEROTIER_INTERFACE, KEY_ZT_SECRET, g_zerotier.secret);
    else
        uci_del_section_option(UCI_CONFIG_ZEROTIER, ZEROTIER_INTERFACE, KEY_ZT_SECRET);

    if (g_zerotier.config_path[0])
        set_uci_option_value(UCI_CONFIG_ZEROTIER, ZEROTIER_INTERFACE, KEY_ZT_CONFIG_PATH, g_zerotier.config_path);
    else
        uci_del_section_option(UCI_CONFIG_ZEROTIER, ZEROTIER_INTERFACE, KEY_ZT_CONFIG_PATH);

    set_uci_option_value(UCI_CONFIG_ZEROTIER, ZEROTIER_INTERFACE, KEY_ZT_COPY_CONFIG_PATH,
                         atoi(g_zerotier.copy_config_path) == 1 ? "1" : "0");

    if (g_zerotier.local_conf[0])
        set_uci_option_value(UCI_CONFIG_ZEROTIER, ZEROTIER_INTERFACE, KEY_ZT_LOCAL_CONF, g_zerotier.local_conf);
    else
        uci_del_section_option(UCI_CONFIG_ZEROTIER, ZEROTIER_INTERFACE, KEY_ZT_LOCAL_CONF);

    /* join：空格拆分 → 逐个 addlist */
    token = strtok_r(g_zerotier.join, " \t,", &saveptr);
    while (token) {
        uci_addlist_config_value_str(UCI_CONFIG_ZEROTIER, ZEROTIER_INTERFACE, KEY_ZT_JOIN, token);
        token = strtok_r(NULL, " \t,", &saveptr);
    }
}
```

**⑤ `set_zerotier_firewall`（static，写 `/etc/config/firewall`）**

```c
static void set_zerotier_firewall(void) {
    char port_str[16] = "9993";
    if (g_zerotier.port[0])
        strncpy(port_str, g_zerotier.port, sizeof(port_str) - 1);

    /* ① 放行入站 UDP（让外部 peer 连接）——具名 section 固定覆盖 */
    uci_del_section_option(UCI_CONFIG_FIREWALL, "zerotier", NULL);
    set_uci_option_value(UCI_CONFIG_FIREWALL, "zerotier", NULL, "rule");
    set_uci_option_value(UCI_CONFIG_FIREWALL, "zerotier", "name", "Allow-ZeroTier");
    set_uci_option_value(UCI_CONFIG_FIREWALL, "zerotier", "src", "wan");
    set_uci_option_value(UCI_CONFIG_FIREWALL, "zerotier", "dest_port", port_str);
    set_uci_option_value(UCI_CONFIG_FIREWALL, "zerotier", "proto", "udp");
    set_uci_option_value(UCI_CONFIG_FIREWALL, "zerotier", "target", "ACCEPT");

    /* ② NAT include 段（替代 luci-app-zerotier 的 uci-defaults 注册） */
    uci_del_section_option(UCI_CONFIG_FIREWALL, "zerotier_nat", NULL);
    set_uci_option_value(UCI_CONFIG_FIREWALL, "zerotier_nat", NULL, "include");
    set_uci_option_value(UCI_CONFIG_FIREWALL, "zerotier_nat", "type", "script");
    set_uci_option_value(UCI_CONFIG_FIREWALL, "zerotier_nat", "path", "/usr/share/zerotier/firewall.include");
    set_uci_option_value(UCI_CONFIG_FIREWALL, "zerotier_nat", "reload", "1");
}
```

**⑥ `ZerotierSet_exec`（写 UCI + system 命令）**

```c
char* ZerotierSet_exec(void* data, int* errcode) {
    *errcode = SUC_CODE;

    set_zerotier();                    /* ① 写 /etc/config/zerotier */
    set_zerotier_firewall();           /* ② 写 /etc/config/firewall */

    if (atoi(g_zerotier.enabled) == 1)
        system("/etc/init.d/zerotier restart");
    else
        system("/etc/init.d/zerotier stop");

    system("/etc/init.d/firewall restart");

    return NULL;
}
```

### 2.3 `cmd.c` 注册（VPN section 约 396-440 行，仿 r47602 用 NO_ACTION）

```c
{"getZerotierInfo", ZerotierGet_parse, NULL, ZerotierGet_back, NULL, NO_ACTION},
{"zerotierSettings", ZerotierSet_parse, ZerotierSet_exec, NULL, NULL, NO_ACTION},
```

> 不需要新增 ACTION 枚举、不改 `fun_action_output_post`（exec 内已自调 system）。

### 2.4 `sub_login.c` 菜单（`glb_vpn_web_showctrl_arr[]` 约 867-885 行）

```c
{"vpn", "ZeroTier", "zerotier", true},
```

> 加在 OpenVPN 项（879 行）之后。`moduleKey` 必须与前端 `case 'zerotier'` 一致。

---

## 三、前端详细设计（react-web）

### 3.1 新增 `views/settings/vpn/component/zerotier.js`

结构仿 r47602 `wireGuardClient.js`（单表单 + `Tabs` 分层）：

| JSON 字段 | 控件 | 必填 | 校验 | tab |
|---|---|---|---|---|
| `enabled` | `Switch` | — | — | General |
| `join` | `Form.List`（多网络 ID） | 是 | 16 位 hex 网络 ID | General |
| `nat` | `Checkbox` | — | — | General |
| `port` | `Input` | 否 | 1-65535 | Advanced |
| `secret` | `Input.Password` | 否 | 留空自动生成 | Advanced |
| `config_path` | `Input` | 否 | — | Advanced |
| `copy_config_path` | `Checkbox` | — | — | Advanced |
| `local_conf` | `Input` | 否 | — | Advanced |

实现点：
- `join` 用 `<Form.List>`（可增删），提交前 `filter` 掉空项（仿 r47602 第 752-754 行）
- `enabled` 用 `shouldUpdate` 控制其余表单项显示（仿 r47602 第 777 行）
- 提交 `POST /acfun/zerotierSettings`，成功弹 `confirmAlert` 倒计时

### 3.2 `api/settings.js` 新增

```js
export const getZerotierInfo = async () => await request.get('/acfun/getZerotierInfo');
export const zerotierSettings = async (data) => await request.post('/acfun/zerotierSettings', data);
```

### 3.3 `views/settings/vpn/index.js`

```js
import ZeroTier from './component/zerotier';
// showComponent 的 switch 增加
case 'zerotier': return <ZeroTier />
```

### 3.4 国际化 `public/locales/{cn,en,tw}/*.json`

新增词条：`ZeroTier`、`Network ID`、`Auto NAT Clients`、`Port`、`Secret`、`Configuration folder`、`Copy configuration folder`、`Local configuration`、`Enable` 等。

---

## 四、NAT 脚本设计（随 goahead 分发，仿 luci-app-zerotier）

### 4.1 分发路径（放 `goahead/files/`，随包 `$(CP) ./files/* $(1)/` 到根文件系统）

| 源码路径 | 安装路径 | 职责 |
|---|---|---|
| `goahead/files/etc/init.d/zerotier-nat` | `/etc/init.d/zerotier-nat` | 等 zt 网卡 + fw3/fw4 NAT 规则 |
| `goahead/files/usr/share/zerotier/firewall.include` | `/usr/share/zerotier/firewall.include` | firewall reload 触发 NAT |

### 4.2 `zerotier-nat` 脚本全文（照搬 luci-zerotier 逻辑）

```sh
#!/bin/sh /etc/rc.common

START=99

PROG=/etc/init.d/zerotier

get_config() {
    config_get_bool enabled $1 enabled 0
    config_get_bool nat $1 nat 0
}

log() {
    echo "$@"
    logger -t zerotier-nat "$@"
}

start() {
    config_load zerotier
    config_foreach get_config zerotier

    [ $enabled -eq 0 ] && return 0
    if ! $PROG running; then return 1; fi
    [ $nat -eq 0 ] && return 0

    # 等 zt 网卡出现
    while [ "$(ifconfig | grep 'zt' | awk '{print $1}')" = "" ]; do
        sleep 1
    done

    local zt_devs FW ip_segment
    zt_devs="$(ifconfig | grep 'zt' | awk '{print $1}')"
    [ -x "$(command -v nft)" ] && FW="fw4" || FW="fw3"

    for i in ${zt_devs}; do
        ip_segment="$(ip route | grep "dev $i proto kernel" | awk '{print $1}')"
        if [ "$FW" = "fw3" ]; then
            iptables -S FORWARD | grep -sq "$i" && continue
            iptables -I FORWARD -i "$i" -j ACCEPT
            iptables -I FORWARD -o "$i" -j ACCEPT
            iptables -t nat -I POSTROUTING -o "$i" -j MASQUERADE
            [ -n "$ip_segment" ] && iptables -t nat -I POSTROUTING -s "${ip_segment}" -j MASQUERADE
        else
            nft list chain inet fw4 forward | grep -sq "$i" && continue
            nft insert rule inet fw4 forward iifname "$i" accept
            nft insert rule inet fw4 forward oifname "$i" accept
            nft insert rule inet fw4 srcnat oifname "$i" counter masquerade
            [ -n "$ip_segment" ] && nft insert rule inet fw4 srcnat ip saddr "${ip_segment}" counter masquerade
        fi
        log "Added nat rules for zt interface $i"
    done
}

stop() {
    local zt_devs FW ip_segment rule chains handles
    zt_devs="$(ifconfig | grep 'zt' | awk '{print $1}')"
    [ -z "${zt_devs}" ] && return 0
    [ -x "$(command -v nft)" ] && FW="fw4" || FW="fw3"

    for i in ${zt_devs}; do
        ip_segment="$(ip route | grep "dev $i proto kernel" | awk '{print $1}')"
        if [ "$FW" = "fw3" ]; then
            iptables -D FORWARD -i "$i" -j ACCEPT 2>/dev/null
            iptables -D FORWARD -o "$i" -j ACCEPT 2>/dev/null
            iptables -t nat -D POSTROUTING -o "$i" -j MASQUERADE 2>/dev/null
            [ -n "$ip_segment" ] && iptables -t nat -D POSTROUTING -s "${ip_segment}" -j MASQUERADE 2>/dev/null
        else
            chains="forward srcnat"
            rule="$i"
            [ -n "$ip_segment" ] && rule="${rule}|$ip_segment"
            for c in $chains; do
                handles=$(nft -a list chain inet fw4 $c | grep -E "$rule" | cut -d'#' -f2 | cut -d' ' -f3)
                for h in $handles; do
                    nft delete rule inet fw4 $c handle $h
                done
            done
        fi
    done
}
```

### 4.3 `firewall.include` 全文

```sh
#!/bin/sh
/etc/init.d/zerotier-nat enabled && /etc/init.d/zerotier-nat restart
exit 0
```

### 4.4 NAT 触发链

```
ZerotierSet_exec → system("firewall restart")
        → fw3/fw4 构建时执行 include 脚本 /usr/share/zerotier/firewall.include
        → zerotier-nat restart → 等 zt 网卡 → 插/删 NAT 规则
```

---

## 五、构建与包配置

| 文件 | 改动 |
|---|---|
| `broadlink/target/X75-ODU-*/target.config` | 启用 `CONFIG_PACKAGE_zerotier=y`（依赖 `kmod-tun`、`libminiupnpc`、`libnatpmp` 由 zerotier 包自动带入） |
| `goahead/files/` | 新增 `etc/init.d/zerotier-nat` + `usr/share/zerotier/firewall.include`（与现有 `goahead.init` 并列） |
| zerotier 主包 | 无需改（X75 feeds 已含 `owrt-feeds/packages/net/zerotier`） |
| `uci_utility.h` | 若无 `UCI_CONFIG_ZEROTIER` 宏，在 sub_vpn.h 内定义即可 |

---

## 六、UCI ↔ JSON ↔ 结构体 三方字段映射总表

| UCI option | UCI 类型 | JSON 键 | 结构体字段 | 前端控件 | 读写函数 |
|---|---|---|---|---|---|
| `enabled` | bool | `enabled` | `enabled` | Switch | set/get_uci_option_value |
| `join` | list | `join[]` | `join`(空格串) | Form.List | uci_addlist / uci_get_option_str |
| `nat` | bool | `nat` | `nat` | Checkbox | set/get_uci_option_value |
| `port` | string | `port` | `port` | Input | set/get_uci_option_value |
| `secret` | string | `secret` | `secret` | Input.Password | set/get_uci_option_value |
| `config_path` | string | `config_path` | `config_path` | Input | set/get_uci_option_value |
| `copy_config_path` | bool | `copy_config_path` | `copy_config_path` | Checkbox | set/get_uci_option_value |
| `local_conf` | string | `local_conf` | `local_conf` | Input | set/get_uci_option_value |

---

## 七、实施顺序（分阶段）

1. **阶段 1 — 后端骨架**：sub_vpn.h 宏/结构体 + sub_vpn.c 全套函数 + cmd.c 注册 + sub_login.c 菜单；先用 curl 手动 POST 验证 UCI 写入正确
2. **阶段 2 — 前端联调**：zerotier.js + api/settings.js + index.js + locales；GET 回显、POST 保存
3. **阶段 3 — 防火墙/NAT**：set_zerotier_firewall + 分发 NAT 脚本 + firewall include；验证 nat=1 客户端可访问局域网（fw3/fw4 双后端）
4. **阶段 4 — 构建验收**：target.config 启用 zerotier，全量编译刷机，跑验收清单

---

## 八、验收标准

- [ ] 网页保存后 `/etc/config/zerotier` 各字段正确（enabled/join/nat/port/secret/config_path/copy_config_path/local_conf）
- [ ] `join` 多网络 ID 生成 `networks.d/*.conf`，服务加入多个网络
- [ ] `nat=1` 客户端可访问局域网（MASQUERADE 生效），fw3/fw4 均验证
- [ ] firewall 有 `Allow-ZeroTier` rule（UDP dest_port）与 `zerotier_nat` include 段
- [ ] 关闭（enabled=0）服务停止、NAT 规则清除
- [ ] secret 留空时首次启动自动生成并回写 UCI
- [ ] 不编译/不依赖 luci-app-zerotier

---

## 九、风险与注意点

| 风险 | 说明 |
|---|---|
| `enabled` 语义 | zerotier 直接用 `enabled`，**勿照抄** wireguard 的 `disabled` 反义逻辑 |
| JSON 函数适配 | 用 X75 的 `convert_string_to_json`/`get_strval_from_json`/`web_back`，**勿照搬** r47602 的 `json_tokener_parse`/`resposeweb`（X75 无） |
| join 数组处理 | 标量用 X75 辅助函数，数组仍需 json-c 原生 API（X75 无 `get_array_from_json`） |
| join list 残留 | 写前 `uci_del_section_option(..., "join")` 清空整个 list |
| NAT 执行时机 | `firewall restart` 时 zt 网卡可能未生成，`zerotier-nat` 保留「等网卡」循环 |
| secret 敏感 | 前端 Password 控件；留空则后端删 option，交由 zerotier 包自动生成 |
| `strtok_r` 非 `strtok` | 用线程安全版，避免 goahead 多请求并发串扰 |
| 菜单 key 一致 | `sub_login.c` 的 `moduleKey: 'zerotier'` ↔ `vpn/index.js` 的 `case 'zerotier'` |
