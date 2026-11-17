#include <SPI.h>

// Scrolls "VCML" endlessly across an 8x8 LED matrix driven by a MAX7219
// using the hardware SPI. LOAD/CS is on pin 10 (SS).
//
// Code B decoding only knows digits and a few letters, so the MAX7219 runs
// in no-decode mode and every digit register holds one row of the matrix:
// row r is digit register r, column c is data bit c, i.e. LED r * 8 + c.
// The wiring of real 8x8 modules differs. If one shows the text mirrored,
// use (0x80 >> col) instead of (1 << col) in showWindow().

const uint8_t CS_PIN = 10;

const uint8_t REG_DIGIT0 = 0x01;
const uint8_t REG_DECODE_MODE = 0x09;
const uint8_t REG_INTENSITY = 0x0a;
const uint8_t REG_SCAN_LIMIT = 0x0b;
const uint8_t REG_SHUTDOWN = 0x0c;
const uint8_t REG_DISPLAY_TEST = 0x0f;

const uint8_t NUM_ROWS = 8;
const uint8_t NUM_COLS = 8;

const unsigned long SCROLL_DELAY_MS = 80;

// 5x7 font, one byte per column from left to right, bit 0 is the top row.
const uint8_t GLYPH_WIDTH = 5;

struct Glyph {
  char character;
  uint8_t columns[GLYPH_WIDTH];
};

const Glyph FONT[] = {
    {'C', {0x3e, 0x41, 0x41, 0x41, 0x22}},
    {'L', {0x7f, 0x40, 0x40, 0x40, 0x40}},
    {'M', {0x7f, 0x02, 0x0c, 0x02, 0x7f}},
    {'V', {0x1f, 0x20, 0x40, 0x20, 0x1f}},
};

const char TEXT[] = "VCML";
const uint8_t TEXT_LENGTH = sizeof(TEXT) - 1;

// Each glyph is followed by one blank column, and the text by GAP more
// blank columns before it repeats.
const uint8_t GAP = 2;
const uint8_t TEXT_COLUMNS = TEXT_LENGTH * (GLYPH_WIDTH + 1) + GAP;

uint8_t columns[TEXT_COLUMNS];
uint8_t offset = 0;

void writeRegister(uint8_t reg, uint8_t data) {
  SPI.beginTransaction(SPISettings(4000000, MSBFIRST, SPI_MODE0));
  digitalWrite(CS_PIN, LOW);
  SPI.transfer(reg);
  SPI.transfer(data);
  digitalWrite(CS_PIN, HIGH);
  SPI.endTransaction();
}

const Glyph* findGlyph(char character) {
  for (const Glyph& glyph : FONT)
    if (glyph.character == character)
      return &glyph;
  return nullptr;
}

void renderText() {
  uint8_t column = 0;
  for (uint8_t i = 0; i < TEXT_LENGTH; i++) {
    const Glyph* glyph = findGlyph(TEXT[i]);
    for (uint8_t x = 0; x < GLYPH_WIDTH; x++)
      columns[column++] = glyph ? glyph->columns[x] : 0;
    columns[column++] = 0;
  }
  // The remaining GAP columns stay blank.
}

// Shows the NUM_COLS columns of the text starting at `start`, wrapping
// around at the end of the text.
void showWindow(uint8_t start) {
  for (uint8_t row = 0; row < NUM_ROWS; row++) {
    uint8_t bits = 0;
    for (uint8_t col = 0; col < NUM_COLS; col++)
      if (columns[(start + col) % TEXT_COLUMNS] & (1 << row))
        bits |= 1 << col;
    writeRegister(REG_DIGIT0 + row, bits);
  }
}

void setup() {
  Serial.begin(9600);
  Serial.println("MAX7219 text scroll demo");
  Serial.flush();  // Keep the serial output ahead of the display changes.

  pinMode(CS_PIN, OUTPUT);
  digitalWrite(CS_PIN, HIGH);
  SPI.begin();

  writeRegister(REG_DISPLAY_TEST, 0);
  writeRegister(REG_SCAN_LIMIT, NUM_ROWS - 1);
  writeRegister(REG_DECODE_MODE, 0x00);
  writeRegister(REG_INTENSITY, 0x08);
  for (uint8_t row = 0; row < NUM_ROWS; row++)
    writeRegister(REG_DIGIT0 + row, 0);
  writeRegister(REG_SHUTDOWN, 1);

  renderText();
}

void loop() {
  showWindow(offset);
  offset = (offset + 1) % TEXT_COLUMNS;
  delay(SCROLL_DELAY_MS);
}
