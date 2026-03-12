#pragma once
#include <vcml.h>

#include "common.h"

struct Timer : public vcml::peripheral {
  vcml::tlm_target_socket in;

  // Timer0 interrupts
  vcml::gpio_initiator_socket ocie0a_out;
  vcml::gpio_initiator_socket ocie0b_out;
  vcml::gpio_initiator_socket toie0_out;

  // Timer1 interrupts
  vcml::gpio_initiator_socket ocie1a_out;
  vcml::gpio_initiator_socket ocie1b_out;
  vcml::gpio_initiator_socket toie1_out;
  vcml::gpio_initiator_socket icie1_out;

  // Timer2 interrupts
  vcml::gpio_initiator_socket ocie2a_out;
  vcml::gpio_initiator_socket ocie2b_out;
  vcml::gpio_initiator_socket toie2_out;

  // Shared
  vcml::reg<u8> gtccr; // 0x43

  // Timer0 (8-bit)
  vcml::reg<u8> tifr0;  // 0x35
  vcml::reg<u8> tccr0a; // 0x44
  vcml::reg<u8> tccr0b; // 0x45
  vcml::reg<u8> tcnt0;  // 0x46
  vcml::reg<u8> ocr0a;  // 0x47
  vcml::reg<u8> ocr0b;  // 0x48
  vcml::reg<u8> timsk0; // 0x6e

  // Timer1 (16-bit)
  vcml::reg<u8> tifr1;   // 0x36
  vcml::reg<u8> tccr1a;  // 0x80
  vcml::reg<u8> tccr1b;  // 0x81
  vcml::reg<u8> tccr1c;  // 0x82
  vcml::reg<u8> tcnt1l;  // 0x84
  vcml::reg<u8> tcnt1h;  // 0x85
  vcml::reg<u8> icr1l;   // 0x86
  vcml::reg<u8> icr1h;   // 0x87
  vcml::reg<u8> ocr1al;  // 0x88
  vcml::reg<u8> ocr1ah;  // 0x89
  vcml::reg<u8> ocr1bl;  // 0x8a
  vcml::reg<u8> ocr1bh;  // 0x8b
  vcml::reg<u8> timsk1;  // 0x6f

  // Timer2 (8-bit)
  vcml::reg<u8> tifr2;  // 0x37
  vcml::reg<u8> tccr2a; // 0xb0
  vcml::reg<u8> tccr2b; // 0xb1
  vcml::reg<u8> tcnt2;  // 0xb2
  vcml::reg<u8> ocr2a;  // 0xb3
  vcml::reg<u8> ocr2b;  // 0xb4
  vcml::reg<u8> assr;   // 0xb6
  vcml::reg<u8> timsk2; // 0x70

  Timer(const sc_core::sc_module_name& nm = "timer");

  virtual void reset() override;

 private:
  SC_HAS_PROCESS(Timer);

  // Internal combined 16-bit values + access latches
  u16 tcnt1_;
  u16 ocr1a_;
  u16 ocr1b_;
  u16 icr1_;

  u8 tcnt1_latch_h_;
  u8 ocr1a_latch_h_;
  u8 ocr1b_latch_h_;
  u8 icr1_latch_h_;

  u8 tcnt1_write_h_;
  u8 ocr1a_write_h_;
  u8 ocr1b_write_h_;
  u8 icr1_write_h_;

  // Prescaler counters
  u32 ps0_;
  u32 ps1_;
  u32 ps2_;

  // Phase correct direction (true=up)
  bool dir0_;
  bool dir1_;
  bool dir2_;

  void run();

  // Register callbacks
  void write_gtccr(u8 val, bool dbg);
  void write_tifr0(u8 val, bool dbg);
  void write_tifr1(u8 val, bool dbg);
  void write_tifr2(u8 val, bool dbg);
  void write_timsk0(u8 val, bool dbg);
  void write_timsk1(u8 val, bool dbg);
  void write_timsk2(u8 val, bool dbg);

  u8 read_tcnt1l(bool dbg);
  u8 read_tcnt1h(bool dbg);
  void write_tcnt1h(u8 val, bool dbg);
  void write_tcnt1l(u8 val, bool dbg);

  u8 read_ocr1al(bool dbg);
  u8 read_ocr1ah(bool dbg);
  void write_ocr1ah(u8 val, bool dbg);
  void write_ocr1al(u8 val, bool dbg);

  u8 read_ocr1bl(bool dbg);
  u8 read_ocr1bh(bool dbg);
  void write_ocr1bh(u8 val, bool dbg);
  void write_ocr1bl(u8 val, bool dbg);

  u8 read_icr1l(bool dbg);
  u8 read_icr1h(bool dbg);
  void write_icr1h(u8 val, bool dbg);
  void write_icr1l(u8 val, bool dbg);

  // Tick helpers
  bool timers_stopped() const;

  u32 timer0_div() const;
  u32 timer1_div() const;
  u32 timer2_div() const;

  void tick_timer0();
  void tick_timer1();
  void tick_timer2();

  void set_tifr0(u8 mask);
  void set_tifr1(u8 mask);
  void set_tifr2(u8 mask);

  void check_interrupts();

  void sync_16bit_regs_from_state();
};