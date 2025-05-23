# 1. 嵌入式系统选择Linux
## 1.1. 初期
Linux不是嵌入式系统首选，原因如下：
1. 文件系统
2. 内存管理单元
3. 实时

## 1.2. 快速增长
1. 特许权使用费
2. 硬件支持
3. 网络
4. 模块化
5. 可伸缩性
6. 源代码
7. 开发者支持
8. 商业支持
9. 工具化

# 2. 嵌入式Linux形势
## 2.1. 嵌入式Linux发行版
### 2.1.1. 非完整的Linux
1. Android
2. Angstrom
3. OpenWrt

### 2.1.2. 完整的Linux
1. Debian
2. Fedora
3. Gentoo
4. SUSE
5. Ubuntu

## 2.2. 嵌入式Linux开发工具
1. Baserock
2. Buildroot -- 完整嵌入式Linux系统所准备的构建系统，使用GNU Make和一套makefile来创建交叉编译的工具链
	uClibc用以构建交叉编译工具链的目标库
	BusyBox是默认的命令行实用程序的集合
3. OpenEmbedded
	其是构建框架，包含工具、配置数据和菜谱以创建针对嵌入式设备的Linux发行版，其核心的是管理构建过程的Bitbake任务执行器
4. Yocto