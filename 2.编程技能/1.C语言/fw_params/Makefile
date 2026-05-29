#
# zhoumingming@szbroadlink.com
#
# BL Property Software.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=fw_params
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

TARGET_CFLAGS += \
	-I$(LINUX_DIR)/drivers/char -I$(STAGING_DIR)/usr/include -I$(STAGING_DIR)/include \

MAKE_FLAGS += \
	CFLAGS="$(TARGET_CFLAGS)"
	CONFIGURE_VARS += LIBS="-lpthread"

define Build/Configure
endef

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/$(PKG_NAME) $(1)/sbin
	(cd $(1)/sbin && ln -sf $(PKG_NAME) fw_printparams && ln -sf $(PKG_NAME) fw_setparams)
endef


$(eval $(call BuildPackage,$(PKG_NAME)))

