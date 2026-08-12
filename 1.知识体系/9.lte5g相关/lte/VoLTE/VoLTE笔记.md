# VoLTE笔记

VoLTE（Voice over LTE）是把语音业务承载在LTE分组域上的技术。语音以IP包形式通过IMS（IP多媒体子系统）传输，相比CSFB和SVLTE，VoLTE无需回落到2G/3G网络，通话质量更高、接续更快，同时支持与数据业务并发。

## VoLTE架构

VoLTE的端到端架构分为接入网、承载网和核心网三部分：

| 层次 | 网元 | 作用 |
| :--- | :--- | :--- |
| 终端 | UE | 支持VoLTE的终端，内置IMS协议栈 |
| 接入网 | eNodeB | LTE空口接入，承载语音和数据IP包 |
| 核心网 | EPC | 提供LTE承载，包括MME、S-GW、P-GW |
| IMS域 | CSCF、HSS、AS | 会话控制、用户签约、增值业务 |
| 互联域 | MGCF、MGW | 与传统2G/3G电路域互通 |

语音包走用户面：UE → eNodeB → S-GW → P-GW → IMS域；控制信令（SIP）同样经过P-GW到达P-CSCF。LTE只负责提供"管道"，语音业务逻辑全部由IMS负责。

## 关键网元

| 网元 | 全称 | 功能 |
| :--- | :--- | :--- |
| P-CSCF | 代理呼叫会话控制功能 | UE接入IMS的第一个节点，类似代理服务器 |
| I-CSCF | 查询呼叫会话控制功能 | 位于归属网络入口，负责选择S-CSCF |
| S-CSCF | 服务呼叫会话控制功能 | 核心控制节点，负责注册状态和会话路由 |
| HSS | 归属用户服务器 | 保存用户签约数据和位置信息 |
| AS | 应用服务器 | 补充业务，如呼叫前转、彩铃 |
| MGCF/MGW | 媒体网关控制功能/媒体网关 | 与CS域互通的信令与媒体转换 |

## IMS注册流程

UE开机并完成LTE附着（EPS承载建立）后，向IMS发起注册，流程如下：

1. UE构造SIP REGISTER请求，携带Authorization头（含用户标识和密钥），经P-CSCF转发。
2. P-CSCF解析归属网络，转发给I-CSCF；I-CSCF向HSS查询用户签约，确定S-CSCF。
3. S-CSCF向HSS鉴权，返回401 Unauthorized，携带鉴权挑战参数（RAND/AUTN）。
4. UE用USIM卡中的Ki计算响应（RES），再次发送REGISTER（带Authorization）。
5. S-CSCF验证RES通过后，向HSS注册用户，返回200 OK。
6. 注册成功后建立默认IMS信令承载（QCI=5），之后UE可发起或接收呼叫。

注册消息路径：UE → P-CSCF → I-CSCF → S-CSCF，用户漫游时P-CSCF在拜访网络，I/S-CSCF在归属网络。

## SIP信令

VoLTE的呼叫控制使用SIP协议，常见方法如下：

| 方法 | 用途 |
| :--- | :--- |
| REGISTER | 注册和注销 |
| INVITE | 发起会话（呼叫） |
| ACK | 确认最终应答 |
| BYE | 结束会话 |
| CANCEL | 取消未接通的呼叫 |
| UPDATE | 更新会话参数（如媒体变更） |
| PRACK | 临时应答的可靠确认 |

典型主叫流程：主叫UE发INVITE → 网络返回100 Trying → 被叫振铃180 Ringing → 被叫接听200 OK → 主叫回ACK → 通话建立（RTP媒体流）。通话结束任一方发BYE，对方回200 OK释放资源。

## 媒体协商与编码

INVITE的SDP体中携带媒体能力协商：主叫列出支持的编码（AMR-WB、AMR-NB、EVS等）和IP端口，被叫从其中选择。VoLTE语音默认使用AMR-WB（宽带自适应多速率编码，采样率16kHz），相比2G的AMR-NB（8kHz）音质明显提升。

## QoS与承载

| 承载 | QCI | 用途 |
| :--- | :--- | :--- |
| 默认承载 | 9 | 普通上网数据 |
| IMS信令承载 | 5 | SIP信令 |
| 语音专用承载 | 1 | 实时语音，GBR保证带宽 |
| 视频专用承载 | 2 | 实时视频 |

语音专用承载通过P-CSCF发起的AAR（媒体授权）请求触发，由PCRF策略控制P-GW建立GBR承载，保证语音在空口拥塞时仍获得优先调度。

## VoLTE与CSFB对比

| 特性 | VoLTE | CSFB |
| :--- | :--- | :--- |
| 承载网络 | LTE+IMS | 回落2G/3G电路域 |
| 接续时延 | 快（约1~2秒） | 慢（需跨系统重选） |
| 音质 | 高清（AMR-WB） | 普通 |
| 数据并发 | 语音与数据可并行 | 语音期间数据中断 |
| 网络改造 | 需要部署IMS | 需要MSC升级支持SGs接口 |
