#include <array>

#include "common.h"

using RegType = u8;

struct Cpu {
  std::array<RegType, 32> rf = {0};  // Register file
  u16 pc = 0;                        // Program counter
  u16 sp = 0;                        // Stack pointer
  u16 x = 0;                         // X register
  u16 y = 0;                         // Y register
  u16 z = 0;                         // Z register

  u32 watchdog_timer = 0;
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
  bool skip;
  bool break_hit;
  bool sleeping;

  template <typename D>
  D MemoryRead(u32 adr) {
    // TODO: Implement.
    return 0;
  }

  template <typename D>
  void MemoryWrite(u64 adr, D dat) {
    // TODO: Implement.
    return;
  }
};