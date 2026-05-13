################################################################################
#
# night-watchdog
#
################################################################################

NIGHT_WATCHDOG_VERSION = 1.0
NIGHT_WATCHDOG_SITE = $(BR2_EXTERNAL)/general/package/night-watchdog/src
NIGHT_WATCHDOG_SITE_METHOD = local
NIGHT_WATCHDOG_LICENSE = GPL-2.0+

define NIGHT_WATCHDOG_INSTALL_TARGET_CMDS
	# 安装脚本到 /usr/bin
	$(INSTALL) -D -m 0755 $(@D)/night_watchdog.sh $(TARGET_DIR)/usr/bin/night_watchdog.sh
	# 安装启动脚本到 /etc/init.d
	$(INSTALL) -D -m 0755 $(@D)/S99night $(TARGET_DIR)/etc/init.d/S99night
endef

$(eval $(generic-package))
