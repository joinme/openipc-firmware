#!/bin/sh

# ===== 阈值参数（请根据日志输出的真实数据修改这里！）=====
MAX_GAIN=8192      # 高于此值视为夜
MIN_GAIN=2048      # 低于此值视为昼
INTERVAL=5        # 轮询间隔（秒）

#TMP_METRICS="/tmp/.metrics_cache"

while true; do
    # 1. 使用 wget 获取标准 metrics 接口 (wget 在 OpenIPC 里最稳定)
    #wget -q -O "$TMP_METRICS" "http://127.0.0.1/metrics" 2>/dev/null

    # 2. 解析当前 ISP 模拟增宊 和 当前夜视状态 (0白天, 1黑夜)
    #GAIN=$(awk '/^isp_again/{print $2}' "$TMP_METRICS")
    #STATE=$(awk '/^night_enabled/{print $2}' "$TMP_METRICS")

    # 一次性获取两个指标，省请求省磁盘
    METRICS=$(wget -q -O - "http://127.0.0.1/metrics" 2>/dev/null)
    GAIN=$(echo "$METRICS" | awk '/^isp_again/{print $2; exit}')
    STATE=$(echo "$METRICS" | awk '/^night_enabled/{print $2; exit}')

    # 3. 数据有效性检查（如果 majestic 没起起来，这里会是空）
    if [ -z "$GAIN" ] || [ -z "$STATE" ]; then
        echo "[Watchdog] Waiting for Majestic ISP data..."
        sleep "$INTERVAL"
        continue
    fi

    # 4. 核心判断逻辑（带日志输出，方便你观察）
    # 如果当前是白天(0)，且增宊过高 -> 切到夜
    if [ "$STATE" = "0" ] && [ "$GAIN" -gt "$MAX_GAIN" ]; then
        echo "[Watchdog] Gain=$GAIN > $MAX_GAIN. Switching to NIGHT."
        wget -q -O - "http://127.0.0.1/night/on" >/dev/null 2>&1

    # 如果当前是黑夜(1)，且增宊过低 -> 切到昼
    elif [ "$STATE" = "1" ] && [ "$GAIN" -lt "$MIN_GAIN" ]; then
        echo "[Watchdog] Gain=$GAIN < $MIN_GAIN. Switching to DAY."
        wget -q -O - "http://127.0.0.1/night/off" >/dev/null 2>&1
    fi

    sleep "$INTERVAL"
done
