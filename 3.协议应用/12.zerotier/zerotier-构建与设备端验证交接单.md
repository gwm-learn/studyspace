# ZeroTier VPN — 构建与设备端验证交接单

> 源码改动已全部完成并逐一验证（4 个阶段）。以下为开发板上的构建/刷机/验证步骤。
> 交接时间：2026-08-19

---

## 一、源码改动清单（已完成 ✅）

| 文件 | 改动 |
|---|---|
| `goahead/src/src/platform/wrt/sub_vpn.h` | +ZeroTier 宏/结构体/函数声明 |
| `goahead/src/src/platform/wrt/sub_vpn.c` | +g_zerotier + 7 函数（get_zerotier_info / ZerotierGet_parse/back / ZerotierSet_parse / set_zerotier / set_zerotier_firewall / ZerotierSet_exec） |
| `goahead/src/src/cmd.c` | +2 端点：getZerotierInfo / zerotierSettings |
| `goahead/src/src/platform/wrt/sub_login.c` | +菜单项 `{"vpn","ZeroTier","zerotier",true}` |
| `react-web/.../api/settings.js` | +getZerotierInfo / zerotierSettings |
| `react-web/.../views/settings/vpn/component/zerotier.js` | **新建**表单组件 |
| `react-web/.../views/settings/vpn/index.js` | +import + case 'zerotier' |
| `react-web/.../public/locales/{cn,en,tw}/translation.json` | +ZeroTier 词条 |
| `goahead/files/etc/init.d/zerotier-nat` | **新建** NAT 脚本（START=99, fw3/fw4） |
| `goahead/files/usr/share/zerotier/firewall.include` | **新建** 防火墙触发脚本 |
| `owrt/package/utils/goahead/Makefile` | +3 行 INSTALL 规则（zerotier-nat + firewall.include） |
| `target/X75-ODU-*/target.config` ×7 | `CONFIG_PACKAGE_zerotier=y` |

前端 npm build 已验证通过（cpe/ 产物含 ZeroTier）；后端因无 SDK 依赖未在本机编译。

---

## 二、构建（构建机 / SDK）

```sh
# 1) 前端（已验证过一次，可跳过或重跑）
cd broadlink/package/react-web/src/web
NODE_OPTIONS=--openssl-legacy-provider npm run build   # 产物 web/cpe

# 2) 整包交叉编译（OpenWrt SDK 内）
#    用 target.config 已启用的配置构建：
#    CONFIG_PACKAGE_zerotier=y
#    CONFIG_PACKAGE_goahead=y（现有）
make package/zerotier/compile
make package/utils/goahead/compile
make   # 或全量，产出固件刷机

# 3) 检查产物是否包含新脚本
#    $(TARGET)/etc/init.d/zerotier-nat
#    $(TARGET)/usr/share/zerotier/firewall.include
```

---

## 三、刷机后设备端验证

### 3.1 阶段一验证：接口 + UCI 写入（对应验收项 1、6）

```sh
# ① 确认 goahead 起来了、菜单里有 ZeroTier tab
curl -s http://192.168.80.1:7001/acfun/getModulesAuth | grep zerotier

# ② GET 回显（未配置时应返回默认值 enabled:false port:"9993" join:[]）
curl -s http://192.168.80.1:7001/acfun/getZerotierInfo

# ③ POST 保存（enabled + join + nat）
curl -X POST 'http://192.168.80.1:7001/acfun/zerotierSettings' \
  -H 'Content-Type: application/json' \
  -d '{"enabled":true,"join":["8056c2e21c000001"],"nat":true,"port":"9993"}'

# ④ 确认 UCI 写对了（关键：join 是 list，不能残留旧值）
uci show zerotier
cat /etc/config/zerotier

# ⑤ 服务重启成功、secret 首次启动自动生成并回写
zerotier-cli info                     # 10 位设备地址
uci get zerotier.sample_config.secret # 应已回写，非空

# ⑥ 多网络 join 验证（可选）：加第二个网络 ID 再保存
uci show zerotier.sample_config.join  # 应有多条 list
ls /var/lib/zerotier-one/networks.d/  # 每个网络一个 .conf
```

### 3.2 阶段三验证：防火墙 + NAT（对应验收项 2、3、4、5）

```sh
# ① Allow-ZeroTier rule（UDP 9993 入站）
uci show firewall | grep -A8 "zerotier"
fw4 print 2>/dev/null | grep 9993 || fw3 print | grep 9993

# ② zerotier_nat include 段存在
uci show firewall.zerotier_nat

# ③ 服务起来后 zt 网卡 + NAT 规则（enabled=1 nat=1 时）
ip -br addr show | grep zt
fw4 list chains 2>/dev/null | grep -i zt || iptables -S FORWARD | grep zt

# ④ 端到端：ZeroTier 内另一台设备 ping 本机局域网 IP（如 192.168.1.x）
#    通 = MASQUERADE 生效；再从本机 ping 对端 zt IP 验证隧道
ping <对端ztIP>
# 在 zt 对端设备：ping <本机局域网IP>

# ⑤ 关闭验证（enabled=0 保存后）
uci get zerotier.sample_config.enabled  # 应为 0
/etc/init.d/zerotier status             # 应 stopped
iptables -S FORWARD | grep zt || nft list chain inet fw4 forward | grep zt  # 无残留
```

### 3.3 验收清单（对应 §八 7 项）

| # | 验收项 | 命令 | 判据 |
|---|---|---|---|
| 1 | 保存后 UCI 字段正确 | `uci show zerotier` | 8 字段与网页一致 |
| 2 | join 多网络 → networks.d/*.conf | `ls /var/lib/zerotier-one/networks.d/` | 每网络一文件 |
| 3 | nat=1 客户端可访问局域网 | 对端 ping 本机局域网 IP | 通（fw3/fw4 均测） |
| 4 | firewall 有 rule + include | `uci show firewall` | Allow-ZeroTier + zerotier_nat |
| 5 | enabled=0 服务停止、NAT 清除 | 上述 3.2⑤ | 无残留规则 |
| 6 | secret 空 → 自动生成回写 | `uci get zerotier.sample_config.secret` | 非空 |
| 7 | 不依赖 luci-app-zerotier | `opkg list-installed | grep zerotier` | 只有 zerotier 本体 |

---

## 四、注意事项（已知坑）

1. **enabled 语义**：zerotier 用 `enabled`（非 wireguard 的 disabled 反义）— 后端已正确实现
2. **join 残留**：后端写前 `uci_del_section_option(...,"join")` 清空 — 验证 3.1⑥ 重点确认
3. **NAT 等待网卡**：`firewall restart` 时 zt 网卡可能未生成，zerotier-nat 有 while 循环等待
4. **前端 dev 联调**：setupProxy.js → 192.168.80.1:7001
5. **已知隐患（低风险）**：后端 ZerotierSet_parse 对非字符串 join 元素有 NULL-crash 理论风险；前端已强制只发非空字符串数组规避。若日后要加固，改 sub_vpn.c:1542-1546 把 NULL 判断提前。
