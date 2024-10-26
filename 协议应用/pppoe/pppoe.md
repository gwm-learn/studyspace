# PPPoE
## pppoe
PPPOE拨号分为三个阶段，Discovery阶段、Session阶段和Terminate阶段

### Discover阶段
1. PPPOE client广播一个PADI报文，报文中包含PPPOE Client想要得到的服务类型信息
2. 所有PPPOE server收到PADI报文之后，将其中请求的服务与自己能够提供的服务进行比较，如果可以提供，单播回复一个PADO报文
3. 根据网络的拓扑结构，PPPOE client可能收到多个PPPOE server发送的PADO报文，PPPOE client选择最先收到的PADO报文对应的PPPOE server作为自己的PPPOE server，并单播发送PADR报文
4. PPPOE server产生一个唯一的会话ID（session ID），标识和PPPOE client的这个会话，通过发送PADS报文把会话ID发送给PPPOE client，会话建立成功后便进入PPPOE session阶段

### Session阶段
1. LCP阶段主要完成建立、配置、检测数据链路连接
2. LCP协商成功之后开始进行认证，认证协议类型由LCP协商结果决定（CHAP或者PAP）
3. 认证成功之后，PPP进入NCP阶段。NCP是一个协议族，用于配置不同的网络层协议，常用的IP控制协议（IPCP），主要负责协商用户的IP地址和DNS服务器地址

### Terminate阶段
PPP通信双方可以使用PPP协议自身来结束PPPOE会话，当无法使用PPP协议结束会话时可以只用PADT报文，进入PPPOE session阶段后，PPPOE client和PPPOE server都可以通过发送PADT报文的方式结束PPPOE连接，PADT数据包可在会话建立以后的任意时刻单播发送，发送或者接受PADT后，就不允许再使用该会话发送PPP流量

# PPP
[PPP](./../../知识体系/路由与交换/m-2-PPP.md)   