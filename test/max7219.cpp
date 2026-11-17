/******************************************************************************
 * Apache License, Version 2.0
 * Copyright (c) 2026 chciken/Niko Zurstraßen
 *****************************************************************************/

// Tests for the MAX7219 LED display driver, see tmp/max7219-max7221.pdf.
//
// LED pin digit * 8 + n is driven by data bit n of that digit, so a digit
// reads back as its register value: DP A B C D E F G = D7 ... D0.

#include <array>

#include "models/max7219.h"

#include "testing.h"

struct max7219_test : public test_base {
  static constexpr u8 REG_NOOP = 0x0;
  static constexpr u8 REG_DIGIT0 = 0x1;
  static constexpr u8 REG_DECODE_MODE = 0x9;
  static constexpr u8 REG_INTENSITY = 0xa;
  static constexpr u8 REG_SCAN_LIMIT = 0xb;
  static constexpr u8 REG_SHUTDOWN = 0xc;
  static constexpr u8 REG_DISPLAY_TEST = 0xf;
  static constexpr u8 DP = 0x80;

  // A single device without a chained device.
  spi::max7219 max;
  spi_initiator_socket spi_out;
  gpio_initiator_socket load;

  // A daisy chain MAX7219 -> MAX7219 -> MAX31855 with a shared chip select.
  spi::max7219 chain_first;
  spi::max7219 chain_second;
  spi::max31855 thermo;
  spi_initiator_socket chain_spi;
  gpio_initiator_socket chain_load;

  max7219_test(const sc_module_name& nm)
      : test_base(nm),
        max("max"),
        spi_out("spi_out"),
        load("load"),
        chain_first("chain_first"),
        chain_second("chain_second"),
        thermo("thermo"),
        chain_spi("chain_spi"),
        chain_load("chain_load") {
    spi_out.bind(max.spi_in);
    load.bind(max.cs);
    max.spi_out.stub();

    chain_spi.bind(chain_first.spi_in);
    chain_first.spi_out.bind(chain_second.spi_in);
    chain_second.spi_out.bind(thermo.spi_in);
    chain_load.bind(chain_first.cs);
    chain_load.bind(chain_second.cs);
    thermo.bind(chain_load, false);

    add_test("power_up", &max7219_test::test_power_up);
    add_test("ledcontrol_init", &max7219_test::test_ledcontrol_init);
    add_test("all_leds", &max7219_test::test_all_leds);
    add_test("address_decode", &max7219_test::test_address_decode);
    add_test("load_latch", &max7219_test::test_load_latch);
    add_test("dout", &max7219_test::test_dout);
    add_test("code_b", &max7219_test::test_code_b);
    add_test("scan_limit", &max7219_test::test_scan_limit);
    add_test("shutdown", &max7219_test::test_shutdown);
    add_test("display_test", &max7219_test::test_display_test);
    add_test("daisy_chain", &max7219_test::test_daisy_chain);
  }

  u8 shift(u8 byte) {
    spi_payload spi(byte);
    spi_out.transport(spi);
    return spi.miso;
  }

  void write(u8 addr, u8 data) {
    load = false;
    shift(addr);
    shift(data);
    load = true;
  }

  static u8 digit(spi::max7219& dev, size_t d) {
    u8 value = 0;
    for (size_t n = 0; n < 8; n++)
      if (dev.led_out[d * 8 + n].read())
        value |= 1u << n;
    return value;
  }

  u8 digit(size_t d) { return digit(max, d); }

  static u16 cmd(u8 reg, u8 data) { return (u16)((reg << 8) | data); }

  u32 chain_frame(u16 second, u16 first) {
    const u8 bytes[] = {(u8)(second >> 8), (u8)second, (u8)(first >> 8), (u8)first};
    u32 miso = 0;
    chain_load = false;
    for (u8 byte : bytes) {
      spi_payload spi(byte);
      chain_spi.transport(spi);
      miso = (miso << 8) | (spi.miso & 0xff);
    }
    chain_load = true;
    return miso;
  }

  void expect_digits(const std::array<u8, 8>& expected) {
    for (size_t d = 0; d < 8; d++)
      EXPECT_EQ(digit(d), expected[d]) << "digit " << d;
  }

  void expect_all(bool on) { expect_digits(on ? std::array<u8, 8>{0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff} : std::array<u8, 8>{}); }

  void ledcontrol_init() {
    write(REG_DISPLAY_TEST, 0);
    write(REG_SCAN_LIMIT, 7);
    write(REG_DECODE_MODE, 0);
    for (u8 d = 0; d < 8; d++)
      write(REG_DIGIT0 + d, 0);
    write(REG_SHUTDOWN, 1);
  }

  void test_power_up() {
    expect_all(false);
    write(REG_DIGIT0, 0xff);
    expect_all(false);
  }

  void test_ledcontrol_init() {
    ledcontrol_init();
    expect_all(false);

    write(REG_DIGIT0 + 3, 0b10100101);
    expect_digits({0, 0, 0, 0b10100101, 0, 0, 0, 0});
  }

  void test_all_leds() {
    ledcontrol_init();
    for (size_t n = 0; n < 64; n++) {
      write(REG_DIGIT0 + n / 8, 1u << (n % 8));
      for (size_t i = 0; i < 64; i++)
        ASSERT_EQ(max.led_out[i].read(), i == n) << "writing led " << n << ", checking led " << i;
      write(REG_DIGIT0 + n / 8, 0);
    }
  }

  void test_address_decode() {
    ledcontrol_init();

    write(0xf0 | (REG_DIGIT0 + 7), 0x42);
    expect_digits({0, 0, 0, 0, 0, 0, 0, 0x42});

    write(REG_NOOP, 0xff);
    write(0xa0 | REG_NOOP, 0xff);
    expect_digits({0, 0, 0, 0, 0, 0, 0, 0x42});

    write(REG_INTENSITY, 0x0f);
    expect_digits({0, 0, 0, 0, 0, 0, 0, 0x42});
  }

  void test_load_latch() {
    ledcontrol_init();

    load = false;
    shift(REG_DIGIT0);
    shift(0x11);
    expect_all(false);
    load = true;
    expect_digits({0x11, 0, 0, 0, 0, 0, 0, 0});

    load = false;
    shift(REG_DIGIT0 + 1);
    shift(0x22);
    shift(REG_DIGIT0 + 2);
    shift(0x33);
    load = true;
    expect_digits({0x11, 0, 0x33, 0, 0, 0, 0, 0});
  }

  void test_dout() {
    load = false;
    shift(0x12);
    shift(0x34);
    EXPECT_EQ(shift(0x00), 0x12);
    EXPECT_EQ(shift(0x00), 0x34);
    EXPECT_EQ(shift(0x00), 0x00);
    load = true;
  }

  void test_code_b() {
    static const u8 font[16] = {
        0b1111110,  // 0
        0b0110000,  // 1
        0b1101101,  // 2
        0b1111001,  // 3
        0b0110011,  // 4
        0b1011011,  // 5
        0b1011111,  // 6
        0b1110000,  // 7
        0b1111111,  // 8
        0b1111011,  // 9
        0b0000001,  // -
        0b1001111,  // E
        0b0110111,  // H
        0b0001110,  // L
        0b1100111,  // P
        0b0000000,  // blank
    };

    ledcontrol_init();
    write(REG_DECODE_MODE, 0xff);
    for (u8 code = 0; code < 16; code++) {
      write(REG_DIGIT0, code);
      EXPECT_EQ(digit(0), font[code]) << "code " << (int)code;

      write(REG_DIGIT0, DP | 0x70 | code);
      EXPECT_EQ(digit(0), DP | font[code]) << "code " << (int)code;
    }

    write(REG_DECODE_MODE, 0x01);
    write(REG_DIGIT0, 0x08);
    write(REG_DIGIT0 + 1, 0x08);
    expect_digits({font[8], 0x08, 0, 0, 0, 0, 0, 0});
  }

  void test_scan_limit() {
    ledcontrol_init();
    for (u8 d = 0; d < 8; d++)
      write(REG_DIGIT0 + d, 0xff);
    expect_all(true);

    write(REG_SCAN_LIMIT, 2);
    expect_digits({0xff, 0xff, 0xff, 0, 0, 0, 0, 0});

    write(REG_SCAN_LIMIT, 0xf8);
    expect_digits({0xff, 0, 0, 0, 0, 0, 0, 0});

    write(REG_SCAN_LIMIT, 7);
    expect_all(true);
  }

  void test_shutdown() {
    ledcontrol_init();
    write(REG_DIGIT0 + 5, 0x5a);

    write(REG_SHUTDOWN, 0);
    expect_all(false);
    write(REG_DIGIT0 + 6, 0xa5);
    expect_all(false);

    write(REG_SHUTDOWN, 0xfe);
    expect_all(false);
    write(REG_SHUTDOWN, 0x01);
    expect_digits({0, 0, 0, 0, 0, 0x5a, 0xa5, 0});
  }

  void test_display_test() {
    ledcontrol_init();
    write(REG_SCAN_LIMIT, 0);
    write(REG_DIGIT0, 0x3c);

    write(REG_DISPLAY_TEST, 1);
    expect_all(true);

    write(REG_SHUTDOWN, 0);
    expect_all(true);

    write(REG_DISPLAY_TEST, 0);
    expect_all(false);
    write(REG_SHUTDOWN, 1);
    expect_digits({0x3c, 0, 0, 0, 0, 0, 0, 0});
  }

  void test_daisy_chain() {
    thermo.temp_thermocouple = -12.75;  // -51 = 0x3fcd (14 bit)
    thermo.temp_internal = 23.5;        // 376 = 0x178 (12 bit)
    const u32 raw = (0x3fcdu << 18) | (0x178u << 4);

    chain_load = true;

    EXPECT_EQ(chain_frame(cmd(REG_SHUTDOWN, 1), cmd(REG_SHUTDOWN, 1)), raw);
    EXPECT_EQ(chain_frame(cmd(REG_SCAN_LIMIT, 7), cmd(REG_SCAN_LIMIT, 7)), raw);
    EXPECT_EQ(chain_frame(cmd(REG_DIGIT0, 0x12), cmd(REG_DIGIT0, 0x34)), raw);
    EXPECT_EQ(digit(chain_second, 0), 0x12);
    EXPECT_EQ(digit(chain_first, 0), 0x34);

    chain_frame(cmd(REG_NOOP, 0), cmd(REG_DIGIT0 + 7, 0x56));
    EXPECT_EQ(digit(chain_second, 7), 0x00);
    EXPECT_EQ(digit(chain_first, 7), 0x56);
    chain_frame(cmd(REG_DIGIT0 + 7, 0x78), cmd(REG_NOOP, 0));
    EXPECT_EQ(digit(chain_second, 7), 0x78);
    EXPECT_EQ(digit(chain_first, 7), 0x56);
    EXPECT_EQ(digit(chain_second, 0), 0x12);
    EXPECT_EQ(digit(chain_first, 0), 0x34);

    thermo.temp_thermocouple = 25.0;  // 100 = 0x064,
    thermo.temp_internal = 25.0;      // 400 = 0x190
    EXPECT_EQ(chain_frame(cmd(REG_NOOP, 0), cmd(REG_NOOP, 0)), (0x064u << 18) | (0x190u << 4));
  }
};

TEST(spi, max7219_test) {
  max7219_test test("test");
  sc_core::sc_start();
}
