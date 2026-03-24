# description

# acount
```SHELL
# 同步容器与宿主机环境
# 创建开发用户（UID/GID与宿主机gwm用户匹配）
# 根据以下查询到的宿主机信息修改开发用户同步到Dockerfile中
gwm@ubuntu-server:~/CODE/Q22$ whoami
gwm
gwm@ubuntu-server:~/CODE/Q22$ id gwm
uid=1000(gwm) gid=1000(gwm) groups=1000(gwm),4(adm),24(cdrom),27(sudo),30(dip),46(plugdev),110(lxd),999(docker)
gwm@ubuntu-server:~/CODE/Q22$

```
# example
```SHELL
# 用户：gwm uid：1000，用户组：gwm，gid：1000，根据自身宿主机用户相关信息进行替换到Dockerfile文件中
RUN groupadd -g 1000 gwm && \
    useradd -u 1000 -g 1000 -m -s /bin/bash gwm && \
    echo 'gwm ALL=(ALL) NOPASSWD:ALL' >> /etc/sudoers
# 设置工作目录
WORKDIR /w/
# 默认以gwm用户运行
USER gwm
```

# build docker image
```SHELL
# docker image name: ubuntu-compiler, tag:T830Q22
docker build -t ubuntu-compiler:T830Q22 . --progress=plain
```

# svn 免认证例子
```SHELL
gwm@ubuntu-server:~/.subversion$ ls
auth  config  README.txt  servers
gwm@ubuntu-server:~/.subversion$ cat auth/svn.simple/489bdeb44c65fa2da865b49696063fd3
K 15
svn:realmstring
V 50
<http://120.76.175.235:8090> Subversion Repository
K 8
username
V 10
gaoweiming
K 8
password
V 10
gaoweiming
K 8
passtype
V 6
simple
END
gwm@ubuntu-server:~/.subversion$ 

```

# run docker container
```SHELL
# $PWD is source code root dir
# 这里的-v /home/gwm/.subversion:/home/gwm/.subversion:ro根据自身宿主机信息修改填写用以免输入svn 用户密码
docker run --name Q22 -v $PWD:/w/ -v /etc/localtime:/etc/localtime:ro -v /home/gwm/.subversion:/home/gwm/.subversion:ro -w /w/ -i -t ubuntu-compiler:T830Q22 /bin/bash 
```

# compile
```SHELL
# compile code
./customize_build_image.sh MT6890_AX1800_DLINK 
```

# rerun container
```SHELL
# get docker container info id
docker ps -a 
# rerun docker container with id
docker exec -it 9df70f9a0714 /bin/bash 
```

# export image
```SHELL
docker save -o ubuntu-compiler-T830Q22.tar ubuntu-compiler:T830Q22
```

# load image
```SHELL
docker load -i ubuntu-compiler-T830Q22.tar
```