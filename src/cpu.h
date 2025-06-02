#include <array>
#include <functional>
#include <vcml.h>

#include "common.h"

using RegType = u8;
using AddrType = u16;
struct Cpu : public vcml::processor {
  Cpu();

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

  u16 new_pc = 0;
  u16 start_pc = 0;
  u64 cycle_counter = 0;
  u64 retired_instructions = 0;

  std::function<void()> FetchInstruction(vcml::u64 adr);

  template <typename D>
  D MemoryRead(u32 adr) {
    // TODO: Implement.
    return 0;
  }

  u8 ProgMemoryRead(u32 adr) {
    // TODO: Implement.
    return 0;
  }

  template <typename D>
  void MemoryWrite(u64 adr, D dat) {
    // TODO: Implement.
    return;
  }

  struct SimulationException {
  };

  struct IllegalInsnException : public SimulationException {
  };

  virtual mwr::u64 cycle_count() const override;
  virtual void reset() override;
  virtual void simulate(size_t cycles) override;
  virtual vcml::u64 program_counter() override;
  virtual vcml::u64 stack_pointer() override;
  virtual bool insert_breakpoint(vcml::u64 addr) override;
  virtual bool remove_breakpoint(vcml::u64 addr) override;

protected:
  virtual bool read_reg_dbg(size_t idx, void* buf, size_t len) override;
  virtual bool write_reg_dbg(size_t idx, const void*, size_t l) override;
  std::vector<u16> breakpoints_;
  std::vector<u16> watchpoints_;

};