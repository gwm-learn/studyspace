CURR_DIR := $(shell pwd)
CURR_BUILD_DIR = $(CURR_DIR)/build
SNMP_LOG_DIR := /var/log/snmpd.log
ROOTFS_SNMP_UCI = /etc/config/snmpd
ROOTFS_SNMP_CONF = /var/net-snmp/conf
ROOTFS_SNMP_MIBS = /var/net-snmp/share/snmp/mibs
SRC_MAKEFILE = $(CURR_DIR)/src/Makefile

EXEC = snmpd

CFLAGS +=	-I$(CURR_DIR)/../../../include \
			-I$(CURR_DIR)/../../../system/include \
			-I$(CURR_DIR)/src/bl_vendor \
			-I$(CURR_DIR)/src/ipc_client 

SNMP_MIB_VENDOR_MODULES :=	../../bl_vendor/vendor \
							../../bl_vendor/ipc_client \
							../../bl_vendor/util

CONFIGURE_ARGS := \
	--prefix=/var/net-snmp \
	--host=mips-linux-uclibc \
	--build=x86_64-linux-gnu \
	--with-cc=mips-linux-uclibc-gcc-4.8.5 \
	--with-ar=mips-linux-uclibc-gcc-ar \
	--with-default-snmp-version="2" \
	--with-sys-contact="xxx@xxx.com" \
	--with-sys-location="location" \
	--with-logfile="$(SNMP_LOG_DIR)" \
	--with-persistent-directory="/var/net-snmp" \
	--without-opaque-special-types \
	--without-rpm \
	--without-perl-modules \
	--disable-manuals \
	--disable-ucd-snmp-compatibility \
	--disable-embedded-perl \
	--disable-snmptrapd-subagent \
	--disable-scripts \
	--enable-mfd-rewrites \
	--enable-shared=no \
	--with-endianness=little \
	--with-mib-modules="$(SNMP_MIB_VENDOR_MODULES)"

build: clean
	@echo "Building net-snmp start"
	cd $(CURR_DIR)/src && ./configure $(CONFIGURE_ARGS)

	make -C $(CURR_DIR)/src CFLAGS="$(CFLAGS)"

	@mkdir -p $(CURR_DIR)/build

	@mkdir -p $(CURR_DIR)/build/bin
	@cp $(CURR_DIR)/src/agent/$(EXEC) $(CURR_DIR)/build/bin/$(EXEC)
	@cp $(CURR_DIR)/conf/snmpd_start.sh $(CURR_DIR)/build/bin/snmpd_start.sh
	@cp $(CURR_DIR)/conf/snmpd_stop.sh $(CURR_DIR)/build/bin/snmpd_stop.sh
	@cp $(CURR_DIR)/conf/snmpd_reload.sh $(CURR_DIR)/build/bin/snmpd_reload.sh

	@mkdir -p $(CURR_DIR)/build/conf
	@cp $(CURR_DIR)/conf/snmpd.config $(CURR_DIR)/build/conf/
	@cp $(CURR_DIR)/conf/snmpd.conf $(CURR_DIR)/build/conf/

	@mkdir -p $(CURR_DIR)/build/mibs
	@cp $(CURR_DIR)/src/mibs/*.txt $(CURR_DIR)/build/mibs

romfs:
	$(ROMFSINST) $(CURR_DIR)/build/bin/$(EXEC) /bin/$(EXEC)
	$(ROMFSINST) $(CURR_DIR)/build/bin/snmpd_start.sh /bin/snmpd_start.sh
	$(ROMFSINST) $(CURR_DIR)/build/bin/snmpd_stop.sh /bin/snmpd_stop.sh
	$(ROMFSINST) $(CURR_DIR)/build/bin/snmpd_reload.sh /bin/snmpd_reload.sh
	$(ROMFSINST) $(CURR_DIR)/build/conf/snmpd.config $(ROOTFS_SNMP_UCI)
	$(ROMFSINST) $(CURR_DIR)/build/conf $(ROOTFS_SNMP_CONF)
	$(ROMFSINST) $(CURR_DIR)/build/mibs $(ROOTFS_SNMP_MIBS)

clean:
ifneq ($(wildcard $(SRC_MAKEFILE)),)
	make -C $(CURR_DIR)/src clean
endif
	@rm -rf $(CURR_DIR)/build



