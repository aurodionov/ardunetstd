include $(TOPDIR)/rules.mk

PKG_NAME:=ardunetstd
PKG_VERSION:=1
PKG_RELEASE:=1

PKG_BUILD_DIR:= $(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk

define Package/ardunetstd
	SECTION:=utils
	CATEGORY:=Utilities
	TITLE:=ardunetstd - ArduNetstD utility
endef

define Package/ardunetstd/description
    ardunetstd - ArduNetstD utility
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

define Build/Compile
	$(TARGET_CC) $(TARGET_CFLAGS) -c -o $(PKG_BUILD_DIR)/ardunetstd.o $(PKG_BUILD_DIR)/ardunetstd.c
	$(TARGET_CC) $(TARGET_LDFLAGS) -o $(PKG_BUILD_DIR)/ardunetstd $(PKG_BUILD_DIR)/ardunetstd.o
endef

define Package/ardunetstd/install
	$(INSTALL_DIR) $(1)/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/ardunetstd $(1)/
endef

$(eval $(call BuildPackage,ardunetstd))

################################
###########openwrt##############
