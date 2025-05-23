# 安装
```SH
sudo yum install samba samba-client -y
```
# 配置
## 开机启动
```SH
sudo systemctl enable smb.service
```
## 配置文件
```SH
sudo cp /etc/samba/smb.conf /etc/samba/smb.conf.bak
sudo vim /etc/samba/smb.conf
```
## 文件内容
```SH
[global]
workgroup = WORKGROUP
server string = Samba Server %v
netbios name = centos
security = user
map to guest = bad user
dns proxy = no
#============================ Share Definitions ==============================   
[project]
path = /home/gwm/Project
browsable =yes
writable = yes
guest ok = yes
read only = no
public = no
vaild users = gwm
```
## 创建用户
```SH
sudo smbpasswd -a gwm
```
## 服务控制
```SH
systemctl enable smb.service
systemctl enable nmb.service
systemctl restart smb.service
systemctl restart nmb.service
```

## 防火墙
```SH
sudo firewall-cmd --permanent --zone=public --add-service=samba
sudo firewall-cmd --reload
```

## selinux
```SH
sudo setenforce 0
sudo vim /etc/selinux/config
#将SELINUX=enforcing改为SELINUX=disabled
```

## 文件权限
```SH
sudo chmod 777 /home/gwm/Project
```

