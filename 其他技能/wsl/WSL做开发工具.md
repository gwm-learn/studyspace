### 问题
目前开发编译代码是放在编译服务器上进行，编译服务器一般就足够开发人员使用了，但会遇到一个比较严重的问题。

一个编译服务器上一般不止一个人在开发或编译，一旦有一个人在编译代码就有可能会造成其他人使用编译服务器时卡顿，严重影响开发进度，由此就想到在自己的开发PC上可以开发编译代码。

一般的解决方法是在windows上安装一个虚拟机软件，再在虚拟机软件上安装各种系统镜像，以满足自身的开发需求，但是一般的虚拟机软件都不是免费软件，使用破解版的话有可能被发律师函，正版软件又太贵了，有钱另说。免费的虚拟机软件又不太靠谱，例如VirtualBox，笔者以前用VirtualBox就经常出现问题。由此在笔者了解到有wsl这个东西之后，就尝试着用wsl作为生产工具使用。    

---------------------------------------------------------------------
### wsl
wsl全称是"适用于 Linux 的 Windows 子系统"，可让开发人员按原样运行 GNU/Linux 环境。
简单来说，wsl是windows自家提供的一个虚拟机方法hyper-v，使用wsl之后，开发人员就不再需要在自己的开发PC上安装各种虚拟机软件，例如vMware、VirtualBox。  

wsl详细介绍可以在微软相关网址上查看。
```
https://docs.microsoft.com/zh-cn/windows/wsl/about
```
--------------------------------------------------------------------
### 安装
wsl安装可选的linux发行版步骤如下

#### 1.启用适用于 Linux 的 Windows 子系统
以管理员身份打开 PowerShell 并运行：
```
dism.exe /online /enable-feature /featurename:Microsoft-Windows-Subsystem-Linux /all /norestart
```

#### 2.检查运行 WSL 2 的要求
wsl2版本要求如下：
1. 对于 x64 系统：版本 1903 或更高版本，采用 内部版本 18362 或更高版本。
2. 对于 ARM64 系统：版本 2004 或更高版本，采用 内部版本 19041 或更高版本。
3. 低于 18362 的版本不支持 WSL 2。 

#### 3.启用虚拟机功能
以管理员身份打开 PowerShell 并运行：
```
dism.exe /online /enable-feature /featurename:VirtualMachinePlatform /all /norestart
```

#### 4.安装 Linux 内核更新包
下载linux内核更新包。
```
x86:
https://wslstorestorage.blob.core.windows.net/wslblob/wsl_update_x64.msi
arm64:
https://wslstorestorage.blob.core.windows.net/wslblob/wsl_update_arm64.msi
```
下载之后点击安装即可。
之后重启计算机更新系统。     

#### 5.将 WSL 2 设置为默认版本
以管理员身份打开 PowerShell 并运行：
```
wsl --set-default-version 2
```

#### 6.安装所选的 Linux 分发
windows官网上提供的安装方法是用Microsoft Store上下载安装，但是这样有个问题，Microsoft Store安装的软件是默认安装在系统C盘的，C盘空间不足的话就非常糟糕了，这里提供了安装在非系统盘上的方法。这里我以ubuntu 20.04版本作为例子。       
相关下载指导链接：   
```
https://docs.microsoft.com/zh-cn/windows/wsl/install-manual
```
相关下载链接如下：
```
Ubuntu 20.04                        :https://aka.ms/wslubuntu2004
Ubuntu 20.04 ARM                    :https://aka.ms/wslubuntu2004arm
Ubuntu 18.04                        :https://aka.ms/wsl-ubuntu-1804
Ubuntu 18.04 ARM                    :https://aka.ms/wsl-ubuntu-1804-arm
Ubuntu 16.04                        :https://aka.ms/wsl-ubuntu-1604
Debian GNU/Linux                    :https://aka.ms/wsl-debian-gnulinux
Kali Linux                          :https://aka.ms/wsl-kali-linux-new
SUSE Linux Enterprise Server 12     :https://aka.ms/wsl-sles-12
SUSE Linux Enterprise Server 15 SP2 :https://aka.ms/wsl-SUSELinuxEnterpriseServer15SP2
openSUSE Leap 15.2                  :https://aka.ms/wsl-opensuseleap15-2
Fedora Remix for WSL                :https://github.com/WhitewaterFoundry/WSLFedoraRemix/releases/
```

1. 从网上下载一个喜欢的发行版，将下载好的发行版后缀 appx 改为 zip，然后解压到非系统盘上面。
2. 进入解压目录，直接点击ubuntu2004.exe进行安装，这会把ubuntu安装在解压目录内，安装过程中会有弹窗，按照提示进行即可。    
3. 安装完成之后解压目录内会多一个文件ext4.vhdx。
4. 接下来便是换源，将以下内容写入/etc/apt/sources.list，写入之前记得备份该文件。
```
deb http://mirrors.aliyun.com/ubuntu/ focal main restricted universe multiverse 
deb-src http://mirrors.aliyun.com/ubuntu/ focal main restricted universe multiverse
deb http://mirrors.aliyun.com/ubuntu/ focal-security main restricted universe multiverse
deb-src http://mirrors.aliyun.com/ubuntu/ focal-security main restricted universe multiverse
deb http://mirrors.aliyun.com/ubuntu/ focal-updates main restricted universe multiverse
deb-src http://mirrors.aliyun.com/ubuntu/ focal-updates main restricted universe multiverse
deb http://mirrors.aliyun.com/ubuntu/ focal-proposed main restricted universe multiverse
deb-src http://mirrors.aliyun.com/ubuntu/ focal-proposed main restricted universe multiverse
deb http://mirrors.aliyun.com/ubuntu/ focal-backports main restricted universe multiverse
deb-src http://mirrors.aliyun.com/ubuntu/ focal-backports main restricted universe multiverse
```
5. 接着执行以下命令更新系统：
```
sudo apt-get -y update && sudo apt-get -y upgrade
```

#### 8.资源限制
wsl需要进行资源限制，不然启动ubuntu之后，编译代码的时候会把系统所有资源占完，导致无法进行其他操作。   
按下Windows + R 键，输入 %UserProfile% 并运行进入用户文件夹,新建文件 .wslconfig ，然后记事本编辑（其他软件也行）,然后保存即可最好保存成ANSI编码或者UTF-8格式。

具体内容根据自己需求配置，例子如下：

```
[wsl2]
memory=2GB
processors=2
swap=0
localhostForwarding=true
```

--------------------------------------------------------------------
### docker
因为笔者安装的linux版本是ubuntu 20.04，有可能不符合项目编译环境，由此可以安装docker来构建符合编译环境的镜像来编译项目。    
docker安装步骤如下
1. 更新
```
sudo apt-get update
sudo apt-get install apt-transport-https ca-certificates curl gnupg-agent software-properties-common
curl -fsSL https://download.docker.com/linux/ubuntu/gpg | sudo apt-key add -
sudo add-apt-repository "deb [arch=amd64] https://download.docker.com/linux/ubuntu $(lsb_release -cs) stable"
sudo apt-get update
```
2. 安装
查看适合自己系统的docker版本
```
apt-cache madison docker-ce
apt-cache madison docker-ce-cli
```
安装低版本
```
sudo apt-get install docker-ce=5:19.03.9~3-0~ubuntu-focal docker-ce-cli=5:19.03.9~3-0~ubuntu-focal containerd.io
```
3. 添加用户组
```
sudo usermod -aG docker your_name
```
4. 启动docker
启动
```
sudo service docker start
```
查看
```
sudo service docker status
```
5. 测试
```
sudo docker run hello-world
```
---------------------------------------------------
### 参考链接
```
https://www.qikegu.com/docs/2970
https://docs.microsoft.com/zh-cn/windows/wsl/release-notes
https://www.jianshu.com/p/bb0179b688c2
https://www.cnblogs.com/coffeebox/p/12672467.html
```
