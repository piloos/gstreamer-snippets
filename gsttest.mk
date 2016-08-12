GSTTEST_LICENSE = Barco
GSTTEST_VERSION = HEAD
GSTTEST_SITE = /home/lodco/misc/gsttest/
GSTTEST_SITE_METHOD = local
GSTTEST_INSTALL_STAGING = YES
GSTTEST_INSTALL_TARGET = NO

define GSTTEST_BUILD_CMDS 
  $(MAKE) CC="$(TARGET_CC)" LD="$(TARGET_LD)" -C $(@D) appsink_v4l2
  $(MAKE) CC="$(TARGET_CC)" LD="$(TARGET_LD)" -C $(@D) appsink_localsource
endef

$(eval $(generic-package))
