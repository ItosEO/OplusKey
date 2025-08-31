#!/system/bin/sh
IS_LONG_CLICK=${1:-false}

cust_click_script() {
    # 在这里编写单击或双击时执行的脚本
}

cust_long_script() {
    # 在这里编写长按时执行的脚本
}

if [ "$IS_LONG_CLICK" = "true" ]; then
    echo "long click"
    cust_long_script
else
    echo "click"
    cust_click_script
fi

