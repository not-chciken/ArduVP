#include <vector>

#include "usart.h"

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

  vcml::gpio_initiator_socket ack_rx;
  vcml::gpio_initiator_socket ack_tx;
  vcml::gpio_initiator_socket ack_udre;

  UsartTest(const sc_module_name& nm)
      : test_base(nm),
        usart("usart"),
        bus("bus"),
        test_clock("test_clock", 16 * vcml::MHz),
        socket("socket"),
        serial_buffer("serial_buffer"),
        ack_rx("ack_rx"),
        ack_tx("ack_tx"),
        ack_udre("ack_udre") {
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

    ack_rx.bind(usart.rxc_ack);
    ack_tx.bind(usart.tx_ack);
    ack_udre.bind(usart.udre_ack);

    add_test("init", &UsartTest::TestInit);
    add_test("baudrate", &UsartTest::TestBaudRate);
    add_test("transmit", &UsartTest::TestTransmit);
    add_test("receive", &UsartTest::TestReceive);
    add_test("parity", &UsartTest::TestParity);
    add_test("character_size", &UsartTest::TestCharacterSize);
    add_test("irq_ack", &UsartTest::TestIrqAck);
    add_test("udre_does_not_raise_rx_irq", &UsartTest::TestUdreDoesNotRaiseRxIrq);
    add_test("udre_irq_on_enable", &UsartTest::TestUdreIrqOnEnable);
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

  void TestParity() {
    vcml::serial_payload payload;
    payload.baud = SERIAL_9600BD;
    payload.width = SERIAL_8_BITS;
    payload.mask = serial_mask(SERIAL_8_BITS);

    socket.writew<u8>(0xc1, 0x10);

    socket.writew<u8>(0xc2, 0x20);  // Check for even parity.
    payload.parity = vcml::SERIAL_PARITY_EVEN;

    payload.data = 0xff;
    serial_buffer.tx.transport(payload);
    sc_core::wait(1, sc_core::SC_MS);
    ASSERT_TRUE((u8)usart.ucsrna & 0x4) << "Wrong parity bit should be set";

    payload.data = 0x1ff;
    serial_buffer.tx.transport(payload);
    sc_core::wait(1, sc_core::SC_MS);
    ASSERT_TRUE((u8)usart.ucsrna & 0x4) << "Wrong parity bit should not be set";

    payload.parity = vcml::SERIAL_PARITY_ODD;
    socket.writew<u8>(0xc2, 0x30);  // Check for odd parity.

    payload.data = 0x1ff;
    serial_buffer.tx.transport(payload);
    sc_core::wait(1, sc_core::SC_MS);
    ASSERT_TRUE((u8)usart.ucsrna & 0x4) << "Wrong parity bit should be set";

    payload.data = 0xff;
    serial_buffer.tx.transport(payload);
    sc_core::wait(1, sc_core::SC_MS);
    ASSERT_TRUE((u8)usart.ucsrna & 0x4) << "Wrong parity bit should not be set";
  }

  void TestCharacterSize() {
    struct {
      u8 ucsrnc_val;
      vcml::serial_bits expected_width;
      u8 expected_mask;
      const char* label;
    } cases[] = {
        {0x00, vcml::SERIAL_5_BITS, 0x1f, "5 bits"},
        {0x02, vcml::SERIAL_6_BITS, 0x3f, "6 bits"},
        {0x04, vcml::SERIAL_7_BITS, 0x7f, "7 bits"},
        {0x06, vcml::SERIAL_8_BITS, 0xff, "8 bits"},
    };

    socket.writew<u8>(0xc1, 0x10);

    for (auto& c : cases) {
      socket.writew<u8>(0xc2, c.ucsrnc_val);
      ASSERT_EQ(usart.serial_tx.data_width(), c.expected_width) << c.label;

      serial_buffer.tx.send(0xff);
      sc_core::wait(1, sc_core::SC_MS);

      u8 data;
      socket.readw<u8>(0xc6, data);
      ASSERT_EQ(data, c.expected_mask) << "Received symbol should be masked to " << c.label;
    }
  }

  void TestIrqAck() {
    socket.writew<u8>(0xc1, 0x48);
    socket.writew<u8>(0xc6, 0xff);
    sc_core::wait(1, sc_core::SC_MS);
    ASSERT_TRUE((u8)usart.ucsrna & 0x40) << "TXCN should be set";
    ASSERT_TRUE(usart.irq_tx.read());

    ack_tx.pulse();

    ASSERT_FALSE((u8)usart.ucsrna & 0x40) << "ack_tx should clear TXCN";
    ASSERT_FALSE(usart.irq_tx.read()) << "ack_tx should drop irq_tx";

    socket.writew<u8>(0xc6, 0x11);
    sc_core::wait(1, sc_core::SC_MS);
    ASSERT_TRUE((u8)usart.ucsrna & 0x40) << "TXC should be able to fire again after being acked";
    ASSERT_TRUE(usart.irq_tx.read());
    ack_tx.pulse();

    socket.writew<u8>(0xc1, 0x10);
    serial_buffer.tx.send(0x42);
    sc_core::wait(1, sc_core::SC_MS);
    ASSERT_TRUE((u8)usart.ucsrna & 0x80) << "RXCN should be set";

    ack_rx.pulse();
    ASSERT_TRUE((u8)usart.ucsrna & 0x80) << "ack_rx must not clear RXCN";

    ASSERT_TRUE((u8)usart.ucsrna & 0x20) << "UDREN is set by default";
    ack_udre.pulse();
    ASSERT_TRUE((u8)usart.ucsrna & 0x20) << "ack_udre must not clear UDREN";
  }

  void TestUdreDoesNotRaiseRxIrq() {
    socket.writew<u8>(0xc1, 0x88);
    ASSERT_FALSE(usart.irq_rx.read());

    socket.writew<u8>(0xc6, 0x55);
    ASSERT_FALSE(usart.irq_rx.read()) << "irq_rx must not be set by writing UDR";

    sc_core::wait(1, sc_core::SC_MS);

    ASSERT_FALSE((u8)usart.ucsrna & 0x80) << "RXCN must not be set - nothing was received";
    ASSERT_FALSE(usart.irq_rx.read()) << "SetUdre must not raise irq_rx";
  }

  void TestUdreIrqOnEnable() {
    socket.writew<u8>(0xc1, 0x08);
    socket.writew<u8>(0xc6, 0x55);
    sc_core::wait(1, sc_core::SC_MS);
    ASSERT_TRUE((u8)usart.ucsrna & 0x20) << "UDREN should be set after transmission";
    ASSERT_FALSE(usart.irq_udre.read()) << "irq_udre must not be raised while UDRIE is clear";

    socket.writew<u8>(0xc1, 0x28);
    ASSERT_TRUE(usart.irq_udre.read()) << "enabling UDRIE while UDREN is set should raise irq_udre";

    socket.writew<u8>(0xc1, 0x08);
    ASSERT_FALSE(usart.irq_udre.read()) << "disabling UDRIE should drop irq_udre";
  }
};

TEST(ArduSystem, UsartTest) {
  UsartTest test("test");
  sc_core::sc_start();
}
