#!/system/bin/sh
LOG_FILE="/data/misc/user/0/bind.log"
STATE_FILE="/data/misc/user/0/bind_state.txt"
MOD_PROP="$(dirname "$0")/module.prop"
SHELL_TXT="/data/misc/user/0/shell.txt"

GET_KEY_CLICK() {
    sleep 0.5
    local key_info=$(getevent -qlc 1 | grep KEY_VOLUME)
    case "$key_info" in
        *KEY_VOLUMEUP*) echo 0; return ;;
        *KEY_VOLUMEDOWN*) echo 1; return ;;
    esac
    echo 2
}

CREATE_MENU() {
    local title="$1"
    shift
    local opts="$@"
    local num_opts=$#
    local sel_idx=0
    while true; do
        echo -ne "\033[H\033[J"
        echo -e "$title"
        echo "-音量[+]切换选项  音量[-]确认选择"
        echo ""
        local i=0
        for opt in $opts; do
            if [ "$sel_idx" -eq "$i" ]; then
                echo "   ➤ $opt"
            else
                echo "    $opt"
            fi
            i=$((i + 1))
        done
        case $(GET_KEY_CLICK) in
            0) sel_idx=$(((sel_idx + 1) % num_opts));;
            1) return $sel_idx;;
        esac
    done
}

KILL_PROCESS() {
    local exp_file="$(dirname "$0")/exp.sh"
    if ps -ef | grep -Fw "sh $exp_file" | grep -v grep >/dev/null; then
        pkill -KILL -f "sh $exp_file"
        sleep 2
    fi
}

DISABLE_MODULE() {
    KILL_PROCESS
    echo "up=未绑定" > "$STATE_FILE"
    echo "down=未绑定" >> "$STATE_FILE"
}

FEATURES="
vx付款码|#vx付款码|
zfb付款码|#zfb付款码|
手电筒|#手电筒|关手电筒
蓝牙|#蓝牙|#关蓝牙
一键录音|#录音|#关录音
vx扫码|#vx扫码|
zfb扫码|#zfb扫码|
NFC|#nfc|#关nfc
一键录屏|#录屏|
一键录像|#录像|#关录像
clash控制|#clash|#关clash
"

WRITE_LOG() {
    local type="$1"
    local feature="$2"
    local datetime=$(date "+%Y-%m-%d %H:%M:%S")
    echo "[$datetime] 绑定 ${type}键 -> ${feature}" >> "$LOG_FILE"
    tail -n 100 "$LOG_FILE" > "${LOG_FILE}.tmp" && mv "${LOG_FILE}.tmp" "$LOG_FILE"
}

UPDATE_DESCRIPTION() {
    local up_mode="未绑定"
    local down_mode="未绑定"
    if [ -f "$STATE_FILE" ]; then
        up_mode=$(grep "^up=" "$STATE_FILE" | cut -d= -f2)
        down_mode=$(grep "^down=" "$STATE_FILE" | cut -d= -f2)
    fi
    if [ -z "$up_mode" ]; then up_mode="未绑定"; fi
    if [ -z "$down_mode" ]; then down_mode="未绑定"; fi
    if [ -f "$MOD_PROP" ]; then
        sed -i "/^description=/c\description=当前拓展 | 上键:$up_mode | 下键:$down_mode" "$MOD_PROP"
    fi
}

RUN_AND_BIND() {
    local type="$1"
    local feature="$2"
    local up_file="/data/misc/user/0/${type}.sh"
    local re_file="/data/misc/user/0/re${type}.sh"

    KILL_PROCESS
    sh "$(dirname "$0")/shell.sh" >/dev/null 2>&1

    echo "$FEATURES" | while IFS="|" read -r name key_on key_off; do
        [ -z "$name" ] && continue
        if [ "$feature" = "$name" ]; then
            local on_line=$(grep -A1 "$key_on" "$SHELL_TXT" | tail -n1)
            echo "$on_line" > "$up_file"
            if [ -n "$key_off" ]; then
                local off_line=$(grep -A1 "$key_off" "$SHELL_TXT" | tail -n1)
                echo "$off_line" > "$re_file"
            else
                rm -f "$re_file"
            fi
        fi
    done

    rm -f "$SHELL_TXT"
    WRITE_LOG "$type" "$feature"
    
    [ ! -f "$STATE_FILE" ] && touch "$STATE_FILE"
    sed -i "/^$type=/d" "$STATE_FILE"
    echo "$type=$feature" >> "$STATE_FILE"

    nohup sh "$(dirname "$0")/exp.sh" >/dev/null 2>&1 &
    echo "您已选择 $feature"
    
    UPDATE_DESCRIPTION
    exit 0
}

IS_MODULE_RUNNING() {
    if ps -ef | grep -Fw "$(dirname "$0")/exp.sh" | grep -v grep >/dev/null; then
        echo "运行中"
        return 0
    else
        echo "未运行"
        return 1
    fi
}

DISPLAY_SUBMENU() {
    local type="$1"
    local names=$(echo "$FEATURES" | while IFS="|" read -r name key_on key_off; do
        [ -n "$name" ] && echo "$name"
    done)
    names="$names 返回上级 退出"

    while true; do
        local title="将${type}键绑定什么功能？[当前免费版可用前五项]"
        CREATE_MENU "$title" $names
        local choice=$?
        local i=0
        for opt in $names; do
            if [ "$i" -eq "$choice" ]; then
                if [ "$opt" = "返回上级" ]; then
                    return
                elif [ "$opt" = "退出" ]; then
                    exit 0
                else
                    local feature_name="$opt"
                    case "$feature_name" in
                        "vx扫码"|"zfb扫码"|"一键录屏"|"一键录像"|"clash控制"|"NFC")
                            local description=""
                            case "$feature_name" in
                                "vx扫码") description="快速启动微信扫一扫功能";;
                                "zfb扫码") description="快速启动支付宝扫一扫功能";;
                                "一键录屏") description="快速启动系统屏幕录制";;
                                "一键录像") description="快速启动相机录像功能";;
                                "clash控制") description="快速控制clash服务的启停";;
                                "NFC") description="快速开关NFC功能";;
                            esac
                            
                            echo -ne "\033[H\033[J"
                            echo "功能名称: ${feature_name}"
                            echo "功能描述:${description}"
                            echo
                            echo "此功能公共版不开放 需捐赠获取"
                            echo "赞助5R或以上可获取捐赠版 包维护更新功能"
                            echo "赞助码位于/data/adb/modules/tri_exp_caelifall下"
                            echo "捐赠后联系qq3423852590或qq1587732944"
                            echo "或加入群957330458通过临时会话私聊"
                            echo "或备用卫星号 Caelifall"
                            echo "捐赠后请加作者qq好友！"
                            echo "按音量下键退出…"
                            
                            while true; do
                                [ $(GET_KEY_CLICK) -eq 1 ] && break
                            done
                            ;;
                        *)
                            RUN_AND_BIND "$type" "$opt"
                            ;;
                    esac
                fi
            fi
            i=$((i + 1))
        done
    done
}

MAIN_LOOP() {
    UPDATE_DESCRIPTION
    while true; do
        local up_mode="未绑定"
        local down_mode="未绑定"
        local module_status="未运行"
        
        if [ -f "$STATE_FILE" ]; then
            up_mode=$(grep "^up=" "$STATE_FILE" | cut -d= -f2)
            down_mode=$(grep "^down=" "$STATE_FILE" | cut -d= -f2)
        fi
        if [ -z "$up_mode" ]; then up_mode="未绑定"; fi
        if [ -z "$down_mode" ]; then down_mode="未绑定"; fi

        IS_MODULE_RUNNING
        if [ $? -eq 0 ]; then
            module_status="运行中"
        fi

        local title="请选择要绑定的键位 中键为初始键\n当前绑定：[上键: $up_mode | 下键: $down_mode]\n当前模块状态: $module_status"
        CREATE_MENU "$title" "上键拓展" "下键拓展" "停用模块" "退出"
        case $? in
            0) DISPLAY_SUBMENU "up";;
            1) DISPLAY_SUBMENU "down";;
            2) DISABLE_MODULE; UPDATE_DESCRIPTION; exit 0;;
            3) exit 0;;
        esac
    done
}

MAIN_LOOP
