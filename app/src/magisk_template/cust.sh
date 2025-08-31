#!/system/bin/sh
IS_LONG_CLICK=${1:-false}

function cust_click_script() {
    # 在这里编写单击时执行的脚本
}

function cust_long_script() {
    # 在这里编写长按时执行的脚本
}

if $IS_LONG_CLICK; then
    # long click
    echo "long click"
    cust_long_script
else
    # single click or double click
    echo "single click or double click"
    cust_click_script
fi

