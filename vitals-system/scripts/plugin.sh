#!/usr/bin/env bash
#
# plugin.sh - manage the runelite-vitals RuneLite plugin dev loop.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PLUGIN_DIR="$(cd "$SCRIPT_DIR/../runelite-vitals" && pwd)"

SETTINGS_JSON="/Applications/RuneLite.app/Contents/Resources/settings.json"
CREDENTIALS="$HOME/.runelite/credentials.properties"
INSECURE_FLAG="--insecure-write-credentials"

RUN_DIR="${TMPDIR:-/tmp}"
RUN_DIR="${RUN_DIR%/}"
PID_FILE="$RUN_DIR/vitals-plugin.pid"
LOG_FILE="$RUN_DIR/vitals-plugin.log"
CLIENT_MAIN="net.runelite.client.RuneLite"

usage() {
	cat <<EOF
plugin.sh - manage the runelite-vitals plugin dev loop

Usage:
  ./plugin.sh launch            Start the dev client (./gradlew run) detached
  ./plugin.sh auth status       Show Jagex dev-auth state
  ./plugin.sh auth enable       Add $INSECURE_FLAG to settings.json
  ./plugin.sh auth disable      Remove that flag and delete credentials.properties
  ./plugin.sh --help|--usage    Show this message

'launch' backgrounds './gradlew run' so no build logs hit the terminal and you
can close it once the client window is up. There is no 'stop' - quit the
RuneLite client and the gradle task ends by itself. Run 'auth enable' plus a
Jagex Launcher pass first, or the client opens at the login screen; the window
can take ~30-60s to appear, so check the log if nothing shows.

Files:
  settings.json  $SETTINGS_JSON
  credentials    ~/.runelite/credentials.properties
  launch pid     $PID_FILE
  launch log     $LOG_FILE
EOF
}

auth_usage() {
	cat <<EOF
plugin.sh auth - Jagex dev-auth for the './gradlew run' client

Usage:
  ./plugin.sh auth status       Show whether dev-auth is provisioned
  ./plugin.sh auth enable       Add $INSECURE_FLAG to the RuneLite
                                clientArguments so a Jagex Launcher run
                                writes ~/.runelite/credentials.properties
  ./plugin.sh auth disable      Remove that flag from settings.json and
                                delete ~/.runelite/credentials.properties
EOF
}

auth_status() {
	local flag creds

	if [ ! -f "$SETTINGS_JSON" ]; then
		flag="absent (settings.json missing)"
	elif grep -qF -- "$INSECURE_FLAG" "$SETTINGS_JSON"; then
		flag="present"
	else
		flag="absent"
	fi

	if [ -f "$CREDENTIALS" ]; then
		creds="present"
	else
		creds="absent"
	fi

	echo "auth status"
	printf '  %-28s : %s\n' "$INSECURE_FLAG" "$flag"
	printf '  %-28s : %s\n' "credentials.properties" "$creds"
	echo
	echo "  settings.json  $SETTINGS_JSON"
	echo "  credentials    ~/.runelite/credentials.properties"
}

auth_enable() {
	if [ ! -f "$SETTINGS_JSON" ]; then
		echo "plugin.sh: settings.json not found at $SETTINGS_JSON" >&2
		echo "           is RuneLite installed at /Applications/RuneLite.app?" >&2
		exit 1
	fi

	if grep -qF -- "$INSECURE_FLAG" "$SETTINGS_JSON"; then
		echo "$INSECURE_FLAG already in settings.json - nothing to do"
		return 0
	fi

	local tmp
	tmp="$(mktemp "${SETTINGS_JSON}.XXXXXX")"
	if jq -c --arg f "$INSECURE_FLAG" \
		'.clientArguments |= (if index($f) then . else . + [$f] end)' \
		"$SETTINGS_JSON" >"$tmp"; then
		chmod 644 "$tmp"
		mv "$tmp" "$SETTINGS_JSON"
	else
		rm -f "$tmp"
		echo "plugin.sh: failed to update settings.json" >&2
		exit 1
	fi

	echo "added $INSECURE_FLAG to settings.json"
	echo "  clientArguments: $(jq -c '.clientArguments' "$SETTINGS_JSON")"
	echo
	echo "next: launch RuneLite once via the Jagex Launcher to write ~/.runelite/credentials.properties"
}

auth_disable() {
	local did=0

	if [ ! -f "$SETTINGS_JSON" ]; then
		echo "settings.json not found at $SETTINGS_JSON - skipping"
	elif ! grep -qF -- "$INSECURE_FLAG" "$SETTINGS_JSON"; then
		echo "$INSECURE_FLAG not in settings.json"
	else
		local tmp
		tmp="$(mktemp "${SETTINGS_JSON}.XXXXXX")"
		if jq -c --arg f "$INSECURE_FLAG" \
			'.clientArguments |= map(select(. != $f))' \
			"$SETTINGS_JSON" >"$tmp"; then
			chmod 644 "$tmp"
			mv "$tmp" "$SETTINGS_JSON"
			echo "removed $INSECURE_FLAG from settings.json"
			echo "  clientArguments: $(jq -c '.clientArguments' "$SETTINGS_JSON")"
			did=1
		else
			rm -f "$tmp"
			echo "plugin.sh: failed to update settings.json" >&2
			exit 1
		fi
	fi

	if [ -f "$CREDENTIALS" ]; then
		rm -f "$CREDENTIALS"
		echo "deleted ~/.runelite/credentials.properties"
		did=1
	else
		echo "credentials.properties not present"
	fi

	[ "$did" -eq 1 ] || echo "nothing to do"
}

launch_running_pid() {
	# echo the PID of a live launch (the gradle process), else return non-zero
	[ -f "$PID_FILE" ] || return 1
	local pid
	pid="$(cat "$PID_FILE" 2>/dev/null || true)"
	[ -n "$pid" ] || return 1
	kill -0 "$pid" 2>/dev/null || return 1
	ps -p "$pid" -o command= 2>/dev/null | grep -qi "gradle" || return 1
	echo "$pid"
}

cmd_launch() {
	local pid
	if pid="$(launch_running_pid)"; then
		echo "dev client already launching/running (gradle PID $pid)"
		return 0
	fi
	if pgrep -f "$CLIENT_MAIN" >/dev/null 2>&1; then
		echo "plugin.sh: a RuneLite client ($CLIENT_MAIN) is already running" >&2
		exit 1
	fi

	[ -f "$CREDENTIALS" ] ||
		echo "note: credentials.properties absent - client will open at the login screen (see './plugin.sh auth')" >&2

	cd "$PLUGIN_DIR"
	nohup ./gradlew run </dev/null >"$LOG_FILE" 2>&1 &
	local new=$!
	echo "$new" >"$PID_FILE"

	sleep 2
	if ! kill -0 "$new" 2>/dev/null; then
		echo "plugin.sh: gradlew exited immediately - last log lines:" >&2
		tail -n 20 "$LOG_FILE" >&2 || true
		rm -f "$PID_FILE"
		exit 1
	fi

	echo "dev client launching in the background (gradle PID $new)"
	echo "  the RuneLite window can take ~30-60s to appear (gradle build + JVM start)"
	echo "  log: $LOG_FILE"
	echo "  once it's up this terminal is safe to close"
}

case "${1:-}" in
	launch)
		cmd_launch
		;;
	auth)
		case "${2:-}" in
			status)
				auth_status
				;;
			enable)
				auth_enable
				;;
			disable)
				auth_disable
				;;
			"" | -h | --help | --usage)
				auth_usage
				;;
			*)
				echo "plugin.sh: unknown auth command '$2'" >&2
				echo >&2
				auth_usage >&2
				exit 1
				;;
		esac
		;;
	"" | -h | --help | --usage | help)
		usage
		;;
	*)
		echo "plugin.sh: unknown command '$1'" >&2
		echo >&2
		usage >&2
		exit 1
		;;
esac
