# 术语
GSM Global System for Mobile Communications 全球移动通讯系统
TDMA Time division multiple access 时分多址   
CDMA Code Division Multiple Access 码分多址   
FDMA Frequency Division Multiple Access 频分多址   
GPRS General Packet Radio Service 通用封包无线服务   
UTRA Universal Terrestrial Radio Access 通用地面无线电接入   
UTRAN UMTS Terrestrial Radio Access Network UMTS的陆地无线接入网   
GERAN GSM EDGE Radio Access Network GSM/EDGE 无线通讯网络
E-UTRAN Evolved UMTS Terrestrial Radio Access Network 演进的UMTS陆地无线接入网
UMTS Universal Mobile Telecommunications System 通用移动通讯系统   
WCDMA Wideband Code Division Multiple Access宽带码分多址 -->UMTS-FDD、UTRA-FDD   
TD-SCDMA Time Division - Synchronous Code Division Multiple Access 时分-同步码分多址   
EDGE Enhanced Data rates for GSM Evolution GSM增强数据率演进   
HSPA High Speed Packet Access 高速分组接入   
HSDPA High Speed Downlink Packet Access 高速下行分组接入   
HSUPA High Speed Uplink Packet Access 高速上行分组接入   
LTE Long Term Evolution 长期演进技术   
ITU The International Telecommunication Union 国际电信联盟   
FDD Frequency Division Duplexing 频分双工   
TDD Time Division Duplexing 时分双工   
MIMO Multi-input Multi-output 多输入多输出系统(多天线技术)   
HOM 高阶调制   
CPC 持续包连接   
RAN Radio Access Network 无线电接入   
CN Core Network 核心网   
RF Radio frequency 射频   
MBMS Multimedia Broadcast Multicast Service 多媒体广播多播业务   
IMS IP Multimedia Subsystem IP多媒体子系统   
EPC Evolved Packet Core network 分组交换核心网   
eNode B 基站   
UE 用户设备   
RB 无线承载   
RRM Radio Resource Management 无线资源管理   
HARQ Hybrid Automatic Repeat Request 混合自动重传请求   
ARQ Automatic Repeat-reQuest 自动重传请求   
NAS Non-access stratum 非接入层   
RNC Radio Network Controller 无线网络控制器   
OFDM Orthogonal Frequency Division Multiplexing 正交频分复用技术   
OFDMA Orthogonal Frequency Division Multiple Access 正交频分多址   
SC-FDMA Single-carrier Frequency-Division Multiple Access 单载波频分多址   
CA Carrier Aggregation 载波聚合   
TTI Transmission Time Interval 传输时间间隔   
PRB Physical Resource Block 物理资源块   
VBS Virtual Resource Block 虚拟资源块   
QPSK Quadrature Phase Shift Keying 正交相移键控   
QAM Quadrature Amplitude Modulation 正交幅度调制   
SFBC Space Frequency Block Code 空间频率分组码   
FSTD Frequency Switch Transmit Diversity 频率切换发送分集   
PSS Primary Synchronization Signal 主同步信号   
SSS Secondary Synchronization Signal 辅同步信号   
MME Mobility Management Entity 网络节点   
ETWS Earthquake and Tsunami Warning System 地震和海啸预警系统   
EPS Evolved Packet System 演进分组系统   
SCTP Stream Control Transmission Protocol 流控制传输协议   
CS Circuit Switch 电路交换   
PS PacketSwitch 分组交换   
PLMN Public Land Mobile Network 公共陆地移动网   
RSSI Received Signal Strength Indication 接收的信号强度指示   

# MIMO
广义来说，MIMO泛指在发射端和接收端同时采用多天线。   
设计思想：利用无线传输环境的某些特性，通过并行地发射多个数据流来获得较高的数据速率。   

# LTE 简述
LTE 支持1.4MHz 3MHz 5MHz 10MHz 15MHz 20MHz几种带宽分配方式。   
2004年正式提出LTE概念并开始相关研究工作。   
长期演进的目标：降低时延，提高用户数据速率，改善系统容量以及覆盖，降低运营商的成本。   
整个EPS系统由核心网、基站和用户设备三个部分组成。其中，EPC负责核心网部分，EPC信令处理部分称为MME，数据处理部分称为SAE Gateway(S-GW)，eNode B负责接入网部分，也称作E-UTRAN。   
eNB功能：RRM功能，IP头压缩及用户数据流加密，UE附着时的MME选择，寻呼信息的调度传输，广播信息的调度传输以及设置和提供eND的测量。   
MME功能：寻呼消息发送，安全控制，Idle态的移动性管理，SAE承载管理以及NAS信令的加密及完整性保护。   
S-GW功能：数据的路由和传输，用户面数据的加密。   

## 空中接口协议
LTE无线接口协议包括物理层(PHY)，媒体接入控制层(MAC)，无线链路控制层(RLC)，分组数据汇聚协议层(PDCP)，无线资源控制层(RRC)   
PHY:物理层向高层提供数据传输服务   
MAC:媒体接入控制层主要实现与调度和HARQ相关的功能   
RLC:无线链路控制层实现与ARQ相关的功能   
PDCP:分组数据汇聚协议层   
RRC:无线资源控制层   


### 控制面
![控制面](./LTE/%E6%8E%A7%E5%88%B6%E9%9D%A2.jpg)   
![控制平面协议栈](./LTE/%E6%8E%A7%E5%88%B6%E5%B9%B3%E9%9D%A2%E5%8D%8F%E8%AE%AE%E6%A0%88.jpg)   

### 用户面
![用户面](./LTE/%E7%94%A8%E6%88%B7%E9%9D%A2.jpg)   
![用户平面协议栈](./LTE/%E7%94%A8%E6%88%B7%E5%B9%B3%E9%9D%A2%E5%8D%8F%E8%AE%AE%E6%A0%88.jpg)   

## LTE射频
射频信号处理是无线通信系统中的重要组成部分，它是连接射频信号传输媒介和基带信号处理部分的重要接口。   

# 其他无线通信系统
## cdma2000
cdma2000是IS-95基础上发展的蜂窝移动通信标准，后来又成为IMT-2000技术家族的一部分。   

## WiMAX
WiMAX是基于IEEE 802.16 标准家族的宽带无线接入的惯称。   
IEEE 802.16是一个无线城域网(MAN)标准。   

# IMT-Advanced
IMT-Advanced系统为具有超过IMT-2000能力的新的移动系统。   

# LTE-A
LTE-A是LTE-Advanced的简称，是LTE技术的后续演进。   


