#include <Wire.h>

// The 24xx256 is a 256 kbit (32 KiB) EEPROM. It is addressed with a 16 bit
// word address and listens on 0b1010_000, as its A0-A2 pins are tied low.
#define EEPROM_ADDR 0x50
#define EEPROM_SIZE 32768

#define DUMP_START 0
#define DUMP_BYTES 64
#define BYTES_PER_LINE 16

bool eeprom_read(uint16_t addr, uint8_t *buf, uint8_t len) {
  Wire.beginTransmission(EEPROM_ADDR);
  Wire.write((uint8_t)(addr >> 8));
  Wire.write((uint8_t)(addr & 0xff));
  if (Wire.endTransmission() != 0) {
    return false;
  }

  if (Wire.requestFrom((uint8_t)EEPROM_ADDR, len) != len) {
    return false;
  }

  for (uint8_t i = 0; i < len; i++) {
    buf[i] = Wire.read();
  }

  return true;
}

void print_hex(uint16_t value, uint8_t digits) {
  for (uint8_t i = digits; i > 1; i--) {
    if (value < ((uint16_t)1 << (4 * (i - 1)))) {
      Serial.print('0');
    }
  }
  Serial.print(value, HEX);
}

void dump_line(uint16_t addr, const uint8_t *buf, uint8_t len) {
  print_hex(addr, 4);
  Serial.print("  ");

  for (uint8_t i = 0; i < len; i++) {
    print_hex(buf[i], 2);
    Serial.print(' ');
  }

  Serial.print(" |");
  for (uint8_t i = 0; i < len; i++) {
    Serial.print((buf[i] >= 0x20 && buf[i] < 0x7f) ? (char)buf[i] : '.');
  }
  Serial.println('|');
}

void setup() {
  Serial.begin(9600);
  Wire.begin();

  Serial.println("24xx256 EEPROM dump");

  uint8_t buf[BYTES_PER_LINE];
  for (uint16_t offset = 0; offset < DUMP_BYTES; offset += BYTES_PER_LINE) {
    uint16_t addr = (DUMP_START + offset) % EEPROM_SIZE;
    if (!eeprom_read(addr, buf, BYTES_PER_LINE)) {
      Serial.print("read failed at 0x");
      print_hex(addr, 4);
      Serial.println();
      return;
    }

    dump_line(addr, buf, BYTES_PER_LINE);
  }
}

void loop() {
}
