#!/usr/bin/env python3
"""Print UDP packets arriving on a port. Used to watch the RuneLite plugin's
HP/PR stream before the real serial bridge exists.

Usage:  python3 bridge/udpdump.py [port]   (default 9999)
"""
import socket
import sys
import time

port = int(sys.argv[1]) if len(sys.argv) > 1 else 9999

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind(("127.0.0.1", port))
print(f"listening on 127.0.0.1:{port} (ctrl-c to stop)")

while True:
    data, addr = sock.recvfrom(2048)
    stamp = time.strftime("%H:%M:%S")
    try:
        text = data.decode("ascii")
    except UnicodeDecodeError:
        print(f"{stamp} {addr[0]}:{addr[1]}  <non-ascii> {data!r}")
        continue
    print(f"{stamp} {addr[0]}:{addr[1]}  {text!r}")
