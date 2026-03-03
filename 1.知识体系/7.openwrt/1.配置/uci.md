# OpenWRT UCI 体系完整说明

UCI（Unified Configuration Interface，统一配置接口）是 OpenWRT 配置管理的核心，也是 CPE 开发、中高频考察的知识点。本文从「定义→核心结构→工作机制→实操命令→实战场景→考点」六个维度，体系化讲解 UCI 体系，兼顾原理与 CPE 实战应用，适配复习和需求。

# 一、UCI 体系核心定义

UCI 是 OpenWRT 为解决嵌入式系统配置碎片化问题，专门设计的 **统一配置管理框架**。

## 核心目标

将网络、WiFi、防火墙、系统等所有配置，统一到一套文本格式 + 标准操作接口中，替代传统 Linux 分散的配置文件（如 /etc/network/interfaces、/etc/dhcp/dhcpd.conf），实现配置的集中管理。

## 核心价值（必说）

> CPE 产品中，上层 Web（LuCI）、命令行、后台脚本、TR069 远程配置，都通过 UCI 接口读写配置，保证配置来源唯一、操作统一，避免配置冲突，降低开发和维护成本。
> 
> 

# 二、UCI 体系核心结构

UCI 的配置全部存储在 **/etc/config/** 目录下（核心目录，必记），每个文件对应一类功能（如 network 对应网络、wireless 对应 WiFi、firewall 对应防火墙），文件内部遵循固定的三层结构，易读、易修改。

## 1. 配置文件的三层结构（最核心，必须记熟）

以 /etc/config/network（CPE 网络核心配置文件）中的 WAN 配置为例，清晰理解三层结构：

```ini
# 示例：/etc/config/network 中的 WAN 配置
config interface 'wan'        # 1. config 段（配置类型 + 实例名）
    option ifname 'eth0'      # 2. option 键值对（具体配置项，单值）
    option proto 'pppoe'      # 键=proto（协议类型），值=pppoe（拨号方式）
    option username 'abc@163.com'
    option password '123456'
    list dns '8.8.8.8'        # 3. list 列表（具体配置项，多值）
    list dns '114.114.114.114'
```

### （1）config 段（配置实例定义）

- 格式：`config <配置类型> [<'实例名'>]`（实例名加单引号，可选但推荐）

- 作用：定义一个具体的配置实例，相当于“配置分组”，区分不同功能的配置。

- 示例解析：`config interface 'wan'` 表示「名为 wan 的网络接口配置实例」；`config wifi-iface 'wlan0'` 表示「名为 wlan0 的 WiFi 接口配置实例」。

- 注意：配置类型是 OpenWRT 约定好的（如 interface、wifi-iface、zone、dhcp），实例名可自定义（如 wan、lan、wlan2g、wlan5g）。

### （2）option 键值对（单值配置项）

- 格式：`option &lt;键名&gt; &lt;值&gt;`（字符串值建议加单引号，数字、布尔值可省略）

- 作用：配置某个实例的具体参数，是 UCI 配置的核心最小单元。

- 示例解析：`option proto 'pppoe'` 定义 wan 接口的拨号协议为 PPPoE；`option ifname 'eth0'` 绑定 wan 接口到物理网卡 eth0。

### （3）list 列表（多值配置项）

- 格式：`list <键名> <值>`

- 作用：存储一个配置项的多个值（如多个 DNS 服务器、多个防火墙规则、多个绑定网卡），同一个 list 键可以重复多次，值会自动合并为列表。

- 示例解析：`list dns '8.8.8.8'` 和 `list dns '114.114.114.114'` 表示 wan 接口的 DNS 服务器为 8.8.8.8 和 114.114.114.114（两个值同时生效）。

## 2. UCI 配置的“生效逻辑”（高频考点）

UCI 配置文件（/etc/config/下）只是「静态配置」，修改后必须经过两步操作，才能让配置生效（缺一不可），这是常考的细节：

1. `uci commit [模块名]`：将内存中修改的配置，写入对应的 /etc/config/ 静态配置文件（若不指定模块名，会提交所有模块的修改）；

2. `uci reload [模块名]` 或 `/etc/init.d/<服务名> restart`：让对应服务重新加载配置，使修改生效（如 reload network 重启网络服务，restart firewall 重启防火墙服务）。

> 注意：只 commit 不 reload → 配置写入文件，但服务仍使用旧配置；只 reload 不 commit → 服务临时生效，重启后修改会丢失。
> 
> 

# 三、UCI 核心操作命令（实操+必记）

所有 UCI 操作都通过 `uci` 命令行工具完成，以下是 CPE 开发中最常用的命令，按「查→改→删→新增→生效」分类整理，直接记熟即可用于实操和。

|操作类型|命令示例|作用说明|
|---|---|---|
|查看配置|`uci show network`|查看 network 模块（/etc/config/network）的所有配置，格式为「模块名.实例名.键名=值」|
||`uci get network.wan.proto`|查看指定配置项：network 模块、wan 实例、proto 键的值（如返回 pppoe）|
|修改配置|`uci set network.wan.proto=dhcp`|修改 network.wan.proto 的值为 dhcp（将 WAN 改为 DHCP Client 模式）|
||`uci add_list network.wan.dns=223.5.5.5`|给 network.wan.dns 列表新增一个值（多值配置用 add_list）|
||`uci set network.lan.ipaddr=192.168.2.1`|修改 LAN 接口的静态 IP 为 192.168.2.1（CPE 常用操作）|
|删除配置|`uci del network.wan.password`|删除 network.wan 实例下的 password 配置项|
||`uci del_list network.wan.dns=8.8.8.8`|删除 network.wan.dns 列表中的某个值（多值配置用 del_list）|
|新增配置|`uci add network interface`|给 network 模块新增一个 interface 类型的配置实例（返回随机实例名，如 cfg012345）|
||`uci rename network.cfg012345=lan2`|将新增的随机实例名（cfg012345）重命名为 lan2（便于管理）|
|配置生效|`uci commit network`|将 network 模块的修改写入 /etc/config/network 文件|
||`uci reload network`|重新加载 network 配置，等同于 /etc/init.d/network restart|
## 实战示例（CPE 中修改 WAN 为静态 IP）

以下命令是 CPE 开发中常见的实操场景，直接套用即可，时可作为实战案例说明：

```bash
# 1. 修改 WAN 接口为静态 IP 模式
uci set network.wan.proto=static
uci set network.wan.ipaddr=192.168.1.100  # WAN 静态 IP
uci set network.wan.netmask=255.255.255.0 # 子网掩码
uci set network.wan.gateway=192.168.1.1   # 网关
uci add_list network.wan.dns=114.114.114.114 # DNS 服务器

# 2. 提交修改并使配置生效
uci commit network
uci reload network
```

# 四、UCI 体系的底层工作机制（问深度时说）

中若被问到“UCI 底层怎么工作的”，可按以下逻辑回答，体现技术深度：

## 1. 配置存储机制

- 静态配置：存储在 /etc/config/ 目录下的文本文件，人类可读、可直接修改（适合手动配置或批量脚本修改）；

- 运行时配置：系统启动后，UCI 会将静态配置加载到内存中，通过 ubus（OpenWRT 核心 IPC 机制）提供给其他服务（如 netifd、hostapd、LuCI）调用。

## 2. 配置解析流程

1. OpenWRT 启动时，procd（系统初始化守护进程）会调用 `uci_load()` 接口，加载 /etc/config/ 目录下所有配置文件；

2. 各个服务（如 netifd 管理网络、hostapd 管理 WiFi）启动时，通过 UCI 提供的 API（C 语言 libuci 库）读取内存中的配置，将其转化为自身的运行参数；

3. 当通过命令行/脚本/LuCI 修改 UCI 配置后，commit 会更新静态文件，reload 会触发服务重新读取配置，实现动态生效。

## 3. API 扩展（CPE 开发常用）

UCI 不仅支持命令行操作，还提供多种 API，满足不同开发场景：

- C 语言 API：通过 libuci 库，自定义业务程序（如 TR069 客户端）可直接读写 UCI 配置，无需解析文本文件；

- Lua 接口：LuCI（OpenWRT Web 管理界面）基于 Lua 调用 UCI API，实现 Web 端的配置修改；

- Shell 脚本：通过 uci 命令行工具，编写批量配置脚本（如 CPE 量产时的参数初始化）。

# 五、UCI 在 CPE 产品中的典型应用场景

结合 CPE 产品开发实际，UCI 贯穿所有核心配置场景，时可结合以下场景说明，体现实战经验：

## 1. 网络配置（核心场景）

- WAN 配置：PPPoE 拨号、5G 拨号、静态 IP、DHCP Client 等，全部通过修改 /etc/config/network 实现；

- LAN 配置：LAN 网段、DHCP 服务器、网桥（LAN 网口 + WiFi 桥接），通过 network 和 dhcp 模块配置；

- WiFi 配置：SSID、加密方式（WPA2/WPA3）、信道、功率，通过 /etc/config/wireless 配置（归属于 LAN 侧，桥接 br-lan）。

## 2. 批量配置下发

CPE 量产时，通过编写 Shell 脚本，批量修改 UCI 配置（如统一设置 WAN 拨号参数、WiFi 名称、管理员密码），实现批量初始化，提升生产效率。

## 3. 远程配置管理

TR069 服务器通过 CWMP 协议，调用 UCI 接口（或通过 ubus 间接调用），远程修改 CPE 的配置参数（如 WiFi 名称、DNS 服务器、端口映射规则），实现远程运维。

## 4. 配置备份与恢复

CPE 配置备份：直接打包 /etc/config/ 目录，即可备份所有配置（UCI 所有静态配置文件）；

配置恢复：将备份的 /etc/config/ 目录覆盖到设备中，commit 后 reload 对应服务，即可恢复所有配置，适合故障排查后的配置还原。

# 六、常见问题 & 标准答案（直接背）

整理 CPE 中高频问到的 UCI 相关问题，给出简洁、专业的标准答案，避免临场卡顿：

## 问题 1：UCI 是什么？和传统 Linux 配置有什么区别？

**标准答案**：UCI 是 OpenWRT 的统一配置接口，核心是将分散的系统、网络、WiFi 等配置，集中到 /etc/config/ 目录，提供统一的命令和 API 操作；传统 Linux 配置分散在不同的配置文件（如 /etc/network/interfaces、/etc/sysctl.conf），操作不统一，嵌入式场景下维护成本高，而 UCI 解决了配置碎片化问题，适配 CPE 等嵌入式设备的管理需求。

## 问题 2：修改 UCI 配置后，为什么必须执行 commit 和 reload？两者的区别是什么？

**标准答案**：commit 的作用是将内存中修改的配置，写入 /etc/config/ 对应的静态配置文件，确保重启后修改不丢失；reload 的作用是让对应服务（如 network、firewall）重新读取配置文件，使修改立即生效。两者缺一不可：只 commit 不 reload，服务仍使用旧配置；只 reload 不 commit，重启后修改会失效。

## 问题 3：CPE 中，如何通过 UCI 配置 WiFi 和 LAN 桥接？

**标准答案**：核心是将 WiFi 接口加入 LAN 网桥（br-lan），步骤如下：1. 配置 LAN 接口为网桥模式（uci set network.lan.type=bridge）；2. 将 LAN 网口和 WiFi 接口绑定到网桥（uci set network.lan.ifname='eth0.1 wlan0'，eth0.1 是 LAN 网口 VLAN，wlan0 是 WiFi 接口）；3. 执行 uci commit network 和 uci reload network，即可实现 WiFi 与 LAN 桥接，两者共享同一网段。

## 问题 4：UCI 中的 option 和 list 有什么区别？分别用在什么场景？

**标准答案**：option 用于单值配置，一个键对应一个值（如 WAN 接口的 proto、ipaddr）；list 用于多值配置，一个键对应多个值（如 DNS 服务器、多网卡绑定）。场景举例：配置 WAN 拨号协议用 option，配置多个 DNS 服务器用 list。

# 七、总结（核心记忆点）

- UCI 核心：**统一配置格式 + 统一操作接口**，解决 OpenWRT 配置碎片化，是 CPE 配置管理的核心；

- 核心结构：/etc/config/ 目录 + 配置文件三层结构（config → option/list）；

- 操作流程：**查/改/删/新增 → commit（写入文件） → reload（服务生效）**（必记）；

- 实战价值：CPE 所有核心配置（网络/WiFi/防火墙/TR069）都依赖 UCI，是上层应用与底层服务的配置桥梁。

掌握以上内容，可轻松应对 CPE 中所有 UCI 相关问题，同时满足日常开发实操需求。