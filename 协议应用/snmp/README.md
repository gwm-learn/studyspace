# net-snmp 软件包相关
1. 下载地址，选择5.9.4	   
```
    http://www.net-snmp.org/download.html
```
2. 解压
```
    tar zxvf net-snmp-5.9.4.tar.gz
```
3. 编译配置
```
# 后续其他参数会根据实际要求修改实现，该configure会在源码顶层目录中的Makefile中调用
./configure --prefix=/var/net-snmp --host=mips-linux-uclibc --build=x86_64-linux-gnu --with-cc=mips-linux-uclibc-gcc-4.8.5 --with-ar=mips-linux-uclibc-gcc-ar --with-default-snmp-version="2" --with-sys-contact="xxx@xxx.com" --with-sys-location="location" --with-logfile="/var/log/snmpd.log" --with-persistent-directory="/var/net-snmp" --without-opaque-special-types --without-rpm --without-perl-modules --disable-manuals --disable-ucd-snmp-compatibility --disable-embedded-perl --disable-snmptrapd-subagent --disable-scripts -enable-mfd-rewrites --enable-shared=no --enable-mini-agent --with-endianness=little
```

# net-snmp 移植
1. net-snmp 相关目录
```
# 软件包移植的目录
swith/sdk/src/app/snmp

# 软件生成的目录
swith/sdk/src/app/snmp/build/bin

# 软件配置生成的目录
swith/sdk/src/app/snmp/build/conf

# 软件安装最终安装的目录
swith/kernel/uClinux/romfs/bin

# 软件配置最终安装的目录
swith/kernel/uClinux/romfs/var/net-snmp
```

2. net-snmp 移植后编译流程
```
1) swith/sdk/src/app/snmp/Makefile 执行build目标，先执行clean操作，再执行configure，再进行编译
2) swith/sdk/src/app/snmp/Makefile 执行romfs目标，将编译好的文件根据需要打包到文件系统中
```

# net-snmp 二次开发
1. MIB文件   
自定义创建添加MIB文件BL-VENDOR-MIB.txt，即swith/sdk/src/app/snmp/src/mibs/BL-VENDOR-MIB.txt
```
BL-VENDOR-MIB DEFINITIONS ::= BEGIN

-- ---------------------------------------------------------- --
-- MIB for broadlink vendor 
-- ---------------------------------------------------------- --

IMPORTS
    enterprises
        FROM RFC1155-SMI
    Integer32,OBJECT-TYPE
        FROM SNMPv2-SMI
    DisplayString
        FROM SNMPv2-TC
    TEXTUAL-CONVENTION
        FROM SNMPv2-TC;

vendor    OBJECT IDENTIFIER ::= {enterprises 36000}

readTest  OBJECT IDENTIFIER ::= {vendor 1}
writeTest OBJECT IDENTIFIER ::= {vendor 2}
SwitchMac OBJECT IDENTIFIER ::= {vendor 3}

    readTest       OBJECT-TYPE           --对象名称
    SYNTAX         Integer32             --类型
    MAX-ACCESS     read-only             --访问方式
    STATUS         current               --状态
    DESCRIPTION    "vendor test read"    --描述
    ::= {vendor 1}                       --父节点

    writeTest      OBJECT-TYPE           --对象名称
    SYNTAX         DisplayString         --类型
    MAX-ACCESS     read-write            --访问方式
    STATUS         current               --状态
    DESCRIPTION    "vendor test write"   --描述
    ::= {vendor 2}                       --父节点

    SwitchMac      OBJECT-TYPE           --对象名称
    SYNTAX         DisplayString         --类型
    MAX-ACCESS     read-write            --访问方式
    STATUS         current               --状态
    DESCRIPTION    "vendor test write"   --描述
    ::= {vendor 3}                       --父节点

END

--说明：
--该文件定义了客制化节点vendor，节点号为1.3.6.1.4.36000，enterprises的OID是1.3.6.1.4
--vendor下添加了子节点readTest进行读操作测试，添加了子节点writeTest进行读写操作测试
```

2. 二次开发代码   
二次开发代码都在目录swith/sdk/src/app/snmp/src/bl_vendor   
vendor.c,vendor.h是节点实现get set逻辑的部分   
ipc_client.c,ipc_client.h是实现与switch平台通信的部分   
util.c,util.h是放置一些其他函数的部分   

3. 二次开发相关编译选项   
    1) configure文件
    ```
    MODULECPP="$CPP $PARTIALTARGETFLAGS $CPPFLAGS -DNETSNMP_FEATURE_CHECKING -I${srcdir}/include -I${srcdir}/agent/mibgroup"
    #修改如下
    MODULECPP="$CPP $PARTIALTARGETFLAGS $CPPFLAGS -DNETSNMP_FEATURE_CHECKING -I${srcdir}/include -I${srcdir}/agent/mibgroup -I${srcdir}/../../../../include -I${srcdir}/../../../../system/include"
    ```
    2) Makefile文件(源码顶层)
    ```
    添加选项--with-mib-modules="$(SNMP_MIB_VENDOR_MODULES)"
    SNMP_MIB_VENDOR_MODULES :=	../../bl_vendor/vendor \
                            ../../bl_vendor/ipc_client \
                            ../../bl_vendor/util
    ```

# net-snmp 运行
1. 运行后台进程
```
# 启动snmpd
snmpd_start.sh
# 关闭snmpd
snmpd_stop.sh
# 重启snmpd
snmpd_restart.sh
```

2. 查看log
```
cat /var/log/snmpd.log
```

3. 本地查看数据   
snmpwalk位于swith/sdk/src/app/snmp/apps，并未打包安装，需要额外导入到系统中
```
snmpwalk -v 1 127.0.0.1 -c public .1
```

4. 远程查看方式1
```
# ip 为交换机本地ip
snmpwalk -v 1 ip -c public .1
```

5. 远程查看方式2   
MIB Browser   
链接：https://blog.csdn.net/mayue_web/article/details/126267593   

# 其他说明
目前c语言实现了get set trap，inform并未实现

# 添加trap
运行snmptrapd接收trap消息   
```
snmptrapd -C -c /var/net-snmp/conf/snmptrapd.conf -df &
```
snmptrapd.conf 配置
```
authCommunity log,execute,net secret
```
