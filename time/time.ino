#include <TimeLib.h>
#include <FastLED.h>

#define LED_PIN  5
#define COLOR_ORDER GRB
#define CHIPSET     WS2812B
#define BRIGHTNESS 16
#define REFRESH_SPEED 50
#define WIDTH 32
#define HEIGHT 8
#define NUM_LEDS (WIDTH * HEIGHT)

CRGB leds[NUM_LEDS];

int xy (int x, int y) {
  if (x == 1) return HEIGHT - y;
  if (x % 2 == 1) return (x - 1) * HEIGHT + (HEIGHT - y);
  return (x - 1) * HEIGHT + y - 1;
}

CRGB *xyLed (int x, int y) {
  return &leds[xy(x, y)];
}

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

unsigned long NUMBERS[] ={
  33095281,
  19907185,
  31121009,
  22740593,
  7503473,
  24835697,
  33224305,
  1113713,
  33226353,
  7536241
};

unsigned long colon = 10240;
unsigned long dot = 16384;

#if 0
void loop() {
  //checkButtons();
  FastLED.clearData();
  drawTime();
  FastLED.show();
  delay(REFRESH_SPEED * 10);
}
#endif

const int BUTTON_PIN_MODE_ONE = 4;
const int BUTTON_PIN_MODE_TWO = 2;

// single character message tags
#define TIME_HEADER   'T'   // Header tag for serial time sync message
#define FORMAT_HEADER 'F'   // Header tag indicating a date format message
#define TIME_REQUEST  7     // ASCII bell character requests a time sync message 

static boolean isLongFormat = true;

void setup()  {
  Serial.begin(9600);
  //pinMode(BUTTON_PIN_MODE_ONE, INPUT_PULLUP);
  //pinMode(BUTTON_PIN_MODE_TWO, INPUT_PULLUP);
  FastLED.addLeds<CHIPSET, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalSMD5050);
  FastLED.setBrightness( BRIGHTNESS );
  
  setSyncProvider( requestSync);  //set function to call when sync required
  //Serial.println("Waiting for sync message");
}

void loop(){    
  if (Serial.available() > 1) { // wait for at least two characters
    char c = Serial.read();
    if( c == TIME_HEADER) {
      processSyncMessage();
    }
  }
  //Serial.print("timeStatus: ");
  //Serial.println(timeStatus());
  if (timeStatus()!= timeNotSet) {
    FastLED.clearData();
    drawTime();
    FastLED.show();
  }
  delay(50);
}

unsigned char getColFromNumber(int num, int col) {
  unsigned long number = NUMBERS[num];
  
  unsigned char c = 0;
  for (int i = 0; i < 5; i++ ) {
    c <<= 1;
    unsigned long mask = 1;
    int maskShift = (24 - (col * 5 + i));
    mask <<= maskShift;
    if (number & mask) {
      c |= 1;
    }
  }
  return c;
}

void drawNumber(int n, int startCol, CRGB::HTMLColorCode color) {
  for (int i = 0; i < 3; i++) {
    unsigned char col = getColFromNumber(n, i);
    for (int j = 0; j < 5; j++) {
      unsigned char mask = 1;
      int maskShift = 4 - j;
      mask <<= maskShift;
      if (col & mask) {
        *xyLed(startCol+i+1, j+2) = color;
      }
    }
  }
}

void drawTime() {
  int h = hour() % 12;
  drawNumber(h / 10, 0, CRGB::DarkRed);
  drawNumber(h % 10, 4, CRGB::DarkRed);
  *xyLed(9, 3) = CRGB::Yellow;
  *xyLed(9, 5) = CRGB::Yellow;
  int m = minute();
  drawNumber(m / 10, 10, CRGB::White);
  drawNumber(m % 10, 14, CRGB::White);
  *xyLed(19, 3) = CRGB::Yellow;
  *xyLed(19, 5) = CRGB::Yellow;
  int s = second();
  drawNumber(s / 10, 20, CRGB::MediumBlue);
  drawNumber(s % 10, 24, CRGB::MediumBlue);
}

void processSyncMessage() {
  unsigned long pctime;
  const unsigned long DEFAULT_TIME = 1357041600; // Jan 1 2013 - paul, perhaps we define in time.h?

   pctime = Serial.parseInt();
   //Serial.print("PCTIME: ");
   //Serial.println(pctime);
   if( pctime >= DEFAULT_TIME) { // check the integer is a valid time (greater than Jan 1 2013)
     setTime(pctime); // Sync Arduino clock to the time received on the serial port
   }
}

time_t requestSync() {
  Serial.write(TIME_REQUEST);  
  return 0; // the time will be sent later in response to serial mesg
}
