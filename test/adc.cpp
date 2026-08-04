#include "adc.h"

#include "testing.h"

struct AdcTest : public test_base {
  Adc adc;
  vcml::tlm_initiator_socket socket;
  vcml::generic::bus bus;

  AdcTest(const sc_module_name& nm)
      : test_base(nm), adc("adc"), bus("bus"), socket("socket") {
    bus.bind(adc.in, vcml::range(0x78, 0x7c));
    bus.bind(socket);

    clk.bind(adc.clk);
    clk.bind(bus.clk);

    rst.bind(adc.rst);
    rst.bind(bus.rst);

    adc.irq.stub();
    adc.in_ext_irq0.stub();
    adc.in_timer0_match_a.stub();
    adc.in_timer0_overflow.stub();
    adc.in_timer1_match_b.stub();
    adc.in_timer1_overflow.stub();
    adc.in_timer1_capture.stub();

    add_test("adc", &AdcTest::TestCompare);
  }

  void TestCompare() {
    socket.writew<u16>(0, 0x0000);
    sc_core::wait(1, sc_core::SC_US);
  }
};

TEST(ArduSystem, AdcTest) {
  AdcTest test("adc_test");
  sc_core::sc_start();
}
