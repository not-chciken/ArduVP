/******************************************************************************
 * Apache License, Version 2.0
 * Copyright (c) 2026 chciken/Niko Zurstraßen
 *****************************************************************************/

#include "adc.h"

#include "testing.h"

static constexpr mwr::u64 ADCL = 0x78;
static constexpr mwr::u64 ADCH = 0x79;
static constexpr mwr::u64 ADCSRA = 0x7a;
static constexpr mwr::u64 ADCSRB = 0x7b;
static constexpr mwr::u64 ADMUX = 0x7c;

static constexpr u8 ADMUX_ADLAR = (u8)mwr::bit(5);
static constexpr u8 ADMUX_REFS0 = (u8)mwr::bit(6);
static constexpr u8 ADMUX_REFS1 = (u8)mwr::bit(7);

static constexpr u8 ADMUX_MUX_TEMP = 0b1000;
static constexpr u8 ADMUX_MUX_BANDGAP = 0b1110;
static constexpr u8 ADMUX_MUX_GND = 0b1111;

static constexpr u8 ADCSRA_ADIE = (u8)mwr::bit(3);
static constexpr u8 ADCSRA_ADIF = (u8)mwr::bit(4);
static constexpr u8 ADCSRA_ADATE = (u8)mwr::bit(5);
static constexpr u8 ADCSRA_ADSC = (u8)mwr::bit(6);
static constexpr u8 ADCSRA_ADEN = (u8)mwr::bit(7);

static constexpr u8 ADCSRB_ADTS0 = (u8)mwr::bit(0);
static constexpr u8 ADCSRB_ADTS1 = (u8)mwr::bit(1);
static constexpr u8 ADCSRB_ADTS2 = (u8)mwr::bit(2);
static constexpr u8 ADCSRB_ACME = (u8)mwr::bit(6);

static const sc_core::sc_time CONVERSION_TIME(10, sc_core::SC_US);

static constexpr uint NUM_ADC_PINS = 8;

struct AdcTest : public test_base {
  Adc adc;
  vcml::tlm_initiator_socket socket;
  vcml::generic::bus bus;
  vcml::gpio_initiator_socket ext_irq0;
  vcml::gpio_initiator_socket ack;
  vcml::signal_initiator_array<float, NUM_ADC_PINS> volt_adc;

  AdcTest(const sc_module_name& nm)
      : test_base(nm),
        adc("adc"),
        bus("bus"),
        socket("socket"),
        ext_irq0("ext_irq0"),
        ack("ack"),
        volt_adc("volt_adc") {
    bus.bind(adc.in, vcml::range(0x78, 0x7c));
    bus.bind(socket);

    clk.bind(adc.clk);
    clk.bind(bus.clk);

    rst.bind(adc.rst);
    rst.bind(bus.rst);

    adc.irq.stub();
    ext_irq0.bind(adc.in_ext_irq0);
    ack.bind(adc.ack);
    adc.in_timer0_match_a.stub();
    adc.in_timer0_overflow.stub();
    adc.in_timer1_match_b.stub();
    adc.in_timer1_overflow.stub();
    adc.in_timer1_capture.stub();
    adc.in_analog_comparator.stub();
    adc.out_acme.stub();
    adc.out_aden.stub();
    adc.out_admux.stub();

    for (size_t i = 0; i < NUM_ADC_PINS; ++i)
      volt_adc[i].bind(adc.volt_adc[i]);

    add_test("simple_adc", &AdcTest::TestSimplAdc);
    add_test("interrupt", &AdcTest::TestInterrupt);
    add_test("reference_voltages", &AdcTest::TestReferenceVoltages);
    add_test("latch_behvaior", &AdcTest::TestLatchBehavior);
    add_test("prescaler", &AdcTest::TestPrescaler);
    add_test("adjustment", &AdcTest::TestAdjustment);
    add_test("reserved_bits", &AdcTest::TestReservedBits);
    add_test("channel_lock", &AdcTest::TestChannelLock);
    add_test("auto_trigger", &AdcTest::TestAutoTrigger);
    add_test("free_running", &AdcTest::TestFreeRunning);
    add_test("reset", &AdcTest::TestReset);
    add_test("temperature_sensor", &AdcTest::TestTemperatureSensor);
    add_test("irq_ack", &AdcTest::TestIrqAck);
    add_test("external_inputs", &AdcTest::TestExternalInputs);
  }

  u16 RunConversion(u8 admux_val) {
    socket.writew<u8>(ADMUX, admux_val);
    socket.writew<u8>(ADCSRA, ADCSRA_ADEN | ADCSRA_ADSC);
    sc_core::wait(CONVERSION_TIME);

    u8 lo = 0, hi = 0;
    socket.readw<u8>(ADCL, lo);
    socket.readw<u8>(ADCH, hi);
    return (u16)((hi << 8) | lo);
  }

  void TestSimplAdc() {
    u16 result = RunConversion(ADMUX_REFS0 | ADMUX_MUX_BANDGAP);

    double expected = (double)adc.volt_1_1 * 1024.0 / (double)adc.volt_avcc;
    ASSERT_NEAR(result, expected, 1.0) << "conversion result should match VIN * 1024 / VREF";
    ASSERT_LE(result, 0x3ff) << "result should not exceed 10 bits";
  }

  void TestInterrupt() {
    socket.writew<u8>(ADMUX, ADMUX_REFS0 | ADMUX_MUX_BANDGAP);

    socket.writew<u8>(ADCSRA, ADCSRA_ADEN | ADCSRA_ADSC);
    sc_core::wait(CONVERSION_TIME);

    ASSERT_TRUE(adc.adcsra & ADCSRA_ADIF) << "ADIF should be set once the conversion completes";
    ASSERT_FALSE(adc.irq) << "IRQ should not be raised while ADIE is disabled";

    socket.writew<u8>(ADCSRA, ADCSRA_ADEN | ADCSRA_ADIF);
    ASSERT_FALSE(adc.adcsra & ADCSRA_ADIF);

    socket.writew<u8>(ADCSRA, ADCSRA_ADEN | ADCSRA_ADIE | ADCSRA_ADSC);
    sc_core::wait(CONVERSION_TIME);

    ASSERT_TRUE(adc.adcsra & ADCSRA_ADIF);
    ASSERT_TRUE(adc.irq) << "IRQ should be raised once ADIF and ADIE are both set";

    socket.writew<u8>(ADCSRA, ADCSRA_ADEN | ADCSRA_ADIE | ADCSRA_ADIF);
    ASSERT_FALSE(adc.adcsra & ADCSRA_ADIF);
    ASSERT_FALSE(adc.irq) << "IRQ should be cleared once ADIF is cleared";
  }

  void TestReferenceVoltages() {
    u16 avcc_result = RunConversion(ADMUX_REFS0 | ADMUX_MUX_BANDGAP);
    double avcc_expected = (double)adc.volt_1_1 * 1024.0 / (double)adc.volt_avcc;
    ASSERT_NEAR(avcc_result, avcc_expected, 1.0) << "AVCC reference";

    u16 aref_result = RunConversion(ADMUX_MUX_BANDGAP);
    double aref_expected = (double)adc.volt_1_1 * 1024.0 / (double)adc.volt_aref;
    ASSERT_NEAR(aref_result, aref_expected, 1.0) << "AREF reference";

    u16 internal_result = RunConversion(ADMUX_REFS1 | ADMUX_REFS0 | ADMUX_MUX_BANDGAP);
    ASSERT_NEAR(internal_result, 0x3ff, 2.0) << "internal 1.1V reference";
  }

  void TestLatchBehavior() {
    u8 lo = 0, hi = 0;

    u16 gnd_result = RunConversion(ADMUX_REFS0 | ADMUX_MUX_GND);
    ASSERT_EQ(gnd_result, 0) << "GND channel should convert to 0";

    socket.writew<u8>(ADCSRA, ADCSRA_ADEN | ADCSRA_ADSC);
    sc_core::wait(CONVERSION_TIME);
    socket.readw<u8>(ADCL, lo);

    socket.writew<u8>(ADMUX, ADMUX_REFS0 | ADMUX_MUX_BANDGAP);
    socket.writew<u8>(ADCSRA, ADCSRA_ADEN | ADCSRA_ADSC);
    sc_core::wait(CONVERSION_TIME);

    socket.readw<u8>(ADCH, hi);
    ASSERT_EQ(hi, 0) << "ADCH should still hold the old (GND) result while locked";

    u16 result = RunConversion(ADMUX_REFS0 | ADMUX_MUX_BANDGAP);
    double expected = (double)adc.volt_1_1 * 1024.0 / (double)adc.volt_avcc;
    ASSERT_NEAR(result, expected, 1.0) << "conversion result should update normally once unlocked";
  }

  void TestPrescaler() {
    struct {
      u8 adps;
      int divisor;
    } cases[] = {
        {0b000, 2}, {0b001, 2}, {0b010, 4}, {0b011, 8}, {0b100, 16}, {0b101, 32}, {0b110, 64}, {0b111, 128},
    };

    for (auto& c : cases) {
      socket.writew<u8>(ADMUX, ADMUX_REFS0 | ADMUX_MUX_BANDGAP);
      socket.writew<u8>(ADCSRA, (u8)0);
      sc_core::wait(sc_core::SC_ZERO_TIME);

      socket.writew<u8>(ADCSRA, ADCSRA_ADEN | ADCSRA_ADSC | c.adps);

      double adc_clock_ns = 1000.0 / 16.0 * c.divisor;
      sc_core::sc_time first_conversion(25.0 * adc_clock_ns, sc_core::SC_NS);

      sc_core::wait(first_conversion - sc_core::sc_time(adc_clock_ns / 2, sc_core::SC_NS));
      ASSERT_TRUE(adc.adcsra & ADCSRA_ADSC) << "conversion should still be in progress with prescaler /" << c.divisor;

      sc_core::wait(sc_core::sc_time(adc_clock_ns, sc_core::SC_NS));
      ASSERT_FALSE(adc.adcsra & ADCSRA_ADSC) << "conversion should have completed with prescaler /" << c.divisor;
    }
  }

  void TestAdjustment() {
    socket.writew<u8>(ADMUX, ADMUX_REFS0 | ADMUX_MUX_BANDGAP);
    socket.writew<u8>(ADCSRA, ADCSRA_ADEN | ADCSRA_ADSC);
    sc_core::wait(CONVERSION_TIME);

    u8 lo = 0, hi = 0;
    socket.readw<u8>(ADCL, lo);
    socket.readw<u8>(ADCH, hi);
    u16 right = (u16)((hi << 8) | lo);
    ASSERT_EQ(hi & 0xfc, 0) << "unused upper bits of ADCH should read 0 when right adjusted";

    socket.writew<u8>(ADMUX, ADMUX_ADLAR | ADMUX_REFS0 | ADMUX_MUX_BANDGAP);
    socket.writew<u8>(ADCSRA, ADCSRA_ADEN | ADCSRA_ADSC);
    sc_core::wait(CONVERSION_TIME);

    socket.readw<u8>(ADCL, lo);
    socket.readw<u8>(ADCH, hi);
    u16 left = (u16)((hi << 2) | (lo >> 6));
    ASSERT_EQ(lo & 0x3f, 0) << "unused lower bits of ADCL should read 0 when left adjusted";

    ASSERT_EQ(left, right) << "left and right adjusted readouts should represent the same conversion";
  }

  void TestReservedBits() {
    socket.writew<u8>(ADMUX, (u8)0xff);
    u8 val = 0;
    socket.readw<u8>(ADMUX, val);
    ASSERT_EQ(val & mwr::bit(4), 0) << "bit 4 of ADMUX is reserved and should always read 0";

    socket.writew<u8>(ADCSRB, (u8)0xff);
    socket.readw<u8>(ADCSRB, val);
    ASSERT_EQ(val, ADCSRB_ACME | ADCSRB_ADTS2 | ADCSRB_ADTS1 | ADCSRB_ADTS0)
        << "reserved bits 7 and 5:3 of ADCSRB should always read 0";
  }

  void TestChannelLock() {
    socket.writew<u8>(ADMUX, ADMUX_REFS0 | ADMUX_MUX_BANDGAP);
    socket.writew<u8>(ADCSRA, ADCSRA_ADEN | ADCSRA_ADSC);

    socket.writew<u8>(ADMUX, ADMUX_REFS0 | ADMUX_MUX_GND);
    sc_core::wait(CONVERSION_TIME);

    u8 lo = 0, hi = 0;
    socket.readw<u8>(ADCL, lo);
    socket.readw<u8>(ADCH, hi);
    u16 result = (u16)((hi << 8) | lo);

    double expected = (double)adc.volt_1_1 * 1024.0 / (double)adc.volt_avcc;
    ASSERT_NEAR(result, expected, 1.0)
        << "channel selection should be locked to the value at the start of the conversion";

    u16 next = RunConversion(ADMUX_REFS0 | ADMUX_MUX_GND);
    ASSERT_EQ(next, 0) << "channel change should take effect for the following conversion";
  }

  void TestAutoTrigger() {
    socket.writew<u8>(ADMUX, ADMUX_REFS0 | ADMUX_MUX_BANDGAP);

    socket.writew<u8>(ADCSRB, (u8)0);
    socket.writew<u8>(ADCSRA, ADCSRA_ADEN | ADCSRA_ADATE | ADCSRA_ADIE);
    ext_irq0.pulse();
    sc_core::wait(CONVERSION_TIME);
    ASSERT_FALSE(adc.adcsra & ADCSRA_ADIF) << "unselected trigger source should not start a conversion";
    ASSERT_FALSE(adc.irq);

    socket.writew<u8>(ADCSRB, ADCSRB_ADTS1);
    ext_irq0.write(false);
    sc_core::wait(sc_core::SC_ZERO_TIME);

    ext_irq0.write(true);
    sc_core::wait(CONVERSION_TIME);
    ASSERT_TRUE(adc.adcsra & ADCSRA_ADIF) << "rising edge on the trigger source should start a conversion";
    ASSERT_TRUE(adc.irq);

    socket.writew<u8>(ADCSRA, ADCSRA_ADEN | ADCSRA_ADATE | ADCSRA_ADIE | ADCSRA_ADIF);

    ext_irq0.write(false);
    sc_core::wait(CONVERSION_TIME);
    ASSERT_FALSE(adc.adcsra & ADCSRA_ADIF) << "falling edge should not start a conversion";
    ASSERT_FALSE(adc.irq);
  }

  void TestFreeRunning() {
    socket.writew<u8>(ADMUX, ADMUX_REFS0 | ADMUX_MUX_BANDGAP);
    socket.writew<u8>(ADCSRB, (u8)0);

    socket.writew<u8>(ADCSRA, ADCSRA_ADEN | ADCSRA_ADATE | ADCSRA_ADIE | ADCSRA_ADSC);
    sc_core::wait(CONVERSION_TIME);
    ASSERT_TRUE(adc.adcsra & ADCSRA_ADIF) << "first conversion should complete and set ADIF";
    ASSERT_TRUE(adc.irq);

    socket.writew<u8>(ADCSRA, ADCSRA_ADEN | ADCSRA_ADATE | ADCSRA_ADIE | ADCSRA_ADIF);
    ASSERT_FALSE(adc.adcsra & ADCSRA_ADIF);

    sc_core::wait(CONVERSION_TIME);
    ASSERT_TRUE(adc.adcsra & ADCSRA_ADIF) << "ADC should keep converting on its own in free running mode";
    ASSERT_TRUE(adc.irq);

    socket.writew<u8>(ADCSRA, ADCSRA_ADEN | ADCSRA_ADATE | ADCSRA_ADIE | ADCSRA_ADIF);
    sc_core::wait(CONVERSION_TIME);
    ASSERT_TRUE(adc.adcsra & ADCSRA_ADIF) << "ADC should still be converting continuously";

    u8 lo = 0, hi = 0;
    socket.readw<u8>(ADCL, lo);
    socket.readw<u8>(ADCH, hi);
    u16 result = (u16)((hi << 8) | lo);
    double expected = (double)adc.volt_1_1 * 1024.0 / (double)adc.volt_avcc;
    ASSERT_NEAR(result, expected, 1.0);
  }

  void TestReset() {
    socket.writew<u8>(ADMUX, ADMUX_REFS0 | ADMUX_MUX_BANDGAP);
    socket.writew<u8>(ADCSRB, ADCSRB_ADTS1);
    socket.writew<u8>(ADCSRA, ADCSRA_ADEN | ADCSRA_ADIE | ADCSRA_ADSC);
    sc_core::wait(CONVERSION_TIME);

    ASSERT_NE((u8)adc.admux, 0);
    ASSERT_NE((u8)adc.adcsrb, 0);
    ASSERT_NE((u8)adc.adcsra, 0);

    adc.reset();

    ASSERT_EQ((u8)adc.adcl, 0);
    ASSERT_EQ((u8)adc.adch, 0);
    ASSERT_EQ((u8)adc.adcsra, 0);
    ASSERT_EQ((u8)adc.adcsrb, 0);
    ASSERT_EQ((u8)adc.admux, 0);
    ASSERT_FALSE(adc.irq) << "IRQ line should be deasserted after reset";
  }

  void TestTemperatureSensor() {
    u8 admux_temp = ADMUX_REFS1 | ADMUX_REFS0 | ADMUX_MUX_TEMP;

    adc.temp_celsius = 25.0f;
    u16 result_25c = RunConversion(admux_temp);
    ASSERT_NEAR(result_25c, 352, 1.0) << "sensor should read ~352 at 25 degC";

    adc.temp_celsius = 125.0f;
    u16 result_125c = RunConversion(admux_temp);
    ASSERT_GT(result_125c, result_25c) << "a higher temperature should yield a higher reading";

    adc.temp_celsius = -40.0f;
    u16 result_m40c = RunConversion(admux_temp);
    ASSERT_LT(result_m40c, result_25c) << "a lower temperature should yield a lower reading";

    ASSERT_NEAR((double)(result_125c - result_25c), 100.0, 5.0) << "sensitivity should be ~1 LSB/degC";
  }

  void TestIrqAck() {
    socket.writew<u8>(ADMUX, ADMUX_REFS0 | ADMUX_MUX_BANDGAP);
    socket.writew<u8>(ADCSRA, ADCSRA_ADEN | ADCSRA_ADIE | ADCSRA_ADSC);
    sc_core::wait(CONVERSION_TIME);

    ASSERT_TRUE(adc.adcsra & ADCSRA_ADIF) << "ADIF should be set once the conversion completes";
    ASSERT_TRUE(adc.irq) << "IRQ should be raised since ADIE is set";

    ack.pulse();

    ASSERT_FALSE(adc.adcsra & ADCSRA_ADIF) << "ack should clear ADIF";
    ASSERT_FALSE(adc.irq) << "ack should drop irq";

    socket.writew<u8>(ADCSRA, ADCSRA_ADEN | ADCSRA_ADIE | ADCSRA_ADSC);
    sc_core::wait(CONVERSION_TIME);

    ASSERT_TRUE(adc.adcsra & ADCSRA_ADIF);
    ASSERT_TRUE(adc.irq);
    ack.pulse();
  }

  void TestExternalInputs() {
    static constexpr float voltages[NUM_ADC_PINS] = {
        0.0f, 0.7f, 1.3f, 2.1f, 2.8f, 3.3f, 4.2f, 4.9f,
    };

    for (size_t ch = 0; ch < NUM_ADC_PINS; ++ch)
      volt_adc[ch] = voltages[ch];
    sc_core::wait(sc_core::SC_ZERO_TIME);

    for (size_t ch = 0; ch < NUM_ADC_PINS; ++ch) {
      u16 result = RunConversion(ADMUX_REFS0 | (u8)ch);
      double expected = (double)voltages[ch] * 1024.0 / (double)adc.volt_avcc;
      ASSERT_NEAR(result, expected, 1.0) << "ADC" << ch << " should convert its own external input voltage";
    }
  }
};

TEST(ArduSystem, AdcTest) {
  AdcTest test("adc_test");
  sc_core::sc_start();
}
