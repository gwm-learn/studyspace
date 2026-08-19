# ZeroTier 支持开发计划（goahead + react-web，替代 luci-app-zerotier）

> 参考模板：MTK7981 `r47602` 提交（wireguard 支持）
> 后端：`broadlink/package/goahead`（C，仿 r47602 模式）
> 前端：`broadlink/package/react-web`（React + antd）
> NAT 动态网卡处理：仿 luci-app-zerotier

---

## 一、目标与范围

在 X75 项目新增 ZeroTier VPN 支持：通过 react-web 网页配置，goahead 后端写 zerotier UCI 与防火墙配置，**完全替代 luci-app-zerotier**（不依赖、不编译它）。

| 项 | 说明 |
|---|---|
| 后端 | `broadlink/package/goahead`（C，仿 r47602 wireguard 模式） |
| 前端 | `broadlink/package/react-web`（React + antd） |
| 底层服务 | 复用 OpenWrt `zerotier` 主包（`/etc/init.d/zerotier`，procd），**不改** |
| NAT 脚本 | 仿 luci-app-zerotier 的 `luci-zerotier` + `firewall.include`，随 goahead 包分发 |
| 界面范围 | **完整**：基础（enabled / join / nat）+ 高级（port / secret / config_path / copy_config_path / local_conf） |

---

## 二、总体架构

```
react-web (views/settings/vpn/component/zerotier.js)
        │ GET /acfun/getZerotierInfo  POST /acfun/zerotierSettings
        ▼
goahead (cmd.c 分发 → sub_vpn.c)
        ├─ get_zerotier_info / zerotier_info_response   (读 UCI → JSON)
        ├─ get_zerotier_from_json / set_zerotier         (JSON → 写 UCI)
        └─ ZerotierSet_exec:
             ├─ 写 /etc/config/zerotier (sample_config section)
             ├─ 写 /etc/config/firewall (rule + include 段)
             ├─ system("/etc/init.d/zerotier restart")    # 主服务(zerotier 包)
             └─ system("/etc/init.d/firewall restart")    # 触发 NAT 脚本
                    │
                    ▼
             /usr/share/zerotier/firewall.include
                    │ (firewall reload 时执行)
                    ▼
             /etc/init.d/zerotier-nat  (仿 luci-zerotier：等 zt 网卡 + iptables/nft)
```

---

## 三、参考模板对照

| 维度 | r47602 wireguard（参考） | 本次 zerotier（落地） |
|---|---|---|
| 后端函数模式 | `get_xxx_info` / `xxx_info_response` / `get_xxx_from_json` / `set_xxx` / `XxxGet_back` / `XxxSet_parse` / `XxxSet_exec` | 完全照搬 |
| 全局结构体 | `g_wireguard_server/client` | `g_zerotier`（单个，非 server/client 分离） |
| UCI 目标文件 | `/etc/config/network` | `/etc/config/zerotier`（named section `sample_config`） |
| 启停命令 | `ifup/ifdown` | `/etc/init.d/zerotier restart` |
| 防火墙 | 写 zone + rule | 写 rule（放行 UDP）+ include 段（NAT） |
| NAT 动态网卡 | 无（接口名固定） | **仿 luci-app-zerotier**（动态等 zt* 网卡） |

---

## 四、后端改动清单（goahead）

### 4.1 `src/src/platform/wrt/sub_vpn.h`

新增（仿 r47602 第 604-607、617-649 行）：

```c
#define ZEROTIER_INTERFACE  "sample_config"        /* /etc/config/zerotier 的 named section */
#define UCI_CONFIG_ZEROTIER "/etc/config/zerotier" /* 若 uci_utility.h 无此宏则新增 */

typedef struct {
    char enabled[4];
    char join[256];            /* 空格/逗号分隔的多网络 ID（对应 UCI list） */
    char nat[4];
    char port[16];
    char secret[128];
    char config_path[64];
    char copy_config_path[4];
    char local_conf[64];
} zerotier_t;

char* ZerotierGet_parse(void* data, int* errcode);
void  ZerotierGet_back(Webs* wp, void* data, int* errcode);
char* ZerotierSet_parse(void* data, int* errcode);
char* ZerotierSet_exec(void* data, int* errcode);
```

> 注：`join` 用字符串而非链表（参考 r47602 wireguard client 的 `allowed_ips` 处理），避免依赖链表工具函数。

### 4.2 `src/src/platform/wrt/sub_vpn.c`

新增以下函数（严格仿 r47602 结构）：

| 函数 | 职责 |
|---|---|
| `get_zerotier_info(zerotier_t*)` | 用 `get_uci_option_value(UCI_CONFIG_ZEROTIER, "sample_config", ...)` 逐项读 `enabled/nat/port/secret/config_path/copy_config_path/local_conf`；`join` 用 `uci_get_option_str("zerotier.sample_config.join", ...)`（list 自动空格拼接） |
| `zerotier_info_response(zerotier_t*)` | 结构体 → JSON；`join` 用 `str_split_iter` 拆成 JSON 数组（仿 wireguard client 的 allowed_ips） |
| `get_zerotier_from_json(json_object*)` | JSON → 结构体；`join` 数组用空格拼接回字符串（仿 r47602 第 415-430 行） |
| `set_zerotier()` | 写 UCI：先 `uci_del_section_option` 重建 `sample_config` 各 option；`join` 全量删除后逐个 `uci_addlist_config_value_str`；`enabled` 直接用 `enabled` 字段（与 wireguard 的 disabled 反义不同） |
| `ZerotierGet_parse/back` | GET：读 + 返回 JSON |
| `ZerotierSet_parse` | POST：解析 JSON 填充 `g_zerotier` |
| `ZerotierSet_exec` | 写 UCI + 防火墙 + system 命令（见下） |

**ZerotierSet_exec 核心逻辑**：

```c
set_zerotier();                                   // ① 写 /etc/config/zerotier
set_zerotier_firewall();                          // ② 写 /etc/config/firewall

if (enabled == 1)
    system("/etc/init.d/zerotier restart");       // ③ 主服务按新配置重启
else
    system("/etc/init.d/zerotier stop");

system("/etc/init.d/firewall restart");           // ④ 重载防火墙 → 触发 NAT 脚本
```

### 4.3 `src/src/cmd.c`

在 VPN section（约第 396-440 行）新增 2 条注册（仿 r47602 用 `NO_ACTION`，exec 内自调 system）：

```c
{"getZerotierInfo", ZerotierGet_parse, NULL, ZerotierGet_back, NULL, NO_ACTION},
{"zerotierSettings", ZerotierSet_parse, ZerotierSet_exec, NULL, NULL, NO_ACTION},
```

> 不需新增 ACTION 枚举（r47602 即此做法，exec 内直接 system，因需按 enabled 状态分流）。

### 4.4 `src/src/platform/wrt/sub_login.c`

在两处菜单数组新增 ZeroTier 项（仿 r47602 第 888 行 `{"vpn", "WireGuard", "wireGuard", true}`）：

1. 主菜单子项（约 830-862 行）：
   ```c
   main: 'vpn', moduleName: 'ZeroTier', moduleKey: 'zerotier', auth: false,
   ```
2. `glb_vpn_web_showctrl_arr[]`（约 867-884 行）：
   ```c
   {"vpn", "ZeroTier", "zerotier", true},
   ```

### 4.5 `src/src/utility.c/h`（按需）

若 X75 无 `str_split_iter`（r47602 在 utility.c 第 1392 行新增），则照搬新增；`join` 用字符串拼接无需链表 foreach 函数。

---

## 五、前端改动清单（react-web）

### 5.1 新增 `views/settings/vpn/component/zerotier.js`

单表单组件（仿 r47602 的 `wireGuardClient.js` 风格，无 server/client 分离）：

| 字段 | 控件 | 校验 | 对应 UCI |
|---|---|---|---|
| enabled | Switch | — | `enabled` |
| join | Form.List（多网络 ID） | 必填、16 位 hex | `list join` |
| nat | Checkbox | — | `nat` |
| port | Input（高级 tab） | 1-65535 | `port` |
| secret | Input.Password（高级，留空自动生成） | 可选 | `secret` |
| config_path | Input（高级） | 可选 | `config_path` |
| copy_config_path | Checkbox（高级） | — | `copy_config_path` |
| local_conf | Input（高级） | 可选 | `local_conf` |

结构：`Tabs` 分「General / Advanced」两个 tab（仿 zerotier settings.lua 的 `t:tab("main"/"more")` 分层）。

### 5.2 `api/settings.js`

```js
export const getZerotierInfo = async () => await request.get('/acfun/getZerotierInfo');
export const zerotierSettings = async (data) => await request.post('/acfun/zerotierSettings', data);
```

### 5.3 `views/settings/vpn/index.js`

```js
import ZeroTier from './component/zerotier';
// showComponent 增加
case 'zerotier': return <ZeroTier />
```

### 5.4 国际化 `public/locales/{cn,en,tw}/`

新增 ZeroTier 相关词条（enabled、Network ID、Auto NAT Clients、Port、Secret 等）。

---

## 六、NAT 脚本设计（仿 luci-app-zerotier，随 goahead 分发）

> 用户确认：zt 接口名/网段动态，仿 luci-app-zerotier 处理。goahead 分发脚本，exec 时经 firewall include 触发。

### 6.1 分发文件（放 goahead 包的 files 目录，随 goahead 安装）

| 安装路径 | 内容来源 | 职责 |
|---|---|---|
| `/etc/init.d/zerotier-nat` | 仿 `luci-app-zerotier/root/etc/init.d/luci-zerotier` | 等 zt 网卡 + fw3/fw4 插删 FORWARD/MASQUERADE 规则 |
| `/usr/share/zerotier/firewall.include` | 仿 `luci-app-zerotier/root/usr/share/zerotier/firewall.include` | firewall reload 时调用 zerotier-nat |

`zerotier-nat` 脚本关键逻辑（照搬 luci-zerotier 第 17-97 行）：
- `config_load zerotier` 读 `enabled` / `nat` 两个开关
- `nat=0` 直接退出
- 循环等 `ifconfig | grep zt` 网卡出现
- 探测 `nft` → fw4，否则 fw3
- fw3：`iptables -I FORWARD -i/-o $i ACCEPT` + `POSTROUTING MASQUERADE`（含网段）
- fw4：`nft insert rule inet fw4 forward/srcnat ...`
- `stop()` 反向删规则

### 6.2 goahead 写 firewall UCI（`set_zerotier_firewall`）

```c
// ① 放行入站 UDP 9993（让外部 peer 连接）
set_uci_option_value(UCI_CONFIG_FIREWALL, "zerotier", NULL, "rule");
set_uci_option_value(UCI_CONFIG_FIREWALL, "zerotier", "name", "Allow-ZeroTier");
set_uci_option_value(UCI_CONFIG_FIREWALL, "zerotier", "src", "wan");
set_uci_option_value(UCI_CONFIG_FIREWALL, "zerotier", "dest_port", port);   // 默认 9993
set_uci_option_value(UCI_CONFIG_FIREWALL, "zerotier", "proto", "udp");
set_uci_option_value(UCI_CONFIG_FIREWALL, "zerotier", "target", "ACCEPT");

// ② NAT include 段（替代 luci-app-zerotier 的 uci-defaults 注册）
set_uci_option_value(UCI_CONFIG_FIREWALL, "zerotier_nat", NULL, "include");
set_uci_option_value(UCI_CONFIG_FIREWALL, "zerotier_nat", "type", "script");
set_uci_option_value(UCI_CONFIG_FIREWALL, "zerotier_nat", "path", "/usr/share/zerotier/firewall.include");
set_uci_option_value(UCI_CONFIG_FIREWALL, "zerotier_nat", "reload", "1");
```

---

## 七、构建与包配置

| 文件 | 改动 |
|---|---|
| `broadlink/target/*/target.config` | 启用 `CONFIG_PACKAGE_zerotier=y`（及其依赖 `kmod-tun`、`libminiupnpc`、`libnatpmp` 等由 zerotier 包声明自动带入） |
| goahead 包 `Makefile` | 若新增 `sub_vpn.c` 内函数无需改；**需将 `files/`（NAT 脚本）加入 goahead 的安装**（`$(CP) ./files/* $(1)/` 或新增 `Package/.../install`） |
| 确认 | X75 feeds 已含 zerotier 主包（`owrt-feeds/packages/net/zerotier`，先前已确认存在），**无需新增** |

---

## 八、UCI 字段映射总表

| UCI option（`/etc/config/zerotier`） | 类型 | 前端控件 | goahead 读写方式 |
|---|---|---|---|
| `enabled` | bool | Switch | `set/get_uci_option_value`（**注意 zerotier 直接用 enabled，非 wireguard 的 disabled 反义**） |
| `join` | list | Form.List | `uci_addlist` 全量重建 / `uci_get_option_str` 读 |
| `nat` | bool | Checkbox | `set/get_uci_option_value` |
| `port` | int | Input | `set/get_uci_option_value`，默认 9993 |
| `secret` | string | Input.Password(留空自动生成) | `set/get_uci_option_value` |
| `config_path` | string | Input | `set/get_uci_option_value` |
| `copy_config_path` | bool | Checkbox | `set/get_uci_option_value` |
| `local_conf` | string | Input | `set/get_uci_option_value` |

---

## 九、实施顺序（分阶段）

1. **阶段 1 — 后端 UCI 读写**：`sub_vpn.h` + `sub_vpn.c` 的 get/set/parse/exec + `cmd.c` 注册 + `sub_login.c` 菜单（先用 `system("zerotier restart")` 验证 UCI 写入）
2. **阶段 2 — 前端**：`zerotier.js` + `api/settings.js` + `vpn/index.js` + locales，联调 GET/POST
3. **阶段 3 — 防火墙/NAT**：`set_zerotier_firewall` + NAT 脚本分发 + goahead Makefile，验证 NAT 生效
4. **阶段 4 — 构建与验收**：target.config 启用 zerotier，全量编译刷机验证

---

## 十、验收标准

- [ ] 网页可配置 enabled/join/nat 及高级项，保存后 `/etc/config/zerotier` 字段正确
- [ ] `join` 多网络 ID 能正确生成 `networks.d/*.conf`（服务能加入多个网络）
- [ ] `nat=1` 时 zerotier 客户端能访问局域网（MASQUERADE 生效），fw3/fw4 双后端均验证
- [ ] 放行 UDP 9993 的 rule 存在，外部 peer 可握手
- [ ] 关闭（enabled=0）时服务停止、NAT 规则清除
- [ ] 不依赖/不编译 luci-app-zerotier
- [ ] secret 留空时首次启动自动生成并回写 UCI（复用 zerotier 包 init 逻辑）

---

## 十一、风险与注意点

| 风险 | 说明 |
|---|---|
| `enabled` vs `disabled` 语义 | wireguard 用 `disabled` 反义，zerotier **直接用 `enabled`**，移植时勿照抄 wireguard 的 disabled 逻辑 |
| `join` list 全量重建 | 写前需先清空旧 list（`uci_del_section_option` 或逐个 dellist），避免残留 |
| NAT 脚本执行时机 | `firewall restart` 触发 include 时 zt 网卡可能未生成，脚本内需保留「等网卡」循环 |
| secret 敏感信息 | 前端展示用 Password 控件，留空走 zerotier 包自动生成 |
| cmd.c 菜单 key 一致性 | `sub_login.c` 的 `moduleKey: 'zerotier'` 必须与 `vpn/index.js` 的 `case 'zerotier'` 一致 |
| 工具函数依赖 | 确认 X75 `utility.c` 是否已有 `str_split_iter`，无则按 r47602 补 |
