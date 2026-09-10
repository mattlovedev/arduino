#!/usr/bin/env python3
"""bridge.py - relay RuneLite Vitals UDP packets to the Arduino over serial.

    RuneLite plugin  --UDP-->  this  --USB serial-->  vitals.ino (SRC_SERIAL)

Listens on 127.0.0.1:<port> for the plugin's "HP <cur> <max>\\n" / "PR <cur> <max>\\n"
lines, converts each to a fill percent, and writes "HP <pct>\\n" / "PR <pct>\\n" to
the board. Reconnects if the serial port drops or its name changes, re-sends the
last values after each (re)connect, and blanks the board (HP 0 / PR 0) when the
plugin goes quiet (i.e. you logged out) so the lights don't stay on.

Usage:  python3 vitals-system/bridge/bridge.py [--port /dev/cu.usbmodemXXXX] [--udp 9999] [-v]
Needs:  pip3 install pyserial
"""
import argparse
import glob
import socket
import sys
import time

try:
    import serial  # pyserial
except ImportError:
    sys.exit("pyserial not installed - run: pip3 install pyserial")

BAUD = 115200
RESET_WAIT = 2.0          # the Mega auto-resets when the serial port is opened
RECONNECT_EVERY = 2.0
IDLE_BLANK_AFTER = 5.0    # no UDP for this long -> assume logged out, blank board
VALID_TAGS = ("HP", "PR")


def find_port(explicit):
    if explicit:
        return explicit
    matches = (sorted(glob.glob("/dev/cu.usbmodem*"))
               or sorted(glob.glob("/dev/tty.usbmodem*")))
    return matches[0] if matches else None


def parse_line(line):
    """'HP 55 99' from the plugin -> 'HP 56' for the board (tag + fill percent).

    All cur/max math lives here so the serial link only ever carries a percent.
    Returns None if the line isn't a valid reading.
    """
    parts = line.split()
    if len(parts) != 3 or parts[0] not in VALID_TAGS:
        return None
    try:
        cur, mx = int(parts[1]), int(parts[2])
    except ValueError:
        return None
    if mx <= 0:
        return None
    return f"{parts[0]} {round(100 * cur / mx)}"


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--port", help="serial device (default: autodetect /dev/cu.usbmodem*)")
    ap.add_argument("--udp", type=int, default=9999, help="UDP port to listen on")
    ap.add_argument("-v", "--verbose", action="store_true",
                    help="log every line received (<-) and forwarded (->)")
    args = ap.parse_args()

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind(("127.0.0.1", args.udp))
    sock.settimeout(0.2)

    def log(*a):
        print(time.strftime("%H:%M:%S"), *a, flush=True)

    log(f"[bridge] listening on 127.0.0.1:{args.udp}")

    ser = None
    last = {}             # {"HP": "HP 56", "PR": "PR 75"}
    next_try = 0.0
    last_rx = time.time()
    blanked = False

    def close_serial():
        nonlocal ser
        if ser:
            try:
                ser.close()
            except Exception:
                pass
        ser = None

    def write_serial(line):
        """Write one 'TAG n' line to the board. Returns True on success."""
        nonlocal ser
        if ser is None:
            return False
        try:
            ser.write((line + "\n").encode())
            return True
        except (OSError, serial.SerialException):
            log("[bridge] write failed, will reconnect")
            close_serial()
            return False

    while True:
        # --- keep the serial link up ---
        if ser is None and time.time() >= next_try:
            next_try = time.time() + RECONNECT_EVERY
            port = find_port(args.port)
            if port:
                try:
                    ser = serial.Serial(port, BAUD, timeout=0)
                    log(f"[bridge] opened {port} @ {BAUD}, waiting {RESET_WAIT:g}s for board reset")
                    time.sleep(RESET_WAIT)
                    for msg in last.values():          # re-prime the board
                        ser.write((msg + "\n").encode())
                    if last:
                        log(f"[bridge] re-sent {' | '.join(last.values())}")
                except (OSError, serial.SerialException) as e:
                    log(f"[bridge] open failed: {e}")
                    close_serial()

        # --- surface anything the board prints back ---
        if ser is not None:
            try:
                waiting = ser.in_waiting
                if waiting:
                    txt = ser.read(waiting).decode("ascii", "replace").strip()
                    if txt:
                        log(f"[board] {txt}")
            except (OSError, serial.SerialException):
                log("[bridge] serial dropped")
                close_serial()

        # --- blank the board if the plugin has gone quiet (logout) ---
        if not blanked and time.time() - last_rx > IDLE_BLANK_AFTER:
            log(f"[bridge] no data for {IDLE_BLANK_AFTER:g}s - blanking board")
            for tag in VALID_TAGS:
                last[tag] = f"{tag} 0"
                write_serial(f"{tag} 0")
            blanked = True

        # --- receive a UDP datagram (may hold multiple lines) ---
        try:
            data, _ = sock.recvfrom(1024)
        except socket.timeout:
            continue
        last_rx = time.time()
        if blanked:
            log("[bridge] data resumed")
            blanked = False

        for raw in data.decode("ascii", "replace").splitlines():
            raw = raw.strip()
            if not raw:
                continue
            if args.verbose:
                log(f"<- {raw}")
            msg = parse_line(raw)
            if not msg:
                continue
            last[msg.split()[0]] = msg
            if write_serial(msg) and args.verbose:
                log(f"-> {msg}")


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\n[bridge] stopped")
