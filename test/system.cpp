#include "system.h"

#include "testing.h"

struct SystemTest : public test_base {
  ArduSystem system;

  SystemTest(const sc_module_name& nm) : test_base(nm), system("system") {
    add_test("start", &SystemTest::TestLoop);
    add_test("nop", &SystemTest::TestNop);
  }

  void TestLoop() {
    system.program_mem.set<u16>(0, 0xcfff);  // Infinite loop (rjmp .-2).

    sc_core::wait(10, sc_core::SC_US);
    sc_core::wait(sc_core::SC_ZERO_TIME);

    ASSERT_EQ(system.cpu.cycle_count(), 162);
    ASSERT_EQ(system.cpu.pc, 0);
    ASSERT_EQ(system.cpu.retired_instructions, 81);
    system.reset.rst.pulse();
  }

  void TestNop() {
    std::memset(system.program_mem.data(), 0, system.program_mem.size);  // NOP everywhere.

    sc_core::wait(10, sc_core::SC_US);
    sc_core::wait(sc_core::SC_ZERO_TIME);

    ASSERT_EQ(system.cpu.cycle_count(), 159);
    ASSERT_EQ(system.cpu.pc, 159);
    ASSERT_EQ(system.cpu.retired_instructions, 159);
    system.reset.rst.pulse();
  }
};

TEST(ArduSystem, SystemTest) {
  SystemTest test("test");
  sc_core::sc_start();
}
