# UIM
User Identity Module   

通过QMI_UIM，QMI客户端可以执行操作和接入SIM卡。   
UIM模块用于访问设备上可用的卡。该实现支持用于GSM/WCDMA设备的SIM卡和USIM卡，以及用于CDMA设备的RUIM卡和CSIM卡   

•访问卡上的文件   
1. 读取透明和记录
2. 书写透明和记录
3. 获取文件属性

•PIN操作   
1. 启用和禁用
2. 验证
3. 解除阻止
4. 更改

•其他任务   
1. 支持文件刷新操作
2. 支持个性化
3. 打开/关闭卡的电源
4. 身份验证
5. 从卡中选择资源调配应用程序
6. 获取调制解调器配置
7. 将原始APDU发送到卡
8. SIM卡接入模式

## 服务类型
UIM 0x0B   

## 消息
1. QMI_UIM_RESET 重置服务保留的颁发控制点的状态   
2. QMI_UIM_GET_SUPPORTED_MSGS 查询当前运行的软件实现的消息集   
3. QMI_UIM_GET_SUPPORTED_FIELDS 查询由当前运行的软件实现的单个命令支持的字段   
4. QMI_UIM_READ_TRANSPARENT 提供对卡中任何透明文件的读取访问，并提供通过路径的访问   
5. QMI_UIM_READ_TRANSPARENT_IND 指示读取透明命令的结果   
6. QMI_UIM_READ_RECORD 提供对卡中线性固定或循环文件中特定记录的读取访问，并通过路径提供访问   
7. QMI_UIM_READ_RECORD_IND 指示读取记录命令的结果   
8. QMI_UIM_WRITE_TRANSPARENT 提供对卡中任何透明文件的写入访问，并提供通过路径的访问   
9. QMI_UIM_WRITE_TRANSPARENT_IND 指示写入透明文件的结果   
10. QMI_UIM_WRITE_RECORD 提供对卡中线性固定或循环文件中特定记录的写入访问，并通过路径提供访问   
11. QMI_UIM_WRITE_RECORD_IND 指示写入记录命令的结果   
12. QMI_UIM_GET_FILE_ATTRIBUTES 检索卡中任何EF或DF的文件属性，并提供路径访问   
13. QMI_UIM_GET_FILE_ATTRIBUTES_IND 指示获取文件属性命令的结果   
14. QMI_UIM_SET_PIN_PROTECTION 启用或禁用特定PIN对UIM内容的保护   
15. QMI_UIM_SET_PIN_PROTECTION_IND 指示设置PIN保护命令的结果   
16. QMI_UIM_VERIFY_PIN 在卡内容接入前验证PIN   
17. QMI_UIM_VERIFY_PIN_IND 指示验证PIN命令的结果   
18. QMI_UIM_UNBLOCK_PIN 使用PUK码解锁一个锁定PIN   
19. QMI_UIM_UNBLOCK_PIN_IND 指示解锁PIN命令的结果   
20. QMI_UIM_CHANGE_PIN 修改指定PIN值   
21. QMI_UIM_CHANGE_PIN_IND 指示修改PIN命令结果   
22. QMI_UIM_DEPERSONALIZATION 停用或取消阻止手机上的个性化设置   
23. QMI_UIM_REFRESH_REGISTER 注册卡触发的文件更改通知   
24. QMI_UIM_REFRESH_OK 允许客户端指示是否可以启动刷新过程   
25. QMI_UIM_REFRESH_COMPLETE 客户端完成刷新过程时调用   
26. QMI_UIM_REFRESH_GET_LAST_EVENT 提供检索上次刷新事件的功能   
27. QMI_UIM_EVENT_REG 从卡注册事件通知   
28. QMI_UIM_GET_CARD_STATUS 检索当前卡状态   
29. QMI_UIM_POWER_DOWN 关闭卡   
30. QMI_UIM_POWER_UP 开启卡   
31. QMI_UIM_STATUS_CHANGE_IND 指示卡状态已经改变   
32. QMI_UIM_REFRESH_IND 接收来自卡的刷新命令   
33. QMI_UIM_AUTHENTICATE 在卡上执行身份验证算法   
34. QMI_UIM_AUTHENTICATE_IND 指示身份验证结果   
35. QMI_UIM_CLOSE_SESSION 强制关闭与非配置应用程序的会话   
36. QMI_UIM_GET_SERVICE_STATUS 检索卡上服务的状态   
37. QMI_UIM_SET_SERVICE_STATUS 改变卡上服务的状态   
38. QMI_UIM_CHANGE_PROVISIONING_SESSION 更换设置会话   
39. QMI_UIM_GET_LABEL 获取UICC卡上应用程序的标签   
40. QMI_UIM_GET_CONFIGURATION 获取UIM模块的调制解调器配置   
41. QMI_UIM_SEND_APDU 发送APDU到卡   
42. QMI_UIM_SEND_APDU_IND 指示发送APDU命令的结果   
43. QMI_UIM_SAP_CONNECTION 作为SAP客户端建立并释放与UIM模块的连接   
44. QMI_UIM_SAP_REQUEST 执行SAP请求   
45. QMI_UIM_SAP_CONNECTION_IND 指示SAP连接状态的改变   
46. QMI_UIM_LOGICAL_CHANNEL 在UICC卡上打开或关闭逻辑信道到应用   
47. QMI_UIM_SUBSCRIPTION_OK 允许客户端指示是否可以继续发布订阅   
48. QMI_UIM_GET_ATR 检索特定卡的重置结果   
49. QMI_UIM_OPEN_LOGICAL_CHANNEL 在UICC卡上打开逻辑信道   
50. QMI_UIM_SESSION_CLOSED_IND 表示调制解调器关闭了设置会话或非设置会话   
51. QMI_UIM_REFRESH_REGISTER_ALL 为所有文件注册卡触发的文件更改通知   
52. QMI_UIM_SET_FILE_STATUS 更改卡上文件的激活状态   
53. QMI_UIM_SWITCH_SLOT 在逻辑插槽和物理插槽之间切换绑定   
54. QMI_UIM_GET_SLOTS_STATUS 检索物理和逻辑插槽的当前状态   
55. QMI_UIM_SLOT_STATUS_CHANGE_IND 指示物理插槽的状态已更改   
56. QMI_UIM_READ_TRANSPARENT_LONG_IND 具有读取透明命令结果的指示   
57. QMI_UIM_SIM_BUSY_STATUS_IND 指示SIM卡的忙状态   
58. QMI_UIM_GET_PLMN_NAME_TABLE_INFO 检索modem使用的plmn表信息   
59. QMI_UIM_PERSONALIZATION 激活和设置手机上的个性化设置数据   
60. QMI_UIM_INCREASE 对卡上的任何文件执行增加操作，并通过路径提供访问   
61. QMI_UIM_INCREASE_IND 指示具有增加确认的客户端   
62. QMI_UIM_RECOVERY 在标识的插槽上执行恢复   
63. QMI_UIM_RESELECT 在指定的逻辑通道上执行应用程序选择   
64. QMI_UIM_RECOVERY_IND 表示SIM卡恢复已成功完成   
65. QMI_UIM_SEND_STATUS 发送状态命令   
66. QMI_UIM_GET_SIM_PROFILE 从卡中查询配置文件信息   
67. QMI_UIM_SET_SIM_PROFILE 切换SIM卡上的配置文件   
68. QMI_UIM_SUPPLY_VOLTAGE 通知调制解调器继续电源电压Vcc停用   
69. QMI_UIM_SUPPLY_VOLTAGE_IND 指示调制解调器必须停用或激活UICC的电源电压Vcc线路   
70. QMI_UIM_CARD_ACTIVATION_STATUS_IND 指示卡激活状态   
71. QMI_UIM_DEPERSONALIZATION_SECURE 执行临时解锁或不使用控制钥匙解锁   
72. QMI_UIM_PERSONALIZATION_SECURE 激活设备并将其锁定为simlock代码   
73. QMI_UIM_EMERGENCY_ONLY 将手机设置为仅紧急模式   
74. QMI_UIM_SIMLOCK_CONFIGURATION 提供对simlock配置数据的访问   
75. QMI_UIM_SIMLOCK_CONFIGURATION_IND simlock配置数据访问请求指示   
76. QMI_UIM_GBA 初始化GBA并生成与NAF共享的密钥   
77. QMI_UIM_GBA_IND 包含状态和共享密钥相关信息的延迟响应指示   
78. QMI_UIM_GET_GBA_IMPI 检索用于会话对应的GBA的IMPI   
79. QMI_UIM_SEARCH_RECORD 提供使用给定模式搜索卡中记录的访问权限   
80. QMI_UIM_SEARCH_RECORD_IND 指示搜索记录命令的结果   
81. QMI_UIM_REMOTE_UNLOCK 执行与远程解锁相关的操作   
82. QMI_UIM_VERIFY_IMSI 验证simlock是否验证与IMSI相关的订阅   
83. QMI_UIM_TEMPORARY_UNLOCK_STATUS_IND 指示调制解调器的临时解锁状态   


