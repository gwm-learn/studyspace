# OpenWrt 学习回顾复习计划

本计划以 **OpenWrt 为核心**，联动知识体系、编程技能、协议应用、其他技能四大知识域，按模块依赖顺序分为 10 个模块，全面深挖源码。复习回顾以仓库已有笔记与 vendored 源码为教材，不生成新笔记，按依赖顺序推进。

## 学习地图总览（10 模块，依赖顺序）

```
模块一  编程与网络基础        → 为读源码打地基
模块二  Linux 开发环境与工具链 → 环境准备
模块三  OpenWrt 系统层        → 理解"系统怎么起来"
模块四  OpenWrt 网络数据面    → 核心中的核心（netifd 深挖）
模块五  网络服务              → DHCP/DNS/NAT/防火墙
模块六  内核网络栈            → 深入内核数据路径
模块七  WiFi 子系统           → 无线数据面
模块八  驱动开发              → 从上层到底层
模块九  CPE 业务与协议应用    → 产品级功能
模块十  综合实践              → 固件构建 + 复盘
```

---

## 模块一：编程与网络基础回顾

**学习目标**：恢复 C 语言手感，重构 TCP/IP 协议族知识框架，为阅读 netifd/kernel 源码扫清障碍。

**复习要点清单**
- [ ] C 语言核心回顾：指针/内存管理、结构体、链表、大小端、宏函数（重读 `2.编程技能/1.C语言/` 下的 大小端/宏函数/结构体/链表申请空间 笔记）
- [ ] 数据结构的实现回顾（`2.编程技能/3.数据结构/`）
- [ ] TCP/IP 分层模型与各层职责（`1.知识体系/1.计算机网络/m-1`）
- [ ] 链路层：以太网帧、MAC、VLAN（`2.TCP IP协议/m-2-链路层`）
- [ ] IP 层：IPv4/IPv6 编址、报文（`m-1-IP地址`）
- [ ] 传输层：TCP（三次握手/拥塞控制）、UDP（`m-11-TCP`、`m-10-UDP`）
- [ ] 应用层基础：DNS、DHCP（`m-9-DNS`、`m-4-DHCP`）
- [ ] 路由与交换基础（`1.知识体系/8.路由与交换/` 全部 7 篇）

**源码深挖点**：重新编译并阅读自己的 C 实践项目 `fw_params`、`mobile-mngr`、`ipc/`、`produce` 的源码，重点看 socket 编程与多进程/多线程通信。

**实践建议**：跑通 `ipc/TEST`（CMake）与 `ipc/Template/7-socket`（make），梳理 socket 编程调用链。

---

## 模块二：Linux 开发环境与工具链

**学习目标**：掌握 OpenWrt 开发所需的环境与构建工具。

**复习要点清单**
- [ ] shell 脚本常用技巧（`4.其他技能/2.shell/`）
- [ ] make 与 Makefile 规则（`4.其他技能/4.make/`）
- [ ] CMake 基础（`4.make/CMakeLists.txt/`）
- [ ] docker 测试环境搭建（`4.其他技能/1.docker/` 及 example 下 4 个 Dockerfile）
- [ ] OpenWrt 交叉编译与固件编译（`7.openwrt/6.其他/编译.md`）
- [ ] Linux 常用命令与技巧（`4.其他技能/3.linux/`）

**源码深挖点**：读懂 `2.编程技能/1.C语言/fw_params/src/Makefile` 的嵌套 make 结构。

**实践建议**：在本机用 docker 起一个 OpenWrt 编译/测试环境，完成一次最小固件编译。

---

## 模块三：OpenWrt 系统层（管理平面）

**学习目标**：理解 OpenWrt 从加电到服务可用的完整启动链路。

**复习要点清单**
- [ ] 启动流程五阶段：Bootloader → 内核 → procd → 服务（`7.openwrt/2.应用管理平面/启动流程.md`）
- [ ] procd 进程管理（`6.其他/procd.md`）
- [ ] ubus IPC 机制（`6.其他/ubus.md`）
- [ ] UCI 配置三层结构（`1.配置/uci.md`）
- [ ] 热插拔机制 hotplug/uevent（`2.应用管理平面/热插拔.md`）
- [ ] opkg 包管理、固件升级、恢复模式（`6.其他/` 下 opkg/upgrade/recovery）

**源码深挖点**：结合 `1.知识体系/4.linux源码/Linux011启动流程.md` 理解内核启动，再看 OpenWrt init 脚本 → procd 的衔接。

**实践建议**：在设备/容器中追踪 `dmesg` + `logread` 验证启动阶段；用 `ubus call` 手动调用服务。

---

## 模块四：OpenWrt 网络数据面（核心）

**学习目标**：吃透 netifd 网络接口管理机制，这是路由器开发的灵魂。

**复习要点清单**
- [ ] 基础网络模型：WAN（PPPoE/DHCP Client/5G拨号/StaticIP）+ LAN（DHCP Server/网桥/网段）（`3.网络业务/1.基础网络模型/`）
- [ ] netifd 功能与状态机（`1.网络管理/1.netifd.md`）
- [ ] netifd 新手入门 → 深度技术分析 → 完整调用链 → 启动与事件驱动流程（`netifd/` 下 6 篇分析文）
- [ ] bridge 原理与 OpenWrt 应用（`1.网络管理/2.bridge.md`）
- [ ] vlan 原理与 OpenWrt 应用（`1.网络管理/3.vlan.md`）
- [ ] 路由表管理 / 策略路由 / 多WAN（`2.路由/1.路由表管理.md`）

**源码深挖点**（本模块重点，全面深挖 vendored 源码 `netifd/`）：
- `main.c` 启动与主循环；`interface.c` 接口状态机；`proto.c`/`proto-shell.c` 协议处理；`ubus.c` RPC 接口；`system-linux.c` netlink 交互。
- 结合 `netifd/AGENTS.md`（构建、CMake、架构）与 6 篇分析文逐函数对照。

**实践建议**：自己编译 netifd（`cmake .. && make`），用 gdb 断点跟踪一次 interface up/down 流程。

---

## 模块五：网络服务（DHCP/DNS/NAT/防火墙）

**学习目标**：掌握路由器三大网络服务及其与内核的交互。

**复习要点清单**
- [ ] dnsmasq 双角色：DHCP + DNS（`4.DHCP&DNS/1.dns/dnsmasq.md`、`dns.md`）
- [ ] OpenWrt DHCP Server/Client 配置（`4.DHCP&DNS/2.dhcp/dhcp.md`、`1.基础网络模型/2.LAN/1.DHCP Server.md`）
- [ ] iptables vs nftables、fw3/fw4 演进（`3.NAT&防火墙/1.iptables&nftables.md`）
- [ ] SNAT/Masquerade（`3.NAT&防火墙/2.NAT源地址转换.md`）
- [ ] 端口映射/DNAT、NAT 回流（`3.NAT&防火墙/3.端口映射.md`）
- [ ] DMZ（`3.NAT&防火墙/4.DMZ.md`）
- [ ] 联动协议层：DHCPv4/v6 报文细节（`2.TCP IP协议/m-4-DHCP`、`m-12-DHCPv6`）、NAT 类型与穿透（`m-6-NAT`）

**源码深挖点**：dnsmasq 源码的 DHCP 处理；`3.协议应用/4.dhcp/` 与 `3.协议应用/2.snmp/src/`（SNMP 联动查看）。

**实践建议**：用 `conntrack -L` 观察 NAT 会话表；抓包对比 SNAT/DNAT 前后的 IP/端口变化。

---

## 模块六：内核网络栈（深挖内核数据路径）

**学习目标**：理解数据包从网卡到 socket 的完整内核路径，及关键内核模块。

**复习要点清单**
- [ ] 收发包流程（`7.openwrt/4.内核网络/1.linux网络栈/1.收发包流程.md` + `5.linux网络/1.内核接收网络包.md`、`3.内核发送网络包.md`）
- [ ] 内核与用户进程协作（`5.linux网络/2.内核与用户进程协作.md`）
- [ ] Netfilter 五链四表、钩子点、conntrack（`4.内核网络/2.内核关键模块/1.Netfilter框架.md` + `6.linux防火墙/` 全部）
- [ ] 网桥模块 / ebtables（`2.网桥模块.md`）
- [ ] PPP 协议栈 / pppd / PPPoE 内核路径（`3.PPP协议栈.md`）
- [ ] USB 子系统：枚举、RNDIS/ECM/QMI、usb-modeswitch（`4.USB子系统.md`）
- [ ] 网络命名空间 netns（`1.linux网络栈/2.网络命名空间.md`）
- [ ] netlink / generic netlink（`5.netlink.md`）

**源码深挖点**：对照 `1.知识体系/4.linux源码/Linux011/`（老内核，逻辑简单易读）理解内核基础机制，再跳回现代内核对应路径；`6.linux防火墙/` 的 Netfilter 图 + 文字对照。

**实践建议**：写一个 netlink 通信的简单 C 程序（可复用 `2.编程技能/1.C语言/net/` 的练习）；用 `ip netns` 做隔离实验。

---

## 模块七：WiFi 子系统

**学习目标**：理解 OpenWrt 无线栈从用户空间到内核的分层。

**复习要点清单**
- [ ] WiFi 基础概念：频段/信道、加密演进（WEP→WPA3）、802.11 协议族、漫游（`5.WiFi子系统/2.wifi相关概念.md` + `1.知识体系/10.wifi/wifi记录.md`）
- [ ] cfg80211 内核子系统（`1.cfg80211.md`）
- [ ] nl80211 netlink 接口（`2.nl80211.md`）
- [ ] mac80211 与驱动的分层关系（补全链路：hostapd/wpa_supplicant → nl80211 → cfg80211 → mac80211 → 驱动）

**源码深挖点**：结合 `2.编程技能` 无直接对应，重点读 `iw` 工具用法与 `hostapd` 配置语义。

**实践建议**：用 `iw dev` 扫描/连接，抓取 mgmt 帧对比 802.11 帧结构。

---

## 模块八：驱动开发

**学习目标**：从上层一路下探到硬件驱动层。

**复习要点清单**
- [ ] Linux 驱动体系总览（`1.知识体系/3.linux驱动/总结.md`，含并发/阻塞IO/异步通知/中断时钟/内存IO访问）
- [ ] 字符设备驱动结构（`3.linux驱动/` 下的 drawio 图 + 总结）
- [ ] GPIO 驱动实战（`3.linux驱动/gpio/`：AGENTS.md、ARCHITECTURE_ANALYSIS.md、gpio_customize）
- [ ] LED 子系统（`3.linux驱动/led/led_driver_analysis.md`）
- [ ] 以太网 switch 驱动 DSA vs swconfig（`7.openwrt/5.驱动/1.以太网switch驱动.md`）

**源码深挖点**：`gpio/gpio_customize/`（用户自研驱动，可直接编译）；`led/leds/`（内核 LED 驱动源码）。

**实践建议**：按 `gpio/AGENTS.md` 用 `make M=... modules` 编译 GPIO 驱动，insmod 验证 sysfs 接口。

---

## 模块九：CPE 业务与协议应用

**学习目标**：掌握路由器产品的业务层功能与相关协议实现。

**复习要点清单**
- [ ] TR-069/TR-369 远程管理（`6.CPE专属业务/1.TR069&TR369.md` + `3.协议应用/1.tr069`、`7.tr369`）
- [ ] QoS / 流量控制 / 端口限速（`6.CPE专属业务/2.Qos.md`、`3.流量控制.md`、`4.端口限速.md`）
- [ ] 5G 拨号：QMI/MBIM/AT（`7.5G拨号/5G拨号.md` + `9.lte5g相关/` 的 QMI 系列）
- [ ] 协议联动复习：PPPoE（`3.协议应用/6.pppoe`）、SNMP（`2.snmp` 源码）、UPnP（`8.upnp`）、wireguard（`9.wireguard`）、L2TP（`10.L2TP`）、OMA-DM（`11.oma dm`）
- [ ] 组播 IGMP/MLD（`2.TCP IP协议/m-8-IGMP和MLD`）

**源码深挖点**：`3.协议应用/2.snmp/src/`（完整 net-snmp 源码，autotools）；TR069 若需可看开源 easycwmp。

**实践建议**：搭建 SNMP 测试环境（`2.snmp/conf/` 有 snmpd 配置脚本）；用 `tcpdump` 抓 TR069 的 HTTP 会话。

---

## 模块十：综合实践

**学习目标**：把前面九模块串起来，完成一次端到端的固件构建与问题排查复盘。

**复习要点清单**
- [ ] Yocto 固件构建（`4.其他技能/6.yocto/`）
- [ ] OpenWrt 编译系统复盘（`7.openwrt/6.其他/编译.md`）
- [ ] 故障排查方法论（`7.openwrt/6.其他/故障排查.md`）
- [ ] docker 化测试环境（`4.其他技能/1.docker/`）
- [ ] 综合排查：从应用 → netifd → 内核 → 驱动 逐层定位问题

**源码深挖点**：综合复盘时回到前几模块的源码锚点逐层对照——netifd（`netifd/` 源码）确认接口状态机、内核（`5.linux网络/` + `6.linux防火墙/`）确认收发包与 NAT 路径、驱动（`3.linux驱动/gpio/`）确认 sysfs 接口，形成"应用→netifd→内核→驱动"的完整调用链闭环。

**实践建议**：设计一个端到端场景（如"5G 拨号 + NAT + WiFi + TR069 远程配置"），把各模块知识点串起来做一次完整复盘。
