#include "timer.h"

#include <algorithm>

static constexpr u8 GTCCR_PSRASY = mwr::bit(1);
static constexpr u8 GTCCR_PSR10 = mwr::bit(0);
static constexpr u8 GTCCR_TSM = mwr::bit(7);

static constexpr u8 TIFR0_TOV0 = mwr::bit(0);
static constexpr u8 TIFR0_OCF0A = mwr::bit(1);
static constexpr u8 TIFR0_OCF0B = mwr::bit(2);

static constexpr u8 TIFR1_TOV1 = mwr::bit(0);
static constexpr u8 TIFR1_OCF1A = mwr::bit(1);
static constexpr u8 TIFR1_OCF1B = mwr::bit(2);
static constexpr u8 TIFR1_ICF1 = mwr::bit(5);

static constexpr u8 TIFR2_TOV2 = mwr::bit(0);
static constexpr u8 TIFR2_OCF2A = mwr::bit(1);
static constexpr u8 TIFR2_OCF2B = mwr::bit(2);

static inline u8 clamp_u8(int v) {
  return (u8)std::clamp(v, 0, 255);
}

Timer::Timer(const sc_core::sc_module_name& nm)
    : peripheral(nm),
      in("in"),
      ocie0a_out("ocie0a_out"),
      ocie0b_out("ocie0b_out"),
      toie0_out("toie0_out"),
      ocie1a_out("ocie1a_out"),
      ocie1b_out("ocie1b_out"),
      toie1_out("toie1_out"),
      icie1_out("icie1_out"),
      ocie2a_out("ocie2a_out"),
      ocie2b_out("ocie2b_out"),
      toie2_out("toie2_out"),
      ocie0a_ack("ocie0a_ack"),
      ocie0b_ack("ocie0b_ack"),
      toie0_ack("toie0_ack"),
      ocie1a_ack("ocie1a_ack"),
      ocie1b_ack("ocie1b_ack"),
      toie1_ack("toie1_ack"),
      icie1_ack("icie1_ack"),
      ocie2a_ack("ocie2a_ack"),
      ocie2b_ack("ocie2b_ack"),
      toie2_ack("toie2_ack"),
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
  gtccr.on_write(&Timer::write_gtccr);

  tifr0.sync_always();
  tifr0.allow_read_write();
  tifr0.on_write(&Timer::write_tifr0);

  tifr1.sync_always();
  tifr1.allow_read_write();
  tifr1.on_write(&Timer::write_tifr1);

  tifr2.sync_always();
  tifr2.allow_read_write();
  tifr2.on_write(&Timer::write_tifr2);

  tccr0a.sync_always();
  tccr0a.allow_read_write();
  tccr0a.on_write_mask(0xf3);

  tccr0b.sync_always();
  tccr0b.allow_read_write();
  tccr0b.on_write(&Timer::write_tccr0b); // TODO: FOC0A and FOC0B.

  tcnt0.sync_always();
  tcnt0.allow_read_write();

  ocr0a.sync_always();
  ocr0a.allow_read_write();

  ocr0b.sync_always();
  ocr0b.allow_read_write();

  timsk0.sync_always();
  timsk0.allow_read_write();
  timsk0.on_write(&Timer::write_timsk0);

  tccr1a.sync_always();
  tccr1a.allow_read_write();
  tccr1a.on_write_mask(0xf3);

  tccr1b.sync_always();
  tccr1b.allow_read_write();
  tccr1b.on_write(&Timer::write_tccr1b);

  tccr1c.sync_always();
  tccr1c.allow_read_write();
  tccr1c.on_write_mask(0x00); // TODO: FOC1A and FOC1B.

  tcnt1l.sync_always();
  tcnt1l.allow_read_write();
  tcnt1l.on_read(&Timer::read_tcnt1l);
  tcnt1l.on_write(&Timer::write_tcnt1l);

  tcnt1h.sync_always();
  tcnt1h.allow_read_write();
  tcnt1h.on_read(&Timer::read_tcnt1h);
  tcnt1h.on_write(&Timer::write_tcnt1h);

  icr1l.sync_always();
  icr1l.allow_read_write();
  icr1l.on_read(&Timer::read_icr1l);
  icr1l.on_write(&Timer::write_icr1l);

  icr1h.sync_always();
  icr1h.allow_read_write();
  icr1h.on_read(&Timer::read_icr1h);
  icr1h.on_write(&Timer::write_icr1h);

  ocr1al.sync_always();
  ocr1al.allow_read_write();
  ocr1al.on_read(&Timer::read_ocr1al);
  ocr1al.on_write(&Timer::write_ocr1al);

  ocr1ah.sync_always();
  ocr1ah.allow_read_write();
  ocr1ah.on_read(&Timer::read_ocr1ah);
  ocr1ah.on_write(&Timer::write_ocr1ah);

  ocr1bl.sync_always();
  ocr1bl.allow_read_write();
  ocr1bl.on_read(&Timer::read_ocr1bl);
  ocr1bl.on_write(&Timer::write_ocr1bl);

  ocr1bh.sync_always();
  ocr1bh.allow_read_write();
  ocr1bh.on_read(&Timer::read_ocr1bh);
  ocr1bh.on_write(&Timer::write_ocr1bh);

  timsk1.sync_always();
  timsk1.allow_read_write();
  timsk1.on_write(&Timer::write_timsk1);

  tccr2a.sync_always();
  tccr2a.allow_read_write();
  tccr2b.sync_always();
  tccr2b.allow_read_write();
  tccr2b.on_write(&Timer::write_tccr2b);
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
  timsk2.on_write(&Timer::write_timsk2);

  SC_THREAD(run_timer0);
  SC_THREAD(run_timer1);
  SC_THREAD(run_timer2);
}

void Timer::reset() {
  peripheral::reset();

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

  const sc_core::sc_time now = sc_core::sc_time_stamp();
  wait_start0_ = wait_start1_ = wait_start2_ = now;

  sync_16bit_regs_from_state();

  resume_ev_.notify(sc_core::SC_ZERO_TIME);
  div0_ev_.notify(sc_core::SC_ZERO_TIME);
  div1_ev_.notify(sc_core::SC_ZERO_TIME);
  div2_ev_.notify(sc_core::SC_ZERO_TIME);
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

void Timer::write_gtccr(u8 val) {
  const bool psrasy = (val & GTCCR_PSRASY) != 0;
  const bool psrsync = (val & GTCCR_PSR10) != 0;

  gtccr = (gtccr & ~GTCCR_TSM) | (val & GTCCR_TSM);

  const sc_core::sc_time now = sc_core::sc_time_stamp();

  if (psrasy) {
    ps2_ = 0;
    wait_start2_ = now;
  }
  if (psrsync) {
    ps0_ = 0;
    ps1_ = 0;
    wait_start0_ = now;
    wait_start1_ = now;
  }

  resume_ev_.notify(sc_core::SC_ZERO_TIME);
}

void Timer::write_tccr0b(u8 val) {
  tccr0b = val & 0xf; // TODO: FOC0A and FOC0B.
  div0_ev_.notify(sc_core::SC_ZERO_TIME);
}

void Timer::write_tccr1b(u8 val) {
  tccr1b = val & 0xdf;
  div1_ev_.notify(sc_core::SC_ZERO_TIME);
}

void Timer::write_tccr2b(u8 val) {
  tccr2b = val;
  div2_ev_.notify(sc_core::SC_ZERO_TIME);
}

void Timer::write_tifr0(u8 val) {
  tifr0 = (u8)(tifr0 & ~val);
  check_interrupts();
}

void Timer::write_tifr1(u8 val) {
  tifr1 = (u8)(tifr1 & ~val);
  check_interrupts();
}

void Timer::write_tifr2(u8 val) {
  tifr2 = (u8)(tifr2 & ~val);
  check_interrupts();
}

void Timer::write_timsk0(u8 val) {
  timsk0 = val & 0b111;
  check_interrupts();
}

void Timer::write_timsk1(u8 val) {
  timsk1 = val & 0b100111;
  check_interrupts();
}

void Timer::write_timsk2(u8 val) {
  timsk2 = val;
  check_interrupts();
}

u8 Timer::read_tcnt1l() {
  tcnt1_latch_h_ = (u8)(tcnt1_ >> 8);
  return (u8)(tcnt1_ & 0xff);
}

u8 Timer::read_tcnt1h() {
  return tcnt1_latch_h_;
}

void Timer::write_tcnt1h(u8 val) {
  tcnt1_write_h_ = val;
  tcnt1h = val;
}

void Timer::write_tcnt1l(u8 val) {
  tcnt1_ = (u16)((u16)tcnt1_write_h_ << 8) | val;
  sync_16bit_regs_from_state();
}

u8 Timer::read_ocr1al() {
  ocr1a_latch_h_ = (u8)(ocr1a_ >> 8);
  return (u8)(ocr1a_ & 0xff);
}

u8 Timer::read_ocr1ah() {
  return ocr1a_latch_h_;
}

void Timer::write_ocr1ah(u8 val) {
  ocr1a_write_h_ = val;
  ocr1ah = val;
}

void Timer::write_ocr1al(u8 val) {
  ocr1a_ = (u16)((u16)ocr1a_write_h_ << 8) | val;
  sync_16bit_regs_from_state();
}

u8 Timer::read_ocr1bl() {
  ocr1b_latch_h_ = (u8)(ocr1b_ >> 8);
  return (u8)(ocr1b_ & 0xff);
}

u8 Timer::read_ocr1bh() {
  return ocr1b_latch_h_;
}

void Timer::write_ocr1bh(u8 val) {
  ocr1b_write_h_ = val;
  ocr1bh = val;
}

void Timer::write_ocr1bl(u8 val) {
  ocr1b_ = (u16)((u16)ocr1b_write_h_ << 8) | val;
  sync_16bit_regs_from_state();
}

u8 Timer::read_icr1l() {
  icr1_latch_h_ = (u8)(icr1_ >> 8);
  return (u8)(icr1_ & 0xff);
}

u8 Timer::read_icr1h() {
  return icr1_latch_h_;
}

void Timer::write_icr1h(u8 val) {
  icr1_write_h_ = val;
  icr1h = val;
}

void Timer::write_icr1l(u8 val) {
  icr1_ = (u16)((u16)icr1_write_h_ << 8) | val;
  sync_16bit_regs_from_state();
}

void Timer::set_tifr0(u8 mask) {
  tifr0 = (u8)(tifr0 | mask);
  check_interrupts();
}

void Timer::set_tifr1(u8 mask) {
  tifr1 = (u8)(tifr1 | mask);
  check_interrupts();
}

void Timer::set_tifr2(u8 mask) {
  tifr2 = (u8)(tifr2 | mask);
  check_interrupts();
}

u32 Timer::timer0_div() const {
  u8 cs = tccr0b & 0b111;
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

  u8 wgm = (u8)(((tccr0b >> 3) & 0x1) << 2 | (tccr0a & 0x3));
  u8 top = (wgm == 7 || wgm == 5) ? (u8)ocr0a : 0xff;
  bool phase_correct = (wgm == 1 || wgm == 5);
  bool ctc = (wgm == 2);
  bool fast_pwm = (wgm == 3 || wgm == 7);

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
        set_tifr0(TIFR0_TOV0);
        next = 1;
      } else {
        next--;
      }
    }
  } else {
    if (old == top) {
      next = 0;
      set_tifr0(TIFR0_TOV0);
    } else {
      next++;
    }
  }

  tcnt0 = next;

  if ((u8)tcnt0 == (u8)ocr0a)
    set_tifr0(TIFR0_OCF0A);
  if ((u8)tcnt0 == (u8)ocr0b)
    set_tifr0(TIFR0_OCF0B);

  if (ctc && (u8)tcnt0 == (u8)ocr0a) {
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

  tcnt1_ = next;
  sync_16bit_regs_from_state();

  if (tcnt1_ == ocr1a_)
    set_tifr1(TIFR1_OCF1A);
  if (tcnt1_ == ocr1b_)
    set_tifr1(TIFR1_OCF1B);

  if (ctc && tcnt1_ == top) {
    tcnt1_ = 0;
    sync_16bit_regs_from_state();
  }

  (void)fast_pwm;
}

void Timer::check_interrupts() {
  static constexpr u8 TIMSK0_TOIE0 = mwr::bit(0);
  static constexpr u8 TIMSK0_OCIE0A = mwr::bit(1);
  static constexpr u8 TIMSK0_OCIE0B = mwr::bit(2);

  static constexpr u8 TIMSK1_TOIE1 = mwr::bit(0);
  static constexpr u8 TIMSK1_OCIE1A = mwr::bit(1);
  static constexpr u8 TIMSK1_OCIE1B = mwr::bit(2);
  static constexpr u8 TIMSK1_ICIE1 = mwr::bit(5);

  static constexpr u8 TIMSK2_TOIE2 = mwr::bit(0);
  static constexpr u8 TIMSK2_OCIE2A = mwr::bit(1);
  static constexpr u8 TIMSK2_OCIE2B = mwr::bit(2);

  const u8 t0_flags = (u8)tifr0;
  const u8 t0_mask = (u8)timsk0;
  const u8 t1_flags = (u8)tifr1;
  const u8 t1_mask = (u8)timsk1;
  const u8 t2_flags = (u8)tifr2;
  const u8 t2_mask = (u8)timsk2;

  if ((t0_flags & TIFR0_TOV0) && (t0_mask & TIMSK0_TOIE0)) {
    toie0_out = true;
    log_debug("Timer0 overflow interrupt");
  } else {
    toie0_out = false;
  }

  if ((t0_flags & TIFR0_OCF0A) && (t0_mask & TIMSK0_OCIE0A)) {
    ocie0a_out = true;
    log_debug("Timer0 compare A interrupt");
  } else {
    ocie0a_out = false;
  }

  if ((t0_flags & TIFR0_OCF0B) && (t0_mask & TIMSK0_OCIE0B)) {
    ocie0b_out = true;
    log_debug("Timer0 compare B interrupt");
  } else {
    ocie0b_out = false;
  }

  if ((t1_flags & TIFR1_TOV1) && (t1_mask & TIMSK1_TOIE1)) {
    toie1_out = true;
    log_debug("Timer1 overflow interrupt");
  } else {
    toie1_out = false;
  }

  if ((t1_flags & TIFR1_OCF1A) && (t1_mask & TIMSK1_OCIE1A)) {
    ocie1a_out = true;
    log_debug("Timer1 compare A interrupt");
  } else {
    ocie1a_out = false;
  }

  if ((t1_flags & TIFR1_OCF1B) && (t1_mask & TIMSK1_OCIE1B)) {
    ocie1b_out = true;
    log_debug("Timer1 compare B interrupt");
  } else {
    ocie1b_out = false;
  }

  if ((t1_flags & TIFR1_ICF1) && (t1_mask & TIMSK1_ICIE1)) {
    icie1_out = true;
    log_debug("Timer1 input capture interrupt");
  } else {
    icie1_out = false;
  }

  if ((t2_flags & TIFR2_TOV2) && (t2_mask & TIMSK2_TOIE2)) {
    toie2_out = true;
    log_debug("Timer2 overflow interrupt");
  } else {
    toie2_out = false;
  }

  if ((t2_flags & TIFR2_OCF2A) && (t2_mask & TIMSK2_OCIE2A)) {
    ocie2a_out = true;
    log_debug("Timer2 compare A interrupt");
  } else {
    ocie2a_out = false;
  }

  if ((t2_flags & TIFR2_OCF2B) && (t2_mask & TIMSK2_OCIE2B)) {
    ocie2b_out = true;
    log_debug("Timer2 compare B interrupt");
  } else {
    ocie2b_out = false;
  }
}

void Timer::gpio_notify(const vcml::gpio_target_socket& socket, bool state) {
  if (!state)
    return;

  if (socket == ocie0a_ack) {
    write_tifr0(TIFR0_OCF0A);
  } else if (socket == ocie0b_ack) {
    write_tifr0(TIFR0_OCF0B);
  } else if (socket == toie0_ack) {
    write_tifr0(TIFR0_TOV0);
  } else if (socket == ocie1a_ack) {
    write_tifr1(TIFR1_OCF1A);
  } else if (socket == ocie1b_ack) {
    write_tifr1(TIFR1_OCF1B);
  } else if (socket == toie1_ack) {
    write_tifr1(TIFR1_TOV1);
  } else if (socket == icie1_ack) {
    write_tifr1(TIFR1_ICF1);
  } else if (socket == ocie2a_ack) {
    write_tifr2(TIFR2_OCF2A);
  } else if (socket == ocie2b_ack) {
    write_tifr2(TIFR2_OCF2B);
  } else if (socket == toie2_ack) {
    write_tifr2(TIFR2_TOV2);
  }
}

u32 Timer::elapsed_cycles(const sc_core::sc_time& start) const {
  const double ratio = (sc_core::sc_time_stamp() - start) / clock_cycle();
  return (u32)(ratio + 0.5);
}

void Timer::run_timer(u32 (Timer::*div_fn)() const, void (Timer::*tick_fn)(),
                       u32& ps, sc_core::sc_time& wait_start,
                       sc_core::sc_event& div_ev) {
  while (true) {
    if (timers_stopped()) {
      wait(resume_ev_);
      continue;
    }

    const u32 div = (this->*div_fn)();
    if (div == 0) {
      wait(div_ev);
      continue;
    }

    if (ps >= div) {
      ps = 0;
      (this->*tick_fn)();
      continue;
    }

    wait_start = sc_core::sc_time_stamp();
    wait(clock_cycles(div - ps), resume_ev_);

    if (resume_ev_.triggered()) {
      ps += elapsed_cycles(wait_start);
    } else {
      ps = 0;
      (this->*tick_fn)();
    }
  }
}

void Timer::run_timer0() {
  run_timer(&Timer::timer0_div, &Timer::tick_timer0, ps0_, wait_start0_,
            div0_ev_);
}

void Timer::run_timer1() {
  sync_16bit_regs_from_state();
  run_timer(&Timer::timer1_div, &Timer::tick_timer1, ps1_, wait_start1_,
            div1_ev_);
}

void Timer::run_timer2() {
  run_timer(&Timer::timer2_div, &Timer::tick_timer2, ps2_, wait_start2_,
            div2_ev_);
}
