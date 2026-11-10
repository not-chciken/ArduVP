#include <SPI.h>

// Reads the thermocouple and cold-junction temperature of a MAX31855 in a
// loop using the hardware SPI. The chip select is on pin 10 (SS).

const uint8_t CS_PIN = 10;

uint32_t readRaw() {
  uint32_t raw = 0;

  SPI.beginTransaction(SPISettings(4000000, MSBFIRST, SPI_MODE0));
  digitalWrite(CS_PIN, LOW);
  for (uint8_t i = 0; i < 4; i++) {
    raw = (raw << 8) | SPI.transfer(0x00);
  }
  digitalWrite(CS_PIN, HIGH);
  SPI.endTransaction();

  return raw;
}

void setup() {
  Serial.begin(9600);
  Serial.println("MAX31855 demo");

  pinMode(CS_PIN, OUTPUT);
  digitalWrite(CS_PIN, HIGH);
  SPI.begin();
}

void loop() {
  uint32_t raw = readRaw();

  if (raw & 0x00010000) {
    Serial.print("Fault:");
    if (raw & 0x4) Serial.print(" short to VCC");
    if (raw & 0x2) Serial.print(" short to GND");
    if (raw & 0x1) Serial.print(" open circuit");
    Serial.println();
  } else {
    // Bits 31..18: signed 14-bit thermocouple temperature in 0.25 C steps.
    int16_t thermocouple = (int16_t)(raw >> 16) >> 2;
    // Bits 15..4: signed 12-bit cold-junction temperature in 0.0625 C steps.
    int16_t internal = (int16_t)(raw & 0xffff) >> 4;

    Serial.print("Thermocouple: ");
    Serial.print(thermocouple * 0.25);
    Serial.println(" C");

    Serial.print("Internal: ");
    Serial.print(internal * 0.0625, 4);
    Serial.println(" C");
  }

  delay(100);
}
