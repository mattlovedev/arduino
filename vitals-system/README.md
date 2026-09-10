# vitals-system

Streams Old School RuneScape character stats — current Hitpoints and Prayer — from
the PC onto a 16×32 WS2812B LED matrix in real time. Top half is a Hitpoints bar,
bottom half a Prayer bar.

```
RuneLite plugin  --UDP-->  bridge.py  --USB serial-->  vitals.ino  ->  LED panels
 (runelite-vitals)          (bridge/)                   (vitals/)
```

Three programs, one per hop, plus wrapper scripts:

| Path | What it is |
|---|---|
| `vitals/` | the Arduino sketch that drives the panels |
| `bridge/` | the PC-side UDP→serial relay |
| `runelite-vitals/` | the RuneLite plugin that reads the stats |
| `scripts/` | build / run helpers — see `scripts/README.md` |

---

## vitals/ — the sketch

`vitals/vitals.ino`, for an Arduino Mega 2560 driving two 32×8 WS2812B panels
stacked vertically (rows 0–7 = Hitpoints, rows 8–15 = Prayer). Each stat is a
horizontal fill bar with a soft edge pixel and white overflow when boosted above
100%.

- **Input** comes from a `DATA_SOURCE` compile switch: `SRC_STATIC` (hard-coded),
  `SRC_DEMO` (oscillating), or `SRC_SERIAL` (the real path — parses the serial
  stream). Currently `SRC_SERIAL`.
- **Serial protocol** @ 115200: newline-terminated `HP <pct>` / `PR <pct>`, one
  integer percent per line (0–200; over 100 = boosted). All cur/max math is done
  upstream, so the board only ever sees a percent.
- **`MODE_TEST`** build sweeps a single pixel to verify the three panel-wiring
  flags before first run.
- Built and flashed with `arduino-cli` (FQBN `arduino:avr:mega:cpu=atmega2560`),
  needs the FastLED library. See `scripts/sketch.sh`.

## bridge/ — the relay

`bridge/bridge.py` sits between the plugin and the board:

- Listens on UDP `127.0.0.1:9999` for the plugin's `HP <cur> <max>` /
  `PR <cur> <max>` lines, converts each to a fill percent, and writes
  `HP <pct>` / `PR <pct>` to the board over serial.
- Autodetects `/dev/cu.usbmodem*`, reconnects if the port drops or is renamed,
  and re-sends the last values after each reconnect.
- Blanks the board (`HP 0` / `PR 0`) after 5 s of silence, so the lights don't
  stay lit after you log out.
- `-v` logs each line received and forwarded; anything the board prints back is
  surfaced too.

`bridge/udpdump.py` is a raw UDP viewer for debugging the packet stream without a
board. Needs `pyserial`. See `scripts/bridge.sh`.

## runelite-vitals/ — the RuneLite plugin

A RuneLite plugin (package `dev.mattlove.vitals`, config group `vitals`) scaffolded
from RuneLite's example-plugin.

- On each game tick while logged in, reads boosted + real Hitpoints and Prayer and
  sends them as one UDP datagram to the bridge (`127.0.0.1:9999` by default).
- Sends on change plus a keepalive every 2 s; goes silent on logout.
- **Dev-loop only.** It runs in a developer-mode client via `./gradlew run`, which
  loads it as a builtin — the normal Jagex-Launcher client can't sideload it. The
  client version is pinned in `build.gradle`; building needs JDK 17.
- Jagex login for that client comes from `~/.runelite/credentials.properties`,
  provisioned with `--insecure-write-credentials`.

See `scripts/plugin.sh` for the launch and auth helpers.

---

## Hardware

Arduino Mega 2560, two 32×8 WS2812B panels (512 LEDs) on data pin 2, `GRB`,
brightness 24. Full pin/button/power notes live with the other sketches in the
repo root.

## scripts/

Thin `bash` wrappers so the common actions are one command each — `sketch.sh`
(build/upload), `bridge.sh` (start/stop/status/debug/test), `plugin.sh`
(launch, auth). Details and typical flows in `scripts/README.md`.
