/******************************************************************************
 * Apache License, Version 2.0
 * Copyright (c) 2026 chciken/Niko Zurstraßen
 *****************************************************************************/

#include "models/bmp180.h"

#include "testing.h"

struct bmp180_test : public test_base, i2c_host {
  static constexpr u8 I2C_ADDR = 0x77;
  static constexpr u8 REG_CALIB = 0xaa;
  static constexpr u8 REG_ID = 0xd0;
  static constexpr u8 REG_SOFT_RESET = 0xe0;
  static constexpr u8 REG_CTRL_MEAS = 0xf4;
  static constexpr u8 REG_OUT_MSB = 0xf6;
  static constexpr u8 CMD_TEMPERATURE = 0x2e;
  static constexpr u8 CMD_PRESSURE = 0x34;
  static constexpr u8 SCO = 0x20;
  static constexpr u8 UNUSED = 0x04;

  i2c::bmp180 bmp;
  i2c_initiator_socket socket;
  u8 data;

  bmp180_test(const sc_module_name& nm) : test_base(nm), bmp("bmp"), socket("socket") {
    socket.bind(bmp.in);
    rst.bind(bmp.rst);

    add_test("datasheet_example", &bmp180_test::test_datasheet_example);
    add_test("address", &bmp180_test::test_address);
    add_test("reset_state", &bmp180_test::test_reset_state);
    add_test("calibration", &bmp180_test::test_calibration);
    add_test("temperature", &bmp180_test::test_temperature);
    add_test("pressure", &bmp180_test::test_pressure);
    add_test("noise", &bmp180_test::test_noise);
    add_test("soft_reset", &bmp180_test::test_soft_reset);
    add_test("conversion_time", &bmp180_test::test_conversion_time);
    add_test("unused_bit", &bmp180_test::test_unused_bit);
    add_test("restart", &bmp180_test::test_restart);
    add_test("write_during_conversion", &bmp180_test::test_write_during_conversion);
    add_test("oss_change", &bmp180_test::test_oss_change);
    add_test("measurement_control", &bmp180_test::test_measurement_control);
    add_test("burst_write", &bmp180_test::test_burst_write);
  }

  void write_regs(u8 addr, const vector<u8>& values) {
    ASSERT_EQ(socket.start(I2C_ADDR, tlm::TLM_WRITE_COMMAND), I2C_ACK);
    data = addr;
    ASSERT_EQ(socket.transport(data), I2C_ACK);
    for (u8 value : values) {
      data = value;
      ASSERT_EQ(socket.transport(data), I2C_ACK);
    }
    ASSERT_EQ(socket.stop(), I2C_ACK);
  }

  void write_reg(u8 addr, u8 value) { write_regs(addr, {value}); }

  vector<u8> read_regs(u8 addr, size_t n) {
    vector<u8> bytes;
    EXPECT_EQ(socket.start(I2C_ADDR, tlm::TLM_WRITE_COMMAND), I2C_ACK);
    data = addr;
    EXPECT_EQ(socket.transport(data), I2C_ACK);
    EXPECT_EQ(socket.start(I2C_ADDR, tlm::TLM_READ_COMMAND), I2C_ACK);
    for (size_t i = 0; i < n; i++) {
      data = 0;
      EXPECT_EQ(socket.transport(data), I2C_ACK);
      bytes.push_back(data);
    }
    EXPECT_EQ(socket.stop(), I2C_ACK);
    return bytes;
  }

  u8 read_reg(u8 addr) { return read_regs(addr, 1)[0]; }

  sc_time conversion_time(u8 cmd) { return sc_time(bmp.conversion_time_ms[cmd >> 6], SC_MS); }

  // Waits until shortly before and after the end of a conversion started at
  // `start` and checks sco.
  void wait_conversion(u8 cmd, const sc_time& start) {
    wait(start + conversion_time(cmd) - sc_time(1, SC_US) - sc_time_stamp());
    EXPECT_TRUE(read_reg(REG_CTRL_MEAS) & SCO);
    wait(sc_time(2, SC_US));
    EXPECT_FALSE(read_reg(REG_CTRL_MEAS) & SCO);
  }

  u32 read_out() {
    vector<u8> out = read_regs(REG_OUT_MSB, 3);
    return (out[0] << 16) | (out[1] << 8) | out[2];
  }

  i32 measure(u8 cmd) {
    sc_time start = sc_time_stamp();
    write_reg(REG_CTRL_MEAS, cmd);
    EXPECT_EQ(read_reg(REG_CTRL_MEAS), cmd & ~UNUSED);
    wait_conversion(cmd, start);
    EXPECT_EQ(read_reg(REG_CTRL_MEAS), cmd & ~(SCO | UNUSED));

    vector<u8> out = read_regs(REG_OUT_MSB, 3);
    if (cmd == CMD_TEMPERATURE)
      return (out[0] << 8) | out[1];

    u8 oss = cmd >> 6;
    return ((out[0] << 16) | (out[1] << 8) | out[2]) >> (8 - oss);
  }

  i32 measure_b5() { return bmp.compensate_b5(measure(CMD_TEMPERATURE)); }

  void test_datasheet_example() {
    ASSERT_EQ(bmp.compensate_b5(27898), 2400);
    ASSERT_EQ(bmp.compensate_temperature(27898), 150);
    ASSERT_EQ(bmp.compensate_b3(2399, 0), 422);
    ASSERT_EQ(bmp.compensate_pressure(23843, 2399, 0), 69964);
    ASSERT_EQ(bmp.compensate_pressure(23843, 2400, 0), 69964);

    bmp.temperature_c = 15.0;
    bmp.pressure_hpa = 699.64;
    ASSERT_EQ(measure(CMD_TEMPERATURE), 27898);
    ASSERT_NEAR(measure(CMD_PRESSURE), 23843, 1);
  }

  void test_address() {
    for (u8 addr = 1; addr < 128; addr++) {
      i2c_response resp = socket.start(addr, tlm::TLM_WRITE_COMMAND);
      ASSERT_EQ(resp, addr == I2C_ADDR ? I2C_ACK : I2C_NACK) << "address " << (unsigned)addr;
      if (resp == I2C_ACK)
        ASSERT_EQ(socket.stop(), I2C_ACK);
    }

    ASSERT_EQ(read_reg(REG_ID), 0x55);
  }

  void test_reset_state() {
    ASSERT_EQ(read_reg(REG_CTRL_MEAS), 0x00);
    ASSERT_EQ(read_regs(REG_OUT_MSB, 3), vector<u8>({0x80, 0x00, 0x00}));
  }

  void test_calibration() {
    vector<u8> bytes = read_regs(REG_CALIB, 22);
    vector<u16> words;
    for (size_t i = 0; i < bytes.size(); i += 2)
      words.push_back((bytes[i] << 8) | bytes[i + 1]);

    vector<u16> expected = {408, (u16)-72, (u16)-14383, 32741, 32757, 23153,
                            6190, 4, (u16)-32768, (u16)-8711, 2868};
    ASSERT_EQ(words, expected);

    bmp.md = 0x1234;
    ASSERT_EQ(read_regs(0xbe, 2), vector<u8>({0x12, 0x34}));
    bmp.md = 2868;
  }

  void test_temperature() {
    for (double temp : {-40.0, -12.3, 0.0, 22.0, 37.5, 85.0}) {
      bmp.temperature_c = temp;
      i32 t = bmp.compensate_temperature(measure(CMD_TEMPERATURE));
      ASSERT_NEAR(t, temp * 10.0, 1.0) << "temperature " << temp;
    }
  }

  void test_pressure() {
    for (double temp : {-20.0, 25.0, 65.0}) {
      bmp.temperature_c = temp;
      i32 b5 = measure_b5();

      for (double press : {300.0, 699.64, 1013.25, 1100.0}) {
        bmp.pressure_hpa = press;

        for (u8 oss = 0; oss < 4; oss++) {
          i32 up = measure(CMD_PRESSURE | (oss << 6));
          i32 p = bmp.compensate_pressure(up, b5, oss);
          ASSERT_NEAR(p, press * 100.0, 3.0)
              << "temperature " << temp << " pressure " << press << " oss " << (int)oss;
        }
      }
    }
  }

  void test_noise() {
    bmp.temperature_c = 25.0;
    bmp.pressure_hpa = 1000.0;
    i32 b5 = measure_b5();

    bmp.pressure_noise = true;
    bmp.temperature_noise = true;

    std::set<i32> temps, pressures;
    for (int i = 0; i < 20; i++) {
      i32 ut = measure(CMD_TEMPERATURE);
      i32 p = bmp.compensate_pressure(measure(CMD_PRESSURE), b5, 0);
      ASSERT_NEAR(bmp.compensate_temperature(ut), 250, 1);
      ASSERT_NEAR(p, 100000, 40);
      temps.insert(ut);
      pressures.insert(p);
    }

    ASSERT_GT(temps.size(), 1u);
    ASSERT_GT(pressures.size(), 1u);

    bmp.pressure_noise = false;
    bmp.temperature_noise = false;
  }

  void test_soft_reset() {
    bmp.temperature_c = 25.0;
    measure(CMD_TEMPERATURE);
    ASSERT_NE(read_regs(REG_OUT_MSB, 3), vector<u8>({0x80, 0x00, 0x00}));

    write_reg(REG_CTRL_MEAS, CMD_PRESSURE);
    write_reg(REG_SOFT_RESET, 0x00);
    ASSERT_EQ(read_reg(REG_CTRL_MEAS), CMD_PRESSURE & ~UNUSED);

    write_reg(REG_SOFT_RESET, 0xb6);
    ASSERT_EQ(read_reg(REG_CTRL_MEAS), 0x00);
    ASSERT_EQ(read_regs(REG_OUT_MSB, 3), vector<u8>({0x80, 0x00, 0x00}));

    wait(conversion_time(CMD_PRESSURE) * 2);
    ASSERT_EQ(read_regs(REG_OUT_MSB, 3), vector<u8>({0x80, 0x00, 0x00}));
  }

  void test_conversion_time() {
    bmp.temperature_c = 25.0;
    bmp.conversion_time_ms.set(3.0, 0);
    bmp.conversion_time_ms.set(5.3, 1);
    bmp.conversion_time_ms.set(9.0, 2);
    bmp.conversion_time_ms.set(17.0, 3);

    i32 ut = measure(CMD_TEMPERATURE);
    for (u8 oss = 0; oss < 4; oss++)
      measure(CMD_PRESSURE | (oss << 6));

    // The duration of temperature conversions depends on oss, too.
    u8 cmd = CMD_TEMPERATURE | (3 << 6);
    measure(cmd);
    ASSERT_EQ(read_out(), (u32)ut << 8);

    bmp.conversion_time_ms.reset();
    ASSERT_EQ(bmp.conversion_time_ms[0], 4.5);
    ASSERT_EQ(bmp.conversion_time_ms[3], 25.5);
  }

  void test_unused_bit() {
    write_reg(REG_CTRL_MEAS, 0x0e);
    ASSERT_EQ(read_reg(REG_CTRL_MEAS), 0x0a);
    write_reg(REG_CTRL_MEAS, 0xff & ~SCO);
    ASSERT_EQ(read_reg(REG_CTRL_MEAS), 0xdb);
    write_reg(REG_CTRL_MEAS, 0x00);
  }

  void test_restart() {
    bmp.temperature_c = 25.0;
    bmp.pressure_hpa = 1000.0;
    u32 ut = measure(CMD_TEMPERATURE);

    // The 2nd start does not restart the conversion, the input selected at
    // the end is used.
    u8 cmd = CMD_PRESSURE | (3 << 6);
    sc_time start = sc_time_stamp();
    write_reg(REG_CTRL_MEAS, cmd);
    wait(2, SC_MS);
    write_reg(REG_CTRL_MEAS, CMD_TEMPERATURE);
    ASSERT_EQ(read_reg(REG_CTRL_MEAS), CMD_TEMPERATURE & ~UNUSED);
    wait_conversion(cmd, start);
    ASSERT_EQ(read_reg(REG_CTRL_MEAS), CMD_TEMPERATURE & ~(SCO | UNUSED));

    // 8 temperature samples, scaled for oss 0
    u32 expected = (0x800000 + (8 * ((i64)ut - 0x8000) << 8)) & 0xffffff;
    ASSERT_EQ(read_out(), expected);
  }

  void test_write_during_conversion() {
    sc_time start = sc_time_stamp();
    write_reg(REG_CTRL_MEAS, CMD_TEMPERATURE);
    wait(1, SC_MS);
    write_reg(REG_CTRL_MEAS, 0x00);
    ASSERT_EQ(read_reg(REG_CTRL_MEAS), SCO);
    write_reg(REG_CTRL_MEAS, CMD_TEMPERATURE & ~SCO);
    ASSERT_EQ(read_reg(REG_CTRL_MEAS), CMD_TEMPERATURE & ~UNUSED);
    wait_conversion(CMD_TEMPERATURE, start);
  }

  void test_oss_change() {
    bmp.temperature_c = 25.0;
    bmp.pressure_hpa = 1000.0;
    u32 up = measure(CMD_PRESSURE);

    // 1 sample, scaled for oss 3
    sc_time start = sc_time_stamp();
    write_reg(REG_CTRL_MEAS, CMD_PRESSURE);
    wait(1, SC_MS);
    write_reg(REG_CTRL_MEAS, (CMD_PRESSURE & ~SCO) | (3 << 6));
    ASSERT_EQ(read_reg(REG_CTRL_MEAS), 0xf0);
    wait_conversion(CMD_PRESSURE, start);
    ASSERT_EQ(read_out(), 0x800000 + ((((i64)up - 0x8000) << 8) >> 3));

    // 8 samples, scaled for oss 0
    u8 cmd = CMD_PRESSURE | (3 << 6);
    u32 up3 = measure(cmd);
    start = sc_time_stamp();
    write_reg(REG_CTRL_MEAS, cmd);
    wait(2, SC_MS);
    write_reg(REG_CTRL_MEAS, CMD_PRESSURE & ~SCO);
    ASSERT_EQ(read_reg(REG_CTRL_MEAS), 0x30);
    wait_conversion(cmd, start);
    ASSERT_EQ(read_out(), (0x800000 + (((i64)up3 - 0x40000) << 8)) & 0xffffff);
  }

  void test_measurement_control() {
    bmp.temperature_c = 25.0;
    bmp.pressure_hpa = 1000.0;
    u32 ut = measure(CMD_TEMPERATURE);
    u32 up = measure(CMD_PRESSURE);

    struct {
      u8 cmd;
      u32 out;
    } cases[] = {
        {0x20, 0x000000},  // no input
        {0x2f, ut << 8},   // temperature
        {0x2a, ut << 8},   // temperature without unused bit
        {0x35, up << 8},   // pressure
        {0x30, up << 8},   // pressure without unused bit
        {0x3f, 0x800000},  // both inputs
        {0xe0, 0x000000},  // no input with oss 3
    };

    for (auto& c : cases) {
      measure(c.cmd);
      ASSERT_EQ(read_out(), c.out) << "cmd " << std::hex << (unsigned)c.cmd;
    }

    // No conversion without sco
    u32 out = read_out();
    write_reg(REG_CTRL_MEAS, 0x14);
    ASSERT_EQ(read_reg(REG_CTRL_MEAS), 0x10);
    wait(conversion_time(CMD_PRESSURE) * 2);
    ASSERT_EQ(read_out(), out);
  }

  void test_burst_write() {
    write_regs(REG_CTRL_MEAS, {0x14, CMD_TEMPERATURE});
    ASSERT_EQ(read_reg(REG_CTRL_MEAS), 0x10);
    write_regs(REG_CTRL_MEAS - 1, {0x00, CMD_TEMPERATURE});
    ASSERT_EQ(read_reg(REG_CTRL_MEAS), 0x10);
    write_reg(REG_CTRL_MEAS, 0x00);
  }
};

TEST(i2c, bmp180_test) {
  bmp180_test test("test");
  sc_core::sc_start();
}
