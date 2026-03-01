jiuqu@192 7.openwrt % tree
.
├── 1.配置
│   └── uci.md
├── 2.应用管理平面
│   ├── 启动流程.md
│   └── 热插拔.md
├── 3.网络业务
│   ├── 1.基础网络模型
│   │   ├── 1.WAN
│   │   │   ├── 1.PPPoE.md
│   │   │   ├── 2.DHCP Client.md
│   │   │   ├── 3.5G拨号.md
│   │   │   └── 4.StaticIP.md
│   │   ├── 2.LAN
│   │   │   ├── 1.DHCP Server.md
│   │   │   ├── 2.网桥.md
│   │   │   └── 3.网段.md
│   │   └── 3.WIFI
│   │       ├── 1.AP.md
│   │       ├── 2.Sta.md
│   │       └── 3.Mesh.md
│   └── 2.关键组件服务
│       ├── 1.网络管理
│       │   ├── 1.netifd.md
│       │   ├── 2.bridge.md
│       │   └── 3.vlan.md
│       ├── 2.拨号接入
│       │   ├── 1.PPPoE.md
│       │   ├── 2.5G.md
│       │   └── 3.路由.md
│       ├── 3.NAT&防火墙
│       │   ├── 1.iptables&nftables.md
│       │   ├── 2.NAT源地址转换.md
│       │   ├── 3.端口映射.md
│       │   └── 4.DMZ.md
│       ├── 4.DHCP&DNS
│       │   ├── 1.dnsmasq
│       │   │   ├── dnsmasq_script.sh
│       │   │   └── dnsmasq.md
│       │   └── 2.静态地址绑定&DNS转发
│       ├── 5.WiFi子系统
│       │   ├── 1.cfg80211.md
│       │   ├── 2.nl80211.md
│       │   └── 2.wifi相关概念.md
│       └── 6.CPE专属业务
│           ├── 1.TR069&TR369.md
│           ├── 2.Qos.md
│           ├── 3.流量控制.md
│           └── 4.端口限速.md
├── 4.内核网络
│   ├── 1.linux网络栈
│   │   ├── 1.收发包流程.md
│   │   └── 2.网络命名空间.md
│   └── 2.内核关键模块
│       ├── 1.Netfilter框架.md
│       ├── 2.网桥模块.md
│       ├── 3.PPP协议栈.md
│       └── 4.USB子系统.md
├── 5.驱动
│   ├── 1.以太网switch驱动
│   ├── 2.wifi驱动
│   ├── 3.5G模块
│   └── 驱动通用知识点
└── 6.其他
    ├── procd.md
    ├── ubus.md
    └── 故障排查.md

