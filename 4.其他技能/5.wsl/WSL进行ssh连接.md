### 问题
wsl全称是"适用于 Linux 的 Windows 子系统"，可让开发人员按原样运行 GNU/Linux 环境，这里我使用的是wsl2作为工作使用中的编译服务器。

在使用wsl2编译代码时，遇到一个问题：

假设需要编译并生成一个名为test_app的应用，并将test_app拷贝到开发板中进行使用，问题就是如何能够快捷方便地完成将test_app拷贝到开发板这个动作。   

可行的方案有三种：   
1. 使用U盘，电脑插上U盘，将编译生成的test_app拷贝到U盘中，再将U盘连接上开发板，将test_app拷贝到开发板中。
2. 电脑PC使用adb命令，使用adb命令将电脑上的test_app拷贝到开发板中。
3. 开发板内系统使用scp命令，将电脑上的test_app拷贝到开发板中。  

这里我选择的是使用scp命令，即支持wsl进行ssh连接。   

---------------------------------------------------------------------

### 前提   
1. wsl2是wsl的升级版，wsl2自身实现了hyper-v虚拟机的功能，可以在wsl2上安装特定完整的linux系统，这里以ubuntu20.04版本为例。   
3. ubuntu20.04与windows连接会有一个专有的ip，即windows用虚拟网卡又做了一次NAT，并且虚拟网卡给ubuntu20.04分配的ip每次开机都会变化，所以需要做对应处理。

---------------------------------------------------------------------

### 实现
#### 获取ubuntu20.04内linux被分配的ip
启动ubuntu20.04，修改/etc/profile文件   
```
vim /etc/profile
```
文件末尾添加如下内容
```
ipaddr=$(ifconfig eth0 | grep 'inet ' | awk '{print $2}') #获取ip
sed -i '/wslhost/d' /mnt/d/ubuntu/ssh/hosts               #删除hosts文件有关键字wslhost的那一行
echo "$ipaddr wslhost" >> /mnt/d/ubuntu/ssh/hosts         #将ip wslhost写入hosts文件
```
/mnt/d指的就是windows D盘，之后的ubuntu/ssh/hosts随自身设置改动。   
之后ubuntu20.04每次启动都会更新hosts内的ip。   

#### windows 做端口转发
```
echo off

set wslhost_l=
REM 开启ubuntu
wt new-tab wsl.exe

timeout /T 8
REM PC得到ubuntu的ip
:main
for /f "delims=" %%i in (D:\ubuntu\ssh\hosts) do (
	echo %%i| findstr wslhost >nul && (
		set wslhost_l=%%i
		goto Loop
	)
	set "_%%i=1"
)
REM PC端口转发到ubuntu
:Loop
for /f "tokens=1,* delims= " %%a in ("%wslhost_l%") do (
	netsh interface portproxy delete v4tov4 listenaddress=0.0.0.0 listenport=2222
	netsh interface portproxy add v4tov4 listenaddress=0.0.0.0 listenport=2222 connectaddress=%%a connectport=22
)
goto End
:End
```
D:\ubuntu\ssh\hosts就是/mnt/d/ubuntu/ssh/hosts。 
listenport改动为自己设置的。
将以上内容写入脚本文件startubuntu.bat中，以管理员身份运行脚本，它会启动ubuntu20.04并进行端口转发。

#### 开放端口    
目前局域网内的其他机器还不能与wsl建立ssh连接，需要对对应端口进行防火墙规则进行设置

修改防火墙规则，这里我添加的是端口2222的入站规则。

1. 控制面板选择系统与安全 
2. 选择防火墙

3. 选择高级设置

4. 点击入站规则

5. 点击新建规则

6. 规则类型选择端口，点击下一步

7. 选定TCP与端口，点击下一步

8. 操作允许连接，点击下一步

9. 配置文件全选，点击下一步

10. 设置名称，点击完成

     

#### 使用
创建startubuntu.bat的桌面快捷方式，修改快捷方式以管理员身份启动。   
1.  右键选择属性 ，快捷方式点击高级 。
2.  管理员运行打钩。 

windows开机之后，双击运行桌面脚本快捷方式startubuntu.bat（此快捷方式默认管理员身份运行）。
之后与windows同局域网下的机器可以直接使用ssh进行连接，即可以使用scp拷贝文件。

```
ssh -v ubuntuUserName@windowsIP -p 2222
```

---------------------------------------------------------------------