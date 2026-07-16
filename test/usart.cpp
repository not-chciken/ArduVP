#include <vector>

#include "system.h"
#include "testing.h"

struct SerialBuffer : public vcml::component, vcml::serial_host {
  virtual void serial_receive(u8 symbol) override { data.push_back(symbol); };
  virtual void reset() override { data.clear(); }

  vcml::serial_target_socket rx;
  vcml::serial_initiator_socket tx;
  std::vector<u8> data;

  SerialBuffer(const sc_module_name& nm) : component(nm), serial_host(), rx("rx"), tx("tx") {}
  virtual ~SerialBuffer() = default;
};

struct UsartTest : public test_base {
  Usart usart;
  vcml::generic::bus bus;
  vcml::generic::clock test_clock;
  vcml::tlm_initiator_socket socket;
  SerialBuffer serial_buffer;

  UsartTest(const sc_module_name& nm)
      : test_base(nm),
        usart("usart"),
        bus("bus"),
        test_clock("test_clock", 16 * vcml::MHz),
        socket("socket"),
        serial_buffer("serial_buffer") {
    bus.bind(usart.in, vcml::range(0xc0, 0xc6));
    bus.bind(socket);

    test_clock.clk.bind(usart.clk);
    test_clock.clk.bind(bus.clk);
    test_clock.clk.bind(serial_buffer.clk);

    rst.bind(usart.rst);
    rst.bind(bus.rst);
    rst.bind(serial_buffer.rst);

    usart.serial_tx.bind(serial_buffer.rx);
    serial_buffer.tx.bind(usart.serial_rx);

    usart.irq_rx.stub();
    usart.irq_tx.stub();
    usart.irq_udre.stub();

    add_test("init", &UsartTest::TestInit);
    add_test("baudrate", &UsartTest::TestBaudRate);
    add_test("transmit", &UsartTest::TestTransmit);
    add_test("receive", &UsartTest::TestReceive);
  }

  void TestInit() {
    ASSERT_EQ((u8)usart.ucsrna, (u8)0x20);
    ASSERT_EQ((u8)usart.ucsrnb, (u8)0x00);
    ASSERT_EQ((u8)usart.ucsrnc, (u8)0x06);
    ASSERT_EQ((u8)usart.ubrrnh, (u8)0x00);
    ASSERT_EQ((u8)usart.ubrrnl, (u8)0x00);
    ASSERT_EQ((u8)usart.ubrrnl, (u8)0x00);
    ASSERT_EQ((u8)usart.udrn, (u8)0x00);
    ASSERT_FALSE(usart.irq_rx);
    ASSERT_FALSE(usart.irq_tx);
    ASSERT_FALSE(usart.irq_udre);
  }

  void TestBaudRate() {
    socket.writew<u8>(0xc4, 0xff);
    socket.writew<u8>(0xc5, 0xff);
    ASSERT_EQ((u8)usart.ubrrnl, 0xff);
    ASSERT_EQ((u8)usart.ubrrnh, 0x0f) << "Upper 4 bits are read-only";

    socket.writew<u8>(0xc4, 25);
    socket.writew<u8>(0xc5, 0);
    ASSERT_EQ(usart.serial_tx.baud(), 38461) << "Transfer rate wrong";

    socket.writew<u8>(0xc4, 0xa0);
    socket.writew<u8>(0xc5, 0x01);
    ASSERT_EQ(usart.serial_tx.baud(), 2398) << "Transfer rate upper byte wrong";

    socket.writew<u8>(0xc0, (u8)0x02);
    ASSERT_EQ(usart.serial_tx.baud(), 4796) << "Transfer rate should be doubled";
  }

  void TestTransmit() {
    socket.writew<u8>(0xc6, 0xff);
    ASSERT_EQ((u8)usart.udrn, 0xff);
    ASSERT_FALSE((u8)usart.ucsrna & 0x40) << "Transmit should be disabled";
    sc_core::wait(1, sc_core::SC_MS);
    ASSERT_FALSE((u8)usart.ucsrna & 0x40) << "Transmit should be disabled";
    ASSERT_FALSE(usart.irq_tx.read());

    socket.writew<u8>(0xc1, 0x48);
    socket.writew<u8>(0xc6, 0xff);
    ASSERT_EQ((u8)usart.udrn, (u8)0xff);
    ASSERT_FALSE((u8)usart.ucsrna & 0x40) << "Transmit should not be finished yet";
    sc_core::wait(1, sc_core::SC_MS);
    ASSERT_TRUE((u8)usart.ucsrna & 0x40) << "Transmit should be finished";
    ASSERT_TRUE(usart.irq_tx.read());

    u8 received = 0;
    ASSERT_EQ(serial_buffer.data.size(), 1) << "Terminal TX should contain exactly 1 byte";
    ASSERT_EQ(serial_buffer.data[0], (u8)0xff) << "Received byte should match transmitted value";
  }

  void TestReceive() {
    socket.writew<u8>(0xc1, 0x10);
    serial_buffer.tx.send(0x42);
    sc_core::wait(1, sc_core::SC_MS);

    ASSERT_TRUE((u8)usart.ucsrna & 0x80) << "RX complete flag (RXCN) should be set";
    u8 data;
    socket.readw<u8>(0xc6, data);
    ASSERT_EQ(data, (0x42)) << "Received byte should match sent value";
    ASSERT_FALSE((u8)usart.ucsrna & 0x80) << "RXCN should be cleared after reading UDR";
  }
};

TEST(ArduSystem, UsartTest) {
  UsartTest test("test");
  sc_core::sc_start();
}
