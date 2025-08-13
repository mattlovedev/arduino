#include <time.h>
#include <FastLED.h>

#define LED_PIN  2

#define COLOR_ORDER GRB
#define CHIPSET     WS2812B

#define BRIGHTNESS 128

#define REFRESH_SPEED 100

#define WIDTH 64
#define HEIGHT 8

#define NUM_LEDS (WIDTH * HEIGHT)
CRGB leds[NUM_LEDS];

const char *MESSAGE = "BLOB CHEF ";

#define NUM_LETTERS_IN_MESSAGE (strlen(MESSAGE))
#define NUM_COLS_IN_MESSAGE (NUM_LETTERS_IN_MESSAGE * 6)

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

CRGB::HTMLColorCode colors[] = {
    CRGB::Green,CRGB::Green,CRGB::Green,CRGB::Green,CRGB::Green,
    CRGB::SpringGreen,CRGB::SpringGreen,CRGB::SpringGreen,CRGB::SpringGreen,CRGB::SpringGreen
  /*CRGB::DarkRed,
  CRGB::OrangeRed,
  CRGB::Yellow,
  CRGB::Green,
  CRGB::DarkCyan,
  CRGB::MediumBlue,
  CRGB::Indigo,
  CRGB::Purple*/
};

int xy (int x, int y) {
  if (x == 1) {
    return HEIGHT - y;
  }
  if (x % 2 == 1) {
    return (x - 1) * HEIGHT + (HEIGHT - y);
  }
  return (x - 1) * HEIGHT + y - 1;
}

CRGB *xyLed (int x, int y) {
  return &leds[xy(x, y)];
}

void verticalLineCol (int x, int startY, int endY, CRGB::HTMLColorCode color) {
  for (int y = startY; y <= endY; y++) {
    *xyLed(x, y) = color;
  }
}

void drawHeart(CRGB::HTMLColorCode color, int centerX) {
  verticalLineCol(centerX-3, 4, 6, color);
  verticalLineCol(centerX-2, 3, 7, color);
  verticalLineCol(centerX-1, 2, 7, color);
  verticalLineCol(centerX, 1, 6, color);
  verticalLineCol(centerX+1, 2, 7, color);
  verticalLineCol(centerX+2, 3, 7, color);
  verticalLineCol(centerX+3, 4, 6, color);
}

void drawPendo(CRGB::HTMLColorCode color, int centerX) {
  verticalLineCol(centerX-3, 4, 4, color);
  verticalLineCol(centerX-2, 4, 5, color);
  verticalLineCol(centerX-1, 4, 6, color);
  verticalLineCol(centerX, 1, 7, color);
  verticalLineCol(centerX+1, 2, 7, color);
  verticalLineCol(centerX+2, 3, 7, color);
  verticalLineCol(centerX+3, 4, 7, color);
}

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
  for (int i = 8; i < 58; i++) {
    unsigned char col = getColFromMessage(getFromWindow(i));
    for (int j = 0; j < 5; j++) {
      unsigned char mask = 1;
      int maskShift = 4 - j;
      mask <<= maskShift;
      if (col & mask) {
        *xyLed(i+1, j+2) = colors[(getFromWindow(i) / 6) % NUM_COLORS];
      }
    }
  }
}

void drawLetter(int startX, CRGB::HTMLColorCode color) {
}

#define BLINK_RATE 4

void blinkHeart() {
  static int blink_frame_heart = 0;
  static boolean blink_heart = false;
  static boolean green = true;
  if (blink_frame_heart++ == BLINK_RATE) {
    blink_frame_heart = 0;
    blink_heart = !blink_heart;
    if (!blink_heart) {
      green = !green;
    }
  }
  if (!blink_heart) {
    //drawHeart(CRGB::Red, 4);
    if (green) {
      drawHeart(CRGB::Green, 4);
    } else {
      drawHeart(CRGB::Red, 4);
    }
  }
}

void blinkPendo() {
  static int blink_frame_pendo = 0;
  static boolean blink_pendo = true;
  static boolean green = true;
  if (blink_frame_pendo++ == BLINK_RATE) {
    blink_frame_pendo = 0;
    blink_pendo = !blink_pendo;
    if (!blink_pendo) {
      green = !green;
    }
  }
  if (!blink_pendo) {
    if (green) {
    drawPendo(CRGB::Green, 61);
    } else {
      drawPendo(CRGB::HotPink, 61);
    }
  }
}

void loop()
{
  slideWindow();
  FastLED.clearData();
  drawWindow();
  blinkHeart();
  blinkPendo();
  FastLED.show();
  delay(REFRESH_SPEED);
}

void setup() {
  Serial.begin(9600);
  FastLED.addLeds<CHIPSET, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalSMD5050);
  FastLED.setBrightness( BRIGHTNESS );
}