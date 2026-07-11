#include "system.h"

#include "testing.h"

class SystemTest : public test_base {
 public:
  ArduSystem system;

 public:
  SystemTest(const sc_module_name& nm) : test_base(nm), system("bus") { add_test("start", &SystemTest::TestStart); }

  void TestStart() { sc_core::wait(10, sc_core::SC_US); }
};

TEST(ArduSystem, SystemTest) {
  SystemTest test("test");
  sc_core::sc_start();
}
