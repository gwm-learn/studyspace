# WSL2 挂载U盘

## 创建文件夹

打开ubuntu 在/mnt/下创建一个文件夹(f)，以便将U盘挂载在此

```bash
sudo mkdir /mnt/f
```

## 挂载

将U盘挂载在上面创建的文件夹F中,U盘插入电脑之后在电脑的盘符为F，需要适合用drvfs进行挂载

```bash
sudo mount -t drvfs F: /mnt/f
```

## 卸载

如果想在windows下正常弹出，需要先将umount

```bash
sudo umount /mnt/f
```

