#!/system/bin/sh

MOD_PATH=$(dirname "$0")
MOD_PROP="${MOD_PATH}/module.prop"

while [ "$(getprop sys.boot_completed)" != "1" ] || [ ! -d "/data/data/android" ]; do
    sleep 1
done

if ! [ -f "/data/misc/user/0/bind_state.txt" ]; then
sed -i "s/^description=.*/description=点执行▶选择模式 自定义你的三段式侧键功能/" "$MOD_PROP"
else
    nohup sh "$MOD_PATH/exp.sh" >/dev/null 2>&1 &
fi

