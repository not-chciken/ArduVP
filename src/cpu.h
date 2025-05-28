#include "common.h"
#include <array>

using RegType = u8;


struct Cpu {
  std::array<RegType, 32> rf = {0}; // Register file
  u16 pc = 0; // Program counter
  struct Sreg {
    bool c;  // Carry
    bool z;  // Zero
    bool n;  // Negative
    bool v;  // Two's complement overflow
    bool s;  // Sign
    bool h;  // Half-carry
    bool t;  // Transfer
    bool i;  // Global interrupt enable
  } sreg;
};