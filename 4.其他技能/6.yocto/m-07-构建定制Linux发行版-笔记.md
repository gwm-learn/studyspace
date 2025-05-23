# 核心镜像--Linux发行版蓝图
1. core-image-minimal：这是允许设备启动到Linux命令行登录的最基础的镜像。登录和命令行解释器是由BusyBox提供的。
2. core-image-minimal-initramfs：这个镜像本质上和core-imageminimal一样，但是具有包括基于RAM的初始根文件系统（initramfs）的Linux内核。
3. core-image-minimal-mtdutils：基于core-image-minimal，这个镜像也包含用户空间工具来与在Linux内核中的内存技术设备（MemoryTechnology Device，MTD）子系统交互以在闪存设备上执行操作。
4. core-image-minimal-dev：基于core-image-minimal，这个镜像也包含用于安装在根文件系统中的所有包的全部开发包（例如头文件）。
5. core-image-rt：基于core-image-minimal，这个镜像目标构建Yocto项目实时内核（real-time kernel）并包含用于实时应用的测试套件与工具。
6. core-image-rt-sdk：除了core-image-rt之外，这个镜像包含由所有已安装包的开发包所组成的系统开发工具包。
7. core-image-base：本质上是core-image-minimal，这个镜像也包含中间件和应用包以支持大量如WiFi、蓝牙、声卡和串行口的硬件。
8. core-image-full-cmdline：这个最小化的镜像添加典型的Linux命令行工具（bash、acl、attr、grep、sed和tar等）到根文件系统。
9. core-image-lsb：这个镜像包含与Linux标准基（Linux StandardBase，LSB）规格说明保持一致性所需的包。
10. core-image-lsb-dev：这个镜像和core-image-lsb相同，但是也包含安装在根文件系统上的所有包的开发包。
11. core-image-lsb-sdk：除了core-image-lsb-dev之外，这个镜像包含例如编译器、汇编程序和链接器，以及性能测试工具和Linux内核开发包的开发工具。
12. core-image-x11：这个基础的图形镜像包含X11服务器和X11终端应用。   
...

## 通过本地配置来扩展核心镜像
最简单的用于增加包和包组到镜像的方法是增加IMAGE_INSTALL到构建环境的conf/local.conf文件中

```
IMAGE_INSTALL_append = " strace sudo sqlite3"
```

使用在构建环境的conf/local.conf中的IMAGE_INSTALL会无条件地影响你将要用这个构建环境所构建的所有镜像。如果你希望仅仅安装额外的包到特定镜像，可以使用条件追加
```
IMAGE_INSTALL_append_pn-<image> = " strace"
```
仅仅向core-image-minimal的根文件系统安装strace工具。所有其他镜像是不受影响的。   

为了便利性的目的，coreimage类定义变量CORE_IMAGE_EXTRA_INSTALL。所有增加到这个变量的包和包组都被类追加到IMAGE_INSTALL。
```
CORE_IMAGE_EXTRA_INSTALL = "strace sudo sqlite3"
```
增加这些包到所有继承自core-image的镜像。直接继承自image的镜像不受影响。   

## 用菜谱扩展核心镜像

## 镜像特性
镜像特性提供特定的、你可以添加到目标镜像的功能性。其可以是额外要被安装的包、配置文件的修改等。

## 包组
### 包组菜谱
包组菜谱的名字，虽然不是由构建系统所强制或者要求的，但也应该遵守惯例packagegroup-<name>.bb。你也希望把它们放在包组集成的菜谱类别的packagegroup子目录中。如果包组横跨菜谱并且可能来自多个类别的包组，那么把它们放进recipes-core类别是好的实践。

# 从头构建镜像

# 镜像选择

## 镜像大小

## 根文件系统类型

## 用户、组和密码

## 调整根文件系统



