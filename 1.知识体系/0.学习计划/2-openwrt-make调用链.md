# OpenWrt Make 调用链详解

> 本文基于本机源码树 `/Volumes/ubuntu/code/openwrt`（`openwrt-25.12` 分支，commit `4a5c6b90d2`）逐文件梳理。
> 覆盖三条命令：`make defconfig`、`make menuconfig`、`make V=s`。
> 所有行号均指当前源码树中的实际位置，可直接对照阅读。

---

## 0. 总览：先理解两个关键设计

### 0.1 顶层 Makefile 的"两段式"设计（`OPENWRT_BUILD`）

顶层 `Makefile`（143 行）是一个 **分叉入口**，靠 `OPENWRT_BUILD` 环境变量区分两阶段：

```makefile
# Makefile 第 22-41 行
ifneq ($(OPENWRT_BUILD),1)
  _SINGLE=export MAKEFLAGS=$(space);          # 清空 MAKEFLAGS，防止递归 make 时参数传染
  override OPENWRT_BUILD=1                     # 强制置 1 并 export
  export OPENWRT_BUILD
  include $(TOPDIR)/include/debug.mk           # 调试宏（DEBUG=d/t/l/r/v）
  include $(TOPDIR)/include/depends.mk         # 子树依赖检测（timestamp.pl）
  include $(TOPDIR)/include/toplevel.mk        # ← 顶层目标全部定义在这里
else
  include rules.mk                             # 全局变量/宏（最重要的基础设施）
  include $(INCLUDE_DIR)/depends.mk
  include $(INCLUDE_DIR)/subdir.mk             # 子目录递归/进度输出
  include target/Makefile                      # 目标系统（linux/sdk/imagebuilder/toolchain/llvm-bpf）
  include package/Makefile                     # 软件包（含 feeds 包）
  include tools/Makefile                       # 主机工具
  include toolchain/Makefile                   # 交叉工具链
  ...
endif
```

- **阶段 A（`OPENWRT_BUILD≠1`）**：你第一次敲 `make xxx` 时进入。只负责"配置类"目标
  （`menuconfig`/`defconfig`/`oldconfig`/`clean`…）以及**转发**构建目标到阶段 B。
- **阶段 B（`OPENWRT_BUILD=1`）**：所有递归进来的子 make。此时才有 `.config` 解析
  （`rules.mk` 第 10 行 `-include $(TOPDIR)/.config`）、四个大子系统的 Makefile。

> 判断依据：任何递归 `$(SUBMAKE)` 都带着 `OPENWRT_BUILD=1` 环境变量，进入 Makefile 后走 `else` 分支。

### 0.2 `include/toplevel.mk`：配置类命令的真正大本营

`make defconfig / menuconfig` 的规则**不在根 Makefile**，而在 `include/toplevel.mk`：

```makefile
# toplevel.mk 第 120-140 行
defconfig: scripts/config/conf prepare-tmpinfo FORCE
	touch .config
	@if [ ! -s .config -a -e $(HOME)/.openwrt/defconfig ]; then cp $(HOME)/.openwrt/defconfig .config; fi
	[ -L .config ] && export KCONFIG_OVERWRITECONFIG=1; \
		$< $(KCONF_FLAGS) --defconfig=.config Config.in

menuconfig: scripts/config/mconf prepare-tmpinfo FORCE
	if [ \! -e .config -a -e $(HOME)/.openwrt/defconfig ]; then \
		cp $(HOME)/.openwrt/defconfig .config; \
	fi
	[ -L .config ] && export KCONFIG_OVERWRITECONFIG=1; \
		$< Config.in
```

两条命令有 **两个共同前置**：

1. `scripts/config/conf` 或 `scripts/config/mconf` —— kconfig 命令行工具本体（C 程序）；
2. `prepare-tmpinfo` —— 扫描全部包/目标，生成 Kconfig 树所需的 `tmp/.config-*.in`。

下面分别展开。

---

## 1. 公共前置一：kconfig 工具链的构建（`conf` / `mconf`）

规则在 `toplevel.mk` 第 106-111 行：

```makefile
scripts/config/%onf: CFLAGS+= -O2
scripts/config/%onf: FORCE
	@$(_SINGLE)$(SUBMAKE) $(if $(findstring s,$(OPENWRT_VERBOSE)),,-s) \
		-C scripts/config $(notdir $@)
```

- 模式规则 `%onf` 同时匹配 `conf`、`mconf`、`nconf`、`qconf`。
- 递归执行 `$(SUBMAKE) -C scripts/config conf`（注意此时 `OPENWRT_BUILD=1`，走阶段 B 但只加载了 rules.mk）。

`scripts/config/Makefile` 是一个 **裁剪版 Linux Kbuild 内核配置系统**：

```makefile
# scripts/config/Makefile
common-objs	:= confdata.o expr.o lexer.lex.o menu.o parser.tab.o \
		   preprocess.o symbol.o util.o            # 公共部分（Kconfig 解析器）

hostprogs	+= conf                                  # conf: conf.o + common-objs
hostprogs	+= mconf                                 # mconf: mconf.o + lxdialog/*.o + common-objs（ncurses）
hostprogs	+= nconf / qconf                          # nconfig / xconfig 用

Q:=$(if $V,,@)         # ← 关键：命令行变量 V 在这里被消费！
quiet:=$(if $V,,_silent)
```

> 注意 `Q:=$(if $V,,@)`：**`V=s` 同样会让 conf/mconf 自身的编译命令回显**，这是 `V=s` 生效的最底层机制之一。

`mconf` 额外依赖 `lxdialog/`（ncurses 对话框库）和 `mconf-cflags/mconf-libs`（由 `mconf-cfg.sh` 探测生成）。

---

## 2. 公共前置二：`prepare-tmpinfo` —— 元数据扫描与 Kconfig 树生成

`toplevel.mk` 第 78-92 行。这是配置系统最核心、也最容易被忽略的一步：

```makefile
prepare-tmpinfo: FORCE
	@+$(MAKE) -r -s $(STAGING_DIR_HOST)/.prereq-build $(PREP_MK)   # ① 主机环境自检
	mkdir -p tmp/info feeds                                          # ② 目录
	[ -e $(TOPDIR)/feeds/base ] || ln -sf ../package $(TOPDIR)/feeds/base
	$(_SINGLE)$(NO_TRACE_MAKE) -j1 -r -s -f include/scan.mk \        # ③ 扫描软件包
		SCAN_TARGET="packageinfo" SCAN_DIR="package" ...
	$(_SINGLE)$(NO_TRACE_MAKE) -j1 -r -s -f include/scan.mk \        # ④ 扫描目标平台
		SCAN_TARGET="targetinfo" SCAN_DIR="target/linux" ...
	for type in package target; do \                                 # ⑤ 生成 Kconfig 片段
		f=tmp/.$${type}info; t=tmp/.config-$${type}.in; \
		[ "$$t" -nt "$$f" ] || ./scripts/$${type}-metadata.pl config "$$f" > "$$t" || ...; \
	done
	[ tmp/.config-feeds.in -nt tmp/.packageauxvars ] || ./scripts/feeds feed_config > tmp/.config-feeds.in
	./scripts/package-metadata.pl mk tmp/.packageinfo > tmp/.packagedeps || ...   # ⑥ 包依赖图
	./scripts/package-metadata.pl pkgaux tmp/.packageinfo > tmp/.packageauxvars || ...
	./scripts/package-metadata.pl usergroup tmp/.packageinfo > tmp/.packageusergroup || ...
	touch $(TOPDIR)/tmp/.build                                                 # ⑦ 里程碑
```

逐步拆解：

### ① 主机环境自检：`$(STAGING_DIR_HOST)/.prereq-build`

规则在 `toplevel.mk` 第 181-193 行，内容为：

```makefile
$(STAGING_DIR_HOST)/.prereq-build: include/prereq-build.mk
	mkdir -p tmp
	@$(_SINGLE)$(NO_TRACE_MAKE) -j1 -r -s -f $(TOPDIR)/include/prereq-build.mk prereq ...
  ifneq ($(realpath $(TOPDIR)/include/prepare.mk),)
	@$(_SINGLE)$(NO_TRACE_MAKE) -j1 -r -s -f $(TOPDIR)/include/prepare.mk prepare ...
  endif
	touch $@
```

- `include/prereq-build.mk` 定义了一大串 `TestHostCommand` / `SetupHostCommand` 检查：
  GNU make ≥ 4.1、大小写敏感文件系统、umask 022、gcc/g++ ≥ 8、ncurses、git、rsync、
  Perl 5.x + 若干模块、Python ≥ 3.7、GNU tar/find/patch/awk/grep、`staging_dir/host/bin/mkhash`（直接由 `scripts/mkhash.c` 编译）等。
- 机制来自 `include/prereq.mk`：每个检查生成 `prereq-<name>` 目标，失败写入
  `tmp/.prereq-error`，最后由 `prereq:` 规则汇总报错。
- 本源码树**没有** `include/prepare.mk`（`toplevel.mk` 用 `realpath` 探测），故 prepare 段被跳过。

### ③④ 扫描：`include/scan.mk`（DUMP 模式）

`prepare-tmpinfo` 以 `-f include/scan.mk` 方式独立运行 make，关键逻辑：

```makefile
# scan.mk
FILELIST:=$(TMP_DIR)/info/.files-$(SCAN_TARGET)-$(SCAN_COOKIE)
$(FILELIST):
	find -L $(SCAN_DIR) -mindepth 1 $(if $(SCAN_DEPTH),-maxdepth $(SCAN_DEPTH)) $(SCAN_EXTRA) \
		-name Makefile | xargs grep -aHE 'call $(GREP_STRING)' | ... > $@
```

1. `find` 找出 `package/`（深度 5）或 `target/linux/`（深度 3）下所有含
   `BuildPackage` / `KernelPackage` / `BuildTarget` 调用的 `Makefile`；
2. 对每个目录执行 `make DUMP=1 FEED=... -C <dir>` —— **以 DUMP 模式运行包 Makefile**，
   `include/package.mk` 中 `$(if $(DUMP),$(Dumpinfo/Package),...)` 会让包把自己的
   `TITLE/DEPENDS/PROVIDES/CATEGORY/…` 元数据**打印到 stdout**；
3. 所有输出合并成 `tmp/.packageinfo` / `tmp/.targetinfo` 两个大文本文件。

> 这也是 `make menuconfig` 首次运行较慢的原因——它要逐个目录 fork make 采集元数据。

### ⑤ 生成 Kconfig 片段（为什么默认是 OpenWrt One）

`scripts/target-metadata.pl config tmp/.targetinfo > tmp/.config-target.in`
会把每个目标写成 `choice` 里的一个 `config TARGET_xxx`，**并且带 `default`**：

```makefile
# 由 target-metadata.pl 第 182-188 行生成（上游提交 5c12fe45b9 改的默认值）
choice
	prompt "Target System"
	default TARGET_mediatek      # ← 默认 Target System 是 mediatek
	reset if !DEVEL
```

配合 `target/linux/mediatek/filogic/target.mk` 的 `DEFAULT_PROFILE:=openwrt_one`，
就解释了上一份文档的结论：**全新源码树 `make menuconfig` 默认选中 OpenWrt One**。

同理 `package-metadata.pl config tmp/.packageinfo > tmp/.config-package.in` 生成全部包
（`menu "Administration"`、`config PACKAGE_xxx`…）。

### ⑥ 包依赖图

`tmp/.packagedeps` 是关键产物：`package/Makefile` 第 13 行 `-include $(TMP_DIR)/.packagedeps`，
它定义了 `package-$(y/m/n)` 列表（哪些包要编、哪些编成模块、哪些不编），
以及 `$(curdir)/builddirs` —— **决定 `make V=s` 时实际编译哪些包**。

### ⑦ `tmp/.build`

`touch tmp/.build` 是一个全局"配置已完成"里程碑，后面所有 stamp 文件都以它为依赖
（见 `subdir.mk` 的 `stampfile` 定义）。

### Kconfig 树全貌（conf/mconf 解析的 `Config.in`）

顶层 `Config.in`（40 行）把这些片段串成完整配置树：

```
Config.in
├── target/Config.in          ──source──►  tmp/.config-target.in    （目标平台/子目标/Profile）
├── config/Config-images.in                （固件镜像格式 squashfs/ext4/…）
├── config/Config-build.in                 （构建选项：编译器/调试/签名…）
├── config/Config-devel.in                 （开发者选项）
├── toolchain/Config.in                    （GCC/glibc|musl/外部工具链）
├── target/imagebuilder/Config.in
├── target/sdk/Config.in
├── target/toolchain/Config.in
└── tmp/.config-package.in     ──►        （全部软件包 PACKAGE_xxx）
```

---

## 3. `make defconfig` 调用链

```
make defconfig
└─ Makefile（OPENWRT_BUILD≠1 分支）
   ├─ override OPENWRT_BUILD=1
   ├─ include include/debug.mk / depends.mk / toplevel.mk
   └─ 目标 defconfig（toplevel.mk:120）
      ├─ 依赖1 scripts/config/conf
      │   └─ $(SUBMAKE) -C scripts/config conf      （编译 kconfig C 程序）
      ├─ 依赖2 prepare-tmpinfo（见 §2，全量元数据扫描）
      │   └─ tmp/.config-target.in / .config-package.in / .config-feeds.in
      │       tmp/.packagedeps / .packageauxvars / .packageusergroup
      ├─ touch .config                              （保证文件存在）
      ├─ [可选] cp ~/.openwrt/defconfig .config     （空配置且有用户默认时）
      └─ ./scripts/config/conf --defconfig=.config Config.in
          ├─ conf_parse("Config.in")       ← 解析整个 Kconfig 树（含全部 default 语句）
          ├─ conf_read(".config")          ← 把现有 .config 当"补丁"读入，标记为已选
          ├─ conf_set_all_new_symbols(def_default)  ← 其余符号全部取 Kconfig 默认值
          └─ conf_write(".config")         ← 写出完整的 .config
```

**语义**：`make defconfig` = 在保留当前 `.config` 已选项的前提下，把**所有未配置的符号
填成 Kconfig 默认值**，生成一份"完整可构建"的 `.config`。等价于"安静地完成菜单配置"。
（注意 `--defconfig=.config` 里的文件同时是输入和输出。）

**产出文件**（均在 TOPDIR 下）：
- `.config` —— 完整配置（gitignore 中，约 1-4 万行）；
- `tmp/` 下全部元数据文件；
- `staging_dir/host/` 下的 mkhash、xxd、symlink 工具。

---

## 4. `make menuconfig` 调用链

```
make menuconfig
└─ Makefile（OPENWRT_BUILD≠1 分支）
   ├─ override OPENWRT_BUILD=1
   ├─ include include/debug.mk / depends.mk / toplevel.mk
   └─ 目标 menuconfig（toplevel.mk:135）
      ├─ 依赖1 scripts/config/mconf
      │   └─ $(SUBMAKE) -C scripts/config mconf
      │       ├─ 探测 ncurses：mconf-cfg.sh → mconf-cflags / mconf-libs
      │       └─ 编译 mconf.o + lxdialog/*.o + common-objs
      ├─ 依赖2 prepare-tmpinfo（同 §2）
      ├─ [可选] cp ~/.openwrt/defconfig .config   （仅当 .config 不存在时）
      └─ ./scripts/config/mconf Config.in
          ├─ conf_parse("Config.in")      ← 解析同一棵 Kconfig 树
          ├─ conf_read(".config")         ← 载入现有配置（不存在则全默认）
          ├─ conf(&rootmenu)              ← 进入 ncurses 交互循环
          │     ↑ 上下左右/空格选择、/? 搜索、依赖高亮（symbols 的 depends/select）
          └─ 退出时 conf_write(".config") ← 用户选择落盘
```

**与 defconfig 的本质区别**：解析同一棵树，但符号值由**人**在 ncurses 界面里决定，
而非程序取默认值。此外 `menuconfig` 走 `lxdialog`（`mconf.o`），`nconfig` 走 `nconf.gui`。

**自动触发场景**：如果你跳过配置直接 `make`（阶段 A 的 `prereq` → `.config` 规则），
`toplevel.mk` 第 94-98 行会在 `.config` 不存在或无效时**自动拉起 menuconfig**：

```makefile
.config: ./scripts/config/conf $(if $(CONFIG_HAVE_DOT_CONFIG),,prepare-tmpinfo)
	@+if [ \! -e .config ] || ! grep CONFIG_HAVE_DOT_CONFIG .config >/dev/null; then \
		[ -e $(HOME)/.openwrt/defconfig ] && cp $(HOME)/.openwrt/defconfig .config; \
		$(_SINGLE)$(NO_TRACE_MAKE) menuconfig $(PREP_MK); \
	fi
```

`CONFIG_HAVE_DOT_CONFIG` 定义于顶层 `Config.in:12`（`bool, default y`），所以一份合法
`.config` 必然含它，以此判断"配置是否已完成"。

---

## 5. `make V=s`（完整构建）调用链

`make V=s` 省略目标名 → 默认目标为根 Makefile 第一个目标 `world`。

### 5.1 阶段 A：顶层预处理（toplevel.mk 的 `%::` 万能规则）

`world` 在阶段 A 是空规则（无配方），于是由 `toplevel.mk` 第 230-242 行的**模式规则 `%::`** 接管：

```makefile
%:::
	@+$(PREP_MK) $(NO_TRACE_MAKE) -r -s prereq
	@( \
		cp .config tmp/.config; \
		./scripts/config/conf $(KCONF_FLAGS) --defconfig=tmp/.config -w tmp/.config Config.in > /dev/null 2>&1; \
		if ./scripts/kconfig.pl '>' .config tmp/.config | grep -q CONFIG; then \
			printf "$(_R)WARNING: your configuration is out of sync. Please run make menuconfig, oldconfig or defconfig!$(_N)\n" >&2; \
		fi \
	)
	@+$(ULIMIT_FIX) $(SUBMAKE) -r $@ $(if $(WARN_PARALLEL_ERROR), || {...})
```

- `$(PREP_MK)= OPENWRT_BUILD= QUIET=0`：把子 make 的 `OPENWRT_BUILD` 清空，使其再次走阶段 A 逻辑。
- **步骤 1**：`prereq`。见 `toplevel.mk:210`：`prereq:: prepare-tmpinfo .config` ——
  也就是**构建前必然先做一遍 §2 的元数据扫描**，并确保 `.config` 存在（没有就弹 menuconfig）。
- **步骤 2**：配置同步检查。用 `conf --defconfig` 求一遍"理想配置"，与当前 `.config`
  diff（`scripts/kconfig.pl '>'`），不一致就打印经典的 **"configuration is out of sync"** 警告。
- **步骤 3**：`$(SUBMAKE) -r world` → 带着 `OPENWRT_BUILD=1` 进入**阶段 B**。

> `make -w` 是 `V=s` 下 `SUBMAKE` 的形态（见 §6），会打印 Entering/Leaving directory。

### 5.2 阶段 B：真正的构建（OPENWRT_BUILD=1）

进入 `else` 分支（Makefile:34-41），加载 `rules.mk` + 四个子系统 Makefile。然后执行：

```makefile
# Makefile:130-139（阶段 B）
prepare: .config $(tools/stamp-compile) $(toolchain/stamp-compile)
	$(_SINGLE)$(SUBMAKE) -r buildinfo

world: prepare $(target/stamp-compile) $(package/stamp-compile) \
	       $(package/stamp-install) $(target/stamp-install) FORCE
	$(_SINGLE)$(SUBMAKE) -r package/index
	$(_SINGLE)$(SUBMAKE) -r json_overview_image_info
	$(_SINGLE)$(SUBMAKE) -r checksum

# 跨子系统依赖（Makefile:46-50）
$(toolchain/stamp-compile): $(tools/stamp-compile) ...
$(target/stamp-compile):   $(toolchain/stamp-compile) $(tools/stamp-compile) $(BUILD_DIR)/.prepared
$(package/stamp-compile):  $(target/stamp-compile) $(package/stamp-cleanup)
$(package/stamp-install):  $(package/stamp-compile)
$(target/stamp-install):   $(package/stamp-compile) $(package/stamp-install)
```

**依赖拓扑**（make 自动按此顺序推进）：

```
tools/stamp-compile ──► toolchain/stamp-compile ──► target/stamp-compile ──► package/stamp-compile
                                                          ▲                        │
                                                          │                        ▼
                                                          └──────────────┬──── package/stamp-install
                                                                         │        │
                                                                 target/stamp-install ◄─┘
                                                                         │
                                                        package/index → json_overview_image_info → checksum
```

### 5.3 stamp 机制：`include/subdir.mk`

stamp 是 OpenWrt 的"增量构建"核心。以 `target/stamp-compile` 为例（target/Makefile:25）：

```makefile
$(eval $(call stampfile,$(curdir),target,compile,$(TMP_DIR)/.build))
# 展开为（subdir.mk:92-108）：
staging_dir/<target>/stamp/.target_compile: $(TMP_DIR)/.build <其他依赖>
	@+$(SCRIPT_DIR)/timestamp.pl -n <stamp> target <依赖> || \
		$(MAKE) <flags> target/compile      # ← 依赖文件比 stamp 新，才真正编译
	@mkdir -p $(dir <stamp>)
	@touch <stamp>
```

`timestamp.pl` 比较 stamp 与所有依赖的 mtime：**只有依赖更新才重编**，否则跳过并 touch。

### 5.4 子目录递归：`subdir.mk` 的 `subdir` 宏

`target/Makefile` 等通过 `$(eval $(call subdir,$(curdir)))` 生成层层转发目标，例如：

- `target/compile: target/linux/compile target/sdk/compile ...`（依 `builddirs` 而定）
- `target/linux/compile:`
  ```makefile
  # subdir.mk:43-50 log_make 生成：
  @+$(SCRIPT_DIR)/time.pl "time: target/linux/compile" \
      $(SUBMAKE) -r -C target/linux BUILD_SUBDIR="target/linux" compile
  ```
- `target/linux/Makefile` 再转发：
  ```makefile
  # target/linux/Makefile:11-12
  compile: FORCE
	@+$(NO_TRACE_MAKE) -C $(firstword $(wildcard feeds/$(BOARD) $(BOARD))) $@
  ```
  即 `make -C target/linux/mediatek compile`（BOARD=mediatek 时）。

### 5.5 内核构建：`include/kernel-build.mk`

在 `target/linux/mediatek/` 下（TARGET_BUILD=1，target.mk:390-391 `BuildTarget?=$(BuildKernel)`）：

```makefile
# kernel-build.mk 第 84-187 行（BuildKernel 宏）
$(STAMP_PREPARED):      $(DL_DIR)/$(LINUX_SOURCE)      → Kernel/Prepare   （解压+打补丁）
$(STAMP_CONFIGURED):    $(STAMP_PREPARED) ... .config  → Kernel/Configure （scripts/config 合并生成 .config）
$(LINUX_DIR)/.modules:  $(STAMP_CONFIGURED) ...        → Kernel/CompileModules（make modules）
$(LINUX_DIR)/.image:    $(STAMP_CONFIGURED) ...        → Kernel/CompileImage（vmlinux+Image）
compile:  $(LINUX_DIR)/.modules
	+$(MAKE) -C image compile TARGET_BUILD=
install:  $(LINUX_DIR)/.image
	+$(MAKE) -C image compile install TARGET_BUILD=
```

- 内核源码下载到 `dl/`，解压到 `build_dir/target-*/linux-*/`；
- 内核 `.config` 由 `scripts/kconfig.pl` 合并 `target/linux/generic/config-6.12`
  + `target/linux/mediatek/config-6.12` + 子目标配置而成（`target.mk` 的 `LINUX_CONF_CMD`）；
- 实际编译走 `$(KERNEL_MAKE)`（即 build_dir 内的 `make -C linux-... ARCH=... CROSS_COMPILE=...`）。

### 5.6 固件镜像生成：`include/image.mk`

`target/linux/<board>/image/Makefile` 最后 `$(eval $(call BuildImage))`（image.mk:965）：

```makefile
kernel_prepare: image_prepare
	$(call Image/Build/targz)            # 内核 tar.gz/cpio.gz 打包
	$(call Image/BuildKernel)            # 生成 uImage/FIT 等内核镜像
	$(call Image/InstallKernel)

install-images: kernel_prepare $(foreach fs,...,$(KDIR)/root.$(fs))
	$(foreach fs,$(TARGET_FILESYSTEMS),$(call Image/Build,$(fs)))   # squashfs/ext4/…
install: install-images
	$(call Image/Manifest)               # 生成 profile.json / sha256sums 等
```

每个设备（`Device/openwrt_one` 之类，定义在 `image/*.mk`）通过 `Device/Build`
用各自的 `KERNEL/IMAGES/IMAGE/*` 模板把 kernel+rootfs 拼成最终固件到 `bin/targets/...`。

### 5.7 软件包构建：`include/package.mk` 的 `Build/CoreTargets`

`package/stamp-compile` → `package/compile` → 对每个 `$(curdir)/builddirs`（即所有 `package-y`）
执行 `make -C package/<name> compile`。每个包的 Makefile 经 `BuildPackage` →
`Build/CoreTargets`（package.mk:245-288）生成标准流水线：

```makefile
$(STAMP_PREPARED):   → Build/Prepare    （下载源码、解压、打补丁）   = .prepared_<hash>
$(STAMP_CONFIGURED): → Build/Configure  （./configure / meson / cmake） = .configured_<hash>
$(STAMP_BUILT):      → Build/Compile + Build/Install（make && make install DESTDIR=） = .built
$(STAMP_INSTALLED):  → Build/InstallDev （头文件/库装进 staging_dir，供其他包依赖） = .<pkg>_installed
```

- stamp 路径定义见 package.mk:124-129；`.prepared_$(hash)` 的 hash 由 `find_md5` 对源码目录
  求值——**源码一变，stamp 就失效重编**。
- 包之间依赖由 `tmp/.packagedeps`（`-include` 进来）驱动，如 `STAMP_CONFIGURED_DEPENDS`。

### 5.8 根文件系统与收尾

`package/stamp-install` → `package/install`（package/Makefile:95-126）：
1. `merge` + `merge-index`：把各包产物合并到 `staging_dir/packages/<arch>`，生成
   `Packages` 索引（opkg）或 `packages.adb`（apk）；
2. `opkg/apk install` 把全部选中的 `.ipk/.apk` 装进 `build_dir/target-*/root-<board>/`；
3. `$(call prepare_rootfs,...)`（include/rootfs.mk:71）做系统收尾（mdev、init 脚本、时区等）。

最终 `world` 收尾：`package/index`（重新生成各包目录索引+签名）、`json_overview_image_info`
（`bin/targets/<board>/<subtarget>/profiles.json`）、`checksum`（`sha256sums`）。

---

## 6. `V` 参数机制详解（`include/verbose.mk`，共 72 行）

所有子 make 的"静默/冗长"都由这一个文件控制：

```makefile
# verbose.mk
ifndef OPENWRT_VERBOSE
  OPENWRT_VERBOSE:=
endif
ifeq ("$(origin V)", "command line")    # ← 只认命令行传入的 V
  OPENWRT_VERBOSE:=$(V)
endif
ifeq ($(OPENWRT_VERBOSE),1)             # V=1 → w（保留警告）
  OPENWRT_VERBOSE:=w
endif
ifeq ($(OPENWRT_VERBOSE),99)            # V=99 → s
  OPENWRT_VERBOSE:=s
endif

NO_TRACE_MAKE := $(MAKE) V=s$(OPENWRT_VERBOSE)   # 内部静默子进程用
```

**quiet 分支**（OPENWRT_VERBOSE 不含 `s`，即默认 / V=1）：

```makefile
SILENT:=>/dev/null $(if $(findstring w,$(OPENWRT_VERBOSE)),,2>&1)   # 默认连 stderr 都吞
export QUIET:=1
SUBMAKE=cmd() { $(SILENT) $(MAKE) -s "$$@" < /dev/null || {...}; }; cmd
.SILENT: $(MAKECMDGOALS)
```

- 默认 `make`：stdout+stderr 全部重定向到 `/dev/null`，且目标标记 `.SILENT`。
- `V=1`：只保留 stderr（编译警告仍可见）。

**verbose 分支**（含 `s`，即 `V=s` 或 `V=99`）：

```makefile
SUBMAKE=$(MAKE) -w            # 递归 make 带 -w：打印 Entering/Leaving directory
define MESSAGE
  printf "%s\n" "$(1)"        # 消息直接打印，不再走 fd8/fd9 重定向
endef
```

**`s`/`c` 两个字母的含义**：
| 取值 | 效果 |
|---|---|
| （空） | 全静默：`>/dev/null 2>&1`，QUIET=1 |
| `V=1` → `w` | 静默但保留警告（stderr） |
| `V=99` / `V=s` | 全部命令回显 + `make -w` 目录切换日志 |
| `V=sc` | 额外输出编译细节：scan.mk 里改用 `$(MAKE)` 而非 `NO_TRACE_MAKE` 回显 DUMP；ninja 加 `-v`（rules.mk NINJA 定义）；`NO_TRACE_MAKE = make V=ssc` |

**为什么 V=s 能"看到一切"**：
1. `SUBMAKE=$(MAKE) -w` → 每个递归 make 不静默，回显各自配方；
2. `scripts/config/Makefile` 的 `Q:=$(if $V,,@)` → conf/mconf 编译命令回显；
3. `MESSAGE` 直接 printf，进度消息（"make[1] -C tools/zstd compile"）不再被 fd 重定向吞掉；
4. 不导出 `QUIET=1` → 各包/工具配方里的 `$(SILENT)` 重定向不生效，编译器输出直达终端。

> 排查构建失败的标准姿势：`make V=s`（想保留日志用 `make V=sc 2>&1 | tee build.log`，
> 或开启 `CONFIG_BUILD_LOG` 让输出自动落到 `logs/`）。

---

## 7. 三命令对照表

| 命令 | 阶段 A 做什么 | 核心工具 | 产出 | 交互 |
|---|---|---|---|---|
| `make defconfig` | prepare-tmpinfo 全量扫描 | `conf --defconfig` | 完整 `.config` | 无 |
| `make menuconfig` | prepare-tmpinfo 全量扫描 | `mconf`（ncurses） | 完整 `.config` | 有 |
| `make V=s` | prereq（含扫描）→ 配置同步检查 → 递归 | `make -w` + 各子系统 | `bin/targets/...` 固件 | 首次无 `.config` 时会弹 menuconfig |

三者共用：`OPENWRT_BUILD` 两段式入口、`prepare-tmpinfo`、`tmp/.config-*.in` Kconfig 树、kconfig 工具链。

---

## 8. 关键文件速查

| 文件 | 作用 |
|---|---|
| `Makefile` | 两段式入口；阶段 B 的 world/prepare/clean |
| `include/toplevel.mk` | 阶段 A 全部目标：defconfig/menuconfig/oldconfig/%:: 转发规则 |
| `include/verbose.mk` | V 参数 → OPENWRT_VERBOSE → SUBMAKE/SILENT/MESSAGE |
| `include/scan.mk` | 包/目标元数据扫描（DUMP 模式） |
| `scripts/package-metadata.pl` | packageinfo → tmp/.config-package.in |
| `scripts/target-metadata.pl` | targetinfo → tmp/.config-target.in（含默认目标） |
| `scripts/config/{conf.c,mconf.c,Makefile}` | kconfig 命令行/界面工具 |
| `scripts/kconfig.pl` | 内核配置合并、配置差异比较 |
| `include/subdir.mk` | stamp 机制 + 子目录递归转发 |
| `include/prereq-build.mk` / `prereq.mk` | 主机环境检查 |
| `include/target.mk` / `kernel-build.mk` / `image.mk` | 目标平台 / 内核 / 固件镜像 |
| `include/package.mk` / `rootfs.mk` | 软件包构建流水线 / 根文件系统 |
| `tools/Makefile`、`toolchain/Makefile` | 主机工具、交叉工具链构建 |
