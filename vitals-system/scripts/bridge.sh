#!/usr/bin/env bash
#
# bridge.sh - run the UDP->serial vitals bridge (bridge.py) as a background
# process: start / stop / status.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BRIDGE="$(cd "$SCRIPT_DIR/../bridge" && pwd)/bridge.py"

RUN_DIR="${TMPDIR:-/tmp}"
RUN_DIR="${RUN_DIR%/}"
PID_FILE="$RUN_DIR/vitals-bridge.pid"
LOG_FILE="$RUN_DIR/vitals-bridge.log"

usage() {
	cat <<EOF
bridge.sh - run the UDP->serial vitals bridge (bridge.py)

Usage:
  ./bridge.sh start [bridge.py args]   Start the bridge, backgrounded
  ./bridge.sh stop                     Stop the backgrounded bridge
  ./bridge.sh status                   Show whether it's running, and on what ports
  ./bridge.sh debug [bridge.py args]   Run in this terminal with -v logging (Ctrl-C to stop)
  ./bridge.sh test [serial_port]       Drive the board directly (no bridge) to check the sketch upload
  ./bridge.sh --help|--usage           Show this message

Anything after 'start' / 'debug' is passed straight to bridge.py, e.g.
  ./bridge.sh start --port /dev/cu.usbmodem2101 --udp 9999

'test' talks straight to the board over serial, so run it with the bridge
stopped - e.g. right after uploading the sketch. Defaults to the first
/dev/cu.usbmodem*; pass a device to override.

Files:
  pid  $PID_FILE
  log  $LOG_FILE
EOF
}

# Print the PID if the bridge is running; return non-zero otherwise.
running_pid() {
	[ -f "$PID_FILE" ] || return 1
	local pid
	pid="$(cat "$PID_FILE" 2>/dev/null || true)"
	[ -n "$pid" ] || return 1
	kill -0 "$pid" 2>/dev/null || return 1
	# guard against PID reuse - confirm it's actually our bridge
	ps -p "$pid" -o command= 2>/dev/null | grep -q "bridge/bridge.py" || return 1
	echo "$pid"
}

cmd_start() {
	local pid
	if pid="$(running_pid)"; then
		echo "bridge already running (PID $pid)"
		return 0
	fi

	nohup python3 "$BRIDGE" "$@" >"$LOG_FILE" 2>&1 &
	local new=$!
	echo "$new" >"$PID_FILE"
	sleep 0.6

	if ! kill -0 "$new" 2>/dev/null; then
		echo "bridge failed to start - last log lines:" >&2
		tail -n 15 "$LOG_FILE" >&2 || true
		rm -f "$PID_FILE"
		return 1
	fi

	echo "bridge started (PID $new)"
	grep -m1 "listening on" "$LOG_FILE" 2>/dev/null | sed 's/^/  /' || true
	echo "  log: $LOG_FILE"
}

cmd_stop() {
	local pid
	if ! pid="$(running_pid)"; then
		echo "bridge not running"
		rm -f "$PID_FILE"
		return 0
	fi

	kill -INT "$pid" 2>/dev/null || true
	for _ in $(seq 1 10); do
		kill -0 "$pid" 2>/dev/null || break
		sleep 0.3
	done
	if kill -0 "$pid" 2>/dev/null; then
		kill -KILL "$pid" 2>/dev/null || true
		sleep 0.3
	fi

	rm -f "$PID_FILE"
	echo "bridge stopped (was PID $pid)"
}

cmd_status() {
	local pid
	if ! pid="$(running_pid)"; then
		echo "bridge: stopped"
		return 0
	fi
	echo "bridge: running (PID $pid)"

	local udp last
	udp="$(grep "listening on" "$LOG_FILE" 2>/dev/null | tail -n1 | sed -E 's/.*listening on //' || true)"
	[ -n "${udp:-}" ] && echo "  udp:    $udp"

	# the most recent serial event says whether the link is currently up
	last="$(grep -E "opened .* @ |serial dropped|write failed|open failed" "$LOG_FILE" 2>/dev/null | tail -n1 || true)"
	case "$last" in
		*"opened "*)
			echo "  serial: $(printf '%s\n' "$last" | sed -E 's/.*opened ([^ ]+) @ .*/\1/')"
			;;
		*dropped* | *"write failed"* | *"open failed"*)
			echo "  serial: disconnected (bridge retrying)"
			;;
		*)
			echo "  serial: not connected yet"
			;;
	esac
	echo "  log:    $LOG_FILE"
}

cmd_debug() {
	local pid
	if pid="$(running_pid)"; then
		echo "a backgrounded bridge is running (PID $pid) - './bridge.sh stop' first" >&2
		exit 1
	fi
	echo "running bridge.py -v in this terminal (Ctrl-C to stop)"
	exec python3 "$BRIDGE" -v "$@"
}

cmd_test() {
	local pid
	if pid="$(running_pid)"; then
		echo "bridge is running (PID $pid) - stop it first: ./bridge.sh stop" >&2
		echo "('test' writes to the board directly and needs the serial port free)" >&2
		exit 1
	fi

	echo "testing the board directly over serial (no bridge) ..."
	python3 - "${1:-}" <<'PY'
import glob, sys, time

try:
    import serial  # pyserial
except ImportError:
    sys.exit("pyserial not installed - run: pip3 install pyserial")

port = sys.argv[1] if len(sys.argv) > 1 and sys.argv[1] else None
if not port:
    hits = sorted(glob.glob("/dev/cu.usbmodem*")) or sorted(glob.glob("/dev/tty.usbmodem*"))
    if not hits:
        sys.exit("no /dev/cu.usbmodem* found - is the board plugged in?")
    port = hits[0]

print(f"  opening {port} @ 115200, waiting 2s for board reset")
try:
    ser = serial.Serial(port, 115200, timeout=0)
except (OSError, serial.SerialException) as e:
    sys.exit(f"could not open {port}: {e}")
time.sleep(2.0)

def send(tag, pct):
    ser.write(f"{tag} {pct}\n".encode())

# two mirrored 0->100->0 sweeps of HP (red, top) and Prayer (blue, bottom)
for _ in range(2):
    for n in list(range(0, 101, 10)) + list(range(90, -1, -10)):
        send("HP", n)
        send("PR", 100 - n)
        time.sleep(0.12)

send("HP", 75)
send("PR", 40)
ser.flush()
time.sleep(2.0)          # hold a stable frame so you can eyeball it

send("HP", 0)            # leave the board dark - test cleans up after itself
send("PR", 0)
ser.flush()
time.sleep(0.3)          # let the bytes physically leave before closing
ser.close()
print("  done - swept, briefly held HP 75% / PR 40%, then blanked the board")
PY
}

case "${1:-}" in
	start)
		shift
		cmd_start "$@"
		;;
	stop)
		cmd_stop
		;;
	status)
		cmd_status
		;;
	debug)
		shift
		cmd_debug "$@"
		;;
	test)
		shift
		cmd_test "$@"
		;;
	"" | -h | --help | --usage | help)
		usage
		;;
	*)
		echo "bridge.sh: unknown option '$1'" >&2
		echo >&2
		usage >&2
		exit 1
		;;
esac
