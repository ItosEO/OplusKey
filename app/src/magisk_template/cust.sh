#!/system/bin/sh
IS_LONG_CLICK=${1:-false}
if $IS_LONG_CLICK; then
    # long click
    echo "long click"
else
    # single click or double click
    echo "single click or double click"
fi
