#include <FastLED.h>

#define LED_PIN  2
#define COLOR_ORDER GRB
#define CHIPSET     WS2812B
#define BRIGHTNESS 64
#define REFRESH_SPEED 50
#define WIDTH 32
#define HEIGHT 16
#define NUM_LEDS (WIDTH * HEIGHT)

CRGB leds[NUM_LEDS];

int xy (int x, int y) {
  if (y < 8) {
    if (x % 2 == 0) {
      return 256 + (31 - x) * 8 + (7 - y);
    }
    return 256 + (31 - x) * 8 + y;
  }
  if (x % 2 == 0) {
    return (x * 8) + (15 - y);
  }
  return (x * 8) + (y - 8);
}

CRGB *xyLed (int x, int y) {
  return &leds[xy(x, y)];
}

unsigned long LETTERS[] = {
31626430,
33216170,
15255082,
33080878,
33216177,
32674977,
15259309,
32641183,
18415153,
13124143,
32641361,
33047056,
32575583,
32575775,
15255086,
32674978,
15254838,
32675002,
19584681,
1113121,
16269839,
7618823,
32772383,
18157905,
1142849,
18667121
};

const char *MESSAGE = "MATT LOVE   ";

#define NUM_LETTERS_IN_MESSAGE (strlen(MESSAGE))
#define NUM_COLS_IN_MESSAGE (NUM_LETTERS_IN_MESSAGE * 6)

CRGB::HTMLColorCode colors[] = {
  CRGB::Purple,
  CRGB::Indigo,
  CRGB::MediumBlue,
  CRGB::DarkCyan,
  CRGB::Green,
  CRGB::Yellow,
  CRGB::OrangeRed,
  CRGB::DarkRed
};

#define NUM_COLORS (sizeof(colors)/sizeof(colors[0]))

static int colorCount = 0;

unsigned char getColFromMessage(int col) {
  if (col % 6 == 5 || col >= NUM_COLS_IN_MESSAGE) {
    return 0;
  }

  int messageIdx = col / 6;

  if (MESSAGE[messageIdx] == ' ') {
    return 0;
  }

  int letterIdx = MESSAGE[messageIdx] - 'A';

  unsigned long letter = LETTERS[letterIdx];

  int colIdx = col % 6; // 0 - 4

  unsigned char c = 0;

  for (int i = 0; i < 5; i++ ) {
    c <<= 1;
    unsigned long mask = 1;
    int maskShift = (24 - (colIdx * 5 + i));
    mask <<= maskShift;
    if (letter & mask) {
      c |= 1;
    }
  }
  
  return c;
}

int windowBegin = 0;

int messageLength = NUM_COLS_IN_MESSAGE;

int getFromWindow(int n) {
  if (windowBegin + n >= messageLength) {
    return windowBegin + n - messageLength;
  }
  return windowBegin + n;
}

void slideWindow() {
  if(++windowBegin == messageLength) {
    windowBegin = 0;
  }
}

void drawWindow() {
  for (int i = 0; i < 32; i++) {
    unsigned char col = getColFromMessage(getFromWindow(i));
    for (int j = 0; j < 5; j++) {
      unsigned char mask = 1;
      int maskShift = 4 - j;
      mask <<= maskShift;
      if (col & mask) {
        *xyLed(i, j+2) = colors[(getFromWindow(i) / 6) % NUM_COLORS];
      }
    }
  }
}

void verticalLineCol (int x, int startY, int endY, CRGB::HTMLColorCode color) {
  for (int y = startY; y <= endY; y++) {
    //*xyLed(x, y) = color;
    //*xyLed(x,y) = 0xde2a64;
    *xyLed(x,y) = 0xc22357;
  }
}

void loop() {
  //slideWindow();
  //FastLED.clearData();
  //drawWindow();
  //FastLED.show();
  delay(REFRESH_SPEED*1000);
}

void setup() {
  Serial.begin(9600);
  FastLED.addLeds<CHIPSET, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalSMD5050);
  FastLED.setBrightness( BRIGHTNESS );
  FastLED.clearData();
  /*verticalLineCol(3, 3, 3, CRGB::Purple);
  verticalLineCol(4, 3, 4, CRGB::Purple);
  verticalLineCol(5, 1, 5, CRGB::Purple);
  verticalLineCol(6, 2, 5, CRGB::Purple);
  verticalLineCol(7, 3, 5, CRGB::Purple);*/

  CRGB::HTMLColorCode color = CRGB::DeepPink;

  verticalLineCol(8, 7, 7, color);
  verticalLineCol(9, 7, 8, color);
  verticalLineCol(10, 7, 9, color);
  verticalLineCol(11, 7, 10, color);
  verticalLineCol(12, 7, 11, color);
  verticalLineCol(13, 7, 12, color);
  verticalLineCol(14, 7, 13, color);
  verticalLineCol(15, 0, 14, color);
  verticalLineCol(16, 1, 14, color);
  verticalLineCol(17, 2, 14, color);
  verticalLineCol(18, 3, 14, color);
  verticalLineCol(19, 4, 14, color);
  verticalLineCol(20, 5, 14, color);
  verticalLineCol(21, 6, 14, color);
  verticalLineCol(22, 7, 14, color);

  FastLED.show();
}
