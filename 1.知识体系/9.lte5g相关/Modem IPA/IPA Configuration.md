# 1. 为modem配置IPACM
    IPACM工具自动配置每个活动接口的路由、筛选和网络地址转换规则，包括互联网协议加速器（IPA）硬件中的IPACM客户端。如果IPACM在设备启动期间处于守护程序模式，它将侦听来自Linux内核的多个netlink或conntrack事件，并配置IPA硬件。IPACM配置文件以XML格式定义为IPACM_cfg.xml位于设备上的根/etc文件夹下。

## 1.1. 用IPACM XML文件配置网络属性
    Monitored interfaces
    Private subnets
    ALG ports
    Max IPACM NAT entry

### 1.1.1. 配置接口去监测IPACM
    使用<IPACMIface>指定受监控的接口架构标记。IPACMIface标记包含带有接口配置的子部分。

#### 1.1.1.1. 配置接口条目架构
    使用<Iface>标记指定每个受监控的接口条目模式。Iface标记包含用于指定接口名称和类别的子部分。

#### 1.1.1.2. 配置接口名
    使用<Name>标记指定IPACM监控的接口名称

#### 1.1.1.3. 配置接口类别
    使用＜Category＞标记指定IPACM监控的接口类别
    LAN
    WAN
    WLAN
    UNKNOWN
    VIRTUAL

#### 1.1.1.4. 配置模式(可选)
    使用<Mode>标记指定回程接口（调制解调器或USB支架回程）的操作模式。模式标记仅对WAN接口类别有效。

    如果默认情况下模式标记不存在，则接口设置为路由器模式(Router)。在路由器模式下，设备由调制解调器或外部路由器分配公共IP。

    网桥模式(Bridge)意味着设备没有公共IP；并且它桥接到调制解调器或外部路由器。

#### 1.1.1.5. 配置WlanMode(可选)
    使用<WlanMode>标记指定WLAN接口操作的访问模式类型。WLAN模式标签仅在接口为WLAN时有效。例如，wlan0、wlan1等。如果默认情况下不存在WlanMode标记，则WLAN接口设置为full模式。

##### 1.1.1.5.1. Full 模式
    与处于完全模式的接入点相关联的Wi-Fi客户端可以完全访问调制解调器（公共网络）和其他AP或USB客户端。

##### 1.1.1.5.2. Internet 模式
    与Internet模式上的接入点关联的Wi-Fi客户端仅访问调制解调器（公共网络），但与其他AP/USB客户端的LAN通信被阻止。

    与同一接入点关联的Wi-Fi客户端始终在Internet和full模式下采用IPA-HW路径。

### 1.1.2. 配置IPACM专用子网
    使用<IPACMPrivateSubnet>标记指定IPACM私有子网架构。IPACMPrivateSubnet标记包含带有IPACM中使用的子网配置的子部分。

    MDM9x25、MDM9x35和MDM9x45 MAX中有三组。请在IPACM_Def.h头文件中更新IPA_MAX_PRIVATE_SUBNET_ENTRIES以配置更多子网。

#### 1.1.2.1. IPACM子网条目架构
    使用＜Subnet＞标记指定每个IPACM配置的子网条目架构。子网标记包含用于指定子网地址和子网掩码的子部分。

#### 1.1.2.2. 配置子网地址
    使用SubnetAddress标记来配置ipv4子网地址

#### 1.1.2.3. 配置子网掩码
    使用SubnetMask标记来配置ipv4子网掩码

### 1.1.3. 配置IPACM ALG 端口列表
    使用<IPACMALG>指定IPACM ALG端口架构标记。IPACMALG标记包含带有受监控应用程序级网关（ALG）端口配置的子部分。这些受监控的ALG端口上的流量被重定向到Linux堆栈，而不是IPA硬件。MDM9x25芯片组支持的最大端口数是10。对于MDM9x35和MDM9x45芯片组，最大端口数为20。头文件IPACM_Def.h中更新IPA_MAX_ALG_ENTRIES以配置更多ALG端口

#### 1.1.3.1. 配置ALG条目
    使用＜ALG＞标记指定每个IPACM ALG条目架构。ALG标签包含用于指定ALG端口和协议的子部分。

#### 1.1.3.2. 配置协议
    使用＜Protocol＞标记指定ALG协议
    UDP/TCP

#### 1.1.3.3. 配置端口
    使用＜Port＞标记指定ALG端口

### 1.1.4. 配置IPACM NAT
    使用＜IPACMNAT＞标记指定IPACM NAT架构。IPACMNAT标记包含最大数量的NAT条目。

####  1.1.4.1. 配置MaxNatEntries
    使用<MaxNatEntries>标记指定IPACM可以支持的最大实时NAT条目

### 1.1.5. 使用IPACM XML文件配置ODU属性
    QCMobileAP可以动态修改XML组件条目。IPACM被动应用更改。

    <ODUCFG>是IPACM中的ODU配置数据库模式标记。ODUCFG标记包含具有以下配置的子部分：
    模式
    eMBMS流量IPA卸载

#### 1.1.5.1. 配置子网条目模式
    使用＜Mode＞标记指定每个ODUCFG配置的子网、条目模式。模式标记包含用于指定设备上ODU配置的工作模式的子部分。

##### 1.1.5.1.1. Mode
    使用＜Mode＞标记指定ODU模式
    Router/Bridge

##### 1.1.5.1.2. eMBMS_offload
    使用<eMBMS_offload>标记指定DL eMBMS流量是否采用IPA-HW路径
