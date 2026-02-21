#include "cpu.h"

#include <vcml.h>

#include "decode/decode.h"

Cpu::Cpu()
    : vcml::processor("cpu", "avr"),
      x(*reinterpret_cast<u16*>(&rf[26])),
      y(*reinterpret_cast<u16*>(&rf[28])),
      z(*reinterpret_cast<u16*>(&rf[30])) {
  set_little_endian();

  // See: vcml/debugging/gdbarch.*
  for (int i = 0; i < rf.size(); ++i)
    define_cpureg_rw(i, RegName(i), sizeof(RegType));

  //define_cpureg_rw(32, "sreg", sizeof(RegType));
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
  define_cpureg_rw(IoRegisters::TIFRO, "tifro", 1);
  define_cpureg_rw(IoRegisters::TIFR1, "tifr1", 1);
  define_cpureg_rw(IoRegisters::TIFR2, "tifr2", 1);
  define_cpureg_rw(IoRegisters::PCIFR, "pcifr", 1);
  define_cpureg_rw(IoRegisters::EIFR, "eifr", 1);
  define_cpureg_rw(IoRegisters::EIMSK, "eimsk", 1);
  define_cpureg_rw(IoRegisters::GPIOR0, "gpi0r0", 1);
  define_cpureg_rw(IoRegisters::EECR, "eecr", 1);
  define_cpureg_rw(IoRegisters::EEDR, "eedr", 1);
  define_cpureg_rw(IoRegisters::EEARL, "eearl", 1);
  define_cpureg_rw(IoRegisters::EEARH, "eearh", 1);
  define_cpureg_rw(IoRegisters::GTCCR, "gtccr", 1);
  define_cpureg_rw(IoRegisters::TCCR0A, "tccr0a", 1);
  define_cpureg_rw(IoRegisters::TCCR0B, "tccr0b", 1);
  define_cpureg_rw(IoRegisters::TCNT0, "tcnt0", 1);
  define_cpureg_rw(IoRegisters::OCR0A, "ocr0a", 1);
  define_cpureg_rw(IoRegisters::OCR0B, "ocr0b", 1);
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
  define_cpureg_rw(IoRegisters::SREG, "sreg", 1);
  define_cpureg_rw(IoRegisters::WDTCSR, "wdtcsr", 1);
  define_cpureg_rw(IoRegisters::CLKPR, "clkpr", 1);
  define_cpureg_rw(IoRegisters::PRR, "prr", 1);
  define_cpureg_rw(IoRegisters::OSCCAL, "osccal", 1);
  define_cpureg_rw(IoRegisters::PCICR, "pcicr", 1);
  define_cpureg_rw(IoRegisters::EICRA, "eicra", 1);
  define_cpureg_rw(IoRegisters::PCMSK0, "pcmsk0", 1);
  define_cpureg_rw(IoRegisters::PCMSK1, "pcmsk1", 1);
  define_cpureg_rw(IoRegisters::PCMSK2, "pcmsk2", 1);
  define_cpureg_rw(IoRegisters::TIMSK0, "timsk0", 1);
  define_cpureg_rw(IoRegisters::TIMSK1, "timsk1", 1);
  define_cpureg_rw(IoRegisters::TIMSK2, "timsk2", 1);
  define_cpureg_rw(IoRegisters::ADCL, "adcl", 1);
  define_cpureg_rw(IoRegisters::ADCH, "adch", 1);
  define_cpureg_rw(IoRegisters::ADCSRA, "adcsra", 1);
  define_cpureg_rw(IoRegisters::ADCSRB, "adcsrb", 1);
  define_cpureg_rw(IoRegisters::ADMUX, "admux", 1);
  define_cpureg_rw(IoRegisters::DIDR0, "didr0", 1);
  define_cpureg_rw(IoRegisters::DIDR1, "didr1", 1);
  define_cpureg_rw(IoRegisters::TCCR1A, "tccr1a", 1);
  define_cpureg_rw(IoRegisters::TCCR1B, "tccr1b", 1);
  define_cpureg_rw(IoRegisters::TCCR1C, "tccr1c", 1);
  define_cpureg_rw(IoRegisters::TCNT1L, "tcnt1l", 1);
  define_cpureg_rw(IoRegisters::TCNT1H, "tcnt1h", 1);
  define_cpureg_rw(IoRegisters::ICR1L, "icr1l", 1);
  define_cpureg_rw(IoRegisters::ICR1H, "icr1h", 1);
  define_cpureg_rw(IoRegisters::OCR1AL, "ocr1al", 1);
  define_cpureg_rw(IoRegisters::OCR1AH, "ocr1ah", 1);
  define_cpureg_rw(IoRegisters::OCR1BL, "ocr1bl", 1);
  define_cpureg_rw(IoRegisters::OCR1BH, "ocr1bh", 1);
  define_cpureg_rw(IoRegisters::TCCR2A, "tccr2a", 1);
  define_cpureg_rw(IoRegisters::TCCR2B, "tccr2b", 1);
  define_cpureg_rw(IoRegisters::TCNT2, "tcnt2", 1);
  define_cpureg_rw(IoRegisters::OCR2A, "ocr2a", 1);
  define_cpureg_rw(IoRegisters::OCR2B, "ocr2b", 1);
  define_cpureg_rw(IoRegisters::ASSR, "assr", 1);
  define_cpureg_rw(IoRegisters::TWBR, "twbr", 1);
  define_cpureg_rw(IoRegisters::TWSR, "twsr", 1);
  define_cpureg_rw(IoRegisters::TWAR, "twar", 1);
  define_cpureg_rw(IoRegisters::TWDR, "twdr", 1);
  define_cpureg_rw(IoRegisters::TWCR, "twcr", 1);
  define_cpureg_rw(IoRegisters::TWAMR, "twamr", 1);
  define_cpureg_rw(IoRegisters::UCSR0A, "ucsr0a", 1);
  define_cpureg_rw(IoRegisters::UCSR0B, "ucsr0b", 1);
}

std::function<void()> Cpu::FetchInstruction(vcml::u64 adr) {
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
  Insn* inst = Decode::DecodeInsn(data);

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

void Cpu::simulate(size_t cycles) {
  for (int i = 0; i < cycles; ++i) {
    if (std::find(breakpoints_.begin(), breakpoints_.end(), pc) != breakpoints_.end()) {
      notify_breakpoint_hit(pc, local_time_stamp());
      return;
    }

    auto inst_func = FetchInstruction(pc * 2);
    if (inst_func) {
      try {
        inst_func();
        //log_error("instruction at PC=0x%04x", pc * 2);
      } catch (...) {
        log_error("Illegal instruction at PC=0x%04x", pc * 2);
        exit(1);
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
  sreg.i = 1; // Global interrupt enabled
  skip = false;
  break_hit = false;
  sleeping = false;
  cycle_counter = 0;

  for (auto& reg : rf) {
    reg = 0;
  }

  watchdog_timer = 0;
}

u8 Cpu::MemoryRead(u32 adr) {
  if (adr <= 0xff) {
    if (adr <= 0x1f) {
      return rf[adr];
    } else if (adr <= 0x5f) {
      return IoRead(adr - 0x20);
    } else {
      return 0;  // TODO: Case external IO registers.
    }
  }

  u8 dat;
  tlm::tlm_response_status rs = data.read(adr, &dat, sizeof(u8), vcml::SBI_NONE);

  if (rs == tlm::TLM_ADDRESS_ERROR_RESPONSE) {
    log_bus_error(insn, vcml::VCML_ACCESS_READ, rs, adr, sizeof(u8));
    exit(1);  // TODO: What happens in this case?
  } else if (rs != tlm::TLM_OK_RESPONSE) {
    log_bus_error(insn, vcml::VCML_ACCESS_READ, rs, adr, sizeof(u8));
    exit(1);
  }

  return dat;
}

u8 Cpu::IoRead(u32 adr) {
  switch (adr) {
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
    log_warn("reading unimplemented IO register addr=0x%08x", adr);
    auto rs = data.read(adr + kIoOffset, &value, 1, vcml::SBI_NONE);  
    if (rs != tlm::TLM_OK_RESPONSE) {
      log_bus_error(data, vcml::VCML_ACCESS_READ, rs, adr + kIoOffset, 1);
    }
    return value;
  }
  }
}

void Cpu::IoWrite(u32 adr, u8 value) {
  switch (adr) {
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
    break;  // TODO: SREG.
  default: {
        auto rs = data.write(adr + kIoOffset, &value, 1, vcml::SBI_NONE);
        if (rs != tlm::TLM_OK_RESPONSE) {
          log_bus_error(data, vcml::VCML_ACCESS_WRITE, rs, adr + kIoOffset, 1);
        }
        log_warn("writing unimplemented IO register addr=0x%08x", adr);
    }
    break;  // TODO: Case IO registers.
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
  if (adr <= 0xff) {
    if (adr <= 0x1f) {
      rf[adr] = dat;
    } else if (adr <= 0x5f) {
      IoWrite(adr - 0x20, dat);
    } else {
      log_warn("unimplemented memory write addr=0x%08x", adr);
      return;  // TODO: Case external IO registers.
    }
    return;
  }

  void* d = &dat;
  tlm::tlm_response_status rs = data.write(adr, d, sizeof(u8), vcml::SBI_NONE);

  if (rs != tlm::TLM_OK_RESPONSE) {
    log_bus_error(insn, vcml::VCML_ACCESS_WRITE, rs, adr, sizeof(u8));
    exit(1);
  }
  return;
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
  else if (regno == 32)
    *(RegType*)buf = 0;  // TODO: sreg
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
    *(RegType*)buf;  // TODO: sreg
  else if (regno == 33)
    sp = *(AddrType*)buf;
  else if (regno == 34)
    pc = (*(u32*)buf) / 2;

  return true;
}

// Reads always target program memory.
vcml::u64 Cpu::read_pmem_dbg(vcml::u64 addr, void* buffer, vcml::u64 size) {
  try {
    if (vcml::success(insn.read(addr, buffer, size, vcml::SBI_DEBUG)))
      return size;
  } catch (vcml::report& r) {
    log_warn("error reading %llu bytes to memory at address 0x%llx: %s", size, addr, r.message());
  }
  return 0;
}