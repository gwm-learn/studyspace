# 概要流程
![lte入网概要流程]()

1. 开机上电
2. UE初始化
	2.1. SIM卡识别 -- 3GPP TS 31.102
	2.2. 搜网相关NV
3. PLMN选择
	3.1. 自动搜网
	3.2. 手动搜网
4. 扫频
	4.1. LTE
	4.2. WCDMA
5. 小区搜索
	5.1. LTE
	5.2. WCDMA
6. 解系统消息
7. 小区选择
8. 小区驻留
9. Attach Request
10. RRC CONNECTION request
11. 随机接入
12. RRC CONNECTION setup
13. RRC CONNECTION
14. Attach accept
15. Attach Complete

# UE初始化
略

# PLMN 选择

# 扫频

# 小区搜索
![小区搜索](./%E5%B0%8F%E5%8C%BA%E6%90%9C%E7%B4%A2.png)   

# 解系统消息
LTE系统广播信息(逻辑信道BCCH)分为MIB(Master Infomation Block)和SIB，MIB为系统基本的配置信息，在PBCH固定的物理资源上传输。SIB在DL-SCH上调度传输。   
MIB消息包含天线数、下行带宽、小区ID、注册的频点的消息。   
SIB消息包含PLMN、小区ID、s准则中q-RxLevMin等消息。   

# 小区选择
小区选择的条件：   
1. 小区所在PLMN需满足以下条件之一
	所选的PLMN
	注册的PLMN
	等效PLMN
2. 小区没有被禁止
3. 小区至少属于一个不被禁止漫游的跟踪区
4. 小区满足S准则，即小区搜索中的接收功率Srxlev>0dB且小区搜索中接收的信号质量Squal>0dB

# 小区驻留
扫到的一个频点满足S准则，小区选择成功后进行小区驻留。   

# Attach request

# RRC CONNECTION Request

# 随机接入

# RRC 连接建立

# Attach accept / Attach complete



