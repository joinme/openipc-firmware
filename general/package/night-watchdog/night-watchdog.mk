################################################################################
#
# night-watchdog
#
################################################################################

NIGHT_WATCHDOG_VERSION = 1.0
NIGHT_WATCHDOG_SITE = $(BR2_EXTERNAL)/general/package/night-watchdog/src
NIGHT_WATCHDOG_SITE_METHOD = local
NIGHT_WATCHDOG_LICENSE = GPL-2.0+

# 这是一个纯脚本包，没有编译过程，直接定义安装命令
define NIGHT_WATCHDOG_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0755 $(@D)/night_watchdog.sh $(TARGET_DIR)/usr/bin/night_watchdog.sh
endef

$(eval $(generic-package))
