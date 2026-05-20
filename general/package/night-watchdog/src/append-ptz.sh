#!/bin/sh

PTZ_CONFIG=$(cat << 'YAML'

ptz:
  enabled: true
  driver: script
  command: "/usr/bin/gpio-motors"
YAML
)

# 检查 majestic.yaml 是否存在
if [ -f /etc/majestic.yaml ]; then
    # 如果文件中不包含 "ptz:" 字样，则追加配置
    if ! grep -q "^ptz:" /etc/majestic.yaml; then
        echo "$PTZ_CONFIG" >> /etc/majestic.yaml
    fi
fi
