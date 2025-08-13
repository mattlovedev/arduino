#include <FastLED.h>
#include <stdlib.h>

#define LED_PIN  2

#define COLOR_ORDER GRB
#define CHIPSET     WS2812B

#define BRIGHTNESS 128

#define WIDTH 64
#define HEIGHT 8

#define REFRESH_SPEED 50
#define RESET_TIME 1000
#define STUCK_TIME 1000

#define NUM_LEDS (WIDTH * HEIGHT)
CRGB leds[NUM_LEDS];

int xy (int x, int y) {
  if (x % 2 == 0) {
    return x * HEIGHT + HEIGHT - y - 1;
  } else {
    return x * HEIGHT + y;
  }
  return 0;
}

CRGB *xyLed (int x, int y) {
  return &leds[xy(x, y)];
}

#define NONE 0
#define LEFT 1
#define UP 2
#define RIGHT 3
#define DOWN 4

struct point {
  int x;
  int y;
};

void drawPoint(struct point p, CRGB::HTMLColorCode color) {
  *xyLed(p.x, p.y) = color;
}

void drawPointTrans(struct point p, CRGB::HTMLColorCode color, int x) {
  *xyLed(p.x + x, p.y) = color;
}

struct snek {
  point *body;
  int len;
  int cap;
  int dir;
};

snek snek = { 0 };

void newSnek() {
  if (snek.body != NULL) {
    free(snek.body); // TODO prob could realloc instead of free/calloc
  }
  snek.body = calloc(10, sizeof(*snek.body));
  snek.len = 1;
  snek.cap = 10;
  snek.dir = NONE;
}

void snekDraw() {
  for (int i = 0; i < snek.len; i++) {
    drawPoint(snek.body[i], CRGB::Green);
  }
}

// same as calling below with 1
bool snekContains(point p) {
  // len - 1 so we don't count the tail which is above to be replaced by head
  for (int i = 0; i < snek.len - 1; i++) {
    if (snek.body[i].x == p.x && snek.body[i].y == p.y) {
      return true;
    }
  }
  return false;
}

bool nSnekContains(point p, int n) {
  for (int i = 0; i < snek.len - n; i++) {
    if (snek.body[i].x == p.x && snek.body[i].y == p.y) {
      return true;
    }
  }
  return false;
}

void snekMove(point head) {
  memmove(&snek.body[1], &snek.body[0], sizeof(*snek.body) * snek.len - 1);
  snek.body[0] = head;
}

void snekFeed(point head) {
  if (snek.len == snek.cap) {
    snek.cap *= 2;
    snek.body = realloc(snek.body, snek.cap);
  }
  memmove(&snek.body[1], &snek.body[0], sizeof(*snek.body) * snek.len);
  snek.body[0] = head;  
  snek.len++;
}

point newFood() {
  int x = random(WIDTH);
  int y = random(HEIGHT);
  while (snekContains(point{x, y})) {
    x = random(WIDTH);
    y = random(HEIGHT);
  }
  return point{x, y};
}

point food = newFood();

void foodDraw() {
  drawPoint(food, CRGB::Red);
}

point coordsForDir(point p, int dir) {
  switch (dir) {
  case LEFT:
    if (--p.x == -1) {
      p.x = WIDTH - 1;
    }
    break;
  case UP:
    if (++p.y == HEIGHT) {
      p.y = 0;
    }
    break;
  case RIGHT:
    if (++p.x == WIDTH) {
      p.x = 0;
    }
    break;
  case DOWN:
    if (--p.y == -1) {
      p.y = HEIGHT - 1;
    }
    break;
  }
  return p;
}

point nCoordsForDir(point p, int dir, int n) {
  switch (dir) {
  case LEFT:
    p.x = (p.x - n + WIDTH) % WIDTH;    
    break;
  case UP:
    p.y = (p.y + n) % HEIGHT;
    break;
  case RIGHT:
    p.x = (p.x + n) % WIDTH;    
    break;
  case DOWN:
    p.y = (p.y - n + HEIGHT) % HEIGHT;
    break;
  }
  return p;  
}

void drawNumPointLen(int start, point *points, int len, CRGB::HTMLColorCode color) {
  for (int i = 0; i < len; i++) {
    drawPointTrans(points[i], color, start);
  }
}

void drawNumPoint(int start, int n, CRGB::HTMLColorCode color) {
  if (n == 0) {
    point p[] = { {0,2}, {0,3}, {0,4}, {0,5}, {1,1}, {1,2}, {1,6}, {2,1}, {2,3}, {2,6}, {3,1}, {3,4}, {3,6}, {4,1}, {4,5}, {4,6}, {5,2}, {5,3}, {5,4}, {5,5} };
    drawNumPointLen(start, p, sizeof(p) / sizeof(point), color);    
  } else if (n == 1) {
    point p[] = { {1,1}, {1,4}, {2,1}, {2,5}, {3,1}, {3,2}, {3,3}, {3,4}, {3,5}, {3,6}, {4,1}, {5,1} };
    drawNumPointLen(start, p, sizeof(p) / sizeof(point), color); 
  } else if (n == 2) {
    point p[] = { {0,1}, {0,2}, {0, 5}, {1,1}, {1,3}, {1, 6}, {2,1}, {2,3}, {2, 6}, {3,1}, {3,3}, {3, 6}, {4,1}, {4,3}, {4, 6}, {5,1}, {5,4}, {5, 5} };
    drawNumPointLen(start, p, sizeof(p) / sizeof(point), color); 
  } else if (n == 3) {
    point p[] = { {0,2}, {0,5}, {1,1}, {1,6}, {2,1}, {2,6}, {3,1}, {3,4}, {3,6}, {4,1}, {4,4}, {4,6}, {5,2}, {5,3}, {5,5} };
    drawNumPointLen(start, p, sizeof(p) / sizeof(point), color); 
  } else if (n == 4) {
    point p[] = { {0,2}, {1,2}, {1,3}, {2,2}, {2,4}, {3,2}, {3,5}, {4,1}, {4,2}, {4,3}, {4,4}, {4,5}, {4,6}, {5,2} };
    drawNumPointLen(start, p, sizeof(p) / sizeof(point), color); 
  } else if (n == 5) {
    point p[] = { {0,2}, {0,4}, {0,5}, {0,6}, {1,1}, {1,4}, {1,6}, {2,1}, {2,4}, {2,6}, {3,1}, {3,4}, {3,6}, {4,1}, {4,4}, {4,6}, {5,2}, {5,3}, {5,6} };
    drawNumPointLen(start, p, sizeof(p) / sizeof(point), color); 
  } else if (n == 6) {
    point p[] = { {0,2}, {0,3}, {0,4}, {0,5}, {1,1}, {1,4}, {1,6}, {2,1}, {2,4}, {2,6}, {3,1}, {3,4}, {3,6}, {4,1}, {4,4}, {4,6}, {5,2}, {5,3} };
    drawNumPointLen(start, p, sizeof(p) / sizeof(point), color); 
  } else if (n == 7) {
    point p[] = { {0,6}, {1,6}, {2,1}, {2,2}, {2,6}, {3,3}, {3,6}, {4,4}, {4,6}, {5,5}, {5,6} };
    drawNumPointLen(start, p, sizeof(p) / sizeof(point), color); 
  } else if (n == 8) {
    point p[] = { {0,2}, {0,3}, {0,5}, {1,1}, {1,4}, {1,6}, {2,1}, {2,4}, {2,6}, {3,1}, {3,4}, {3,6}, {4,1}, {4,4}, {4,6}, {5,2}, {5,3}, {5,5} };
    drawNumPointLen(start, p, sizeof(p) / sizeof(point), color); 
  } else if (n == 9) {
    point p[] = { {0,4}, {0,5}, {1,1}, {1,3}, {1,6}, {2,1}, {2,3}, {2,6}, {3,1}, {3,3}, {3,6}, {4,1}, {4,3}, {4,6}, {5,2}, {5,3}, {5,4}, {5,5} };
    drawNumPointLen(start, p, sizeof(p) / sizeof(point), color); 
  }
}

void drawNum(int start, int num, CRGB::HTMLColorCode color) {
  int hundred = num / 100;
  if (hundred) {
    drawNumPoint(start, hundred, color);
  }

  int ten = (num % 100) / 10;
  drawNumPoint(start + 8, ten, color);

  int one = (num % 10);
  drawNumPoint(start + 16, one, color);
}


void showScores() {
  static int max = 0;
  CRGB::HTMLColorCode color = CRGB::Red;
  if (snek.len > max) {
    max = snek.len;
    color = CRGB::Green;
  } else if (snek.len == max) {
    color = CRGB::Blue;
  }
  for (int i = 0; i < 5; i++) {
    FastLED.clearData();
    if (i % 2 == 0) {
      drawNum(5, snek.len, color);
    }
    drawNum(37, max, CRGB::Blue);
    FastLED.show();
    delay(500);    
  }

}

void snekUpdate(int dir) {
  point head = coordsForDir(snek.body[0], dir);
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

struct choice {
  int dir;
  int n;
};

int cmpfunc (const void * a, const void * b) {
  choice ac = *((choice *) a);
  choice bc = *((choice *) b);
  return ac.n - bc.n;
}

int* snekChoices() {
    int nLeft = (snek.body[0].x - food.x + WIDTH) % WIDTH;
    int nRight = (food.x - snek.body[0].x + WIDTH) % WIDTH;
    int nUp = (food.y - snek.body[0].y + HEIGHT) % HEIGHT;
    int nDown = (snek.body[0].y - food.y + HEIGHT) % HEIGHT;
    choice dirs[4] = {
      { dir: LEFT, n: nLeft },
      { dir: RIGHT, n: nRight },
      { dir: UP, n: nUp },
      { dir: DOWN, n: nDown }
    };
    static int choices[4];
    qsort(&dirs, 4, sizeof(choice), cmpfunc);
    if (dirs[0].n == 0 && dirs[1].n == 0) {
      choices[0] = dirs[2].dir;
      choices[1] = dirs[3].dir;
      choices[2] = dirs[0].dir;
      choices[3] = dirs[1].dir;
      return choices;
    }
    for (int i = 0; i < 4; i++) {
      choices[i] = dirs[i].dir;
    }
    return choices;
}

bool isLoop(int dir) {
  // 2 ahead of a loop isnt what matters, it's once we've already made a loop that matters
  //if (nSnekContains(nCoordsForDir(snek.body[0], dir, 2), 2)) {
    //return true;
  //}
  return false;
}

// a loop too small
bool closedLoop(int dir) {
  return false;
}

int evalChoices(int choices[4]) {

  for (int i = 0; i < 4; i++) {
    if (snekContains(coordsForDir(snek.body[0], choices[i]))) {
      // this is how we know we're about to turn into or away from the loop
      // rather than leave it up to the next choice, we need to potentially reorder/skip over
      // compare i+1 with i+2, both of which would not contain snake, but i+1 could be closed loop
      continue;
    } else {
      return choices[i];
    }
  }
  /*for (int i = 0; i < 4; i++) {
    if (!snekContains(coordsForDir(snek.body[0], choices[i]))) {      
      return choices[i];
    }
  }*/
  return choices[0]; // get rekt
}

void loop() {
  snekUpdate(evalChoices(snekChoices()));
  FastLED.clearData();
  snekDraw();
  foodDraw();
  FastLED.show();
  delay(REFRESH_SPEED);
}

void setupRandom() {
  unsigned long seed = 0;
  for (int i=0; i<32; i++) {
    seed = seed | ((analogRead(A0) & 0x01) << i);
  }
  randomSeed(seed);
}

void testRandom() {
  drawNum(2, random(1000), CRGB::Green);
}

void setup() {
  Serial.begin(9600);
  //pinMode(A0, INPUT_PULLUP);
  //analogRead(A0);
  //pinMode(A0, INPUT);
  setupRandom();
  //randomSeed(analogRead(0));
  //delay(50);
  FastLED.addLeds<CHIPSET, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalSMD5050);
  FastLED.setBrightness( BRIGHTNESS );
  FastLED.clearData();
  newSnek();
  snekDraw();
  foodDraw();
  //testRandom();
  FastLED.show();
  delay(REFRESH_SPEED);
}
