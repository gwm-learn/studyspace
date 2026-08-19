# docker
```
docker build -t openwrt-compiler:mt7981 . --progress=plain

docker stop openwrt-compiler
docker rm openwrt-compiler
docker run --name openwrt-compiler -h openwrt-compiler -v $PWD:/w -v /etc/localtime:/etc/localtime:ro -w /w -it openwrt-compiler:mt7981 /bin/bash
```

# code
```
git clone https://git.openwrt.org/openwrt/openwrt.git

git branch  -a
git checkout  openwrt-25.12

make menuconfig
make V=s
```