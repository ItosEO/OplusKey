#!/system/bin/sh
rm -rf "/data/misc/user/0"
touch /data/adb/modules/tri_exp_caelifall/remove > /dev/null 2>&1
ps -A -o pid,args | grep tri_exp_caelifall | grep -v grep | awk '{print $1}' | xargs -r kill -9 > /dev/null 2>&1

GET_KEY_CLICK() {
    sleep 0.5
    case "$(getevent -qlc 1 | grep KEY_VOLUME)" in
        *KEY_VOLUMEUP*) echo 0;;
        *KEY_VOLUMEDOWN*) echo 1;;
        *) echo 2;;
    esac
}

echo "-------------------------------------------"
echo " 欢迎使用 Oplus侧键拓展模块 "
echo " 功能：让你的侧边键 / 自定义按键 拥有更多玩法！"
echo "-------------------------------------------"

if [ -f "/proc/tristatekey/tri_state" ]; then
    echo "已为您屏蔽系统原有功能（卸载自动恢复，也可通过action功能调整）"
    echo "✅ 检测到 [三段式侧键]"
    echo "👉 请在模块目录中的 t-stage.sh 中自定义操作"
    chmod 0200 /proc/tristatekey/tri_state 2 >/dev/null
    touch "$MODPATH/disable_tri"
    echo "⚠️ 已屏蔽系统原有功能（卸载自动恢复，也可在 action 功能调整）"
else
    echo "✅ 检测到 [自定义按键]"
    echo "👉 模块支持监听 [单击 / 双击 / 长按]"
    echo "👉 请在模块目录中的 cust.sh 中自定义操作"
    echo "⚠️ 请先到系统设置中将侧键设为 [无操作]"
    echo " "
    echo "请选择要监听的事件："
    echo "  音量 [+] → 单击"
    echo "  音量 [-] → 双击"
    case $(GET_KEY_CLICK) in
        0) echo "✅ 已选择监听 [单击]"
           echo "👉 如需切换双击，请在模块目录创建 double_click 文件";;
        1) touch "$MODPATH/double_click"
           echo "✅ 已选择监听 [双击]"
           echo "👉 如需切换单击，请删除模块目录中的 double_click 文件";;
    esac
fi
echo "-------------------------------------------"
echo "-------------------------------------------"
echo " 我们为您准备了一些预设功能，可以复制到 cust.sh 或 t-stage.sh 使用："
echo " - 手电筒（只支持骁龙设备）"
echo " - 付款码"
echo " - 扫一扫"
echo " - NFC"
echo " - 蓝牙"
echo " - 录音"
echo " - 录屏"
echo " - Clash 服务"
echo " "
echo " "
echo "-------------------------------------------"
echo "✅ 安装完成！您可以在 cust.sh / t-stage.sh 中定制功能"
echo "👉 更多玩法欢迎自行扩展"
echo ""
echo "感谢使用本模块！"
echo "作者：ItosEO & YangFengTuoZi"
echo "-------------------------------------------"
set_perm_recursive $MODPATH 0 0 0755 0755