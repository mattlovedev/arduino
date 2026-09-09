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
//   3. switch DATA_SOURCE = SRC_SERIAL and have the bridge send fill percents:
//        HP <pct>\n      (0..100, may exceed 100 when boosted)
//        PR <pct>\n
//      All cur/max math is done upstream; the board only ever sees a percent.

#include <FastLED.h>

// ===================== build switches =====================
#define MODE_RUN    0
#define MODE_TEST   1
#define MODE        MODE_RUN

#define SRC_STATIC  0   // render the percents baked into hp/pray below
#define SRC_DEMO    1   // oscillate both bars across their full range
#define SRC_SERIAL  2   // parse "HP <pct>" / "PR <pct>" lines from Serial
#define DATA_SOURCE SRC_SERIAL

// ===================== hardware =====================
#define LED_PIN       2
#define CHIPSET       WS2812B
#define COLOR_ORDER   GRB
#define BRIGHTNESS    24
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
  int   pct;     // target fill percent, 0..100 (>100 when boosted above max)
  float shown;   // eased percent actually drawn
  CRGB  color;
};

Stat hp   = { 55, 55.0f, CRGB(255,   8,   0) };  // hitpoints red
Stat pray = { 75, 75.0f, CRGB(  0,   0, 255) };  // prayer blue

#define EASE 0.18f

// index within one panel, local coords px:0..31 py:0..7
static uint16_t panelLocal(uint8_t px, uint8_t py) {
  px = PANEL_W - 1 - px;                // data-in / column 0 is on the right
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
  float frac = s.shown / 100.0f;                 // 0..1 (>1 when boosted)
  for (uint8_t x = 0; x < WIDTH; x++) {
    float fill = constrain((frac - x / (float)WIDTH) * WIDTH, 0.0f, 1.0f);
    CRGB c = CRGB::Black;                        // empty columns stay dark
    if (fill > 0.0f) {
      c = s.color;
      if (fill < 1.0f) c.nscale8_video((uint8_t)(fill * 255));  // soften the edge pixel
    }
    for (uint8_t r = 0; r < PANEL_H; r++) setPixel(x, rowTop + r, c);
  }
  if (frac > 1.0f) {  // boosted above max: white overflow from the left
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
  hp.pct   = beatsin16(5, 0, 100);
  pray.pct = beatsin16(8, 0, 100);
}

#else  // SRC_SERIAL
char    lineBuf[16];
uint8_t lineLen = 0;

static void applyReading(const char *tag, int pct) {
  pct = constrain(pct, 0, 200);
  if      (!strcmp(tag, "HP")) hp.pct   = pct;
  else if (!strcmp(tag, "PR")) pray.pct = pct;
}

void pumpData() {
  while (Serial.available()) {
    char ch = Serial.read();
    if (ch == '\n' || ch == '\r') {
      lineBuf[lineLen] = '\0';
      if (lineLen) {
        char *tag = strtok(lineBuf, " ");
        char *a   = strtok(NULL, " ");
        if (tag && a) applyReading(tag, atoi(a));
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
  EVERY_N_MILLISECONDS(20) {
    float prevHp = hp.shown, prevPr = pray.shown;
    hp.shown   += (hp.pct   - hp.shown)   * EASE;
    pray.shown += (pray.pct - pray.shown) * EASE;
    if (fabsf(hp.pct   - hp.shown) < 0.3f) hp.shown   = hp.pct;   // settle & stop
    if (fabsf(pray.pct - pray.shown) < 0.3f) pray.shown = pray.pct;
    // FastLED.show() blocks interrupts ~15ms (512 WS2812B) -> board is deaf to
    // serial while it runs. Only render when the picture actually moved, so an
    // idle board listens 100% of the time and reacts instantly to new data.
    if (hp.shown != prevHp || pray.shown != prevPr) render();
  }
#endif
}
