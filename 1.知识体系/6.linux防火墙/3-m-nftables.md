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

# nft 命令实用指南

## 表与链的创建

nftables 没有内置表和链，一切从创建开始：

```shell
# 创建 inet 地址族（同时处理 IPv4/IPv6）的 filter 表
nft add table inet filter
# 在表中创建 input 链，绑定 input 钩子
nft add chain inet filter input { type filter hook input priority 0 \; }
```

`inet` 地址族一套规则同时覆盖 IPv4 和 IPv6，避免忘记配置 v6 规则导致的漏防。常用链优先级：raw(-300) → mangle(-150) → nat(-100) → filter(0)。

## 基础规则增删改查

```shell
nft list ruleset                      # 列出全部规则集
nft add rule inet filter input tcp dport 22 accept    # 放行 ssh
nft insert rule inet filter input ip saddr 192.168.1.0/24 accept  # 链首插入
nft -a list chain inet filter input   # -a 显示每条规则的 handle
nft delete rule inet filter input handle 3            # 按 handle 删除
nft delete table inet filter          # 删除整张表
```

## 常用匹配与动作

| 匹配/动作 | 示例 | 说明 |
|-----------|------|------|
| 端口匹配 | `tcp dport 80` / `udp dport 53` | 可写端口范围 `1024-65535` |
| 地址匹配 | `ip saddr 10.0.0.0/8` / `ip6 daddr ::1` | 支持 CIDR |
| 接口匹配 | `iifname "wan0"` / `oifname "lan"` | 入/出接口，可用通配符 |
| 状态匹配 | `ct state established,related` | 连接跟踪状态匹配 |
| 计数器 | `counter` | 统计命中次数，配合 `nft list` 查看 |
| 日志 | `log prefix "DROP: "` | 打印内核日志，便于排查 |
| 跳转 | `jump my_chain` / `goto my_chain` | 调用自定义链 |
| 动作 | `accept / drop / reject / queue` | 放行、丢弃、拒绝（回 ICMP）、入队 |

## 规则集文件与原子加载

nftables 支持把整个规则集写入脚本文件，一次性原子加载，出错整体回滚，比 iptables 逐条执行更安全：

```shell
# /etc/nftables.conf 示例片段
table inet filter {
    chain input {
        type filter hook input priority filter; policy drop;
        ct state established,related accept
        iifname "lo" accept
        tcp dport 22 accept
    }
}
```

```shell
nft -f /etc/nftables.conf   # 加载整个规则集
nft list ruleset > /etc/nftables.conf   # 导出当前规则集用于备份
```

## 与 OpenWrt 的关系

OpenWrt 使用 nftables 后端（fw4），规则通过 `/etc/config/firewall` 的 UCI 配置描述，由 fw4 翻译生成 nftables 规则集，普通用户不直接编辑 nftables 脚本。调试时可用 `nft list ruleset` 查看 fw4 生成的实际规则。复杂匹配（如多端口、多网段）可用集合合并，减少链遍历次数，提升规则集性能。

