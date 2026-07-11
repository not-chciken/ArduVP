#include "system.h"
#include "testing.h"

struct UsartTest : public test_base {
  Usart usart;
  vcml::generic::bus bus;
  vcml::generic::clock test_clock;
  vcml::tlm_initiator_socket socket;

  UsartTest(const sc_module_name& nm)
      : test_base(nm), usart("usart"), bus("bus"), test_clock("test_clock", 16 * vcml::MHz), socket("socket") {
    bus.bind(usart.in, vcml::range(0xc0, 0xc6));
    bus.bind(socket);

    test_clock.clk.bind(usart.clk);
    test_clock.clk.bind(bus.clk);

    rst.bind(usart.rst);
    rst.bind(bus.rst);

    usart.serial_rx.stub();
    usart.serial_tx.stub();

    usart.irq_rx.stub();
    usart.irq_tx.stub();
    usart.irq_udre.stub();

    add_test("init", &UsartTest::TestInit);
    add_test("baudrate", &UsartTest::TestBaudRate);
    add_test("transmit", &UsartTest::TestTransmit);
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
    socket.writew<u8>(0xc4, (u8)0xff);
    socket.writew<u8>(0xc5, (u8)0xff);
    ASSERT_EQ((u8)usart.ubrrnl, (u8)0xff);
    ASSERT_EQ((u8)usart.ubrrnh, (u8)0x0f) << "Upper 4 bits are read-only";

    socket.writew<u8>(0xc4, (u8)25);
    socket.writew<u8>(0xc5, (u8)0);
    ASSERT_EQ(usart.serial_tx.baud(), 38461);

    socket.writew<u8>(0xc4, (u8)0xa0);
    socket.writew<u8>(0xc5, (u8)0x01);
    ASSERT_EQ(usart.serial_tx.baud(), 2398);

    socket.writew<u8>(0xc0, (u8)0x02); // Double the transfer rate.
    ASSERT_EQ(usart.serial_tx.baud(), 4796);
  }

  void TestTransmit() {
    socket.writew<u8>(0xc6, (u8)0xff);
    ASSERT_EQ((u8)usart.udrn, (u8)0xff);
    ASSERT_FALSE((u8)usart.ucsrna & 0x40) << "Transmit should be disabled";
    sc_core::wait(1, sc_core::SC_MS);
    ASSERT_FALSE((u8)usart.ucsrna & 0x40) << "Transmit should be disabled";
    ASSERT_FALSE(usart.irq_tx.read());

    socket.writew<u8>(0xc1, (u8)0x48); // Enable transmit and interrupt.
    socket.writew<u8>(0xc6, (u8)0xff);
    ASSERT_EQ((u8)usart.udrn, (u8)0xff);
    ASSERT_FALSE((u8)usart.ucsrna & 0x40) << "Transmit should not be finished yet";
    sc_core::wait(1, sc_core::SC_MS);
    ASSERT_TRUE((u8)usart.ucsrna & 0x40) << "Transmit should be finished";
    ASSERT_TRUE(usart.irq_tx.read());
  }
};

TEST(ArduSystem, UsartTest) {
  UsartTest test("test");
  sc_core::sc_start();
}
