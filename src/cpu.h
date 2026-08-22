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

  std::array<RegType, 32> rf = {0};  // Register file
  u16 pc = 0;                        // Program counter
  u16 sp = 0x8ff;                    // Stack pointer
  u16& x;                            // X register (R26-R27)
  u16& y;                            // Y register (R28-R29)
  u16& z;                            // Z register (R30-R31)

  u32 watchdog_timer = 0;

  struct Sreg {
    union {
      struct {
        u8 c : 1;  // Carry
        u8 z : 1;  // Zero
        u8 n : 1;  // Negative
        u8 v : 1;  // Two's complement overflow
        u8 s : 1;  // Sign
        u8 h : 1;  // Half-carry
        u8 t : 1;  // Transfer
        u8 i : 1;  // Global interrupt enable
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
  bool break_hit;
  bool sleeping;

  u16 new_pc = 0;
  u16 start_pc = 0;
  u64 cycle_counter = 0;
  u64 retired_instructions = 0;

  std::function<void()> FetchInstruction(vcml::u64 adr, uint* cyc);

  enum IoRegisters {
    PINB = 0x23,
    DDRB = 0x24,
    PORTB = 0x25,
    PINC = 0x26,
    DDRC = 0x27,
    PORTC = 0x28,
    PIND = 0x29,
    DDRD = 0x2a,
    PORTD = 0x2b,
    TIFRO = 0x35,
    TIFR1 = 0x36,
    TIFR2 = 0x37,
    PCIFR = 0x3b,
    EIFR = 0x3c,
    EIMSK = 0x3d,
    GPIOR0 = 0x3e,
    EECR = 0x3f,
    EEDR = 0x40,
    EEARL = 0x41,
    EEARH = 0x42,
    GTCCR = 0x43,
    TCCR0A = 0x44,
    TCCR0B = 0x45,
    TCNT0 = 0x46,
    OCR0A = 0x47,
    OCR0B = 0x48,
    GPIOR1 = 0x5a,
    GPIOR2 = 0x5b,
    SPCR = 0x4c,
    SPSR = 0x4d,
    SPDR = 0x4e,
    ACSR = 0x50,
    SCMR = 0x53,
    MCUSR = 0x54,
    MCUCR = 0x55,
    SPMCSR = 0x57,
    SPL = 0x5d,
    SPH = 0x5e,
    SREG = 0x5f,
    WDTCSR = 0x60,
    CLKPR = 0x61,
    PRR = 0x64,
    OSCCAL = 0x66,
    PCICR = 0x68,
    EICRA = 0x69,
    PCMSK0 = 0x6b,
    PCMSK1 = 0x6c,
    PCMSK2 = 0x6d,
    TIMSK0 = 0x6e,
    TIMSK1 = 0x6f,
    TIMSK2 = 0x70,
    DIDR0 = 0x7e,
    DIDR1 = 0x7f,
    TCCR1A = 0x80,
    TCCR1B = 0x81,
    TCCR1C = 0x82,
    TCNT1L = 0x84,
    TCNT1H = 0x85,
    ICR1L = 0x86,
    ICR1H = 0x87,
    OCR1AL = 0x88,
    OCR1AH = 0x89,
    OCR1BL = 0x8a,
    OCR1BH = 0x8b,
    TCCR2A = 0xb0,
    TCCR2B = 0xb1,
    TCNT2 = 0xb2,
    OCR2A = 0xb3,
    OCR2B = 0xb4,
    ASSR = 0xb6,
    TWBR = 0xb8,
    TWSR = 0xb9,
    TWAR = 0xba,
    TWDR = 0xbb,
    TWCR = 0xbc,
    TWAMR = 0xbd,
    UCSR0A = 0xc0,
    UCSR0B = 0xc1,
    UCSR0C = 0xc2,
    UBRR0L = 0xc4,
    UBRR0H = 0xc5,
    UDR0 = 0xc6
  };

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

  // AVR semantics: after SEI, interrupts are enabled after the following
  // instruction. Instruction implementations may set this when they execute SEI.
  bool irq_sei_delay = false;

 protected:
  static u32 constexpr kIoOffset = 0x20;
  virtual vcml::u64 read_pmem_dbg(vcml::u64 addr, void* ptr, vcml::u64 sz) override;
  virtual bool read_reg_dbg(size_t idx, void* buf, size_t len) override;
  virtual bool write_reg_dbg(size_t idx, const void*, size_t l) override;
  virtual void interrupt(size_t irq, bool set) override;
  void start_of_simulation() override;
  void end_of_simulation() override;

  bool service_pending_interrupt();

  // IRQ handling state (simple level-triggered model).
  u64 irq_level_mask_ = 0; // bit i: irq[i] currently asserted
  u64 irq_acked_mask_ = 0; // bit i: irq[i] already accepted, wait for deassert

  std::vector<u16> breakpoints_;
  std::vector<u16> watchpoints_;

  std::chrono::steady_clock::time_point sim_start_time_;
};
