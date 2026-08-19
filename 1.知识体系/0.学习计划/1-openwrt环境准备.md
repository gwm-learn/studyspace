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

//todo 
OpenWrt 架构本质
feeds 机制
config 配置原理
固件打包流程