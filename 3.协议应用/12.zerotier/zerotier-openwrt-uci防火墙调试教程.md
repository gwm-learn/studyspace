# ZeroTier OpenWrt UCI + 防火墙 调试教程

> 目标：在一台**已装 `zerotier` 包、但没有 luci-app-zerotier**（或想验证手工配置）的 OpenWrt 设备上，用纯命令行把 ZeroTier 跑通、加入网络、打通防火墙，验证「能不能正常工作」。
> 全程 SSH 操作，命令可直接复制。

---

## 0. 环境确认（先跑一遍）

```sh
# 1) zerotier 是否已装、可执行文件在哪（zerotier-cli 是 zerotier-one 的软链）
which zerotier-one zerotier-cli zerotier-idtool
ls -l /etc/init.d/zerotier /etc/config/zerotier

# 2) TUN 模块是否就绪（zt 网卡依赖 kmod-tun）
ls /dev/net/tun && echo "tun OK" || echo "tun 缺失 opkg install kmod-tun"

# 3) 防火墙后端是 fw4(nftables) 还是 fw3(iptables) —— 全文都要按这个分流
command -v nft >/dev/null && echo "fw4 (nftables)" || echo "fw3 (iptables)"
```

> 关键事实：**zerotier 主包不碰防火墙**，`/etc/config/zerotier` 由主包提供，防火墙完全靠你自己（或 luci-app-zerotier）配。

---

## 1. UCI 基础配置

主服务 `/etc/init.d/zerotier` **只读这些 option**：`enabled`、`join`、`port`、`secret`、`config_path`、`copy_config_path`、`local_conf`。
（`nat` 不是主服务读的，是 NAT 辅助脚本读的，见第 4 节。）

```sh
# 先看当前默认配置
uci show zerotier

# 1) 启用
uci set zerotier.sample_config.enabled='1'

# 2) 端口（不设则默认 9993，数据面 UDP + 控制面共用）
uci set zerotier.sample_config.port='9993'

# 3) join：清空默认的公共 Earth 网络，加入自己的网络 ID（16 位 hex）
#    —— 默认 config 里是 list join '8056c2e21c000001'，必须整表清空再 add_list，否则残留
uci -q delete zerotier.sample_config.join
uci add_list zerotier.sample_config.join='<你的16位hex网络ID>'

# 4) (可选) 需要 zt 客户端访问本机局域网时，给 NAT 脚本用的开关
uci set zerotier.sample_config.nat='1'

uci commit zerotier
```

> 网络 ID 在 [my.zerotier.com](https://my.zerotier.com) 建网获取。测试期可先用公共网 `8056c2e21c000001`，但**正式环境务必删掉它**。
> `secret` 留空即可——主服务首次启动会自动 `zerotier-idtool generate` 并 `uci commit` 回写，设备 10 位地址据此固定。

---

## 2. 启动并验证加入网络

```sh
/etc/init.d/zerotier enable        # 开机自启
/etc/init.d/zerotier restart       # 立即生效（reload 也是 stop+start）

# 看日志确认启动/生成 secret/join
logread -e zerotier

# 设备 10 位地址（去 ZeroTier Central 授权这个节点用）
zerotier-cli info

# 已加入的网络及分配到的 IP / 状态
zerotier-cli listnetworks

# 看 zt 网卡是否起来、是否拿到 IP
ip -br addr show | grep zt
```

**通过判据**：`listnetworks` 里该网络显示 `OK` 且 `ip -br addr` 有 `zt*` 网卡带 IP（通常是 `10.x.x.x`）。若状态是 `REQUESTING_CONFIGURATION` / `ACCESS_DENIED`，说明还没在 Central 授权该节点。

---

## 3. 防火墙：放行入站 UDP 9993

**为什么**：让外部 peer 能直连本机的主端口（打洞 + 数据面）。ZeroTier 默认端口 9993，UDP 为主。

```sh
uci set firewall.zerotier=rule
uci set firewall.zerotier.name='Allow-ZeroTier'
uci set firewall.zerotier.src='wan'
uci set firewall.zerotier.dest_port='9993'
uci set firewall.zerotier.proto='udp'
uci set firewall.zerotier.target='ACCEPT'
uci commit firewall
/etc/init.d/firewall restart
```

> 这套 UCI rule 对 fw3/fw4 都生效（两者都消费同一份 `firewall` UCI）。若你的 wan 区名不是 `wan`（少数设备是 `wwan`），把 `src` 改成对应 zone 名。

---

## 4. 防火墙：NAT（让 zt 客户端访问本机局域网）

这一步 `enabled=1` **且** `nat=1` 才生效。分「临时验证」和「持久化脚本」两档。

### 4.1 先找 zt 网卡名

```sh
ls /sys/class/net | grep zt        # 例如 ztdiysk6pz
# 或用脚本同款动态探测：
ifconfig | grep zt | awk '{print $1}'
```

### 4.2 临时验证（手动插规则，重启失效）

**fw3（iptables）：**
```sh
iptables -I FORWARD -i ztdiysk6pz -j ACCEPT
iptables -I FORWARD -o ztdiysk6pz -j ACCEPT
iptables -t nat -I POSTROUTING -o ztdiysk6pz -j MASQUERADE
```

**fw4（nftables）：**
```sh
nft insert rule inet fw4 forward iifname "ztdiysk6pz" accept
nft insert rule inet fw4 forward oifname "ztdiysk6pz" accept
nft insert rule inet fw4 srcnat oifname "ztdiysk6pz" counter masquerade
```

### 4.3 持久化（= 要替代 luci-app-zerotier 的实现，动态等 zt 网卡）

**① 写 NAT 脚本** `/etc/init.d/zerotier-nat`（`START=99`，晚于主服务 `START=90`；读 `enabled`+`nat` 两个开关）：

```sh
#!/bin/sh /etc/rc.common
START=99
PROG=/etc/init.d/zerotier

get_config() {
    config_get_bool enabled $1 enabled 0
    config_get_bool nat $1 nat 0
}

start() {
    config_load zerotier
    config_foreach get_config zerotier
    [ $enabled -eq 0 ] && return 0
    $PROG running || return 1
    [ $nat -eq 0 ] && return 0

    # 等 zt 网卡出现（firewall restart 时可能还没起来）
    while [ "$(ifconfig | grep 'zt' | awk '{print $1}')" = "" ]; do sleep 1; done

    local zt_devs FW ip_segment
    zt_devs="$(ifconfig | grep 'zt' | awk '{print $1}')"
    [ -x "$(command -v nft)" ] && FW="fw4" || FW="fw3"

    for i in ${zt_devs}; do
        ip_segment="$(ip route | grep "dev $i proto kernel" | awk '{print $1}')"
        if [ "$FW" = "fw3" ]; then
            iptables -S FORWARD | grep -sq "$i" && continue
            iptables -I FORWARD -i "$i" -j ACCEPT
            iptables -I FORWARD -o "$i" -j ACCEPT
            iptables -t nat -I POSTROUTING -o "$i" -j MASQUERADE
            [ -n "$ip_segment" ] && iptables -t nat -I POSTROUTING -s "${ip_segment}" -j MASQUERADE
        else
            nft list chain inet fw4 forward | grep -sq "$i" && continue
            nft insert rule inet fw4 forward iifname "$i" accept
            nft insert rule inet fw4 forward oifname "$i" accept
            nft insert rule inet fw4 srcnat oifname "$i" counter masquerade
            [ -n "$ip_segment" ] && nft insert rule inet fw4 srcnat ip saddr "${ip_segment}" counter masquerade
        fi
        logger -t zerotier-nat "added nat rules for $i"
    done
}

stop() {
    local zt_devs FW ip_segment rule chains handles
    zt_devs="$(ifconfig | grep 'zt' | awk '{print $1}')"
    [ -z "${zt_devs}" ] && return 0
    [ -x "$(command -v nft)" ] && FW="fw4" || FW="fw3"
    for i in ${zt_devs}; do
        ip_segment="$(ip route | grep "dev $i proto kernel" | awk '{print $1}')"
        if [ "$FW" = "fw3" ]; then
            iptables -D FORWARD -i "$i" -j ACCEPT 2>/dev/null
            iptables -D FORWARD -o "$i" -j ACCEPT 2>/dev/null
            iptables -t nat -D POSTROUTING -o "$i" -j MASQUERADE 2>/dev/null
            [ -n "$ip_segment" ] && iptables -t nat -D POSTROUTING -s "${ip_segment}" -j MASQUERADE 2>/dev/null
        else
            chains="forward srcnat"
            rule="$i"
            [ -n "$ip_segment" ] && rule="${rule}|$ip_segment"
            for c in $chains; do
                handles=$(nft -a list chain inet fw4 $c | grep -E "$rule" | cut -d'#' -f2 | cut -d' ' -f3)
                for h in $handles; do
                    nft delete rule inet fw4 $c handle $h
                done
            done
        fi
    done
}
```

```sh
chmod +x /etc/init.d/zerotier-nat
```

**② 写触发脚本** `/usr/share/zerotier/firewall.include`（防火墙 reload 时被调用）：

```sh
#!/bin/sh
/etc/init.d/zerotier-nat enabled && /etc/init.d/zerotier-nat restart
exit 0
```

```sh
chmod +x /usr/share/zerotier/firewall.include
```

**③ 在 firewall UCI 里注册 include 段**（替代 luci 的 uci-defaults）：

```sh
uci set firewall.zerotier_nat=include
uci set firewall.zerotier_nat.type='script'
uci set firewall.zerotier_nat.path='/usr/share/zerotier/firewall.include'
uci set firewall.zerotier_nat.reload='1'
uci commit firewall
/etc/init.d/firewall restart
```

---

## 5. 端到端验证清单

```sh
# ① 主服务在跑
/etc/init.d/zerotier status

# ② 拿到 zt 网卡 + IP
ip -br addr show | grep zt

# ③ 设备已加入网络且 OK
zerotier-cli listnetworks

# ④ 有 peer 在线（说明能连上网络控制面）
zerotier-cli listpeers        # 有 LEAF/DIRECT 或 RELAY 都算通

# ⑤ 入站放行 rule 已存在
#    fw3: iptables -S | grep -i zerotier  或  iptables -L -n | grep 9993
#    fw4: nft list ruleset | grep -A2 9993

# ⑥ NAT 规则已插入（nat=1 时）
#    fw3: iptables -S FORWARD | grep zt ; iptables -t nat -S POSTROUTING | grep zt
#    fw4: nft list chain inet fw4 forward | grep zt ; nft list chain inet fw4 srcnat | grep zt
```

**真正的「能用」判据**：
- 局域网内某台机器（或本机）`ping <对端 zt IP>` 能通 → 隧道 + 转发 OK。
- 从 ZeroTier 里的另一台设备 `ping <本机局域网某设备 IP>` 能通 → NAT(MASQUERADE) 生效。
- 从外网 peer 看 `zerotier-cli listpeers` 显示 `DIRECT` 而不是 `RELAY` → 入站 UDP 9993 放行成功、NAT 打洞成功。

---

## 6. 常见问题排查

| 症状 | 原因 / 处理 |
|---|---|
| `listnetworks` 一直 `ACCESS_DENIED` | 去 ZeroTier Central 授权该节点（用 `zerotier-cli info` 的 10 位地址） |
| 没有 `zt*` 网卡 | `kmod-tun` 未装/未加载：`opkg install kmod-tun`，或 `ls /dev/net/tun` 检查 |
| `secret` 每次变 / 地址漂移 | 检查 UCI 里 secret 是否被回写；重启前先 `uci commit zerotier` |
| join 没生效 / 还连着旧网络 | 旧 `join` list 残留：`uci show zerotier | grep join` 确认已清空 |
| 外部 peer 连不上（RELAY 而非 DIRECT） | 入站 UDP 9993 未放行，或端口/`src` zone 名写错 |
| zt 客户端 ping 不通局域网 | `nat=1` 未设，或 NAT 规则没插（检查第 ⑤⑥ 步） |
| 改了配置不生效 | 主服务靠 `service_triggers` 监听 UCI，但仍建议显式 `restart`；防火墙改完要 `/etc/init.d/firewall restart` |
