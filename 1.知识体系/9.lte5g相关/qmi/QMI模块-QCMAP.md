# QCMAP
Qualcomm Mobile Access Point Service   

QMI_QCMAP提供一个命令集，用于与无线移动站连接以访问移动AP服务   

QCMAP把LTE模块变成一台迷你路由器：一端通过WWAN（LTE拨号）接入运营商网络，另一端通过WLAN（热点）或USB/RMNET以太网接入用户设备，在中间完成NAT、防火墙、DHCP分配地址等网关功能。QCMAP服务ID为0x20，与WDS（0x1b）、NAS（0x03）等QMI服务平级，客户端通过服务ID发起请求。

## 连接管理

| 命令 | 功能 |
| :--- | :--- |
| QMI_QCMAP_START_WWAN_CONNECTION | 启动WWAN连接（LTE拨号上网） |
| QMI_QCMAP_STOP_WWAN_CONNECTION | 停止WWAN连接 |
| QMI_QCMAP_GET_WWAN_STATUS | 查询WWAN连接状态 |
| QMI_QCMAP_SET_WLAN_CONFIG | 配置热点SSID、密码、加密方式 |
| QMI_QCMAP_GET_MOBILE_AP_STATUS | 查询移动AP整体运行状态 |
| QMI_QCMAP_SET_MAX_CONNECTION | 设置最大并发客户端数 |

START_WWAN_CONNECTION是热点上网的总开关，参数包括tech_pref（接入技术偏好）、apn_name（APN）、用户名密码、IP版本（IPv4/IPv6）。启动成功后模块获得运营商分配的IP地址，成为移动AP的WAN口。

## 防火墙

QCMAP内置防火墙功能，可对进出移动AP的数据流做过滤：

| 命令 | 功能 |
| :--- | :--- |
| QMI_QCMAP_SET_FIREWALL | 配置防火墙规则 |
| QMI_QCMAP_GET_FIREWALL_STATUS | 查询防火墙状态 |
| QMI_QCMAP_SET_IP_FILTERING | 设置IP包过滤规则 |
| QMI_QCMAP_GET_IP_FILTERING | 查询IP包过滤规则 |
| QMI_QCMAP_SET_SNAT_IP_FILTERING | 设置SNAT（源地址转换）过滤规则 |
| QMI_QCMAP_GET_SNAT_IP_FILTERING | 查询SNAT过滤规则 |

防火墙工作方式：
1. SET_FIREWALL支持enable/disable开关，控制整机防火墙。
2. IP过滤规则按"源IP、目的IP、协议（TCP/UDP/ICMP）、端口"五元组匹配，命中后执行allow或deny。
3. SNAT过滤只作用于从内网发出的流量，规则数量有限，多余流量按默认策略放行或丢弃。

典型应用：只允许内网设备访问指定服务器IP的80端口，其余流量一律丢弃，防止客户端乱上网。

## DHCP服务

QCMAP内置DHCP服务器，为接入热点的客户端分配IP地址：

| 命令 | 功能 |
| :--- | :--- |
| QMI_QCMAP_SET_DHCP_SERVER_CONFIG | 配置DHCP服务器 |
| QMI_QCMAP_GET_DHCP_SERVER_CONFIG | 查询DHCP服务器配置 |
| QMI_QCMAP_SET_DHCP_RESERVED_IP | 为指定MAC保留固定IP |
| QMI_QCMAP_GET_DHCP_LEASE_INFO | 查询租约信息 |

SET_DHCP_SERVER_CONFIG 的主要字段：

| 字段 | 说明 |
| :--- | :--- |
| starting_ip_address / ending_ip_address | 地址池起始/结束IP |
| subnet_mask / lease_time | 子网掩码 / 租约时间 |
| gateway_address | 网关（默认指向模块自身） |
| primary_dns / secondary_dns | 首选/备用DNS |

DHCP服务默认开启，地址池范围要覆盖max_connection配置的客户端数。客户端申请地址时，模块从池中分配空闲IP并把网关、DNS一并下发，客户端无需手动配置即可上网。

## NAT与端口转发

| 命令 | 功能 |
| :--- | :--- |
| QMI_QCMAP_SET_NAT_ENTRY | 添加NAT转换条目 |
| QMI_QCMAP_SET_PORT_FORWARDING | 设置端口映射 |

端口转发让外部访问到达内网设备：配置"外部端口+协议"映射到"内网IP+端口"，例如把外网的8080端口映射到192.168.225.10:80，配合防火墙放行即可从公网访问内网Web服务。

## 其他功能与事件上报

| 命令/指示 | 功能 |
| :--- | :--- |
| QMI_QCMAP_SET_QOS | 配置流分类与限速 |
| QMI_QCMAP_SET_DNS_QUERY_CONFIG | 配置DNS转发与域名过滤 |
| QMI_QCMAP_WWAN_STATUS_IND | 指示：WWAN连接建立/断开通知 |
| QMI_QCMAP_MOBILE_AP_STATUS_IND | 指示：移动AP状态变化通知 |

客户端注册indication后，无需轮询即可获知拨号掉线、客户端上下线等事件，上层据此做重连或日志记录。

## 典型调用流程

1. 打开QCMAP服务（qmi_connection_get_service_id等）。
2. QMI_QCMAP_SET_WLAN_CONFIG 配置热点SSID/密码。
3. QMI_QCMAP_SET_DHCP_SERVER_CONFIG 配置地址池。
4. QMI_QCMAP_START_WWAN_CONNECTION 发起LTE拨号，监听QMI_QCMAP_WWAN_STATUS_IND。
5. 拨号成功后下发NAT条目并开启防火墙，客户端连上热点，DHCP分配地址，流量经NAT转发到运营商网络。
