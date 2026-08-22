#include "io_ports.h"

#include "testing.h"

static constexpr mwr::u64 PINB = 0x23;
static constexpr mwr::u64 DDRB = 0x24;
static constexpr mwr::u64 PORTB = 0x25;
static constexpr mwr::u64 PINC = 0x26;
static constexpr mwr::u64 DDRC = 0x27;
static constexpr mwr::u64 PORTC = 0x28;
static constexpr mwr::u64 PIND = 0x29;
static constexpr mwr::u64 DDRD = 0x2a;
static constexpr mwr::u64 PORTD = 0x2b;

static constexpr uint NUM_PORT_PINS = 8;

struct PortWiring {
  vcml::gpio_initiator_array<NUM_PORT_PINS> drive;
  vcml::gpio_target_array<NUM_PORT_PINS> sense;

  PortWiring(const string& nm):
      drive((nm + "_drive").c_str()), sense((nm + "_sense").c_str()) {}
};

struct IoPortsTest : public test_base {
  IoPorts io_ports;
  vcml::tlm_initiator_socket socket;
  vcml::generic::bus bus;

  PortWiring wb, wc, wd;

  IoPortsTest(const sc_module_name& nm)
      : test_base(nm),
        io_ports("io_ports"),
        socket("socket"),
        bus("bus"),
        wb("wb"),
        wc("wc"),
        wd("wd")
  {
    bus.bind(io_ports.in, vcml::range(PINB, PORTD));
    bus.bind(socket);

    clk.bind(bus.clk);
    clk.bind(io_ports.clk);

    rst.bind(bus.rst);
    rst.bind(io_ports.rst);

    for (size_t i = 0; i < NUM_PORT_PINS; ++i) {
      wb.drive[i].bind(io_ports.in_pinb[i]);
      wb.sense[i].bind(io_ports.out_pinb[i]);

      wc.drive[i].bind(io_ports.in_pinc[i]);
      wc.sense[i].bind(io_ports.out_pinc[i]);

      wd.drive[i].bind(io_ports.in_pind[i]);
      wd.sense[i].bind(io_ports.out_pind[i]);
    }

    add_test("reset", &IoPortsTest::TestReset);
    add_test("port_b", &IoPortsTest::TestPortB);
    add_test("port_c", &IoPortsTest::TestPortC);
    add_test("port_d", &IoPortsTest::TestPortD);
    add_test("bit_independence", &IoPortsTest::TestBitIndependence);
  }

  void TestPortB() { TestPort(PINB, DDRB, PORTB, wb, "B"); }
  void TestPortC() { TestPort(PINC, DDRC, PORTC, wc, "C"); }
  void TestPortD() { TestPort(PIND, DDRD, PORTD, wd, "D"); }

  void TestReset() {
    ASSERT_EQ((uint)io_ports.ddrb, 0) << "DDRB should reset to 0 (all pins input)";
    ASSERT_EQ((uint)io_ports.portb, 0) << "PORTB should reset to 0";

    ASSERT_EQ((uint)io_ports.ddrc, 0) << "DDRC should reset to 0 (all pins input)";
    ASSERT_EQ((uint)io_ports.portc, 0) << "PORTC should reset to 0";

    ASSERT_EQ((uint)io_ports.ddrd, 0) << "DDRD should reset to 0 (all pins input)";
    ASSERT_EQ((uint)io_ports.portd, 0) << "PORTD should reset to 0";

    socket.writew<u8>(DDRB, (u8)0xff);
    socket.writew<u8>(PORTB, (u8)0xff);
    ASSERT_NE((uint)io_ports.ddrb, 0);
    ASSERT_NE((uint)io_ports.portb, 0);

    io_ports.reset();

    ASSERT_EQ((uint)io_ports.ddrb, 0) << "DDRB should return to 0 after reset";
    ASSERT_EQ((uint)io_ports.portb, 0) << "PORTB should return to 0 after reset";
  }

  void TestPort(mwr::u64 pin_addr, mwr::u64 ddr_addr, mwr::u64 port_addr, PortWiring& w,
                const string& label) {
    TestDirectionAndOutputDrive(pin_addr, ddr_addr, port_addr, w, label);
    TestInputRead(pin_addr, ddr_addr, port_addr, w, label);
    TestPortReadbackIndependentOfDirection(ddr_addr, port_addr, label);
    TestPinToggleWrite(pin_addr, ddr_addr, port_addr, w, label);
  }

  void TestDirectionAndOutputDrive(mwr::u64 pin_addr, mwr::u64 ddr_addr, mwr::u64 port_addr,
                                    PortWiring& w, const string& label) {
    socket.writew<u8>(ddr_addr, (u8)0xff);

    for (size_t n = 0; n < NUM_PORT_PINS; ++n) {
      socket.writew<u8>(port_addr, (u8)(1u << n));
      sc_core::wait(sc_core::SC_ZERO_TIME);

      for (size_t i = 0; i < NUM_PORT_PINS; ++i) {
        ASSERT_EQ((bool)w.sense[i], i == n)
            << "port " << label << " pin " << i
            << " output drive mismatch while pin " << n << " set high";
      }

      u8 port_val = 0, pin_val = 0;
      socket.readw<u8>(port_addr, port_val);
      socket.readw<u8>(pin_addr, pin_val);
      ASSERT_EQ(port_val, (u8)(1u << n)) << "PORT" << label << " should read back the written value";
      ASSERT_EQ(pin_val, (u8)(1u << n))
          << "PIN" << label << " should reflect the actual (output) pin state";
    }

    socket.writew<u8>(port_addr, (u8)0x00);
    sc_core::wait(sc_core::SC_ZERO_TIME);
    for (uint i = 0; i < NUM_PORT_PINS; ++i)
      ASSERT_FALSE((bool)w.sense[i]) << "port " << label << " pin " << i << " should be driven low";
  }

  void TestInputRead(mwr::u64 pin_addr, mwr::u64 ddr_addr, mwr::u64 port_addr, PortWiring& w,
                      const string& label) {
    socket.writew<u8>(ddr_addr, (u8)0x00);
    socket.writew<u8>(port_addr, (u8)0x00);

    for (uint n = 0; n < NUM_PORT_PINS; ++n)
      w.drive[n] = false;
    sc_core::wait(sc_core::SC_ZERO_TIME);

    u8 pin_val = 0;
    socket.readw<u8>(pin_addr, pin_val);
    ASSERT_EQ(pin_val, 0) << "PIN" << label << " should read 0 while all external pins are low";

    for (uint n = 0; n < NUM_PORT_PINS; ++n)
      w.drive[n] = true;
    sc_core::wait(sc_core::SC_ZERO_TIME);

    socket.readw<u8>(pin_addr, pin_val);
    ASSERT_EQ(pin_val, 0xff) << "PIN" << label << " should read the externally driven pin values";

    for (size_t n = 0; n < NUM_PORT_PINS; ++n)
      w.drive[n] = false;
    w.drive[3] = true;
    sc_core::wait(sc_core::SC_ZERO_TIME);

    socket.readw<u8>(pin_addr, pin_val);
    ASSERT_EQ(pin_val, (u8)(1u << 3))
        << "PIN" << label << " should reflect only the externally driven pin";
  }

  void TestPortReadbackIndependentOfDirection(mwr::u64 ddr_addr, mwr::u64 port_addr,
                                               const string& label) {
    socket.writew<u8>(ddr_addr, (u8)0x00);
    socket.writew<u8>(port_addr, (u8)0xa5);

    u8 port_val = 0;
    socket.readw<u8>(port_addr, port_val);
    ASSERT_EQ(port_val, 0xa5)
        << "PORT" << label << " readback should match the last written value regardless of DDR" << label;
  }

  void TestPinToggleWrite(mwr::u64 pin_addr, mwr::u64 ddr_addr, mwr::u64 port_addr,
                           PortWiring& w, const string& label) {
    socket.writew<u8>(ddr_addr, (u8)0xff);
    socket.writew<u8>(port_addr, (u8)0x00);
    sc_core::wait(sc_core::SC_ZERO_TIME);

    socket.writew<u8>(pin_addr, (u8)0x01);
    sc_core::wait(sc_core::SC_ZERO_TIME);

    u8 port_val = 0;
    socket.readw<u8>(port_addr, port_val);
    ASSERT_EQ(port_val, 0x01) << "PORT" << label << " bit 0 should toggle when 1 is written to PIN" << label;
    ASSERT_TRUE((bool)w.sense[0]) << "port " << label << " pin 0 should now be driven high";

    socket.writew<u8>(pin_addr, (u8)0x01);
    sc_core::wait(sc_core::SC_ZERO_TIME);

    socket.readw<u8>(port_addr, port_val);
    ASSERT_EQ(port_val, 0x00) << "PORT" << label << " bit 0 should toggle back to 0";
    ASSERT_FALSE((bool)w.sense[0]) << "port " << label << " pin 0 should now be driven low";

    socket.writew<u8>(port_addr, (u8)0b00000110);
    socket.writew<u8>(pin_addr, (u8)0b00000001);
    sc_core::wait(sc_core::SC_ZERO_TIME);

    socket.readw<u8>(port_addr, port_val);
    ASSERT_EQ(port_val, 0b00000111) << "PIN" << label << " write should only toggle the requested bit(s)";
  }

  void TestBitIndependence() {
    socket.writew<u8>(DDRB, (u8)0xff);
    socket.writew<u8>(PORTB, (u8)0x00);
    sc_core::wait(sc_core::SC_ZERO_TIME);

    socket.writew<u8>(DDRB, (u8)0b00000101);
    socket.writew<u8>(PORTB, (u8)0b00000101);

    for (size_t n = 0; n < NUM_PORT_PINS; ++n)
      wc.drive[n] = false;
    for (size_t n = 0; n < NUM_PORT_PINS; ++n)
      wd.drive[n] = false;
    sc_core::wait(sc_core::SC_ZERO_TIME);

    for (size_t i = 0; i < NUM_PORT_PINS; ++i) {
      bool expect_high = (i == 0 || i == 2);
      ASSERT_EQ((bool)wb.sense[i], expect_high) << "port B pin " << i << " unexpectedly driven";
    }

    u8 ddrc_val = 0, portc_val = 0, ddrd_val = 0, portd_val = 0;
    socket.readw<u8>(DDRC, ddrc_val);
    socket.readw<u8>(PORTC, portc_val);
    socket.readw<u8>(DDRD, ddrd_val);
    socket.readw<u8>(PORTD, portd_val);
    ASSERT_EQ(ddrc_val, 0) << "configuring port B must not affect port C's direction register";
    ASSERT_EQ(portc_val, 0) << "configuring port B must not affect port C's data register";
    ASSERT_EQ(ddrd_val, 0) << "configuring port B must not affect port D's direction register";
    ASSERT_EQ(portd_val, 0) << "configuring port B must not affect port D's data register";
  }
};

TEST(ArduSystem, IoPortsTest) {
  IoPortsTest test("ioports_test");
  sc_core::sc_start();
}
