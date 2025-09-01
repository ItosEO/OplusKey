#!/system/bin/sh

MODE=${1:-none}

stageup() {
    # 在这里添加三段式最上面的操作
}

stagecenter() {
    # 在这里添加三段式中间的操作
}

stagedown() {
    # 在这里添加三段式最下面的操作
}




# 判断模式
if [ "$MODE" = "up" ]; then
    stageup
elif [ "$MODE" = "center" ]; then
    stagecenter
elif [ "$MODE" = "down" ]; then
    stagedown
else
    echo "[Error] 参数错误"
fi
