#!/usr/bin/env sh

if [[ $# -ne 1 ]]; then
    echo "用法: $0 <日志文件路径>"
    exit 1
fi

LOG_FILE="$1"

if [[ ! -f "$LOG_FILE" ]]; then
    echo "文件不存在: $LOG_FILE"
    exit 1
fi

parse_human_size() {
    size_str="$1"
    unit="${size_str: -1}"
    number="${size_str%[KMGkmg]}"

    case "$unit" in
        K|k) awk "BEGIN {print $number * 1024}" ;;
        M|m) awk "BEGIN {print $number * 1024 * 1024}" ;;
        G|g) awk "BEGIN {print $number * 1024 * 1024 * 1024}" ;;
        *) echo "$number" ;;
    esac
}

start_time=$(date +%s)

start_size_str=$(ls -lh "$LOG_FILE" | awk '{print $5}')
start_size=$(parse_human_size "$start_size_str")
last_size=$start_size
total_bytes=0
write_seconds=0
has_shown_no_change=0

echo "监测文件: $LOG_FILE"
echo "本脚本未做精细检测，因此实际写入大小可能存在些许波动"
echo "按 Ctrl+C 停止监测并显示平均写入速度..."

trap 'on_exit' INT

on_exit() {
    end_time=$(date +%s)
    duration=$((end_time - start_time))
    ((duration == 0)) && duration=1
    ((write_seconds == 0)) && write_seconds=1

    avg_speed_total=$(awk "BEGIN{printf \"%.2f\", $total_bytes / $duration / 1024 / 1024}")
    avg_speed_effective=$(awk "BEGIN{printf \"%.2f\", $total_bytes / $write_seconds / 1024 / 1024}")
    total_MB=$(awk "BEGIN{printf \"%.2f\", $total_bytes / 1024 / 1024}")

    echo ""
    echo "----------- 总结 -----------"
    echo "持续时间：$duration 秒"
    echo "有效写入时间：$write_seconds 秒"
    echo "写入总量：$total_MB MB"
    echo "绝对平均速率：$avg_speed_total MB/s"
    echo "有效平均速率：$avg_speed_effective MB/s"
    echo "----------------------------"
    exit 0
}

while true; do
    sleep 1
    size_str=$(ls -lh "$LOG_FILE" | awk '{print $5}')
    current_size=$(parse_human_size "$size_str")
    delta=$(awk "BEGIN {print $current_size - $last_size}")

    if (( $(echo "$delta > 0" | bc -l) )); then
        total_bytes=$(awk "BEGIN {print $total_bytes + $delta}")
        last_size=$current_size
        has_shown_no_change=0
        ((write_seconds++))

        speed_MB=$(awk "BEGIN{printf \"%.2f\", $delta / 1024 / 1024}")
        printf "%s | 当前大小: %s | 速度: %.2f MB/s\n" "$(date '+%H:%M:%S')" "$size_str" "$speed_MB"
    else
        if [[ $has_shown_no_change -eq 0 ]]; then
            echo "$(date '+%H:%M:%S') | 文件无变化，等待写入中..."
            has_shown_no_change=1
        fi
    fi
done
