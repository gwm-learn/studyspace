# (mac) ssh in ubuntu
```
ssh gwm@192.168.0.104
```

# (mac) samba in ubuntu
```
[ubuntu]
    comment = Ubuntu Server
    path = /home/gwm
    browseable = yes
    read only = no
    valid users = gwm
    create mask = 0644
    directory mask = 0755
```

# code in ubuntu
```
git clone https://git.openwrt.org/openwrt/openwrt.git

git branch  -a
git checkout  openwrt-25.12
```

# docker in ubuntu
```
docker build -t openwrt-compiler:mt7981 . --progress=plain

docker stop openwrt-compiler
docker rm openwrt-compiler
docker run --name openwrt-compiler -h openwrt-compiler -v $PWD:/w -v /etc/localtime:/etc/localtime:ro -w /w -it openwrt-compiler:mt7981 /bin/bash
```

# (ubuntu) build in docker
```
make menuconfig
make V=s
```

# (mac) serial in openwrt
```
picocom /dev/tty.usbmodem00001 -b 115200 -f n
ctrl A + ctrl X --> exit
```

# OpenWrt 架构本质
OpenWrt 本身并不提供现成完整固件，它是一套源码编排、补丁管理、包构建、镜像组装的工程框架：

1. 拉取上游原生内核、U‑Boot、各类开源软件源码
2. 叠加平台补丁、板级 DTS、驱动补丁
3. 根据 `.config` 裁剪、编译内核、用户态软件包
4. 把内核、rootfs、分区布局拼装生成目标设备的固件镜像

## 核心组成

1. Buildsystem（构建系统）：整个框架的核心，Make 为驱动，负责下载源码、打补丁、编译、打包镜像。
2. Feeds：软件包源仓库，分离基础系统和扩展应用，可自定义第三方包。
3. Target：硬件平台抽象层，区分不同 SoC、板子，存放板级 DTS、内核配置、平台补丁。
4. Packages：单个软件包构建描述，每个程序有自己的 Makefile，定义源码地址、补丁、编译安装规则。
5. 根文件系统 + procd/netifd/uci 用户态基础设施：编译产物运行时，才形成我们看到的路由器操作系统。

# OpenWrt Buildsystem（构建系统）
OpenWrt Buildsystem 是整个 OpenWrt 的**工程核心**，一套以 GNU Make 作为驱动的嵌入式固件构建框架。
它不是运行在路由器上的程序，而是运行在编译主机（Ubuntu）上的工程体系，**职责：把分散的上游源码、补丁、配置，输出成目标设备可烧写的固件镜像**。

> 区分：
> - Buildsystem：编译主机侧的工程框架（源码目录整套Makefile逻辑）
> - 固件镜像：输出产物，烧录到板子上运行。

## 核心设计思想
1. **解耦上游源码与本地修改**
不把内核、uboot、应用软件源码直接存进OpenWrt仓库。只记录：源码地址、版本、补丁文件、编译参数。编译时自动下载。
2. **包为最小单元**
每一个内核、工具、应用都是一个package，拥有独立Makefile，可单独编译、打补丁、开关。
3. **Target硬件抽象**
一套源码树支持几十上百种不同路由器板子，通过Target/Subtarget区分SoC平台与具体板卡。
4. **高度可配置**
通过`.config`控制：选板子、选内核模块、选软件包、裁剪功能。

## Buildsystem 目录关键模块（源码根目录）
|目录|作用|
|---|---|
|`Makefile`|顶层总控Makefile，所有`make xxx`入口|
|`include/`|构建系统核心脚本、规则、模板，package/target的编译规则全部在这里定义|
|`scripts/`|辅助脚本：下载源码、打补丁、生成固件、kconfig处理、feeds更新等工具|
|`target/`|硬件层：内核配置、DTS、平台补丁、固件镜像生成脚本，每一个板子定义在这里|
|`package/`|内置软件包，每个子目录是一个软件的构建描述（Makefile+补丁）|
|`feeds.conf.default`|feeds源配置，指向外部软件包仓库|
|`dl/`|编译缓存，存放下载好的上游源码压缩包，避免重复下载|
|`build_dir/`|编译工作目录：源码解压、打补丁、实际编译发生在这里，可理解为编译临时工作区|
|`staging_dir/`|交叉编译工具链、头文件、库文件安装目录，交叉编译的sysroot|
|`bin/`|**最终输出目录**，编译完成后固件、ipk包输出到此|

## 完整构建流水线（执行`make`后发生什么）
1. **Feeds处理**：读取feeds配置，拉取第三方packages，合并到构建系统包列表
2. **Kconfig解析**：读取`.config`，解析选择的Target、软件包、内核配置选项
3. **工具链构建**：编译生成对应目标架构交叉编译器（gcc、binutils、libc）
4. **下载源码**：根据各个package定义，把上游源码包下载到`dl`目录
5. **打补丁**：将平台补丁、板级补丁，应用到解压后的源码（build_dir）
6. **编译各个包**
    - 先编译内核、uboot
    - 再编译用户态所有软件包，生成ipk
7. **组装根文件系统(rootfs)**：选取选中ipk，拼装成最小根文件系统
8. **固件镜像生成**：target下的镜像脚本，把 uboot、kernel、rootfs 按照Flash分区布局拼接，生成 `.bin`/`.img` 路由器固件。

> 重点：**build_dir是临时工作区，修改build_dir下源码不会永久生效**，重新`make clean`会全部删除。想要永久修改，必须把改动做成patch，放到package/target对应的patches目录。这是工程师最容易踩坑点。

## 关键常用命令对应Buildsystem行为
```bash
make menuconfig      # 调用Kconfig，生成 .config
make defconfig       # 根据target默认配置生成.config
make download        # 预下载全部源码，适合离线编译
make -jN             # 完整执行整条构建流水线
make package/xxx/compile  # 只编译单个软件包
make target/linux/compile # 只编译内核
make clean           # 清空build_dir，保留dl、.config
make dirclean        # 清空build_dir+staging_dir，保留dl
make distclean       # 完全清理，回到刚clone源码状态，删除.config dl bin
```

# OpenWrt Feeds 机制

## 概述

Feeds 是 OpenWrt Buildsystem 的**外部软件包源管理机制**。

OpenWrt 主源码仓库 `package/` 目录只维护核心基础包，大量扩展软件、第三方应用、硬件驱动不放在主线仓库，全部交由 feeds 仓库维护。

> **feeds 本质**：一组外部 git 仓库地址列表，把外部软件包仓库“挂载”进本地构建系统，参与 `menuconfig` 选择、编译、打包生成 `.ipk` 文件。

> **关键区分**：
> - `package/`：主线内置包，跟随 OpenWrt 主仓库一同维护。
> - feeds：外部软件包集合，可以是官方维护、社区、企业私有仓库。

---

## feeds 配置文件

源码根目录：`feeds.conf.default`，复制修改为 `feeds.conf` 生效。

**配置语法**：
src-type feed-name repo-url [branch/tag]


**示例官方默认配置片段**：
```conf
src-git base https://git.openwrt.org/openwrt/openwrt.git;openwrt-23.05
src-git packages https://git.openwrt.org/feed/packages.git;openwrt-23.05
src-git luci https://git.openwrt.org/project/luci.git;openwrt-23.05
src-git routing https://git.openwrt.org/feed/routing.git;openwrt-23.05
src-git telephony https://git.openwrt.org/feed/telephony.git;openwrt-23.05
src-git：使用 git 获取仓库
```

src-svn：svn 源

src-file：本地目录，企业内部开发常用，直接指向本地文件夹，不需要网络拉取

官方内置 feeds 用途
feed 名称	用途
base	系统基础组件，少量核心包
packages	海量通用第三方软件工具
luci	LuCI 网页界面全部源码
routing	路由协议：BGP、OSPF、VRRP 等网络组件
telephony	语音电话、SIP 相关软件
feeds 核心命令
脚本路径：./scripts/feeds

bash
### 更新 feeds：拉取/同步所有 feed 仓库代码到本地 feeds 目录
./scripts/feeds update -a

### 安装 feed 包：把 feeds 里的包注册到构建系统，出现在 menuconfig 菜单
./scripts/feeds install -a

### 只更新指定 feed
./scripts/feeds update luci

### 只安装单个软件包，不执行全部 install
./scripts/feeds install xxx

### 列出所有 feeds 以及包状态
./scripts/feeds list -a

## 目录产物
执行 update 之后，源码根目录生成 feeds/ 文件夹，存放下载下来的各个 feed 仓库源码。

⚠️ 注意：不要直接修改 feeds/ 目录内文件，feeds update 会覆盖本地改动。
如果需要自定义修改第三方 feed 包，有两种方案：

将包拷贝到本地 package/ 目录，优先级高于 feeds。

使用本地 src-file feed，放入私有补丁与修改。

## feeds 工作完整流程
读取 feeds.conf / feeds.conf.default 获取源地址。

feeds update：克隆/拉取各个外部仓库到本地 feeds/*。

feeds install：为每个包生成符号链接，注册进构建系统包数据库。

执行 make menuconfig，feeds 中的软件包就可以在配置菜单里看到，进行选中/取消。

执行 make 编译，和主线 package 包完全同等流程：打补丁、编译、生成 .ipk。

## 包查找优先级（非常重要）
OpenWrt Buildsystem 包搜索优先级，高优先级覆盖低优先级：

package/ 本地目录（最高优先级，企业定制首选）

feeds/* 各个 feed 仓库

实战技巧：如果想修改某个 feed 里面的软件包，不要改 feeds 目录；复制包到根目录 package/xxx，构建系统优先读取此处，不受后续 feeds update 覆盖。

企业开发场景：私有 feeds
商用项目常用私有 feed，存放公司自研业务包，不提交上游主线。

示例 feeds.conf 添加私有本地 feed：

src-file mycustom ./my-custom-feed
目录 ./my-custom-feed 内部组织和官方 feed 结构一致，每个软件包一个子目录。
执行 update/install 后，私有业务包直接进入 menuconfig。

常见踩坑点
只 update 不 install：拉取了 feed 源码，但是包不会出现在 menuconfig，很多新手踩坑。

修改 feeds/ 目录源码，执行 feeds update 全部被覆盖，修改丢失。

# OpenWrt config 配置原理
## 概述
OpenWrt 的 config 整套配置体系，基于 Linux Kconfig 改造而来，是 Buildsystem 的配置中枢。
用户通过 `make menuconfig` / `make defconfig` 等命令生成/修改 `.config` 文件，构建系统读取 `.config`，决定：目标硬件平台、内核配置、启用哪些软件包、编译模块还是内置进固件。

> 关键点：
> - `.config`：**编译主机上的配置文件**，存在 OpenWrt 源码根目录，只作用编译过程；**不会烧录进路由器设备**。
> - 路由器设备上的配置是 UCI（/etc/config/*），和此处编译期 config 完全不是一回事，不要混淆。

## 三层配置模型：Target / Subtarget / Package
OpenWrt 把配置划分为三层，自上而下约束。

1. **Target（大平台）**
代表一类 SoC 系列平台，例如 `mediatek`、`ramips`、`qualcommax`。
定义平台通用：工具链架构、内核版本、通用补丁、根文件系统参数。
对应目录：`target/linux/<target_name>`

2. **Subtarget（子平台）**
Target 下细分芯片型号，例如 `mediatek` 下的 `mt7981`、`mt7986`。
存放该芯片专属内核配置、DTS、驱动补丁。
对应目录：`target/linux/<target_name>/<subtarget_name>`

3. **Package（软件包层）**
控制每一个软件包的编译选项，三种状态：
- `<*>`：Y，**内置**，编译进固件镜像，开机即存在
- `<M>`：M，**模块**，编译生成 ipk，固件不内置，可后续安装
- `< >`：N，**不编译**，完全不参与构建

> 配置生效优先级：`defconfig 默认配置` ← `menuconfig交互配置` ← `.config` 最终生效。

## 关键配置相关文件
|文件|位置|说明|
|---|---|---|
|`.config`|源码根目录|编译总配置，menuconfig输出产物，make读取此文件进行编译|
|`config/Config.in`|源码根目录|顶层Kconfig入口文件，menuconfig菜单总入口|
|`target/linux/*/config-*`|target目录|平台内核碎片配置，会合并生成内核`.config`|
|`profiles/*.mk`|target/linux/<target>/<subtarget>/profiles|板卡profile，每个路由器板子的默认defconfig|
|`package/*/Config.in`|各个package目录|每个软件包自身的Kconfig选项，出现在menuconfig菜单|

### target/linux 内核碎片配置机制
OpenWrt 不维护一份巨大完整内核 `.config`，而是用碎片化 config。
`config-5.15`、`config-6.6` 这类文件存放零散内核开关，编译构建的时候，脚本会把这些碎片配置合并到上游内核的默认配置。

好处：只记录本平台需要改动的内核选项，其余沿用内核上游默认，方便跟随内核大版本升级。

## 常用命令与 config 的关系
```bash
# 交互式图形配置，修改后保存生成 .config
make menuconfig

# 使用板子profile默认配置生成 .config
make defconfig

# 对比：把当前 .config 和默认defconfig做差异，输出差异片段
make savedefconfig

# 更新内核碎片配置（将当前内核配置导出回target碎片config文件）
make target/linux/update_kernel_config

# 只同步 .config，不编译，解析所有依赖关系
make oldconfig
```

## make defconfig 流程

1. 根据 Target/Subtarget 选中对应板子 profile
2. 加载平台默认内核碎片配置
3. 加载各个 package 默认开关
4. 输出完整 `.config` 文件

> 
> 工程师实操注意：更换板子 / Subtarget，需要重新执行 `make defconfig` 重新生成适配的 `.config`，旧的 .config 不能跨平台直接复用。

# OpenWrt 固件打包流程
## 概述
固件打包是 Buildsystem 构建流水线的**最后阶段**。
在内核、U‑Boot、全部用户态软件包编译完成之后，构建系统把内核镜像、根文件系统、板级分区信息进行组装、压缩、拼接，最终生成路由器可烧写的 `.bin` / `.img` 固件文件，输出到 `bin/targets/<target>/<subtarget>/`。

> 重要概念区分：
> 1. `vmlinux`：内核原始ELF，调试用，不能直接烧写Flash。
> 2. `zImage` / `Image.gz`：压缩后的内核镜像，可被U‑Boot加载。
> 3. `rootfs`：根文件系统，包含所有用户程序、库、配置模板。
> 4. 最终 `.bin`：**板卡完整固件镜像**，U‑Boot + 内核 + rootfs 按Flash分区布局拼接而成。

## 前置条件（打包之前已经完成）
1. 工具链编译完成
2. 所有选中的软件包编译完成，生成 `.ipk` 安装包
3. Linux内核编译完成，产出压缩内核镜像 `Image.gz`
4. U‑Boot（如果本平台需要）编译完成
5. Target下的板级Profile、DTS、分区定义全部就绪

## 完整打包流水线（make执行后期）
### 1. 准备根文件系统（rootfs）
脚本位置：`target/linux/Makefile`、`scripts/rootfs.mk`

1. 创建一套空白的目录骨架（`/etc` `/bin` `/sbin` `/usr` 等标准目录）
2. 将选中的ipk包解压安装到这个根文件系统目录
3. 拷贝平台默认配置模板（UCI默认配置、启动脚本）
4. 执行各种板级预处理：创建符号链接、设备节点、权限修正
5. **剔除没有选中的软件包**，生成裁剪过后最小根文件系统

> 工作目录：`build_dir/target‑*/root‑<board>`

### 2. 生成rootfs镜像
把上面目录树打包成块设备镜像，常见两种格式：
- SquashFS：**OpenWrt默认**，只读压缩文件系统；固件本体，分区挂载为只读，配置改动 overlayfs 存到额外分区。商用路由器主流。
- JFFS2 / UBIFS：Flash专用可读写文件系统，NAND设备使用。

> 重点：SquashFS 是只读，路由器修改配置靠 overlay 分区叠加，不是改写固件内rootfs。

输出产物示例：`rootfs.squashfs`

### 3. 处理内核镜像
1. 使用内核编译产出 `Image.gz`
2. **追加设备树DTS二进制blob**，生成 `fitImage` / `zImage‑dtb`。
> ARM路由平台，内核必须附带DTS，U‑Boot启动时解析设备树。
3. 部分平台会做头部封装（添加U‑Boot可识别镜像头、校验头）

输出产物示例：`kernel.bin`、`fit‑uImage.itb`

### 4. 调用板级镜像生成脚本（target/image）
> 每个平台的打包规则在：`target/linux/<target>/image/Makefile`
这里定义：
- Flash分区布局（kernel分区大小、rootfs分区大小）
- 是否拼接U‑Boot
- 固件头部、校验、签名规则
- 生成哪些输出固件文件（factory.bin / sysupgrade.bin）

两个最常见固件产物：
1. **factory.bin**：整机完整固件，包含U‑Boot、环境变量、内核、rootfs。用于TTL烧写、编程器烧写、原厂出厂刷机。
2. **sysupgrade.bin**：系统升级包，**不包含U‑Boot**，只包含内核+rootfs。路由器网页后台升级使用，不覆盖bootloader。

> ⚠️ 错误混用会砖机：sysupgrade 不能拿来给空白Flash编程器烧写；factory一般不能直接网页升级。

### 5. 拼接、填充、校验、填充对齐
1. 根据DTS / image Makefile定义的分区大小，对kernel、rootfs镜像做对齐填充（Flash块大小对齐）
2. 拼接各段镜像数据
3. 添加平台校验和、签名（如果开启固件签名）
4. 裁剪、生成最终bin固件

### 6. 输出到bin目录
最终固件、ipk包、内核调试文件统一输出：
`bin/targets/<target平台>/<subtarget芯片>/`

## 关键目录与产物一览
|路径|说明|
|---|---|
|`build_dir/target‑*/root‑xxx`|组装中的根文件系统目录|
|`build_dir/target‑*/linux‑xxx/`|内核编译产物，vmlinux、Image.gz、dtb|
|`target/linux/*/image/Makefile`|平台固件打包规则，定义factory/sysupgrade生成逻辑|
|`bin/targets/.../*.bin`|最终可烧写固件|

## 两个重要机制
### 1. OverlayFS 工作原理（SquashFS固件）
固件内rootfs是SquashFS只读；板子上有独立的overlay分区。
- 读文件：优先读overlay分区，没有就回退读SquashFS固件。
- 修改文件：不会改写固件，改动写入overlay分区。
- 恢复出厂：清空overlay分区即可，固件本体不变。

### 2. A/B双分区升级（商用路由器）
在image Makefile和DTS中定义两套kernel/rootfs分区。
sysupgrade固件写入备用分区，校验成功后切换启动分区；升级失败依旧可以从旧分区启动。OpenWrt主线支持，大量商用路由使用。

## 常用调试命令
```bash
# 只执行打包镜像，不重新编译全部源码
make target/linux/image/compile

# 清理镜像产物，不清理内核与包
make target/linux/image/clean

# 解压查看squashfs根文件系统内容
unsquashfs bin/targets/xxx/rootfs.squashfs
```