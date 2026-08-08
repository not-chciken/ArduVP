#include "comparator.h"

#include "testing.h"

static constexpr mwr::u64 ACSR = 0x50;

static constexpr u8 ACSR_ACIS0 = (u8)mwr::bit(0);
static constexpr u8 ACSR_ACIS1 = (u8)mwr::bit(1);
static constexpr u8 ACSR_ACIC = (u8)mwr::bit(2);
static constexpr u8 ACSR_ACIE = (u8)mwr::bit(3);
static constexpr u8 ACSR_ACI = (u8)mwr::bit(4);
static constexpr u8 ACSR_ACO = (u8)mwr::bit(5);
static constexpr u8 ACSR_ACBG = (u8)mwr::bit(6);
static constexpr u8 ACSR_ACD = (u8)mwr::bit(7);
static constexpr uint NUM_ADC_PINS = 8;
static constexpr uint NUM_AIN_PINS = 2;

struct ComparatorTest : public test_base {
  Comparator comparator;
  vcml::tlm_initiator_socket socket;
  vcml::generic::bus bus;

  vcml::gpio_initiator_socket acme;
  vcml::gpio_initiator_socket aden;
  vcml::signal_initiator_socket<u8> admux;

  vcml::signal_initiator_array<float, NUM_ADC_PINS> volt_adc;
  vcml::signal_initiator_array<float, NUM_AIN_PINS> volt_ain;

  ComparatorTest(const sc_module_name& nm)
      : test_base(nm),
        comparator("comparator"),
        bus("bus"),
        socket("socket"),
        acme("acme"),
        aden("aden"),
        admux("admux"),
        volt_adc("volt_adc"),
        volt_ain("volt_ain") {
    bus.bind(comparator.in, vcml::range(ACSR, ACSR));
    bus.bind(socket);

    clk.bind(comparator.clk);
    clk.bind(bus.clk);

    rst.bind(comparator.rst);
    rst.bind(bus.rst);

    acme.bind(comparator.in_acme);
    aden.bind(comparator.in_aden);
    admux.bind(comparator.in_admux);

    for (size_t i = 0; i < NUM_ADC_PINS; i++)
      volt_adc[i].bind(comparator.volt_adc[i]);
    for (size_t i = 0; i < NUM_AIN_PINS; i++)
      volt_ain[i].bind(comparator.volt_ain[i]);

    comparator.aco.stub();
    comparator.acic.stub();
    comparator.irq.stub();

    add_test("reset", &ComparatorTest::TestReset);
    add_test("disable", &ComparatorTest::TestDisable);
    add_test("interrupt", &ComparatorTest::TestInterrupt);
    add_test("modes", &ComparatorTest::TestModes);
    add_test("inputs", &ComparatorTest::TestInputs);
    add_test("bandgap", &ComparatorTest::TestBandgap);
    add_test("input_capture", &ComparatorTest::TestInputCapture);
    add_test("aco_read_only", &ComparatorTest::TestAcoReadOnly);
  }

  u8 ReadAcsr() {
    u8 val = 0;
    socket.readw<u8>(ACSR, val);
    return val;
  }

  void WriteAcsr(u8 val) { socket.writew<u8>(ACSR, val); }

  // AIN0 is the positive input, AIN1 the default negative input.
  void SetAin(float ain0, float ain1) {
    volt_ain[0] = ain0;
    volt_ain[1] = ain1;
    sc_core::wait(sc_core::SC_ZERO_TIME);
  }

  void TestReset() {
    ASSERT_EQ(ReadAcsr(), 0) << "ACSR should read 0 after reset (AIN0 == AIN1 == 0V, so ACO is also clear)";
  }

  void TestDisable() {
    SetAin(2.0f, 1.0f);
    ASSERT_TRUE(ReadAcsr() & ACSR_ACO) << "ACO should be set once AIN0 > AIN1";

    WriteAcsr(ACSR_ACD);
    sc_core::wait(sc_core::SC_ZERO_TIME);
    ASSERT_FALSE(ReadAcsr() & ACSR_ACO) << "ACO should read 0 while the comparator is powered down (ACD set)";

    // Would flip the comparison result if the comparator were still active.
    SetAin(2.0f, 3.0f);
    ASSERT_FALSE(ReadAcsr() & ACSR_ACO) << "a disabled comparator should not track input changes";

    WriteAcsr((u8)0);
    sc_core::wait(sc_core::SC_ZERO_TIME);
    ASSERT_TRUE(ReadAcsr() & ACSR_ACO) << "ACO should resume tracking the inputs once ACD is cleared";
  }

  void TestInterrupt() {
    WriteAcsr((u8)0);  // toggle mode (ACIS1:0 == 00), ACIE disabled
    SetAin(0.0f, 1.0f);

    SetAin(2.0f, 1.0f);  // ACO: 0 -> 1, a toggle event
    ASSERT_TRUE(ReadAcsr() & ACSR_ACI) << "ACI should be set once the comparator output toggles";
    ASSERT_FALSE(comparator.irq) << "IRQ should not be raised while ACIE is disabled";

    WriteAcsr(ACSR_ACI);  // write 1 to clear
    ASSERT_FALSE(ReadAcsr() & ACSR_ACI) << "writing a 1 to ACI should clear the flag";

    WriteAcsr(ACSR_ACIE);
    SetAin(0.0f, 1.0f);  // ACO: 1 -> 0, another toggle event
    ASSERT_TRUE(ReadAcsr() & ACSR_ACI);
    ASSERT_TRUE(comparator.irq) << "IRQ should be raised once ACI and ACIE are both set";

    WriteAcsr(ACSR_ACIE | ACSR_ACI);
    ASSERT_FALSE(comparator.irq) << "IRQ should be cleared once ACI is cleared";
  }

  void TestModes() {
    struct {
      u8 acis;
      bool trigger_on_rise;
      bool trigger_on_fall;
    } cases[] = {
        {0b00, true, true},   // toggle
        {0b10, false, true},  // falling edge
        {0b11, true, false},  // rising edge
    };

    for (auto& c : cases) {
      WriteAcsr((u8)0);
      SetAin(0.0f, 1.0f);  // ACO = 0
      WriteAcsr(c.acis);

      SetAin(2.0f, 1.0f);  // rising edge on ACO
      ASSERT_EQ((bool)(ReadAcsr() & ACSR_ACI), c.trigger_on_rise) << "ACIS=" << (int)c.acis << ", rising edge";
      WriteAcsr(c.acis | ACSR_ACI);

      SetAin(0.0f, 1.0f);  // falling edge on ACO
      ASSERT_EQ((bool)(ReadAcsr() & ACSR_ACI), c.trigger_on_fall) << "ACIS=" << (int)c.acis << ", falling edge";
    }
  }

  void TestInputs() {
    volt_ain[0] = 2.0f;  // AIN0 (positive input)
    sc_core::wait(sc_core::SC_ZERO_TIME);

    // AIN1 should be the negative input whenever ACME is clear...
    acme.write(false);
    aden.write(false);
    volt_ain[1] = 3.0f;
    sc_core::wait(sc_core::SC_ZERO_TIME);
    ASSERT_FALSE(ReadAcsr() & ACSR_ACO) << "AIN0 < AIN1 via AIN1";

    volt_ain[1] = 1.0f;
    sc_core::wait(sc_core::SC_ZERO_TIME);
    ASSERT_TRUE(ReadAcsr() & ACSR_ACO) << "AIN0 > AIN1 via AIN1";

    // ...or whenever the ADC is enabled (ADEN set), even if ACME is set.
    acme.write(true);
    aden.write(true);
    volt_adc[0] = 3.0f;  // should be ignored while ADEN is set
    sc_core::wait(sc_core::SC_ZERO_TIME);
    ASSERT_TRUE(ReadAcsr() & ACSR_ACO) << "AIN1 should still apply when ADEN is set, regardless of ACME/MUX";

    // With ACME set and the ADC disabled, MUX2:0 selects the negative input from ADC0..7.
    aden.write(false);
    for (u8 ch = 0; ch < 8; ch++) {
      admux.write(ch);

      volt_adc[ch] = 3.0f;  // above AIN0 -> ACO clear
      sc_core::wait(sc_core::SC_ZERO_TIME);
      ASSERT_FALSE(ReadAcsr() & ACSR_ACO) << "ADC" << (int)ch << " selected, above AIN0";

      volt_adc[ch] = 1.0f;  // below AIN0 -> ACO set
      sc_core::wait(sc_core::SC_ZERO_TIME);
      ASSERT_TRUE(ReadAcsr() & ACSR_ACO) << "ADC" << (int)ch << " selected, below AIN0";

      volt_adc[ch] = 0.0f;  // restore so it doesn't leak into the next channel
    }
  }

  void TestBandgap() {
    comparator.volt_bandgap = 1.1f;
    volt_ain[1] = 0.5f;  // AIN1

    WriteAcsr((u8)0);
    volt_ain[0] = 0.0f;  // AIN0, below AIN1
    sc_core::wait(sc_core::SC_ZERO_TIME);
    ASSERT_FALSE(ReadAcsr() & ACSR_ACO) << "AIN0 should apply to the positive input while ACBG is clear";

    WriteAcsr(ACSR_ACBG);
    sc_core::wait(sc_core::SC_ZERO_TIME);
    ASSERT_TRUE(ReadAcsr() & ACSR_ACO) << "the bandgap reference (1.1V) should replace AIN0 once ACBG is set";
  }

  void TestInputCapture() {
    SetAin(0.0f, 1.0f);  // ACO = 0

    WriteAcsr((u8)0);  // ACIC disabled
    SetAin(2.0f, 1.0f);  // ACO: 0 -> 1
    ASSERT_FALSE(comparator.acic) << "the input capture pin should stay low while ACIC is disabled";

    WriteAcsr(ACSR_ACIC);
    SetAin(0.0f, 1.0f);  // ACO: 1 -> 0
    ASSERT_FALSE(comparator.acic);

    SetAin(2.0f, 1.0f);  // ACO: 0 -> 1
    ASSERT_TRUE(comparator.acic) << "the comparator output should drive the input capture pin once ACIC is set";
  }

  void TestAcoReadOnly() {
    SetAin(0.0f, 1.0f);  // ACO should be 0 (AIN0 < AIN1)

    WriteAcsr(ACSR_ACO);  // attempt to force ACO high via a register write
    sc_core::wait(sc_core::SC_ZERO_TIME);

    ASSERT_FALSE(ReadAcsr() & ACSR_ACO) << "ACO is read-only and always reflects the live comparator output";
  }
};

TEST(ArduSystem, ComparatorTest) {
  ComparatorTest test("comparator_test");
  sc_core::sc_start();
}
