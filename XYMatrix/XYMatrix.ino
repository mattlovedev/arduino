#include <FastLED.h>

#define LED_PIN  2

#define COLOR_ORDER GRB
#define CHIPSET     WS2812B

#define BRIGHTNESS 64

#define REFRESH_SPEED 50

#define WIDTH 32
#define HEIGHT 8

#define NUM_LEDS (WIDTH * HEIGHT)
CRGB leds[NUM_LEDS];

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

int xy (int x, int y) {
  if (x == 1) return HEIGHT - y;
  if (x % 2 == 1) return (x - 1) * HEIGHT + (HEIGHT - y);
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

void verticalLine (int x, int startY, int endY) {
  for (int y = startY; y <= endY; y++) {
    *xyLed(x, y) = colors[y-1];
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

void scroll() {
  CRGB columnBuffer[HEIGHT];
  for (int y = 1; y <= HEIGHT; y++) {
    memcpy(&columnBuffer[y-1], xyLed(WIDTH,y), sizeof(CRGB));
  }
  for (int x = WIDTH; x > 1; x--) {
    for (int y = 1; y <= HEIGHT; y++) {
      memcpy(xyLed(x,y), xyLed(x-1,y), sizeof(CRGB));
    }
  }
  for (int y = 1; y <= HEIGHT; y++) {
    memcpy(xyLed(1,y), &columnBuffer[y-1], sizeof(CRGB));
  }
}

CRGB::HTMLColorCode q_colors[WIDTH];
int q_vals[WIDTH];
int q_idx = 0;

void q_add_level(int level, CRGB::HTMLColorCode color) {
  if (++q_idx == WIDTH) {
    q_idx = 0;
  }
  q_vals[q_idx] = level;
  q_colors[q_idx] = color;
}

int q_from_idx(int idx) {
  if (q_idx < idx)
    return q_vals[(q_idx - idx + WIDTH) % WIDTH];
  return q_vals[q_idx - idx];
}

CRGB::HTMLColorCode color_from_idx(int idx) {
  if (q_idx < idx)
    return q_colors[(q_idx - idx + WIDTH) % WIDTH];
  return q_colors[q_idx - idx];
}

void draw_wave() {
  for (int i = 0; i < WIDTH; i++) {
    //verticalLineCol(i+1, 1, q_from_idx(i), color_from_idx(i));
    //verticalLine(i+1, 1, q_from_idx(i));
    *xyLed(i+1, q_from_idx(i)) = color_from_idx(i);
  }
}

void addRandom() {
  //q_add_level(rand() % 8 + 1, globalColor);
}

int last = 5;
void addSmoothed() {
  //int r = rand() % 5;
  int r = random(5);
  if (r != 0) {
    if (last == 1) {
      last++;
    } else if (last == HEIGHT) {
      last--;
    } else {
      if (r % 2 == 1) {
        last++;
      } else { // r % 2 == 0
        last--;
      }
    }
  }
  q_add_level(last, colors[last-1]);
}

void blinkHeart() {
    #define BLINK_RATE 8
    static int blink_frame = 0;
    static boolean blink_heart = false;
    if (blink_frame++ == BLINK_RATE) {
      blink_frame = 0;
      blink_heart = !blink_heart;
    }
    if (!blink_heart) {
      //drawHeart(color_from_idx(6), 7);
      //drawHeart(color_from_idx(15), 16);
      //drawHeart(color_from_idx(24), 25);
    }
}

const int BUTTON_PIN_MODE_ONE = 4;
const int BUTTON_PIN_MODE_TWO = 2;

void checkButtons() {
  static int brightness_levels[] = { 0, 16, 32, 64, 128, 255 };
  static bool button_one_pressed_down = false;
  static bool button_two_pressed_down = false;
  static unsigned int brightness_idx = 1;
  // down
  if (digitalRead(BUTTON_PIN_MODE_ONE) == LOW) {
    button_one_pressed_down = true;
    // else if HIGH &&
  } else if (button_one_pressed_down) {
    FastLED.setBrightness( brightness_levels[ --brightness_idx % 6 ] );
    button_one_pressed_down = false;
  }
  // up
  if (digitalRead(BUTTON_PIN_MODE_TWO) == LOW) {
    button_two_pressed_down = true;
    // else if HIGH &&
  } else if (button_two_pressed_down) {
    FastLED.setBrightness( brightness_levels[ ++brightness_idx % 6 ] );
    button_two_pressed_down = false;
  }
}

void loop()
{
    //checkButtons();
  
    addSmoothed();
    FastLED.clearData();

    draw_wave();

    //blinkHeart();
    drawHeart(color_from_idx(15), 16);

    FastLED.show();
    delay(REFRESH_SPEED);
}

void setup() {
  Serial.begin(9600);
  pinMode(BUTTON_PIN_MODE_ONE, INPUT_PULLUP);
  pinMode(BUTTON_PIN_MODE_TWO, INPUT_PULLUP);
  FastLED.addLeds<CHIPSET, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalSMD5050);
  FastLED.setBrightness( BRIGHTNESS );
  //randomSeed(analogRead(0));
}

#if 0

// hook up the out of the mic to analog input A0
#define MIC_IN A1

// Sample window width in milliseconds (50 ms = 20Hz)
int sampleWindow = 50;

void loop() 
{
    // read the analog sensor as volts
    //double soundSensed = sampleSoundPeak();

    //Serial.println(soundSensed);
    
    // convert to volts
    //double volts = (soundSensed * 3.3) / 1024; 

   // map 1v p-p level to the max scale of the display
   //int displayPeak = map(soundSensed, 0, 25, 0, 8);

   //Serial.println(soundSensed);
   //static int last = 1;
   //if (displayPeak > last && last < HEIGHT) last++;
   //else if (displayPeak < last && last > 1) last--;
   //q_add_level(last, colors[last-1]);
   addSmoothed();
   FastLED.clearData();
   draw_wave();
   FastLED.show();
   delay(REFRESH_SPEED);
}

/**
 * Sense biggest input difference are being input from the analog MIC sensor
 * over a certain "window" of time. 
 * Values returned are in the range 0 - 1024.
 **/
double sampleSoundPeak()
{
    // record start time 
    double startMillis = millis(); 

    // this will be the highest peak, so start it very small    
    int signalMax = 0;
    
    // this will be the lowest peak, so start it very high
    int signalMin = 1024;
    
    // will hold the current value from the microphone
    int sample;
    
    // collect data for 50 ms
    while ( (millis() - startMillis) < sampleWindow ) 
    {
        // read a value from mic and record it into sample variable
        sample = analogRead( MIC_IN );
        
        // toss out spurious readings
        if (sample < 1024000)
        {
        
            // if the current sample is larger than the max
             if (sample > signalMax)
             {      
                   // this is the new max -- save it
                   signalMax = sample; 
             }
             // otherwise, if the current sample is smaller than the min
             else if (sample < signalMin)
             {
                   // this is the new min -- save it
                   signalMin = sample; 
             }
         }
     }
     
     // now that we've collected our data,
     // determine the peak-peak amplitude as max - min
     int peakDifference = signalMax - signalMin; 
    
     // give it back to the caller of this method
     return peakDifference;
}
#endif
