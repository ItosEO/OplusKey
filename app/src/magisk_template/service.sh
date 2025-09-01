#!/system/bin/sh

MODDIR=${0%/*}

while [ "$(getprop sys.boot_completed)" != "1" ] ; do
    sleep 3
done

if [ -f "/proc/tristatekey/tri_state" ]; then
    MODE="three-stage"
else
    MODE="cust-action"
fi

if [ -f "$MODDIR/disable_tri" ] && [ "$MODE" = "three-stage" ]; then
    chmod 0200 "/proc/tristatekey/tri_state" 2>/dev/null
fi

nohup "$MODDIR/oplus_key" $MODE > /dev/null 2>&1 &
# nohup "$MODDIR/oplus_key" cust-action > /cache/opluskeysh 2>&1 &
