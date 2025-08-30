#!/system/bin/sh

if ! [ -f "/proc/tristatekey/tri_state" ]; then
    echo "你的手机不支持此模块！"
    exit 1
fi

rm -rf "/data/misc/user/0"
echo "-------------------------------------------" && sleep 0.1
echo "拓展你的一加三段式侧键功能！"
echo "手电筒 付款码 扫一扫 nfc 蓝牙 录音 录屏 录像 clash"
echo "[更多功能更新中 请关注]"
echo " "
echo " "
echo "安装完成"
echo " "
echo "点个关注吗?"
echo "音量[+]现在去   音量[-]点过了"

local key_click=""
while [ -z "$key_click" ]; do
    key_click=$(getevent -qlc 1 | awk '/KEY_VOLUMEUP|KEY_VOLUMEDOWN/ {print $3}')
    sleep 0.5
done

if [ "$key_click" = "KEY_VOLUMEUP" ]; then
    am start -a android.intent.action.VIEW -d "http://www.coolapk.com/u/24621888"
fi

echo " "

local Model=$(getprop ro.vendor.oplus.market.name)
sed -i "s/^description=.*/description=[未启用]请重启设备… $Model $(date +"%H:%M:%S")/" "$MODPATH/module.prop"