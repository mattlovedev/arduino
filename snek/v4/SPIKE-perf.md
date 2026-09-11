# SPIKE: v4 is notably slower than v2/v3 at the same STEP_MS

Status: OPEN — tabled, not started.

## Symptom

v4 runs visibly slower than v2/v3 even though `STEP_MS` is 50 in all of
them. The unresolved "orbits food forever" bug (tracked separately —
see snek v4 loop bug memory) is a distinct issue and NOT the thing
under investigation here.

## Working theory

`loop()` in v4.ino runs `chooseMove()` -> `applyMove()` -> `render()`
-> `delay(STEP_MS)`, and that delay is unconditional. STEP_MS was never
a target period, just a pause tacked on after everything else
finishes. So the real step time is `compute_time + 50ms`, not a flat
50ms. v2/v3's decision logic was near-instant, so total time was ~=
50ms; v4's `chooseMove()` does real work now, and every ms it costs is
pure addition on top of the delay.

Suspected hot path: `chooseMove()` can call `bestSafeToward()` twice
(toward food, then toward tail as a stall fallback). Each call runs
`reachableArea()` (a bounded BFS flood-fill) once per candidate
direction in the filter pass, then again per surviving candidate in
the tie-break pass — up to ~16 flood-fills in a single tick worst
case. Inside each flood-fill, `enqueue()` calls `s.hits(p)`, an O(len)
linear scan of the snake body. So per-tick cost is roughly
O(flood-fills x cells-visited x len), which gets worse as the snake
grows.

Secondary suspect: `Snake::segment()` uses `%`, and the AVR Mega 2560
has no hardware divide (software-emulated, notably more expensive than
add/sub). It's called deep inside the O(len) `hits()` loop that the
flood-fill hammers repeatedly.

## Investigative steps (not yet run)

1. Confirm the mechanism directly: wrap `chooseMove()` with `micros()`
   before/after in `loop()`, log the delta (min/max/avg) over serial.
   Confirms real per-tick compute cost vs. the assumed 50ms.
2. Split cost further: time the food-directed vs. tail-directed
   `bestSafeToward()` calls separately, and count how many times
   `reachableArea()` actually runs per tick (confirm whether it's
   hitting the ~16x worst case or usually much less).
3. Measure growth vs. snake length: log `(len, compute_time)` pairs
   across a full game run. Roughly quadratic growth would confirm the
   `hits()`-inside-flood-fill theory over something constant like
   `render()`.
4. Baseline against v3: add identical `micros()` instrumentation to
   v3.ino so the comparison is measured, not assumed from reading the
   code.
5. Isolate `render()` / `FastLED.show()` timing separately — LED
   protocol timing is fixed by LED count and shouldn't have changed
   between versions, but worth ruling out rather than assuming.
6. Check `arduino-cli compile` flash/RAM stats as a sanity check —
   confirm the two static 512-byte flood-fill scratch arrays aren't
   pushing close to a resource limit that could cause other slowdowns.

## Why this matters for v5

This is exactly the kind of question the planned v5 board->PC
telemetry (see snek v5 plan) should make trivial to answer instead of
reasoning about it from `delay()` semantics after the fact. Worth
keeping this spike in mind when scoping what that telemetry needs to
capture (per-phase timing, not just final outcome).
