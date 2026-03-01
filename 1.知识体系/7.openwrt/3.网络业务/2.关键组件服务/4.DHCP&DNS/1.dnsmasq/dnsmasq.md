# 协议与功能架构
## 协议支持
DNS协议：支持标准DNS查询（UDP/TCP 53端口）、DNS缓存、DNS转发、域名过滤（支持正则表达式和通配符）。   
DHCP协议：提供IPv4/IPv6地址分配、租期管理、静态IP绑定、Option字段扩展（如网关/DNS服务器推送）。   
TFTP协议：可选功能，用于网络启动（PXE）场景。   

## 核心功能
DNS代理：缓存解析结果减少延迟，支持按域名分流（如国内/国外域名走不同上游DNS）。   
DHCP服务器：为局域网设备动态分配IP，支持MAC绑定、保留地址池（start/limit配置）。   
域名劫持防护：通过bogus-nxdomain过滤虚假DNS响应（3）。   

# 工作流程分析
## 启动流程

### 初始化阶段：
由/etc/init.d/dnsmasq脚本启动，读取/etc/config/dhcp和/etc/config/network配置。   
生成运行时配置文件/var/etc/dnsmasq.conf ，合并规则如resolv-file=/tmp/resolv.conf.auto （38）。   
### 热插拔处理：
网络接口变化时，通过/etc/hotplug.d/iface/25-dnsmasq脚本重启服务，避免配置冲突（2）。   

## DNS解析流程
```
graph TD  
A[客户端请求] --> B{dnsmasq缓存命中?}  
B -- 是 --> C[直接返回结果]  
B -- 否 --> D{域名匹配规则?}  
D -- 本地hosts或劫持列表 --> E[返回自定义IP]  
D -- 指定上游DNS --> F[转发到对应服务器]  
D -- 默认规则 --> G[转发到resolv.conf.auto 中的上游]  
``` ```  
- **优先级**：本地缓存 > `/etc/hosts` > 自定义规则 > 上游DNS（[1]()[5]()）。
```

## DHCP分配流程
四步握手：Discover → Offer → Request → Ack。   
租期管理：默认租期12小时（可配置leasetime），续租在50%和87.5%时间点触发（8）。   

# 详细配置指南
## 核心配置文件
主配置：/etc/config/dhcp
```
config dnsmasq  
    option domainneeded '1'        # 忽略无域名请求  
    option boguspriv '1'           # 屏蔽私有IP响应  
    option resolvfile '/tmp/resolv.conf.auto'   # 上游DNS来源  
config dhcp 'lan'  
    option interface 'lan'  
    option start '100'             # IP池起始地址  
    option limit '150'             # 地址数量  
```
运行时配置：/var/etc/dnsmasq.conf （自动生成，包含所有合并规则）。   

## 高级配置示例

### 域名分流（国内外分离）：
#/etc/dnsmasq.d/gfw.conf   
```
server=/google.com/8.8.8.8          # 指定域名走特定DNS  
server=/cn/114.114.114.114         # 国内域名走本地DNS  
```   
### 广告屏蔽：
```
address=/ad.com/0.0.0.0             # 域名指向无效IP  
addn-hosts=/etc/hosts.ads           # 加载自定义屏蔽列表  
```
### IPset联动（配合防火墙规则）：
```
ipset=/youtube.com/gfwlist          # 解析结果自动加入ipset集合  
``` 
#### 调试命令
查看DNS缓存：dnsmasq --test   
实时日志：logread -f | grep dnsmasq   

##  强制重载配置：
/etc/init.d/dnsmasq restart（56）。

# 典型问题与优化
## DNS污染处理   
使用pdnsd或dnscrypt-proxy作为上游，通过TCP查询绕过污染（76）。   
## 性能优化   
启用no-negcache禁用否定缓存，减少无效查询延迟。   
分离dnsmasq-full版本以支持ipset和高级路由功能（5）。   
## 配置冲突   
避免同时修改Web界面和手动编辑文件，优先通过LUCI配置生成器操作（8）。   

# 开发遇到问题
1. dhcp.@dnsmasq[0].dhcpscript该参数可以运行脚本，以实时调试dnsmasq，查看dnsmasq触发的事件   
2. 系统时间同步之前分配了ip，时间同步之后系统时间大幅度变化超过了租期时间，会触发del事件   