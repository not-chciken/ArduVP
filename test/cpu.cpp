#include "cpu.h"

#include "testing.h"

static constexpr double kCycle = 0.062499;

struct CpuTest : public test_base {
  Cpu cpu;
  vcml::tlm_initiator_socket socket;
  vcml::generic::memory memory;
  vcml::generic::bus bus;

  CpuTest(const sc_module_name& nm)
      : test_base(nm), cpu(), socket("socket"), memory("memory", vcml::MiB * 1), bus("bus") {
    bus.bind(memory.in, vcml::range(0x0, vcml::MiB * 1));
    bus.bind(socket);
    bus.bind(cpu.data);
    bus.bind(cpu.insn);

    clk.bind(bus.clk);
    clk.bind(cpu.clk);
    clk.bind(memory.clk);

    rst.bind(bus.rst);
    rst.bind(cpu.rst);
    rst.bind(memory.rst);

    add_test("nop", &CpuTest::TestNop);
    add_test("add", &CpuTest::TestAdd);
    add_test("adc", &CpuTest::TestAdc);
    add_test("adiw", &CpuTest::TestAdiw);
    add_test("and", &CpuTest::TestAnd);
    add_test("andi", &CpuTest::TestAndi);
    add_test("asr", &CpuTest::TestAsr);
  }

  void reset() override {
    std::memset(memory.data(), 0, memory.size);
    test_base::reset();
  }

  void TestNop() {
    socket.writew<u16>(0, 0x0000);
    sc_core::wait(kCycle, sc_core::SC_US);
  }

  void TestAdd() {
    cpu.rf[0] = 3;
    socket.writew<u16>(0, 0x0c00);  // ADD R0, R0
    sc_core::wait(kCycle, sc_core::SC_US);
    ASSERT_EQ(cpu.rf[0], 6);
    ASSERT_EQ(cpu.sreg.c, 0);
    ASSERT_EQ(cpu.sreg.z, 0);
    ASSERT_EQ(cpu.sreg.n, 0);

    cpu.rf[0] = 0x80;
    socket.writew<u16>(2, 0x0c00);  // ADD R0, R0
    sc_core::wait(kCycle, sc_core::SC_US);
    ASSERT_EQ(cpu.rf[0], 0x00);
    ASSERT_EQ(cpu.sreg.c, 1);
    ASSERT_EQ(cpu.sreg.z, 1);
    ASSERT_EQ(cpu.sreg.n, 0);

    cpu.rf[0] = 0x40;
    socket.writew<u16>(4, 0x0c00);  // ADD R0, R0
    sc_core::wait(kCycle, sc_core::SC_US);
    ASSERT_EQ(cpu.rf[0], 0x80);
    ASSERT_EQ(cpu.sreg.c, 0);
    ASSERT_EQ(cpu.sreg.z, 0);
    ASSERT_EQ(cpu.sreg.n, 1);
    ASSERT_EQ(cpu.sreg.v, 1);

    cpu.rf[0] = 0x08;
    socket.writew<u16>(6, 0x0c00);  // ADD R0, R0
    sc_core::wait(kCycle, sc_core::SC_US);
    ASSERT_EQ(cpu.rf[0], 0x10);
    ASSERT_EQ(cpu.sreg.h, 1);
    ASSERT_EQ(cpu.sreg.c, 0);
    ASSERT_EQ(cpu.sreg.z, 0);
  }

  void TestAdc() {
    cpu.rf[0] = 5;
    cpu.sreg.c = 0;
    socket.writew<u16>(0, 0x1c00);  // ADC R0, R0
    sc_core::wait(kCycle, sc_core::SC_US);
    ASSERT_EQ(cpu.rf[0], 10);
    ASSERT_EQ(cpu.sreg.c, 0);
    ASSERT_EQ(cpu.sreg.z, 0);
    ASSERT_EQ(cpu.sreg.n, 0);

    cpu.rf[0] = 2;
    cpu.sreg.c = 1;
    socket.writew<u16>(2, 0x1c00);  // ADC R0, R0 at PC=1
    sc_core::wait(kCycle, sc_core::SC_US);
    ASSERT_EQ(cpu.rf[0], 5);
    ASSERT_EQ(cpu.sreg.c, 0);
    ASSERT_EQ(cpu.sreg.z, 0);

    cpu.rf[0] = 0x80;
    cpu.sreg.c = 0;
    socket.writew<u16>(4, 0x1c00);  // ADC R0, R0 at PC=2
    sc_core::wait(kCycle, sc_core::SC_US);
    ASSERT_EQ(cpu.rf[0], 0x00);
    ASSERT_EQ(cpu.sreg.c, 1);
    ASSERT_EQ(cpu.sreg.z, 1);
  }

  void TestAdiw() {
    cpu.rf[24] = 0;
    cpu.rf[25] = 0;
    socket.writew<u16>(0, 0x9601);  // ADIW R24, 1
    sc_core::wait(2 * kCycle, sc_core::SC_US);
    ASSERT_EQ(cpu.rf[24], 0x01);
    ASSERT_EQ(cpu.rf[25], 0x00);
    ASSERT_EQ(cpu.sreg.z, 0);
    ASSERT_EQ(cpu.sreg.c, 0);
    ASSERT_EQ(cpu.sreg.n, 0);
    ASSERT_EQ(cpu.retired_instructions, 1);

    cpu.rf[24] = 0xff;
    cpu.rf[25] = 0x00;
    socket.writew<u16>(2, 0x96cf);  // ADIW R24, 63
    sc_core::wait(2 * kCycle, sc_core::SC_US);
    ASSERT_EQ(cpu.rf[24], 0x3e);
    ASSERT_EQ(cpu.rf[25], 0x01);
    ASSERT_EQ(cpu.sreg.z, 0);
    ASSERT_EQ(cpu.sreg.c, 0);

    cpu.rf[24] = 0xff;
    cpu.rf[25] = 0xff;
    socket.writew<u16>(4, 0x9601);  // ADIW R24, 1
    sc_core::wait(2 * kCycle, sc_core::SC_US);
    ASSERT_EQ(cpu.rf[24], 0x00);
    ASSERT_EQ(cpu.rf[25], 0x00);
    ASSERT_EQ(cpu.sreg.z, 1);
    ASSERT_EQ(cpu.sreg.c, 1);
  }

  void TestAnd() {
    cpu.rf[0] = 0xAB;
    socket.writew<u16>(0, 0x2000);  // AND R0, R0
    sc_core::wait(kCycle, sc_core::SC_US);
    ASSERT_EQ(cpu.rf[0], 0xab);
    ASSERT_EQ(cpu.sreg.z, 0);
    ASSERT_EQ(cpu.sreg.v, 0);
    ASSERT_EQ(cpu.sreg.n, 1);
    ASSERT_EQ(cpu.retired_instructions, 1);

    cpu.rf[0] = 0;
    socket.writew<u16>(2, 0x2000);  // AND R0, R0
    sc_core::wait(kCycle, sc_core::SC_US);
    ASSERT_EQ(cpu.rf[0], 0);
    ASSERT_EQ(cpu.sreg.z, 1);
    ASSERT_EQ(cpu.sreg.n, 0);
    ASSERT_EQ(cpu.sreg.v, 0);

    cpu.rf[0] = 0x55;
    socket.writew<u16>(4, 0x2000);  // AND R0, R0
    sc_core::wait(kCycle, sc_core::SC_US);
    ASSERT_EQ(cpu.rf[0], 0x55);
    ASSERT_EQ(cpu.sreg.z, 0);
    ASSERT_EQ(cpu.sreg.n, 0);
  }

  void TestAndi() {
    cpu.rf[16] = 0xAB;
    socket.writew<u16>(0, 0x700f);  // ANDI R16, 0x0f
    sc_core::wait(kCycle, sc_core::SC_US);
    ASSERT_EQ(cpu.rf[16], 0x0b);
    ASSERT_EQ(cpu.sreg.z, 0);
    ASSERT_EQ(cpu.sreg.n, 0);
    ASSERT_EQ(cpu.sreg.v, 0);
    ASSERT_EQ(cpu.retired_instructions, 1);

    cpu.rf[16] = 0xff;
    socket.writew<u16>(2, 0x7000);  // ANDI R16, 0x00
    sc_core::wait(kCycle, sc_core::SC_US);
    ASSERT_EQ(cpu.rf[16], 0x00);
    ASSERT_EQ(cpu.sreg.z, 1);
    ASSERT_EQ(cpu.sreg.n, 0);

    cpu.rf[16] = 0x80;
    socket.writew<u16>(4, 0x7f0f);  // ANDI R16, 0xff
    sc_core::wait(kCycle, sc_core::SC_US);
    ASSERT_EQ(cpu.rf[16], 0x80);
    ASSERT_EQ(cpu.sreg.z, 0);
    ASSERT_EQ(cpu.sreg.n, 1);
    ASSERT_EQ(cpu.sreg.v, 0);
  }

  void TestAsr() {
    cpu.rf[0] = 0x80;
    socket.writew<u16>(0, 0x9405);  // ASR R0
    sc_core::wait(kCycle, sc_core::SC_US);
    ASSERT_EQ(cpu.rf[0], 0xC0);
    ASSERT_EQ(cpu.sreg.c, 0);
    ASSERT_EQ(cpu.sreg.n, 1);
    ASSERT_EQ(cpu.sreg.z, 0);
    ASSERT_EQ(cpu.retired_instructions, 1);

    cpu.rf[0] = 0x01;
    socket.writew<u16>(2, 0x9405);  // ASR R0
    sc_core::wait(kCycle, sc_core::SC_US);
    ASSERT_EQ(cpu.rf[0], 0x00);
    ASSERT_EQ(cpu.sreg.c, 1);
    ASSERT_EQ(cpu.sreg.z, 1);
    ASSERT_EQ(cpu.sreg.n, 0);

    cpu.rf[0] = 0x4A;
    socket.writew<u16>(4, 0x9405);  // ASR R0
    sc_core::wait(kCycle, sc_core::SC_US);
    ASSERT_EQ(cpu.rf[0], 0x25);
    ASSERT_EQ(cpu.sreg.c, 0);
    ASSERT_EQ(cpu.sreg.n, 0);
    ASSERT_EQ(cpu.sreg.z, 0);

    cpu.rf[0] = 0xff;
    socket.writew<u16>(6, 0x9405);  // ASR R0
    sc_core::wait(kCycle, sc_core::SC_US);
    ASSERT_EQ(cpu.rf[0], 0xff);
    ASSERT_EQ(cpu.sreg.c, 1);
    ASSERT_EQ(cpu.sreg.n, 1);
    ASSERT_EQ(cpu.sreg.z, 0);
  }
};

TEST(ArduSystem, CpuTest) {
  CpuTest test("cpu_test");
  sc_core::sc_start();
}
