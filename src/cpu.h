#include <vcml.h>

#include <array>
#include <chrono>
#include <functional>

#include "common.h"

using RegType = u8;
using AddrType = u16;

struct Cpu : public vcml::processor {
  Cpu();
  virtual ~Cpu() = default;

  vcml::gpio_initiator_array<> irq_ack;

  std::array<RegType, 32> rf = {0};
  u16 pc = 0;
  u16 sp = 0x8ff;
  u16& x;
  u16& y;
  u16& z;

  u32 watchdog_timer = 0;

  struct Sreg {
    union {
      struct {
        u8 c : 1;
        u8 z : 1;
        u8 n : 1;
        u8 v : 1;
        u8 s : 1;
        u8 h : 1;
        u8 t : 1;
        u8 i : 1;
      };
      u8 value;
    };

    operator u8() const { return value; }
    Sreg& operator=(u8 v) {
      value = v;
      return *this;
    }
  } sreg;

  bool skip;
  bool branch_taken;
  bool break_hit;
  bool sleeping;

  u16 new_pc = 0;
  u16 start_pc = 0;
  u64 cycle_counter = 0;
  u64 retired_instructions = 0;

  std::function<void()> FetchInstruction(vcml::u64 adr, uint* cyc);

  u16 InsnDmiRead(vcml::u64 adr);

  u8 gpior0 = 0;
  u8 gpior1 = 0;
  u8 gpior2 = 0;

  u8 MemoryRead(u32 adr);
  u8 IoRead(u32 adr);
  void IoWrite(u32 adr, u8 value);
  u8 ProgMemoryRead(u32 adr);
  void MemoryWrite(u64 adr, u8 dat);

  struct SimulationException {};

  struct IllegalInsnException : public SimulationException {};

  virtual mwr::u64 cycle_count() const override;
  virtual void reset() override;
  virtual void simulate(size_t max_cycles) override;
  virtual vcml::u64 program_counter() override;
  virtual vcml::u64 stack_pointer() override;
  virtual bool insert_breakpoint(vcml::u64 addr) override;
  virtual bool remove_breakpoint(vcml::u64 addr) override;
  virtual bool disassemble(vcml::u8* ibuf, vcml::u64& addr, string& code) override;

  bool irq_sei_delay = false;

 protected:
  static u32 constexpr kIoOffset = 0x20;
  virtual vcml::u64 read_pmem_dbg(vcml::u64 addr, void* ptr, vcml::u64 sz) override;
  virtual bool read_reg_dbg(size_t idx, void* buf, size_t len) override;
  virtual bool write_reg_dbg(size_t idx, const void*, size_t l) override;
  virtual void interrupt(size_t irq, bool set) override;
  virtual void invalidate_direct_mem_ptr(vcml::tlm_initiator_socket& origin, vcml::u64 start, vcml::u64 end) override;
  void start_of_simulation() override;
  void end_of_simulation() override;

  bool service_pending_interrupt();

  u8* insn_dmi_ptr_ = nullptr;

  u64 irq_level_mask_ = 0;  // bit i: irq[i] currently asserted
  u64 irq_acked_mask_ = 0;  // bit i: irq[i] already accepted, wait for deassert

  std::vector<u16> breakpoints_;
  std::vector<u16> watchpoints_;

  std::chrono::steady_clock::time_point sim_start_time_;
};
