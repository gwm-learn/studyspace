# OpenWrt 恢复出厂设置（Factory Reset）实现原理详解

## 一、核心前置认知

### 1. 恢复出厂的本质

OpenWrt 系统分为只读分区和可写分区两部分：

- **只读分区（`/rom`）**：存放系统基础文件（内核、默认配置、系统命令），由 SquashFS 格式存储，不可修改；
- **可写分区（`/overlay`）**：存放用户自定义配置（`/etc/config/`、安装的软件、修改的文件），由 JFFS2/UBIFS/EXT4 格式存储。

**恢复出厂设置的核心逻辑**：清空可写分区（`/overlay`）的所有用户数据，让系统仅加载只读分区（`/rom`）的默认配置，最终回到首次刷入固件时的初始状态。

### 2. 核心触发方式

OpenWrt 支持两种恢复出厂的触发途径，底层逻辑一致：

- **手动触发**：LuCI 后台（系统→备份 / 升级→恢复出厂设置）、SSH 执行 `firstboot` 命令；
- **硬件触发**：长按设备「复位键（Reset）」5-10 秒，由热插拔脚本触发恢复出厂。

### 3. 核心工具

- **`firstboot`**：OpenWrt 恢复出厂的核心命令（本质是 Shell 脚本 `/sbin/firstboot`）；
- **`jffs2reset`/`ubirsvol`**：针对不同可写分区格式的擦除工具；
- **`procd` + 热插拔脚本**：处理硬件复位键触发的恢复出厂。

## 二、恢复出厂完整执行流程

无论是 LuCI/SSH 触发，还是硬件复位键触发，最终都会调用 `firstboot` 脚本，执行以下 6 个核心阶段：

### 阶段 1：参数解析与安全校验

`firstboot` 脚本首先解析参数（如 `-y` 跳过确认、`-r` 仅重置网络配置）；

- 校验当前系统状态：确保 `/overlay` 分区已挂载，且为可写状态；
- 非 `-y` 参数时，会弹出确认提示（LuCI 触发时自动带 `-y`）：

```bash
# firstboot 脚本内置的确认逻辑
[ "$FORCE" != "y" ] && echo -n "Are you sure? [y/N] " && read -n1 confirm
[ "$confirm" != "y" ] && exit 1
```

### 阶段 2：停止依赖服务

为避免擦除配置时文件被占用，先停止所有非核心服务：

- 通过 ubus 列出所有运行中的服务：

```bash
SERVICES=$(ubus call service list | jsonfilter -e '@.*.name')
```

- 逐一停止服务（网络、防火墙、SSH、LuCI 等）：

```bash
for service in $SERVICES; do
    /etc/init.d/$service stop 2>/dev/null
done
```

- 卸载非必要挂载点（如 `/mnt`、`/tmp/mounts` 等）：

```bash
umount /mnt/* /overlay/upper 2>/dev/null
```

### 阶段 3：清空可写分区（核心操作）

根据 `/overlay` 分区的文件系统格式，执行不同的擦除逻辑：

#### 场景 1：JFFS2 格式（主流路由器）

```bash
# 卸载 /overlay 分区
umount /overlay
# 擦除 JFFS2 分区（mtd 工具操作 Flash）
jffs2reset -y
# 重新挂载 /overlay（此时为空）
mount -t jffs2 /dev/mtdblock$(find_mtd_part overlay) /overlay
```

#### 场景 2：UBIFS 格式（新路由器 / ARM 设备）

```bash
# 卸载 UBI 卷
umount /overlay
ubi detach /dev/ubi0 2>/dev/null
# 重新创建 UBI 卷并格式化
ubiformat /dev/mtd$(find_mtd_part ubi) -y
ubi attach /dev/mtd$(find_mtd_part ubi)
ubirmvol /dev/ubi0 -N rootfs_data 2>/dev/null
ubimkvol /dev/ubi0 -N rootfs_data -s $(cat /sys/class/mtd/ubi/size)
# 挂载空的 UBIFS 分区
mount -t ubifs ubi0:rootfs_data /overlay
```

#### 场景 3：x86 软路由（EXT4 格式）

```bash
# 卸载 /overlay 分区
umount /overlay
# 格式化 EXT4 分区（清空所有数据）
mkfs.ext4 /dev/sda2 2>/dev/null
# 重新挂载
mount /dev/sda2 /overlay
```

### 阶段 4：重置核心配置文件

即使 `/overlay` 已清空，仍会主动重置关键系统文件，确保绝对干净：

```bash
# 清空临时目录
rm -rf /tmp/* /tmp/.* 2>/dev/null
# 重置 /etc 目录为只读分区的默认版本
cp -a /rom/etc/* /etc/
# 重置网络、防火墙等核心配置
rm -rf /etc/config/network /etc/config/firewall
cp /rom/etc/config/network /etc/config/
cp /rom/etc/config/firewall /etc/config/
```

### 阶段 5：标记重启并执行

- 创建重启标记文件（`/tmp/factory_reset`），告知系统重启后无需加载旧配置；
- 执行 `reboot -f` 强制重启（硬件触发时，由热插拔脚本调用 `reboot`）；
- 若为 LuCI 触发，会先返回 “恢复出厂成功” 的页面，再后台执行重启。

### 阶段 6：重启后系统初始化

设备重启后，Procd 执行启动流程时：

- 检测到 `/overlay` 为空，直接加载 `/rom` 中的默认配置；
- 重建 `/etc/config/` 下的所有默认配置文件（网络、DHCP、防火墙等）；
- 初始化默认网络（LAN 口 `192.168.1.1/24`、DHCP 开启）；
- 启动核心服务，系统回到出厂状态。

## 三、硬件复位键触发恢复出厂的特殊逻辑

硬件复位键的触发是 “热插拔 + firstboot” 的组合，额外增加了以下步骤：

### 1. 按键事件检测

复位键按下时，内核产生 `button` 类型的 uevent，触发 `/etc/hotplug.d/button/` 下的脚本；

典型脚本逻辑（`/etc/hotplug.d/button/00-reset`）：

```bash
[ "$BUTTON" = "reset" ] && [ "$ACTION" = "pressed" ] && {
    # 延迟执行（避免误触）
    sleep 3
    # 调用 firstboot 恢复出厂
    firstboot -y
    # 重启
    reboot -f
}
```

### 2. 防误触机制

- 多数设备要求长按复位键 5-10 秒才触发，脚本中通过 `sleep` 实现；
- 部分设备会通过 LED 闪烁提示 “即将恢复出厂”，进一步降低误触概率。

## 四、关键技术细节

### 1. 仅重置网络配置的实现

若只需重置网络（而非全量恢复），`firstboot` 支持 `-r` 参数，核心逻辑：

```bash
# 仅删除网络相关配置，恢复默认
rm -rf /etc/config/network /etc/config/wireless
cp /rom/etc/config/network /etc/config/
/etc/init.d/network restart
```

### 2. 恢复出厂不影响的内容

- 只读分区（`/rom`）的所有文件（系统内核、默认软件、基础命令）；
- Flash 中的 Bootloader 分区（U-Boot/CFE）；
- 原厂校准数据（如 WiFi 功率校准、MAC 地址）。

### 3. 恢复出厂失败的常见原因

- `/overlay` 分区损坏（Flash 坏块）：需通过 `mtd erase overlay` 强制擦除；
- 分区挂载为只读：执行 `mount -o remount,rw /overlay` 重新挂载；
- 脚本权限问题：`chmod +x /sbin/firstboot` 恢复执行权限。

### 4. 手动强制恢复出厂（应急）

若系统无法正常登录，可通过 U-Boot 触发：

1. 上电时按住复位键进入 U-Boot 模式；
2. 执行 U-Boot 命令擦除 overlay 分区：

```bash
nand erase overlay
reset
```

重启后自动恢复出厂。

## 总结

1. OpenWrt 恢复出厂的核心是 **清空可写分区 `/overlay`**，让系统仅加载只读分区 `/rom` 的默认配置；
2. 核心工具是 **`firstboot` 脚本**，整合了停止服务 → 卸载分区 → 擦除数据 → 重置配置 → 重启全流程；
3. 硬件复位键触发本质是 **“热插拔脚本检测按键事件 → 调用 firstboot → 重启”**；
4. 恢复出厂仅清空用户配置 / 安装的软件，不修改系统内核、Bootloader 等核心分区。