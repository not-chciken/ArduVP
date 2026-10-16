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
  }

  void write_reg(u8 addr, u8 value) {
    ASSERT_EQ(socket.start(I2C_ADDR, tlm::TLM_WRITE_COMMAND), I2C_ACK);
    data = addr;
    ASSERT_EQ(socket.transport(data), I2C_ACK);
    data = value;
    ASSERT_EQ(socket.transport(data), I2C_ACK);
    ASSERT_EQ(socket.stop(), I2C_ACK);
  }

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

  sc_time conversion_time(u8 cmd) {
    if (cmd == CMD_TEMPERATURE)
      return sc_time(4.5, SC_MS);
    static const double times[] = {4.5, 7.5, 13.5, 25.5};
    return sc_time(times[cmd >> 6], SC_MS);
  }

  i32 measure(u8 cmd) {
    write_reg(REG_CTRL_MEAS, cmd);
    EXPECT_EQ(read_reg(REG_CTRL_MEAS), cmd);
    wait(conversion_time(cmd) - sc_time(1, SC_US));
    EXPECT_TRUE(read_reg(REG_CTRL_MEAS) & SCO);
    wait(sc_time(2, SC_US));
    EXPECT_EQ(read_reg(REG_CTRL_MEAS), cmd & ~SCO);

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

    bmp.temperature = 15.0;
    bmp.pressure = 699.64;
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
      bmp.temperature = temp;
      i32 t = bmp.compensate_temperature(measure(CMD_TEMPERATURE));
      ASSERT_NEAR(t, temp * 10.0, 1.0) << "temperature " << temp;
    }
  }

  void test_pressure() {
    for (double temp : {-20.0, 25.0, 65.0}) {
      bmp.temperature = temp;
      i32 b5 = measure_b5();

      for (double press : {300.0, 699.64, 1013.25, 1100.0}) {
        bmp.pressure = press;

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
    bmp.temperature = 25.0;
    bmp.pressure = 1000.0;
    i32 b5 = measure_b5();

    bmp.pressure_noise = true;
    bmp.temperature_noise = true;

    std::set<i32> temps, pressures;
    for (int i = 0; i < 20; i++) {
      i32 t = bmp.compensate_temperature(measure(CMD_TEMPERATURE));
      i32 p = bmp.compensate_pressure(measure(CMD_PRESSURE), b5, 0);
      ASSERT_NEAR(t, 250, 6);
      ASSERT_NEAR(p, 100000, 40);
      temps.insert(t);
      pressures.insert(p);
    }

    ASSERT_GT(temps.size(), 1u);
    ASSERT_GT(pressures.size(), 1u);

    bmp.pressure_noise = false;
    bmp.temperature_noise = false;
  }

  void test_soft_reset() {
    bmp.temperature = 25.0;
    measure(CMD_TEMPERATURE);
    ASSERT_NE(read_regs(REG_OUT_MSB, 3), vector<u8>({0x80, 0x00, 0x00}));

    write_reg(REG_CTRL_MEAS, CMD_PRESSURE);
    write_reg(REG_SOFT_RESET, 0x00);
    ASSERT_EQ(read_reg(REG_CTRL_MEAS), CMD_PRESSURE);

    write_reg(REG_SOFT_RESET, 0xb6);
    ASSERT_EQ(read_reg(REG_CTRL_MEAS), 0x00);
    ASSERT_EQ(read_regs(REG_OUT_MSB, 3), vector<u8>({0x80, 0x00, 0x00}));

    wait(conversion_time(CMD_PRESSURE) * 2);
    ASSERT_EQ(read_regs(REG_OUT_MSB, 3), vector<u8>({0x80, 0x00, 0x00}));
  }
};

TEST(i2c, bmp180_test) {
  bmp180_test test("test");
  sc_core::sc_start();
}
