// colortest — flood the whole matrix with one color at a time, advancing every
// 2 s through: red, orange, yellow, green, blue, purple, white, black.
// Color name is printed to Serial @ 115200 so you can match what you see.

#include <FastLED.h>

#define LED_PIN       2
#define CHIPSET       WS2812B
#define COLOR_ORDER   GRB
#define BRIGHTNESS    64
#define MAX_MILLIAMPS 2000

#define WIDTH    32
#define HEIGHT   16
#define NUM_LEDS (WIDTH * HEIGHT)

#define STEP_MS  2000

CRGB leds[NUM_LEDS];

struct NamedColor { const char *name; CRGB rgb; };

const NamedColor PALETTE[] = {
  { "red",    CRGB(255,   0,   0) },
  { "orange", CRGB(255,  80,   0) },
  { "yellow", CRGB(255, 255,   0) },
  { "green",  CRGB(  0, 255,   0) },
  { "blue",   CRGB(  0,   0, 255) },
  { "purple", CRGB(128,   0, 255) },
  { "white",  CRGB(255, 255, 255) },
  { "black",  CRGB(  0,   0,   0) },
};
const uint8_t PALETTE_LEN = sizeof(PALETTE) / sizeof(PALETTE[0]);

void setup() {
  Serial.begin(115200);
  FastLED.addLeds<CHIPSET, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS)
         .setCorrection(UncorrectedColor);
  FastLED.setBrightness(BRIGHTNESS);
  FastLED.setMaxPowerInVoltsAndMilliamps(5, MAX_MILLIAMPS);
  FastLED.clear(true);
}

void loop() {
  static uint8_t i = 0;

  EVERY_N_MILLISECONDS(STEP_MS) {
    const NamedColor &c = PALETTE[i];
    fill_solid(leds, NUM_LEDS, c.rgb);
    FastLED.show();
    Serial.println(c.name);
    i = (i + 1) % PALETTE_LEN;
  }
}
