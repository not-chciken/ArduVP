#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>

#include "images.h"

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

const unsigned char* const IMAGES[] = { RICK1_BITMAP, RICK2_BITMAP };
const uint8_t NUM_IMAGES = sizeof(IMAGES) / sizeof(IMAGES[0]);

const uint8_t FADE_STEP = 5;
const unsigned long FADE_DELAY = 10;
const unsigned long HOLD_DELAY = 1000;

uint8_t current = 0;

void setContrast(uint8_t contrast) {
  display.ssd1306_command(SSD1306_SETCONTRAST);
  display.ssd1306_command(contrast);
}

void setup() {
  Serial.begin(9600);
  Serial.println(F("Starting SSD1306 demo"));

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ;
  }
}

void loop() {
  setContrast(0);
  display.clearDisplay();
  display.drawBitmap(0, 0, IMAGES[current], SCREEN_WIDTH, SCREEN_HEIGHT, WHITE);
  display.display();

  // Blend in.
  digitalWrite(LED_BUILTIN, HIGH);
  for (int c = 0; c <= 255; c += FADE_STEP) {
    setContrast(c);
    delay(FADE_DELAY);
  }

  delay(HOLD_DELAY);

  // Blend out.
  digitalWrite(LED_BUILTIN, LOW);
  for (int c = 255; c >= 0; c -= FADE_STEP) {
    setContrast(c);
    delay(FADE_DELAY);
  }

  current = (current + 1) % NUM_IMAGES;
}
