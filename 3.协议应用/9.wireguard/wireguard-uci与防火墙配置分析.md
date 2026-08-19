# WireGuard VPN 支持 - UCI 与防火墙配置分析（r47602）

> 分析对象：`/home/gwm/code/MTK7981`（SVN，`mtk7981-241206` 分支）
> 提交：`r47602`（gaoweiming，2026-08-18），提交信息 `[wireGuard] add vpn support of wireGuard`
> 核心文件：`broadlink/package/goahead/src/src/platform/wrt/sub_vpn.c`（+578 行）

---

## 一、提交概览

r47602 为 Broadlink 自研 Web 管理界面（goahead）增加 WireGuard VPN 支持，共改动 13 个文件：

| 文件 | 类型 | 作用 |
|---|---|---|
| `sub_vpn.c` | 修改 | **核心实现**：server/client 的 UCI 读写 + 防火墙配置 |
| `sub_vpn.h` | 修改 | 宏定义、结构体、函数声明 |
| `sub_login.c` | 修改 | VPN 菜单增加 "WireGuard" 项 |
| `cmd.c` | 修改 | 注册 4 个 HTTP API 命令 |
| `utility.c` / `utility.h` | 修改 | 链表遍历 + 字符串分割工具函数 |
| `web/src/api/settings.js` | 修改 | 前端 4 个 API 封装 |
| `web/src/config/index.js` | 修改 | 菜单模块配置 |
| `wireGuardIndex.js` / `wireGuardServer.js` / `wireGuardClient.js` | 新增 | 前端页面（Tabs + Server/Client 表单） |
| `views/settings/vpn/index.js` | 修改 | 路由注册 |
| `target/.../target.config` | 修改 | 启用 wireguard 相关包 |

---

## 二、总体架构与底层依赖

### 2.1 关键结论

WireGuard 的**底层实现完全复用 OpenWrt 原生 wireguard 栈**，Broadlink 只是在自己的 goahead Web 管理界面里加了一层封装——通过操作 `/etc/config/network` 和 `/etc/config/firewall` 来配置 wireguard，接口的实际拉起由 OpenWrt 原生 `proto wireguard`（netifd 的 `wireguard.sh`）完成。

### 2.2 target.config 启用的包

```sh
CONFIG_PACKAGE_kmod-wireguard=y          # 内核模块（WireGuard 核心）
CONFIG_PACKAGE_luci-app-wireguard=y      # OpenWrt 官方 LuCI 界面
CONFIG_PACKAGE_luci-proto-wireguard=y    # LuCI 的 wireguard 协议支持
CONFIG_PACKAGE_wireguard-tools=y         # wg / wg-quick 工具
CONFIG_PACKAGE_iperf=y / iperf3=y        # 附带启用（推测用于测吞吐）
```

### 2.3 关键常量（sub_vpn.h）

```c
#define WIREGUARD_CLIENT_INTERFACE  "wg_client"            // client 接口名
#define NETWORK_WIREGUARD_CLIENT    "wireguard_wg_client"  // client peer section 名
#define WIREGUARD_SERVER_INTERFACE  "wg_server"            // server 接口名
#define NETWORK_WIREGUARD_SERVER    "wireguard_wg_server"  // server peer section 前缀
```

### 2.4 目标配置文件

| 宏 | 值 | 说明 |
|---|---|---|
| `UCI_CONFIG_NETWORK` | `/etc/config/network` | 接口 + peer 定义 |
| `UCI_CONFIG_FIREWALL` | `/etc/config/firewall` | zone + rule |

### 2.5 UCI 辅助函数语义（uci_utility.c）

分析中用到这些函数，其行为需明确：

| 函数 | 行为 |
|---|---|
| `set_uci_option_value(pkg, sec, opt, val)` | `uci_set`（sec/opt 不存在时自动创建），**option=NULL 时设置 section 的 type**，然后 `uci_commit` |
| `get_uci_option_value(cfg, sec, opt, out, sz)` | 按 section **名字**精确匹配读取 option |
| `uci_del_section_option(cfg, sec, opt)` | opt 非 NULL 删 option，**opt=NULL 删整个 section**，commit |
| `uci_addlist_config_value_str(pkg, sec, opt, val)` | `uci_add_list` 向 list option 追加值 |
| `uci_dellist_config_value_str(pkg, sec, opt, val)` | `uci_del_list` 从 list option 删除值 |
| `is_uci_option_exist(pkg, sec, opt)` | opt=NULL 时判断 **section 是否存在** |
| `uci_get_option_str(name, out, sz)` | name 为完整路径（如 `network.wireguard_wg_client.allowed_ips`），list 类型用空格拼接 |

---

## 三、UCI 配置详细分析（`/etc/config/network`）

### 3.1 Server 端（`set_wireguard_server`）

生成配置：

```sh
config interface 'wg_server'
    option proto 'wireguard'
    option private_key '<server私钥>'
    option listen_port '51820'
    option addresses '10.0.0.1/24'
    option mtu '1420'
    option disabled '0'          # enabled 的反义

config wireguard_wg_server 'wireguard_wg_server0'   # peer #0
    option description 'wg_client0'
    option public_key '<client公钥>'
    option preshared_key '<预共享密钥>'
    option allowed_ips '10.0.0.2/32'
    option route_allowed_ips '0'

config wireguard_wg_server 'wireguard_wg_server1'   # peer #1
    ...
```

**逐项说明：**

| Option | 类型 | 默认 | 语义 | 代码位置 |
|---|---|---|---|---|
| `proto` | string | `wireguard` | 协议类型，触发 netifd wireguard.sh | 第 1586 行（`set_uci_option_value(..., "proto", "wireguard")`） |
| `private_key` | string | — | 服务端私钥（前端 `Input.Password` 隐藏） | 第 1587 行 |
| `listen_port` | int | — | 监听端口（前端校验 1-65535） | 第 1588 行 |
| `addresses` | string | — | 服务端接口地址（CIDR） | 第 1589 行 |
| `mtu` | int | — | MTU（前端校验 0-1420） | 第 1590 行 |
| `disabled` | bool | `1` | `enabled` 的反义表达 | 第 1585 行 |

**peer section（`wireguard_wg_serverN`）**：section 名 = `NETWORK_WIREGUARD_SERVER + index`（`wireguard_wg_server0/1/...`），type 也是 `wireguard_wg_server`。每项：

| Option | 说明 |
|---|---|
| `description` | `wg_client0`（自动编号，仅标识用途） |
| `public_key` | 客户端公钥 |
| `preshared_key` | 预共享密钥（可空） |
| `allowed_ips` | 该客户端的 IP/CIDR |
| `route_allowed_ips` | 是否自动建路由（`1`/`0`） |

**写入逻辑**：先循环 `while(is_uci_option_exist(..., "wireguard_wg_serverN", NULL))` 删除所有旧 peer section，再通过 `foreach_in_comm_list_ex` 遍历链表逐个重建（`set_wiregurad_client_peer`）。**每次保存是全量重建**（先删后建）。

### 3.2 Client 端（`set_wireguard_client`）

生成配置：

```sh
config interface 'wg_client'
    option proto 'wireguard'
    option private_key '<client私钥>'
    option addresses '10.0.0.2/24'
    option disabled '0'
    option peerdns '1'            # 是否用对端 DNS
    option dns '8.8.8.8'          # peerdns=1 时写入
    option mtu '1420'             # 默认 1420

config wireguard_wg_client 'wireguard_wg_client'   # 单个 peer（连一个 server）
    option description 'wg_server'
    list   allowed_ips '0.0.0.0/0'    # 注意：list 类型
    option route_allowed_ips '1'
    option public_key '<server公钥>'
    option preshared_key '<预共享密钥>'
    option endpoint_host 'server.example.com'
    option endpoint_port '51820'
    option persistent_keepalive '25'   # 默认 25 秒
```

**逐项说明：**

| Option | 类型 | 默认 | 语义 |
|---|---|---|---|
| `proto` | string | `wireguard` | 协议类型 |
| `private_key` | string | — | 客户端私钥 |
| `addresses` | string | — | 客户端接口地址 |
| `peerdns` | bool | `0` | 是否使用对端下发的 DNS |
| `dns` | string | — | `peerdns=1` 时写入，否则置空 |
| `mtu` | int | `1420` | 空则默认 1420（代码 `strlen ? mtu : "1420"`） |

**peer section（`wireguard_wg_client`）**：section 名与 type 相同（单个 peer，连一个 server）。重点差异：

| Option | 说明 |
|---|---|
| `allowed_ips` | **list 类型**——前端是数组，后端用 `str_split_iter` 按空格/逗号/制表符分割后逐个 `uci_addlist` |
| `endpoint_host` | 对端服务器公网 IP/域名 |
| `endpoint_port` | 对端端口（1-65535） |
| `persistent_keepalive` | 保活间隔，空则默认 `25` 秒 |
| `route_allowed_ips` | 是否自动建路由 |

### 3.3 底层 `proto wireguard` 约定（OpenWrt 原生）

代码中的 section 命名严格遵循 OpenWrt netifd 的 wireguard 协议约定：

- `config interface 'wg_server'` → 接口定义
- `config wireguard_wg_server` → peer section，**type 必须是 `wireguard_<接口名>`**

因此 `NETWORK_WIREGUARD_SERVER = "wireguard_wg_server"` 正是"接口名 `wg_server` + 前缀 `wireguard_`"。实际的 WireGuard 接口配置（生成 `wg0`、密钥、endpoint 等）由 `lib/netifd/proto/wireguard.sh` 读取这些 section 完成，goahead 代码**不直接调用 `wg` 命令**，只负责写 UCI 和 `ifup`/`ifdown`。

---

## 四、防火墙配置详细分析（`/etc/config/firewall`）

### 4.1 zone 索引约定

代码用匿名 section 索引 `@zone[0]` / `@zone[1]` 定位 zone。经核实：

| 索引 | zone | 证据 |
|---|---|---|
| `@zone[0]` | **lan** | OpenWrt 默认 firewall 第一个 zone；wireguard server 接口归入此 zone 使其可访问局域网 |
| `@zone[1]` | **wan** | `sub_wan.c:389` / `sub_vpn.c:45` 均用 `uci_get_option_str("firewall.@zone[1].network", wan_iface, ...)` 读取 wan 接口 |

### 4.2 Server 端防火墙（`set_wireguard_server`）

```c
// ① 将 wg_server 接口加入 lan zone（@zone[0]）
uci_dellist_config_value_str(UCI_CONFIG_FIREWALL, "@zone[0]", "network", WIREGUARD_SERVER_INTERFACE);
uci_addlist_config_value_str(UCI_CONFIG_FIREWALL, "@zone[0]", "network", WIREGUARD_SERVER_INTERFACE);

// ② 添加放行规则
set_uci_option_value(UCI_CONFIG_FIREWALL, "wg", NULL, "rule");      // type=rule 的 section "wg"
set_uci_option_value(UCI_CONFIG_FIREWALL, "wg", "name", "Allow-WireGuard");
set_uci_option_value(UCI_CONFIG_FIREWALL, "wg", "src", "wan");
set_uci_option_value(UCI_CONFIG_FIREWALL, "wg", "dest_port", listen_port);
set_uci_option_value(UCI_CONFIG_FIREWALL, "wg", "proto", "udp");
set_uci_option_value(UCI_CONFIG_FIREWALL, "wg", "target", "ACCEPT");
```

生成 `/etc/config/firewall`：

```sh
config zone          # @zone[0] = lan
    ...
    list network 'wg_server'     # 新增

config rule 'wg'
    option name 'Allow-WireGuard'
    option src 'wan'
    option dest_port '51820'
    option proto 'udp'
    option target 'ACCEPT'
```

**语义**：
- ① 把 `wg_server` 归入 **lan zone** → wireguard 客户端与局域网同属一个 zone，可互通（Forward 默认放行）
- ② 放行 **从 WAN 进来的 UDP 流量到 `listen_port`** → 允许外部客户端握手连接

**注意**：`wg` 是**具名 section**（不是匿名 `config rule`），所以每次保存都是固定覆盖同名的 `wg` section，不会重复累积。

### 4.3 Client 端防火墙（`set_wireguard_client`）

```c
// 仅将 wg_client 接口加入 wan zone（@zone[1]）
uci_dellist_config_value_str(UCI_CONFIG_FIREWALL, "@zone[1]", "network", WIREGUARD_CLIENT_INTERFACE);
uci_addlist_config_value_str(UCI_CONFIG_FIREWALL, "@zone[1]", "network", WIREGUARD_CLIENT_INTERFACE);
```

生成 `/etc/config/firewall`：

```sh
config zone          # @zone[1] = wan
    ...
    list network 'wg_client'     # 新增
```

**语义**：把 `wg_client` 归入 **wan zone** → 让客户端流量走 wan 的 NAT/MASQUERADE 出口。**不添加任何放行规则**（因为 client 是主动出站连接，无需入站放行）。

### 4.4 server vs client 防火墙差异原因

| 维度 | Server | Client |
|---|---|---|
| 加入 zone | lan（`@zone[0]`） | wan（`@zone[1]`） |
| 放行规则 | 有（Allow-WireGuard，UDP dest_port） | 无 |
| 原因 | 需从 WAN 收连接 + 让客户端访问局域网 | 主动出站，走 wan NAT |

---

## 五、数据流与调用时序

### 5.1 API 注册与调用链

`cmd.c` 注册 4 个 API，前端经 `/acfun/` 前缀访问：

| API | 方法 | 后端函数 |
|---|---|---|
| `getWireguardServerInfo` | GET | `WireguardServerGet_parse` → `WireguardServerGet_back` |
| `wireguardServerSettings` | POST | `WireguardServerSet_parse` → `WireguardServerSet_exec` |
| `getWireguardClientInfo` | GET | `WireguardClientGet_parse` → `WireguardClientGet_back` |
| `wireguardClientSettings` | POST | `WireguardClientSet_parse` → `WireguardClientSet_exec` |

### 5.2 完整时序

```
前端表单 (wireGuardServer.js / wireGuardClient.js)
        │ POST /acfun/wireguardServerSettings (JSON)
        ▼
cmd.c 分发 → WireguardServerSet_parse     # 解析 JSON → 填充全局结构体
        ▼
WireguardServerSet_exec
        ├─ set_wireguard_server()          # 写 /etc/config/network + firewall
        │    ├─ 写 interface 'wg_server' 各 option
        │    ├─ 删旧 peer → 重建 wireguard_wg_serverN section
        │    ├─ firewall: lan zone 加 wg_server + rule "wg"
        │    └─ (以上均 uci_commit 立即落盘)
        ├─ system("/etc/init.d/firewall restart")   # 重载防火墙（server 独有）
        └─ system("ifup wg_server") 或 "ifdown wg_server"
             └─ netifd → proto wireguard.sh → 拉起 wg 接口
```

**Client 端时序差异**：`WireguardClientSet_exec` **不执行 `firewall restart`**，只有 `ifup wg_client` / `ifdown wg_client`。

---

## 六、关键差异与易错点总结

| 点 | 说明 |
|---|---|
| 底层复用 OpenWrt 原生栈 | 只写 UCI + `ifup`/`ifdown`，不直接调 `wg`；实际接口由 netifd `wireguard.sh` 拉起 |
| peer section 命名约定 | type 必须是 `wireguard_<接口名>`；server 用 `wireguard_wg_serverN` 多 peer，client 用单个 `wireguard_wg_client` |
| `allowed_ips` 类型不一致 | server 端是**单值 option**，client 端是 **list option**（数组分割后逐个 addlist） |
| `enabled` 用 `disabled` 反义表达 | `enabled=1 → disabled=0`，`enabled=0 → disabled=1` |
| 关闭时防火墙配置残留 | `enabled=0` 分支**只 set `disabled=1` 后 return**，不清除已写入的 zone 成员和 `wg` rule（接口 down 后无流量，但配置仍在） |
| firewall restart 不对称 | server 每次保存都 `firewall restart`；client 从不 restart（改动 zone 需重启防火墙才真正生效，client 的 zone 改动要等下次防火墙重载） |
| 每次保存全量重建 | server 的 peer 先全部删除再重建；接口 option 则先 `uci_del_section_option` 删 section 再重写 |
| 端口校验范围 | 前端 1-65535，MTU 0-1420，保活 ≥0 秒（默认 25） |
| `wg` rule 具名覆盖 | 具名 section `wg` 每次固定覆盖，不会像匿名 section 那样累积重复 |
| 附带 iperf | target.config 顺带启用 `iperf`/`iperf3`，推测用于 wireguard 隧道吞吐测试 |

---

## 附：与 zerotier 的架构对照

| 维度 | WireGuard（本项目 r47602） | ZeroTier（luci-app-zerotier） |
|---|---|---|
| 配置载体 | `/etc/config/network`（interface + peer） | `/etc/config/zerotier`（独立 UCI） |
| 底层实现 | OpenWrt 原生 `proto wireguard` | 独立 `zerotier-one` 守护进程 |
| 配置写入方 | goahead Web（sub_vpn.c） | LuCI（settings.lua CBI） |
| 防火墙处理 | 直接改 `/etc/config/firewall` 的 zone + rule | 动态插 iptables/nft 规则（不落 UCI rule） |
| 服务启停 | `ifup`/`ifdown`（netifd） | `/etc/init.d/zerotier`（procd） |
