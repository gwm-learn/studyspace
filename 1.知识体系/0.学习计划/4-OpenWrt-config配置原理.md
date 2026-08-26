# OpenWrt Config 配置原理：Target、Subtarget、Package 三级配置逻辑

## 任务声明

- **任务**: 分析并输出 OpenWrt 构建系统三级配置逻辑（Target、Subtarget、Package）
- **交付物**: Markdown 文档 `openwrt-config-system.md`
- **涉及范围**: `/home/gwm/code/openwrt` 的构建系统配置相关文件
- **验证方式**: 生成可读的 Markdown 文档供学习参考
- **风险偏好**: 快速迭代

---

## Phase 0 · Fact-Check 结论

### 目录结构映射

原文件路径 `/Volumes/ubuntu/code/openwrt` 对应本环境 `/home/gwm/code/openwrt`。

### 关键文件清单

| 文件路径 | 用途 |
|---------|------|
| `target/linux/<board>/Makefile` | Target 主定义 |
| `target/linux/<board>/<subtarget>/target.mk` | Subtarget 定义 |
| `target/linux/Makefile` | Target 构建入口 |
| `package/Makefile` | Package 收集入口 |
| `include/target.mk` | Target/Subtarget 构建逻辑核心（第 66-91 行定义 subtarget 选择） |
| `include/toplevel.mk` | 顶层构建编排，`prepare-tmpinfo` 生成配置 |
| `scripts/target-metadata.pl` | 生成 `tmp/.config-target.in` |
| `scripts/package-metadata.pl` | 生成 `tmp/.config-package.in` |

### 三级配置层次结构

```
Config.in (根配置)
├── target/Config.in
│   └── tmp/.config-target.in  ← 由 scripts/target-metadata.pl 生成
│       ├── Target System (choice) - CONFIG_TARGET_<board>
│       ├── Subtarget (choice)   - CONFIG_TARGET_<board>_<subtarget>
│       └── Target Profile (choice)
└── tmp/.config-package.in  ← 由 scripts/package-metadata.pl 生成
    └── CONFIG_PACKAGE_<package>
```

---

## 概述

OpenWrt 的配置系统采用三级分层架构：**Target（目标平台） → Subtarget（子平台） → Package（软件包）**。这三层配置共同决定了固件的功能特性和适用硬件。

## 目录结构

```
/home/gwm/code/openwrt/
├── target/linux/<board>/           # Target 定义目录
│   ├── Makefile                   # Target 主配置
│   ├── <subtarget1>/target.mk     # Subtarget 1 配置
│   ├── <subtarget2>/target.mk     # Subtarget 2 配置
│   └── ...
├── package/                        # Package 源码目录
│   └── <category>/<package>/
│       └── Makefile               # Package 构建定义
└── include/                        # 构建系统核心文件
    ├── target.mk                   # Target/Subtarget 构建逻辑
    ├── package.mk                  # Package 构建逻辑
    └── toplevel.mk                 # 顶层构建编排
```

---

## 第一级：Target（目标平台）

### 定义位置

`target/linux/<board>/Makefile`

### 核心变量

| 变量 | 含义 | 示例 (x86) |
|------|------|-----------|
| `ARCH` | CPU 架构 | `i386`, `mips`, `arm` |
| `BOARD` | 主板/平台名称 | `x86`, `ath79`, `mediatek` |
| `BOARDNAME` | 显示名称 | `Atheros ATH79` |
| `SUBTARGETS` | 可用子平台列表 | `generic legacy geode 64` |
| `KERNEL_PATCHVER` | 内核版本 | `6.12` |
| `FEATURES` | 特性标志 | `squashfs ext4 pci usb` |
| `DEFAULT_PACKAGES` | 默认包含的软件包 | `dropbear netifd uci` |

### 实际示例

**target/linux/x86/Makefile:**
```makefile
include $(TOPDIR)/rules.mk

ARCH:=i386
BOARD:=x86
BOARDNAME:=x86
FEATURES:=squashfs ext4 vdi vmdk vhdx pcmcia targz fpu boot-part rootfs-part
SUBTARGETS:=generic legacy geode 64

KERNEL_PATCHVER:=6.12
KERNELNAME:=bzImage

include $(INCLUDE_DIR)/target.mk

DEFAULT_PACKAGES += \
    partx-utils mkf2fs \
    e2fsprogs kmod-button-hotplug \
    grub2-bios-setup

$(eval $(call BuildTarget))
```

**target/linux/ath79/Makefile:**
```makefile
ARCH:=mips
BOARD:=ath79
BOARDNAME:=Atheros ATH79
CPU_TYPE:=24kc
SUBTARGETS:=generic mikrotik nand tiny

FEATURES:=ramdisk squashfs usbgadget
KERNEL_PATCHVER:=6.12

include $(INCLUDE_DIR)/target.mk
```

### 配置变量命名

Target 配置生成 `CONFIG_TARGET_<board>` 变量：
- `CONFIG_TARGET_x86=y`
- `CONFIG_TARGET_ath79=y`

---

## 第二级：Subtarget（子平台）

### 定义位置

`target/linux/<board>/<subtarget>/target.mk`

### 核心变量

| 变量 | 含义 |
|------|------|
| `BOARDNAME` | 子平台显示名称 |
| `CPU_TYPE` | CPU 类型（可选） |
| `FEATURES` | 附加特性（会合并到父级） |
| `DEFAULT_PACKAGES` | 附加默认软件包 |
| `Target/Description` | 描述文本 |

### 实际示例

**target/linux/ath79/generic/target.mk:**
```makefile
BOARDNAME:=Generic

DEFAULT_PACKAGES += wpad-basic-mbedtls

define Target/Description
    Build firmware images for generic Atheros AR71xx/AR913x/AR934x based boards.
endef
```

**target/linux/ath79/tiny/target.mk:**
```makefile
BOARDNAME:=Devices with small flash
FEATURES += low_mem small_flash

DEFAULT_PACKAGES += wpad-basic-mbedtls

define Target/Description
    Build firmware images for Atheros AR71xx/AR913x/AR934x based boards with small flash
endef
```

**target/linux/x86/64/target.mk:**
```makefile
ARCH:=x86_64
BOARDNAME:=x86_64

define Target/Description
        Build images for 64 bit systems including virtualized guests.
endef
```

### subtarget 选择机制

`include/target.mk` 中的关键逻辑（第 66-91 行）：

```makefile
# 第 69 行：根据 CONFIG_TARGET_<board>_<subtarget> 选择活跃 subtarget
SUBTARGET:=$(strip $(foreach subdir,$(patsubst $(PLATFORM_DIR)/%/target.mk,%,$(wildcard $(PLATFORM_DIR)/*/target.mk)),\
    $(if $(CONFIG_TARGET_$(call target_conf,$(BOARD)_$(subdir))),$(subdir))))

# 第 77 行：生成 TARGETID
TARGETID:=$(BOARD)$(if $(SUBTARGET),/$(SUBTARGET))
# 例如：ath79/generic, x86/64
```

### 配置变量命名

Subtarget 配置生成 `CONFIG_TARGET_<board>_<subtarget>` 变量：
- `CONFIG_TARGET_ath79_generic=y`
- `CONFIG_TARGET_ath79_tiny=y`
- `CONFIG_TARGET_x86_64=y`

---

## 第三级：Package（软件包）

### 定义位置

`package/<category>/<name>/Makefile`

### Package 构建宏

每个 Package 通过 `BuildPackage` 宏定义：

```makefile
include $(TOPDIR)/include/package.mk

define Package/<name>
    SECTION:=net
    CATEGORY:=Network
    TITLE:=Package description
    DEPENDS:=+libc +BUSYBOX_CONFIG_xxx
endef

define BuildPackage
    $(call Build/DefaultTargets)
    # 包含安装脚本等
endef
```

### 实际示例

**package/network/config/netifd/Makefile 片段:**
```makefile
include $(TOPDIR)/include/package.mk

define Package/netifd
    SECTION:=net
    CATEGORY:=Network
    TITLE:=Network interface daemon
    DEPENDS:=+libubox +libubus +libuci +libnl-tiny +jshn
endef

define BuildPackage
    $(call Build/DefaultTargets)
    $(call Build/InstallDev,$(1))
endef
```

### 配置变量命名

Package 配置生成 `CONFIG_PACKAGE_<package>` 变量：
- `CONFIG_PACKAGE_netifd=y`
- `CONFIG_PACKAGE_luci=y`
- `CONFIG_PACKAGE_dnsmasq=m` (m = 模块/编译但不固守)

---

## 配置生成流程

### 1. 菜单配置准备

`toplevel.mk` 中定义 `prepare-tmpinfo` 目标：

```makefile
prepare-tmpinfo: FORCE
    # 扫描 target 定义生成 tmp/.targetinfo
    $(_SINGLE)$(NO_TRACE_MAKE) -j1 -r -s -f include/scan.mk \
        SCAN_TARGET="targetinfo" SCAN_DIR="target/linux" SCAN_NAME="target" \
        SCAN_DEPTH=3 SCAN_EXTRA="" SCAN_MAKEOPTS="TARGET_BUILD=1"
    
    # 扫描 package 定义生成 tmp/.packageinfo
    $(_SINGLE)$(NO_TRACE_MMAKE) -j1 -r -s -f include/scan.mk \
        SCAN_TARGET="packageinfo" SCAN_DIR="package" SCAN_NAME="package" \
        SCAN_DEPTH=5 SCAN_EXTRA=""

    # 生成最终 .config-* 文件供 menuconfig 使用
    ./scripts/target-metadata.pl config tmp/.targetinfo > tmp/.config-target.in
    ./scripts/package-metadata.pl config tmp/.packageinfo > tmp/.config-package.in
```

### 2. 配置层次结构

```
Config.in (根配置)
├── target/Config.in
│   └── tmp/.config-target.in  ← 由 scripts/target-metadata.pl 生成
│       ├── Target System (choice)
│       │   ├── TARGET_x86
│       │   ├── TARGET_ath79
│       │   └── ...
│       ├── Subtarget (choice, 条件显示)
│       │   ├── TARGET_ath79_generic
│       │   ├── TARGET_ath79_tiny
│       │   ├── TARGET_ath79_mikrotik
│       │   └── ...
│       └── Target Profile (choice)
│           ├── TARGET_ath79_generic_Default (示例)
│           └── ...
├── config/Config-images.in
├── config/Config-build.in
├── config/Config-devel.in
├── toolchain/Config.in
├── target/imagebuilder/Config.in
├── target/sdk/Config.in
├── target/toolchain/Config.in
└── tmp/.config-package.in  ← 由 scripts/package-metadata.pl 生成
    └── Package 配置项 (CONFIG_PACKAGE_*)
```

### 3. scripts/target-metadata.pl 解析

关键处理逻辑（简化自 `scripts/target-metadata.pl` 第 79-216 行）：

```perl
# 生成 Target System choice
print <<EOF;
choice
    prompt "Target System"
    default TARGET_mediatek
    reset if !DEVEL
EOF

# 遍历所有 Target（跳过 subtarget）
foreach my $target (@target_sort) {
    next if $target->{subtarget};  # 跳过 subtarget
    print_target($target);
}
print "endchoice\n";

# 生成 Subtarget choice（仅当存在 subtargets 时）
if (@subtargets > 0) {
    print <<EOF;
choice
    prompt "Subtarget" if HAS_SUBTARGETS
EOF
    # 生成 subtarget 选项
    foreach my $target (@target) {
        next unless $target->{subtarget};
        print_target($target);
    }
    print "endchoice\n";
}
```

---

## .config 文件中的配置变量

### Target 级别变量

```bash
# 目标平台
CONFIG_TARGET_x86=y
CONFIG_TARGET_x86_64=y
CONFIG_TARGET_ath79=y

# Subtarget
CONFIG_TARGET_ath79_generic=y
CONFIG_TARGET_ath79_tiny=y

# Target Profile
CONFIG_TARGET_ath79_generic_Default=y
```

### Architecture 级别变量

```bash
CONFIG_ARCH=x86_64
CONFIG_ARCH_HAS_SUBTARGETS=y
CONFIG_HAS_SUBTARGETS=y
```

### Package 级别变量

```bash
CONFIG_PACKAGE_netifd=y
CONFIG_PACKAGE_dnsmasq=y
CONFIG_PACKAGE_firewall4=y
CONFIG_PACKAGE_uci=y
CONFIG_PACKAGE_luci=y
CONFIG_PACKAGE_kmod-ath9k=m
```

---

## 构建依赖关系

### 编译顺序（来自根 Makefile 第 46-51 行）

```makefile
# 编译顺序
$(toolchain/stamp-compile): $(tools/stamp-compile) ...
$(target/stamp-compile): $(toolchain/stamp-compile) $(tools/stamp-compile) $(BUILD_DIR)/.prepared
$(package/stamp-compile): $(target/stamp-compile) ...
$(package/stamp-install): $(package/stamp-compile)
$(target/stamp-install): $(package/stamp-compile) $(package/stamp-install)
```

### Target 与 Subtarget 的 include 顺序

`include/target.mk` 第 82-85 行：

```makefile
include $(PLATFORM_DIR)/Makefile           # 先包含 Target 主 Makefile
ifneq ($(PLATFORM_DIR),$(PLATFORM_SUBDIR))
    include $(PLATFORM_SUBDIR)/target.mk    # 再包含 Subtarget 的 target.mk
endif
```

这允许 subtarget 的 `target.mk` 覆盖/扩展父级设置。

---

## 配置示例流程

### 选择 Target: ath79

1. **选择 Target System** → `CONFIG_TARGET_ath79=y`
2. **选择 Subtarget** → `CONFIG_TARGET_ath79_generic=y` 或 `CONFIG_TARGET_ath79_tiny=y`
3. **编译时**：
   - `PLATFORM_DIR = target/linux/ath79`
   - `SUBTARGET = generic` (根据 CONFIG_TARGET_ath79_generic)
   - `PLATFORM_SUBDIR = target/linux/ath79/generic`
   - 读取 `target/linux/ath79/Makefile` + `target/linux/ath79/generic/target.mk`

### 选择 Package: netifd

1. **menuconfig 中选中** → `CONFIG_PACKAGE_netifd=y`
2. **编译时**：根据 `package/Makefile` 中的 `package-y += netifd` 编译
3. **安装时**：生成 `.ipk` 包或在镜像中安装

---

## 关键文件清单

| 文件路径 | 用途 |
|---------|------|
| `target/linux/<board>/Makefile` | Target 主定义 |
| `target/linux/<board>/<subtarget>/target.mk` | Subtarget 定义 |
| `target/linux/Makefile` | Target 构建入口 |
| `package/Makefile` | Package 收集入口 |
| `package/<pkg>/Makefile` | 单个 Package 定义 |
| `include/target.mk` | Target/Subtarget 构建逻辑 |
| `include/package.mk` | Package 构建宏定义 |
| `include/toplevel.mk` | 顶层构建编排 |
| `include/scan.mk` | 信息扫描逻辑 |
| `scripts/target-metadata.pl` | 生成 .config-target.in |
| `scripts/package-metadata.pl` | 生成 .config-package.in |
| `scripts/config/conf` | 配置工具 |
| `scripts/config/mconf` | menuconfig UI |

---

## 总结

OpenWrt 配置系统的三级分层：

1. **Target（目标平台）**：定义硬件架构和基本平台特性
2. **Subtarget（子平台）**：定义同一平台下的不同变体（如 generic/tiny）
3. **Package（软件包）**：定义可选择安装的应用/库/内核模块

这三层配置通过 `include/target.mk` 的宏和 `scripts/target-metadata.pl` 的代码生成逻辑有机结合，最终通过 `.config` 文件中的 `CONFIG_TARGET_*` 和 `CONFIG_PACKAGE_*` 变量控制整个固件编译过程。
