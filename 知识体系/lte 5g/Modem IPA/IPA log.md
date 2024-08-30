# 1. issue
    高通公司需要QXDM和IPACM日志、TCP转储和网络设备配置信息，以进行数据暂停和T-put问题调试。

## 1.1. 默认log配置的从启动开始的qxdm log

## 1.2. IPACM log

### 1.2.1. 支持systemd的平台--SA415、SDx55、SA515及以上
1. 将设备置于selinux permissive模式
    vi /etc/selinux/config
    change enforcing --> permissive

2. 在ipacm服务文件的[Service]部分插入以下内容
    vi /lib/systemd/system/ipacm.service
    StandardOutput=file:/data/ipacm_log.txt

3. 重新启动DUT以使服务文件生效
4. 执行测试 & 在设备重启之前拉取/data/ipacm_log.txt

### 1.2.2. 不支持systemd的平台--sdx24, sdx20, 9x40(9640), 9x50等
1. killall -15 ipacm
2. ipacm>/data/ipacm_log.txt &
3. adb pull /data/ipacm_log.txt

## 1.3. 在adb shell中输出一般A7信息
1. ifconfig
2. brctl show
3. route -n
4. brctl show
5. conntrack -L
6. conntrack -E
7. ip n s
8. ip r s
9. arp
10. iptables -t filter -L -n -v
11. iptables -t mangle -L -n -v
12. iptables -t nat -L -n -v

## 1.4. tcp dump and pc wireshark
    ifconfig输出中每个网络设备的tcp dump
    PC wireshark

## 1.5. IPA 统计信息
1. Common Logs required for both MSM/MDM
    Common logs for v4/v6 for all platforms:
    cat /sys/kernel/debug/ipa/stats (couple of times during UL data transfer only)
    cat /sys/kernel/debug/ipa/hdr
    cat /sys/kernel/debug/ipa/msg
    cat /sys/kernel/debug/ipa/status_stats (during UL data transfer only)

2. V4 data transfer:
    cat /sys/kernel/debug/ipa/ip4_flt
    cat /sys/kernel/debug/ipa/ip4_rt
    cat /sys/kernel/debug/ipa/ip4_nat

3. V6 data transfer:
    cat /sys/kernel/debug/ipa/ip6_flt
    cat /sys/kernel/debug/ipa/ip6_rt

4. dmesg > /data/dmesg_log.txt

## 1.6. 捕获ipa驱动的ipc_logs
    cat /sys/kernel/debug/ipc_logging/ipa/log > /data/ipa_ipc_logs.txt

## 1.7. 具有有效应用程序和调制解调器符号的崩溃转储（在结束时，但在数据传输期间）

## 1.8. 提供 mobileap_cfg.xml and IPACM_cfg.xml 配置文件

## 1.9. 使用adb shell 的命令“top”检查A7处理器CPU负载。在执行测试和执行测试过程之前，需要查询CPU负载

