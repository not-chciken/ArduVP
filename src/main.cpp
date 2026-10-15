#include <systemc.h>
#include <vcml.h>

#include "system.h"

extern "C" int sc_main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[]) {
  ArduSystem system("system");
  system.run();
  return 0;
}
