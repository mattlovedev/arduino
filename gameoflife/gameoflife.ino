#include <FastLED.h>

#define LED_PIN  2

#define COLOR_ORDER GRB
#define CHIPSET     WS2812B

#define BRIGHTNESS 128

#define WIDTH 64
#define HEIGHT 8

#define REFRESH_SPEED 100
#define RESET_TIME 1000
#define STUCK_TIME 1000

#define NUM_LEDS (WIDTH * HEIGHT)
CRGB leds[NUM_LEDS];

#define NUM_COLORS 8
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

#define UPSIDE_DOWN 0
#if UPSIDE_DOWN
int xy (int x, int y) {
  if (y >= 8) {
    
  }
}
#elif 0
int xy (int x, int y) {
  if (y < 8) {
    if (x % 2 == 0) return 256 + (31 - x) * 8 + (7 - y);
    return 256 + (31 - x) * 8 + y;
  }
  if (x % 2 == 0) return (x * 8) + (15 - y);
  return (x * 8) + (y - 8);
}
#else
int xy (int x, int y) {
  if (x % 2 == 0) {
    return x * HEIGHT + HEIGHT - y - 1;
  } else {
    return x * HEIGHT + y;
  }
  return 0;
}
#endif

CRGB *xyLed (int x, int y) {
  return &leds[xy(x, y)];
}



#define NORTH 0
#define NORTH_EAST 1
#define EAST 2
#define SOUTH_EAST 3
#define SOUTH 4
#define SOUTH_WEST 5
#define WEST 6
#define NORTH_WEST 7

#define INVALID_SPACE -1

int returnIfValid(int x, int y, int dir) {
  if (x == 0) {
    if (dir == NORTH_WEST || dir == WEST || dir == SOUTH_WEST) {
      x = WIDTH; //return INVALID_SPACE;
    }
  } else if (x == WIDTH - 1) {
    if (dir == NORTH_EAST || dir == EAST || dir == SOUTH_EAST) {
      x = -1; //return INVALID_SPACE;
    }
  }
  if (y == 0) {
    if (dir == SOUTH_EAST || dir == SOUTH || dir == SOUTH_WEST) {
      y = HEIGHT; //return INVALID_SPACE;
    }
  } else if (y == HEIGHT - 2) { //HEIGHT - 1) {
    if (dir == NORTH_EAST || dir == NORTH || dir == NORTH_WEST) {
      y = -1; //return INVALID_SPACE;
    }
  }
  switch (dir) {
  case NORTH:
    return xy(x, y + 1);
  case NORTH_EAST:
    return xy(x + 1, y + 1);
  case EAST:
    return xy(x + 1, y);
  case SOUTH_EAST:
  return xy(x + 1, y - 1);
  case SOUTH:
    return xy(x, y - 1);
  case SOUTH_WEST:
    return xy(x - 1, y - 1);
  case WEST:
    return xy(x - 1, y);
  case NORTH_WEST:
    return xy(x - 1, y + 1);
  }
  return xy(x, y); // should never happen
}

void setAlive(int x, int y) {
  *xyLed(x,y) = CRGB::Green;
}

void setDead(int x, int y) {
  *xyLed(x,y) = CRGB::Black;
}

void setDying(int x, int y) {
  *xyLed(x,y) = CRGB::Aqua;
}

void setBeingBorn(int x, int y) {
  *xyLed(x,y) = CRGB::Blue;
}

int numNeighbors(int x, int y) {
  int num = 0;
  for (int dir = NORTH; dir <= NORTH_WEST; dir++) {
    int neighbor = returnIfValid(x, y, dir);
    if (neighbor != INVALID_SPACE) {
      if (leds[neighbor].green > 0) {
        num++;
      }
    }
  }
  return num;
}

static int age = -1;
static int gens = 0;

void setAge() {
  if (gens % 100 == 0 && age < WIDTH - 1) {
    age++;
    gens = 0;
  }
  for (int x = 0; x < age; x++) {
    *xyLed(x, HEIGHT-1) = colors[0];
  }
  if (age == 10) {
    initialState();
  }
}

int deltaLife() {
  int changed = 0;
  int alive = 0;
  for (int x = 0; x < WIDTH; x++) {
    for (int y = 0; y < HEIGHT; y++) {
      if (xyLed(x,y)->green > 0) {
        int neighbors = numNeighbors(x, y);
        if (neighbors != 2 && neighbors != 3) {
          setDying(x,y);
          changed++;
        } else {
          alive++;
        }
      } else { // *xyLed(x, y) == CRGB::Black // if is dead
        if (numNeighbors(x, y) == 3) {
          setBeingBorn(x,y);
          changed++;
          alive++;
        }
      }
    }
  }
  gens++;
  
  return changed;
  //return alive;
}

void killAllLife() {
  for (int x = 0; x < WIDTH; x++) {
    for (int y = 0; y < HEIGHT; y++) {
      *xyLed(x,y) = CRGB::Black;
    }
  }
}

void drawLivingLife() {
  for (int x = 0; x < WIDTH; x++) {
    for (int y = 0; y < HEIGHT; y++) {
      CRGB *led = xyLed(x,y);
      if (led->blue > 0) { 
        if (led->green > 0) {
          *xyLed(x, y) = CRGB::Black; // dying -> dead
        } else {
          *xyLed(x, y) = CRGB::Green; // being born -> alive
        }
      }
    }
  }
  setAge();
  FastLED.show();
}

#define HIST_SIZE 50
static int history[HIST_SIZE];
static int nextH = 1;

void drawLife() {
  int delta = deltaLife();
  if (delta == 0) {
    initialState();
    drawLivingLife();
    delay(REFRESH_SPEED);
    return;
  }
  history[nextH] = delta;
  nextH = (nextH + 1) % HIST_SIZE;

  bool stuck = true;
  for (int i = 1; i < HIST_SIZE; i++) {
    if (history[i] != history[0]) {
      stuck = false;
      break;
    }
  }
  //stuck = false;
  
  if (stuck) {
    for (int x = 0; x < WIDTH; x++) {
      for (int y = 0; y < HEIGHT-1; y++) {
        if (xyLed(x,y)->green > 0 && xyLed(x,y)->blue == 0) {
          *xyLed(x, y) = CRGB::Red; // stuck
        }
      }
    }
    FastLED.show();
    delay(STUCK_TIME);
    initialState();
  } else {
    drawLivingLife();
    delay(REFRESH_SPEED);
  }
}




void loop()
{
  drawLife();
  //FastLED.clearData();
  //move();
  
  //*xyLed(snek.x, snek.y) = CRGB::Green;
  //FastLED.show();
  //delay(REFRESH_SPEED);
  /*for (int y = 0; y < HEIGHT; y++) {
    for (int x = 0; x < WIDTH; x++) {
      *xyLed(x, y) = CRGB::Green;
      FastLED.show();
      *xyLed(x, y) = CRGB::Black;
      delay(REFRESH_SPEED);      
    }
  }*/
}

void randomInitialState() {
  int odds = 30; //random(100); // i want this percent chance so if 70 then 70/100
  for (int x = 0; x < WIDTH; x++) {
    for (int y = 0; y < HEIGHT; y++) {
      if (random(100) <= odds) {
        setAlive(x,y);
      }
    }
  }
}

void drawGlider(int x, int y) {
  setAlive(x+0,y+0);  setAlive(x+1,y+0);
  setAlive(x+2,y+0);  setAlive(x+2,y+1);
  setAlive(x+1,y+2);
}

void drawLWSS(int x, int y) {
  setAlive(x+0,y+0);  setAlive(x+1,y+0);
  setAlive(x+2,y+0);  setAlive(x+3,y+0);
  setAlive(x+0,y+1);  setAlive(x+4,y+1);
  setAlive(x+0,y+2);  setAlive(x+1,y+3);
  setAlive(x+4,y+3);
}

void drawPenta(int x, int y) {
  setAlive(x+0,y+1);  setAlive(x+1,y+1);
  setAlive(x+2,y+0);  setAlive(x+2,y+2);
  setAlive(x+3,y+1);  setAlive(x+4,y+1);
  setAlive(x+5,y+1);  setAlive(x+6,y+1);
  setAlive(x+7,y+0);  setAlive(x+7,y+2);
  setAlive(x+8,y+1);  setAlive(x+9,y+1);
}



void initialState() {
  FastLED.clearData();
  age = -1;
  gens = 0;
  history[0] = -1;
  killAllLife();
  randomInitialState();

  //setAlive(0, 1);
  //setAlive(1, 1);
  //setAlive(2, 1);
  //drawGlider(0,0);
  //drawLWSS(0,8);
  //drawPenta(10,3);
  
  drawLivingLife();
  delay(RESET_TIME);
}

void setup() {
  Serial.begin(9600);
  randomSeed(analogRead(0));
  FastLED.addLeds<CHIPSET, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalSMD5050);
  FastLED.setBrightness( BRIGHTNESS );
  FastLED.clearData();
  initialState();
}
