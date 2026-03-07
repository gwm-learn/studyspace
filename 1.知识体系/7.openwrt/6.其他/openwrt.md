# OpenWrt 架构与设计

## 一、OpenWrt 概述

### 1.1 什么是 OpenWrt？
OpenWrt 是一个基于 Linux 的嵌入式操作系统，专为路由器和嵌入式设备设计。它提供了一个完全可写的文件系统，并配备了包管理系统，允许用户自由安装、移除和定制软件，从而摆脱厂商固件的限制。

### 1.2 历史背景
- **2004 年**：基于 Linksys WRT54G 路由器的 GPL 源码发布，社区开始开发替代固件。
- **2005 年**：OpenWrt 项目正式成立，逐步形成独立的构建系统和包管理。
- **2010 年**：引入 LuCI Web 界面，成为最流行的第三方路由器固件之一。
- **2016 年**：LEDE（Linux Embedded Development Environment）项目从 OpenWrt 分支，2018 年重新合并为 OpenWrt。
- **至今**：持续活跃，支持数百种设备，涵盖家用路由器、企业网关、物联网设备等。

### 1.3 核心特点
- **完全开源**：遵循 GPL 等自由软件许可证。
- **可扩展性**：通过 `opkg` 包管理系统安装数千个软件包。
- **统一配置**：使用 UCI（Unified Configuration Interface）管理配置。
- **轻量级**：采用 BusyBox、musl libc（或 uClibc）减小体积。
- **稳定性**：进程由 `procd` 监控，支持自动重启（respawn）。
- **网络功能强大**：完整的 Linux 网络栈，支持防火墙、QoS、VPN 等。

### 1.4 适用场景
- **家用路由器**：替代厂商固件，提供更多功能（如广告过滤、流量监控）。
- **企业网关**：支持 VLAN、多 WAN、负载均衡、VPN 等高级网络特性。
- **物联网网关**：集成 MQTT、CoAP 等协议，连接传感器网络。
- **网络实验平台**：提供灵活的 Linux 环境，用于网络协议开发与测试。
- **嵌入式开发**：作为基础操作系统，快速构建定制嵌入式产品。

---

## 二、系统架构

### 2.1 整体架构图
```
+-----------------------------------+
|         应用程序层                |
|  (LuCI、samba、dropbear、etc.)   |
+-----------------------------------+
|         系统服务层                |
|  (netifd、firewall、dnsmasq、    |
|   ubus、procd、UCI)              |
+-----------------------------------+
|         运行环境层                |
|  (BusyBox、libc、内核模块)       |
+-----------------------------------+
|         Linux 内核层              |
|  (网络栈、驱动、文件系统)         |
+-----------------------------------+
|         硬件抽象层                |
|  (SoC、交换机、无线、Flash)      |
+-----------------------------------+
```

### 2.2 硬件抽象层
OpenWrt 支持多种硬件平台，包括：
- **CPU 架构**：MIPS、ARM、ARM64、x86、PowerPC 等。
- **网络芯片**：Atheros、MediaTek、Qualcomm、Realtek 等。
- **无线芯片**：支持 802.11a/b/g/n/ac/ax，通常通过 `mac80211` 驱动。
- **存储**：NOR/NAND Flash、SPI Flash、eMMC、SD 卡。

### 2.3 Linux 内核层
OpenWrt 使用主线 Linux 内核，并打上针对嵌入式设备的补丁，包括：
- **网络优化**：减少内存占用、提高转发性能。
- **驱动集成**：包含大量无线、以太网、USB 驱动。
- **文件系统**：支持 SquashFS、JFFS2、UBIFS、ext4 等。
- **内核模块**：按需加载，减小内核体积。

### 2.4 运行环境层
- **C 库**：musl libc（默认）或 uClibc，轻量且高效。
- **工具集**：BusyBox 提供核心 Unix 工具（ash、cp、ls 等）的简化实现。
- **初始化系统**：`procd`（OpenWrt 专用）替代传统的 SysVinit。

### 2.5 系统服务层
这是 OpenWrt 的核心，包含以下关键组件：
- **UCI**：统一配置接口，集中管理所有配置文件。
- **ubus**：进程间通信总线，提供 RPC 和事件发布/订阅。
- **procd**：进程管理守护进程，负责服务启动、监控、重启。
- **netifd**：网络接口守护进程，管理接口、协议、桥接、VLAN。
- **防火墙**：基于 `iptables`/`nftables`，提供默认安全策略。
- **DHCP/DNS**：`dnsmasq` 同时提供 DHCP 和 DNS 缓存服务。

### 2.6 应用程序层
用户可自行安装的软件包，例如：
- **Web 界面**：LuCI（Lua Configuration Interface）。
- **文件共享**：samba、vsftpd。
- **远程访问**：dropbear（SSH 服务器）、openvpn、wireguard。
- **网络工具**：tcpdump、iperf3、curl。

---

## 三、核心组件详解

### 3.1 UCI（Unified Configuration Interface）
UCI 是 OpenWrt 的配置管理中心，旨在将分散的配置文件统一为简单、一致的语法。

#### 3.1.1 配置文件位置
所有 UCI 配置文件位于 `/etc/config/` 目录，例如：
- `network`：网络接口、VLAN、桥接。
- `wireless`：无线网络配置。
- `firewall`：防火墙规则。
- `system`：系统设置（时区、日志、NTP）。

#### 3.1.2 配置语法
```
config <section-type> '<section-name>'
    option <key> '<value>'
    list <list-key> '<list-value>'
```

示例：
```uci
config interface 'lan'
    option proto 'static'
    option ipaddr '192.168.1.1'
    option netmask '255.255.255.0'
    option device 'br-lan'
```

#### 3.1.3 命令行工具
```bash
# 读取配置
uci get network.lan.ipaddr

# 修改配置
uci set network.lan.ipaddr='192.168.2.1'

# 提交更改
uci commit network

# 导出配置
uci export network
```

#### 3.1.4 配置生效
UCI 配置修改后，需重启相关服务才能使更改生效。例如：
```bash
uci commit network
/etc/init.d/network reload
```

### 3.2 ubus（OpenWrt 微总线）
ubus 是轻量级的进程间通信（IPC）机制，用于 OpenWrt 内部服务间的消息传递。

#### 3.2.1 架构
- **ubusd**：守护进程，管理注册的服务与方法。
- **libubus**：客户端库，供服务调用。
- **ubus-cli**：命令行工具，用于测试与调试。

#### 3.2.2 常见对象与方法
- `network`：网络相关操作（重启、重载）。
- `network.interface.<name>`：接口状态、up/down。
- `service`：服务管理（启动、停止、列表）。
- `system`：系统信息（重启、关机）。

#### 3.2.3 调用示例
```bash
# 查看接口状态
ubus call network.interface.lan status

# 重启网络
ubus call network restart

# 监听事件
ubus listen &
```

#### 3.2.4 事件机制
ubus 支持事件发布与订阅，允许服务在状态变化时通知其他服务。例如，`netifd` 在接口 up 时发布 `interface.up` 事件。

### 3.3 procd（进程管理守护进程）
procd 是 OpenWrt 的初始化系统和服务管理器，替代传统的 SysVinit。

#### 3.3.1 主要功能
- **服务启动**：根据 `/etc/init.d/` 中的脚本启动服务。
- **服务监控**：通过 `respawn` 机制自动重启崩溃的服务。
- **状态管理**：维护服务状态（运行、停止、重启）。
- **触发器**：根据配置文件变化自动重启服务。

#### 3.3.2 服务定义
每个服务在 `/etc/init.d/` 中有一个脚本，其中包含 `START`、`STOP` 等变量，以及 `start()`、`stop()` 函数。

procd 风格的服务脚本使用 `procd_open_instance`、`procd_set_param` 等函数定义。

#### 3.3.3 常用命令
```bash
# 启动服务
/etc/init.d/network start

# 重启服务
/etc/init.d/network restart

# 查看服务状态
service network status
```

#### 3.3.4 与 ubus 集成
procd 通过 ubus 提供 `service` 对象，允许远程查询和管理服务状态。

### 3.4 netifd（网络接口守护进程）
netifd 负责管理所有网络接口，包括物理接口、VLAN、桥接、PPP 等，以及 DHCP、PPPoE 等协议。

#### 3.4.1 主要功能
- **配置解析**：读取 `/etc/config/network`，生成内部对象。
- **接口管理**：维护接口状态机（down、up、running、error）。
- **协议处理**：执行 DHCP、PPPoE、静态 IP 等协议客户端。
- **事件响应**：响应内核事件（链路 up/down）和用户命令。

#### 3.4.2 与 UCI 的交互
netifd 监视 `/etc/config/network` 的更改，并在配置变化时重新配置接口。

#### 3.4.3 调试命令
```bash
# 查看接口状态
ubus call network.interface.lan status

# 查看 netifd 日志
logread | grep netifd
```

### 3.5 防火墙（firewall）
OpenWrt 默认使用 `iptables`（或 `nftables`）实现防火墙，并通过 `/etc/config/firewall` 进行配置。

#### 3.5.1 区域（Zone）概念
- `lan`：信任的内部网络。
- `wan`：不信任的外部网络。
- `guest`：访客网络（可选）。

#### 3.5.2 配置示例
```uci
config zone
    option name 'lan'
    option network 'lan'
    option input 'ACCEPT'
    option output 'ACCEPT'
    option forward 'ACCEPT'

config zone
    option name 'wan'
    option network 'wan'
    option input 'REJECT'
    option output 'ACCEPT'
    option forward 'REJECT'
    option masq '1'   # 启用 NAT
```

#### 3.5.3 规则生效
修改配置后，需重启防火墙：
```bash
/etc/init.d/firewall restart
```

### 3.6 dnsmasq（DHCP & DNS 服务器）
dnsmasq 是轻量级的 DHCP 和 DNS 转发服务器，OpenWrt 用它为 LAN 客户端提供 IP 地址和域名解析。

#### 3.6.1 配置文件
`/etc/config/dhcp` 控制 dnsmasq 的行为。

#### 3.6.2 功能
- **DHCP 服务器**：为指定接口分配 IP 地址。
- **DNS 缓存**：加速域名解析。
- **DNS 转发**：将查询转发到上游 DNS 服务器。
- **静态租约**：基于 MAC 地址分配固定 IP。

---

## 四、文件系统结构

### 4.1 只读根文件系统
OpenWrt 的根文件系统通常为 SquashFS，压缩率高且只读，确保系统核心不被意外修改。

### 4.2 Overlay 文件系统
OverlayFS 将只读的根文件系统与可写的 `overlay` 分区合并，实现“写时复制”。用户对文件的修改存储在 `overlay` 中，不影响原始根文件系统。

#### 4.2.1 Overlay 分区
- 通常使用 JFFS2（NOR Flash）或 UBIFS（NAND Flash）。
- 挂载在 `/overlay`，与根文件系统合并为 `/`。

#### 4.2.2 持久化存储
所有对 `/etc`、`/root`、`/var` 等目录的修改都保存在 `overlay` 中，重启后不丢失。

### 4.3 关键目录
- `/etc`：配置文件（UCI 配置在此）。
- `/usr`：用户程序与库（只读）。
- `/tmp`：临时文件（RAM 磁盘）。
- `/var`：运行时数据（日志、锁文件等）。
- `/www`：LuCI Web 界面文件。
- `/lib`：库文件（只读）。

### 4.4 软件包安装
通过 `opkg` 安装的软件包将文件写入 `overlay`，因此即使根文件系统只读，也能添加新软件。

---

## 五、网络架构

### 5.1 网络栈组成
OpenWrt 采用标准的 Linux 网络栈，并在此基础上添加了针对路由器的优化。

#### 5.1.1 数据流向
```
外部网络 → 物理接口 → 内核网络栈 → 防火墙 → 路由决策 → 内部接口 → 客户端
```

#### 5.1.2 网络命名空间
OpenWrt 默认未使用网络命名空间，但可通过配置实现容器或 VPN 隔离。

### 5.2 网络配置流程
1. **硬件初始化**：内核驱动加载，识别网络接口。
2. **netifd 启动**：读取 `/etc/config/network`，创建接口、桥接、VLAN。
3. **协议执行**：根据配置启动 DHCP、PPPoE 等协议。
4. **路由设置**：根据协议结果添加默认路由、静态路由。
5. **防火墙加载**：根据 `/etc/config/firewall` 设置 iptables 规则。

### 5.3 无线网络
- **驱动**：`mac80211` 框架支持大部分无线芯片。
- **配置**：通过 `/etc/config/wireless` 定义无线接口和 SSID。
- **管理**：`hostapd` 用于 AP 模式，`wpa_supplicant` 用于客户端模式。

### 5.4 高级网络功能
- **多 WAN**：通过 `mwan3` 包实现负载均衡和故障转移。
- **QoS**：使用 `qos-scripts` 或 `sqm-scripts` 进行流量整形。
- **VPN**：支持 OpenVPN、WireGuard、IPsec。
- **IPv6**：完整支持 DHCPv6、SLAAC、6in4 等。

---

## 六、构建系统

### 6.1 OpenWrt Buildroot
OpenWrt 使用自研的构建系统（基于 Buildroot），用于编译固件、内核和软件包。

#### 6.1.1 核心组件
- **顶层 Makefile**：协调编译流程。
- **Config.in**：Kconfig 配置界面，选择目标平台、软件包等。
- **package/**：软件包定义（每个包一个目录）。
- **target/**：目标平台定义（如 `ar71xx`、`ramips`、`x86`）。
- **toolchain/**：交叉编译工具链。

#### 6.1.2 编译流程
```bash
# 获取源码
git clone https://github.com/openwrt/openwrt.git

# 更新 feeds（软件包索引）
./scripts/feeds update -a
./scripts/feeds install -a

# 配置平台和软件包
make menuconfig

# 开始编译
make -j$(nproc)
```

#### 6.1.3 输出产物
- `bin/targets/`：固件镜像（如 `openwrt-ramips-mt7620-device-squashfs-sysupgrade.bin`）。
- `build_dir/`：中间构建文件。
- `staging_dir/`：工具链和宿主库。

### 6.2 SDK（软件开发工具包）
SDK 是为特定平台预编译的工具链，用于开发第三方软件包，无需完整编译 OpenWrt。

#### 6.2.1 使用 SDK 编译软件包
```bash
# 解压 SDK
tar xf openwrt-sdk-*.tar.xz

# 配置
cd openwrt-sdk-*
./scripts/feeds update
./scripts/feeds install

# 编译包
make package/helloworld/compile
```

### 6.3 软件包开发
软件包通过 `Makefile` 定义，位于 `package/` 目录下。OpenWrt 使用自己的构建系统（`OpenWrt Makefile`）来定义下载、编译、安装步骤。

示例 `Makefile`：
```makefile
include $(TOPDIR)/rules.mk

PKG_NAME:=helloworld
PKG_VERSION:=1.0
PKG_RELEASE:=1

include $(INCLUDE_DIR)/package.mk

define Package/helloworld
  SECTION:=utils
  CATEGORY:=Utilities
  TITLE:=Hello World
endef

define Package/helloworld/description
  A simple hello world program.
endef

define Build/Compile
  $(TARGET_CC) $(TARGET_CFLAGS) -o $(PKG_BUILD_DIR)/helloworld helloworld.c
endef

define Package/helloworld/install
  $(INSTALL_DIR) $(1)/usr/bin
  $(INSTALL_BIN) $(PKG_BUILD_DIR)/helloworld $(1)/usr/bin/
endef

$(eval $(call BuildPackage,helloworld))
```

---

## 七、软件包管理

### 7.1 opkg 包管理器
opkg 是 OpenWrt 的轻量级包管理器，源自 IPKG（Itsy Package Management System）。

#### 7.1.1 配置文件
`/etc/opkg.conf` 定义软件源、架构等。

#### 7.1.2 常用命令
```bash
# 更新软件包列表
opkg update

# 安装软件包
opkg install package-name

# 移除软件包
opkg remove package-name

# 列出已安装包
opkg list-installed

# 查找包
opkg find *keyword*
```

#### 7.1.3 软件源
OpenWrt 官方提供多个软件源，按版本和架构分类。例如：
```
src/gz openwrt_base https://downloads.openwrt.org/releases/21.02.0/packages/mipsel_24kc/base
src/gz openwrt_luci https://downloads.openwrt.org/releases/21.02.0/packages/mipsel_24kc/luci
```

### 7.2 软件包格式
软件包为 `.ipk` 文件，本质上是 `ar` 归档，包含：
- `control.tar.gz`：控制信息（包名、版本、依赖）。
- `data.tar.gz`：实际文件。
- `debian-binary`：格式版本号。

### 7.3 依赖管理
opkg 自动解决依赖关系，但有时需要手动安装缺失的依赖。

### 7.4 自定义软件包
用户可创建自己的软件包，并将其添加到本地 feed 中，然后通过 SDK 编译。

---

## 八、配置管理

### 8.1 配置备份与恢复
OpenWrt 提供 `sysupgrade` 工具用于固件升级，并可在升级时保留配置。

#### 8.1.1 备份配置
```bash
# 生成备份文件
sysupgrade -b backup.tar.gz
```

#### 8.1.2 恢复配置
将备份文件解压到根目录，然后重启。

### 8.2 配置版本控制
可将 `/etc/config/` 目录纳入 Git 等版本控制系统，便于跟踪配置变更。

### 8.3 配置模板
某些软件包（如 `luci-app-xxx`）会在首次安装时生成默认 UCI 配置。用户可修改这些配置，但删除软件包不会自动删除配置。

### 8.4 配置验证
使用 `uci show` 可查看当前配置状态，确保配置正确。

---

## 九、进程与服务管理

### 9.1 procd 服务管理
如前所述，procd 负责服务生命周期管理。

#### 9.1.1 服务脚本位置
- `/etc/init.d/`：服务启动脚本。
- `/etc/rc.d/`：符号链接，指向 `/etc/init.d/` 中的脚本，用于定义启动顺序。

#### 9.1.2 启用/禁用服务
```bash
# 启用服务（创建符号链接）
/etc/init.d/network enable

# 禁用服务（删除符号链接）
/etc/init.d/network disable
```

### 9.2 日志管理
OpenWrt 使用 `logd` 和 `ubus` 记录系统日志。日志可通过 `logread` 查看。

```bash
# 查看日志
logread

# 跟踪日志
logread -f

# 清除日志
logread -c
```

### 9.3 监控与调试
- `top`、`htop`：查看进程资源占用。
- `ps`：列出进程。
- `netstat`、`ss`：查看网络连接。
- `tcpdump`：抓包分析。

---

## 十、开发与调试

### 10.1 开发环境搭建
推荐在 Linux 主机上搭建 OpenWrt 编译环境，或使用 Docker 镜像。

### 10.2 调试工具
- **gdb**：远程调试（需在目标设备上安装 `gdbserver`）。
- **strace**：跟踪系统调用。
- **valgrind**：内存检查（性能开销大，嵌入式设备慎用）。
- **tcpdump**：网络抓包。

### 10.3 日志添加
在自定义软件中添加日志，可通过 `syslog` 输出到 `logd`。

```c
#include <syslog.h>

syslog(LOG_INFO, "Hello from my program");
```

### 10.4 内核调试
- `dmesg`：查看内核日志。
- `/proc`、`/sys`：查询内核状态。
- `kgdb`：内核调试（需要串口）。

---

## 十一、安全考虑

### 11.1 默认安全设置
OpenWrt 默认启用防火墙，仅允许 LAN 到 WAN 的转发，WAN 口入站连接被拒绝。

### 11.2 安全加固建议
- **修改默认密码**：首次登录后立即修改 root 密码。
- **禁用不必要的服务**：如 Telnet、FTP。
- **使用 SSH 密钥认证**：禁用密码登录。
- **定期更新**：通过 `opkg upgrade` 更新软件包。
- **防火墙精细化**：仅开放必要的端口。

### 11.3 安全漏洞管理
关注 OpenWrt 安全公告（[openwrt.org/advisory](https://openwrt.org/advisory)），及时打补丁。

---

## 十二、常见架构面试题

### 12.1 基础概念
1. **OpenWrt 与普通 Linux 发行版的主要区别是什么？**
   > OpenWrt 专为嵌入式网络设备设计，采用只读根文件系统 + overlay、UCI 统一配置、opkg 包管理、procd 进程管理等特有组件。

2. **UCI 的作用是什么？请举例说明。**
   > UCI 是统一配置接口，集中管理所有服务的配置。例如，网络配置保存在 `/etc/config/network`，通过 `uci set network.lan.ipaddr='192.168.1.1'` 修改。

3. **ubus 和 procd 之间的关系是什么？**
   > ubus 是进程间通信总线，procd 通过 ubus 提供服务管理接口（如启动、停止服务），其他进程可通过 ubus 调用 procd 的方法。

### 12.2 系统设计
4. **OpenWrt 如何实现配置持久化？**
   > 通过 overlay 文件系统：只读的根文件系统（SquashFS）与可写的 overlay 分区（JFFS2/UBIFS）合并，用户修改保存在 overlay 中。

5. **netifd 如何管理网络接口状态？**
   > netifd 为每个接口维护一个状态机（down、up、running、error），根据内核事件（链路变化）和协议事件（DHCP 成功）转换状态，并通过 ubus 发布事件。

6. **描述 OpenWrt 的启动流程。**
   > 1. Bootloader（如 U-Boot）加载内核；2. 内核挂载根文件系统；3. 启动 `/sbin/init`（即 procd）；4. procd 执行 `/etc/inittab` 和 `/etc/rc.d/` 中的服务脚本；5. 服务按顺序启动（网络、防火墙等）。

### 12.3 高级问题
7. **如何为 OpenWrt 开发一个新的软件包？**
   > 在 `package/` 目录下创建目录，编写 `Makefile` 定义下载、编译、安装步骤，然后通过 SDK 或完整编译系统编译。

8. **OpenWrt 如何处理无线网络？**
   > 通过 `mac80211` 驱动框架，配置保存在 `/etc/config/wireless`，由 `hostapd`（AP 模式）或 `wpa_supplicant`（客户端模式）实现。

9. **如何实现 VLAN 间的通信？**
   > 为每个 VLAN 接口配置 IP 地址，启用 IP 转发，并配置防火墙允许 VLAN 间流量。

10. **OpenWrt 的构建系统是如何工作的？**
    > 基于 Buildroot，使用 Kconfig 选择目标平台和软件包，通过顶层 Makefile 调用各包的 Makefile 进行交叉编译，最终生成固件镜像。

---

## 十三、总结与资源

### 13.1 总结
OpenWrt 是一个强大而灵活的嵌入式 Linux 发行版，其架构围绕网络设备的需求精心设计。掌握其核心组件（UCI、ubus、procd、netifd）和构建系统，对于开发和定制 CPE 路由器至关重要。

### 13.2 学习资源
- **官方文档**：[https://openwrt.org/docs/start](https://openwrt.org/docs/start)
- **源码仓库**：[https://github.com/openwrt/openwrt](https://github.com/openwrt/openwrt)
- **论坛**：[https://forum.openwrt.org/](https://forum.openwrt.org/)
- **Wiki**：[https://openwrt.org/](https://openwrt.org/)

### 13.3 推荐书籍
- *《OpenWrt 智能路由系统开发》*（中文）
- *《Embedded Linux Development with OpenWrt》*（英文）

### 13.4 实践建议
1. 购买一台支持 OpenWrt 的路由器（如 TP-Link Archer C7），刷入固件并实验。
2. 尝试编译自己的固件，添加自定义软件包。
3. 阅读核心组件源码（netifd、procd、ubus），深入理解其实现。

---

*最后更新：2026-03-07*  
*作者：OpenWrt CPE 开发团队*  
*文档用途：学习巩固与面试准备*