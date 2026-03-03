# OpenWrt 升级（sysupgrade）实现原理详解

## 一、核心前置知识

### 1. 设备分区结构（以典型路由器为例）

OpenWrt 设备的 Flash 分区是升级的物理基础，典型分区表如下：

| 分区名 | 作用 | 关键特性 |
|--------|------|----------|
| bootloader | 存放 U-Boot/CFE | 只读，升级不触碰 |
| firmware | 核心分区（内核 + 根文件系统） | 升级的核心操作目标 |
| kernel | 内核镜像（部分设备拆分） | 可选，部分设备合并到 firmware |
| rootfs | 根文件系统（部分设备拆分） | 可选，部分设备合并到 firmware |
| overlay | 可写配置分区 | 保留配置时需备份此分区 |
| factory | 原厂配置 / 校准数据 | 升级不修改 |

### 2. 核心工具：sysupgrade

`sysupgrade` 是 OpenWrt 升级的唯一官方工具，本质是一个 Shell 脚本（`/sbin/sysupgrade`），整合了配置备份、固件校验、分区擦写、重启等全流程，底层依赖 `mtd`（Flash 操作工具）、`ubiformat`（UBIFS 分区工具）等核心组件。

## 二、sysupgrade 完整执行流程（核心）

`sysupgrade` 的执行可分为 8 个关键阶段，每个阶段对应明确的操作逻辑：

### 阶段 1：参数解析与环境检查

- 解析命令行参数（`-n`/`-F`/`-b` 等），初始化变量（如固件路径、是否保留配置）；
- 检查核心依赖：`mtd`、`sha256sum`、`ubus` 等工具是否存在；
- 校验设备硬件信息（如 `board_name`），匹配固件的硬件兼容列表（防止刷错固件）；
- 检查 Flash 剩余空间，确保固件可写入。

### 阶段 2：配置备份（保留配置时）

若未指定 `-n`（不保留配置），则执行配置备份：

- 定义需备份的目录 / 文件（核心是 `/etc/config/`，还包括 `/etc/passwd`、`/etc/shadow`、`/etc/dropbear` 等）；

```bash
# 核心备份列表（sysupgrade 内置）
CONF_TAR="/tmp/sysupgrade.tgz"
BACKUP_LIST="/etc /usr/lib/lua /usr/share/rpcd/acl.d"
```

- 将备份文件打包为 `/tmp/sysupgrade.tgz`（临时存储在 RAM 的 `/tmp` 目录）；
- 验证备份包完整性（MD5 校验）。

### 阶段 3：固件校验

- 解压固件镜像，提取固件头部的 magic number（魔数）和硬件兼容信息；
- 校验固件签名（官方固件），确保未被篡改；
- 对比固件的 `board_name` 与设备实际 `board_name`，不匹配则终止（除非加 `-F` 强制）；
- 校验固件大小，确保不超过 firmware 分区容量。

### 阶段 4：系统预处理（停止服务 / 卸载分区）

为避免升级时文件被占用，执行前置清理：

- 通过 ubus 停止所有非核心服务：

```bash
ubus call service list | jsonfilter -e '@.*.name' | while read -r service; do
    /etc/init.d/$service stop
done
```

- 卸载非必要挂载点（`/overlay`、`/mnt` 等）：

```bash
umount /overlay /mnt/usb 2>/dev/null
```

- 同步内存数据到磁盘（`sync`），避免数据丢失。

### 阶段 5：擦除目标分区

通过 `mtd` 工具擦除 firmware 分区（核心操作）：

```bash
mtd erase firmware
```

若为 UBIFS 分区，先卸载 UBI 卷，再擦除：

```bash
ubi detach /dev/ubi0
ubiformat /dev/mtdX -y
```

### 阶段 6：写入新固件

将固件镜像写入 firmware 分区（核心写操作）：

```bash
mtd write /tmp/openwrt-sysupgrade.bin firmware
```

- 写入时会做分片校验，确保每块数据写入正确；
- 若固件包含分区表（如 kernel+rootfs 拆分），会自动拆分写入对应子分区。

### 阶段 7：恢复配置（保留配置时）

- 将 `/tmp/sysupgrade.tgz` 备份包写入新固件的 overlay 分区预留空间；
- 标记配置恢复标记（如创建 `/tmp/restore_config`），让系统重启后自动恢复配置。

### 阶段 8：重启系统

- 执行 `reboot -f` 强制重启，Bootloader 加载新写入的固件；
- 重启后，新固件初始化：
  - 若有配置备份，自动解压 `/tmp/sysupgrade.tgz` 到 `/overlay`；
  - 重建 overlay 分区，合并只读根文件系统与可写配置；
  - 启动所有服务，完成升级。

## 三、关键技术细节

### 1. 固件镜像结构

OpenWrt 的 `sysupgrade.bin` 是可直接写入 Flash 的镜像，结构如下：

```
[头部信息（magic+硬件信息）] + [内核镜像（zImage）] + [根文件系统（SquashFS）] + [可选配置备份区]
```

- **头部信息**：包含 `board_name`、固件版本、校验和，供 `sysupgrade` 校验；
- **内核镜像**：压缩的 Linux 内核，Bootloader 加载的核心；
- **根文件系统**：只读 SquashFS 镜像，包含系统基础文件。

### 2. 保留配置的底层逻辑

OpenWrt 的配置存储在 `/overlay` 分区（可写），保留配置的核心是：

1. 升级前将 `/overlay` 核心文件打包到 RAM（`/tmp`）；
2. 新固件写入后，将备份包写入新固件的 `/overlay` 分区；
3. 重启后，新系统的 OverlayFS 合并 `/rom`（新固件只读区）和 `/overlay`（旧配置可写区），实现“新系统 + 旧配置”。

### 3. 固件校验的核心代码片段（sysupgrade 内置）

```bash
# 提取固件的 board_name
image_board=$(get_image_board "$IMAGE")
# 设备实际 board_name
board=$(board_name)

# 校验硬件匹配
if [ "$image_board" != "$board" ] && [ -z "$FORCE" ]; then
    echo "固件不匹配当前设备：$board != $image_board"
    exit 1
fi
```

### 4. Flash 写入的底层调用

`sysupgrade` 最终调用 `mtd` 工具操作 Flash，核心命令：

```bash
# 擦除分区
mtd erase <partition>
# 写入固件
mtd write <固件路径> <partition>
# 查看分区信息
mtd info
```

## 四、特殊场景的升级逻辑

### 1. x86 平台升级（如软路由）

- x86 平台无 Flash 分区，固件为 `img.gz` 格式，升级逻辑：
  1. 将固件写入硬盘分区（如 `/dev/sda1`）；
  2. 重写 GRUB 引导项，指向新内核；
  3. 重启后 GRUB 加载新内核，完成升级。

### 2. 跨版本升级（如 21.02 → 23.05）

- **核心风险**：配置文件格式变化（如网络接口、防火墙规则）；
- `sysupgrade` 内置 `config migrate` 工具，自动迁移旧配置到新格式；
- 若迁移失败，需手动修改 `/etc/config/` 下的配置文件。

## 五、故障兜底机制

### 1. 写入失败的保护

- `sysupgrade` 会先将固件写入 RAM（`/tmp`），校验通过后再写入 Flash；
- 写入 Flash 时按块校验，单块失败则终止，避免分区损坏。

### 2. 启动失败的恢复

- Bootloader 通常有双固件分区（`firmware1`/`firmware2`），若新固件启动失败，自动回滚到旧固件；
- 无双分区设备可通过 TFTP 从 U-Boot 重刷固件。

## 总结

1. OpenWrt 升级的核心是 **`sysupgrade` 脚本**，整合了 **备份 → 校验 → 擦除 → 写入 → 恢复 → 重启** 全流程。
2. 物理层面是对 **firmware 分区的擦写**，逻辑层面是保留 / 重置 `/overlay` 配置分区。
3. 关键校验点是 **硬件匹配（`board_name`）** 和 **固件完整性（签名 / 大小）**，`-F` 可跳过硬件校验（谨慎使用）。
4. 保留配置的本质是 **备份 `/overlay` 分区到 RAM，新固件写入后恢复该分区**。