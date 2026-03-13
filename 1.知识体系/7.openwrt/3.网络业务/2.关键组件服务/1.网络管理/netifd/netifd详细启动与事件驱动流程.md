# netifd 详细启动与事件驱动流程

## 概述

netifd 采用 **"一次性初始化 + 终身事件驱动"** 架构：

1. **启动阶段**：加载配置、创建接口、绑定设备与协议（只执行一次）
2. **运行阶段**：uloop 事件循环处理所有 up/down 事件和状态变化

## 第一阶段：一次性初始化（启动时执行）

### 1.1 构造函数阶段（main() 函数之前）

在 main() 函数执行前，GCC 的 `__attribute__((constructor))` 特性自动执行标记函数。

| 文件 | 行号 | 函数 | 作用 |
|------|------|------|------|
| `utils.h` | 约 50 | `#define __init __attribute__((constructor))` | 定义构造函数宏 |
| `interface.c` | 1417 | `static void __init interface_init_list(void)` | 初始化接口虚拟列表树 |
| `device.c` | 547 | `static void __init dev_init(void)` | 初始化设备 AVL 树 |
| `bridge.c` | 1424 | `static void __init bridge_device_type_init(void)` | 注册网桥设备类型 |
| `macvlan.c` | 266 | `static void __init macvlan_device_type_init(void)` | 注册 MACVLAN 设备类型 |
| `proto-static.c` | 109 | `static void __init static_proto_init(void)` | 注册静态 IP 协议处理器 |

**执行顺序**（由链接器决定）：
```
程序启动 → 自动执行所有 __init 函数 → main() 开始执行
```

### 1.2 主函数初始化阶段

| 文件 | 行号 | 函数/代码 | 作用 |
|------|------|-----------|------|
| `main.c` | 279 | `int main(int argc, char **argv)` | 程序入口点 |
| `main.c` | 286-319 | 命令行参数解析 | 处理 -d、-s 等选项 |
| `main.c` | 321-322 | `openlog("netifd", 0, LOG_DAEMON)` | 初始化系统日志 |
| `main.c` | 324 | `netifd_setup_signals()` | 设置信号处理器（SIGINT/SIGTERM） |
| `main.c` | 325 | `netifd_ubus_init(socket)` | 初始化 ubus 连接和 uloop |

### 1.3 子系统初始化

| 文件 | 行号 | 函数调用 | 作用 |
|------|------|----------|------|
| `main.c` | 330 | `proto_shell_init()` | 扫描 /lib/netifd/proto/ 目录注册脚本协议 |
| `main.c` | 331 | `extdev_init()` | 初始化外部设备支持 |
| `main.c` | 332 | `wireless_init()` | 初始化无线子系统 |
| `main.c` | 334 | `system_init()` | 创建 netlink socket，注册内核事件回调 |
| `main.c` | 339 | `config_init_all()` | **核心：加载并解析所有 UCI 配置** |

### 1.4 配置加载与接口创建流程

#### 1.4.1 配置解析总入口

| 文件 | 行号 | 函数 | 作用 |
|------|------|------|------|
| `config.c` | 744 | `void config_init_all(void)` | 配置初始化总入口 |

#### 1.4.2 接口配置解析

| 文件 | 行号 | 函数 | 作用 |
|------|------|------|------|
| `config.c` | 350 | `static void config_parse_interface(...)` | 解析 UCI interface 节 |
| `config.c` | 约 380 | `interface_alloc()` 调用 | 创建接口结构体 |

**接口创建细节**：
| 文件 | 行号 | 函数 | 作用 |
|------|------|------|------|
| `interface.c` | 814 | `struct interface *interface_alloc(...)` | 分配接口结构体，解析属性 |
| `interface.c` | 约 850 | 解析 `proto`、`ifname`、`ipaddr` 等属性 | 设置接口配置参数 |
| `interface.c` | 991 | `void interface_add(struct interface *iface)` | 将接口加入全局接口树 |
| `interface.c` | 约 1000 | `interface_update()` 回调触发 | 处理接口配置更新 |

#### 1.4.3 设备配置解析

| 文件 | 行号 | 函数 | 作用 |
|------|------|------|------|
| `config.c` | 400 | `static void config_parse_device(...)` | 解析 UCI device 节 |
| `device.c` | 320 | `struct device *device_create(...)` | 创建设备结构体 |
| `device.c` | 约 350 | 设置设备类型（bridge、vlan、tunnel 等） | 配置设备参数 |

#### 1.4.4 协议绑定

| 文件 | 行号 | 函数 | 作用 |
|------|------|------|------|
| `interface.c` | 约 900 | `proto_attach_interface()` 调用 | 绑定协议处理器到接口 |
| `proto.c` | 636 | `int proto_attach_interface(...)` | 查找并绑定协议处理器 |
| `proto.c` | 约 650 | `get_proto_handler()` | 在 handlers AVL 树中查找协议 |

#### 1.4.5 设备关联

| 文件 | 行号 | 函数 | 作用 |
|------|------|------|------|
| `interface.c` | 640 | `static void interface_claim_device(...)` | 根据配置获取或创建设备 |
| `interface.c` | 约 660 | `interface_set_main_dev()` | 设置接口的主设备 |
| `device.c` | 600 | `void device_claim(struct device *dev, ...)` | 声明设备使用权，增加引用计数 |

#### 1.4.6 接口状态初始化

| 文件 | 行号 | 函数/代码 | 作用 |
|------|------|-----------|------|
| `interface.c` | 约 1100 | `iface->state = IFS_DOWN` | 接口初始状态设为 DOWN |
| `interface.c` | 1450 | `void interface_check_state(...)` | 检查启动条件 |
| `interface.c` | 1177 | `void interface_start_pending(void)` | 启动符合条件的挂起接口 |

### 1.5 初始化完成

| 文件 | 行号 | 函数/代码 | 作用 |
|------|------|-----------|------|
| `main.c` | 341 | `uloop_run()` | **进入事件循环，初始化阶段结束** |

**此时**：
- 所有接口已创建并处于 `IFS_DOWN` 状态
- 设备已关联
- 协议已绑定
- uloop 已启动，等待事件

## 第二阶段：事件驱动运行（uloop 终身运行）

### 2.1 uloop 事件循环核心

| 文件 | 行号 | 函数/代码 | 作用 |
|------|------|-----------|------|
| `main.c` | 341 | `uloop_run()` | 启动事件循环，永不返回（除非收到终止信号） |

**uloop 管理的三种事件类型**：
1. **文件描述符事件**（ubus socket、netlink socket）
2. **定时器事件**（重连定时器、状态检查定时器）
3. **进程事件**（外部脚本进程退出）

### 2.2 接口 up 事件处理流程

#### 2.2.1 触发方式

**方式一：ubus RPC 命令**
```
ubus call network.interface.lan up
    ↓
ubus socket 可读事件
    ↓
uloop 调用 ubus 回调
```

**方式二：自动启动**（配置中 `auto=true`）
```
接口检查条件满足
    ↓
interface_check_state() 触发
    ↓
自动调用 interface_set_up()
```

#### 2.2.2 核心 up 调用链

| 文件 | 行号 | 函数 | 作用 |
|------|------|------|------|
| `ubus.c` | 1100 | `static int netifd_handle_up(...)` | 处理 ubus up 命令 |
| `interface.c` | 1123 | `void interface_set_up(struct interface *iface)` | up 命令入口函数 |
| `interface.c` | 1128 | `interface_clear_errors()` | 清理之前的错误 |
| `device.c` | 600 | `device_claim()` | 确保设备可用（如未声明） |
| `interface.c` | 351 | `static void __interface_set_up(...)` | **核心 up 逻辑** |
| `interface.c` | 约 360 | `iface->state = IFS_SETUP` | 状态设为 SETUP |
| `proto.c` | 656 | `int interface_proto_event(...)` | 向协议发送 SETUP 命令 |
| `proto-shell.c` | 205 | `proto_shell_handler()` | shell 协议处理函数 |
| `main.c` | 138 | `netifd_start_process()` | 创建子进程执行外部脚本 |

#### 2.2.3 脚本执行与完成

| 文件 | 行号 | 函数 | 作用 |
|------|------|------|------|
| `main.c` | 100 | `netifd_process_cb()` | 进程退出回调 |
| `proto-shell.c` | 720 | `proto_shell_script_cb()` | 脚本完成回调 |
| `proto-shell.c` | 740 | `proto_shell_task_finish()` | 任务完成处理 |
| `interface.c` | 444 | `interface_proto_event_cb()` | 协议事件回调 |
| `interface.c` | 约 460 | `iface->state = IFS_UP` | 状态设为 UP |
| `interface.c` | 520 | `interface_set_l3_dev()` | 设置三层设备 |
| `interface-ip.c` | 320 | `interface_ip_set_enabled()` | 启用 IP 配置 |
| `interface.c` | 600 | `interface_event()` | 触发接口事件 |
| `ubus.c` | 1200 | `netifd_ubus_interface_notify()` | 广播接口 up 事件 |

### 2.3 接口 down 事件处理流程

#### 2.3.1 触发方式

**方式一：ubus RPC 命令**
```
ubus call network.interface.lan down
    ↓
ubus socket 可读事件
    ↓
uloop 调用 ubus 回调
```

**方式二：设备事件**
```
设备链路断开
    ↓
netlink 事件 → device_set_link(false)
    ↓
device_broadcast_event() → interface_main_dev_cb()
    ↓
interface_check_state() → interface_set_down()
```

#### 2.3.2 核心 down 调用链

| 文件 | 行号 | 函数 | 作用 |
|------|------|------|------|
| `ubus.c` | 约 1150 | `static int netifd_handle_down(...)` | 处理 ubus down 命令 |
| `interface.c` | 1180 | `void interface_set_down(struct interface *iface)` | down 命令入口函数 |
| `interface.c` | 390 | `static void __interface_set_down(...)` | **核心 down 逻辑** |
| `interface.c` | 约 400 | `iface->state = IFS_TEARDOWN` | 状态设为 TEARDOWN |
| `proto.c` | 656 | `interface_proto_event(PROTO_CMD_TEARDOWN)` | 向协议发送 TEARDOWN 命令 |
| `proto-shell.c` | 290 | `proto_shell_handler()` 处理 teardown | 执行 teardown 脚本 |
| `interface-ip.c` | 约 400 | `interface_ip_flush()` | 清理所有 IP 配置 |
| `device.c` | 620 | `device_release()` | 释放设备引用 |
| `interface.c` | 约 420 | `iface->state = IFS_DOWN` | 状态设为 DOWN |
| `ubus.c` | 1200 | `netifd_ubus_interface_notify()` | 广播接口 down 事件 |

### 2.4 内核事件处理（热插拔、链路状态）

#### 2.4.1 netlink 事件接收

| 文件 | 行号 | 函数 | 作用 |
|------|------|------|------|
| `system-linux.c` | 450 | `static int cb_rtnl_event(...)` | netlink 事件回调 |
| `system-linux.c` | 约 470 | 解析 RTM_NEWLINK/RTM_DELLINK | 获取接口名和 carrier 状态 |

#### 2.4.2 设备状态更新

| 文件 | 行号 | 函数 | 作用 |
|------|------|------|------|
| `device.c` | 430 | `void device_set_present(...)` | 更新设备存在状态 |
| `device.c` | 480 | `void device_set_link(...)` | 更新设备链路状态 |
| `device.c` | 530 | `void device_broadcast_event(...)` | 广播设备事件给所有用户 |

#### 2.4.3 接口状态重新评估

| 文件 | 行号 | 函数 | 作用 |
|------|------|------|------|
| `interface.c` | 650 | `interface_main_dev_cb()` | 设备事件回调 |
| `interface.c` | 1450 | `interface_check_state()` | 重新评估接口状态 |
| `interface.c` | 约 1480 | 可能调用 `interface_set_up()` 或 `interface_set_down()` | 触发状态变化 |

### 2.5 配置重载处理

**注意**：配置重载会部分重新执行初始化流程，但不是完整的重启。

| 文件 | 行号 | 函数 | 作用 |
|------|------|------|------|
| `ubus.c` | 约 1000 | `netifd_handle_reload()` | 处理 reload 命令 |
| `config.c` | 900 | `void config_reload(void)` | 重载配置 |
| `config.c` | 约 920 | 重新解析 UCI 配置 | 与旧配置比较差异 |
| `interface.c` | 800 | `interface_change_config()` | 处理接口配置变更 |
| `interface.c` | 约 820 | `set_config_state(IFC_RELOAD)` | 设置重载状态 |
| `interface.c` | 1450 | `interface_check_state()` | 重新评估所有接口状态 |

## 关键数据结构状态变化

### 接口状态机（interface.c）
```c
// interface.h 中定义
enum interface_state {
    IFS_DOWN,          // 初始状态/停止完成
    IFS_SETUP,         // 正在启动
    IFS_UP,            // 启动完成
    IFS_TEARDOWN,      // 正在停止
};

// 状态转换触发条件
IFS_DOWN → IFS_SETUP:   auto=true & enabled=true & device available
IFS_SETUP → IFS_UP:     协议 SETUP 成功 (IFPEV_UP)
IFS_SETUP → IFS_DOWN:   协议 SETUP 失败/超时
IFS_UP → IFS_TEARDOWN:  enabled=false 或 link_state=false
IFS_TEARDOWN → IFS_DOWN: 协议 TEARDOWN 完成 (IFPEV_DOWN)
```

### 设备状态（device.c）
```c
// device.h 中定义
enum device_event {
    DEV_EVENT_ADD,          // 设备出现
    DEV_EVENT_REMOVE,       // 设备移除  
    DEV_EVENT_UP,           // 设备激活（接口声明）
    DEV_EVENT_DOWN,         // 设备停用（接口释放）
    DEV_EVENT_LINK_UP,      // 链路建立
    DEV_EVENT_LINK_DOWN,    // 链路断开
};
```

## 总结：一次性初始化 + 事件驱动架构

### 代码执行流程总结
```
第一阶段（一次性）：
    __init 构造函数（main 前）
    ↓
    main() 函数执行
    ↓
    ubus/uloop 初始化
    ↓
    子系统初始化（proto、wireless、system）
    ↓
    config_init_all() 加载配置
    ↓
    创建 interface、device，绑定 proto
    ↓
    接口进入 IFS_DOWN 状态
    ↓
    uloop_run() 启动事件循环

第二阶段（事件驱动）：
    uloop 等待事件：
        - ubus socket（用户命令）
        - netlink socket（内核事件）  
        - 进程管道（脚本完成）
        - 定时器（超时/重试）
    ↓
    事件触发 → 调用对应回调
    ↓
    状态机转换（up/down）
    ↓
    广播状态变化
    ↓
    返回事件等待
```

### 架构优势
1. **高效资源利用**：初始化后保持稳定状态对象，无需重复创建
2. **快速事件响应**：事件驱动确保毫秒级响应外部变化
3. **清晰状态管理**：状态机模式保证逻辑一致性
4. **模块化解耦**：回调机制降低模块间依赖性
5. **易于扩展**：新事件类型只需注册到 uloop 即可

### 关键设计原则
- **初始化与运行分离**：启动时完成所有资源配置，运行时只处理状态变化
- **事件驱动一切**：所有状态变化都由事件触发，避免轮询浪费
- **状态机为核心**：统一的状态转换逻辑，集中处理业务规则
- **回调解耦**：模块间通过回调接口交互，减少直接依赖

---

*文档生成时间：2026年3月13日*  
*基于 netifd 源码深度分析，行号基于当前代码版本，可能随版本更新而变化*