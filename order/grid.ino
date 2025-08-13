#define ROWS_PER_COL_SINGLE 8
#define COLS_PER_ROW_SINGLE 32
#define LEDS_PER_BOARD (ROWS_PER_COL_SINGLE * COLS_PER_ROW_SINGLE)

#define COL_SIZE 16
#define ROW_SIZE 32

#define UPSIDE_DOWN 0

#if UPSIDE_DOWN

int xy (int x, int y) {
  if (y >= 8) {
    
  }
}

#else

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

#endif

CRGB *xyLed (int x, int y) {
  return &leds[xy(x, y)];
}

CRGB *xyLed (int i) {
  return &leds[i];
}