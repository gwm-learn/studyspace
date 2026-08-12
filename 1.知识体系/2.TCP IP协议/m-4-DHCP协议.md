# DHCPv4 协议详解

## 1. 概述：DHCPv4在IPv4网络中的角色

DHCP（Dynamic Host Configuration Protocol，动态主机配置协议）的前身是 BOOTP（RFC 951），v4 主规范为 **RFC 2131**，选项定义见 **RFC 2132**。DHCPv4 的核心作用：

- 为 IPv4 主机**动态分配 IP 地址**，免去手工逐台配置
- 同时下发子网掩码、默认网关、DNS 服务器等网络参数
- 通过**租约（Lease）**机制管理地址的分配、续期与回收，实现地址复用

DHCPv4 基于 UDP 传输，**服务器端口 67，客户端端口 68**。客户端在尚未获得 IP 地址时没有可用的源地址，只能以源 IP `0.0.0.0`、目的地址 `255.255.255.255` 的**广播**方式寻找服务器。

### 1.1 三种地址分配方式（RFC 2131）

| 方式 | 说明 | 适用场景 |
|------|------|----------|
| 动态分配（Dynamic） | 服务器从地址池中挑选地址，有租约，到期回收 | PC、手机等普通终端 |
| 自动分配（Automatic） | 与动态类似，但首次分配后地址固定归该客户端 | 需要地址保持稳定的设备 |
| 手动分配（Manual） | 管理员手工把 MAC 与地址绑定，DHCP 仅下发参数 | 服务器、打印机、网络设备 |

## 2. 四步交互流程（DISCOVER → OFFER → REQUEST → ACK）

客户端上电、接口 UP 后进入 INIT 状态，开始四步交互获取地址：

```
客户端                                          DHCP服务器
  │  ① DHCPDISCOVER（广播）                       │
  │ ───────────────────────────────────────────► │
  │  ② DHCPOFFER（单播或广播）                   │
  │ ◄─────────────────────────────────────────── │
  │  ③ DHCPREQUEST（广播）                       │
  │ ───────────────────────────────────────────► │
  │  ④ DHCPACK（单播或广播）                     │
  │ ◄─────────────────────────────────────────── │
  ▼
绑定完成（BOUND），地址可以使用
```

### 2.1 DISCOVER：寻找服务器

- 客户端广播发送 DISCOVER，源 IP `0.0.0.0`、源端口 68，目的地址 `255.255.255.255`、目的端口 67
- 关键选项：Message Type(53)=1，可选 Requested IP(50)、Hostname(12)
- 客户端不知道链路内有哪些服务器，DISCOVER 是"谁可以为我提供地址"的广播询问

### 2.2 OFFER：服务器报价

- 每个收到 DISCOVER 的服务器都会**预留**一个地址并回复 OFFER
- yiaddr 字段填提议的 IP 地址；options 携带 Subnet Mask(1)、Router(3)、DNS(6)、Lease Time(51)、Server Identifier(54)
- 若报文 flags 中的广播位（B 位）置 1，OFFER 用广播发送，否则可单播
- 客户端可能收到**多个** OFFER，一般选择**最先到达**的一个

### 2.3 REQUEST：选定服务器

- 客户端广播 REQUEST，携带 Server Identifier(54)=选中的服务器 IP，以及 Requested IP(50)=接受提议的地址
- 之所以**仍然广播**：同子网内的其他服务器也能收到该报文，得知"客户端选了别人"，可以**收回**各自预留的地址，避免地址泄漏

### 2.4 ACK / NAK：服务器确认

- 服务器收到 REQUEST 后确认地址仍可用，回复 **ACK**（Message Type=5），客户端正式进入 BOUND 状态，启动 T1/T2 定时器
- 若地址已被占用或策略不允许，回复 **NAK**（Message Type=6），客户端回到 INIT 状态重新 DISCOVER
- 客户端收到 ACK 后通常做一次 **ARP 冲突检测**：若发现地址冲突，发送 DECLINE(4) 通知服务器

| 阶段 | 报文 | 方向 | 关键内容 |
|------|------|------|----------|
| ① | DISCOVER | 客户端 → 广播 | Message Type=1，请求分配地址 |
| ② | OFFER | 服务器 → 客户端 | yiaddr=提议地址，携带掩码/网关/DNS/租期/Server ID |
| ③ | REQUEST | 客户端 → 广播 | Message Type=3，Server ID=选定服务器，Requested IP |
| ④ | ACK / NAK | 服务器 → 客户端 | Message Type=5 确认 / 6 拒绝 |

> 四步交互的意义在于**多服务器场景**下的协商：DISCOVER/OFFER 让客户端了解各家报价，REQUEST/ACK 完成"唯一归属"确认，避免多个服务器对同一地址重复分配。

### 2.5 DHCPINFORM：无地址获取参数

已通过静态配置或 PPPoE 等其他方式获得 IP 的主机，可用 **INFORM**（Message Type=8）仅请求 DNS、网关等参数。服务器回复 ACK（不携带 yiaddr），不分配地址，也不建立租约。

## 3. DHCPv4 报文格式

DHCP 报文由 BOOTP 固定头部、4 字节 magic cookie 和变长 options 组成，固定部分共 **236 字节**：

| 字段 | 长度(字节) | 说明 |
|------|-----------|------|
| **op** | 1 | 报文方向：1=BOOTREQUEST（客户端→服务器），2=BOOTREPLY（服务器→客户端） |
| **htype** | 1 | 硬件类型：1=以太网（Ethernet） |
| **hlen** | 1 | 硬件地址长度：以太网为 6 |
| **hops** | 1 | 客户端置 0，每经过一个中继代理加 1 |
| **xid** | 4 | 事务 ID，随机生成，客户端用它匹配请求与应答 |
| **secs** | 2 | 客户端开始请求后经过的秒数 |
| **flags** | 2 | 仅最高位（B 位，广播标志）有意义：置 1 要求服务器用广播回复 |
| **ciaddr** | 4 | 客户端当前 IP：BOUND 状态下续租、释放时填写 |
| **yiaddr** | 4 | "your" 地址：服务器分配给客户端的 IP，OFFER/ACK 中填写 |
| **siaddr** | 4 | 下一跳服务器 IP：无盘工作站 TFTP 引导等场景使用 |
| **giaddr** | 4 | 中继代理（Relay Agent）的 IP：客户端置 0，见第 6 章 |
| **chaddr** | 16 | 客户端硬件地址（MAC），客户端身份标识的核心 |
| **sname** | 64 | 服务器主机名（可选） |
| **file** | 128 | 启动文件名（可选，无盘引导使用） |
| **magic cookie** | 4 | 固定值 `0x63825363`，标识后续内容为 DHCP 选项 |
| **options** | 变长 | DHCP 选项，见第 5 章 |

> 报文格式直接沿用 BOOTP，DHCP 靠 magic cookie + options 与 BOOTP 区分。客户端通过 **xid + chaddr** 匹配服务器回包。

## 4. 租约管理（T1 / T2 / Renew / Rebind / Release）

DHCPv4 通过租约时间（Lease Time，常见默认 86400 秒即 1 天）与 T1、T2 两个定时器管理地址生命周期：

| 定时器 | 计算 | 触发动作 |
|--------|------|----------|
| **T1** | 0.5 × 租期 | 向**原服务器**单播 REQUEST 续租，进入 RENEWING 状态 |
| **T2** | 0.875 × 租期 | Renew 未收到应答，向**任意服务器**广播 REQUEST 续租，进入 REBINDING 状态 |

续租 REQUEST 携带 Requested IP(50)=当前地址、ciaddr=当前地址及当前租期，服务器确认后续发新租期，客户端重置 T1/T2。

```
BOUND ──T1到期──► RENEWING ──超时──► REBINDING ──超时且租约到期──► INIT
  │                 │                    │
  └──收到ACK────────┘  └──收到ACK────────┘
```

其他租约相关行为：

- **Release（释放）**：客户端主动放弃地址（关机、接口 DOWN），单播发送 RELEASE 报文，服务器把地址收回地址池
- **Decline（拒绝）**：客户端 ARP 检测到地址冲突后发送 DECLINE，服务器将该地址标记为不可用（进入冷却期）
- **租约到期仍未续上**：地址被收回，客户端返回 INIT 重新走四步流程

## 5. 关键 DHCP 选项

选项位于报文尾部，格式为 `选项号(1字节) + 长度(1字节) + 值`。常用选项：

| 选项号 | 选项名 | 作用 |
|--------|--------|------|
| **1** | Subnet Mask | 子网掩码，客户端据此计算本地子网范围 |
| **3** | Router | 默认网关地址列表 |
| **6** | DNS Server | DNS 服务器地址列表 |
| **12** | Hostname | 客户端上报的主机名，便于服务器记录与识别 |
| **15** | Domain Name | 域名后缀，配合主机名完成域名解析 |
| **50** | Requested IP | 客户端请求的特定地址（DISCOVER/REQUEST 中携带） |
| **51** | Lease Time | 租约时长（秒），服务器指定的地址有效时间 |
| **53** | Message Type | 报文类型：1=DISCOVER、2=OFFER、3=REQUEST、4=DECLINE、5=ACK、6=NAK、7=RELEASE、8=INFORM |
| **54** | Server Identifier | 服务器标识（IP 地址），客户端据此选定服务器 |

## 6. DHCP 中继代理（Relay Agent）

DHCP 广播报文**只在本地子网内传播**。客户端与服务器不在同一子网时，需靠中继代理（Relay Agent，通常是网关路由器）转发，实现在**跨子网分配地址**：

1. 客户端广播 DISCOVER 到 `255.255.255.255`，目的端口 67
2. 中继收到后，把报文源 IP 改为自己的接口地址，**giaddr 字段填入中继接口 IP**，hops 加 1，单播转发给服务器（如配置 `ip helper-address` 指向的服务器地址）
3. 服务器依据 **giaddr** 判断客户端所在的子网，从**对应地址池**中分配地址
4. 应答报文经中继回传：中继收到 ACK 后，把目的地址还原为客户端所在子网（可再广播给客户端）

```
客户端 ──DISCOVER(广播)──► 中继路由器 ──DISCOVER(giaddr=中继IP, 单播)──► DHCP服务器
客户端 ◄──ACK(广播)────── 中继路由器 ◄──ACK────────────────────────────── 服务器
```

中继还可在转发时追加 **Relay Agent Information 选项（option 82，RFC 3046）**，携带接入端口、交换机/DSLAM 标识等信息，服务器据此实现**按端口、按用户差异化分配**（不同地址段、不同租期、不同网关）。

> 同一子网内、无中继时，服务器通过 yiaddr/siaddr 与 chaddr 直接回包即可；跨子网场景必须依赖中继填充 giaddr，否则服务器无法判断客户端属于哪个地址池。

## 7. DHCPv4 vs DHCPv6 关键差异

| 对比项 | DHCPv4 | DHCPv6 |
|--------|--------|--------|
| 传输层端口 | UDP：服务器 67，客户端 68 | UDP：服务器 547，客户端 546 |
| 报文数量 | 8 种（DISCOVER/OFFER/REQUEST/ACK/NAK/DECLINE/RELEASE/INFORM） | 13 种（SOLICIT/ADVERTISE/REPLY/...），type 字段区分 |
| 报文类型标识 | 靠 options 中的 Message Type(53) 区分 | 报文头部直接携带 type 字段 |
| 客户端标识 | MAC 地址 + chaddr 字段 | DUID（DHCP Unique Identifier） |
| 地址冲突检测 | 客户端 ACK 后做 ARP 探测，冲突发 DECLINE；服务器也可预先 ping 探测 | 由独立的 DAD（重复地址检测）完成，与 DHCP 解耦 |
| 链路层寻址 | 以**广播**（255.255.255.255）为主，可选单播 | 全部使用**组播**（FF02::1:2）或单播，无广播 |
| 地址获取方式 | 地址与参数绑定分配；仅取参数用 INFORM | 地址与参数分离，有状态/无状态（SLAAC + Information-Request）两种模式 |
| 前缀委派 | 无对应机制 | 支持 PD（前缀代理，IA_PD） |
| 跨链路转发 | 中继代理填 giaddr 后单播转发 | 中继用 Relay-Forward/Relay-Reply 封装（option 9）转发 |

> DHCPv6 的完整笔记见 [m-12-DHCPv6协议.md](./m-12-DHCPv6协议.md)。

## 8. 参考标准

| RFC | 标题 | 与本主题的关系 |
|-----|------|----------------|
| **RFC 2131** | Dynamic Host Configuration Protocol | DHCPv4 主规范，定义四步交互、报文格式与租约状态机 |
| **RFC 2132** | DHCP Options and BOOTP Vendor Extensions | 定义选项格式与全部标准选项（1、3、6、50、51、53、54 等） |
| **RFC 1541** | Dynamic Host Configuration Protocol | DHCPv4 前身规范，已被 RFC 2131 取代 |
| **RFC 3046** | DHCP Relay Agent Information Option | 定义中继 option 82，用于跨子网差异化分配 |
| **RFC 3396** | Encoding Long Options in DHCP | 定义长度超过 255 字节选项的编码方式 |

## 9. 协议应用与实战

- DHCP 报文抓包分析、dhclient / dnsmasq 配置示例与排障：见 [协议应用DHCP](./../../3.协议应用/4.dhcp/DHCP.md)
- DHCPv6 详细笔记：[m-12-DHCPv6协议.md](./m-12-DHCPv6协议.md)
