#!/bin/bash
#
# Advanced IRC Flood / Stress Test for ft_irc
# Simulates multiple clients, fragmented packets, slowloris, and message floods.
#

PASS="123"
PORT="8989"
HOST="127.0.0.1"

CLIENTS=50         # number of fake clients
MSGS_PER_CLIENT=500  # messages per client
SLOWLORIS_DELAY=0.2 # seconds between bytes
FRAG_PROB=20       # % chance to split packets randomly
GARBAGE_PROB=10    # % chance to send garbage
FLOOD_DELAY=0.0005 # delay between PRIVMSG bursts

#########################################
# Random helpers
#########################################

rand() { shuf -i 0-99 -n 1; }
garbage() {
    head /dev/urandom | tr -dc 'A-Za-z0-9!@#$%^&*()' | head -c 16
}

#########################################
# Send possibly fragmented packet
#########################################
send_frag() {
    local text="$1"
    local len=${#text}

    if (( $(rand) < FRAG_PROB )); then
        # send random fragment
        split=$((RANDOM % (len - 1) + 1))
        printf "%s" "${text:0:$split}"
        sleep 0.01
        printf "%s" "${text:$split}"
    else
        printf "%s" "$text"
    fi
}

#########################################
# One client connection
#########################################
run_client() {
    local id="$1"

    (
        # Registration (possibly fragmented)
        send_frag "PASS $PASS\r\n"
        send_frag "NICK client-$id\r\n"
        send_frag "USER client-$id 0 * :client-$id\r\n"
        send_frag "JOIN #test\r\n"

        # optional slowloris mode: send 1 byte at a time
        if (( $(rand) < 10 )); then   # 10% of clients do slowloris
            for b in PASS "123" "PING" "L" "O" "L"; do
                printf "%s" "$b"
                sleep "$SLOWLORIS_DELAY"
            done
            printf "\r\n"
        fi

        # flood messages
        for ((m=1; m<=MSGS_PER_CLIENT; m++)); do
            # maybe send garbage
            if (( $(rand) < GARBAGE_PROB )); then
                send_frag "$(garbage)\r\n"
            else
                send_frag "PRIVMSG #test :hello from $id [$m]\r\n"
            fi
            sleep "$FLOOD_DELAY"
        done
    ) | nc -C "$HOST" "$PORT" >/dev/null 2>&1 &
}

#########################################
# Main: spawn clients
#########################################
echo "[*] Launching $CLIENTS clients on $HOST:$PORT"
echo "[*] Each sends $MSGS_PER_CLIENT messages"
echo "[*] Flood delay: $FLOOD_DELAY sec"
echo "[*] Slowloris delay: $SLOWLORIS_DELAY sec"
echo

for ((i=1; i<=CLIENTS; i++)); do
    run_client "$i"
    sleep 0.01   # avoid too many connections at once
done

echo "[*] All clients launched. Monitoring..."
wait
echo "[*] Stress test complete."
