#include "twi.h"

#include "testing.h"

static constexpr unsigned int TWBR = 0xb8;
static constexpr unsigned int TWSR = 0xb9;
static constexpr unsigned int TWAR = 0xba;
static constexpr unsigned int TWDR = 0xbb;
static constexpr unsigned int TWCR = 0xbc;
static constexpr unsigned int TWAMR = 0xbd;

static constexpr u8 TWINT = 0x80;
static constexpr u8 TWEA = 0x40;
static constexpr u8 TWSTA = 0x20;
static constexpr u8 TWSTO = 0x10;
static constexpr u8 TWWC = 0x08;
static constexpr u8 TWEN = 0x04;
static constexpr u8 TWIE = 0x01;

static constexpr u8 SLAVE_ADDR = 0x50;

struct TwiTest : public test_base, vcml::i2c_host {
  Twi twi;
  vcml::generic::bus bus;
  vcml::generic::clock test_clock;
  vcml::tlm_initiator_socket socket;

  vcml::i2c_target_socket in_twi;
  vcml::i2c_initiator_socket ext_master;

  vcml::gpio_initiator_socket ack_twi;
  vcml::gpio_target_socket irq_sink;

  std::vector<u8> slave_received;
  std::vector<u8> slave_to_send;
  size_t slave_send_pos;
  bool slave_nack_writes;
  int slave_start_count;
  int slave_stop_count;
  vcml::tlm_command slave_last_cmd;

  // Records every actual level change seen on twi.irq, as opposed to just
  // its final value: the CPU's interrupt controller only redelivers an
  // interrupt on an observed edge, so a bug that leaves the signal
  // continuously high across two distinct events would still pass a
  // plain "is it asserted" check.
  std::vector<bool> irq_events;

  TwiTest(const sc_module_name& nm)
      : test_base(nm),
        vcml::i2c_host(),
        twi("twi"),
        bus("bus"),
        test_clock("test_clock", 16 * vcml::MHz),
        socket("socket"),
        in_twi("in_twi"),
        ext_master("ext_master"),
        ack_twi("ack_twi"),
        irq_sink("irq_sink"),
        slave_send_pos(0),
        slave_nack_writes(false),
        slave_start_count(0),
        slave_stop_count(0),
        slave_last_cmd(vcml::TLM_IGNORE_COMMAND) {
    bus.bind(twi.in, vcml::range(0xb8, 0xbd), 0xb8);
    bus.bind(socket);

    test_clock.clk.bind(twi.clk);
    test_clock.clk.bind(bus.clk);

    rst.bind(twi.rst);
    rst.bind(bus.rst);

    in_twi.set_address(SLAVE_ADDR);
    twi.out_i2c.bind(in_twi);
    ext_master.bind(twi.in_i2c);

    ack_twi.bind(twi.ack);
    twi.irq.bind(irq_sink);

    add_test("init", &TwiTest::TestInit);
    add_test("master_transmit", &TwiTest::TestMasterTransmit);
    add_test("master_transmit_nack", &TwiTest::TestMasterTransmitNack);
    add_test("master_receive", &TwiTest::TestMasterReceive);
    add_test("repeated_start", &TwiTest::TestRepeatedStart);
    add_test("write_collision", &TwiTest::TestWriteCollision);
    add_test("twsr_prescaler_bits", &TwiTest::TestTwsrPrescalerBits);
    add_test("twen_aborts_transaction", &TwiTest::TestTwenAbortsTransaction);
    add_test("irq_not_cleared_by_ack", &TwiTest::TestIrqNotClearedByAck);
    add_test("twint_preserved_without_clear", &TwiTest::TestTwintPreservedWithoutClear);
    add_test("slave_receiver", &TwiTest::TestSlaveReceiver);
    add_test("slave_receiver_nack", &TwiTest::TestSlaveReceiverNack);
    add_test("slave_transmitter", &TwiTest::TestSlaveTransmitter);
    add_test("slave_general_call", &TwiTest::TestSlaveGeneralCall);
    add_test("slave_address_mismatch", &TwiTest::TestSlaveAddressMismatch);
    add_test("slave_address_mask", &TwiTest::TestSlaveAddressMask);
    add_test("irq_refires_across_consecutive_events", &TwiTest::TestIrqRefiresAcrossConsecutiveEvents);
  }

  virtual void reset() override {
    slave_received.clear();
    slave_to_send.clear();
    slave_send_pos = 0;
    slave_nack_writes = false;
    slave_start_count = 0;
    slave_stop_count = 0;
    slave_last_cmd = vcml::TLM_IGNORE_COMMAND;
    irq_events.clear();
  }

  virtual void gpio_notify(const vcml::gpio_target_socket& socket, bool state) override {
    if (socket == irq_sink)
      irq_events.push_back(state);
  }

  u8 Rd(unsigned int addr) {
    u8 val = 0;
    socket.readw<u8>(addr, val);
    return val;
  }

  void Wr(unsigned int addr, u8 val) { socket.writew<u8>(addr, val); }

  virtual vcml::i2c_response i2c_start(const vcml::i2c_target_socket&, vcml::tlm_command cmd) override {
    slave_start_count++;
    slave_last_cmd = cmd;
    slave_send_pos = 0;
    return vcml::I2C_ACK;
  }

  virtual vcml::i2c_response i2c_stop(const vcml::i2c_target_socket&) override {
    slave_stop_count++;
    return vcml::I2C_ACK;
  }

  virtual vcml::i2c_response i2c_write(const vcml::i2c_target_socket&, u8 data) override {
    slave_received.push_back(data);
    return slave_nack_writes ? vcml::I2C_NACK : vcml::I2C_ACK;
  }

  virtual vcml::i2c_response i2c_read(const vcml::i2c_target_socket&, u8& data) override {
    data = slave_send_pos < slave_to_send.size() ? slave_to_send[slave_send_pos++] : (u8)0xff;
    return vcml::I2C_ACK;
  }

  void TestInit() {
    ASSERT_EQ((u8)twi.twbr, (u8)0x00);
    ASSERT_EQ((u8)twi.twcr, (u8)0x00);
    ASSERT_EQ((u8)twi.twsr, (u8)0xf8);
    ASSERT_EQ((u8)twi.twdr, (u8)0xff);
    ASSERT_EQ((u8)twi.twar, (u8)0xfe);
    ASSERT_EQ((u8)twi.twamr, (u8)0x00);
    ASSERT_FALSE(twi.irq.read());
  }

  void TestMasterTransmit() {
    Wr(TWCR, TWEN | TWSTA | TWINT);
    ASSERT_EQ(Rd(TWSR), (u8)0x08) << "START should be signalled";
    ASSERT_TRUE(Rd(TWCR) & TWINT);

    Wr(TWDR, (u8)(SLAVE_ADDR << 1));
    Wr(TWCR, TWEN | TWINT);
    ASSERT_EQ(Rd(TWSR), (u8)0x18) << "SLA+W should be ACKed";
    ASSERT_EQ(slave_start_count, 1);
    ASSERT_EQ(slave_last_cmd, vcml::TLM_WRITE_COMMAND);

    Wr(TWDR, (u8)0x42);
    Wr(TWCR, TWEN | TWINT);
    ASSERT_EQ(Rd(TWSR), (u8)0x28) << "data byte should be ACKed";
    ASSERT_EQ(slave_received.size(), (size_t)1);
    ASSERT_EQ(slave_received[0], (u8)0x42);

    Wr(TWCR, TWEN | TWSTO | TWINT);
    ASSERT_EQ(Rd(TWSR), (u8)0xf8);
    ASSERT_FALSE(Rd(TWCR) & TWINT) << "TWINT must not be set after a STOP";
    ASSERT_EQ(slave_stop_count, 1);
  }

  void TestMasterTransmitNack() {
    Wr(TWCR, TWEN | TWSTA | TWINT);
    ASSERT_EQ(Rd(TWSR), (u8)0x08);

    Wr(TWDR, (u8)(0x51 << 1));
    Wr(TWCR, TWEN | TWINT);
    ASSERT_EQ(Rd(TWSR), (u8)0x20) << "SLA+W to unknown address should be NACKed";
    ASSERT_EQ(slave_start_count, 0) << "mismatched address must not reach the slave";

    Wr(TWCR, TWEN | TWSTO | TWINT);
    ASSERT_EQ(Rd(TWSR), (u8)0xf8);
  }

  void TestMasterReceive() {
    slave_to_send = {0x11, 0x22};

    Wr(TWCR, TWEN | TWSTA | TWINT);
    ASSERT_EQ(Rd(TWSR), (u8)0x08);

    Wr(TWDR, (u8)((SLAVE_ADDR << 1) | 1));
    Wr(TWCR, TWEN | TWINT);
    ASSERT_EQ(Rd(TWSR), (u8)0x40) << "SLA+R should be ACKed";
    ASSERT_EQ(slave_last_cmd, vcml::TLM_READ_COMMAND);

    Wr(TWCR, TWEN | TWEA | TWINT);
    ASSERT_EQ(Rd(TWSR), (u8)0x50);
    ASSERT_EQ(Rd(TWDR), (u8)0x11);

    Wr(TWCR, TWEN | TWINT);
    ASSERT_EQ(Rd(TWSR), (u8)0x58);
    ASSERT_EQ(Rd(TWDR), (u8)0x22);

    Wr(TWCR, TWEN | TWSTO | TWINT);
    ASSERT_EQ(slave_stop_count, 1);
  }

  void TestRepeatedStart() {
    Wr(TWCR, TWEN | TWSTA | TWINT);
    Wr(TWDR, (u8)(SLAVE_ADDR << 1));
    Wr(TWCR, TWEN | TWINT);
    ASSERT_EQ(Rd(TWSR), (u8)0x18);

    Wr(TWCR, TWEN | TWSTA | TWINT);
    ASSERT_EQ(Rd(TWSR), (u8)0x10);
    ASSERT_EQ(slave_stop_count, 0) << "a repeated start must not emit a STOP";

    Wr(TWDR, (u8)((SLAVE_ADDR << 1) | 1));
    Wr(TWCR, TWEN | TWINT);
    ASSERT_EQ(Rd(TWSR), (u8)0x40);
    ASSERT_EQ(slave_start_count, 2);

    Wr(TWCR, TWEN | TWSTO | TWINT);
    ASSERT_EQ(slave_stop_count, 1);
  }

  void TestWriteCollision() {
    ASSERT_FALSE(Rd(TWCR) & TWINT);
    Wr(TWDR, (u8)0x55);
    ASSERT_EQ(Rd(TWDR), (u8)0xff) << "a write while TWINT is low must be discarded";
    ASSERT_TRUE(Rd(TWCR) & TWWC);

    Wr(TWCR, TWEN | TWSTA | TWINT);
    ASSERT_TRUE(Rd(TWCR) & TWINT);
    Wr(TWDR, (u8)0x66);
    ASSERT_EQ(Rd(TWDR), (u8)0x66) << "a write while TWINT is high must succeed";
    ASSERT_FALSE(Rd(TWCR) & TWWC) << "TWWC must be cleared by a successful TWDR write";
  }

  void TestTwsrPrescalerBits() {
    ASSERT_EQ(Rd(TWSR), (u8)0xf8);

    Wr(TWSR, (u8)0xff);
    ASSERT_EQ(Rd(TWSR), (u8)0xfb) << "only the prescaler bits are software-writable";

    Wr(TWCR, TWEN | TWSTA | TWINT);
    ASSERT_EQ(Rd(TWSR), (u8)0x0b) << "status bits update, prescaler bits are preserved";
  }

  void TestTwenAbortsTransaction() {
    Wr(TWCR, TWEN | TWSTA | TWINT);
    Wr(TWDR, (u8)(SLAVE_ADDR << 1));
    Wr(TWCR, TWEN | TWINT);
    ASSERT_EQ(Rd(TWSR), (u8)0x18);

    Wr(TWCR, 0x00);
    ASSERT_EQ(Rd(TWSR), (u8)0xf8);
    ASSERT_FALSE(Rd(TWCR) & (TWEN | TWINT));
    ASSERT_FALSE(twi.irq.read());
    ASSERT_EQ(slave_stop_count, 1) << "disabling TWEN mid-transaction must release the bus";
  }

  void TestIrqNotClearedByAck() {
    Wr(TWCR, TWEN | TWIE | TWSTA | TWINT);
    ASSERT_TRUE(twi.irq.read());

    ack_twi.pulse();

    ASSERT_TRUE(twi.irq.read()) << "TWINT is explicitly not auto-cleared on interrupt entry";
    ASSERT_TRUE(Rd(TWCR) & TWINT);
  }

  void TestTwintPreservedWithoutClear() {
    Wr(TWCR, TWEN | TWIE | TWSTA | TWINT);
    ASSERT_TRUE(Rd(TWCR) & TWINT);
    u8 status_before = Rd(TWSR);

    Wr(TWCR, TWEN | TWIE | TWEA);
    ASSERT_TRUE(Rd(TWCR) & TWINT) << "writing a zero to TWINT must leave it untouched";
    ASSERT_EQ(Rd(TWSR), status_before);
    ASSERT_TRUE(Rd(TWCR) & TWEA);
  }

  void TestSlaveReceiver() {
    Wr(TWAR, (u8)(0x30 << 1));
    Wr(TWCR, TWEN | TWEA | TWIE);

    ASSERT_EQ(ext_master.start(0x30, vcml::TLM_WRITE_COMMAND), vcml::I2C_ACK);
    ASSERT_EQ(Rd(TWSR), (u8)0x60);
    ASSERT_TRUE(twi.irq.read());

    u8 data = 0x77;
    ASSERT_EQ(ext_master.transport(data), vcml::I2C_ACK);
    ASSERT_EQ(Rd(TWDR), (u8)0x77);
    ASSERT_EQ(Rd(TWSR), (u8)0x80);

    Wr(TWCR, TWEN | TWEA | TWIE | TWINT);
    ASSERT_FALSE(Rd(TWCR) & TWINT);

    ASSERT_EQ(ext_master.stop(), vcml::I2C_ACK);
    ASSERT_EQ(Rd(TWSR), (u8)0xa0);
  }

  void TestSlaveReceiverNack() {
    Wr(TWAR, (u8)(0x30 << 1));
    Wr(TWCR, TWEN | TWEA);

    ASSERT_EQ(ext_master.start(0x30, vcml::TLM_WRITE_COMMAND), vcml::I2C_ACK);
    ASSERT_EQ(Rd(TWSR), (u8)0x60);

    Wr(TWCR, TWEN | TWINT);

    u8 data = 0x11;
    ASSERT_EQ(ext_master.transport(data), vcml::I2C_NACK);
    ASSERT_EQ(Rd(TWSR), (u8)0x88);
  }

  void TestSlaveTransmitter() {
    Wr(TWAR, (u8)(0x30 << 1));
    Wr(TWCR, TWEN | TWEA);

    ASSERT_EQ(ext_master.start(0x30, vcml::TLM_READ_COMMAND), vcml::I2C_ACK);
    ASSERT_EQ(Rd(TWSR), (u8)0xa8);

    Wr(TWDR, (u8)0x99);
    Wr(TWCR, TWEN | TWEA | TWINT);

    u8 data = 0;
    ASSERT_EQ(ext_master.transport(data), vcml::I2C_ACK);
    ASSERT_EQ(data, (u8)0x99);
    ASSERT_EQ(Rd(TWSR), (u8)0xb8);

    Wr(TWDR, (u8)0xaa);
    Wr(TWCR, TWEN | TWINT);

    data = 0;
    ASSERT_EQ(ext_master.transport(data), vcml::I2C_NACK);
    ASSERT_EQ(data, (u8)0xaa);
    ASSERT_EQ(Rd(TWSR), (u8)0xc0);
  }

  void TestSlaveGeneralCall() {
    Wr(TWAR, (u8)((0x31 << 1) | 0x01));
    Wr(TWCR, TWEN | TWEA);

    ASSERT_EQ(ext_master.start(0x00, vcml::TLM_WRITE_COMMAND), vcml::I2C_ACK);
    ASSERT_EQ(Rd(TWSR), (u8)0x70);

    u8 data = 0x55;
    ASSERT_EQ(ext_master.transport(data), vcml::I2C_ACK);
    ASSERT_EQ(Rd(TWSR), (u8)0x90);
    ASSERT_EQ(Rd(TWDR), (u8)0x55);
  }

  void TestSlaveAddressMismatch() {
    Wr(TWAR, (u8)(0x30 << 1));
    Wr(TWCR, TWEN | TWEA);

    ASSERT_EQ(ext_master.start(0x31, vcml::TLM_WRITE_COMMAND), vcml::I2C_NACK);
    ASSERT_EQ(Rd(TWSR), (u8)0xf8) << "status must stay idle for a non-matching address";
    ASSERT_FALSE(twi.irq.read());
  }

  void TestSlaveAddressMask() {
    Wr(TWAR, (u8)(0x30 << 1));
    Wr(TWAMR, (u8)(0x01 << 1));
    Wr(TWCR, TWEN | TWEA);

    ASSERT_EQ(ext_master.start(0x30, vcml::TLM_WRITE_COMMAND), vcml::I2C_ACK);
    ASSERT_EQ(Rd(TWSR), (u8)0x60);
    ext_master.stop();

    ASSERT_EQ(ext_master.start(0x31, vcml::TLM_WRITE_COMMAND), vcml::I2C_ACK) << "masked bit should still match";
    ASSERT_EQ(Rd(TWSR), (u8)0x60);
    ext_master.stop();

    ASSERT_EQ(ext_master.start(0x32, vcml::TLM_WRITE_COMMAND), vcml::I2C_NACK) << "unmasked bits must still match";
  }

  void TestIrqRefiresAcrossConsecutiveEvents() {
    Wr(TWCR, TWEN | TWIE | TWSTA | TWINT);
    ASSERT_EQ(Rd(TWSR), (u8)0x08);
    ASSERT_FALSE(irq_events.empty());
    ASSERT_TRUE(irq_events.back()) << "irq must assert for the START event";

    irq_events.clear();

    // Loading SLA+W and clearing TWINT in one write both services the
    // still-pending START interrupt and, since our i2c transport is
    // synchronous, immediately raises a brand new one for the SLA+W ACK
    // within that same write. The CPU's interrupt controller only
    // redispatches on an observed edge, so if irq never actually dips low
    // in between, the second event is silently lost forever - exactly
    // what happened with the ssd1306 workload before this fix.
    Wr(TWDR, (u8)(SLAVE_ADDR << 1));
    Wr(TWCR, TWEN | TWIE | TWINT);
    ASSERT_EQ(Rd(TWSR), (u8)0x18);

    ASSERT_GE(irq_events.size(), (size_t)2) << "irq must produce a real low pulse before reasserting";
    bool saw_low = false;
    for (bool ev : irq_events)
      saw_low |= !ev;
    ASSERT_TRUE(saw_low) << "irq must actually deassert between the two events";
    ASSERT_TRUE(irq_events.back()) << "irq must be asserted again for the new SLA+W ACK event";
  }
};

TEST(ArduSystem, TwiTest) {
  TwiTest test("test");
  sc_core::sc_start();
}
