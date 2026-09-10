# vitals-system scripts

Helpers for the three parts that get OSRS HP/Prayer onto the LED panels:

| Script | Wraps | Purpose |
|---|---|---|
| `sketch.sh` | `arduino-cli` | build / flash `../vitals/vitals.ino` |
| `bridge.sh` | `../bridge/bridge.py` | run the UDP→serial bridge |
| `plugin.sh` | `../runelite-vitals/` (`./gradlew`) | drive the RuneLite plugin dev loop |

Data path: `RuneLite plugin --UDP--> bridge.py --USB serial--> vitals.ino (SRC_SERIAL)`

## Conventions

All three scripts:

- are plain `bash` with `set -euo pipefail`
- resolve their own location, so they work from anywhere (`./bridge.sh …` here, or `vitals-system/scripts/bridge.sh …` from elsewhere)
- print usage on no arguments, `--help`, or `--usage`
- exit `1` with usage on stderr for an unknown command
- keep pid/log files under `$TMPDIR` (macOS per-user temp; cleared on reboot)

---

## sketch.sh

`arduino-cli` wrapper for `../vitals`. Board `FQBN` and `PORT` are pinned at the top of the script.

| Command | Does |
|---|---|
| `./sketch.sh build` | Compile the sketch |
| `./sketch.sh upload` | Compile, then upload to the Mega |

- Upload target: `/dev/cu.usbmodem1201` — edit `PORT` in the script if the board moves.
- Needs: `arduino-cli`, the `arduino:avr` core, the FastLED library.

---

## bridge.sh

Runs `../bridge/bridge.py`, which listens on UDP `127.0.0.1:9999` and relays fill percents to the board over serial.

| Command | Does |
|---|---|
| `./bridge.sh start [args]` | Start the bridge backgrounded |
| `./bridge.sh stop` | Stop the backgrounded bridge |
| `./bridge.sh status` | Running? plus the UDP + serial ports in use |
| `./bridge.sh debug [args]` | Run in the foreground with `-v` logging (Ctrl-C to stop) |
| `./bridge.sh test [serial_port]` | Drive the board directly — bridge must be **stopped** |

**`start` / `debug`** forward anything after them to `bridge.py`:

| Arg | Default | Effect |
|---|---|---|
| `-v` | off | log every line received (`<-`) and forwarded (`->`) |
| `--port <dev>` | first `/dev/cu.usbmodem*` | pin the serial device |
| `--udp <port>` | `9999` | UDP port to listen on (must match the plugin config) |

- `start` is a no-op if one is already running; on immediate failure it prints the log tail and exits `1`.
- `stop` sends SIGINT, escalating to SIGKILL after ~3s.
- `status` reads the log for the current serial device, or shows `disconnected (bridge retrying)` if the link dropped.
- `debug` refuses to run while a backgrounded bridge is up (UDP bind clash). It writes no pidfile, so `status` won't track it.

**`test`** has its own `pyserial` code and talks straight to the board — no bridge involved — so run it with the bridge stopped, e.g. right after `./sketch.sh upload` to confirm the board is alive. It runs two mirrored HP/Prayer sweeps, holds `HP 75% / PR 40%` briefly, then blanks the board. Default device is the first `/dev/cu.usbmodem*`; pass one to override.

- Files: `$TMPDIR/vitals-bridge.pid`, `$TMPDIR/vitals-bridge.log`
- Needs: `python3` with `pyserial`

---

## plugin.sh

Manages the `../runelite-vitals` dev loop. `./gradlew run` starts a developer-mode RuneLite client that loads the plugin as a builtin — the normal Jagex-Launcher client can't sideload it.

| Command | Does |
|---|---|
| `./plugin.sh launch` | Start the dev client (`./gradlew run`) detached |
| `./plugin.sh auth status` | Show whether dev-auth is provisioned |
| `./plugin.sh auth enable` | Add `--insecure-write-credentials` to the RuneLite `settings.json` |
| `./plugin.sh auth disable` | Remove that flag **and** delete `credentials.properties` |

### launch

Backgrounds `./gradlew run` with `nohup` and output to the log, so no build spam hits the terminal and you can close it once the client window is up (~30–60s). **There is no `stop`** — quit the RuneLite client and the gradle task ends by itself.

- Refuses to start a second client.
- Warns (non-fatally) if `credentials.properties` is missing — without it the client opens at the login screen instead of signing in.
- If gradle exits within the first couple of seconds it prints the log tail and fails.

### auth

Jagex login for a non-launcher client comes from `~/.runelite/credentials.properties`, which the RuneLite client only writes when started with `--insecure-write-credentials`. Flow:

1. `./plugin.sh auth enable`
2. launch RuneLite once via the **Jagex Launcher** → it writes `credentials.properties`
3. `./plugin.sh launch` now signs in automatically
4. `./plugin.sh auth disable` when you're done or stepping away — pulls the flag, deletes the credentials file

`enable` / `disable` edit `settings.json` with `jq` (atomic write, other keys untouched) and are idempotent. `credentials.properties` is full game-account access in plaintext — don't leave it lying around.

- Files: RuneLite `settings.json`, `~/.runelite/credentials.properties`, `$TMPDIR/vitals-plugin.{pid,log}`
- Needs: JDK 17 (pinned in `../runelite-vitals/gradle.properties`), `jq`

---

## Typical flows

Play session, board docked:

```
./sketch.sh upload      # only if the sketch changed
./bridge.sh start       # background UDP->serial relay
./plugin.sh launch      # background dev client; terminal is free to close
```

Check a fresh sketch upload, no RuneLite:

```
./sketch.sh upload
./bridge.sh test        # sweeps the panels, then blanks
```

Teardown / leaving the machine:

```
# quit the RuneLite client
./bridge.sh stop
./plugin.sh auth disable   # if you provisioned dev-auth
```
