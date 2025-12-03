# 安装
1. 环境准备，开启hyper-v 和windows子系统
2. 微软商店安装wsl ubuntu 24.04

# 迁移
```SHELL
wsl -l -v
  NAME            STATE           VERSION
* Ubuntu-24.04    Running         2
wsl --export Ubuntu-24.04 E:\Ubuntu\ubuntu.tar
wsl --unregister Ubuntu-24.04
wsl --import Ubuntu-24.04 E:\Ubuntu\ E:\Ubuntu\ubuntu.tar --version 2
ubuntu2404.exe config --default-user gwm
```
# 配置
1. /etc/wsl.conf
```SHELL
[boot]
systemd=true
[network]
generateResolvConf=true
hostname=wsl
```
2. C:\Users\CJTX\.wslconfig
```SHELL
[wsl2]
processors=8
memory=8GB
swap=4GB
[experimental]
autoMemoryReclaim=Gradual
sparseVhd=true
```

# docker
1. 卸载
```SHELL
# 卸载Docker CE相关包
sudo apt purge -y docker-ce docker-ce-cli containerd.io docker-buildx-plugin docker-compose-plugin

# 卸载其他可能的Docker包
sudo apt purge -y docker docker-engine docker.io containerd runc

# 清理依赖包
sudo apt autoremove -y
sudo apt autoclean

# 删除Docker配置目录
sudo rm -rf /etc/docker

# 删除Docker数据目录（可能的位置）
sudo rm -rf /var/lib/docker
sudo rm -rf /data/docker
sudo rm -rf /opt/docker-data
sudo rm -rf /opt/docker

# 删除运行时文件
sudo rm -rf /var/run/docker*
sudo rm -rf /run/docker*

# 删除日志文件
sudo rm -rf /var/log/docker*

# 删除网络配置
sudo rm -rf /var/lib/containerd

# 删除用户配置
rm -rf ~/.docker 2>/dev/null || echo "用户Docker配置不存在"

# 删除Docker仓库配置
sudo rm -f /etc/apt/sources.list.d/docker.list

# 删除GPG密钥
sudo rm -f /usr/share/keyrings/docker*.gpg
sudo rm -f /usr/share/keyrings/docker-archive-keyring.gpg

# 删除Docker用户组
sudo groupdel docker 2>/dev/null || echo "docker组不存在"

# 从用户组中移除docker
sudo gpasswd -d $USER docker 2>/dev/null || echo "用户不在docker组中"
```

2. 安装
```SHELL
# 下载Docker GPG密钥
curl -fsSL https://download.docker.com/linux/ubuntu/gpg | sudo gpg --dearmor -o /usr/share/keyrings/docker.gpg

# 设置密钥文件权限
sudo chmod a+r /usr/share/keyrings/docker.gpg

# 验证密钥文件
ls -la /usr/share/keyrings/docker.gpg

# 验证密钥内容
sudo gpg --no-default-keyring --keyring /usr/share/keyrings/docker.gpg --list-keys

# 添加Docker仓库到APT源
echo "deb [arch=$(dpkg --print-architecture) signed-by=/usr/share/keyrings/docker.gpg] https://download.docker.com/linux/ubuntu $(lsb_release -cs) stable" | sudo tee /etc/apt/sources.list.d/docker.list > /dev/null

# 验证仓库配置
cat /etc/apt/sources.list.d/docker.list

# 更新包列表
sudo apt update

# 检查Docker包是否可用
apt-cache policy docker-ce

# 查看可用的Docker版本
apt-cache madison docker-ce | head -5

# 安装Docker CE及相关组件
sudo apt install -y docker-ce docker-ce-cli containerd.io docker-buildx-plugin docker-compose-plugin

# 验证安装
docker --version
docker compose version

# 启动Docker服务
sudo service docker start

# 检查服务状态
sudo service docker status
```

3. 配置/etc/docker/daemon.json
```SHELL
{"features":{"buildkit": false}}
```

# vscode
1. 安装wsl插件
2. wsl + vscode 编辑代码