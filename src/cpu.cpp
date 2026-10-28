/******************************************************************************
 * Apache License, Version 2.0
 * Copyright (c) 2026 chciken/Niko Zurstraßen
 *****************************************************************************/

#include "cpu.h"

#include <vcml.h>

#include "decode/decode.h"

constexpr u64 PROGRAM_MEM_SIZE = 0x8000;
constexpr u64 SRAM_BASE = 0x100;
constexpr u64 SRAM_SIZE = 0x800;

Cpu::Cpu()
    : vcml::processor("cpu", "avr"),
      irq_ack("irq_ack"),
      x(*reinterpret_cast<u16*>(&rf[26])),
      y(*reinterpret_cast<u16*>(&rf[28])),
      z(*reinterpret_cast<u16*>(&rf[30])) {
  set_little_endian();

  // See: vcml/debugging/gdbarch.*
  for (size_t i = 0; i < rf.size(); ++i)
    define_cpureg_rw(i, RegName(i), sizeof(RegType));

  define_cpureg_rw(32, "sreg", sizeof(RegType));
  define_cpureg_rw(33, "sp", sizeof(AddrType));
  define_cpureg_rw(34, "pc2", sizeof(u32));
  define_cpureg_rw(35, "pc", sizeof(AddrType));
}

u16 Cpu::InsnDmiRead(vcml::u64 adr) {
  u16 insn_data;

  if (adr >= PROGRAM_MEM_SIZE) {
    tlm::tlm_response_status rs = insn.read(adr, &insn_data, 2, vcml::SBI_NONE);
    if (rs != tlm::TLM_OK_RESPONSE) {
      log_bus_error(insn, vcml::VCML_ACCESS_READ, rs, adr, 2);
      exit(1);
    }
    return insn_data;
  }

  if (insn_dmi_ptr_ == nullptr) {
    insn_dmi_ptr_ = insn.lookup_dmi_ptr(0x0000, PROGRAM_MEM_SIZE, vcml::VCML_ACCESS_READ);
    if (insn_dmi_ptr_ == nullptr) {
      log_error("could not fetch dmi pointer");
      exit(1);
    }
  }
  memcpy(&insn_data, insn_dmi_ptr_ + adr, sizeof(insn_data));
  return insn_data;
}

u8* Cpu::SramDmiPtr(vcml::u64 adr) {
  if (adr < SRAM_BASE || adr >= SRAM_BASE + SRAM_SIZE)
    return nullptr;

  if (sram_dmi_ptr_ == nullptr) {
    sram_dmi_ptr_ = data.lookup_dmi_ptr(SRAM_BASE, SRAM_SIZE, vcml::VCML_ACCESS_READ_WRITE);
    if (sram_dmi_ptr_ == nullptr) {
      log_debug("could not fetch sram dmi pointer, falling back to bus accesses");
      return nullptr;
    }
  }

  return sram_dmi_ptr_ + (adr - SRAM_BASE);
}

Instruction Cpu::FetchInstruction(vcml::u64 adr) {
  DisasContext ctx;
  ctx.next_word = (adr + 4 <= PROGRAM_MEM_SIZE) ? InsnDmiRead(adr + 2) : 0;

  Instruction inst = Decode::DoDecode(InsnDmiRead(adr), &ctx);
  new_pc += inst.is_32bit ? 2 : 1;
  return inst;
}

void Cpu::simulate(size_t max_cycles) {
  sc_core::wait(sc_core::SC_ZERO_TIME);  // Avoids immediate start after reset.

  uint cycles = 0;
  while (cycles < max_cycles) {
    if (service_pending_interrupt()) {
      ++cycle_counter;
      continue;
    }

    if (std::find(breakpoints_.begin(), breakpoints_.end(), pc * 2) != breakpoints_.end()) {
      notify_breakpoint_hit(pc * 2, local_time_stamp());
      return;
    }

    const Instruction inst = FetchInstruction(pc * 2);
    try {
      inst.Execute(this, inst.args);
      ++retired_instructions;
    } catch (...) {
      log_error("Illegal instruction at PC=0x%04x", pc * 2);
      exit(1);
    }

    if (skip) {
      u16 data;
      insn.read(new_pc * 2, &data, 2, vcml::SBI_DEBUG);
      DisasContext ctx = {0};
      const Instruction skipped = Decode::DoDecode(data, &ctx);
      cycles += skipped.is_32bit ? 2 : 1;
      new_pc += skipped.is_32bit ? 2 : 1;
      skip = false;
    }

    if (branch_taken) {
      ++cycles;
      branch_taken = false;
    }

    pc = new_pc;
    cycles += inst.cycles;
  }
  cycle_counter += cycles;
};

void Cpu::reset() {
  processor::reset();
  pc = start_pc;
  new_pc = start_pc;
  sp = 0x8ff;
  x = 0;
  y = 0;
  z = 0;
  sreg.c = 0;
  sreg.z = 0;
  sreg.n = 0;
  sreg.v = 0;
  sreg.s = 0;
  sreg.h = 0;
  sreg.t = 0;
  sreg.i = 1;
  skip = false;
  branch_taken = false;
  break_hit = false;
  sleeping = false;
  cycle_counter = 0;
  retired_instructions = 0;

  for (auto& reg : rf) {
    reg = 0;
  }

  watchdog_timer = 0;

  irq_level_mask_ = 0;
  irq_acked_mask_ = 0;
  irq_sei_delay = false;
}

u8 Cpu::MemoryRead(u32 adr) {
  if (adr <= 0x5f) {
    if (adr <= 0x1f) {
      return rf[adr];
    } else {
      return IoRead(adr - 0x20);
    }
  }

  if (u8* dmi_ptr = SramDmiPtr(adr))
    return *dmi_ptr;

  u8 dat;
  tlm::tlm_response_status rs = data.read(adr, &dat, sizeof(u8), vcml::SBI_NONE);

  if (rs != tlm::TLM_OK_RESPONSE) {
    log_bus_error(insn, vcml::VCML_ACCESS_READ, rs, adr, sizeof(u8));
    log_warn("unimplemented memory read addr=%x", adr);
  }

  return dat;
}

u8 Cpu::IoRead(u32 adr) {
  switch (adr) {
  case 0x1e:
    return gpior0;
  case 0x2a:
    return gpior1;
  case 0x2b:
    return gpior2;
  case 0x3d:
    return sp & 0xff;
  case 0x3e:
    return (sp >> 8);
  case 0x3f:
    return (u8)sreg;
  default: {
    u8 value;
    auto rs = data.read(adr + kIoOffset, &value, 1, vcml::SBI_NONE);
    if (rs != tlm::TLM_OK_RESPONSE) {
      log_warn("unimplemented memory read addr=%x", adr);
      log_bus_error(data, vcml::VCML_ACCESS_READ, rs, adr + kIoOffset, 1);
    }
    return value;
  }
  }
}

void Cpu::IoWrite(u32 adr, u8 value) {
  switch (adr) {
  case 0x1e:
    gpior0 = value;
    break;
  case 0x2a:
    gpior1 = value;
    break;
  case 0x2b:
    gpior2 = value;
    break;
  case 0x3d:
    sp = (sp & 0xff00) | value;
    break;
  case 0x3e:
    sp = (sp & 0xff) | (value << 8);
    break;
  case 0x3f:
    sreg = value;
    break;
  default: {
    auto rs = data.write(adr + kIoOffset, &value, 1, vcml::SBI_NONE);
    if (rs != tlm::TLM_OK_RESPONSE) {
      log_warn("unimplemented memory read addr=%x", adr);
      log_bus_error(data, vcml::VCML_ACCESS_WRITE, rs, adr + kIoOffset, 1);
    }
  } break;
  }
}

u8 Cpu::ProgMemoryRead(u32 adr) {
  u8 data;
  tlm::tlm_response_status rs = insn.read(adr, &data, 1, vcml::SBI_NONE);

  if (rs != tlm::TLM_OK_RESPONSE) {
    log_bus_error(insn, vcml::VCML_ACCESS_READ, rs, adr, sizeof(u8));
    exit(1);
  }

  return data;
}

void Cpu::MemoryWrite(u64 adr, u8 dat) {
  if (adr <= 0x5f) {
    if (adr <= 0x1f) {
      rf[adr] = dat;
    } else {
      IoWrite(adr - 0x20, dat);
    }
    return;
  }

  if (u8* dmi_ptr = SramDmiPtr(adr)) {
    *dmi_ptr = dat;
    return;
  }

  void* d = &dat;
  tlm::tlm_response_status rs = data.write(adr, d, sizeof(u8), vcml::SBI_NONE);

  if (rs != tlm::TLM_OK_RESPONSE) {
    log_warn("unimplemented memory write addr=%lx", adr);
    log_bus_error(insn, vcml::VCML_ACCESS_WRITE, rs, adr, sizeof(u8));
  }
}

mwr::u64 Cpu::cycle_count() const {
  return cycle_counter;
};

vcml::u64 Cpu::program_counter() {
  return pc * 2;
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

bool Cpu::disassemble(vcml::u8* ibuf, vcml::u64& addr, string& code) {
  const u16 word = ibuf[0] | (ibuf[1] << 8);
  DisasContext ctx = {0};
  Instruction inst = Decode::DoDecode(word, &ctx);

  if (inst.is_32bit) {
    ctx.next_word = ibuf[2] | (ibuf[3] << 8);
    inst = Decode::DoDecode(word, &ctx);
  }

  code = inst.GetStatementString(inst.args);
  addr += inst.is_32bit ? 4 : 2;
  return true;
}

bool Cpu::read_reg_dbg(size_t regno, void* buf, [[maybe_unused]] size_t len) {
  if (regno < rf.size())
    *(RegType*)buf = rf[regno];
  else if (regno == 32)
    *(RegType*)buf = sreg;
  else if (regno == 33)
    *(AddrType*)buf = sp;
  else if (regno == 34)
    *(u32*)buf = pc * 2;
  else if (regno == 35)
    *(AddrType*)buf = pc;
  else
    *(RegType*)buf = 0;

  return true;
}

bool Cpu::write_reg_dbg(size_t regno, const void* buf, [[maybe_unused]] size_t len) {
  if (regno < rf.size())
    rf[regno] = *(RegType*)buf;
  else if (regno == 32)
    sreg = *(RegType*)buf;
  else if (regno == 33)
    sp = *(AddrType*)buf;
  else if (regno == 34)
    pc = (*(u32*)buf) / 2;
  else if (regno == 35)
    pc = (*(u16*)buf);

  return true;
}

void Cpu::interrupt(size_t irq, bool set) {
  if (irq >= 64) {
    return;
  }

  const u64 bit = 1ull << irq;
  if (set) {
    irq_level_mask_ |= bit;
  } else {
    irq_level_mask_ &= ~bit;
    irq_acked_mask_ &= ~bit;
  }

  service_pending_interrupt();
}

void Cpu::invalidate_direct_mem_ptr(vcml::tlm_initiator_socket& origin, vcml::u64 start, vcml::u64 end) {
  processor::invalidate_direct_mem_ptr(origin, start, end);

  if (&origin == &insn) {
    insn_dmi_ptr_ = nullptr;
  } else if (&origin == &data) {
    sram_dmi_ptr_ = nullptr;
  }
}

void Cpu::start_of_simulation() {
  sim_start_time_ = std::chrono::steady_clock::now();
}

void Cpu::end_of_simulation() {
  auto elapsed = std::chrono::steady_clock::now() - sim_start_time_;
  double elapsed_secs = std::chrono::duration<double>(elapsed).count();
  double mips = elapsed_secs > 0.0 ? retired_instructions / elapsed_secs / 1e6 : 0.0;

  log_info("retired instructions: %lu", retired_instructions);
  log_info("cycle count:          %llu", cycle_count());
  log_info("MIPS:                 %.3f", mips);
}

static constexpr u16 IRQ_TABLE[] = {
    0x000E, 0x0010, 0x0012, 0x0014, 0x0016, 0x0018, 0x001A, 0x001C, 0x001E, 0x0020,
    0x0022, 0x0024, 0x0026, 0x0028, 0x002A, 0x002C, 0x002E, 0x0030, 0x0032,
};

bool Cpu::service_pending_interrupt() {
  if (irq_sei_delay) {
    irq_sei_delay = false;
    return false;
  }

  if (!sreg.i)
    return false;

  if (irq_level_mask_ == 0)
    return false;

  for (size_t idx = 0; idx < std::size(IRQ_TABLE); ++idx) {
    const u64 bit = 1ull << idx;
    if ((irq_level_mask_ & bit) == 0)
      continue;
    if ((irq_acked_mask_ & bit) != 0)
      continue;

    irq_acked_mask_ |= bit;

    sleeping = false;

    const u16 ret_pc = new_pc;
    MemoryWrite(sp, (u8)(ret_pc >> 8));
    --sp;
    MemoryWrite(sp, (u8)(ret_pc & 0xff));
    --sp;

    sreg.i = false;

    irq_ack[idx].pulse();

    const u16 vector_word = IRQ_TABLE[idx];
    pc = vector_word;
    new_pc = vector_word;
    return true;
  }

  return false;
}

// For GDB, everything below 0x800000 is instruction memory.
// At 0x800000, the data memory starts.
vcml::u64 Cpu::read_pmem_dbg(vcml::u64 addr, void* buffer, vcml::u64 size) {
  constexpr vcml::u64 GDB_DATA_MEM_START_ADR = 0x800000;
  try {
    if (addr < GDB_DATA_MEM_START_ADR) {
      if (vcml::success(insn.read(addr, buffer, size, vcml::SBI_DEBUG)))
        return size;
    } else {
      if (vcml::success(data.read(addr - GDB_DATA_MEM_START_ADR, buffer, size, vcml::SBI_DEBUG)))
        return size;
    }
  } catch (vcml::report& r) {
    log_warn("error reading %llu bytes to memory at address 0x%llx: %s", size, addr, r.message());
  }
  return 0;
}
