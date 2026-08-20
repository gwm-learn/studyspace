# OpenWrt Feeds 机制详解

> 本文基于本机源码树 `/Volumes/ubuntu/code/openwrt`（`openwrt-25.12` 分支）逐文件梳理。
> 核心代码：`scripts/feeds`（979 行 Perl）、`feeds.conf.default`、`include/feeds.mk`、
> `include/scan.mk`、`include/toplevel.mk`、`package/base-files/image-config.in`。

---

## 0. 一句话总结

**Feeds 是"构建前（Pre-build）的第三方源码管理阶段"**，由用户手动执行
`./scripts/feeds update -a` + `./scripts/feeds install -a` 完成，**不依赖 `.config`、不由
`make` 自动触发**。构建系统（`make defconfig` / `menuconfig` / `V=s`）只是**消费** feeds 的
两个产物：

1. `feeds/<name>.index` —— 每个 feed 的**包元数据索引**（供 install 时查找包）；
2. `package/feeds/<feed>/<pkg>` —— 指向 feed 源码目录的**符号链接**（供扫描/编译）。

---

## 1. 配置文件与数据流全景

```
feeds.conf(.default)                # 定义 feed 来源
   │  ./scripts/feeds update -a
   ▼
feeds/<name>/                       # VCS 克隆下来的源码（git/svn/...）
   │  对每个 feed 跑 include/scan.mk（DUMP 模式）
   ▼
feeds/<name>.index / .targetindex   # ← 符号链接 → <name>.tmp/.packageinfo（包元数据）
   │  ./scripts/feeds install -a
   ▼
package/feeds/<feed>/<pkg>          # ← 符号链接 → ../../../feeds/<name>/<path>
target/linux/feeds/<target>         # ← feed 提供的目标平台（若有）
   │
   │  make defconfig / menuconfig / V=s   （构建系统消费阶段）
   ▼
prepare-tmpinfo
   ├─ scan.mk 扫描 package/（含 package/feeds/ 符号链接）→ tmp/.packageinfo
   ├─ scripts/feeds feed_config          → tmp/.config-feeds.in（CONFIG_FEED_<name>）
   ├─ package-metadata.pl                → tmp/.config-package.in / .packagedeps
   └─ feeds/base 符号链接 → ../package    （核心包伪装成一个 feed）
```

---

## 2. 配置文件格式（`feeds.conf.default`）

```conf
src-git packages https://git.openwrt.org/feed/packages.git;openwrt-25.12
src-git luci https://git.openwrt.org/project/luci.git;openwrt-25.12
src-git routing https://git.openwrt.org/feed/routing.git;openwrt-25.12
src-git telephony https://git.openwrt.org/feed/telephony.git;openwrt-25.12
src-git video https://github.com/openwrt/video.git;openwrt-25.12
```

行格式：`src-<type> [--flag[=value]] <name> <url>[;branch][^commit]`

| 类型 | 说明 |
|---|---|
| `src-git` | 浅克隆（`--depth 1`），URL 用 `;` 追加分支、`^` 追加 commit |
| `src-git-full` | 完整 git 克隆 |
| `src-svn` / `src-bzr` / `src-hg` / `src-darcs` / `src-gitsvn` | 其他 VCS |
| `src-cpy` / `src-link` | 本地目录拷贝 / 链接（本地开发用） |
| `src-dummy` | 只建空目录（占位） |
| `src-include` | 包含另一个配置文件 |

解析逻辑（`scripts/feeds` 的 `parse_file`，第 46-87 行）：

```perl
sub parse_config() {
    my %name;
    parse_file("feeds.conf", \%name) or      # 用户配置优先
        parse_file("feeds.conf.default", \%name) or
        die "Unable to open feeds configuration";
}
```

> `feeds.conf` 存在时**完全替代** `feeds.conf.default`（不是追加）。URL 变化时
> `update_location`（第 96-121 行）记录新 URL 并触发重新克隆。

---

## 3. 阶段一：`./scripts/feeds update -a`

### 3.1 总流程（`update` 子程序，第 853-898 行）

```
update -a
├─ mkdir feeds/
├─ foreach feed in feeds.conf:
│    ├─ update_feed(type, name, src, force, rebase, stash)
│    │    └─ update_feed_via() → 按 %update_method 表执行 VCS 命令
│    └─ push name → @index_feeds
├─ foreach feed in @index_feeds:
│    └─ update_index(name) → 扫描生成 feeds/<name>.index
└─ refresh_config() → 若 .config 存在则 make defconfig 刷新
```

### 3.2 源码获取（`update_feed_via`，第 207-263 行）

核心是 `%update_method` 哈希表（第 145-202 行），每种 VCS 定义 `init/update/controldir/revision`：

```perl
'src-git' => {
    'init'          => "git clone --depth 1 '%s' '%s'",
    'init_branch'   => "git clone --depth 1 --branch '%s' '%s' '%s'",
    'update'        => "git pull --ff-only",
    'post_update'   => "git submodule update --init --recursive --depth 1",
    'controldir'    => ".git",
    'revision'      => "git rev-parse HEAD | tr -d '\n'"},
```

决策逻辑：

```perl
if ( $relocate || !$m->{'update'} || !-d "$localpath/$m->{'controldir'}" ) {
    # 首次克隆：URL 变了 / 该类型不支持 update / 目录里没有版本控制元数据
    system(sprintf($m->{'init_branch'}, $branch, $base_branch, $localpath));  # 有 ;branch
    # 或 init_commit（^commit 固定版本）
    # 或 init（默认）
} elsif ($m->{'init_commit'} and $commit) {
    # 指定了 commit 则永不更新
} else {
    # 增量更新：git pull --ff-only（-f 强制 / -r rebase / -s autostash）
    system("cd '$localpath'; $update_cmd");
}
```

产物：`feeds/<name>/`（如 `feeds/packages/`、`feeds/luci/`）。

### 3.3 索引生成（`update_index`，第 123-143 行）

```perl
sub update_index($) {
    my $name = shift;
    ...
    system("$mk -s prepare-mk OPENWRT_BUILD= TMP_DIR=\"$ENV{TOPDIR}/feeds/$name.tmp\"");
    # 用 scan.mk 以 DUMP 模式扫描该 feed，产物落到 feeds/<name>.tmp/
    system("$mk -s -f include/scan.mk ... SCAN_TARGET=\"packageinfo\" SCAN_DIR=\"feeds/$name\" ...");
    system("$mk -s -f include/scan.mk ... SCAN_TARGET=\"targetinfo\" SCAN_DIR=\"feeds/$name\" ...");
    # 索引文件（注意是符号链接，指向 tmp 下的元数据）
    system("ln -sf $name.tmp/.packageinfo ./feeds/$name.index");
    system("ln -sf $name.tmp/.targetinfo ./feeds/$name.targetindex");
}
```

要点：
- 复用了与 `prepare-tmpinfo` **完全相同的** `include/scan.mk` 扫描机制，只是 `SCAN_DIR`
  换成 `feeds/<name>`、`TMP_DIR` 换成 `feeds/<name>.tmp`；
- 产物 `feeds/<name>.index` 是一个**符号链接**，内容就是该 feed 全部包的元数据
  （`parse_package_metadata` 能解析的格式）。

### 3.4 配置刷新（`refresh_config`，第 684-701 行）

```perl
sub refresh_config {
    ...
    return if not (-e '.config');          # 没有 .config 就不建（用户没配置过）
    system("rm -f tmp/.packageinfo");      # 强制下次重新扫描
    system("$mk defconfig Config.in >/dev/null 2>/dev/null");   # 或 oldconfig（-d 参数）
}
```

`update` 后如果已有 `.config`，会重跑 defconfig，让 `CONFIG_FEED_<name>`（§5.2）同步更新。

---

## 4. 阶段二：`./scripts/feeds install -a`

### 4.1 总流程（`install` 子程序，第 703-751 行）

```
install -a
├─ get_installed()      # make -s prepare-tmpinfo OPENWRT_BUILD= → 核心包元数据
├─ foreach feed: get_feed($f)   # 读 feeds/<name>.index 载入包元数据
├─ foreach feed（-a 或指定 -p <feed>）:
│    foreach src 包名:
│        install_src(feed, name, force)
│           ├─ lookup_src()      # 在哪个 feed 里？
│           ├─ is_core_src()     # 与核心包冲突检查（默认不覆盖，-f 强制）
│           ├─ do_install_src()  # 建符号链接
│           └─ 递归安装 builddepends / depends 依赖
└─ refresh_config(-d y|m|n)     # 可选：设置新包的默认选择状态
```

### 4.2 包定位（`lookup_*` / `get_feed`）

- `get_feed`（第 275-296 行）：缓存机制，从 `feeds/<name>.index` + `.targetindex`
  解析出包、源包、目标、虚拟包四张哈希表；
- `lookup_src` / `lookup_package` / `lookup_target`：按"指定 feed → 依次遍历全部 feed"
  的顺序查找；虚拟包（`provides`）通过 `%vpackage` 映射回真实源包。

### 4.3 符号链接创建（`do_install_src`，第 438-455 行）

```perl
sub do_install_src($$) {
    my $feed = shift;
    my $src = shift;
    my $path = $src->{makefile};
    $path =~ s/\/Makefile$//;
    -d "./package/feeds" or mkdir "./package/feeds";
    -d "./package/feeds/$feed->[1]" or mkdir "./package/feeds/$feed->[1]";
    system("ln -sf ../../../$path ./package/feeds/$feed->[1]/");
}
```

即：`package/feeds/<feed>/<pkg> -> ../../../feeds/<feed>/<path>`。

目标平台的包走 `do_install_target`（第 457-486 行）：链接到 `target/linux/feeds/<name>`，
并删除 `tmp/info/.packageinfo-kernel_linux` 强制下次重扫（否则内核模块扫描不到新目标）。

### 4.4 冲突与依赖

- **核心包保护**：`is_core_src`（第 524-531 行）检查 `tmp/info/.packageinfo-*`，
  发现同名核心包时打印
  `WARNING: Not overriding core package '<name>'; use -f to force`，默认跳过；
- **依赖递归**：`install_src` 尾部递归安装源包的 `builddepends`/`builddepends/host`
  以及所有二进制包的 `depends`（过滤 `@` 虚拟依赖和 `+` 前缀），保证闭包完整。

### 4.5 对应 make 目标（`include/toplevel.mk` 第 247-258 行）

```makefile
package/symlinks:          # 全量更新 + 安装
	./scripts/feeds update -a
	./scripts/feeds install -a
package/symlinks-install:  # 只重建索引 + 安装
	./scripts/feeds update -i
	./scripts/feeds install -a
package/symlinks-clean:    # 全部卸载
	./scripts/feeds uninstall -a
```

---

## 5. 阶段三：构建系统如何消费 feeds

> 这回答了"feeds 机制在哪个阶段工作"——**feeds 的产出在配置阶段和构建阶段被消费**。

### 5.1 `prepare-tmpinfo` 中的 feeds 处理（`toplevel.mk` 第 78-92 行）

```makefile
prepare-tmpinfo: FORCE
	@+$(MAKE) -r -s $(STAGING_DIR_HOST)/.prereq-build $(PREP_MK)
	mkdir -p tmp/info feeds
	[ -e $(TOPDIR)/feeds/base ] || ln -sf ../package $(TOPDIR)/feeds/base   # ← ①
	$(_SINGLE)$(NO_TRACE_MAKE) -j1 -r -s -f include/scan.mk \               # ← ② 扫 package/
		SCAN_TARGET="packageinfo" SCAN_DIR="package" SCAN_NAME="package" SCAN_DEPTH=5 ...
	$(_SINGLE)$(NO_TRACE_MAKE) -j1 -r -s -f include/scan.mk \               # ← ③ 扫 target/linux
		SCAN_TARGET="targetinfo" SCAN_DIR="target/linux" SCAN_NAME="target" SCAN_DEPTH=3 ...
	for type in package target; do \
		... ./scripts/$${type}-metadata.pl config "$$f" > "$$t" ...; \
	done
	[ tmp/.config-feeds.in -nt tmp/.packageauxvars ] || \                    # ← ④
		./scripts/feeds feed_config > tmp/.config-feeds.in
	./scripts/package-metadata.pl mk tmp/.packageinfo > tmp/.packagedeps ...
	...
```

- **①** `feeds/base` 是核心包的符号链接（`-> ../package`），让"核心包"在 feeds
  语义里也表现为一个名为 `base` 的 feed（`scripts/feeds` 的 `FEED` 变量、per-feed 仓库等
  逻辑可以统一处理）；
- **②** 扫描 `package/` 深度 5 → 覆盖 `package/feeds/<feed>/<pkg>`（符号链接跟随，
  scan.mk 用 `find -L`）；
- **③** 扫描 `target/linux/` → 覆盖 `target/linux/feeds/`（feed 目标平台）；
- **④** `scripts/feeds feed_config` 生成 `tmp/.config-feeds.in`。

### 5.2 `feed_config`：为每个 feed 生成 Kconfig 选项（`scripts/feeds` 第 900-915 行）

```perl
sub feed_config() {
    foreach my $feed (@feeds) {
        my $installed = (-f "feeds/$feed->[1].index");        # 是否已 update 出索引
        printf "\tconfig FEED_%s\n", $feed->[1];
        printf "\t\ttristate \"Enable feed %s\"\n", $feed->[1];
        printf "\t\tdepends on PER_FEED_REPO\n";
        printf "\t\tdefault y\n" if $installed;
        ...
    }
}
```

这个片段被 `package/base-files/image-config.in` 引入配置树：

```kconfig
menuconfig PER_FEED_REPO
	bool "Separate feed repositories" if IMAGEOPT
	default y
	help
		If set, a separate repository is generated within bin/*/packages/
		for the core packages and each enabled feed.

source "tmp/.config-feeds.in"
```

而 `image-config.in` 又由 `scripts/package-metadata.pl`（第 354-356 行）塞进
`tmp/.config-package.in`：

```perl
print "source \"package/*/image-config.in\"\n";
if (scalar glob "package/feeds/*/*/image-config.in") {
    print "source \"package/feeds/*/*/image-config.in\"\n";
}
```

**效果**：menuconfig 的 "Global build settings → Separate feed repositories" 下会出现
`Enable feed packages / luci / routing / ...` 选项，`default y if` 该 feed 已 update。
`y` = 独立仓库包含该 feed、`m` = 在仓库配置里注释掉、`n` = 不生成该 feed 仓库。

### 5.3 扫描期的 feed 归类（`include/scan.mk`）

```makefile
define feedname
$(if $(patsubst feeds/%,,$(1)),,$(word 2,$(subst /, ,$(1))))   # 路径以 feeds/ 开头时取第二段
endef
...
# 扫描 DUMP 时把 FEED 环境变量传给包 Makefile（scan.mk 第 53 行）
$(MAKE) -r DUMP=1 FEED="$(call feedname,$(2))" -C $(SCAN_DIR)/$(2) ...
```

- 包 Makefile 里可通过 `FEED` 变量感知自己属于哪个 feed；
- 对 feeds 目录，`SCAN_DEPS` 额外跟踪 `feeds/<name>/*.mk`（feed 公共 Makefile 变化触发重扫）。

### 5.4 包依赖图（`tmp/.packagedeps`）

`package-metadata.pl mk` 把核心包 + `package/feeds/` 下的包统一产成 `package-y/m/n`。
`package/Makefile` 第 13-17 行：

```makefile
-include $(TMP_DIR)/.packagedeps
package-y += kernel/linux
$(curdir)/builddirs:=$(sort $(package-) $(package-y) $(package-m))   # ← feeds 包在此入列
```

**这决定了 `make V=s` 时实际编译哪些包**——feeds 包与核心包在编译期完全平等。

### 5.5 构建/打包期（`include/feeds.mk`，`package/Makefile` 第 10 行引入）

```makefile
FEEDS_INSTALLED:=$(notdir $(wildcard $(TOPDIR)/package/feeds/*))
FEEDS_AVAILABLE:=$(sort $(FEEDS_INSTALLED) $(shell $(SCRIPT_DIR)/feeds list -n 2>/dev/null))

PACKAGE_SUBDIRS=$(PACKAGE_DIR)
ifneq ($(CONFIG_PER_FEED_REPO),)
  PACKAGE_SUBDIRS += $(OUTPUT_DIR)/packages/$(ARCH_PACKAGES)/base
  PACKAGE_SUBDIRS += $(foreach FEED,$(FEEDS_AVAILABLE),$(OUTPUT_DIR)/packages/$(ARCH_PACKAGES)/$(FEED))
endif
```

- `CONFIG_PER_FEED_REPO=y` 时，ipk/apk 按 feed 分目录输出到 `bin/packages/<arch>/<feed>/`；
- `FeedSourcesAppendOPKG` / `FeedSourcesAppendAPK`（第 35-60 行）为固件生成
  opkg `distfeeds.conf` / apk 仓库列表：核心包仓库 + `CONFIG_FEED_<name>` 标记的
  feed 仓库（`m` 则整行注释掉）；
- `FeedPackageDir`（第 26-32 行）：按 `Package/<name>/subdir` 决定单个包的输出目录。

### 5.6 feed 目标平台的优先级（`include/target.mk` 第 68 行）

```makefile
PLATFORM_DIR:=$(firstword $(wildcard $(TOPDIR)/target/linux/feeds/$(BOARD) $(TOPDIR)/target/linux/$(BOARD)))
```

feed 提供的目标平台（`target/linux/feeds/`）**优先于**核心目标目录被选中。

---

## 6. 卸载与清理

```shell
./scripts/feeds uninstall <pkg>   # 删 package/feeds/*/<pkg> 符号链接
./scripts/feeds uninstall -a      # 全删 package/feeds/ + target/linux/feeds/ 符号链接
./scripts/feeds clean             # 连源码一起删：./feeds ./package/feeds ./target/linux/feeds
make package/symlinks-clean       # = uninstall -a
make distclean                    # 顶层清理同样包含 feeds 与 package/feeds
```

`uninstall` 后同样会 `refresh_config()`（有 `.config` 时重跑 defconfig 去掉相应选项）。

---

## 7. 常用命令速查

| 命令 | 作用 |
|---|---|
| `./scripts/feeds update -a` | 克隆/更新全部 feed 源码 + 生成 `.index` |
| `./scripts/feeds update -i` | 只重建索引（不拉取） |
| `./scripts/feeds update luci` | 只更新指定 feed |
| `./scripts/feeds install -a` | 把全部 feed 包链接进 `package/feeds/` |
| `./scripts/feeds install -p luci <pkg>` | 从指定 feed 安装某包 |
| `./scripts/feeds search <关键词>` | 在所有 feed 里搜包 |
| `./scripts/feeds list` / `-s` | 列出 feed 内容 / 带 URL 与版本 |
| `make package/symlinks` | 等价 update -a + install -a |

---

## 8. 关键文件速查

| 文件 | 作用 |
|---|---|
| `feeds.conf.default` / `feeds.conf` | feed 来源定义（后者优先） |
| `scripts/feeds` | 全部 feeds 操作（Perl，979 行） |
| `include/feeds.mk` | 构建期 feed 常量、per-feed 仓库、opkg/apk 源生成 |
| `include/scan.mk` | 包/目标元数据扫描（update 与 prepare-tmpinfo 共用） |
| `package/base-files/image-config.in` | `PER_FEED_REPO` 菜单 + `source tmp/.config-feeds.in` |
| `scripts/package-metadata.pl` | 把 `image-config.in` 引入配置树、生成 `.packagedeps` |
| `include/toplevel.mk` | `prepare-tmpinfo` 中触发 `feeds feed_config`；`package/symlinks*` 目标 |
| `include/target.mk` | `target/linux/feeds/` 优先匹配规则 |

---

## 9. 与本仓库实际的对照

本仓库当前状态：`feeds/` 下只有 `base`（prepare-tmpinfo 自动创建的指向 `../package`
的符号链接），`package/feeds/`、`feeds/*.index` 均为空——**尚未执行过
`./scripts/feeds update/install`**。因此：

- `tmp/.config-feeds.in` 仍会生成（5 个 feed 的 `FEED_*` 选项），但因无 `.index`，
  全部 `default` 为空（不会默认启用）；
- menuconfig 里看不到任何 feed 包；
- `make V=s` 只编译核心 `package/` 下的包。

若需要 LuCI 等，标准操作是：

```shell
./scripts/feeds update -a
./scripts/feeds install -a
make defconfig        # 或 menuconfig 后保存，刷新 FEED_* 与包选项
make -j$(nproc) V=s
```
