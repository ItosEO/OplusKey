#!/system/bin/sh
MODDIR=${0%/*}

# 持久化设备类型
if [ -f "/proc/tristatekey/tri_state" ]; then
    DEVICE_TYPE="tri"    # 三段式按键
else
    DEVICE_TYPE="custom" # 自定义按键
fi

get_key_click() {
    sleep 0.5
    local key_info=$(getevent -qlc 1 | grep KEY_VOLUME)
    case "$key_info" in
        *KEY_VOLUMEUP*) echo 0; return ;;
        *KEY_VOLUMEDOWN*) echo 1; return ;;
    esac
    echo 2
}

toggle_tristate() {
    local tri_file="/proc/tristatekey/tri_state"
    if [ ! -e "$tri_file" ]; then
        echo "[提示] 三段式按键文件不存在"
        return 1
    fi
    local perm=$(stat -c %a "$tri_file" 2>/dev/null || echo "")
    if [ "$perm" = "200" ]; then
        chmod 0644 "$tri_file" 2>/dev/null
        echo "[三段式] 当前状态: 已解除屏蔽"
    else
        chmod 0200 "$tri_file" 2>/dev/null
        echo "[三段式] 当前状态: 已设置屏蔽"
    fi
}

toggle_custom_click() {
    local flag_file="${MODDIR}/double_click"
    if [ -f "$flag_file" ]; then
        rm -f "$flag_file" 2>/dev/null
        echo "[自定义按键] 当前模式: 单击监听"
    else
        touch "$flag_file" 2>/dev/null
        echo "[自定义按键] 当前模式: 双击监听"
    fi
}

show_prompt() {
    if [ "$DEVICE_TYPE" = "tri" ]; then
        local perm=$(stat -c %a /proc/tristatekey/tri_state 2>/dev/null || echo "")
        local status="未知"
        [ "$perm" = "200" ] && status="已屏蔽" || status="未屏蔽"
        echo "检测到三段式按键设备"
        echo "当前屏蔽状态: $status"
        echo "按音量+切换屏蔽 / 按音量-退出"
    else
        local click_mode="单击"
        [ -f "${MODDIR}/double_click" ] && click_mode="双击"
        echo "自定义按键设备"
        echo "当前模式: $click_mode"
        echo "按音量+切换单/双击 / 按音量-退出"
    fi
}

main() {
    show_prompt
    local key=$(get_key_click)
    if [ "$key" -eq 0 ]; then
        if [ "$DEVICE_TYPE" = "tri" ]; then
            toggle_tristate
        else
            toggle_custom_click
        fi
    elif [ "$key" -eq 1 ]; then
        echo "退出脚本"
        exit 0
    else
        echo "未检测到有效按键，请重试"
    fi
}

main
