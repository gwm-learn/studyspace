1. 简介
OpenWrt提供了一个系统总线ubus，它类似于Linux桌面操作系统的d-Bus，目标是提供系统级的进程间通信（IPC）功能。ubus在设计理念上与d-Bus基本保持一致，但减少了系统内存占用空间，这样可以适应于嵌入式linux操作系统的低内存和cpu性能的特殊环境。

ubus模块的核心是ubusd守护进程，作为sever端在系统启动时由初始化进程procd初始化。它提供了其他守护进程将自己注册以及发送消息的接口（libubus或ubus命令行），接口通过使用unix socket来实现，并使用TLV(type-length-value)消息，ubus内部使用Blob_buf,Blob_attr等结构来表示，length最大为65536字节，所以ubus不适合传输大量数据（进程间传输大量数据应采用共享内存）。

2. 组成
ubusd守护进程
ubus接口库（libubus.so）
ubus命令行工具

3. 基础
ubus实现的基础是unix socket，即本地socket，它相对于网络通信的inet socket更高效，更具可靠性。unix socket采用C/S模型，客户端和服务端的实现方式和网络socket类似。实现一个简单的socket server服务器和客户端需要做如下工作：

1）建立一个socket server端，绑定到一个本地的socket文件，并监听clients的连接。
2）建立一个或多个socket client端，连接server。
3）client和server相互发送消息。
4）client或server收到对方消息后，针对消息进行相应处理。

ubus同样实现了上述组件，并对socket连接及消息传输和处理进行了封装：

1）ubus提供了一个socket server：ubusd守护进程。因此开发者不需要自己实现server端。
2）ubus提供了创建socket client端的接口，并且支持三种方式（c语言、lua脚本、shell脚本）。
3）ubus对client和server之间通信的消息格式进行了定义：client和server都必须将消息封装成json格式。
4）ubus对client端的消息处理抽象出“对象（object）”、“方法（method）”的概念。一个对象中包含多个方法，client需要向server注册收到特定json消息时的处理方法，对象和方法都有自己的名字，发送请求方只需在消息中指定要调用的对象和方法的名字即可。

4. 命令行
ubus list 该指令可以查看所有注册在ubus中的对象
ubus -v list system 该指令可以查看注册对象的所有注册方法
ubus call 该指令可以调用注册对象的某个方法
