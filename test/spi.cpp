/******************************************************************************
 * Apache License, Version 2.0
 * Copyright (c) 2026 chciken/Niko Zurstraßen
 *****************************************************************************/

// Tests for the ATmega328P hardware SPI, see datasheet chapter 18.
//
// The tests do not depend on the transfer being timed: after starting a
// transfer they wait long enough for eight bits at the slowest SCK rate, so
// they hold for an instantaneous model as well as for a cycle-accurate one.

#include <deque>
#include <vector>

#include "spi.h"

#include "testing.h"

namespace {

constexpr uint64_t kSpcr = 0x4c;
constexpr uint64_t kSpsr = 0x4d;
constexpr uint64_t kSpdr = 0x4e;

constexpr u8 kSpr0 = 1 << 0;
constexpr u8 kSpr1 = 1 << 1;
constexpr u8 kCpha = 1 << 2;
constexpr u8 kCpol = 1 << 3;
constexpr u8 kMstr = 1 << 4;
constexpr u8 kDord = 1 << 5;
constexpr u8 kSpe = 1 << 6;
constexpr u8 kSpie = 1 << 7;

constexpr u8 kSpi2x = 1 << 0;
constexpr u8 kWcol = 1 << 6;
constexpr u8 kSpif = 1 << 7;

constexpr vcml::hz_t kClockHz = 16 * vcml::MHz;

}  // namespace

// Records everything the master shifts out and answers with queued bytes.
struct SpiDevice : public vcml::component, public vcml::spi_host {
  vcml::spi_target_socket spi_in;
  std::vector<u8> received;
  std::deque<u8> responses;

  SpiDevice(const sc_module_name& nm) : component(nm), spi_host(), spi_in("spi_in") {}
  virtual ~SpiDevice() = default;

  virtual void reset() override {
    component::reset();
    received.clear();
    responses.clear();
  }

  virtual void spi_transport(const vcml::spi_target_socket&, vcml::spi_payload& spi) override {
    received.push_back(spi.mosi);
    spi.miso = 0xff;  // An idle MISO line reads as ones.
    if (!responses.empty()) {
      spi.miso = responses.front();
      responses.pop_front();
    }
  }
};

// Exposes the internal SCK frequency, which has no other observable effect.
struct SpiDut : public Spi {
  using Spi::Spi;
  vcml::hz_t SckFrequency() const { return sck_frequency_; }
};

struct SpiTest : public test_base {
  SpiDut spi;
  vcml::generic::bus bus;
  vcml::generic::clock test_clock;
  vcml::tlm_initiator_socket socket;
  SpiDevice device;
  vcml::gpio_initiator_socket ack;

  SpiTest(const sc_module_name& nm)
      : test_base(nm),
        spi("spi"),
        bus("bus"),
        test_clock("test_clock", kClockHz),
        socket("socket"),
        device("device"),
        ack("ack") {
    bus.bind(spi.in, vcml::range(kSpcr, kSpdr));
    bus.bind(socket);

    test_clock.clk.bind(spi.clk);
    test_clock.clk.bind(bus.clk);
    test_clock.clk.bind(device.clk);

    rst.bind(spi.rst);
    rst.bind(bus.rst);
    rst.bind(device.rst);

    spi.spi_out.bind(device.spi_in);
    spi.irq.stub();
    ack.bind(spi.ack);

    add_test("init", &SpiTest::TestInit);
    add_test("spcr_access", &SpiTest::TestSpcrAccess);
    add_test("spsr_access", &SpiTest::TestSpsrAccess);
    add_test("master_transmit", &SpiTest::TestMasterTransmit);
    add_test("master_receive", &SpiTest::TestMasterReceive);
    add_test("master_multiple_bytes", &SpiTest::TestMasterMultipleBytes);
    add_test("data_order_and_mode", &SpiTest::TestDataOrderAndMode);
    add_test("spif_without_irq", &SpiTest::TestSpifWithoutIrq);
    add_test("spif_cleared_by_spsr_then_spdr", &SpiTest::TestSpifClearedBySpsrThenSpdr);
    add_test("spif_kept_without_spsr_read", &SpiTest::TestSpifKeptWithoutSpsrRead);
    add_test("irq", &SpiTest::TestIrq);
    add_test("irq_ack", &SpiTest::TestIrqAck);
    add_test("irq_follows_spie", &SpiTest::TestIrqFollowsSpie);
    add_test("disabled", &SpiTest::TestDisabled);
    add_test("slave_does_not_drive_clock", &SpiTest::TestSlaveDoesNotDriveClock);
    add_test("clock_rate", &SpiTest::TestClockRate);
    add_test("reset", &SpiTest::TestReset);
  }

  void Write(uint64_t addr, u8 val) { ASSERT_OK(socket.writew<u8>(addr, val)); }

  u8 Read(uint64_t addr) {
    u8 val = 0;
    EXPECT_OK(socket.readw<u8>(addr, val));
    return val;
  }

  // Eight bits at the slowest rate (fosc/128) take 64 us.
  void WaitForTransfer() { sc_core::wait(1, sc_core::SC_MS); }

  void Transfer(u8 val) {
    Write(kSpdr, val);
    WaitForTransfer();
  }

  void TestInit() {
    EXPECT_EQ(Read(kSpcr), 0x00);
    EXPECT_EQ(Read(kSpsr), 0x00);
    EXPECT_FALSE(spi.irq.read());
    EXPECT_TRUE(device.received.empty());
  }

  void TestSpcrAccess() {
    Write(kSpcr, 0xff);
    EXPECT_EQ(Read(kSpcr), 0xff) << "All SPCR bits are read/write";
    Write(kSpcr, 0x00);
    EXPECT_EQ(Read(kSpcr), 0x00);
    Write(kSpcr, kSpe | kMstr | kSpr0);
    EXPECT_EQ(Read(kSpcr), kSpe | kMstr | kSpr0);
  }

  void TestSpsrAccess() {
    Write(kSpsr, 0xff);
    EXPECT_EQ(Read(kSpsr), kSpi2x) << "Only SPI2X is writable";
    EXPECT_FALSE(spi.irq.read()) << "SPIF cannot be set by software";

    Write(kSpsr, 0x00);
    EXPECT_EQ(Read(kSpsr), 0x00);
  }

  void TestMasterTransmit() {
    Write(kSpcr, kSpe | kMstr);
    Transfer(0xa5);

    ASSERT_EQ(device.received.size(), 1u) << "Writing SPDR starts exactly one transfer";
    EXPECT_EQ(device.received[0], 0xa5);
    EXPECT_TRUE(Read(kSpsr) & kSpif) << "SPIF is set once the byte has been shifted";
  }

  void TestMasterReceive() {
    Write(kSpcr, kSpe | kMstr);
    device.responses.push_back(0x3c);
    Transfer(0x00);

    EXPECT_TRUE(Read(kSpsr) & kSpif);
    EXPECT_EQ(Read(kSpdr), 0x3c) << "SPDR reads the byte shifted in on MISO";
    EXPECT_EQ(Read(kSpdr), 0x3c) << "The receive buffer keeps the last byte";
  }

  void TestMasterMultipleBytes() {
    const std::vector<u8> tx = { 0x01, 0x80, 0xff, 0x00, 0x5a };
    const std::vector<u8> rx = { 0xde, 0xad, 0xbe, 0xef, 0x42 };
    device.responses.assign(rx.begin(), rx.end());

    Write(kSpcr, kSpe | kMstr);
    for (size_t i = 0; i < tx.size(); i++) {
      Transfer(tx[i]);
      ASSERT_TRUE(Read(kSpsr) & kSpif) << "byte " << i;
      EXPECT_EQ(Read(kSpdr), rx[i]) << "byte " << i;
    }

    EXPECT_EQ(device.received, tx);
  }

  // Bit order and clock phase/polarity only affect the wire format, so the
  // byte seen by the device must not change.
  void TestDataOrderAndMode() {
    const u8 configs[] = { kDord, kCpol, kCpha, kCpol | kCpha, kDord | kCpol | kCpha };
    for (u8 config : configs) {
      device.received.clear();
      device.responses.push_back(0x81);

      Write(kSpcr, kSpe | kMstr | config);
      Transfer(0x1e);

      ASSERT_EQ(device.received.size(), 1u) << "config " << (int)config;
      EXPECT_EQ(device.received[0], 0x1e) << "config " << (int)config;
      EXPECT_TRUE(Read(kSpsr) & kSpif) << "config " << (int)config;
      EXPECT_EQ(Read(kSpdr), 0x81) << "config " << (int)config;
    }
  }

  // The datasheet example polls SPIF with the interrupt disabled.
  void TestSpifWithoutIrq() {
    Write(kSpcr, kSpe | kMstr);
    Transfer(0x55);

    EXPECT_TRUE(Read(kSpsr) & kSpif) << "SPIF must be set even if SPIE is cleared";
    EXPECT_FALSE(spi.irq.read()) << "No interrupt request without SPIE";
  }

  void TestSpifClearedBySpsrThenSpdr() {
    Write(kSpcr, kSpe | kMstr);
    Transfer(0x55);

    ASSERT_TRUE(Read(kSpsr) & kSpif);
    EXPECT_TRUE(Read(kSpsr) & kSpif) << "Reading SPSR alone does not clear SPIF";
    Read(kSpdr);
    EXPECT_FALSE(Read(kSpsr) & kSpif) << "Reading SPSR, then SPDR clears SPIF";

    // Writing SPDR is an access as well: the flag of the previous transfer
    // must not survive until the next one completes.
    Transfer(0x56);
    ASSERT_TRUE(Read(kSpsr) & kSpif);
    Transfer(0x57);
    EXPECT_TRUE(Read(kSpsr) & kSpif);
    EXPECT_EQ(device.received.size(), 3u);
  }

  void TestSpifKeptWithoutSpsrRead() {
    Write(kSpcr, kSpe | kMstr);
    Transfer(0x55);

    Read(kSpdr);
    EXPECT_TRUE(Read(kSpsr) & kSpif) << "SPDR access without reading SPSR first keeps SPIF";
  }

  void TestIrq() {
    Write(kSpcr, kSpe | kMstr | kSpie);
    EXPECT_FALSE(spi.irq.read()) << "No interrupt request before a transfer";

    Transfer(0x55);
    EXPECT_TRUE(Read(kSpsr) & kSpif);
    EXPECT_TRUE(spi.irq.read()) << "Transfer complete requests an interrupt if SPIE is set";

    Read(kSpdr);
    EXPECT_FALSE(Read(kSpsr) & kSpif);
    EXPECT_FALSE(spi.irq.read()) << "Clearing SPIF withdraws the interrupt request";
  }

  void TestIrqAck() {
    Write(kSpcr, kSpe | kMstr | kSpie);
    device.responses.push_back(0x77);
    Transfer(0x55);
    ASSERT_TRUE(spi.irq.read());

    ack.pulse();
    sc_core::wait(sc_core::SC_ZERO_TIME);

    EXPECT_FALSE(spi.irq.read()) << "Executing the interrupt vector withdraws the request";
    EXPECT_FALSE(Read(kSpsr) & kSpif) << "Executing the interrupt vector clears SPIF";
    EXPECT_EQ(Read(kSpdr), 0x77) << "Received data survives the acknowledge";

    Transfer(0x56);
    EXPECT_TRUE(spi.irq.read()) << "The next transfer requests an interrupt again";
  }

  void TestIrqFollowsSpie() {
    Write(kSpcr, kSpe | kMstr);
    Transfer(0x55);
    ASSERT_TRUE(Read(kSpsr) & kSpif);
    ASSERT_FALSE(spi.irq.read());

    Write(kSpcr, kSpe | kMstr | kSpie);
    EXPECT_TRUE(spi.irq.read()) << "Setting SPIE with SPIF pending requests an interrupt";

    Write(kSpcr, kSpe | kMstr);
    EXPECT_FALSE(spi.irq.read()) << "Clearing SPIE withdraws the interrupt request";
    EXPECT_TRUE(Read(kSpsr) & kSpif) << "Clearing SPIE does not clear SPIF";
  }

  void TestDisabled() {
    Write(kSpcr, kMstr | kSpie);
    Transfer(0x55);

    EXPECT_TRUE(device.received.empty()) << "SPE must be set to enable any SPI operation";
    EXPECT_FALSE(Read(kSpsr) & kSpif);
    EXPECT_FALSE(spi.irq.read());
  }

  // A slave only shifts on clock pulses of an external master.
  void TestSlaveDoesNotDriveClock() {
    Write(kSpcr, kSpe | kSpie);
    Transfer(0x55);

    EXPECT_TRUE(device.received.empty()) << "A slave must not start a transfer on its own";
    EXPECT_FALSE(Read(kSpsr) & kSpif);
    EXPECT_FALSE(spi.irq.read());
  }

  // Datasheet table 18-5.
  void TestClockRate() {
    struct Rate {
      bool spi2x;
      u8 spr;
      vcml::hz_t divider;
    };
    const Rate rates[] = {
      { false, 0, 4 }, { false, 1, 16 }, { false, 2, 64 }, { false, 3, 128 },
      { true, 0, 2 },  { true, 1, 8 },   { true, 2, 32 },  { true, 3, 64 },
    };

    EXPECT_EQ(spi.SckFrequency(), kClockHz / 4) << "Reset value";

    for (const Rate& rate : rates) {
      Write(kSpsr, rate.spi2x ? kSpi2x : 0);
      Write(kSpcr, kSpe | kMstr | rate.spr);
      EXPECT_EQ(spi.SckFrequency(), kClockHz / rate.divider)
          << "SPI2X=" << rate.spi2x << " SPR=" << (int)rate.spr;
    }

    // SPI2X written after SPCR must take effect as well.
    Write(kSpsr, 0);
    Write(kSpcr, kSpe | kMstr | kSpr0);
    Write(kSpsr, kSpi2x);
    EXPECT_EQ(spi.SckFrequency(), kClockHz / 8);
  }

  void TestReset() {
    Write(kSpsr, kSpi2x);
    Write(kSpcr, kSpe | kMstr | kSpie | kSpr1);
    Transfer(0x55);
    ASSERT_TRUE(spi.irq.read());

    spi.reset();

    EXPECT_EQ(Read(kSpcr), 0x00);
    EXPECT_EQ(Read(kSpsr), 0x00);
    EXPECT_FALSE(spi.irq.read());
    EXPECT_EQ(spi.SckFrequency(), kClockHz / 4);

    device.received.clear();
    Transfer(0x55);
    EXPECT_TRUE(device.received.empty()) << "SPI is disabled after reset";
    EXPECT_FALSE(Read(kSpsr) & kSpif);
  }
};

TEST(spi, simulate) {
  SpiTest test("spi_test");
  sc_core::sc_start();
}
