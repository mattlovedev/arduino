#include <FastLED.h>

#define BRIGHTNESS 16

#define REFRESH_SPEED 10

#define WIDTH 32
#define HEIGHT 16

#define NUM_LEDS (WIDTH * HEIGHT)

CRGB leds[NUM_LEDS];

void loop() {
  for (int y = 0; y < HEIGHT; y++) {
    for (int x = 0; x < WIDTH; x++) {
      *xyLed(x, y) = CRGB::Green;
      FastLED.show();
      delay(REFRESH_SPEED);
      *xyLed(x, y) = CRGB::Black;
    }
  }
  /*for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CRGB::Green;
    FastLED.show();
    leds[i] = CRGB::Black;
    delay(REFRESH_SPEED);
  }*/
  //*xyLed(0, 0) = CRGB::Green;
}



#define CHIPSET     WS2812B
#define LED_PIN  2
#define COLOR_ORDER GRB

void setup() {
  Serial.begin(9600);
  FastLED.addLeds<CHIPSET, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalSMD5050);
  FastLED.setBrightness( BRIGHTNESS );
}