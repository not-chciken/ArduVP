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
    add_test("bclr", &CpuTest::TestBclr);
    add_test("bld", &CpuTest::TestBld);
    add_test("brbc", &CpuTest::TestBrbc);
    add_test("brbs", &CpuTest::TestBrbs);
    add_test("brcc", &CpuTest::TestBrcc);
    add_test("brcs", &CpuTest::TestBrcs);
    add_test("break", &CpuTest::TestBreak);
    add_test("breq", &CpuTest::TestBreq);
    add_test("bset", &CpuTest::TestBset);
    add_test("bst", &CpuTest::TestBst);
    add_test("call", &CpuTest::TestCall);
    add_test("cbi", &CpuTest::TestCbi);
    add_test("com", &CpuTest::TestCom);
    add_test("cp", &CpuTest::TestCp);
    add_test("ijmp", &CpuTest::TestIjmp);
    add_test("ldi", &CpuTest::TestLdi);
    add_test("mov", &CpuTest::TestMov);
    add_test("movw", &CpuTest::TestMovw);
    add_test("mul", &CpuTest::TestMul);
    add_test("ori", &CpuTest::TestOri);
    add_test("out", &CpuTest::TestOut);
    add_test("pop", &CpuTest::TestPop);
    add_test("push", &CpuTest::TestPush);
    add_test("rcall", &CpuTest::TestRcall);
    add_test("ret", &CpuTest::TestRet);
    add_test("reti", &CpuTest::TestReti);
    add_test("rjmp", &CpuTest::TestRjmp);
    add_test("rol", &CpuTest::TestRol);
    add_test("ror", &CpuTest::TestRor);
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

  void TestBclr() {
    cpu.sreg = 0xff;
    socket.writew<u16>(0, 0x9488);  // BCLR C
    sc_core::wait(kCycle, sc_core::SC_US);
    ASSERT_EQ(cpu.sreg.i, 1);
    ASSERT_EQ(cpu.sreg.t, 1);
    ASSERT_EQ(cpu.sreg.h, 1);
    ASSERT_EQ(cpu.sreg.s, 1);
    ASSERT_EQ(cpu.sreg.v, 1);
    ASSERT_EQ(cpu.sreg.n, 1);
    ASSERT_EQ(cpu.sreg.z, 1);
    ASSERT_EQ(cpu.sreg.c, 0);

    cpu.sreg = 0xff;
    socket.writew<u16>(2, 0x9498);  // BCLR Z
    sc_core::wait(kCycle, sc_core::SC_US);
    ASSERT_EQ(cpu.sreg.i, 1);
    ASSERT_EQ(cpu.sreg.t, 1);
    ASSERT_EQ(cpu.sreg.h, 1);
    ASSERT_EQ(cpu.sreg.s, 1);
    ASSERT_EQ(cpu.sreg.v, 1);
    ASSERT_EQ(cpu.sreg.n, 1);
    ASSERT_EQ(cpu.sreg.z, 0);
    ASSERT_EQ(cpu.sreg.c, 1);

    cpu.sreg = 0xff;
    socket.writew<u16>(4, 0x94a8);  // BCLR N
    sc_core::wait(kCycle, sc_core::SC_US);
    ASSERT_EQ(cpu.sreg.i, 1);
    ASSERT_EQ(cpu.sreg.t, 1);
    ASSERT_EQ(cpu.sreg.h, 1);
    ASSERT_EQ(cpu.sreg.s, 1);
    ASSERT_EQ(cpu.sreg.v, 1);
    ASSERT_EQ(cpu.sreg.n, 0);
    ASSERT_EQ(cpu.sreg.z, 1);
    ASSERT_EQ(cpu.sreg.c, 1);

    cpu.sreg = 0xff;
    socket.writew<u16>(6, 0x94b8);  // BCLR V
    sc_core::wait(kCycle, sc_core::SC_US);
    ASSERT_EQ(cpu.sreg.i, 1);
    ASSERT_EQ(cpu.sreg.t, 1);
    ASSERT_EQ(cpu.sreg.h, 1);
    ASSERT_EQ(cpu.sreg.s, 1);
    ASSERT_EQ(cpu.sreg.v, 0);
    ASSERT_EQ(cpu.sreg.n, 1);
    ASSERT_EQ(cpu.sreg.z, 1);
    ASSERT_EQ(cpu.sreg.c, 1);

    cpu.sreg = 0xff;
    socket.writew<u16>(8, 0x94c8);  // BCLR S
    sc_core::wait(kCycle, sc_core::SC_US);
    ASSERT_EQ(cpu.sreg.i, 1);
    ASSERT_EQ(cpu.sreg.t, 1);
    ASSERT_EQ(cpu.sreg.h, 1);
    ASSERT_EQ(cpu.sreg.s, 0);
    ASSERT_EQ(cpu.sreg.v, 1);
    ASSERT_EQ(cpu.sreg.n, 1);
    ASSERT_EQ(cpu.sreg.z, 1);
    ASSERT_EQ(cpu.sreg.c, 1);

    cpu.sreg = 0xff;
    socket.writew<u16>(10, 0x94d8);  // BCLR H
    sc_core::wait(kCycle, sc_core::SC_US);
    ASSERT_EQ(cpu.sreg.i, 1);
    ASSERT_EQ(cpu.sreg.t, 1);
    ASSERT_EQ(cpu.sreg.h, 0);
    ASSERT_EQ(cpu.sreg.s, 1);
    ASSERT_EQ(cpu.sreg.v, 1);
    ASSERT_EQ(cpu.sreg.n, 1);
    ASSERT_EQ(cpu.sreg.z, 1);
    ASSERT_EQ(cpu.sreg.c, 1);

    cpu.sreg = 0xff;
    socket.writew<u16>(12, 0x94e8);  // BCLR T
    sc_core::wait(kCycle, sc_core::SC_US);
    ASSERT_EQ(cpu.sreg.i, 1);
    ASSERT_EQ(cpu.sreg.t, 0);
    ASSERT_EQ(cpu.sreg.h, 1);
    ASSERT_EQ(cpu.sreg.s, 1);
    ASSERT_EQ(cpu.sreg.v, 1);
    ASSERT_EQ(cpu.sreg.n, 1);
    ASSERT_EQ(cpu.sreg.z, 1);
    ASSERT_EQ(cpu.sreg.c, 1);

    cpu.sreg = 0xff;
    socket.writew<u16>(14, 0x94f8);  // BCLR I
    sc_core::wait(kCycle, sc_core::SC_US);
    ASSERT_EQ(cpu.sreg.i, 0);
    ASSERT_EQ(cpu.sreg.t, 1);
    ASSERT_EQ(cpu.sreg.h, 1);
    ASSERT_EQ(cpu.sreg.s, 1);
    ASSERT_EQ(cpu.sreg.v, 1);
    ASSERT_EQ(cpu.sreg.n, 1);
    ASSERT_EQ(cpu.sreg.z, 1);
    ASSERT_EQ(cpu.sreg.c, 1);
  }

  void TestBld() {
    cpu.sreg = 0;

    cpu.sreg.t = 1;
    socket.writew<u16>(0, 0xf800);  // BLD R0, 0
    sc_core::wait(kCycle, sc_core::SC_US);
    ASSERT_EQ(cpu.rf[0], mwr::bit(0));

    cpu.sreg.t = 1;
    socket.writew<u16>(2, 0xf812);  // BLD R1, 2
    sc_core::wait(kCycle, sc_core::SC_US);
    ASSERT_EQ(cpu.rf[1], mwr::bit(2));

    cpu.sreg.t = 0;
    socket.writew<u16>(4, 0xf812);  // BLD R1, 2
    sc_core::wait(kCycle, sc_core::SC_US);
    ASSERT_EQ(cpu.rf[1], 0);
  }

  void TestBrbc() {
    cpu.sreg = 0;

    socket.writew<u16>(0, 0xf400);  // BRBC C, 0
    sc_core::wait(kCycle, sc_core::SC_US);
    ASSERT_EQ(cpu.pc, 1);

    socket.writew<u16>(2, 0xf409);  // BRBC Z, 1
    sc_core::wait(kCycle, sc_core::SC_US);
    ASSERT_EQ(cpu.pc, 3);

    cpu.sreg.z = 1;
    socket.writew<u16>(6, 0xf409);  // BRBC Z, 1
    sc_core::wait(kCycle, sc_core::SC_US);
    ASSERT_EQ(cpu.pc, 4);

    socket.writew<u16>(8, 0xf7f2);  // BRBC N, -2
    sc_core::wait(kCycle, sc_core::SC_US);
    ASSERT_EQ(cpu.pc, 3);
  }

  void TestBrbs() {
    cpu.sreg = 0;

    socket.writew<u16>(0, 0xf000);  // BRBS C, 0
    sc_core::wait(kCycle, sc_core::SC_US);
    ASSERT_EQ(cpu.pc, 1);

    cpu.sreg.z = 1;
    socket.writew<u16>(2, 0xf009);  // BRBS Z, 1
    sc_core::wait(kCycle, sc_core::SC_US);
    ASSERT_EQ(cpu.pc, 3);

    cpu.sreg.z = 0;
    socket.writew<u16>(2, 0xf009);  // BRBS Z, 1
    sc_core::wait(kCycle, sc_core::SC_US);
    ASSERT_EQ(cpu.pc, 4);

    cpu.sreg.n = 1;
    socket.writew<u16>(8, 0xf3f2);  // BRBS N, -2
    sc_core::wait(kCycle, sc_core::SC_US);
    ASSERT_EQ(cpu.pc, 3);
  }

  void TestBrcc() {
    cpu.sreg = 0;

    socket.writew<u16>(0, 0xf408);  // BRCC C, 1
    sc_core::wait(kCycle, sc_core::SC_US);
    ASSERT_EQ(cpu.pc, 2);

    cpu.sreg.c = 1;
    socket.writew<u16>(4, 0xf408);  // BRCC C, 1
    sc_core::wait(kCycle, sc_core::SC_US);
    ASSERT_EQ(cpu.pc, 3);

    cpu.sreg.c = 0;
    socket.writew<u16>(6, 0xf7f0);  // BRCC C, -2
    sc_core::wait(kCycle, sc_core::SC_US);
    ASSERT_EQ(cpu.pc, 2);
  }

  void TestBrcs() {
    cpu.sreg = 0;

    socket.writew<u16>(0, 0xf008);  // BRCS C, 1
    sc_core::wait(kCycle, sc_core::SC_US);
    ASSERT_EQ(cpu.pc, 1);

    cpu.sreg.c = 1;
    socket.writew<u16>(2, 0xf008);  // BRCS C, 1
    sc_core::wait(kCycle, sc_core::SC_US);
    ASSERT_EQ(cpu.pc, 3);

    socket.writew<u16>(6, 0xf3f0);  // BRCS C, -2
    sc_core::wait(kCycle, sc_core::SC_US);
    ASSERT_EQ(cpu.pc, 2);
  }

  void TestBreak() {
    socket.writew<u16>(0, 0x9598);  // BREAK
    sc_core::wait(kCycle, sc_core::SC_US);
    ASSERT_TRUE(cpu.break_hit);
  }

  void TestBreq() {
    socket.writew<u16>(0, 0xf009);  // BREQ 1
    sc_core::wait(kCycle, sc_core::SC_US);
    ASSERT_EQ(cpu.pc, 1);

    cpu.sreg.z = 1;
    socket.writew<u16>(2, 0xf009);  // BREQ 1
    sc_core::wait(kCycle, sc_core::SC_US);
    ASSERT_EQ(cpu.pc, 3);

    socket.writew<u16>(6, 0xf3f1);  // BREQ -2
    sc_core::wait(kCycle, sc_core::SC_US);
    ASSERT_EQ(cpu.pc, 2);
  }

  void TestBset() {
    cpu.sreg = 0x00;
    socket.writew<u16>(0, 0x9408);  // BSET C
    sc_core::wait(kCycle, sc_core::SC_US);
    ASSERT_EQ(cpu.sreg.i, 0);
    ASSERT_EQ(cpu.sreg.t, 0);
    ASSERT_EQ(cpu.sreg.h, 0);
    ASSERT_EQ(cpu.sreg.s, 0);
    ASSERT_EQ(cpu.sreg.v, 0);
    ASSERT_EQ(cpu.sreg.n, 0);
    ASSERT_EQ(cpu.sreg.z, 0);
    ASSERT_EQ(cpu.sreg.c, 1);

    cpu.sreg = 0x00;
    socket.writew<u16>(2, 0x9418);  // BSET Z
    sc_core::wait(kCycle, sc_core::SC_US);
    ASSERT_EQ(cpu.sreg.i, 0);
    ASSERT_EQ(cpu.sreg.t, 0);
    ASSERT_EQ(cpu.sreg.h, 0);
    ASSERT_EQ(cpu.sreg.s, 0);
    ASSERT_EQ(cpu.sreg.v, 0);
    ASSERT_EQ(cpu.sreg.n, 0);
    ASSERT_EQ(cpu.sreg.z, 1);
    ASSERT_EQ(cpu.sreg.c, 0);

    cpu.sreg = 0x00;
    socket.writew<u16>(4, 0x9428);  // BSET N
    sc_core::wait(kCycle, sc_core::SC_US);
    ASSERT_EQ(cpu.sreg.i, 0);
    ASSERT_EQ(cpu.sreg.t, 0);
    ASSERT_EQ(cpu.sreg.h, 0);
    ASSERT_EQ(cpu.sreg.s, 0);
    ASSERT_EQ(cpu.sreg.v, 0);
    ASSERT_EQ(cpu.sreg.n, 1);
    ASSERT_EQ(cpu.sreg.z, 0);
    ASSERT_EQ(cpu.sreg.c, 0);

    cpu.sreg = 0x00;
    socket.writew<u16>(6, 0x9438);  // BSET V
    sc_core::wait(kCycle, sc_core::SC_US);
    ASSERT_EQ(cpu.sreg.i, 0);
    ASSERT_EQ(cpu.sreg.t, 0);
    ASSERT_EQ(cpu.sreg.h, 0);
    ASSERT_EQ(cpu.sreg.s, 0);
    ASSERT_EQ(cpu.sreg.v, 1);
    ASSERT_EQ(cpu.sreg.n, 0);
    ASSERT_EQ(cpu.sreg.z, 0);
    ASSERT_EQ(cpu.sreg.c, 0);

    cpu.sreg = 0x00;
    socket.writew<u16>(8, 0x9448);  // BSET S
    sc_core::wait(kCycle, sc_core::SC_US);
    ASSERT_EQ(cpu.sreg.i, 0);
    ASSERT_EQ(cpu.sreg.t, 0);
    ASSERT_EQ(cpu.sreg.h, 0);
    ASSERT_EQ(cpu.sreg.s, 1);
    ASSERT_EQ(cpu.sreg.v, 0);
    ASSERT_EQ(cpu.sreg.n, 0);
    ASSERT_EQ(cpu.sreg.z, 0);
    ASSERT_EQ(cpu.sreg.c, 0);

    cpu.sreg = 0x00;
    socket.writew<u16>(10, 0x9458);  // BSET H
    sc_core::wait(kCycle, sc_core::SC_US);
    ASSERT_EQ(cpu.sreg.i, 0);
    ASSERT_EQ(cpu.sreg.t, 0);
    ASSERT_EQ(cpu.sreg.h, 1);
    ASSERT_EQ(cpu.sreg.s, 0);
    ASSERT_EQ(cpu.sreg.v, 0);
    ASSERT_EQ(cpu.sreg.n, 0);
    ASSERT_EQ(cpu.sreg.z, 0);
    ASSERT_EQ(cpu.sreg.c, 0);

    cpu.sreg = 0x00;
    socket.writew<u16>(12, 0x9468);  // BSET T
    sc_core::wait(kCycle, sc_core::SC_US);
    ASSERT_EQ(cpu.sreg.i, 0);
    ASSERT_EQ(cpu.sreg.t, 1);
    ASSERT_EQ(cpu.sreg.h, 0);
    ASSERT_EQ(cpu.sreg.s, 0);
    ASSERT_EQ(cpu.sreg.v, 0);
    ASSERT_EQ(cpu.sreg.n, 0);
    ASSERT_EQ(cpu.sreg.z, 0);
    ASSERT_EQ(cpu.sreg.c, 0);

    cpu.sreg = 0x00;
    socket.writew<u16>(14, 0x9478);  // BSET I
    sc_core::wait(kCycle, sc_core::SC_US);
    ASSERT_EQ(cpu.sreg.i, 1);
    ASSERT_EQ(cpu.sreg.t, 0);
    ASSERT_EQ(cpu.sreg.h, 0);
    ASSERT_EQ(cpu.sreg.s, 0);
    ASSERT_EQ(cpu.sreg.v, 0);
    ASSERT_EQ(cpu.sreg.n, 0);
    ASSERT_EQ(cpu.sreg.z, 0);
    ASSERT_EQ(cpu.sreg.c, 0);
  }

  void TestBst() {
    cpu.rf[0] = 1;
    socket.writew<u16>(0, 0xfa00);  // BST R0, 0
    sc_core::wait(kCycle, sc_core::SC_US);
    ASSERT_EQ(cpu.sreg.t, 1);

    cpu.rf[0] = 0;
    socket.writew<u16>(2, 0xfa00);  // BST R0, 0
    sc_core::wait(kCycle, sc_core::SC_US);
    ASSERT_EQ(cpu.sreg.t, 0);
  }

  void TestCall() {
    cpu.sp = 0x8ff;
    socket.writew<u16>(0, 0x0000);      // NOP
    socket.writew<u16>(2, 0x0000);      // NOP
    socket.writew<u32>(4, 0x0010940e);  // CALL 0x10
    sc_core::wait(6 * kCycle, sc_core::SC_US);
    ASSERT_EQ(cpu.pc, 0x10);
    ASSERT_EQ(cpu.sp, 0x8fd);
    memory.data()[0x8ff] = 2;
  }

  void TestCbi() {
    cpu.gpior0 = 0xff;

    socket.writew<u16>(0, 0x98f0);  // CBI GPIOR0, 0
    sc_core::wait(2 * kCycle, sc_core::SC_US);
    ASSERT_EQ((uint)cpu.gpior0, 0xfe);

    socket.writew<u16>(2, 0x98f7);  // CBI GPIOR0, 7
    sc_core::wait(2 * kCycle, sc_core::SC_US);
    ASSERT_EQ((uint)cpu.gpior0, 0x7e);
  }

  void TestCom() {
    cpu.rf[0] = 0xff;
    socket.writew<u16>(0, 0x9400);  // COM R0
    sc_core::wait(kCycle, sc_core::SC_US);
    ASSERT_EQ(cpu.rf[0], 0x00);

    cpu.rf[1] = 0x01;
    socket.writew<u16>(2, 0x9410);  // COM R1
    sc_core::wait(kCycle, sc_core::SC_US);
    ASSERT_EQ(cpu.rf[1], 0xfe);
  }

  void TestCp() {
    cpu.rf[0] = 0x00;
    cpu.rf[1] = 0x00;
    socket.writew<u16>(0, 0x1410);  // CP R1, R0
    sc_core::wait(kCycle, sc_core::SC_US);
    ASSERT_EQ(cpu.rf[1], 0x00);
    ASSERT_EQ(cpu.sreg.h, 0);
    ASSERT_EQ(cpu.sreg.s, 0);
    ASSERT_EQ(cpu.sreg.v, 0);
    ASSERT_EQ(cpu.sreg.n, 0);
    ASSERT_EQ(cpu.sreg.z, 1);
    ASSERT_EQ(cpu.sreg.c, 0);

    cpu.rf[0] = 0x01;
    cpu.rf[1] = 0x00;
    socket.writew<u16>(2, 0x1410);  // CP R1, R0
    sc_core::wait(kCycle, sc_core::SC_US);
    ASSERT_EQ(cpu.rf[1], 0x00);
    ASSERT_EQ(cpu.sreg.h, 1);
    ASSERT_EQ(cpu.sreg.s, 0);
    ASSERT_EQ(cpu.sreg.v, 0);
    ASSERT_EQ(cpu.sreg.n, 1);
    ASSERT_EQ(cpu.sreg.z, 0);
    ASSERT_EQ(cpu.sreg.c, 1);

    cpu.rf[0] = 0x00;
    cpu.rf[1] = 0x01;
    socket.writew<u16>(4, 0x1410);  // CP R1, R0
    sc_core::wait(kCycle, sc_core::SC_US);
    ASSERT_EQ(cpu.rf[1], 0x01);
    ASSERT_EQ(cpu.sreg.h, 0);
    ASSERT_EQ(cpu.sreg.s, 0);
    ASSERT_EQ(cpu.sreg.v, 0);
    ASSERT_EQ(cpu.sreg.n, 0);
    ASSERT_EQ(cpu.sreg.z, 0);
    ASSERT_EQ(cpu.sreg.c, 0);
  }

  void TestIjmp() {
    cpu.z = 0x1337;
    socket.writew<u16>(0, 0x9409);  // IMP
    sc_core::wait(2 * kCycle, sc_core::SC_US);
    ASSERT_EQ(cpu.pc, 0x1337);

    cpu.rf[30] = 0x30;
    socket.writew<u16>(2 * 0x1337, 0x9409);  // IMP
    sc_core::wait(2 * kCycle, sc_core::SC_US);
    ASSERT_EQ(cpu.pc, 0x1330);
  }

  void TestLdi() {
    socket.writew<u16>(0, 0xe00f);  // LDI R6, 15
    sc_core::wait(1 * kCycle, sc_core::SC_US);
    ASSERT_EQ(cpu.rf[16], 15);

    socket.writew<u16>(2, 0xe1e7);  // LDI R30, 23
    sc_core::wait(1 * kCycle, sc_core::SC_US);
    ASSERT_EQ(cpu.rf[30], 23);
  }

  void TestMov() {
    cpu.rf[0] = 23;
    socket.writew<u16>(0, 0x2c10);  // MOV R1, RO
    sc_core::wait(1 * kCycle, sc_core::SC_US);
    ASSERT_EQ(cpu.rf[0], 23);
    ASSERT_EQ(cpu.rf[1], 23);

    cpu.rf[30] = 42;
    socket.writew<u16>(2, 0x2e2e);  // MOV R2, R30
    sc_core::wait(1 * kCycle, sc_core::SC_US);
    ASSERT_EQ(cpu.rf[2], 42);
    ASSERT_EQ(cpu.rf[30], 42);
  }

  void TestMovw() {
    cpu.rf[0] = 13;
    cpu.rf[1] = 37;
    socket.writew<u16>(0, 0x0110);  // MOV R2, RO
    sc_core::wait(1 * kCycle, sc_core::SC_US);
    ASSERT_EQ(cpu.rf[2], 13);
    ASSERT_EQ(cpu.rf[3], 37);

    cpu.rf[2] = 0xca;
    cpu.rf[3] = 0xfe;
    socket.writew<u16>(2, 0x01e1);  // MOV 28, R2
    sc_core::wait(1 * kCycle, sc_core::SC_US);
    ASSERT_EQ(cpu.rf[28], 0xca);
    ASSERT_EQ(cpu.rf[29], 0xfe);
  }

  void TestMul() {
    cpu.sreg = 0;
    cpu.rf[0] = 13;
    cpu.rf[1] = 37;
    socket.writew<u16>(0, 0x9c10);  // MUL R1, RO
    sc_core::wait(2 * kCycle, sc_core::SC_US);
    ASSERT_EQ(cpu.rf[0], 0xe1);
    ASSERT_EQ(cpu.rf[1], 0x01);
    ASSERT_EQ(cpu.sreg.z, 0);
    ASSERT_EQ(cpu.sreg.c, 0);

    cpu.sreg = 0;
    cpu.rf[5] = 1;
    cpu.rf[6] = 0;
    socket.writew<u16>(2, 0x9c56);  // MUL R5, R6
    sc_core::wait(2 * kCycle, sc_core::SC_US);
    ASSERT_EQ(cpu.rf[0], 0);
    ASSERT_EQ(cpu.rf[1], 0);
    ASSERT_EQ(cpu.sreg.z, 1);
    ASSERT_EQ(cpu.sreg.c, 0);

    cpu.sreg = 0;
    cpu.rf[5] = 0xff;
    cpu.rf[10] = 0xff;
    socket.writew<u16>(4, 0x9c5a);  // MUL R5, R10
    sc_core::wait(2 * kCycle, sc_core::SC_US);
    ASSERT_EQ(cpu.rf[0], 0x01);
    ASSERT_EQ(cpu.rf[1], 0xfe);
    ASSERT_EQ(cpu.sreg.z, 0);
    ASSERT_EQ(cpu.sreg.c, 1);
  }

  void TestOri() {
    cpu.sreg = 0;
    socket.writew<u16>(0, 0x6f0f);  // ORI R16, 0xFF
    sc_core::wait(1 * kCycle, sc_core::SC_US);
    ASSERT_EQ(cpu.rf[16], 0xff);
    ASSERT_EQ(cpu.sreg.n, 1);
    ASSERT_EQ(cpu.sreg.z, 0);

    cpu.sreg = 0;
    socket.writew<u16>(2, 0x6010);  // ORI R17, 0x00
    sc_core::wait(1 * kCycle, sc_core::SC_US);
    ASSERT_EQ(cpu.rf[17], 0x00);
    ASSERT_EQ(cpu.sreg.n, 0);
    ASSERT_EQ(cpu.sreg.z, 1);
  }

  void TestOut() {
    cpu.rf[1] = 42;
    socket.writew<u16>(0, 0xba1e);  // OUT GPIOR0, R1
    sc_core::wait(1 * kCycle, sc_core::SC_US);
    ASSERT_EQ(cpu.gpior0, 42);
  }

  void TestPop() {
    cpu.sp = 0x800;
    socket.writew<u8>(0x801, 42);
    socket.writew<u16>(0, 0x91ff);  // POP R31
    sc_core::wait(2 * kCycle, sc_core::SC_US);
    ASSERT_EQ(cpu.rf[31], 42);
  }

  void TestPush() {
    cpu.sp = 0x801;
    cpu.rf[31] = 99;
    socket.writew<u16>(0, 0x93ff);  // PUSH R31
    sc_core::wait(2 * kCycle, sc_core::SC_US);
    ASSERT_EQ(memory.get<u8>(0x801), 99);
  }

  void TestRcall() {
    cpu.sp = 0x800;
    socket.writew<u16>(0, 0x0000);  // NOP
    socket.writew<u16>(2, 0xd080);  // RCALL 0x80
    sc_core::wait(4 * kCycle, sc_core::SC_US);
    ASSERT_EQ(cpu.pc, 0x82);
    ASSERT_EQ(cpu.sp, 0x7fe);
    ASSERT_EQ(memory.get<u8>(0x800), 0x0);
    ASSERT_EQ(memory.get<u8>(0x7ff), 0x1 + 1);

    socket.writew<u16>(0x82 * 2, 0xdffc);  // RCALL -4
    sc_core::wait(3 * kCycle, sc_core::SC_US);
    ASSERT_EQ(cpu.pc, 0x7f);
    ASSERT_EQ(cpu.sp, 0x7fc);
    ASSERT_EQ(memory.get<u8>(0x7fe), 0x0);
    ASSERT_EQ(memory.get<u8>(0x7fd), 0x82 + 1);
  }

  void TestRet() {
    cpu.sreg = 0;
    cpu.sp = 0x800;
    socket.writew<u16>(0, 0x9508);  // RET
    socket.writew<u8>(0x801, 0x37);
    socket.writew<u8>(0x802, 0x13);
    sc_core::wait(4 * kCycle, sc_core::SC_US);
    ASSERT_EQ(cpu.pc, 0x1337);
    ASSERT_EQ(cpu.sp, 0x802);
    ASSERT_EQ(cpu.sreg.i, 0);
  }

  void TestReti() {
    cpu.sreg = 0;
    cpu.sp = 0x800;
    socket.writew<u16>(0, 0x9518);  // RETI
    socket.writew<u8>(0x801, 0x37);
    socket.writew<u8>(0x802, 0x13);
    sc_core::wait(4 * kCycle, sc_core::SC_US);
    ASSERT_EQ(cpu.pc, 0x1337);
    ASSERT_EQ(cpu.sp, 0x802);
    ASSERT_EQ(cpu.sreg.i, 1);
  }

  void TestRjmp() {
    socket.writew<u16>(0, 0x0000);  // NOP
    socket.writew<u16>(2, 0xc00f);  // RJMP 0xf
    sc_core::wait(3 * kCycle, sc_core::SC_US);
    ASSERT_EQ(cpu.pc, 0x11);
  }

  void TestRol() {
    cpu.sreg = 0;
    cpu.rf[31] = 0x80;
    socket.writew<u16>(0, 0x1fff);  // ROL R31
    sc_core::wait(1 * kCycle, sc_core::SC_US);
    ASSERT_EQ(cpu.rf[31], 0x00);
    ASSERT_EQ(cpu.sreg.s, 0);
    ASSERT_EQ(cpu.sreg.v, 1);
    ASSERT_EQ(cpu.sreg.n, 0);
    ASSERT_EQ(cpu.sreg.z, 1);
    ASSERT_EQ(cpu.sreg.c, 1);

    cpu.sreg = 0;
    cpu.rf[31] = 0x7f;
    socket.writew<u16>(2, 0x1fff);  // ROL R31
    sc_core::wait(1 * kCycle, sc_core::SC_US);
    ASSERT_EQ(cpu.rf[31], 0xfe);
    ASSERT_EQ(cpu.sreg.s, 0);
    ASSERT_EQ(cpu.sreg.v, 1);
    ASSERT_EQ(cpu.sreg.n, 1);
    ASSERT_EQ(cpu.sreg.z, 0);
    ASSERT_EQ(cpu.sreg.c, 0);

    cpu.sreg = 0;
    cpu.rf[31] = 0x01;
    socket.writew<u16>(4, 0x1fff);  // ROL R31
    sc_core::wait(1 * kCycle, sc_core::SC_US);
    ASSERT_EQ(cpu.rf[31], 0x02);
    ASSERT_EQ(cpu.sreg.s, 0);
    ASSERT_EQ(cpu.sreg.v, 0);
    ASSERT_EQ(cpu.sreg.n, 0);
    ASSERT_EQ(cpu.sreg.z, 0);
    ASSERT_EQ(cpu.sreg.c, 0);
  }

  void TestRor() {
    cpu.sreg = 0;
    cpu.rf[30] = 0x01;
    socket.writew<u16>(0, 0x95e7);  // ROR R31
    sc_core::wait(1 * kCycle, sc_core::SC_US);
    ASSERT_EQ(cpu.rf[30], 0x00);
    ASSERT_EQ(cpu.sreg.s, 1);
    ASSERT_EQ(cpu.sreg.v, 1);
    ASSERT_EQ(cpu.sreg.n, 0);
    ASSERT_EQ(cpu.sreg.z, 1);
    ASSERT_EQ(cpu.sreg.c, 1);

    cpu.sreg = 0;
    cpu.rf[30] = 0x00;
    socket.writew<u16>(2, 0x95e7);  // ROR R31
    sc_core::wait(1 * kCycle, sc_core::SC_US);
    ASSERT_EQ(cpu.rf[30], 0x00);
    ASSERT_EQ(cpu.sreg.s, 0);
    ASSERT_EQ(cpu.sreg.v, 0);
    ASSERT_EQ(cpu.sreg.n, 0);
    ASSERT_EQ(cpu.sreg.z, 1);
    ASSERT_EQ(cpu.sreg.c, 0);

    cpu.sreg = 0;
    cpu.sreg.c = 1;
    cpu.rf[30] = 0x00;
    socket.writew<u16>(4, 0x95e7);  // ROR R31
    sc_core::wait(1 * kCycle, sc_core::SC_US);
    ASSERT_EQ(cpu.rf[30], 0x80);
    ASSERT_EQ(cpu.sreg.s, 0);
    ASSERT_EQ(cpu.sreg.v, 1);
    ASSERT_EQ(cpu.sreg.n, 1);
    ASSERT_EQ(cpu.sreg.z, 0);
    ASSERT_EQ(cpu.sreg.c, 0);
  }
};

TEST(ArduSystem, CpuTest) {
  CpuTest test("cpu_test");
  sc_core::sc_start();
}
