#include "timer.h"

#include <algorithm>

namespace {

// GTCCR bits
static constexpr u8 GTCCR_PSRASY = 1u << 1;
static constexpr u8 GTCCR_PSR10 = 1u << 0;  // PSRSYNC on ATmega328P
static constexpr u8 GTCCR_TSM = 1u << 7;

// TIFR0 bits
static constexpr u8 TIFR0_TOV0 = 1u << 0;
static constexpr u8 TIFR0_OCF0A = 1u << 1;
static constexpr u8 TIFR0_OCF0B = 1u << 2;

// TIFR1 bits
static constexpr u8 TIFR1_TOV1 = 1u << 0;
static constexpr u8 TIFR1_OCF1A = 1u << 1;
static constexpr u8 TIFR1_OCF1B = 1u << 2;
static constexpr u8 TIFR1_ICF1 = 1u << 5;

// TIFR2 bits
static constexpr u8 TIFR2_TOV2 = 1u << 0;
static constexpr u8 TIFR2_OCF2A = 1u << 1;
static constexpr u8 TIFR2_OCF2B = 1u << 2;

static inline u8 clamp_u8(int v) {
  return (u8)std::clamp(v, 0, 255);
}

}  // namespace

Timer::Timer(const sc_core::sc_module_name& nm)
    : peripheral(nm),
      in("in"),
      gtccr("gtccr", 0x43, 0x00),
      tifr0("tifr0", 0x35, 0x00),
      tccr0a("tccr0a", 0x44, 0x00),
      tccr0b("tccr0b", 0x45, 0x00),
      tcnt0("tcnt0", 0x46, 0x00),
      ocr0a("ocr0a", 0x47, 0x00),
      ocr0b("ocr0b", 0x48, 0x00),
      timsk0("timsk0", 0x6e, 0x00),
      tifr1("tifr1", 0x36, 0x00),
      tccr1a("tccr1a", 0x80, 0x00),
      tccr1b("tccr1b", 0x81, 0x00),
      tccr1c("tccr1c", 0x82, 0x00),
      tcnt1l("tcnt1l", 0x84, 0x00),
      tcnt1h("tcnt1h", 0x85, 0x00),
      icr1l("icr1l", 0x86, 0x00),
      icr1h("icr1h", 0x87, 0x00),
      ocr1al("ocr1al", 0x88, 0x00),
      ocr1ah("ocr1ah", 0x89, 0x00),
      ocr1bl("ocr1bl", 0x8a, 0x00),
      ocr1bh("ocr1bh", 0x8b, 0x00),
      timsk1("timsk1", 0x6f, 0x00),
      tifr2("tifr2", 0x37, 0x00),
      tccr2a("tccr2a", 0xb0, 0x00),
      tccr2b("tccr2b", 0xb1, 0x00),
      tcnt2("tcnt2", 0xb2, 0x00),
      ocr2a("ocr2a", 0xb3, 0x00),
      ocr2b("ocr2b", 0xb4, 0x00),
      assr("assr", 0xb6, 0x00),
      timsk2("timsk2", 0x70, 0x00),
      tcnt1_(0),
      ocr1a_(0),
      ocr1b_(0),
      icr1_(0),
      tcnt1_latch_h_(0),
      ocr1a_latch_h_(0),
      ocr1b_latch_h_(0),
      icr1_latch_h_(0),
      tcnt1_write_h_(0),
      ocr1a_write_h_(0),
      ocr1b_write_h_(0),
      icr1_write_h_(0),
      ps0_(0),
      ps1_(0),
      ps2_(0),
      dir0_(true),
      dir1_(true),
      dir2_(true) {
  set_little_endian();

  gtccr.sync_always();
  gtccr.allow_read_write();
  gtccr.on_write(&Timer::write_gtccr, this);

  // Flag registers: cleared by writing 1s.
  tifr0.sync_always();
  tifr0.allow_read_write();
  tifr0.on_write(&Timer::write_tifr0, this);

  tifr1.sync_always();
  tifr1.allow_read_write();
  tifr1.on_write(&Timer::write_tifr1, this);

  tifr2.sync_always();
  tifr2.allow_read_write();
  tifr2.on_write(&Timer::write_tifr2, this);

  tccr0a.sync_always();
  tccr0a.allow_read_write();
  tccr0b.sync_always();
  tccr0b.allow_read_write();
  tcnt0.sync_always();
  tcnt0.allow_read_write();
  ocr0a.sync_always();
  ocr0a.allow_read_write();
  ocr0b.sync_always();
  ocr0b.allow_read_write();
  timsk0.sync_always();
  timsk0.allow_read_write();

  tccr1a.sync_always();
  tccr1a.allow_read_write();
  tccr1b.sync_always();
  tccr1b.allow_read_write();
  tccr1c.sync_always();
  tccr1c.allow_read_write();

  // 16-bit access semantics
  tcnt1l.sync_always();
  tcnt1l.allow_read_write();
  tcnt1l.on_read(&Timer::read_tcnt1l, this);
  tcnt1l.on_write(&Timer::write_tcnt1l, this);

  tcnt1h.sync_always();
  tcnt1h.allow_read_write();
  tcnt1h.on_read(&Timer::read_tcnt1h, this);
  tcnt1h.on_write(&Timer::write_tcnt1h, this);

  icr1l.sync_always();
  icr1l.allow_read_write();
  icr1l.on_read(&Timer::read_icr1l, this);
  icr1l.on_write(&Timer::write_icr1l, this);

  icr1h.sync_always();
  icr1h.allow_read_write();
  icr1h.on_read(&Timer::read_icr1h, this);
  icr1h.on_write(&Timer::write_icr1h, this);

  ocr1al.sync_always();
  ocr1al.allow_read_write();
  ocr1al.on_read(&Timer::read_ocr1al, this);
  ocr1al.on_write(&Timer::write_ocr1al, this);

  ocr1ah.sync_always();
  ocr1ah.allow_read_write();
  ocr1ah.on_read(&Timer::read_ocr1ah, this);
  ocr1ah.on_write(&Timer::write_ocr1ah, this);

  ocr1bl.sync_always();
  ocr1bl.allow_read_write();
  ocr1bl.on_read(&Timer::read_ocr1bl, this);
  ocr1bl.on_write(&Timer::write_ocr1bl, this);

  ocr1bh.sync_always();
  ocr1bh.allow_read_write();
  ocr1bh.on_read(&Timer::read_ocr1bh, this);
  ocr1bh.on_write(&Timer::write_ocr1bh, this);

  timsk1.sync_always();
  timsk1.allow_read_write();

  tccr2a.sync_always();
  tccr2a.allow_read_write();
  tccr2b.sync_always();
  tccr2b.allow_read_write();
  tcnt2.sync_always();
  tcnt2.allow_read_write();
  ocr2a.sync_always();
  ocr2a.allow_read_write();
  ocr2b.sync_always();
  ocr2b.allow_read_write();
  assr.sync_always();
  assr.allow_read_write();
  timsk2.sync_always();
  timsk2.allow_read_write();

  SC_THREAD(run);
}

void Timer::reset() {
  peripheral::reset();

  // Registers reset to 0 by default in this model.
  gtccr = 0;

  tifr0 = 0;
  tccr0a = 0;
  tccr0b = 0;
  tcnt0 = 0;
  ocr0a = 0;
  ocr0b = 0;
  timsk0 = 0;

  tifr1 = 0;
  tccr1a = 0;
  tccr1b = 0;
  tccr1c = 0;
  timsk1 = 0;

  tifr2 = 0;
  tccr2a = 0;
  tccr2b = 0;
  tcnt2 = 0;
  ocr2a = 0;
  ocr2b = 0;
  assr = 0;
  timsk2 = 0;

  tcnt1_ = 0;
  ocr1a_ = 0;
  ocr1b_ = 0;
  icr1_ = 0;
  tcnt1_latch_h_ = 0;
  ocr1a_latch_h_ = 0;
  ocr1b_latch_h_ = 0;
  icr1_latch_h_ = 0;
  tcnt1_write_h_ = 0;
  ocr1a_write_h_ = 0;
  ocr1b_write_h_ = 0;
  icr1_write_h_ = 0;

  ps0_ = ps1_ = ps2_ = 0;
  dir0_ = dir1_ = dir2_ = true;

  sync_16bit_regs_from_state();
}

void Timer::sync_16bit_regs_from_state() {
  tcnt1l = (u8)(tcnt1_ & 0xff);
  tcnt1h = (u8)(tcnt1_ >> 8);
  ocr1al = (u8)(ocr1a_ & 0xff);
  ocr1ah = (u8)(ocr1a_ >> 8);
  ocr1bl = (u8)(ocr1b_ & 0xff);
  ocr1bh = (u8)(ocr1b_ >> 8);
  icr1l = (u8)(icr1_ & 0xff);
  icr1h = (u8)(icr1_ >> 8);
}

bool Timer::timers_stopped() const {
  return (gtccr & GTCCR_TSM) != 0;
}

void Timer::write_gtccr(u8 val, bool dbg) {
  // Implement prescaler reset bits and TSM.
  // Writing a 1 to PSRASY resets Timer2 prescaler.
  // Writing a 1 to PSRSYNC resets Timer0/1 prescaler.
  const bool psrasy = (val & GTCCR_PSRASY) != 0;
  const bool psrsync = (val & GTCCR_PSR10) != 0;

  // TSM is a stored bit.
  gtccr = (gtccr & ~GTCCR_TSM) | (val & GTCCR_TSM);

  if (psrasy) {
    ps2_ = 0;
    // PSRASY is not stored.
  }
  if (psrsync) {
    ps0_ = 0;
    ps1_ = 0;
    // PSRSYNC is not stored.
  }

  (void)dbg;
}

void Timer::write_tifr0(u8 val, bool dbg) {
  // Clear flags by writing 1.
  tifr0 = (u8)(tifr0 & ~val);
  (void)dbg;
}

void Timer::write_tifr1(u8 val, bool dbg) {
  tifr1 = (u8)(tifr1 & ~val);
  (void)dbg;
}

void Timer::write_tifr2(u8 val, bool dbg) {
  tifr2 = (u8)(tifr2 & ~val);
  (void)dbg;
}

u8 Timer::read_tcnt1l(bool dbg) {
  (void)dbg;
  // Reading low latches high.
  tcnt1_latch_h_ = (u8)(tcnt1_ >> 8);
  return (u8)(tcnt1_ & 0xff);
}

u8 Timer::read_tcnt1h(bool dbg) {
  (void)dbg;
  return tcnt1_latch_h_;
}

void Timer::write_tcnt1h(u8 val, bool dbg) {
  (void)dbg;
  tcnt1_write_h_ = val;
  // Do not commit until low byte is written.
  tcnt1h = val;
}

void Timer::write_tcnt1l(u8 val, bool dbg) {
  (void)dbg;
  tcnt1_ = (u16)((u16)tcnt1_write_h_ << 8) | val;
  sync_16bit_regs_from_state();
}

u8 Timer::read_ocr1al(bool dbg) {
  (void)dbg;
  ocr1a_latch_h_ = (u8)(ocr1a_ >> 8);
  return (u8)(ocr1a_ & 0xff);
}

u8 Timer::read_ocr1ah(bool dbg) {
  (void)dbg;
  return ocr1a_latch_h_;
}

void Timer::write_ocr1ah(u8 val, bool dbg) {
  (void)dbg;
  ocr1a_write_h_ = val;
  ocr1ah = val;
}

void Timer::write_ocr1al(u8 val, bool dbg) {
  (void)dbg;
  ocr1a_ = (u16)((u16)ocr1a_write_h_ << 8) | val;
  sync_16bit_regs_from_state();
}

u8 Timer::read_ocr1bl(bool dbg) {
  (void)dbg;
  ocr1b_latch_h_ = (u8)(ocr1b_ >> 8);
  return (u8)(ocr1b_ & 0xff);
}

u8 Timer::read_ocr1bh(bool dbg) {
  (void)dbg;
  return ocr1b_latch_h_;
}

void Timer::write_ocr1bh(u8 val, bool dbg) {
  (void)dbg;
  ocr1b_write_h_ = val;
  ocr1bh = val;
}

void Timer::write_ocr1bl(u8 val, bool dbg) {
  (void)dbg;
  ocr1b_ = (u16)((u16)ocr1b_write_h_ << 8) | val;
  sync_16bit_regs_from_state();
}

u8 Timer::read_icr1l(bool dbg) {
  (void)dbg;
  icr1_latch_h_ = (u8)(icr1_ >> 8);
  return (u8)(icr1_ & 0xff);
}

u8 Timer::read_icr1h(bool dbg) {
  (void)dbg;
  return icr1_latch_h_;
}

void Timer::write_icr1h(u8 val, bool dbg) {
  (void)dbg;
  icr1_write_h_ = val;
  icr1h = val;
}

void Timer::write_icr1l(u8 val, bool dbg) {
  (void)dbg;
  icr1_ = (u16)((u16)icr1_write_h_ << 8) | val;
  sync_16bit_regs_from_state();
}

void Timer::set_tifr0(u8 mask) {
  tifr0 = (u8)(tifr0 | mask);
}

void Timer::set_tifr1(u8 mask) {
  tifr1 = (u8)(tifr1 | mask);
}

void Timer::set_tifr2(u8 mask) {
  tifr2 = (u8)(tifr2 | mask);
}

u32 Timer::timer0_div() const {
  const u8 cs = (u8)(tccr0b & 0x07);
  switch (cs) {
  case 0:
    return 0;  // stopped
  case 1:
    return 1;
  case 2:
    return 8;
  case 3:
    return 64;
  case 4:
    return 256;
  case 5:
    return 1024;
  case 6:
  case 7:
  default:
    return 0;  // external clock not modeled
  }
}

u32 Timer::timer1_div() const {
  const u8 cs = (u8)(tccr1b & 0x07);
  switch (cs) {
  case 0:
    return 0;
  case 1:
    return 1;
  case 2:
    return 8;
  case 3:
    return 64;
  case 4:
    return 256;
  case 5:
    return 1024;
  case 6:
  case 7:
  default:
    return 0;
  }
}

u32 Timer::timer2_div() const {
  const u8 cs = (u8)(tccr2b & 0x07);
  switch (cs) {
  case 0:
    return 0;
  case 1:
    return 1;
  case 2:
    return 8;
  case 3:
    return 32;
  case 4:
    return 64;
  case 5:
    return 128;
  case 6:
    return 256;
  case 7:
    return 1024;
  default:
    return 0;
  }
}

void Timer::tick_timer0() {
  log_debug("timer 0 tick");

  const u8 wgm = (u8)(((tccr0b >> 3) & 0x1) << 2 | (tccr0a & 0x3));
  const u8 top = (wgm == 7 || wgm == 5) ? (u8)ocr0a : 0xff;
  const bool phase_correct = (wgm == 1 || wgm == 5);
  const bool ctc = (wgm == 2);
  const bool fast_pwm = (wgm == 3 || wgm == 7);

  u8 old = (u8)tcnt0;
  u8 next = old;

  if (phase_correct) {
    if (dir0_) {
      if (next >= top) {
        dir0_ = false;
        next = (top > 0) ? (u8)(top - 1) : 0;
      } else {
        next++;
      }
    } else {
      if (next == 0) {
        dir0_ = true;
        // In phase correct PWM, overflow flag set at BOTTOM.
        set_tifr0(TIFR0_TOV0);
        next = 1;
      } else {
        next--;
      }
    }
  } else {
    // normal / CTC / fast PWM style single-slope
    if (old == top) {
      next = 0;
      // Overflow flag in normal/fast pwm: set on wrap.
      set_tifr0(TIFR0_TOV0);
    } else {
      next++;
    }
  }

  tcnt0 = next;

  // Compare matches (best-effort, not modelling output-pin behavior)
  if ((u8)tcnt0 == (u8)ocr0a)
    set_tifr0(TIFR0_OCF0A);
  if ((u8)tcnt0 == (u8)ocr0b)
    set_tifr0(TIFR0_OCF0B);

  if (ctc && (u8)tcnt0 == (u8)ocr0a) {
    // Clear on compare match.
    tcnt0 = 0;
  }

  (void)fast_pwm;
}

void Timer::tick_timer2() {
  log_debug("timer 2 tick");
  const u8 wgm = (u8)(((tccr2b >> 3) & 0x1) << 2 | (tccr2a & 0x3));
  const u8 top = (wgm == 7 || wgm == 5) ? (u8)ocr2a : 0xff;
  const bool phase_correct = (wgm == 1 || wgm == 5);
  const bool ctc = (wgm == 2);

  u8 old = (u8)tcnt2;
  u8 next = old;

  if (phase_correct) {
    if (dir2_) {
      if (next >= top) {
        dir2_ = false;
        next = (top > 0) ? (u8)(top - 1) : 0;
      } else {
        next++;
      }
    } else {
      if (next == 0) {
        dir2_ = true;
        set_tifr2(TIFR2_TOV2);
        next = 1;
      } else {
        next--;
      }
    }
  } else {
    if (old == top) {
      next = 0;
      set_tifr2(TIFR2_TOV2);
    } else {
      next++;
    }
  }

  tcnt2 = next;
  if ((u8)tcnt2 == (u8)ocr2a)
    set_tifr2(TIFR2_OCF2A);
  if ((u8)tcnt2 == (u8)ocr2b)
    set_tifr2(TIFR2_OCF2B);

  if (ctc && (u8)tcnt2 == (u8)ocr2a) {
    tcnt2 = 0;
  }
}

void Timer::tick_timer1() {
  log_debug("timer 1 tick");
  const u8 wgm = (u8)(((tccr1b >> 3) & 0x3) << 2 | (tccr1a & 0x3));

  auto top_for_mode = [&]() -> u16 {
    switch (wgm) {
    case 0:  // normal
      return 0xffff;
    case 4:  // CTC, TOP=OCR1A
      return std::max<u16>(1, ocr1a_);
    case 12:  // CTC, TOP=ICR1
      return std::max<u16>(1, icr1_);
    case 14:  // fast PWM, TOP=ICR1
      return std::max<u16>(1, icr1_);
    case 15:  // fast PWM, TOP=OCR1A
      return std::max<u16>(1, ocr1a_);
    default:
      return 0xffff;
    }
  };

  const u16 top = top_for_mode();
  const bool ctc = (wgm == 4 || wgm == 12);
  const bool fast_pwm = (wgm == 14 || wgm == 15);

  const u16 old = tcnt1_;
  u16 next = old;

  // For now: only single-slope operation for implemented modes.
  if (old >= top) {
    next = 0;
    set_tifr1(TIFR1_TOV1);
  } else {
    next = (u16)(old + 1);
  }

  tcnt1_= next;
  sync_16bit_regs_from_state();

  if (tcnt1_ == ocr1a_)
    set_tifr1(TIFR1_OCF1A);
  if (tcnt1_ == ocr1b_)
    set_tifr1(TIFR1_OCF1B);

  if (ctc && tcnt1_ == top) {
    // Clear on compare match for CTC.
    tcnt1_ = 0;
    sync_16bit_regs_from_state();
  }

  (void)fast_pwm;
}

void Timer::run() {
  // Ensure regs are consistent.
  sync_16bit_regs_from_state();

  while (true) {
    wait_clock_cycle();

    if (timers_stopped())
      continue;

    // Timer0
    const u32 div0 = timer0_div();
    if (div0 != 0) {
      ps0_++;
      if (ps0_ >= div0) {
        ps0_ = 0;
        tick_timer0();
      }
    }

    // Timer1
    const u32 div1 = timer1_div();
    if (div1 != 0) {
      ps1_++;
      if (ps1_ >= div1) {
        ps1_ = 0;
        tick_timer1();
      }
    }

    // Timer2
    const u32 div2 = timer2_div();
    if (div2 != 0) {
      ps2_++;
      if (ps2_ >= div2) {
        ps2_ = 0;
        tick_timer2();
      }
    }
  }
}
