#include "timer.h"

#include "testing.h"

static constexpr mwr::u64 TIFR0 = 0x35;
static constexpr mwr::u64 TIFR1 = 0x36;
static constexpr mwr::u64 GTCCR = 0x43;
static constexpr mwr::u64 TCCR0A = 0x44;
static constexpr mwr::u64 TCCR0B = 0x45;
static constexpr mwr::u64 TCNT0 = 0x46;
static constexpr mwr::u64 OCR0A = 0x47;
static constexpr mwr::u64 OCR0B = 0x48;
static constexpr mwr::u64 TIMSK0 = 0x6e;
static constexpr mwr::u64 TIMSK1 = 0x6f;
static constexpr mwr::u64 TCCR1A = 0x80;
static constexpr mwr::u64 TCCR1B = 0x81;
static constexpr mwr::u64 TCCR1C = 0x82;
static constexpr mwr::u64 TCNT1L = 0x84;
static constexpr mwr::u64 TCNT1H = 0x85;
static constexpr mwr::u64 ICR1L = 0x86;
static constexpr mwr::u64 ICR1H = 0x87;
static constexpr mwr::u64 OCR1AL = 0x88;
static constexpr mwr::u64 OCR1AH = 0x89;
static constexpr mwr::u64 OCR1BL = 0x8a;
static constexpr mwr::u64 OCR1BH = 0x8b;

struct TimerTest : public test_base {
  Timer timer;
  vcml::generic::bus bus;
  vcml::generic::clock test_clock;
  vcml::tlm_initiator_socket socket;

  TimerTest(const sc_module_name& nm)
      : test_base(nm), timer("timer"), bus("bus"), test_clock("test_clock", 16 * vcml::MHz), socket("socket") {
    bus.bind(timer.in, vcml::range(0xc0, 0xc6));
    bus.bind(socket);

    bus.bind(timer.in, vcml::range(0x35, 0x37), 0x35);
    bus.bind(timer.in, vcml::range(0x43, 0x48), 0x43);
    bus.bind(timer.in, vcml::range(0x6e, 0x70), 0x6e);
    bus.bind(timer.in, vcml::range(0x80, 0x8b), 0x80);
    bus.bind(timer.in, vcml::range(0xb0, 0xb6), 0xb0);

    test_clock.clk.bind(timer.clk);
    test_clock.clk.bind(bus.clk);

    rst.bind(timer.rst);
    rst.bind(bus.rst);

    timer.ocie0a_out.stub();
    timer.ocie0b_out.stub();
    timer.toie0_out.stub();

    timer.ocie1a_out.stub();
    timer.ocie1b_out.stub();
    timer.toie1_out.stub();
    timer.icie1_out.stub();

    timer.ocie2a_out.stub();
    timer.ocie2b_out.stub();
    timer.toie2_out.stub();

    add_test("init", &TimerTest::TestInit);
    add_test("reserved_bits", &TimerTest::TestReservedBits);
    add_test("count", &TimerTest::TestCount);
    add_test("overflow", &TimerTest::TestOverflow);
    add_test("compare", &TimerTest::TestCompare);
  }

  void TestInit() {
    ASSERT_EQ((uint)timer.gtccr, 0);

    ASSERT_EQ((uint)timer.tifr0, 0);
    ASSERT_EQ((uint)timer.tccr0a, 0);
    ASSERT_EQ((uint)timer.tccr0b, 0);
    ASSERT_EQ((uint)timer.tcnt0, 0);
    ASSERT_EQ((uint)timer.ocr0a, 0);
    ASSERT_EQ((uint)timer.ocr0b, 0);
    ASSERT_EQ((uint)timer.timsk0, 0);

    ASSERT_EQ((uint)timer.tifr1, 0);
    ASSERT_EQ((uint)timer.tccr1a, 0);
    ASSERT_EQ((uint)timer.tccr1b, 0);
    ASSERT_EQ((uint)timer.tccr1c, 0);
    ASSERT_EQ((uint)timer.tcnt1l, 0);
    ASSERT_EQ((uint)timer.tcnt1h, 0);

    ASSERT_EQ((uint)timer.icr1l, 0);
    ASSERT_EQ((uint)timer.icr1h, 0);
    ASSERT_EQ((uint)timer.ocr1al, 0);
    ASSERT_EQ((uint)timer.ocr1ah, 0);
    ASSERT_EQ((uint)timer.ocr1bl, 0);
    ASSERT_EQ((uint)timer.ocr1bh, 0);
    ASSERT_EQ((uint)timer.timsk1, 0);

    ASSERT_FALSE(timer.ocie0a_out);
    ASSERT_FALSE(timer.ocie0b_out);
    ASSERT_FALSE(timer.toie0_out);

    ASSERT_FALSE(timer.ocie1a_out);
    ASSERT_FALSE(timer.ocie1b_out);
    ASSERT_FALSE(timer.toie1_out);
    ASSERT_FALSE(timer.icie1_out);

    ASSERT_FALSE(timer.ocie2a_out);
    ASSERT_FALSE(timer.ocie2b_out);
    ASSERT_FALSE(timer.toie2_out);
  }

  void TestReservedBits() {
    u8 data;

    socket.writew<u8>(GTCCR, (u8)0xff);
    socket.readw<u8>(GTCCR, data);
    ASSERT_EQ((uint)data, 0b10000000);

    socket.writew<u8>(TIFR0, (u8)0xff);
    socket.readw<u8>(TIFR0, data);
    ASSERT_EQ((uint)data, 0);

    socket.writew<u8>(TCCR0A, (u8)0xff);
    socket.readw<u8>(TCCR0A, data);
    ASSERT_EQ((uint)data, 0b11110011);

    socket.writew<u8>(TCCR0B, (u8)0xff);
    socket.readw<u8>(TCCR0B, data);
    ASSERT_EQ((uint)data, 0b00001111);

    socket.writew<u8>(TCNT0, (u8)0xff);
    socket.readw<u8>(TCNT0, data);
    ASSERT_EQ((uint)data, 0b11111111);

    socket.writew<u8>(OCR0A, (u8)0xff);
    socket.readw<u8>(OCR0A, data);
    ASSERT_EQ((uint)data, 0b11111111);

    socket.writew<u8>(OCR0B, (u8)0xff);
    socket.readw<u8>(OCR0B, data);
    ASSERT_EQ((uint)data, 0b11111111);

    socket.writew<u8>(TIMSK0, (u8)0xff);
    socket.readw<u8>(TIMSK0, data);
    ASSERT_EQ((uint)data, 0b111);

    socket.writew<u8>(TIFR1, (u8)0xff);
    socket.readw<u8>(TIFR1, data);
    ASSERT_EQ((uint)data, 0);

    socket.writew<u8>(TCCR1A, (u8)0xff);
    socket.readw<u8>(TCCR1A, data);
    ASSERT_EQ((uint)data, 0b11110011);

    socket.writew<u8>(TCCR1B, (u8)0xff);
    socket.readw<u8>(TCCR1B, data);
    ASSERT_EQ((uint)data, 0b11011111);

    socket.writew<u8>(TCCR1C, (u8)0xff);
    socket.readw<u8>(TCCR1C, data);
    ASSERT_EQ((uint)data, 0);

    socket.writew<u8>(TCNT1H, (u8)0xff);
    socket.writew<u8>(TCNT1L, (u8)0xff);
    socket.readw<u8>(TCNT1L, data);
    ASSERT_EQ((uint)data, 0b11111111);
    socket.readw<u8>(TCNT1H, data);
    ASSERT_EQ((uint)data, 0b11111111);

    socket.writew<u8>(ICR1H, (u8)0xff);
    socket.writew<u8>(ICR1L, (u8)0xff);
    socket.readw<u8>(ICR1L, data);
    ASSERT_EQ((uint)data, 0b11111111);
    socket.readw<u8>(ICR1H, data);
    ASSERT_EQ((uint)data, 0b11111111);

    socket.writew<u8>(OCR1AH, (u8)0xff);
    socket.writew<u8>(OCR1AL, (u8)0xff);
    socket.readw<u8>(OCR1AL, data);
    ASSERT_EQ((uint)data, 0b11111111);
    socket.readw<u8>(OCR1AH, data);
    ASSERT_EQ((uint)data, 0b11111111);

    socket.writew<u8>(OCR1BH, (u8)0xff);
    socket.writew<u8>(OCR1BL, (u8)0xff);
    socket.readw<u8>(OCR1BL, data);
    ASSERT_EQ((uint)data, 0b11111111);
    socket.readw<u8>(OCR1BH, data);
    ASSERT_EQ((uint)data, 0b11111111);

    socket.writew<u8>(TIMSK1, (u8)0xff);
    socket.readw<u8>(TIMSK1, data);
    ASSERT_EQ((uint)data, 0b00100111);
  }

  void TestCount() {
    ASSERT_EQ(timer.tcnt0, 0);
    sc_core::wait(10, sc_core::SC_US);
    ASSERT_EQ(timer.tcnt0, 0);

    socket.writew<u8>(TCCR0B, (u8)0x01);
    sc_core::wait(10, sc_core::SC_US);
    sc_core::wait(SC_ZERO_TIME);
    ASSERT_EQ((uint)timer.tcnt0, 160);

    socket.writew<u8>(TCCR0B, (u8)0x02);
    sc_core::wait(10, sc_core::SC_US);
    sc_core::wait(SC_ZERO_TIME);
    ASSERT_EQ((uint)timer.tcnt0, 160 + 20);

    socket.writew<u8>(TCCR1B, (u8)0x01);
    sc_core::wait(10, sc_core::SC_US);
    sc_core::wait(SC_ZERO_TIME);
    ASSERT_EQ((uint)timer.tcnt1l, 160);

    socket.writew<u8>(TCCR1B, (u8)0x02);
    sc_core::wait(10, sc_core::SC_US);
    sc_core::wait(SC_ZERO_TIME);
    ASSERT_EQ((uint)timer.tcnt1l, 160 + 20);

    sc_core::wait(100, sc_core::SC_US);
    sc_core::wait(SC_ZERO_TIME);
    ASSERT_EQ((uint)timer.tcnt1l, (160 + 20 + 200) % 256);
    ASSERT_EQ((uint)timer.tcnt1h, (160 + 20 + 200) / 256);
  }

  void TestOverflow() {
    socket.writew<u8>(TCCR0B, (u8)0x01);

    sc_core::wait(15, sc_core::SC_US);
    sc_core::wait(SC_ZERO_TIME);
    ASSERT_EQ((uint)timer.tcnt0, 240);
    ASSERT_FALSE(timer.tifr0 & 1);
    ASSERT_FALSE(timer.toie0_out);

    sc_core::wait(1, sc_core::SC_US);
    sc_core::wait(SC_ZERO_TIME);
    ASSERT_EQ((uint)timer.tcnt0, 0);
    ASSERT_TRUE(timer.tifr0 & 1);
    ASSERT_FALSE(timer.toie0_out);
    socket.writew<u8>(TIFR0, (u8)0x01);
    ASSERT_FALSE(timer.tifr0 & 1);

    socket.writew<u8>(TIMSK0, (u8)0x01);
    sc_core::wait(17, sc_core::SC_US);
    ASSERT_TRUE(timer.tifr0 & 1);
    ASSERT_TRUE(timer.toie0_out);

    socket.writew<u8>(TCCR1B, (u8)0x01);

    sc_core::wait(16 * 255, sc_core::SC_US);
    sc_core::wait(SC_ZERO_TIME);
    ASSERT_EQ((uint)timer.tcnt1h, 255);
    ASSERT_FALSE(timer.tifr1 & 1);
    ASSERT_FALSE(timer.toie1_out);

    sc_core::wait(16, sc_core::SC_US);
    sc_core::wait(SC_ZERO_TIME);
    ASSERT_TRUE(timer.tifr1 & 1);
    ASSERT_FALSE(timer.toie1_out);
    socket.writew<u8>(TIFR1, (u8)0x01);
    ASSERT_FALSE(timer.tifr1 & 1);

    socket.writew<u8>(TIMSK1, (u8)0x01);
    sc_core::wait(17 * 256, sc_core::SC_US);
    ASSERT_TRUE(timer.tifr1 & 1);
    ASSERT_TRUE(timer.toie1_out);
  }

  void TestCompare() {
    socket.writew<u8>(OCR0A, (u8)23);
    socket.writew<u8>(OCR0B, (u8)99);
    socket.writew<u8>(TIMSK0, 0b110);
    socket.writew<u8>(TCCR0B, (u8)0x01);

    ASSERT_EQ(timer.tcnt0, 0);
    ASSERT_FALSE(timer.tifr0 & 0b10);
    ASSERT_FALSE(timer.tifr0 & 0b100);

    sc_core::wait(1, sc_core::SC_US);

    ASSERT_FALSE(timer.tifr0 & 0b10);
    ASSERT_FALSE(timer.tifr0 & 0b100);

    sc_core::wait(1, sc_core::SC_US);

    ASSERT_TRUE(timer.tifr0 & 0b10);
    ASSERT_FALSE(timer.tifr0 & 0b100);

    sc_core::wait(5, sc_core::SC_US);

    ASSERT_TRUE(timer.tifr0 & 0b10);
    ASSERT_TRUE(timer.tifr0 & 0b100);

    socket.writew<u8>(TIFR0, (u8)0b110);

    ASSERT_FALSE(timer.tifr0 & 0b10);
    ASSERT_FALSE(timer.tifr0 & 0b100);

    socket.writew<u8>(OCR1AH, (u8)23);
    socket.writew<u8>(OCR1AL, (u8)23);
    socket.writew<u8>(OCR1BH, (u8)99);
    socket.writew<u8>(OCR1BL, (u8)99);
    socket.writew<u8>(TIMSK1, 0b110);
    socket.writew<u8>(TCCR1B, (u8)0x01);

    ASSERT_EQ((uint)timer.tcnt1l, 0);
    ASSERT_EQ((uint)timer.tcnt1h, 0);
    ASSERT_FALSE(timer.tifr1 & 0b10);
    ASSERT_FALSE(timer.tifr1 & 0b100);

    sc_core::wait(1, sc_core::SC_US);

    ASSERT_FALSE(timer.tifr1 & 0b10);
    ASSERT_FALSE(timer.tifr1 & 0b100);

    sc_core::wait(16 * 25, sc_core::SC_US);

    ASSERT_TRUE(timer.tifr1 & 0b10);
    ASSERT_FALSE(timer.tifr1 & 0b100);

    sc_core::wait(16 * 75, sc_core::SC_US);

    ASSERT_TRUE(timer.tifr1 & 0b10);
    ASSERT_TRUE(timer.tifr1 & 0b100);

    socket.writew<u8>(TIFR1, (u8)0b110);

    ASSERT_FALSE(timer.tifr1 & 0b10);
    ASSERT_FALSE(timer.tifr1 & 0b100);
  }
};

TEST(ArduSystem, TimerTest) {
  TimerTest test("test");
  sc_core::sc_start();
}
