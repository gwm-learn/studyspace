# nftables和iptables的差别
nftables不仅替代了iptables的功能，还替代了ip6tables、arptables、etables。   
nftables拥有使用额外脚本的能力。   
nftables不包含任何内置表。   

# nftables基本语法
```SHELL
nft <command> <subcommand> <chain> <rule definition>
```
command:add、list、insert、delete、flush   
subcommand:table、chain、rule   

# nftables特性
地址族：   
1. ip：IPv4地址   
2. ip6：IPv6地址   
3. inet：ARP   
4. bridge：处理桥接数据包   

钩子：   
1. prerouting：刚到达并未被nftables的其他部分所路由或处理的数据包   
2. input：以及被接收并且已经经过prerouting钩子的传入数据包   
3. forward：如果数据包将被发送到另一个设备，它将会通过forward钩子   
4. output：从本地系统传出的数据包   
5. postrouting：仅仅在离开系统之前，postrouting钩子使得可以对数据包进行进一步的处理   

