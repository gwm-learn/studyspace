# OPKG 包管理器

## 1. OPKG 概述

OPKG (Open Package Management) 是 OpenWrt 系统默认的轻量级包管理工具，类似于 Debian/Ubuntu 中的 APT 或 RedHat/CentOS 中的 YUM。它专为嵌入式系统设计，体积小、资源占用低，用于软件包的安装、更新、卸载和查询，是管理 OpenWrt 系统软件的核心工具。

## 2. 核心概念

### 2.1 软件源 (Repository)

- **定义**：存放预编译软件包的远程服务器地址，OPKG 通过这些地址获取软件包和依赖
- **分类**：
  - 官方源：OpenWrt 官方维护的软件源，稳定性最高
  - 第三方源：社区或个人维护的源，提供官方源中没有的软件包
- **架构适配**：OpenWrt 支持多种嵌入式架构（如 mipsel、arm、x86_64），源会自动匹配设备架构

### 2.2 软件包格式

- OpenWrt 软件包后缀为 **.ipk**
- 包含文件：
  - 主包：.ipk 核心文件（包含编译后的二进制、配置文件）
  - 控制文件：描述包名、版本、依赖、作者等信息
  - 校验文件：用于验证包完整性

### 2.3 依赖关系

- **运行依赖**：软件运行必需的库 / 工具（如 libc、openssl）
- **安装依赖**：安装过程中需要的工具（如 tar、gzip）
- **冲突**：不能与当前包共存的软件包

## 3. 基础配置

### 3.1 源配置文件

OPKG 源配置默认存放在 `/etc/opkg/distfeeds.conf`，也可自定义配置到 `/etc/opkg/customfeeds.conf`。

```bash
# 典型的源配置示例
src/gz openwrt_core https://downloads.openwrt.org/releases/23.05.2/targets/ramips/mt7621/packages
src/gz openwrt_base https://downloads.openwrt.org/releases/23.05.2/packages/mipsel_24kc/base
src/gz openwrt_luci https://downloads.openwrt.org/releases/23.05.2/packages/mipsel_24kc/luci
src/gz openwrt_packages https://downloads.openwrt.org/releases/23.05.2/packages/mipsel_24kc/packages
src/gz openwrt_routing https://downloads.openwrt.org/releases/23.05.2/packages/mipsel_24kc/routing
src/gz openwrt_telephony https://downloads.openwrt.org/releases/23.05.2/packages/mipsel_24kc/telephony
```

### 3.2 配置参数说明

| 配置项 | 说明 |
|--------|------|
| src/gz | 源类型（gz 表示压缩包） |
| openwrt_core | 源名称（自定义） |
| 网址 | 源的远程地址（需匹配设备架构和 OpenWrt 版本） |

### 3.3 配置修改步骤

1. **备份原有配置**：

   ```bash
   cp /etc/opkg/distfeeds.conf /etc/opkg/distfeeds.conf.bak
   ```

2. **编辑配置文件**（推荐使用 vi 或 nano）：

   ```bash
   vi /etc/opkg/distfeeds.conf
   ```

3. **注释 / 取消注释源**（# 开头为注释）：

   ```bash
   # 注释掉不需要的源
   # src/gz openwrt_telephony https://xxx.xxx.xxx
   ```

4. **更新源缓存**：

   ```bash
   opkg update
   ```

## 4. 常用命令

### 4.1 基础操作

| 命令 | 功能说明 |
|------|----------|
| `opkg update` | 更新软件源缓存（必做：安装 / 更新包前执行） |
| `opkg install <package>` | 安装指定软件包 |
| `opkg remove <package>` | 卸载指定软件包 |
| `opkg upgrade <package>` | 升级指定软件包 |
| `opkg upgrade` | 升级所有可升级的软件包（谨慎使用） |

### 4.2 查询操作

| 命令 | 功能说明 |
|------|----------|
| `opkg list` | 列出所有可用软件包 |
| `opkg list-installed` | 列出已安装的软件包 |
| `opkg list-upgradable` | 列出可升级的软件包 |
| `opkg search <keyword>` | 按关键词搜索软件包 |
| `opkg info <package>` | 查看指定包的详细信息（版本、依赖、描述） |
| `opkg files <package>` | 查看指定包安装的所有文件路径 |

### 4.3 高级操作

| 命令 | 功能说明 |
|------|----------|
| `opkg configure <package>` | 重新配置已安装的包 |
| `opkg download <package>` | 仅下载包不安装（保存到当前目录） |
| `opkg install <file.ipk>` | 安装本地 ipk 包 |
| `opkg remove --force-removal-of-dependent-packages <package>` | 强制卸载包（包括依赖它的包） |
| `opkg clean` | 清理下载的包缓存 |

### 4.4 命令示例

```bash
# 更新源缓存
opkg update

# 安装常用工具
opkg install wget curl nano

# 查看已安装的 luci 相关包
opkg list-installed | grep luci

# 查看 wget 包的详细信息
opkg info wget

# 卸载不需要的包
opkg remove nano

# 安装本地下载的 ipk 包
opkg install /tmp/mypackage.ipk
```

## 5. 常见问题与解决方案

### 5.1 源更新失败

- **原因**：网络不通、源地址失效、架构不匹配
- **解决方案**：
  1. 检查网络：`ping downloads.openwrt.org`
  2. 确认源地址匹配设备架构：`cat /etc/openwrt_release` 查看版本和架构
  3. 更换国内镜像源（如清华镜像）：

     ```bash
     # 替换为清华源示例
     sed -i 's/downloads.openwrt.org/mirrors.tuna.tsinghua.edu.cn\/openwrt/g' /etc/opkg/distfeeds.conf
     ```

### 5.2 依赖缺失

- **现象**：安装包时提示 Missing dependency
- **解决方案**：
  - 自动安装依赖：`opkg install --autoremove <package>`
  - 手动安装缺失的依赖包：`opkg install <missing_dependency>`

### 5.3 磁盘空间不足

- **现象**：安装包时提示 No space left on device
- **解决方案**：
  - 查看磁盘空间：`df -h`
  - 清理无用包：`opkg remove <unused_package>`
  - 清理缓存：`opkg clean`
  - 扩展存储空间（如挂载 U 盘）

### 5.4 包版本冲突

- **现象**：提示 Conflicts with existing package
- **解决方案**：
  - 卸载冲突包：`opkg remove <conflict_package>`
  - 强制安装：`opkg install --force-overwrite <package>`

## 6. 高级用法

### 6.1 自定义软件源

1. 创建自定义源配置文件：

   ```bash
   vi /etc/opkg/customfeeds.conf
   ```

2. 添加第三方源（示例）：

   ```bash
   src/gz custom_repo https://myrepo.example.com/packages/mipsel_24kc
   ```

3. 更新源缓存：`opkg update`

### 6.2 离线安装包

- **下载包及依赖**：

  ```bash
  # 下载包到指定目录
  opkg download -d /tmp/packages <package>
  # 下载依赖包
  for dep in $(opkg depends <package> | awk '/Depends:/ {print $2}'); do opkg download -d /tmp/packages $dep; done
  ```

- **离线安装（无网络环境）**：

  ```bash
  opkg install /tmp/packages/*.ipk
  ```

### 6.3 包优先级

当多个源包含同名包时，可通过配置优先级选择优先安装的源：

```bash
# 在源配置中添加优先级（数字越小优先级越高）
src/gz openwrt_core https://xxx.xxx.xxx priority:10
src/gz custom_repo https://yyy.yyy.yyy priority:20
```

## 7. 最佳实践

- **定期更新源**：建议每周执行 `opkg update` 保持源缓存最新
- **谨慎升级系统包**：核心包（如 libc、kernel）升级可能导致系统不稳定
- **备份配置**：修改源配置或安装重要包前，备份 `/etc/config` 和 `/etc/opkg`
- **优先使用官方源**：第三方源可能存在安全风险或兼容性问题
- **清理无用包**：定期卸载不需要的包，释放存储空间

## 8. 总结

- **核心定位**：OPKG 是 OpenWrt 专属的轻量级包管理器，核心用于 .ipk 包的安装、更新、卸载，配置文件位于 `/etc/opkg/` 目录下。
- **基础流程**：使用 OPKG 的核心流程为「更新源缓存（opkg update）→ 操作包（安装 / 卸载 / 升级）→ 验证结果」。
- **常见问题**：源更新失败、依赖缺失、空间不足是最常见问题，可通过检查网络、安装依赖、清理缓存解决。