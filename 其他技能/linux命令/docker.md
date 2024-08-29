# 运行
## 查看镜像
```SHELL
docker images
```

## 查看所有容器
```SHELL
docker ps -a
```

## 创建容器
```SHELL
docker run --name T750-COMPILER -v $PWD:/root/workspack/ -v /etc/localtime:/etc/localtime -w /root/workspack/ -i -t -d  ubuntu-compiler:T750 /bin/bash
```

## 启动已停止的容器
```SHELL
docker start b51658521542
```

## 进入容器
```SHELL
docker exec -it b51658521542 /bin/bash
```
