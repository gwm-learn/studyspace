# WDS
Wireless Data Service   

QMI_WDS提供一个与无线移动站接口的命令集，提供IP连接和相关增值服务。QMI_WDS为在主机PC上运行的以下应用程序提供与无线无线网络上的IP数据服务相关的命令：   
1. 数据呼叫设置和挂断   
2. 网络注册和连接   
3. 数据包传输统计   
4. 数据承载率   
5. 数据会话配置文件管理   

除非为DUN明确指定，否则所有与数据会话相关的消息仅适用于RmNet。   

## 服务类型
WDS 0x01   

## 每个控制点的状态变量
1. report_channel_rate   
2. pkt_stats_report_period   
3. pkt_stats_report_mask   
4. report_data_bearer_tech   
5. report_dormancy_status   
6. report_mip_status   
7. report_current_data_ bearer_tech   
8. report_evdo_page_monitor_period_change   
9. report_data_call_status   
10. report_preferred_data_system   
11. report_data_system_status   
12. report_data_bearer_tech_ex   
13. report_embms_tmgi_list   
14. suppress_pkt_srvc_ind   
15. report_extended_ip_config_change   
16. report_lte_attach_pdn_list_change   
17. report_reverse_ip_transport_filter_setup   
18. report_handoff_information   

## 消息
1. QMI_WDS_RESET 重置请求控制点的WDS服务状态变量   
2. QMI_WDS_SET_EVENT_REPORT 设置请求控制点的无线数据连接状态报告条件   
3. QMI_WDS_SET_EVENT_REPORT_IND 指明与WDS连接相关的状态更改   
4. QMI_WDS_ABORT 终止之前发出的QMI_WDS命令   
5. QMI_WDS_INDICATION_REGISTER 为请求控制点的不同QMI_WDS指示设置注册状态   
6. QMI_WDS_GET_SUPPORTED_MSGS 查询当前运行的软件实现的消息集   
7. QMI_WDS_GET_SUPPORTED_FIELDS 查询由当前运行的软件实现的单个命令支持的字段   
8. QMI_WDS_START_NETWORK_INTERFACE 代表请求的控制点激活分组数据会话（如果尚未启动）   
9. QMI_WDS_STOP_NETWORK_INTERFACE 代表请求的控制点停用分组数据会话（除非由其他控制点使用）   
10. QMI_WDS_GET_PKT_SRVC_STATUS 查询当前数据包数据连接状态   
11. QMI_WDS_GET_PKT_SRVC_STATUS_IND 指示当前数据包数据连接状态的更改   
12. QMI_WDS_GET_CURRENT_CHANNEL_RATE 查询数据包数据连接的当前比特率   
13. QMI_WDS_GET_PKT_STATISTICS 从当前分组数据会话开始查询分组数据传输统计信息   
14. QMI_WDS_GO_DORMANT 强制设备立即丢弃服务无线电接口上的通信信道   
15. QMI_WDS_GO_ACTIVE 强制设备立即重建服务无线电接口上的通信信道   
16. QMI_WDS_CREATE_PROFILE 使用指定的设置创建已配置的配置文件   
17. QMI_WDS_MODIFY_PROFILE_SETTINGS 改变配置文件中的设置   
18. QMI_WDS_DELETE_PROFILE 删除配置文件   
19. QMI_WDS_GET_PROFILE_LIST 在无线设备上检索配置文件的列表   
20. QMI_WDS_GET_PROFILE_SETTINGS 检索配置文件中的配置   
21. QMI_WDS_GET_DEFAULT_SETTINGS 检索默认的数据会话设置   
22. QMI_WDS_GET_RUNTIME_SETTINGS 检索当前正在使用的包数据会话设置   
23. QMI_WDS_SET_MIP_MODE 为设备设置当前移动IP模式设置   
24. QMI_WDS_GET_MIP_MODE 检索为设备提供的移动IP模式设置   
25. QMI_WDS_GET_DORMANCY_STATUS 查询当前流量通道状态   
26. QMI_WDS_GET_AUTOCONNECT_SETTING 查询自动连接设置   
27. QMI_WDS_GET_CALL_DURATION 查询当前呼叫持续时间   
28. QMI_WDS_GET_DATA_BEARER_TECHNOLOGY 查询当前的数据承载技术   
29. QMI_WDS_GET_DUN_CALL_INFO 查询当前modem里拦截状态   
30. QMI_WDS_DUN_CALL_INFO_IND 指明DUN数据连接状态的变化   
31. QMI_WDS_GET_ACTIVE_MIP_PROFILE 从设备查询当前移动IP模式配置文件索引   
32. QMI_WDS_SET_ACTIVE_MIP_PROFILE 设置设备当前移动IP模式配置文件   
33. QMI_WDS_READ_MIP_PROFILE 从设备查询移动IP配置文件   
34. QMI_WDS_MODIFY_MIP_PROFILE 在设备上修改移动IP配置文件   
35. QMI_WDS_GET_MIP_SETTINGS 从设备查询移动IP设置   
36. QMI_WDS_SET_MIP_SETTINGS 在设备上设置当前移动IP设置   
37. QMI_WDS_GET_LAST_MIP_STATUS 从设备上查询最近移动IP状态   
38. QMI_WDS_GET_CURRENT_DATA_BEARER_TECHNOLOGY 查询当前数据承载技术   
39. QMI_WDS_CALL_HISTORY_LIST 从设备上查询呼叫历史记录列表   
40. QMI_WDS_CALL_HISTORY_READ 从设备上查询一条呼叫历史记录   
41. QMI_WDS_CALL_HISTORY_DELETE 从设备上清空所有呼叫历史记录   
42. QMI_WDS_CALL_HISTORY_MAX_SIZE 查询设备呼叫历史记录最大存储数量   
43. QMI_WDS_GET_DEFAULT_PROFILE_NUM 检索无线设备上为指定技术配置的默认配置文件号   
44. QMI_WDS_SET_DEFAULT_PROFILE_NUM 设置无线设备上为指定技术配置的默认配置文件号   
45. QMI_WDS_RESET_PROFILE_TO_DEFAULT 将指定配置文件和技术的所有参数重置为默认值   
46. QMI_WDS_RESET_PROFILE_PARAM_TO_INVALID 将指定技术的指定配置文件参数类型重置为无效   
47. QMI_WDS_SET_CLIENT_IP_FAMILY_PREF 设置控制点IP首选项   
48. QMI_WDS_FMC_SET_TUNNEL_PARAMS 设置FMC的通道参数   
49. QMI_WDS_FMC_CLEAR_TUNNEL_PARAMS 清除FMC的通道参数   
50. QMI_WDS_FMC_GET_TUNNEL_PARAMS 从设备上查询FMC通道参数   
51. QMI_WDS_SET_AUTOCONNECT_SETTINGS 设置自动连接设置   
52. QMI_WDS_GET_DNS_SETTINGS 查询设备的当前DNS设置   
53. QMI_WDS_SET_DNS_SETTINGS 设置设备的当前DNS设置   
54. QMI_WDS_GET_PRE_DORMANCY_CDMA_SETTINGS 在休眠之前检索数据包数据会话信息   
55. QMI_WDS_SET_CAM_TIMER 设置聊天应用程序管理器计时器值   
56. QMI_WDS_GET_CAM_TIMER 查询聊天应用程序管理器计时器值   
57. QMI_WDS_SET_SCRM 禁用/启用补充通道请求消息（SCRM）   
58. QMI_WDS_GET_SCRM 检索补充通道请求消息无论是否开启或关闭   
59. QMI_WDS_SET_RDUD 启用或禁用减少休眠，然后是未经请求的数据   
60. QMI_WDS_GET_RDUD 检索是否启用或禁用了先减少休眠，然后是未经请求的数据   
61. QMI_WDS_GET_SIP_MIP_CALL_TYPE 查询SIP/MIP呼叫类型   
62. QMI_WDS_SET_EVDO_PAGE_MONITOR_PERIOD 设置EV-DO插槽周期索引   
63. QMI_WDS_EVDO_PAGE_MONITOR_PERIOD_RESULT_IND 指示尝试更改EV-DO插槽周期的结果   
64. QMI_WDS_SET_EVDO_FORCE_LONG_SLEEP 启用或禁用EV-DO强制长时间睡眠功能   
65. QMI_WDS_GET_EVDO_PAGE_MONITOR_PERIOD 检索有关EV-DO页面监视周期的详细信息   
66. QMI_WDS_GET_CALL_THROTTLE_INFO 查询系统是否已限制调用并返回剩余的限制延迟   
67. QMI_WDS_GET_NSAPI 根据接入点名称检索网络服务接入点标识符（NSAPI）   
68. QMI_WDS_SET_DUN_CTRL_PREF 设置控制点的首选项以控制调制解调器接收的拨号网络（DUN）呼叫请求   
69. QMI_WDS_GET_DUN_CTRL_INFO 查询在modem上的DUN呼叫状态   
70. QMI_WDS_SET_DUN_CTRL_EVENT_REPORT 设置控制点的DUN控制事件报告首选项   
71. QMI_WDS_DUN_CTRL_EVENT_REPORT_IND 指示与调制解调器上挂起的DUN呼叫请求相关的事件   
72. QMI_WDS_CONTROL_PENDING_DUN_CALL 允许或不允许挂起的DUN调用请求   
73. QMI_WDS_EMBMS_TMGI_ACTIVATE 激活eMBMS临时移动组标识（TMGI）   
74. QMI_WDS_EMBMS_TMGI_ACTIVATE_IND 指示TMGI激活请求的结果   
75. QMI_WDS_EMBMS_TMGI_DEACTIVATE 停用eMBMS TMGI   
76. QMI_WDS_EMBMS_TMGI_DEACTIVATE_IND 指示TMGI停用请求的结果   
77. QMI_WDS_EMBMS_TMGI_LIST_QUERY 查询TMGI表   
78. QMI_WDS_EMBMS_TMGI_LIST_IND 指明当前活动的或有效的TMGI列表   
79. QMI_WDS_GET_PREFERRED_DATA_SYSTEM 查询首选数据系统   
80. QMI_WDS_GET_LAST_DATA_CALL_STATUS 查询最近数据呼叫状态   
81. QMI_WDS_GET_CURRENT_DATA_SYSTEM_STATUS 查询当前数据系统状态   
82. QMI_WDS_GET_PDN_THROTTLE_INFO 查询PDN油门信息   
83. QMI_WDS_GET_LTE_ATTACH_PARAMS LTE附加PDN参数查询   
84. QMI_WDS_RESET_PKT_STATISTICS 重置数据包数据传输统计信息   
85. QMI_WDS_GET_FLOW_CONTROL_STATUS 查询当前数据呼叫流控制状态   
86. QMI_WDS_EMBMS_TMGI_ACT_DEACT 启用或停用TMGIs   
87. QMI_WDS_EMBMS_TMGI_ACT_DEACT_IND 指明TMGI启用或停用请求   
88. QMI_WDS_BIND_DATA_PORT 绑定一个控制点到一个SIO数据端口   
89. QMI_WDS_SET_ADDITIONAL_PDN_FILTER 将筛选器设置为允许在同一数据端口上共享多个PDN   
90. QMI_WDS_REMOVE_ADDITIONAL_PDN_FILTER 删除设置为允许在单个端口上共享其他PDN的筛选器   
91. QMI_WDS_EXTENDED_IP_CONFIG_IND 指明数据会话的任意IP配置的改变   
92. QMI_WDS_REVERSE_IP_TRANSPORT_CONNECTION_IND_REGISTRATION 反向IP传输连接相关指示的注册机制   
93. QMI_WDS_REVERSE_IP_TRANSPORT_CONNECTION_IND 指示当前反向IP传输连接状态的更改   
94. QMI_WDS_GET_IPSEC_STATIC_SA_CONFIG 检索ePDG调用的IPSec静态安全关联（SA）   
95. QMI_WDS_REVERSE_IP_TRANSPORT_CONFIG_COMPLETE 发送应用处理器（AP）端反向IP传输配置完成的通知   
96. QMI_WDS_GET_DATA_BEARER_TECHNOLOGY_EX 查询数据承载技术   
97. QMI_WDS_GET_LTE_MAX_ATTACH_PDN_NUM 查询附加FDN支持最大数   
98. QMI_WDS_SET_LTE_ATTACH_PDN_LIST 设置LTE附加PDN列表   
99. QMI_WDS_GET_LTE_ATTACH_PDN_LIST 查询附加PDN列表   
100. QMI_WDS_LTE_ATTACH_PDN_LIST_IND 指明LTE附加PDN列表中的改变   
101. QMI_WDS_SET_LTE_DATA_RETRY 启用或禁用重试LTE数据连接   
102. QMI_WDS_GET_LTE_DATA_RETRY 检索当前LTE重试设置   
103. QMI_WDS_SET_LTE_ATTACH_TYPE 设置要执行的附加是初始还是切换   
104. QMI_WDS_GET_LTE_ATTACH_TYPE 检索当前LTE附加类型   
105. QMI_WDS_REVERSE_IP_TRANSPORT_FILTER_SETUP_IND 指示必须设置反向IP传输筛选器   
106. QMI_WDS_HANDOFF_INFORMATION_IND 表示正在进行或已完成切换   
107. QMI_WDS_SET_DATA_PATH 设置客户端数据路径   
108. QMI_WDS_GET_DATA_PATH 查询当前modem数据路径   
109. QMI_WDS_UPDATE_LTE_ATTACH_PDN_LIST_PROFILES 触发modem去更新配置文件参数   
110. QMI_WDS_EMBMS_SAI_LIST_QUERY 查询服务区标识SAI列表   
111. QMI_WDS_EMBMS_SAI_LIST_IND 指明当前有效的SAI列表   
112. QMI_WDS_BIND_MUX_DATA_PORT 将控制点绑定到多路复用数据端口   
113. QMI_WDS_SET_THROUGHPUT_INFO_IND_FREQ 设置用于生成吞吐量信息指示的计时器   
114. QMI_WDS_GET_LAST_THROUGHPUT_INFO 查询上次报告的吞吐量信息   
115. QMI_WDS_THROUGHPUT_INFO_IND 指明吞吐量信息   
116. QMI_WDS_INITIATE_ESP_REKEY 启动ESP重新密钥   
117. QMI_WDS_CONFIGURE_PROFILE_EVENT_LIST 注册配置文件更改事件   
118. QMI_WDS_PROFILE_CHANGED_IND 指示为报告更改事件而配置的配置文件中的更改   
119. QMI_WDS_GET_CAPABILITIES 查询调制解调器功能   
120. QMI_WDS_GET_ROAMING_INFO 在漫游期间检索APN名称   
121. QMI_WDS_ROAMING_INFO_IND 漫游期间指明APN名字   
122. QMI_WDS_GET_DELEGATED_IPV6_PREFIX 查询委派的IPv6前缀   
123. QMI_WDS_REMOVE_DELEGATED_IPV6_PREFIX 删除委派的IPv6前缀   
124. QMI_WDS_ABORT_GO_DORMANT 终止前面的QMI_WDS_GO_DORMANT命令   
125. QMI_WDS_BIND_SUBSCRIPTION 将控制点绑定到指定的订阅   
126. QMI_WDS_GET_BIND_SUBSCRIPTION 查询控制点的当前订阅   
127. QMI_WDS_SET_LTE_DATA_CALL_TYPE 为活动的LTE呼叫设置数据呼叫类型   
128. QMI_WDS_SET_DOWNLINK_THROUGHPUT_INFO_IND_FREQ 设置用于生成QMI_WDS_DOWNLINK_THROUGHPUT_INFO_IND指示的计时器   
129. QMI_WDS_DOWNLINK_THROUGHPUT_INFO_IND 指明下行链路吞吐量信息   
130. QMI_WDS_GET_DOWNLINK_THROUGHPUT_INFO_PARAMS 查询下行链路吞吐量信息参数   
131. QMI_WDS_EMBMS_CONTENT_DESC_UPDATE 更新eMBMS内容描述参数   
132. QMI_WDS_EMBMS_CONTENT_DESC_CONTROL_IND 指明eMBMS内容描述参数   
133. QMI_WDS_POLICY_REFRESH 刷新指定的策略   
134. QMI_WDS_POLICY_REFRESH_RESULT_IND 指示尝试刷新策略的结果   
135. QMI_WDS_POLICY_READY_IND 指明策略文件已经准备好了   
136. QMI_WDS_APN_PARAM_INFO_CHANGE_IND 在活动的数据呼叫指明旧的和新的APN参数   
137. QMI_WDS_SET_SILENT_REDIAL 通知调制解调器执行无声重拨   
138. QMI_WDS_REFRESH_DHCP_CONFIG_INFO 刷新DHCP配置信息   
139. QMI_WDS_SET_INTERNAL_RUNTIME_SETTINGS 设置/修改内部分组数据会话设置   
140. QMI_WDS_GET_INTERNAL_RUNTIME_SETTINGS 检索当前正在使用的内部数据包数据会话设置   
141. QMI_WDS_INTERNAL_IFACE_EV_REGISTER 注册IFACE事件   
142. QMI_WDS_INTERNAL_IFACE_EV_IND 指示IFACE事件的发生   

