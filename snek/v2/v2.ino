// snek v2 — a self-playing snake on the vitals 16x32 LED stack.
//
// A tidy-up of ../snek.ino, moved onto the two-panel display from
// vitals-system/vitals. The maths and every decision the snake makes
// are unchanged from v1; what's new is the layout, the naming, the
// digit-font storage, and the panel mapping (v1 drove one 64x8 panel,
// this drives a 32-wide, 16-tall stack). Build this folder on its own
// (arduino-cli compile snek/v2); it does not touch ../snek.ino.
//
// Hardware
//   Two 32x8 WS2812B panels stacked vertically, DIN on pin 2 — a
//   32-wide, 16-tall display. Same panels and wiring as
//   vitals-system/vitals.
//     rows 0..7   -> top panel
//     rows 8..15  -> bottom panel
//   Each panel is an internal vertical serpentine, 8 LEDs per column.
//   The three flags below (which panel gets DIN first, whether each is
//   mounted upside-down) match the vitals build.
//
// How it plays
//   Every STEP_MS the snake ranks the four directions by wrapped
//   distance to the food (nearest first), then takes the first one that
//   doesn't run into its own body — looking exactly one step ahead. If
//   all four are blocked it takes the nearest anyway and dies: the run
//   length and the best-ever length flash (run length on the top panel,
//   best on the bottom), then a new game starts. The board is a torus,
//   so the snake and the food wrap around every edge.
//
// Bring-up
//   MODE = MODE_TEST walks a single green pixel across the grid and
//   prints x,y to Serial @ 9600. It should travel left->right along the
//   top row, then step down, ending bottom-right. Adjust the panel
//   flags until it does, then set MODE = MODE_RUN to play.

#include <FastLED.h>
#include <stdlib.h>
#include <string.h>

// ===================== build switches =====================
#define MODE_RUN    0
#define MODE_TEST   1   // walk a single green pixel across the grid
#define MODE_DIGITS 2   // static 123 on the top panel, 456 on the bottom
#define MODE        MODE_RUN

// ===================== hardware =====================
#define LED_PIN     2
#define CHIPSET     WS2812B
#define COLOR_ORDER GRB
#define BRIGHTNESS  128

// ===================== panel layout =====================
// Each panel: 8 leds per column, internal vertical serpentine. Verify
// with MODE_TEST, then leave these alone.
#define PANEL_W    32
#define PANEL_H    8
#define PANEL_LEDS (PANEL_W * PANEL_H)

#define TOP_IS_FIRST         0   // 1 if the data line (DIN) enters the TOP panel
#define TOP_PANEL_ROTATED    1   // top panel physically mounted upside-down
#define BOTTOM_PANEL_ROTATED 0

// ===================== board geometry =====================
#define WIDTH    PANEL_W
#define HEIGHT   (PANEL_H * 2)
#define NUM_LEDS (WIDTH * HEIGHT)
#define STEP_MS  50            // delay between snake moves

CRGB leds[NUM_LEDS];

// ===================== directions =====================
#define NONE  0
#define LEFT  1
#define UP    2
#define RIGHT 3
#define DOWN  4

// ===================== types =====================
// Defined before any function so Arduino's auto-generated prototypes
// can see them.
struct Point {
  int x;
  int y;
};

struct Snake {
  Point *body;   // body[0] is the head
  int    len;
  int    cap;    // Points allocated in body
  int    dir;
};

Snake snek = {};
Point food;

// ===================== score-digit font =====================
// 5-wide x 7-tall glyphs, one per digit, drawn in the panel's own
// orientation: x runs left->right, y runs top->bottom with y=0 at the
// top. Each array lists that digit's lit cells as interleaved x,y.
// Kept in flash; the render path reads them with pgm_read_byte.
//
//   0        1        2        3        4        5
//   .###.    ..#..    .###.    ###..    ...#.    #####
//   #...#    .##..    #...#    ....#    ..##.    #....
//   #...#    ..#..    ....#    ....#    .#.#.    #....
//   #...#    ..#..    ...#.    .###.    #..#.    ####.
//   #...#    ..#..    ..#..    ....#    #####    ....#
//   #...#    ..#..    .#...    ....#    ...#.    ....#
//   .###.    .###.    #####    ###..    ...#.    ####.
//
//   6        7        8        9
//   .###.    #####    .###.    .###.
//   #....    ....#    #...#    #...#
//   #....    ...#.    #...#    #...#
//   ####.    ..#..    .###.    .####
//   #...#    .#...    #...#    ....#
//   #...#    .#...    #...#    ....#
//   .###.    .#...    .###.    .###.
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
  { GLYPH_0, sizeof(GLYPH_0) / 2 },
  { GLYPH_1, sizeof(GLYPH_1) / 2 },
  { GLYPH_2, sizeof(GLYPH_2) / 2 },
  { GLYPH_3, sizeof(GLYPH_3) / 2 },
  { GLYPH_4, sizeof(GLYPH_4) / 2 },
  { GLYPH_5, sizeof(GLYPH_5) / 2 },
  { GLYPH_6, sizeof(GLYPH_6) / 2 },
  { GLYPH_7, sizeof(GLYPH_7) / 2 },
  { GLYPH_8, sizeof(GLYPH_8) / 2 },
  { GLYPH_9, sizeof(GLYPH_9) / 2 },
};

// ===================== panel mapping =====================
// Ported from vitals-system/vitals. index within one panel, local
// coords px:0..31 py:0..7.
static uint16_t panelLocal(uint8_t px, uint8_t py) {
  px = PANEL_W - 1 - px;                // data-in / column 0 is on the right
  return (px & 1) ? (px * PANEL_H + py)
                  : (px * PANEL_H + (PANEL_H - 1 - py));
}

// global (x,y), origin top-left, -> strip index
static int xy(int x, int y) {
  bool inTop = (y < PANEL_H);
  uint8_t px = x;
  uint8_t py = inTop ? y : (y - PANEL_H);
  bool rotated = inTop ? TOP_PANEL_ROTATED : BOTTOM_PANEL_ROTATED;
  if (rotated) { px = PANEL_W - 1 - px; py = PANEL_H - 1 - py; }
  uint16_t base = (inTop == (bool)TOP_IS_FIRST) ? 0 : PANEL_LEDS;
  return base + panelLocal(px, py);
}

static void drawPixel(int x, int y, CRGB::HTMLColorCode color) {
  if (x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT) return;   // two-panel map: never index off-grid
  leds[xy(x, y)] = color;
}

// ===================== font rendering =====================
// one digit, its top-left corner at (xOffset, yOffset)
static void drawDigit(int xOffset, int yOffset, int digit, CRGB::HTMLColorCode color) {
  if (digit < 0 || digit > 9) return;      // v1 drew nothing outside 0..9
  const Glyph &g = GLYPHS[digit];
  for (uint8_t i = 0; i < g.pairs; i++) {
    uint8_t cx = pgm_read_byte(&g.cells[2 * i]);
    uint8_t cy = pgm_read_byte(&g.cells[2 * i + 1]);
    drawPixel(xOffset + cx, yOffset + cy, color);
  }
}

// up to three digits: hundreds (only when non-zero), tens, ones — at
// xOffset, xOffset+8, xOffset+16, exactly as v1's drawNum.
static void drawNumber(int xOffset, int yOffset, int value, CRGB::HTMLColorCode color) {
  int hundreds = value / 100;
  if (hundreds) drawDigit(xOffset, yOffset, hundreds, color);
  drawDigit(xOffset + 8,  yOffset, (value % 100) / 10, color);
  drawDigit(xOffset + 16, yOffset, value % 10,         color);
}

// ===================== snake =====================
static void newSnek() {
  if (snek.body != NULL) free(snek.body);
  snek.body = (Point *) calloc(10, sizeof(*snek.body));   // head starts at (0,0)
  snek.len  = 1;
  snek.cap  = 10;
  snek.dir  = NONE;
}

static void snekDraw() {
  for (int i = 0; i < snek.len; i++)
    drawPixel(snek.body[i].x, snek.body[i].y, CRGB::Green);
}

// Is p on the snake? The tail (body[len-1]) is skipped: it vacates its
// cell on the same step, so moving onto it is legal.
static bool snekContains(Point p) {
  for (int i = 0; i < snek.len - 1; i++)
    if (snek.body[i].x == p.x && snek.body[i].y == p.y) return true;
  return false;
}

// shift the body up one slot and drop in the new head (no growth)
static void snekMove(Point head) {
  memmove(&snek.body[1], &snek.body[0], sizeof(*snek.body) * (snek.len - 1));
  snek.body[0] = head;
}

// same, but keep the tail and double the buffer if it's full
static void snekFeed(Point head) {
  if (snek.len == snek.cap) {
    snek.cap *= 2;
    snek.body = (Point *) realloc(snek.body, snek.cap * sizeof(*snek.body));
  }
  memmove(&snek.body[1], &snek.body[0], sizeof(*snek.body) * snek.len);
  snek.body[0] = head;
  snek.len++;
}

// ===================== food =====================
static Point newFood() {
  int x = random(WIDTH);
  int y = random(HEIGHT);
  while (snekContains(Point{x, y})) {
    x = random(WIDTH);
    y = random(HEIGHT);
  }
  return Point{x, y};
}

static void foodDraw() {
  drawPixel(food.x, food.y, CRGB::Red);
}

// ===================== movement =====================
// one step in dir from p, wrapping around every edge
static Point coordsForDir(Point p, int dir) {
  switch (dir) {
    case LEFT:  if (--p.x == -1)     p.x = WIDTH - 1;  break;
    case UP:    if (++p.y == HEIGHT) p.y = 0;          break;
    case RIGHT: if (++p.x == WIDTH)  p.x = 0;          break;
    case DOWN:  if (--p.y == -1)     p.y = HEIGHT - 1; break;
  }
  return p;
}

// ===================== the AI =====================
struct Choice {
  int dir;
  int n;     // wrapped distance to the food in that direction
};

static int cmpChoice(const void *a, const void *b) {
  return ((const Choice *)a)->n - ((const Choice *)b)->n;
}

// Rank the four directions nearest-first. Returns a pointer to a static
// buffer, valid until the next call.
static int *snekChoices() {
  int nLeft  = (snek.body[0].x - food.x + WIDTH)  % WIDTH;
  int nRight = (food.x - snek.body[0].x + WIDTH)  % WIDTH;
  int nUp    = (food.y - snek.body[0].y + HEIGHT) % HEIGHT;
  int nDown  = (snek.body[0].y - food.y + HEIGHT) % HEIGHT;

  Choice dirs[4] = {
    { LEFT,  nLeft  },
    { RIGHT, nRight },
    { UP,    nUp    },
    { DOWN,  nDown  },
  };
  qsort(dirs, 4, sizeof(Choice), cmpChoice);

  static int choices[4];
  if (dirs[0].n == 0 && dirs[1].n == 0) {
    // head already shares a row or column with the food: close the gap
    // on the other axis first, keep the aligned axis as the fallback.
    choices[0] = dirs[2].dir;
    choices[1] = dirs[3].dir;
    choices[2] = dirs[0].dir;
    choices[3] = dirs[1].dir;
    return choices;
  }
  for (int i = 0; i < 4; i++) choices[i] = dirs[i].dir;
  return choices;
}

// first ranked direction that doesn't hit the body; if all four do,
// take the best one and die.
static int evalChoices(int choices[4]) {
  for (int i = 0; i < 4; i++)
    if (!snekContains(coordsForDir(snek.body[0], choices[i])))
      return choices[i];
  return choices[0];
}

// ===================== one game step =====================
static void showScores();   // defined below

static void snekUpdate(int dir) {
  Point head = coordsForDir(snek.body[0], dir);
  snek.dir = dir;

  if (snekContains(head)) {
    showScores();
    newSnek();
    food = newFood();
  } else if (head.x == food.x && head.y == food.y) {
    snekFeed(head);
    food = newFood();
  } else {
    snekMove(head);
  }
}

// ===================== score screen =====================
// Blink the run length (top panel) above the best-ever length (bottom
// panel), five times. The run length is green if it's a new best, blue
// if it ties, red otherwise; the best-ever length is always blue.
static void showScores() {
  static int best = 0;
  CRGB::HTMLColorCode color = CRGB::Red;
  if (snek.len > best) {
    best = snek.len;
    color = CRGB::Green;
  } else if (snek.len == best) {
    color = CRGB::Blue;
  }
  for (int i = 0; i < 5; i++) {
    FastLED.clearData();
    if (i % 2 == 0) drawNumber(4, 0, snek.len, color);
    drawNumber(4, PANEL_H, best, CRGB::Blue);
    FastLED.show();
    delay(500);
  }
}

// ===================== RNG seed =====================
// 32 low bits of a floating analog pin, one bit per read.
static void setupRandom() {
  unsigned long seed = 0;
  for (int i = 0; i < 32; i++)
    seed |= (unsigned long)(analogRead(A0) & 0x01) << i;
  randomSeed(seed);
}

// ===================== main =====================
void setup() {
  Serial.begin(9600);
  setupRandom();
  FastLED.addLeds<CHIPSET, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS)
         .setCorrection(TypicalSMD5050);
  FastLED.setBrightness(BRIGHTNESS);
  FastLED.clearData();
  newSnek();
  food = newFood();
  snekDraw();
  foodDraw();
  FastLED.show();
  delay(STEP_MS);
}

#if MODE == MODE_TEST
void loop() {
  static uint8_t tx = 0, ty = 0;
  FastLED.clearData();
  drawPixel(tx, ty, CRGB::Green);
  FastLED.show();
  Serial.print("x="); Serial.print(tx);
  Serial.print(" y="); Serial.println(ty);
  delay(120);
  if (++tx >= WIDTH) { tx = 0; if (++ty >= HEIGHT) ty = 0; }
}
#elif MODE == MODE_DIGITS
void loop() {
  FastLED.clearData();
  drawNumber(4, 0,       123, CRGB::Green);   // top panel
  drawNumber(4, PANEL_H, 456, CRGB::Blue);    // bottom panel
  FastLED.show();
  delay(1000);
}
#else
void loop() {
  snekUpdate(evalChoices(snekChoices()));
  FastLED.clearData();
  snekDraw();
  foodDraw();
  FastLED.show();
  delay(STEP_MS);
}
#endif
