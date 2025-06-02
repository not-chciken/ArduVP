#include "cpu.h"
#include "decode/decode.h"

#include <vcml.h>

Cpu::Cpu()
  : vcml::processor("cpu", "avr") {

  set_little_endian();

  // See: vcml/debugging/gdbarch.*
  // for (int i = 0; i < rf.size(); ++i)
  //   define_cpureg_rw(i, RegName(i), sizeof(RegType));

  // define_cpureg_rw(32, "pc", sizeof(AddrType));
}

std::function<void()> Cpu::FetchInstruction(vcml::u64 adr) {
  u16 data;
  tlm::tlm_response_status rs = insn.read(adr, &data, 2, vcml::SBI_NONE);

  if (rs == tlm::TLM_ADDRESS_ERROR_RESPONSE) {
    // TODO: Implement.
    return std::function<void()>();
  } else if (rs != tlm::TLM_OK_RESPONSE) {
    log_bus_error(insn, vcml::VCML_ACCESS_READ, rs, adr, 2);
    exit(1);
  }

  Insn* inst = Decode::DecodeInsn(data);

  if (inst->is_32bit) {
    u16 data2;
     rs = insn.read(adr, &data2, 2, vcml::SBI_NONE);
    if (rs == tlm::TLM_ADDRESS_ERROR_RESPONSE) {
      // TODO: Implement.
      return std::function<void()>();
    } else if (rs != tlm::TLM_OK_RESPONSE) {
      log_bus_error(insn, vcml::VCML_ACCESS_READ, rs, adr, 2);
      exit(1);
    }
    return inst->GenFunc32(data, data2, this);
  }

  return inst->GenFunc(data, this);
}


void Cpu::simulate(size_t cycles) {
  for (int i = 0; i < cycles; ++i) {
    if (std::find(breakpoints_.begin(), breakpoints_.end(), pc) != breakpoints_.end()) {
      notify_breakpoint_hit(pc);
      return;
    }

    auto inst_func = FetchInstruction(pc);
    if (inst_func) {
      try {
        inst_func();
      } catch (...) {
      }
    }
    pc = new_pc;
    ++retired_instructions;
    ++cycle_counter;
  }
  wait_clock_cycles(cycles);
};

void Cpu::reset() {
  pc = start_pc;
  sp = 0xFFFF;
  x = 0;
  y = 0;
  z = 0;
  sreg = {false, false, false, false, false, false, false, true}; // Global interrupt enabled
  skip = false;
  break_hit = false;
  sleeping = false;
  cycle_counter = 0;

  for (auto& reg : rf) {
      reg = 0;
  }

  watchdog_timer = 0;
}

mwr::u64 Cpu::cycle_count() const {
  return cycle_counter;
};

vcml::u64 Cpu::program_counter() {
  return pc;
}

vcml::u64 Cpu::stack_pointer() {
  return sp;
}

bool Cpu::insert_breakpoint(vcml::u64 addr) {
  if (std::find(breakpoints_.begin(), breakpoints_.end(), addr) == breakpoints_.end())
    breakpoints_.push_back(addr);
  return true;
}

bool Cpu::remove_breakpoint(vcml::u64 addr) {
  breakpoints_.erase(std::remove(breakpoints_.begin(), breakpoints_.end(), addr), breakpoints_.end());
  return true;
}

bool Cpu::read_reg_dbg(size_t regno, void* buf, size_t len) {
  if (regno < rf.size())
    *(RegType*)buf = rf[regno];
  else if (regno == rf.size())
    *(AddrType*)buf = pc;
  else
    *(RegType*)buf = 0;

  return true;
}

bool Cpu::write_reg_dbg(size_t regno, const void* buf, size_t len) {
  if (regno < rf.size())
    rf[regno] = *(RegType*)buf;
  else if (regno == rf.size())
    pc = *(AddrType*)buf;

  return true;
}