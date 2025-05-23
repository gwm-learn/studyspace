# WSL2 谷歌浏览器

## 添加资源列表

```bash
sudo wget http://www.linuxidc.com/files/repo/google-chrome.list -P /etc/apt/sources.list.d/
```

## 导入谷歌公钥

```bash
wget -O - https://dl.google.com/linux/linux_signing_key.pub | sudo apt-key add -
```

## 更新系统

```bash
sudo apt-get -y update && sudo apt-get -y upgrade
```

## 安装谷歌浏览器

```bash
sudo apt-get install google-chrome-stable
```

