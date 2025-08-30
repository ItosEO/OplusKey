#!/system/bin/sh
last_state=$(cat /proc/tristatekey/tri_state)
script_dir="/data/misc/user/0/"

while true
do
    sleep 1
    current_state=$(cat /proc/tristatekey/tri_state)

    if [ "$current_state" -ne "$last_state" ]; then
        case "$last_state:$current_state" in
            2:1)
                nohup sh "$script_dir/up.sh" &>/dev/null &
                ;;
            1:2)
                nohup sh "$script_dir/reup.sh" &>/dev/null &
                ;;
            2:3)
                nohup sh "$script_dir/down.sh" &>/dev/null &
                ;;
            3:2)
                nohup sh "$script_dir/redown.sh" &>/dev/null &
                ;;
        esac
        last_state=$current_state
    fi
done
