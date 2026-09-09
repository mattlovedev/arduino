// vitals — stream OSRS character stats to a 16x32 LED matrix.
//
// Hardware: two 32x8 WS2812B panels stacked vertically on an Arduino Mega.
//   rows 0..7   -> top panel    -> Hitpoints
//   rows 8..15  -> bottom panel -> Prayer
//
// Bring-up:
//   1. build with MODE = MODE_TEST, open Serial Monitor @ 115200, watch the
//      single green pixel sweep. It should travel left->right along the top
//      row first, then step down. Fix the three wiring flags below until it does.
//   2. switch to MODE = MODE_RUN with DATA_SOURCE = SRC_STATIC and tune colors.
//   3. switch DATA_SOURCE = SRC_SERIAL and have the PC send:
//        HP <cur> <max>\n
//        PR <cur> <max>\n

#include <FastLED.h>

// ===================== build switches =====================
#define MODE_RUN    0
#define MODE_TEST   1
#define MODE        MODE_RUN

#define SRC_STATIC  0   // render the values baked into hp/pray below
#define SRC_DEMO    1   // oscillate both bars across their full range
#define SRC_SERIAL  2   // parse "HP c m" / "PR c m" lines from Serial
#define DATA_SOURCE SRC_STATIC

// ===================== hardware =====================
#define LED_PIN       2
#define CHIPSET       WS2812B
#define COLOR_ORDER   GRB
#define BRIGHTNESS    96
#define MAX_MILLIAMPS 2000

// ===================== panel layout =====================
// Each panel is an internal vertical serpentine: 8 leds per column, even
// columns wired bottom->top. Verify with MODE_TEST, then leave these alone.
#define PANEL_W    32
#define PANEL_H    8
#define PANEL_LEDS (PANEL_W * PANEL_H)

#define WIDTH    PANEL_W
#define HEIGHT   (PANEL_H * 2)
#define NUM_LEDS (WIDTH * HEIGHT)

#define TOP_IS_FIRST         0   // 1 if the data line (DIN) enters the TOP panel
#define TOP_PANEL_ROTATED    1   // top panel physically mounted upside-down
#define BOTTOM_PANEL_ROTATED 0

CRGB leds[NUM_LEDS];

// ===================== stats =====================
// (declared up here so Arduino's auto-generated prototypes can see the type)
struct Stat {
  int   cur;     // current level, may exceed max when boosted
  int   max;     // base level
  float shown;   // eased value actually drawn
  CRGB  color;
};

Stat hp   = { 55, 99, 55.0f, CRGB(255,   8,   0) };  // hitpoints red
Stat pray = { 31, 99, 31.0f, CRGB(  0,   0, 255) };  // prayer blue

#define TRACK_LEVEL 14   // brightness of a bar's empty track (0..255)
#define EASE        0.18f

// index within one panel, local coords px:0..31 py:0..7
static uint16_t panelLocal(uint8_t px, uint8_t py) {
  return (px & 1) ? (px * PANEL_H + py)
                  : (px * PANEL_H + (PANEL_H - 1 - py));
}

// global (x,y), origin top-left, -> strip index
int xy(int x, int y) {
  bool inTop = (y < PANEL_H);
  uint8_t px = x;
  uint8_t py = inTop ? y : (y - PANEL_H);
  bool rotated = inTop ? TOP_PANEL_ROTATED : BOTTOM_PANEL_ROTATED;
  if (rotated) { px = PANEL_W - 1 - px; py = PANEL_H - 1 - py; }
  uint16_t base = (inTop == (bool)TOP_IS_FIRST) ? 0 : PANEL_LEDS;
  return base + panelLocal(px, py);
}

void setPixel(int x, int y, const CRGB &c) {
  if (x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT) return;
  leds[xy(x, y)] = c;
}

// ===================== rendering =====================
void drawBar(uint8_t rowTop, const Stat &s) {
  float frac = (s.max > 0) ? s.shown / s.max : 0.0f;
  for (uint8_t x = 0; x < WIDTH; x++) {
    float fill = constrain((frac - x / (float)WIDTH) * WIDTH, 0.0f, 1.0f);
    uint8_t level = TRACK_LEVEL + (uint8_t)(fill * (255 - TRACK_LEVEL));
    CRGB c = s.color;
    c.nscale8_video(level);
    for (uint8_t r = 0; r < PANEL_H; r++) setPixel(x, rowTop + r, c);
  }
  if (frac > 1.0f) {  // boosted: white overflow from the left
    uint8_t cols = (uint8_t)ceil(constrain(frac - 1.0f, 0.0f, 1.0f) * WIDTH);
    for (uint8_t x = 0; x < cols; x++)
      for (uint8_t r = 0; r < PANEL_H; r++) setPixel(x, rowTop + r, CRGB::White);
  }
}

void render() {
  drawBar(0,       hp);
  drawBar(PANEL_H, pray);
  FastLED.show();
}

// ===================== data source =====================
#if DATA_SOURCE == SRC_STATIC
void pumpData() {}

#elif DATA_SOURCE == SRC_DEMO
void pumpData() {
  hp.cur   = beatsin16(5, 0, hp.max);
  pray.cur = beatsin16(8, 0, pray.max);
}

#else  // SRC_SERIAL
char    lineBuf[24];
uint8_t lineLen = 0;

static void applyReading(const char *tag, int cur, int mx) {
  if (mx <= 0) return;
  cur = constrain(cur, 0, mx * 2);
  if      (!strcmp(tag, "HP")) { hp.cur = cur;   hp.max = mx; }
  else if (!strcmp(tag, "PR")) { pray.cur = cur; pray.max = mx; }
}

void pumpData() {
  while (Serial.available()) {
    char ch = Serial.read();
    if (ch == '\n' || ch == '\r') {
      lineBuf[lineLen] = '\0';
      if (lineLen) {
        char *tag = strtok(lineBuf, " ");
        char *a   = strtok(NULL, " ");
        char *b   = strtok(NULL, " ");
        if (tag && a && b) applyReading(tag, atoi(a), atoi(b));
      }
      lineLen = 0;
    } else if (lineLen < sizeof(lineBuf) - 1) {
      lineBuf[lineLen++] = ch;
    }
  }
}
#endif

// ===================== main =====================
void setup() {
  Serial.begin(115200);
  FastLED.addLeds<CHIPSET, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS)
         .setCorrection(UncorrectedColor);
  FastLED.setBrightness(BRIGHTNESS);
  FastLED.setMaxPowerInVoltsAndMilliamps(5, MAX_MILLIAMPS);
  FastLED.clear(true);
}

void loop() {
#if MODE == MODE_TEST
  static uint8_t tx = 0, ty = 0;
  FastLED.clear();
  setPixel(tx, ty, CRGB::Green);
  FastLED.show();
  Serial.print("x="); Serial.print(tx);
  Serial.print(" y="); Serial.println(ty);
  delay(120);
  if (++tx >= WIDTH) { tx = 0; if (++ty >= HEIGHT) ty = 0; }
#else
  pumpData();
  EVERY_N_MILLISECONDS(16) {
    hp.shown   += (hp.cur   - hp.shown)   * EASE;
    pray.shown += (pray.cur - pray.shown) * EASE;
    render();
  }
#endif
}
