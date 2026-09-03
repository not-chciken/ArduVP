#include <mwr/logging/publishers/terminal.h>
#include <systemc.h>
#include <vcml.h>

#include <iostream>
#include "system.h"

extern "C" int sc_main(int argc, char* argv[]) {
  ArduSystem system("system");
  system.run();
  return 0;
}