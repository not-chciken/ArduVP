/******************************************************************************
 * Apache License, Version 2.0
 * Copyright (c) 2026 chciken/Niko Zurstraßen
 *****************************************************************************/

#include <systemc.h>
#include <vcml.h>

#include "system.h"

extern "C" int sc_main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[]) {
  ArduSystem system("system");
  system.run();
  return 0;
}
