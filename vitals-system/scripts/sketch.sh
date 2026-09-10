#!/usr/bin/env bash
#
# sketch.sh - build (and optionally upload) the vitals sketch.
#
# Thin wrapper over arduino-cli for vitals-system/vitals/. Board settings are
# pinned below; edit them here if the Mega moves to a different port.

set -euo pipefail

FQBN="arduino:avr:mega:cpu=atmega2560"
PORT="/dev/cu.usbmodem1201"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SKETCH_DIR="$(cd "$SCRIPT_DIR/../vitals" && pwd)"

usage() {
	cat <<EOF
sketch.sh - build / upload the vitals sketch

Usage:
  ./sketch.sh build             Compile the sketch only
  ./sketch.sh upload            Compile, then upload to the Mega
  ./sketch.sh --help|--usage    Show this message

Config (edit at the top of this script):
  FQBN    $FQBN
  PORT    $PORT
  sketch  $SKETCH_DIR
EOF
}

case "${1:-}" in
	build)
		arduino-cli compile -b "$FQBN" "$SKETCH_DIR"
		;;
	upload)
		arduino-cli compile -b "$FQBN" "$SKETCH_DIR"
		arduino-cli upload -b "$FQBN" -p "$PORT" "$SKETCH_DIR"
		;;
	"" | -h | --help | --usage | help)
		usage
		;;
	*)
		echo "sketch.sh: unknown option '$1'" >&2
		echo >&2
		usage >&2
		exit 1
		;;
esac
