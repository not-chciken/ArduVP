#include "ssd1306.h"

#include "testing.h"

struct ssd1306_test : public test_base, i2c_host {
  static constexpr u8 SSD1306_ADDR = 0x3cu;
  static constexpr u8 CONTROL_MULTI_CMD = 0x00u;
  static constexpr u8 CONTROL_MULTI_DATA = 0x40u;
  static constexpr u8 CONTROL_SINGLE_CMD = 0x80u;
  static constexpr u8 CONTROL_SINGLE_DATA = 0xc0u;

  i2c::ssd1306 screen;
  i2c_initiator_socket socket;
  i2c_response resp;
  u8 data;

  ssd1306_test(const sc_module_name& nm) : test_base(nm), screen("screen"), socket("socket") {
    socket.bind(screen.in);
    add_test("address", &ssd1306_test::test_address);
    add_test("on_off", &ssd1306_test::test_on_off);
    add_test("contrast", &ssd1306_test::test_contrast);
    add_test("inverse", &ssd1306_test::test_inverse);
    add_test("read_status", &ssd1306_test::test_read_status);

    rst.bind(screen.rst);
  }

  void test_address() {
    i2c_response resp;

    resp = socket.start(0x23, tlm::TLM_WRITE_COMMAND);
    ASSERT_EQ(resp, I2C_NACK);

    resp = socket.start(I2C_ADDR_BCAST, tlm::TLM_WRITE_COMMAND);
    ASSERT_EQ(resp, I2C_ACK);

    resp = socket.start(SSD1306_ADDR, tlm::TLM_WRITE_COMMAND);
    ASSERT_EQ(resp, I2C_ACK);
  }

  void test_on_off() {
    resp = socket.start(SSD1306_ADDR, tlm::TLM_WRITE_COMMAND);
    ASSERT_EQ(resp, I2C_ACK);
    data = CONTROL_SINGLE_CMD;
    resp = socket.transport(data);
    ASSERT_EQ(resp, I2C_ACK);

    data = 0xaf;
    resp = socket.transport(data);
    ASSERT_EQ(resp, I2C_ACK);
    ASSERT_TRUE(screen.is_display_on());

    data = 0xaf;
    resp = socket.transport(data);
    ASSERT_EQ(resp, I2C_ACK);
    ASSERT_TRUE(screen.is_display_on());

    data = 0xae;
    resp = socket.transport(data);
    ASSERT_EQ(resp, I2C_ACK);
    ASSERT_FALSE(screen.is_display_on());

    resp = socket.stop();
    ASSERT_EQ(resp, I2C_ACK);
  }

  void test_contrast() {
    resp = socket.start(SSD1306_ADDR, tlm::TLM_WRITE_COMMAND);
    ASSERT_EQ(resp, I2C_ACK);
    data = CONTROL_MULTI_CMD;
    resp = socket.transport(data);
    ASSERT_EQ(resp, I2C_ACK);

    data = 0x81;
    resp = socket.transport(data);
    ASSERT_EQ(resp, I2C_ACK);

    data = 0x12;
    resp = socket.transport(data);
    ASSERT_EQ(resp, I2C_ACK);
    ASSERT_EQ(screen.get_contrast(), 0x12);

    resp = socket.stop();
    ASSERT_EQ(resp, I2C_ACK);
  }

  void test_inverse() {
    resp = socket.start(SSD1306_ADDR, tlm::TLM_WRITE_COMMAND);
    ASSERT_EQ(resp, I2C_ACK);
    data = CONTROL_MULTI_CMD;
    resp = socket.transport(data);
    ASSERT_EQ(resp, I2C_ACK);

    data = 0xa7;
    resp = socket.transport(data);
    ASSERT_EQ(resp, I2C_ACK);
    ASSERT_TRUE(screen.is_inverse());

    data = 0xa6;
    resp = socket.transport(data);
    ASSERT_EQ(resp, I2C_ACK);
    ASSERT_FALSE(screen.is_inverse());

    resp = socket.stop();
    ASSERT_EQ(resp, I2C_ACK);
  }

  void test_read_status() {
    resp = socket.start(SSD1306_ADDR, tlm::TLM_READ_COMMAND);
    ASSERT_EQ(resp, I2C_ACK);
    data = CONTROL_MULTI_CMD;
    resp = socket.transport(data);
    ASSERT_EQ(resp, I2C_ACK);

    data = 0;
    resp = socket.transport(data);
    ASSERT_EQ(resp, I2C_ACK);
    ASSERT_EQ(data, 0x40);

    resp = socket.stop();
    ASSERT_EQ(resp, I2C_ACK);
  }

};

TEST(i2c, ssd1306_test) {
  ssd1306_test test("test");
  sc_core::sc_start();
}
