#include "cpu.h"

#include <vcml.h>

#include "decode/decode.h"

Cpu::Cpu()
    : vcml::processor("cpu", "avr"),
      irq_ack("irq_ack"),
      x(*reinterpret_cast<u16*>(&rf[26])),
      y(*reinterpret_cast<u16*>(&rf[28])),
      z(*reinterpret_cast<u16*>(&rf[30])) {
  set_little_endian();

  // See: vcml/debugging/gdbarch.*
  for (int i = 0; i < rf.size(); ++i)
    define_cpureg_rw(i, RegName(i), sizeof(RegType));

  define_cpureg_rw(32, "sreg", sizeof(RegType));
  define_cpureg_rw(33, "sp", sizeof(AddrType));
  define_cpureg_rw(34, "pc", sizeof(u32));

  define_cpureg_rw(IoRegisters::PINB, "pinb", 1);
  define_cpureg_rw(IoRegisters::DDRB, "ddrb", 1);
  define_cpureg_rw(IoRegisters::PORTB, "portb", 1);
  define_cpureg_rw(IoRegisters::PINC, "pinc", 1);
  define_cpureg_rw(IoRegisters::DDRC, "ddrc", 1);
  define_cpureg_rw(IoRegisters::PORTC, "portc", 1);
  define_cpureg_rw(IoRegisters::PIND, "pind", 1);
  define_cpureg_rw(IoRegisters::DDRD, "ddrd", 1);
  define_cpureg_rw(IoRegisters::PORTD, "portd", 1);
  define_cpureg_rw(IoRegisters::PCIFR, "pcifr", 1);
  define_cpureg_rw(IoRegisters::EIFR, "eifr", 1);
  define_cpureg_rw(IoRegisters::EIMSK, "eimsk", 1);
  define_cpureg_rw(IoRegisters::GPIOR0, "gpi0r0", 1);
  define_cpureg_rw(IoRegisters::EECR, "eecr", 1);
  define_cpureg_rw(IoRegisters::EEDR, "eedr", 1);
  define_cpureg_rw(IoRegisters::EEARL, "eearl", 1);
  define_cpureg_rw(IoRegisters::EEARH, "eearh", 1);
  define_cpureg_rw(IoRegisters::GTCCR, "gtccr", 1);
  define_cpureg_rw(IoRegisters::GPIOR1, "gpi0r1", 1);
  define_cpureg_rw(IoRegisters::GPIOR2, "gpi0r2", 1);
  define_cpureg_rw(IoRegisters::SPCR, "spcr", 1);
  define_cpureg_rw(IoRegisters::SPSR, "spsr", 1);
  define_cpureg_rw(IoRegisters::SPDR, "spdr", 1);
  define_cpureg_rw(IoRegisters::ACSR, "acsr", 1);
  define_cpureg_rw(IoRegisters::SCMR, "scmr", 1);
  define_cpureg_rw(IoRegisters::MCUSR, "mcusr", 1);
  define_cpureg_rw(IoRegisters::MCUCR, "mcucr", 1);
  define_cpureg_rw(IoRegisters::SPMCSR, "spmcsr", 1);
  define_cpureg_rw(IoRegisters::SPL, "spl", 1);
  define_cpureg_rw(IoRegisters::SPH, "sph", 1);
  define_cpureg_rw(IoRegisters::SREG, "TODO", 1);
  define_cpureg_rw(IoRegisters::WDTCSR, "wdtcsr", 1);
  define_cpureg_rw(IoRegisters::CLKPR, "clkpr", 1);
  define_cpureg_rw(IoRegisters::PRR, "prr", 1);
  define_cpureg_rw(IoRegisters::OSCCAL, "osccal", 1);
  define_cpureg_rw(IoRegisters::PCICR, "pcicr", 1);
  define_cpureg_rw(IoRegisters::EICRA, "eicra", 1);
  define_cpureg_rw(IoRegisters::PCMSK0, "pcmsk0", 1);
  define_cpureg_rw(IoRegisters::PCMSK1, "pcmsk1", 1);
  define_cpureg_rw(IoRegisters::PCMSK2, "pcmsk2", 1);
  define_cpureg_rw(IoRegisters::DIDR0, "didr0", 1);
  define_cpureg_rw(IoRegisters::DIDR1, "didr1", 1);
  define_cpureg_rw(IoRegisters::ASSR, "assr", 1);
  define_cpureg_rw(IoRegisters::TWBR, "twbr", 1);
  define_cpureg_rw(IoRegisters::TWSR, "twsr", 1);
  define_cpureg_rw(IoRegisters::TWAR, "twar", 1);
  define_cpureg_rw(IoRegisters::TWDR, "twdr", 1);
  define_cpureg_rw(IoRegisters::TWCR, "twcr", 1);
  define_cpureg_rw(IoRegisters::TWAMR, "twamr", 1);
}

std::function<void()> Cpu::FetchInstruction(vcml::u64 adr, uint* cycles) {
  u16 data;
  tlm::tlm_response_status rs = insn.read(adr, &data, 2, vcml::SBI_NONE);

  if (rs == tlm::TLM_ADDRESS_ERROR_RESPONSE) {
    log_bus_error(insn, vcml::VCML_ACCESS_READ, rs, adr, 2);
    exit(1);
  } else if (rs != tlm::TLM_OK_RESPONSE) {
    log_bus_error(insn, vcml::VCML_ACCESS_READ, rs, adr, 2);
    exit(1);
  }

  new_pc += 1;
  Insn::InsnBase* inst = Decode::DecodeInsn(data);
  *cycles = inst->cycles;

  if (inst->is_32bit) {
    new_pc += 1;
    u16 data2;
    rs = insn.read(adr + 2, &data2, 2, vcml::SBI_NONE);
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

void Cpu::simulate(size_t max_cycles) {
  uint cycles = 0;
  while (cycles < max_cycles) {
    // Accept pending interrupts between instructions.
    if (service_pending_interrupt()) {
      ++cycle_counter;
      continue;
    }

    if (std::find(breakpoints_.begin(), breakpoints_.end(), pc * 2) != breakpoints_.end()) {
      notify_breakpoint_hit(pc * 2, local_time_stamp());
      return;
    }

    uint cyc = 0;
    auto inst_func = FetchInstruction(pc * 2, &cyc);
    if (inst_func) {
      try {
        inst_func();
        ++retired_instructions;
      } catch (...) {
        log_error("Illegal instruction at PC=0x%04x", pc * 2);
        exit(1);
      }
    }

    if (skip) {
      u16 data;
      tlm::tlm_response_status rs = insn.read(new_pc, &data, 2, vcml::SBI_DEBUG);
      Insn::InsnBase* insn = Decode::DecodeInsn(data);
      cycles += insn->is_32bit ? 2 : 1;
      new_pc += insn->is_32bit ? 2 : 1;
      skip = false;
    }

    pc = new_pc;
    cycles += cyc;
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
  sreg.i = 1;  // Global interrupt enabled
  skip = false;
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
  u16 word = ibuf[0] | (ibuf[1] << 8);
  Insn::InsnBase* inst = Decode::DecodeInsn(word);

  if (!inst)
    return false;

  if (inst->is_32bit) {
    u16 word2 = ibuf[2] | (ibuf[3] << 8);
    u32 full = ((u32)word2 << 16) | word;
    code = inst->GetStatementString(full);
    addr += 4;
  } else {
    code = inst->GetStatementString(word);
    addr += 2;
  }
  return true;
}

bool Cpu::read_reg_dbg(size_t regno, void* buf, size_t len) {
  if (regno < rf.size())
    *(RegType*)buf = rf[regno];
  else if (regno == 32)
    *(RegType*)buf = sreg;
  else if (regno == 33)
    *(AddrType*)buf = sp;
  else if (regno == 34)
    *(u32*)buf = pc * 2;
  else
    *(RegType*)buf = 0;

  return true;
}

bool Cpu::write_reg_dbg(size_t regno, const void* buf, size_t len) {
  if (regno < rf.size())
    rf[regno] = *(RegType*)buf;
  else if (regno == 32)
    sreg = *(RegType*)buf;
  else if (regno == 33)
    sp = *(AddrType*)buf;
  else if (regno == 34)
    pc = (*(u32*)buf) / 2;

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
    // Once the line is deasserted, it can be accepted again.
    irq_acked_mask_ &= ~bit;
  }

  // If the CPU is currently waiting, try to service immediately.
  // If interrupts are disabled, service_pending_interrupt() will defer.
  (void)service_pending_interrupt();
}

void Cpu::start_of_simulation() {
  sim_start_time_ = std::chrono::steady_clock::now();
}

void Cpu::end_of_simulation() {
  auto elapsed = std::chrono::steady_clock::now() - sim_start_time_;
  double elapsed_secs = std::chrono::duration<double>(elapsed).count();
  double mips = elapsed_secs > 0.0 ? retired_instructions / elapsed_secs / 1e6 : 0.0;

  log_info("Retired instructions: %lu", retired_instructions);
  log_info("Cycle count: %llu", cycle_count());
  log_info("MIPS: %.3f", mips);
}

static constexpr u16 IRQ_TABLE[] = {
    0x000E, 0x0010, 0x0012, 0x0014, 0x0016, 0x0018, 0x001A,
    0x001C, 0x001E, 0x0020, 0x0022, 0x0024, 0x0026, 0x0028,
};

bool Cpu::service_pending_interrupt() {
  if (irq_sei_delay) {
    irq_sei_delay = false;
    return false;
  }

  if (!sreg.i)
    return false;

  for (size_t idx = 0; idx < std::size(IRQ_TABLE); ++idx) {
    const u64 bit = 1ull << idx;
    if ((irq_level_mask_ & bit) == 0)
      continue;
    if ((irq_acked_mask_ & bit) != 0)
      continue;

    irq_acked_mask_ |= bit;

    sleeping = false;

    const u16 ret_pc = pc;
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
