#
# broadlink
#
# BL Property Software.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=produce
PKG_RELEASE:=1

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)
PKG_CONFIG_DEPENDS:=

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/kernel.mk

define Package/$(PKG_NAME)
  SECTION:=utils
  CATEGORY:=Utilities
  TITLE:=$(PKG_NAME) set Command
  DEPENDS:=+libpthread +librt
endef

define Package/$(PKG_NAME)/description
  An program to excute command.
endef

define Build/Prepare
	@echo  "Build/Prepare >>>>>"
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

define Build/Configure
	@echo  "Build/Configure >>>>>"
endef

define Build/Clean
	@echo  "Build/Clean >>>>>"
endef
ifneq ($(strip $(CONFIG_PACKAGE_mobile)),)
TARGET_CFLAGS += -DBUILD_SUPPORT_MOBILE
endif

ifneq ($(strip $(CONFIG_CUS_PARAMS_SOFTVERSION)),)
CUS_PARAMS_SOFTVERSION=$(CONFIG_CUS_PARAMS_SOFTVERSION)
export CUS_PARAMS_SOFTVERSION
CUSTOMER_PARAMS_CFLAGS += -DCUS_PARAMS_SOFTVERSION=\\\"${CUS_PARAMS_SOFTVERSION}\\\"
endif

ifneq ($(strip $(CONFIG_CUS_PARAMS_HARDVERSION)),)
CUS_PARAMS_HARDVERSION=$(CONFIG_CUS_PARAMS_HARDVERSION)
export CUS_PARAMS_HARDVERSION
CUSTOMER_PARAMS_CFLAGS += -DCUS_PARAMS_HARDVERSION=\\\"${CUS_PARAMS_HARDVERSION}\\\"
endif

TARGET_CFLAGS += -DME_GOAHEAD_REALM=\\\"broadlink.com\\\"

define Build/Compile
	@echo  "Build/Compile >>>>>"
	@echo "+++++++++ MAKE=$(MAKE)"
	@echo "+++++++++ PKG_BUILD_DIR=$(PKG_BUILD_DIR)"
	@echo "+++++++++ TARGET_CC=$(TARGET_CC)"
	@echo "+++++++++ TARGET_CFLAGS=$(TARGET_CFLAGS)"
	@echo "+++++++++ CUSTOMER_PARAMS_CFLAGS=$(CUSTOMER_PARAMS_CFLAGS)"
	@echo "+++++++++ TARGET_LDFLAGS=$(TARGET_LDFLAGS) ++"
	$(MAKE) -C $(PKG_BUILD_DIR) \
		CC="$(TARGET_CC)" \
		CFLAGS="$(TARGET_CFLAGS) $(CUSTOMER_PARAMS_CFLAGS) -Wall" \
		LDFLAGS="$(TARGET_LDFLAGS)"
endef

define Package/$(PKG_NAME)/install
	@echo  "Package/install >>>>>"
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_DIR) $(1)/sbin
	$(INSTALL_DIR) $(1)/etc
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/$(PKG_NAME) $(1)/usr/bin
	$(INSTALL_BIN) ./files/bl_boot $(1)/sbin
	$(INSTALL_BIN) ./files/bl_reset $(1)/sbin
	$(INSTALL_BIN) ./files/bl_wifi_brlan $(1)/sbin
	$(INSTALL_BIN) ./files/bl_wps $(1)/sbin
	$(INSTALL_BIN) ./files/bl_notify_telnet $(1)/sbin
	$(INSTALL_BIN) ./files/bl_login $(1)/sbin
	$(INSTALL_BIN) ./files/bl_sync_account $(1)/sbin
	$(INSTALL_BIN) ./files/check_network $(1)/sbin
	cp -rf $(PKG_BUILD_DIR)/ccdver $(1)/etc
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
