#!/system/bin/sh
SCRIPT_DIR="/data/misc/user/0"
LAST_STATE_FILE="/data/local/tmp/state"
STATE_FILE="/proc/tristatekey/tri_state"
DEVICE="FFF"

handle_event() {
   
    if read current_state < "$STATE_FILE"; then
        if [ -f "$LAST_STATE_FILE" ]; then
            read last_state < "$LAST_STATE_FILE"
        else
            last_state=""
        fi

        if [ -n "$current_state" ] && [ "$current_state" != "$last_state" ]; then
            case "${last_state}:${current_state}" in
                2:1) sh "$SCRIPT_DIR/up.sh" ;;
                1:2) sh "$SCRIPT_DIR/reup.sh" ;;
                2:3) sh "$SCRIPT_DIR/down.sh" ;;
                3:2) sh "$SCRIPT_DIR/redown.sh" ;;
            esac
            echo "$current_state" > "$LAST_STATE_FILE"
        fi
    fi
}

if [ -f "$STATE_FILE" ]; then
    read init_state < "$STATE_FILE"
    echo "$init_state" > "$LAST_STATE_FILE"
else
    echo "" > "$LAST_STATE_FILE"
fi

getevent -lc 1 "$DEVICE" | grep -q "KEY_F3" && handle_event && exec "$0"