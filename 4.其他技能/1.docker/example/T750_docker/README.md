# description


# build docker image
```SHELL
# docker image name: ubuntu-compiler, tag:T750
docker build -t ubuntu-compiler:T750 .
```

# run docker container
```SHELL
# $PWD is source code root dir
docker run --name compiler -v $PWD:/root/workspack/ -v /etc/localtime:/etc/localtime -w /root/workspack/ -i -t ubuntu-compiler:T750 /bin/bash 
```

# compile
```SHELL
# compile code
./customize_buildimage.sh MT6890_AX1800_DLINK 
```

# rerun container
```SHELL
# get docker container info id
docker ps -a 
# rerun docker container with id
docker exec -it 9df70f9a0714 /bin/bash 
```