// snek v3 — the decision algorithm, written out and then implemented.
//
// The comment block below is a SPEC: the exact move-picking logic from
// snek/v2 (faithful to the original snek/snek.ino), in prose +
// pseudocode, for review before v4 changes it. Everything after the
// "IMPLEMENTATION" divider is a fresh coding of that spec in my own
// style — not a refactor of v1/v2.
//
// =====================================================================
// ONE-PARAGRAPH SUMMARY
// =====================================================================
//
// The snake plays itself. Once every STEP_MS it looks at three things —
// where its head is, where the food is, and which cells its body
// occupies — and nothing else. It ranks the four compass directions by
// how few steps that direction's axis would take to line up with the
// food (measured with wraparound, because the board is a torus), then
// walks that ranked list and takes the first direction that doesn't run
// its head straight into its own body on the very next cell. It looks
// exactly one cell ahead: it never checks whether a move will trap it.
// If all four directions are blocked it takes the top-ranked one anyway
// and dies. On death the score flashes and the game resets.
//
// =====================================================================
// STATE
// =====================================================================
//
//   WIDTH, HEIGHT        board size in cells; the board is a torus
//                        (x wraps mod WIDTH, y wraps mod HEIGHT)
//   body[0 .. len-1]     snake cells; body[0] is the head, body[len-1]
//                        the tail
//   food                 one cell, never currently on the snake
//   dir                  last direction moved. STORED, BUT NEVER READ
//                        by the decision logic — the snake has no
//                        heading/momentum; each step is decided from
//                        scratch.
//
// Direction convention (from coordsForDir):
//   LEFT  => x - 1        RIGHT => x + 1
//   UP    => y + 1        DOWN  => y - 1      (yes, UP increases y)
// It's internally consistent; screen orientation is handled later by
// the panel map, not here.
//
// =====================================================================
// PER-STEP ALGORITHM
// =====================================================================
//
//   STEP():
//       ranked = RANK_DIRECTIONS()      // 4 directions, best-first
//       dir    = FIRST_SAFE(ranked)     // falls back to ranked[0]
//       APPLY(dir)
//       redraw
//
//
//   RANK_DIRECTIONS():                  // = snekChoices()
//       H = body[0]                     // head
//       F = food
//
//       // wrapped single-axis distances: how many steps in THIS
//       // direction until the head shares the food's column / row.
//       dLEFT  = (H.x - F.x + WIDTH)  mod WIDTH
//       dRIGHT = (F.x - H.x + WIDTH)  mod WIDTH
//       dUP    = (F.y - H.y + HEIGHT) mod HEIGHT
//       dDOWN  = (H.y - F.y + HEIGHT) mod HEIGHT
//
//       // note: dLEFT and dRIGHT are both 0 (head on food's column)
//       // or both non-zero; never exactly one. Same for dUP/dDOWN.
//       // So dLEFT + dRIGHT == WIDTH  and  dUP + dDOWN == HEIGHT,
//       // except when that axis is already aligned (0 + 0).
//
//       dirs = [ (LEFT,  dLEFT ),
//                (RIGHT, dRIGHT),
//                (UP,    dUP   ),
//                (DOWN,  dDOWN ) ]
//
//       sort dirs ascending by distance
//       // Implemented with qsort and a (a.dist - b.dist) comparator.
//       // qsort is NOT stable: among equal distances the resulting
//       // order is whatever the C library produces for this input —
//       // arbitrary, though deterministic. There is no defined
//       // tie-break preference (e.g. no "prefer horizontal").
//
//       if dirs[0].dist == 0 AND dirs[1].dist == 0:
//           // The head already shares the food's row OR its column,
//           // so the two zero entries are the axis that's already
//           // solved. Moving along that axis would only un-align it.
//           // Push the still-open axis to the front; keep the solved
//           // axis as the last resort.
//           //   dirs[2], dirs[3] = the open axis, nearer way first
//           //   dirs[0], dirs[1] = the solved axis
//           return [ dirs[2].dir, dirs[3].dir, dirs[0].dir, dirs[1].dir ]
//
//       // normal case: nearest axis first
//       return [ dirs[0].dir, dirs[1].dir, dirs[2].dir, dirs[3].dir ]
//
//
//   FIRST_SAFE(ranked):                 // = evalChoices()
//       for dir in ranked:              // best to worst
//           next = STEP_CELL(H, dir)    // one cell, with wrap
//           if not HITS_BODY(next):
//               return dir
//       return ranked[0]                // fully boxed in: take the
//                                       // best-ranked move and die
//                                       // on APPLY
//
//
//   HITS_BODY(p):                       // = snekContains()
//       // checks body[0 .. len-2] ONLY. The tail, body[len-1], is
//       // deliberately skipped: it moves out of its cell on this same
//       // step, so stepping onto it is legal.
//       for i in 0 .. len-2:
//           if body[i].x == p.x AND body[i].y == p.y:
//               return true
//       return false
//
//
//   STEP_CELL(p, dir):                  // = coordsForDir()
//       one step from p in dir, each axis wrapping in [0 .. DIM-1]
//
//
//   APPLY(dir):                         // = snekUpdate()
//       head = STEP_CELL(H, dir)
//       if HITS_BODY(head):
//           GAME OVER  -> showScores(); reset snake to length 1 at
//                         (0,0), dir = NONE; spawn new food
//       else if head == food:
//           EAT        -> prepend head, keep the tail (len grows by 1);
//                         spawn new food
//       else:
//           MOVE       -> prepend head, drop the tail (len unchanged)
//
//
//   NEW_FOOD():                         // = newFood()
//       pick a uniform random cell; repeat while HITS_BODY(cell).
//       Because HITS_BODY exempts the tail, the food can land on the
//       current tail cell.
//
// =====================================================================
// WHAT THIS ALGORITHM IS
// =====================================================================
//
//   - Greedy pursuit with a one-cell safety check. Nothing more.
//   - "Distance" is the single smaller-looking wrapped gap on one axis,
//     ranked per direction. It is NOT true path length and takes no
//     account of the body being in the way further along.
//   - Direction is chosen fresh every step from head + food + body.
//     Current heading (dir) is ignored, so the snake has no turn
//     inertia and will happily 180 when len <= 2 (see edge cases).
//
// =====================================================================
// WHAT IT DOES NOT DO   (candidate work for v4)
// =====================================================================
//
//   - No lookahead past one cell. It never asks "if I go here, do I
//     still have a way out?" -> it routinely seals itself into a pocket
//     of its own body and dies with open board elsewhere.
//   - No flood-fill / reachable-area check.
//   - No tail-chasing or other survival fallback when the food route
//     is unsafe.
//   - No preference to keep moving straight, hug a wall, or avoid
//     cutting the board in half.
//   - Torus edges are treated as free shortcuts; it will cross an edge
//     to save a step.
//
// =====================================================================
// TIE-BREAKS & EDGE CASES
// =====================================================================
//
//   - Equal distances: order is whatever qsort yields for that exact
//     input. No defined preference. Two different builds of the C
//     library could rank ties differently.
//   - The `dirs[1].dist == 0` half of the special-case test is
//     redundant: zeros always come in pairs, so dirs[0].dist == 0
//     already implies dirs[1].dist == 0.
//   - Reversing straight back onto itself: BLOCKED for len >= 3
//     (body[1] is inside the checked range); ALLOWED for len <= 2
//     (only the head is checked, and the tail is exempt).
//   - All four directions blocked: FIRST_SAFE returns ranked[0], APPLY
//     sees HITS_BODY(head), and the game ends this step.
//   - On reset: snake is length 1 at (0,0), dir = NONE, fresh food.
//     The best-ever score persists across games.
//
// =====================================================================
// WORKED EXAMPLE — why one-cell lookahead loses
// =====================================================================
//
//   . . . . . . . . . .      H  head, F  food, #  body
//   . # # # # # # # . . .
//   . #           # . . .     Food is down-left of the head, so
//   . #   . H      # . . .    RANK_DIRECTIONS puts LEFT (or DOWN)
//   . #           # . . .     first. Both lead into the pocket.
//   . # # # # # #  # . . .    FIRST_SAFE only checks the next cell,
//   . . . . . . . . . . .     which is empty, so it goes in.
//   . . . F . . . . . . .     A step or two later every exit is body
//                             and the snake dies — even though going
//                             RIGHT first, around the outside, was
//                             always safe.
//
// =====================================================================


// #####################################################################
// IMPLEMENTATION
//
// My own coding of the spec above. Behaviour matches it. Two things to
// call out:
//
//  * Ranking ties are broken in fixed LEFT, RIGHT, UP, DOWN order
//    (a small stable insertion sort) instead of being left to qsort.
//    The spec says tie order is undefined, so this is compliant — but
//    it means v3 and v2 can pick different moves when two directions
//    are equidistant.
//  * `dir` (last heading) isn't stored at all, since nothing reads it.
//  * The snake body is a fixed ring buffer (WIDTH*HEIGHT Points of
//    static RAM, ~2 KB) rather than v1/v2's grow-on-demand malloc.
//
// The panel map and the digit font are lifted from v2 unchanged — they
// are hardware / presentation, not the thing under study.
// #####################################################################

#include <FastLED.h>

// ----------------------- configuration -----------------------
#define LED_CHIPSET     WS2812B
#define LED_COLOR_ORDER GRB
constexpr uint8_t LED_PIN    = 2;
constexpr uint8_t BRIGHTNESS = 128;

// Two stacked 32x8 panels -> a 32-wide, 16-tall board (as in vitals).
constexpr int PANEL_W  = 32;
constexpr int PANEL_H  = 8;
constexpr int WIDTH    = PANEL_W;
constexpr int HEIGHT   = PANEL_H * 2;
constexpr int NUM_LEDS = WIDTH * HEIGHT;

// Panel wiring — verify with a pixel sweep once, then leave alone.
constexpr bool TOP_IS_FIRST         = false;  // does DIN enter the top panel?
constexpr bool TOP_PANEL_ROTATED    = true;   // top panel mounted upside-down
constexpr bool BOTTOM_PANEL_ROTATED = false;

constexpr int STEP_MS = 50;   // wall time per snake move

// ----------------------- types -----------------------
struct Point {
  int x;
  int y;
  bool operator==(const Point& o) const { return x == o.x && y == o.y; }
};

enum Direction { Left, Right, Up, Down, DirCount };

// UP increases y, DOWN decreases y — matches the spec's convention.
constexpr Point kStep[DirCount] = {
  { -1,  0 },   // Left
  { +1,  0 },   // Right
  {  0, +1 },   // Up
  {  0, -1 },   // Down
};

enum MoveOutcome { Moved, Ate, Died };

// Snake body as a ring buffer: cells[head] is the head, and segment(i)
// walks back toward the tail. O(1) grow / advance, no heap.
struct Snake {
  static constexpr int kCap = WIDTH * HEIGHT;

  Point cells[kCap];
  int   head;
  int   len;

  void reset() {
    head = 0;
    len  = 1;
    cells[0] = { 0, 0 };
  }

  Point headCell() const { return cells[head]; }

  // i == 0 -> head, i == len-1 -> tail
  Point segment(int i) const {
    int idx = (head - i) % kCap;
    if (idx < 0) idx += kCap;
    return cells[idx];
  }

  // Move the head onto `to`; keep the tail iff `grow` (we ate).
  void advance(Point to, bool grow) {
    head = (head + 1) % kCap;
    cells[head] = to;
    if (grow) len++;
  }

  // Is `p` on the snake, ignoring the tail? The tail vacates its cell
  // on the same tick, so stepping onto it is legal.
  bool hits(Point p) const {
    for (int i = 0; i < len - 1; i++)
      if (segment(i) == p) return true;
    return false;
  }
};

CRGB  leds[NUM_LEDS];
Snake snake;
Point food;

// ----------------------- panel map (from v2) -----------------------
static uint16_t panelLocal(uint8_t px, uint8_t py) {
  px = PANEL_W - 1 - px;                       // column 0 is on the right
  return (px & 1) ? (px * PANEL_H + py)
                  : (px * PANEL_H + (PANEL_H - 1 - py));
}

static int xy(int x, int y) {
  bool inTop   = (y < PANEL_H);
  uint8_t px   = x;
  uint8_t py   = inTop ? y : (y - PANEL_H);
  bool rotated = inTop ? TOP_PANEL_ROTATED : BOTTOM_PANEL_ROTATED;
  if (rotated) { px = PANEL_W - 1 - px; py = PANEL_H - 1 - py; }
  uint16_t base = (inTop == TOP_IS_FIRST) ? 0 : (PANEL_W * PANEL_H);
  return base + panelLocal(px, py);
}

static void setCell(Point p, const CRGB& color) {
  if (p.x < 0 || p.x >= WIDTH || p.y < 0 || p.y >= HEIGHT) return;
  leds[xy(p.x, p.y)] = color;
}

// ----------------------- digit font (from v2) -----------------------
// 5-wide x 7-tall glyphs, x right / y down, y=0 at the top. Each array
// is the digit's lit cells as interleaved x,y, held in flash.
static const uint8_t GLYPH_0[] PROGMEM = { 1,0, 2,0, 3,0, 0,1, 4,1, 0,2, 4,2, 0,3, 4,3, 0,4, 4,4, 0,5, 4,5, 1,6, 2,6, 3,6 };
static const uint8_t GLYPH_1[] PROGMEM = { 2,0, 1,1, 2,1, 2,2, 2,3, 2,4, 2,5, 1,6, 2,6, 3,6 };
static const uint8_t GLYPH_2[] PROGMEM = { 1,0, 2,0, 3,0, 0,1, 4,1, 4,2, 3,3, 2,4, 1,5, 0,6, 1,6, 2,6, 3,6, 4,6 };
static const uint8_t GLYPH_3[] PROGMEM = { 0,0, 1,0, 2,0, 4,1, 4,2, 1,3, 2,3, 3,3, 4,4, 4,5, 0,6, 1,6, 2,6 };
static const uint8_t GLYPH_4[] PROGMEM = { 3,0, 2,1, 3,1, 1,2, 3,2, 0,3, 3,3, 0,4, 1,4, 2,4, 3,4, 4,4, 3,5, 3,6 };
static const uint8_t GLYPH_5[] PROGMEM = { 0,0, 1,0, 2,0, 3,0, 4,0, 0,1, 0,2, 0,3, 1,3, 2,3, 3,3, 4,4, 4,5, 0,6, 1,6, 2,6, 3,6 };
static const uint8_t GLYPH_6[] PROGMEM = { 1,0, 2,0, 3,0, 0,1, 0,2, 0,3, 1,3, 2,3, 3,3, 0,4, 4,4, 0,5, 4,5, 1,6, 2,6, 3,6 };
static const uint8_t GLYPH_7[] PROGMEM = { 0,0, 1,0, 2,0, 3,0, 4,0, 4,1, 3,2, 2,3, 1,4, 1,5, 1,6 };
static const uint8_t GLYPH_8[] PROGMEM = { 1,0, 2,0, 3,0, 0,1, 4,1, 0,2, 4,2, 1,3, 2,3, 3,3, 0,4, 4,4, 0,5, 4,5, 1,6, 2,6, 3,6 };
static const uint8_t GLYPH_9[] PROGMEM = { 1,0, 2,0, 3,0, 0,1, 4,1, 0,2, 4,2, 1,3, 2,3, 3,3, 4,3, 4,4, 4,5, 1,6, 2,6, 3,6 };

struct Glyph {
  const uint8_t *cells;   // PROGMEM, interleaved x,y
  uint8_t        pairs;
};

static const Glyph GLYPHS[10] = {
  { GLYPH_0, sizeof(GLYPH_0) / 2 }, { GLYPH_1, sizeof(GLYPH_1) / 2 },
  { GLYPH_2, sizeof(GLYPH_2) / 2 }, { GLYPH_3, sizeof(GLYPH_3) / 2 },
  { GLYPH_4, sizeof(GLYPH_4) / 2 }, { GLYPH_5, sizeof(GLYPH_5) / 2 },
  { GLYPH_6, sizeof(GLYPH_6) / 2 }, { GLYPH_7, sizeof(GLYPH_7) / 2 },
  { GLYPH_8, sizeof(GLYPH_8) / 2 }, { GLYPH_9, sizeof(GLYPH_9) / 2 },
};

static void drawDigit(int originX, int originY, int digit, const CRGB& color) {
  if (digit < 0 || digit > 9) return;
  const Glyph& g = GLYPHS[digit];
  for (uint8_t i = 0; i < g.pairs; i++) {
    uint8_t cx = pgm_read_byte(&g.cells[2 * i]);
    uint8_t cy = pgm_read_byte(&g.cells[2 * i + 1]);
    setCell({ originX + cx, originY + cy }, color);
  }
}

static void drawNumber(int originX, int originY, int value, const CRGB& color) {
  if (value >= 100) drawDigit(originX, originY, (value / 100) % 10, color);
  drawDigit(originX + 8,  originY, (value / 10) % 10, color);
  drawDigit(originX + 16, originY, value % 10,        color);
}

// ----------------------- score screen -----------------------
// Blink the run length (top panel) over the best-ever length (bottom
// panel), five frames. Run length is green on a new best, blue on a
// tie, red otherwise; best is always blue.
static void showScores(int runLen) {
  static int best = 0;
  CRGB runColor = CRGB::Red;
  if      (runLen > best)  { best = runLen; runColor = CRGB::Green; }
  else if (runLen == best) { runColor = CRGB::Blue; }

  for (int frame = 0; frame < 5; frame++) {
    FastLED.clearData();
    if (frame % 2 == 0) drawNumber(4, 0, runLen, runColor);
    drawNumber(4, PANEL_H, best, CRGB::Blue);
    FastLED.show();
    delay(500);
  }
}

// ############### THE ALGORITHM ###############

// One step from `from` in `d`, wrapping each axis. Only ever called
// after a single-cell move, so a branch per edge covers it.
static int wrapAxis(int v, int size) {
  if (v < 0)     return size - 1;
  if (v >= size) return 0;
  return v;
}

static Point stepCell(Point from, Direction d) {
  return { wrapAxis(from.x + kStep[d].x, WIDTH),
           wrapAxis(from.y + kStep[d].y, HEIGHT) };
}

// A uniformly random empty cell. Treats the tail as free (same as
// Snake::hits), so food can briefly land under the tail.
static Point randomFreeCell(const Snake& s) {
  Point p;
  do {
    p = { int(random(WIDTH)), int(random(HEIGHT)) };
  } while (s.hits(p));
  return p;
}

// Wrapped single-axis gap: steps in `d` until the head shares the
// food's column (Left/Right) or row (Up/Down).
static int axisDistance(Direction d, Point head, Point food) {
  switch (d) {
    case Left:  return (head.x - food.x + WIDTH)  % WIDTH;
    case Right: return (food.x - head.x + WIDTH)  % WIDTH;
    case Up:    return (food.y - head.y + HEIGHT) % HEIGHT;
    case Down:  return (head.y - food.y + HEIGHT) % HEIGHT;
    default:    return 0;
  }
}

// Fill `out` with all four directions, best first:
//  - normally: smallest axis distance first
//  - if the head is already aligned with the food on one axis (that
//    axis gives a 0/0 pair): lead with the other axis and keep the
//    aligned one as the last resort.
static void rankDirections(Point head, Point food, Direction out[DirCount]) {
  struct Ranked { Direction dir; int dist; };
  Ranked r[DirCount];
  for (int i = 0; i < DirCount; i++) {
    Direction d = Direction(i);
    r[i] = { d, axisDistance(d, head, food) };
  }

  // stable insertion sort, ascending by dist; ties keep L,R,U,D order
  for (int i = 1; i < DirCount; i++) {
    Ranked key = r[i];
    int j = i - 1;
    while (j >= 0 && r[j].dist > key.dist) {
      r[j + 1] = r[j];
      --j;
    }
    r[j + 1] = key;
  }

  // Zeros come in pairs, so r[0].dist == 0 already implies r[1] too.
  if (r[0].dist == 0) {
    out[0] = r[2].dir;   // open axis, nearer way first
    out[1] = r[3].dir;
    out[2] = r[0].dir;   // aligned axis, last resort
    out[3] = r[1].dir;
  } else {
    for (int i = 0; i < DirCount; i++) out[i] = r[i].dir;
  }
}

// The move for this tick: the best-ranked direction whose next cell
// isn't our own body. If every direction is blocked, return the
// best-ranked one anyway — we die on it.
static Direction chooseMove(const Snake& s, Point food) {
  Direction ranked[DirCount];
  rankDirections(s.headCell(), food, ranked);
  for (Direction d : ranked)
    if (!s.hits(stepCell(s.headCell(), d)))
      return d;
  return ranked[0];
}

static MoveOutcome applyMove(Snake& s, Point& food, Direction d) {
  Point next = stepCell(s.headCell(), d);
  if (s.hits(next)) return Died;

  bool ate = (next == food);
  s.advance(next, ate);
  if (ate) food = randomFreeCell(s);
  return ate ? Ate : Moved;
}

// ############### render & main ###############

static void render() {
  FastLED.clearData();
  for (int i = 0; i < snake.len; i++) setCell(snake.segment(i), CRGB::Green);
  setCell(food, CRGB::Red);
  FastLED.show();
}

void setup() {
  randomSeed(analogRead(A0));   // the original harvested 32 bits; one is plenty here

  FastLED.addLeds<LED_CHIPSET, LED_PIN, LED_COLOR_ORDER>(leds, NUM_LEDS)
         .setCorrection(TypicalSMD5050);
  FastLED.setBrightness(BRIGHTNESS);

  snake.reset();
  food = randomFreeCell(snake);
  render();
}

void loop() {
  Direction dir = chooseMove(snake, food);

  if (applyMove(snake, food, dir) == Died) {
    showScores(snake.len);
    snake.reset();
    food = randomFreeCell(snake);
  }

  render();
  delay(STEP_MS);
}
