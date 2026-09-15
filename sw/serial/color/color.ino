static const char MESSAGE[] = " It's a double rainbow all the way... ";

static const uint8_t LINES_PER_ARC = 5;

static const uint8_t ARC_OUTER = 30;
static const uint8_t ARC_BAND = 2;
static const uint8_t ARC_BANDS = 7;
static const uint8_t ARC_WIDTH = 2 * ARC_OUTER + 1;
static const uint8_t ARC_ROWS = ARC_OUTER / 2;

static const uint8_t BOW[ARC_BANDS][3] = {
  {255,   0,   0},  // red
  {255, 127,   0},  // orange
  {255, 255,   0},  // yellow
  {  0, 200,   0},  // green
  {  0, 100, 255},  // blue
  { 75,   0, 190},  // indigo
  {148,   0, 211},  // violet
};

static uint8_t phase = 0;

static void hueToRgb(uint8_t hue, uint8_t *r, uint8_t *g, uint8_t *b) {
  uint8_t sector = hue / 43;
  uint8_t offset = (hue % 43) * 6;

  switch (sector) {
    case 0:  *r = 255;          *g = offset;       *b = 0;            break;
    case 1:  *r = 255 - offset; *g = 255;          *b = 0;            break;
    case 2:  *r = 0;            *g = 255;          *b = offset;       break;
    case 3:  *r = 0;            *g = 255 - offset; *b = 255;          break;
    case 4:  *r = offset;       *g = 0;            *b = 255;          break;
    default: *r = 255;          *g = 0;            *b = 255 - offset; break;
  }
}

static void setForeground(uint8_t r, uint8_t g, uint8_t b) {
  Serial.print("\033[38;2;");
  Serial.print(r);
  Serial.print(';');
  Serial.print(g);
  Serial.print(';');
  Serial.print(b);
  Serial.print('m');
}

static void setBackground(uint8_t r, uint8_t g, uint8_t b) {
  Serial.print("\033[48;2;");
  Serial.print(r);
  Serial.print(';');
  Serial.print(g);
  Serial.print(';');
  Serial.print(b);
  Serial.print('m');
}

static void printRainbow(const char *text, uint8_t start) {
  size_t len = strlen(text);

  for (size_t i = 0; i < len; i++) {
    uint8_t r, g, b;
    hueToRgb(start + (uint8_t)((i * 256) / len), &r, &g, &b);
    setForeground(r, g, b);
    Serial.print(text[i]);
  }

  Serial.println("\033[0m");
}

static uint8_t isqrt16(uint16_t n) {
  uint16_t r = 0, rem = 0;

  for (uint8_t i = 0; i < 8; i++) {
    rem = (rem << 2) | (n >> 14);
    n <<= 2;
    r <<= 1;
    if (rem > 2 * r) {
      rem -= 2 * r + 1;
      r += 1;
    }
  }

  return (uint8_t)r;
}

static int8_t bandAt(int16_t dx, int16_t dy) {
  uint8_t d = isqrt16((uint16_t)(dx * dx + dy * dy));

  if (d >= ARC_OUTER) {
    return -1;
  }

  int8_t band = (int8_t)((ARC_OUTER - 1 - d) / ARC_BAND);
  return (band < (int8_t)ARC_BANDS) ? band : -1;
}

static void drawArc(void) {
  int16_t cx = ARC_WIDTH / 2;

  for (uint8_t y = 0; y < ARC_ROWS; y++) {
    int16_t dy = (int16_t)(ARC_ROWS - 1 - y) * 2;
    int8_t current = -1;

    for (uint8_t x = 0; x < ARC_WIDTH; x++) {
      int8_t band = bandAt((int16_t)x - cx, dy);

      if (band != current) {
        if (band < 0) {
          Serial.print("\033[0m");
        } else {
          setBackground(BOW[band][0], BOW[band][1], BOW[band][2]);
        }
        current = band;
      }

      Serial.print(' ');
    }

    Serial.println("\033[0m");
  }
}

void setup() {
  Serial.begin(9600);
}

void loop() {
  for (uint8_t i = 0; i < LINES_PER_ARC; i++) {
    printRainbow(MESSAGE, phase);
    phase += 7;
    delay(100);
  }

  drawArc();
  drawArc();
  Serial.println("");
  delay(500);
}
