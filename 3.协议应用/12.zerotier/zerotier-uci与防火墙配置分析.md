# ZeroTier 程序 UCI + 防火墙 配置完整分析

> 分析对象：
> - zerotier 主包：`owrt_workspace/owrt/owrt-feeds/packages/net/zerotier`
> - 上游源码：`owrt_workspace/owrt/build_dir/target-aarch64_cortex-a53_musl/ZeroTierOne-1.10.3`
> - luci 界面包：`owrt_workspace/owrt/package/utils/luci-app-zerotier`

---

## 一、总体架构与文件分工

```
┌─────────────────────────────────────────────────────────────────┐
│  zerotier 主包 (owrt-feeds/packages/net/zerotier)               │
│    ├─ files/etc/config/zerotier   → 安装为 /etc/config/zerotier │
│    ├─ files/etc/init.d/zerotier   → 主服务 procd 脚本           │
│    └─ Makefile: conffiles 声明 /etc/config/zerotier             │
├─────────────────────────────────────────────────────────────────┤
│  luci-app-zerotier (package/utils/luci-app-zerotier)            │
│    ├─ settings.lua      → LuCI 界面，读写 /etc/config/zerotier  │
│    ├─ luci-zerotier     → NAT 防火墙辅助脚本（不是主服务）      │
│    ├─ uci-defaults      → 注册 firewall.include + ucitrack      │
│    └─ firewall.include  → firewall 重载时触发 NAT 规则          │
└─────────────────────────────────────────────────────────────────┘
```

**关键结论**：`/etc/config/zerotier` 由 **zerotier 主包** 提供；**防火墙完全由 luci-app-zerotier 管理**，zerotier 主包不碰防火墙。

---

## 二、UCI 配置 `/etc/config/zerotier` 详细分析

### 2.1 来源与安装

- **定义文件**：`owrt-feeds/packages/net/zerotier/files/etc/config/zerotier`
- **安装方式**：Makefile `define Package/zerotier/install` 中 `$(CP) ./files/* $(1)/`，整体拷贝到根文件系统
- **升级保留**：Makefile 第 64-66 行：

```makefile
define Package/zerotier/conffiles
/etc/config/zerotier
endef
```

`conffiles` 声明使该文件在 `sysupgrade` 时作为用户配置保留，不会被包升级覆盖。

### 2.2 默认配置内容

```sh
config zerotier sample_config
	option enabled 0
	#option config_path '/etc/zerotier'
	#option copy_config_path '1'
	#option port '9993'
	#option local_conf '/etc/zerotier.conf'
	option secret ''
	list join '8056c2e21c000001'
```

结构特点：**单个命名 section `sample_config`**，`zerotier` 是 section 类型，`sample_config` 是 section 名。这决定了 LuCI 侧 `Map("zerotier")` + `NamedSection("sample_config", "zerotier")` 的对应关系。

### 2.3 每个 option 逐项说明

| Option | 类型 | 默认 | 语义 | 消费位置（init.d/zerotier） |
|---|---|---|---|---|
| `enabled` | bool | `0` | 是否启动该实例 | `section_enabled()` 第 10-13 行；`start_instance()` 第 20-23 行，为 0 时 `return 1` 并打印 "disabled in config" |
| `config_path` | string | 空 | 持久化配置目录（ZT controller 模式） | 第 25、37-51 行：非空时把工作目录软链/复制到该路径 |
| `copy_config_path` | bool | `0` | 是否把 `config_path` 复制到 RAM（防写闪存） | 第 29、46-50 行：`1` 用 `cp -r`，`0` 用 `ln -s` |
| `port` | int | `9993` | 主端口（UDP/TCP 数据面 + 控制面 JSON API） | 第 59-61 行：拼 `-p${port}` 传给 zerotier-one |
| `secret` | string | 空 | identity.secret 内容（设备身份私钥） | 第 63-81 行：空则首次启动生成并回写 UCI |
| `local_conf` | string | 空 | local.conf 配置文件路径 | 第 83-85 行：存在则软链到工作目录 |
| `join` | list | `8056c2e21c000001` | 要加入的网络 ID（可多个） | 第 87-92 行：每个 ID 生成一个 `networks.d/<id>.conf` 空文件 |

### 2.4 各 option 的运行时机制（结合源码印证）

**① `port` → `-p` 参数**

`init.d/zerotier` 第 59-61 行拼参数，`one.cpp` 第 196-202 行解析：

```cpp
case 'p':
    port = Utils::strToUInt(argv[i] + 2);
    if ((port > 0xffff)||(port == 0)) { ... 报错 ... }
```

`one.cpp` 第 2126 行注释明确：`case 'p': // port -- for both UDP and TCP, packets and control plane`。即该端口同时承载数据面（UDP/TCP 打洞）和控制面（127.0.0.1 的 JSON API）。

**② `secret` → identity.secret 自动生成**

`init.d/zerotier` 第 63-81 行逻辑：

```sh
if [ -z "$secret" ]; then
    zerotier-idtool generate "$sf" > /dev/null   # 生成身份
    secret="$(cat $sf)"; rm "$sf"
    uci set zerotier.$cfg.secret="$secret"        # 回写 UCI
    uci commit zerotier
fi
echo "$secret" > $path/identity.secret            # 写入工作目录
rm -f $path/identity.public                        # 清掉旧公钥
```

对应 man page（`doc/zerotier-one.8.md` 第 20 行）：首次启动在全新工作目录会生成 identity，**identity.secret 决定设备的 10 位 ZeroTier 地址 + ECC-256 密钥对**，丢失即丢地址。init 脚本主动把它回写进 UCI，实现了"地址持久化在 UCI 配置里"。

**③ `join` → networks.d 空文件**

`init.d/zerotier` 第 87-92 行：

```sh
add_join() { touch $path/networks.d/$1.conf; }
config_list_foreach $cfg 'join' add_join
```

对应 man page 第 86 行：ZeroTier 启动时**扫描 `networks.d/` 下 `<network ID>.conf` 文件来恢复网络**；空文件则直接从 controller 拉取配置。所以 `touch` 一个空文件 = "预配置加入该网络"，无需走 API。

**④ `config_path` + `copy_config_path` → 软链/复制**

`init.d/zerotier` 第 31-57 行核心：

```sh
path=${CONFIG_PATH}_$cfg                    # /var/lib/zerotier-one_sample_config
rm -rf $path
if [ -n "$config_path" -a "$config_path" != "$path" ]; then
    [ "$copy_config_path" = "1" ] && cp -r $config_path $path || ln -s $config_path $path
fi
mkdir -p $path/networks.d
rm -f $CONFIG_PATH; ln -s $path $CONFIG_PATH   # 默认软链指向最新实例
```

默认工作目录是 `/var/lib/zerotier-one`（第 8 行），多实例时用 `_<section名>` 后缀区分。

### 2.5 主服务生命周期（procd）

`init.d/zerotier` 关键参数：

| 项 | 值 | 说明 |
|---|---|---|
| `START` | 90 | 启动优先级 |
| `USE_PROCD` | 1 | 用 procd 托管 |
| `PROG` | `/usr/bin/zerotier-one` | 主程序 |
| `respawn` | 是 | 崩溃自动重启 |
| `stderr 1` | 是 | 错误输出到日志 |

- **启动**：`start_service()` → `config_foreach start_instance 'zerotier'`（**支持多实例**，每个 `zerotier` section 一个 procd 实例）
- **重载触发**：`service_triggers()` → `procd_add_reload_trigger 'zerotier'`，即 `/etc/config/zerotier` 变更时 procd 自动 reload
- **reload**：`reload_service()` = `stop` + `start`

### 2.6 依赖包（影响运行环境）

Makefile 第 29 行：`DEPENDS:=+libpthread +libstdcpp +kmod-tun +ip +libminiupnpc +libnatpmp`

- `kmod-tun`：TUN 虚拟网卡内核模块（zt* 接口的基础）
- `ip`：init 脚本和 luci-zerotier 里 `ip route` 提取网段用
- `libminiupnpc` + `libnatpmp`：NAT 打洞（UPnP/NAT-PMP）

---

## 三、防火墙配置详细分析

### 3.1 架构总览

防火墙分三层，全部由 luci-app-zerotier 提供：

```
UCI 配置层  (firewall.zerotier include section)  ← uci-defaults 注册
   │
   ▼
触发层      (firewall.include 脚本)
   │
   ▼
规则层      (luci-zerotier init 脚本动态插 iptables/nft 规则)
```

### 3.2 UCI 层：`uci-defaults/luci-zerotier`

```sh
uci -q batch <<-EOF >/dev/null
	delete ucitrack.@zerotier[-1]
	add ucitrack zerotier
	set ucitrack.@zerotier[-1].init='luci-zerotier'
	commit ucitrack

	delete firewall.zerotier
	set firewall.zerotier=include
	set firewall.zerotier.type=script
	set firewall.zerotier.path='/usr/share/zerotier/firewall.include'
	set firewall.zerotier.reload=1
	commit firewall
EOF
```

产生两个 UCI 配置：

**① `/etc/config/firewall` 中的 include 段**

```sh
config include 'zerotier'
	option type 'script'
	option path '/usr/share/zerotier/firewall.include'
	option reload '1'
```

`type=script` 表示这是**自定义脚本型 include**（而非普通 rule/redirect），fw3/fw4 在构建/重载防火墙时会执行 `path` 指向的脚本。`reload=1` 表示防火墙重载时该脚本也要被重新执行。

**② `/etc/config/ucitrack` 中的联动段**

```sh
config zerotier
	option init 'luci-zerotier'
```

ucitrack 是 LuCI 的"服务联动"机制：当**防火墙**配置变化并 reload 时，自动触发 `luci-zerotier` 这个 init 脚本。源码注释特别注明"ucitrack 只对非 PROCD 程序有效"——因为主服务 `zerotier` 是 procd 的（用 `service_triggers` 自己处理 reload），而 `luci-zerotier` 是传统 rc.common 脚本，需要 ucitrack 手动联动。

### 3.3 触发层：`firewall.include`

```sh
#!/bin/sh
/etc/init.d/luci-zerotier enabled && /etc/init.d/luci-zerotier restart
exit 0
```

逻辑：防火墙 reload 时，若 `luci-zerotier` 已 `enabled`（即 `/etc/config/zerotier` 里 `enabled=1`），则 `restart` 它，重新应用 NAT 规则。

### 3.4 规则层：`init.d/luci-zerotier`（NAT 核心）

**注意**：这不是主服务，是 NAT 辅助脚本（`START=99`，晚于主服务 `START=90`）。`PROG=/etc/init.d/zerotier` 引用主服务用于状态检查。

#### start() 前置检查（三重门）

```sh
config_load zerotier
config_foreach get_config zerotier   # 读 enabled / nat 两个布尔值
[ $enabled -eq 0 ] && return 0       # ① 未启用 → 退出
$PROG running || return 1            # ② 主服务没跑 → 退出
[ $nat -eq 0 ] && return 0           # ③ nat 未开 → 退出
```

对应 LuCI `settings.lua` 里的 `enabled`（Enable）和 `nat`（Auto NAT Clients）两个开关。

#### 等待 zt 网卡

```sh
while [ "$(ifconfig | grep 'zt' | awk '{print $1}')" = "" ]; do sleep 1; done
```

阻塞等待 ZeroTier 创建 `zt*` TUN 接口后才继续。

#### 后端探测

```sh
[ -x "$(command -v nft)" ] && FW="fw4" || FW="fw3"
```

存在 `nft` 命令 → 用 fw4（nftables），否则 → fw3（iptables）。

#### fw3 分支（iptables 规则）

```sh
iptables -I FORWARD -i "$i" -j ACCEPT            # 允许从 zt 接口转发入
iptables -I FORWARD -o "$i" -j ACCEPT            # 允许向 zt 接口转发出
iptables -t nat -I POSTROUTING -o "$i" -j MASQUERADE   # 出 zt 方向做源 NAT
iptables -t nat -I POSTROUTING -s "${ip_segment}" -j MASQUERADE  # 按 ZT 网段做源 NAT
```

其中 `ip_segment` 由 `ip route | grep "dev $i proto kernel"` 提取——即该 zt 接口上由内核自动生成的路由网段（ZeroTier 分配的网络段）。

#### fw4 分支（nftables 规则）

```sh
nft insert rule inet fw4 forward iifname "$i" accept
nft insert rule inet fw4 forward oifname "$i" accept
nft insert rule inet fw4 srcnat oifname "$i" counter masquerade
nft insert rule inet fw4 srcnat ip saddr "${ip_segment}" counter masquerade
```

语义与 fw3 完全对应，只是表结构变为 OpenWrt 22.03+ 的 `inet fw4` 表（`forward` 链 + `srcnat` 链）。

#### 去重与 stop()

- **去重**：插入前 `grep -sq "$i"` 检查该接口规则是否已存在，避免重复插入
- **stop()**：反向删除——fw3 用 `-D`，fw4 用 `nft -a list` 拿 handle 再 `delete rule ... handle`，精准删除（第 79-96 行）

### 3.5 NAT 功能本质

"Auto NAT Clients" 开关的作用：**让 ZeroTier 网络里的其他客户端能通过本机访问本机的局域网**。MASQUERADE 规则使从 zt 接口出去/回来的流量被源地址转换，实现跨网段路由。

---

## 四、完整数据流与启动时序

```
LuCI 界面 (settings.lua Map "zerotier")
        │  读写 /etc/config/zerotier
        ▼
┌─ 主服务 /etc/init.d/zerotier (START=90, procd) ─────────────┐
│  config_foreach start_instance                                │
│    ├─ enabled 检查                                            │
│    ├─ config_path 软链/复制                                    │
│    ├─ secret 空 → zerotier-idtool generate → 回写 UCI           │
│    ├─ join → touch networks.d/<id>.conf                        │
│    └─ procd 启动: zerotier-one [-pPORT] /var/lib/zerotier-one_x│
└──────────────────────────────────────────────────────────────┘
        │ 创建 zt* TUN 接口 + 内核路由 (proto kernel)
        ▼
┌─ 辅助脚本 /etc/init.d/luci-zerotier (START=99) ─────────────┐
│  enabled=1 且 主服务 running 且 nat=1 → 等 zt 网卡             │
│  fw3: iptables FORWARD ACCEPT + POSTROUTING MASQUERADE       │
│  fw4: nft forward accept + srcnat masquerade                 │
└──────────────────────────────────────────────────────────────┘
        ▲
        │ firewall 重载时
        ▼
firewall.include → luci-zerotier enabled && restart
        ▲
        │ ucitrack 联动
        ▼
/etc/config/firewall reload
```

**时序要点**：
1. 主服务（START=90）先起，创建 zt 网卡
2. NAT 辅助脚本（START=99）后起，等网卡后插规则
3. 之后任何防火墙 reload（用户改防火墙 / ucitrack 触发）都会经 `firewall.include` 重新应用 NAT 规则
4. UCI `zerotier` 变更由 procd 的 `service_triggers` 自动 reload 主服务

---

## 五、关键差异与易错点总结

| 点 | 说明 |
|---|---|
| 两个 init 脚本分工 | `zerotier` = 主服务（procd，管本体）；`luci-zerotier` = NAT 规则（传统脚本） |
| `/etc/config/zerotier` 归属 | zerotier 主包提供，非 luci-app |
| `enabled` 双重含义 | 既是主服务开关（zerotier init），也是 NAT 前置开关（luci-zerotier init） |
| `nat` 才是 NAT 开关 | `enabled=1` + `nat=1` 才会加防火墙 NAT 规则 |
| 防火墙规则非 UCI 持久化 | 实际 iptables/nft 规则是运行时动态插入，UCI 里只有 include 段 |
| 双防火墙后端 | fw3(iptables) / fw4(nft) 自动探测，规则需两套 |
| `join` 默认加入 Earth | 默认 `8056c2e21c000001` 是公共网络，实际部署需删除/替换 |
| `secret` 回写 UCI | 首次启动生成的 identity 会 `uci commit` 持久化，地址不会漂移 |
