# WSL2 桌面设置

## 更新系统

```bash
sudo apt-get -y update && sudo apt-get -y upgrade
```

## 安装xfce4和xrdp

```bash
sudo apt install -y xfce4 xrdp
```

xfce4过程中会出现选择显示管理DM选择的提示,建议用`lightdm`

如果错过了安装过程中出现的这个向导,那么可以在安装完成后执行下面的命令重新设置DM

```bash
sudo dpkg-reconfigure lightdm
```

## 修改xrdp默认端口

```bash
sudo vim /etc/xrdp/xrdp.ini
# 修改下面这一行,将默认的3389改成其他端口即可
port=3390
```

## 指定登录session类型

```bash
vim ~/.xsession

# 写入下面内容(就一行)
xfce4-session
```

## 启动xrdp

由于WSL2里面不能用`systemd`,所以需要手动启动

```bash
sudo /etc/init.d/xrdp start
```

## 远程访问

在Windows系统中运行`mstsc`命令打开远程桌面连接,地址输入`localhost:3390`

输入WSL2中使用的账号密码进行登录

## 防止黑屏

进入图形界面，点击右上角Applications--->Settings--->Light Locker Settings

Locking->Automatically lock the session选择Never

## 语言支持

```bash
sudo apt-get install fonts-arphic-uming language-pack-gnome-zh-hant fcitx-ui-qimpanel ibus-table-wubi fonts-arphic-ukai fcitx-sunpinyin ibus-table-cangjie3 fonts-noto-cjk-extra gnome-user-docs-zh-hans fcitx-table-cangjie ibus-libpinyin fcitx-module-cloudpinyin fcitx-table-wubi fcitx-pinyin fcitx-chewing ibus-table-quick-classic ibus-chewing fonts-noto-cjk ibus-table-cangjie5 language-pack-gnome-zh-hans
```

