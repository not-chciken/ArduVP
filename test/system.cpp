#include "system.h"

#include "testing.h"

class SystemTest : public test_base {
 public:
  ArduSystem system;

 public:
  SystemTest(const sc_module_name& nm) : test_base(nm), system("system") {
    add_test("start", &SystemTest::TestLoop);
    add_test("nop", &SystemTest::TestNop);
  }

  void TestLoop() {
    system.program_mem.set<u16>(0, 0xcfff);  // Infinite loop (rjmp .-2).

    sc_core::wait(10, sc_core::SC_US);

    ASSERT_EQ(system.cpu.cycle_count(), 160);
    ASSERT_EQ(system.cpu.pc, 0);
    ASSERT_EQ(system.cpu.retired_instructions, 80);
    system.reset.rst.pulse();
  }

  void TestNop() {
    std::memset(system.program_mem.data(), 0, system.program_mem.size);  // NOP everywhere.

    sc_core::wait(10, sc_core::SC_US);

    ASSERT_EQ(system.cpu.cycle_count(), 160);
    ASSERT_EQ(system.cpu.pc, 160);
    ASSERT_EQ(system.cpu.retired_instructions, 160);
    system.reset.rst.pulse();
  }
};

TEST(ArduSystem, SystemTest) {
  SystemTest test("test");
  sc_core::sc_start();
}
