# description

# build docker image
```SHELL
# docker image name: ubuntu-server, tag:snmpd
docker build -t ubuntu-server:snmpd .
```

# run docker container
```SHELL
# $PWD is source code root dir
docker run --name compiler-snmpd -v $PWD/conf:/etc/snmp -v /etc/localtime:/etc/localtime -w /root/ -i -t ubuntu-server:snmpd /bin/bash 
```

# compile
```SHELL
# compile code

```

# rerun container
```SHELL
# get docker container info id
docker ps -a 
# rerun docker container with id
docker exec -it 9df70f9a0714 /bin/bash 
```

# docker network
```SHELL
docker network create --driver macvlan --subnet=192.168.100.0/24 --gateway=192.168.100.1 -o parent=ens33 net33
```

# run docker container
```SHELL
# $PWD is source code root dir
docker run --name SNMP-SERVER --cap-add=NET_ADMIN --network=net33 --ip 192.168.100.156 -v $PWD/conf:/etc/snmp -v /etc/localtime:/etc/localtime -w /root/ -i -t -d ubuntu-server:snmpd /bin/bash
```