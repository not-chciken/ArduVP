#include <SPI.h>

// Drives an 8-digit 7-segment display (or 8x8 LED matrix) with a MAX7219
// using the hardware SPI. LOAD/CS is on pin 10 (SS).
//
// Every write is a 16-bit frame (register address, data) latched on the
// rising edge of LOAD. On ArduVP, the MAX7219 is the first device of an SPI
// daisy chain with a MAX31855 behind it; since the MAX7219 latches the last
// 16 bits clocked out, plain 16-bit frames work and the MAX31855 just sees an
// aborted read.
//
// Each step is announced on the serial port before the display changes.

const uint8_t CS_PIN = 10;

const uint8_t REG_DIGIT0 = 0x01;
const uint8_t REG_DECODE_MODE = 0x09;
const uint8_t REG_INTENSITY = 0x0a;
const uint8_t REG_SCAN_LIMIT = 0x0b;
const uint8_t REG_SHUTDOWN = 0x0c;
const uint8_t REG_DISPLAY_TEST = 0x0f;

const uint8_t CODE_B_BLANK = 0x0f;
const uint8_t DECIMAL_POINT = 0x80;

const uint8_t NUM_DIGITS = 8;

void writeRegister(uint8_t reg, uint8_t data) {
  SPI.beginTransaction(SPISettings(4000000, MSBFIRST, SPI_MODE0));
  digitalWrite(CS_PIN, LOW);
  SPI.transfer(reg);
  SPI.transfer(data);
  digitalWrite(CS_PIN, HIGH);
  SPI.endTransaction();
}

void step(const char* name) {
  Serial.println(name);
  Serial.flush();  // Keep the serial output ahead of the display changes.
}

void setup() {
  Serial.begin(9600);
  Serial.println("MAX7219 demo");

  pinMode(CS_PIN, OUTPUT);
  digitalWrite(CS_PIN, HIGH);
  SPI.begin();

  step("Init");
  writeRegister(REG_DISPLAY_TEST, 0);
  writeRegister(REG_SCAN_LIMIT, NUM_DIGITS - 1);
  writeRegister(REG_DECODE_MODE, 0xff);
  writeRegister(REG_INTENSITY, 0x08);
  for (uint8_t digit = 0; digit < NUM_DIGITS; digit++)
    writeRegister(REG_DIGIT0 + digit, CODE_B_BLANK);
  writeRegister(REG_SHUTDOWN, 1);

  // Digit 0 is the rightmost digit.
  step("Code B: 1234.5678");
  for (uint8_t digit = 0; digit < NUM_DIGITS; digit++) {
    uint8_t code = NUM_DIGITS - digit;
    if (digit == 4)
      code |= DECIMAL_POINT;
    writeRegister(REG_DIGIT0 + digit, code);
  }

  step("Scan limit: 4 digits");
  writeRegister(REG_SCAN_LIMIT, 3);

  step("No decode: diagonal");
  writeRegister(REG_SCAN_LIMIT, NUM_DIGITS - 1);
  writeRegister(REG_DECODE_MODE, 0x00);
  for (uint8_t digit = 0; digit < NUM_DIGITS; digit++)
    writeRegister(REG_DIGIT0 + digit, 1 << digit);

  step("Display test");
  writeRegister(REG_DISPLAY_TEST, 1);

  step("Display test off");
  writeRegister(REG_DISPLAY_TEST, 0);

  step("Shutdown");
  writeRegister(REG_SHUTDOWN, 0);

  step("Done");
}

void loop() {
}
