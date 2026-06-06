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
    # 用 sed 修改
	sed -i '
	  # ===== OSD 段 =====
	  /^osd:/,/^[a-z]/ {
	      # 启用 OSD
	      s/^\([[:space:]]*\)enabled:[[:space:]]*false/\1enabled: true/
	      
	      # 修改时间格式模板
	      s/^\([[:space:]]*\)template:[[:space:]]*'"'"'%d.%m.%Y %H:%M:%S'"'"'$/\1template: '"'"'%Y.%m.%d %A %H:%M:%S'"'"'/
	      
	      # 添加 size: 2 字段（如果不存在）
	      /^\([[:space:]]*\)posY:[[:space:]]*16$/a\ \ \ size: 2
	  }
	
	  # ===== Audio 段 =====
	  /^audio:/,/^[a-z]/ {
	      # 启用音频
	      s/^\([[:space:]]*\)enabled:[[:space:]]*false/\1enabled: true/
	      
	      # 在 speakerPin: 15 之后插入 speakerPinInvert: false
	      /^[[:space:]]*speakerPin:[[:space:]]*15$/a\  speakerPinInvert: false
	  }
	' /etc/majestic.yaml
fi
