# netifd 完整调用链流程

本文档详细描述 netifd 从启动到运行的完整函数调用链，遵循 **"一次性初始化 + 终身事件驱动"** 架构。

## 第一阶段：构造函数初始化（main() 函数之前）

编译器自动执行标记为 `__attribute__((constructor))` 的函数。

```
程序启动
    ├── utils.h:50 | #define __init __attribute__((constructor))  [宏定义]
    │
    ├── interface.c:1417 | static void __init interface_init_list(void)
    │   └── 初始化 interfaces vlist 树，设置 interface_update 回调
    │
    ├── device.c:547 | static void __init dev_init(void)
    │   └── avl_init(&devices, device_avl_cmp, false, NULL)  [初始化设备 AVL 树]
    │
    ├── bridge.c:1424 | static void __init bridge_device_type_init(void)
    │   └── add_device_type(&bridge_device_type)  [注册桥设备类型]
    │
    ├── macvlan.c:266 | static void __init macvlan_device_type_init(void)
    │   └── add_device_type(&macvlan_device_type)  [注册 MACVLAN 设备类型]
    │
    └── proto-static.c:109 | static void __init static_proto_init(void)
        └── add_proto_handler(&proto)  [注册静态 IP 协议处理器]
```

## 第二阶段：main() 函数初始化

```
main.c:279 | int main(int argc, char **argv)
    ├── main.c:286-319 | 命令行参数解析
    ├── main.c:321-322 | openlog("netifd", 0, LOG_DAEMON)  [初始化系统日志]
    ├── main.c:324 | netifd_setup_signals()  [设置信号处理器]
    ├── main.c:325 | netifd_ubus_init(socket)
    │   ├── ubus.c:1365 | uloop_init()  [初始化 uloop 事件循环]
    │   ├── ubus.c:1370 | ubus_connect()  [连接 ubus 守护进程]
    │   └── ubus.c:1380 | netifd_ubus_add_fd()  [将 ubus socket 加入 uloop]
    │
    ├── main.c:330 | proto_shell_init()  [扫描 /lib/netifd/proto/ 注册脚本协议]
    ├── main.c:331 | extdev_init()  [初始化外部设备支持]
    ├── main.c:332 | wireless_init()  [初始化无线子系统]
    ├── main.c:334 | system_init()
    │   └── system-linux.c:330 | 创建 netlink socket，注册回调
    │
    └── main.c:339 | config_init_all()  [核心：加载并解析所有配置]
```

## 第三阶段：配置加载与接口创建（只执行一次）

```
config.c:744 | void config_init_all(void)
    ├── config.c:350 | config_parse_interface()
    │   ├── interface.c:814 | interface_alloc()  [创建接口结构体]
    │   │   ├── 解析 name、proto、ifname、ipaddr 等属性
    │   │   └── iface->state = IFS_DOWN  [初始状态设为 DOWN]
    │   │
    │   ├── interface.c:991 | interface_add()
    │   │   ├── vlist_add(&interfaces, &iface->node)  [加入全局接口树]
    │   │   └── 触发 interface_update() 回调
    │   │
    │   ├── proto.c:636 | proto_attach_interface()
    │   │   └── get_proto_handler()  [在 handlers AVL 树中查找协议]
    │   │
    │   └── interface.c:640 | interface_claim_device()
    │       ├── device_get()  [查找或创建设备]
    │       └── interface_set_main_dev()  [设置接口主设备]
    │
    ├── config.c:400 | config_parse_device()
    │   └── device.c:320 | device_create()  [创建设备结构体]
    │
    └── config.c:600 | config_init_wireless()  [解析无线配置]
```

## 第四阶段：事件循环启动

```
main.c:341 | uloop_run()  [永不返回，除非收到终止信号]
    ├── 监听事件源：
    │   ├── ubus socket (RPC 命令)
    │   ├── netlink socket (内核事件)
    │   ├── 进程管道 (脚本退出)
    │   └── 定时器 (超时/重试)
    │
    └── 事件触发 → 调用对应回调函数
```

## 第五阶段：接口 UP 事件处理链（事件驱动）

### 5.1 UP 命令触发
```
用户命令: ubus call network.interface.lan up
    ↓
ubus socket 可读 → uloop 调用 ubus 回调
    ↓
ubus.c:1100 | netifd_handle_up()
    ↓
interface.c:1123 | interface_set_up(struct interface *iface)
```

### 5.2 UP 核心处理链
```
interface_set_up(struct interface *iface)
    ├── interface.c:1128 | interface_clear_errors()
    ├── device.c:600 | device_claim()  [确保设备可用]
    └── interface.c:351 | __interface_set_up()
        ├── iface->state = IFS_SETUP
        └── proto.c:656 | interface_proto_event(PROTO_CMD_SETUP)
            ↓
            proto-shell.c:205 | proto_shell_handler()
                ├── 构建脚本参数：script, proto, "setup", iface, config, dev
                └── main.c:138 | netifd_start_process()  [执行外部脚本]
                    ├── fork() + execvp()
                    └── 注册进程回调 netifd_process_cb()
```

### 5.3 脚本完成后的回调链
```
main.c:100 | netifd_process_cb()
    ↓
proto-shell.c:720 | proto_shell_script_cb()
    ↓
proto-shell.c:740 | proto_shell_task_finish()
    ↓
interface.c:444 | interface_proto_event_cb()
    ├── interface.c:520 | interface_set_l3_dev()
    ├── interface-ip.c:320 | interface_ip_set_enabled()
    ├── iface->state = IFS_UP
    └── interface.c:600 | interface_event(IFEV_UP)
        ├── interface-event.c:150 | interface_queue_event()
        └── ubus.c:1200 | netifd_ubus_interface_notify()  [广播 up 事件]
```

## 第六阶段：接口 DOWN 事件处理链（事件驱动）

### 6.1 DOWN 命令触发
```
触发方式之一：ubus call network.interface.lan down
    ↓
ubus socket 可读 → uloop 调用 ubus 回调
    ↓
ubus.c:1150 | netifd_handle_down()
    ↓
interface.c:1180 | interface_set_down(struct interface *iface)
```

### 6.2 DOWN 核心处理链
```
interface_set_down(struct interface *iface)
    └── interface.c:390 | __interface_set_down()
        ├── iface->state = IFS_TEARDOWN
        └── proto.c:656 | interface_proto_event(PROTO_CMD_TEARDOWN)
            ↓
            proto-shell.c:290 | proto_shell_handler()  [处理 teardown]
                └── main.c:138 | netifd_start_process()  [执行 teardown 脚本]
```

### 6.3 teardown 完成后的清理链
```
脚本执行完成
    ↓
main.c:100 | netifd_process_cb()
    ↓
proto-shell.c:740 | proto_shell_task_finish()
    ↓
interface.c:444 | interface_proto_event_cb(IFPEV_DOWN)
    ├── interface-ip.c:400 | interface_ip_flush()  [清理 IP 配置]
    ├── device.c:620 | device_release()  [释放设备引用]
    ├── iface->state = IFS_DOWN
    └── interface.c:600 | interface_event(IFEV_DOWN)
        └── ubus.c:1200 | netifd_ubus_interface_notify()  [广播 down 事件]
```

## 第七阶段：内核事件处理链（热插拔、链路状态）

```
内核发送 netlink 消息 (RTM_NEWLINK/RTM_DELLINK)
    ↓
system-linux.c:450 | cb_rtnl_event()
    ├── 解析接口名和 carrier 状态
    ├── device.c:350 | device_find()  [查找设备]
    ├── device.c:430 | device_set_present()  [更新存在状态]
    ├── device.c:480 | device_set_link()  [更新链路状态]
    └── device.c:530 | device_broadcast_event()
        ↓
        遍历 dev->users 列表，调用每个用户的回调
        ↓
        interface.c:650 | interface_main_dev_cb()
            ↓
            interface.c:1450 | interface_check_state()
                ├── 重新评估接口状态
                └── 可能触发 interface_set_up() 或 interface_set_down()
```

## 第八阶段：配置重载处理链

```
用户命令: ubus call network reload
    ↓
ubus socket 可读 → uloop 调用 ubus 回调
    ↓
ubus.c:1000 | netifd_handle_reload()
    ↓
config.c:900 | config_reload()
    ├── 重新解析 UCI 配置，与旧配置比较
    ├── interface.c:800 | interface_change_config()
    │   ├── 判断变更类型 (reload/reload_ip)
    │   ├── set_config_state(IFC_RELOAD)
    │   └── interface.c:1450 | interface_check_state()
    │       └── 触发接口状态重新评估
    ├── device.c:700 | device_change_config()
    └── wireless.c:800 | wireless_change_config()
```

## 状态机转换总结

### 接口状态（interface.h）
```c
enum interface_state {
    IFS_DOWN,          // 初始状态/停止完成
    IFS_SETUP,         // 正在启动
    IFS_UP,            // 启动完成
    IFS_TEARDOWN,      // 正在停止
};
```

### 状态转换条件
```
IFS_DOWN → IFS_SETUP:   auto=true & enabled=true & device available
IFS_SETUP → IFS_UP:     协议 SETUP 成功 (IFPEV_UP)
IFS_SETUP → IFS_DOWN:   协议 SETUP 失败/超时
IFS_UP → IFS_TEARDOWN:  enabled=false 或 link_state=false
IFS_TEARDOWN → IFS_DOWN: 协议 TEARDOWN 完成 (IFPEV_DOWN)
```

### 设备事件（device.h）
```c
enum device_event {
    DEV_EVENT_ADD,          // 设备出现
    DEV_EVENT_REMOVE,       // 设备移除  
    DEV_EVENT_UP,           // 设备激活（接口声明）
    DEV_EVENT_DOWN,         // 设备停用（接口释放）
    DEV_EVENT_LINK_UP,      // 链路建立
    DEV_EVENT_LINK_DOWN,    // 链路断开
};
```

## 架构总结

### 一次性初始化（启动时执行一次）
1. **构造函数注册**：`__init` 函数在 main 前自动执行，注册模块
2. **主函数初始化**：初始化日志、信号、ubus、uloop、子系统
3. **配置加载与对象创建**：解析 UCI → 创建 interface → 关联 device → 绑定 proto
4. **接口状态初始化**：所有接口进入 `IFS_DOWN` 状态

### 事件驱动（运行时终身执行）
1. **uloop 启动**：`uloop_run()` 永不返回，监听多个事件源
2. **事件触发**：ubus 命令、内核事件、脚本完成、定时器
3. **回调调用**：事件触发对应的回调函数
4. **状态机转换**：回调函数驱动状态机状态变化
5. **广播通知**：状态变化通过 ubus 广播给监听者

### 关键设计特点
- **资源高效**：初始化后保持稳定状态对象，无需重复创建
- **响应迅速**：事件驱动确保毫秒级响应外部变化
- **状态一致**：状态机模式保证逻辑清晰，避免竞态条件
- **模块化解耦**：回调机制降低模块间依赖性，易于扩展
- **统一事件管理**：uloop 统一管理所有 I/O 事件、定时器、进程

---

*文档生成时间：2026年3月13日*  
*基于 netifd 源码深度分析，行号基于当前代码版本*