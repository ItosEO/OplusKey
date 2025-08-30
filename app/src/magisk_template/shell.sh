#!/system/bin/sh
FILE_PATH="/data/misc/user/0/shell.txt"
DIR_PATH=$(dirname "$FILE_PATH")

if [ ! -d "$DIR_PATH" ]; then
  mkdir -p "$DIR_PATH"
fi

cat > "$FILE_PATH" << EOF
#vx付款码
am start -n com.tencent.mm/.plugin.offline.ui.WalletOfflineCoinPurseUI

#zfb付款码
am start -a android.intent.action.VIEW -d "alipays://platformapi/startapp?appId=20000056"

#手电筒
echo 0 > /sys/class/leds/led:torch_0/brightness; echo 0 > /sys/class/leds/led:switch_0/brightness; echo 500 > /sys/class/leds/led:torch_0/brightness; echo 1 > /sys/class/leds/led:switch_0/brightness

#关手电筒
echo 0 > /sys/class/leds/led:torch_0/brightness; echo 0 > /sys/class/leds/led:switch_0/brightness

#蓝牙
svc bluetooth enable

#关蓝牙
svc bluetooth disable

#录音
am start --user 0 -n com.coloros.soundrecorder/.TransparentActivity >/dev/null 2>&1

#关录音
am force-stop com.coloros.soundrecorder
EOF
