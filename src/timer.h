#pragma once
#include <vcml.h>

#include "common.h"

struct Timer : public vcml::peripheral {
  vcml::tlm_target_socket in;

  vcml::gpio_initiator_socket ocie0a_out;
  vcml::gpio_initiator_socket ocie0b_out;
  vcml::gpio_initiator_socket toie0_out;

  vcml::gpio_initiator_socket ocie1a_out;
  vcml::gpio_initiator_socket ocie1b_out;
  vcml::gpio_initiator_socket toie1_out;
  vcml::gpio_initiator_socket icie1_out;

  vcml::gpio_initiator_socket ocie2a_out;
  vcml::gpio_initiator_socket ocie2b_out;
  vcml::gpio_initiator_socket toie2_out;

  vcml::reg<u8> gtccr;

  vcml::reg<u8> tifr0;
  vcml::reg<u8> tccr0a;
  vcml::reg<u8> tccr0b;
  vcml::reg<u8> tcnt0;
  vcml::reg<u8> ocr0a;
  vcml::reg<u8> ocr0b;
  vcml::reg<u8> timsk0;

  vcml::reg<u8> tifr1;
  vcml::reg<u8> tccr1a;
  vcml::reg<u8> tccr1b;
  vcml::reg<u8> tccr1c;
  vcml::reg<u8> tcnt1l;
  vcml::reg<u8> tcnt1h;
  vcml::reg<u8> icr1l;
  vcml::reg<u8> icr1h;
  vcml::reg<u8> ocr1al;
  vcml::reg<u8> ocr1ah;
  vcml::reg<u8> ocr1bl;
  vcml::reg<u8> ocr1bh;
  vcml::reg<u8> timsk1;

  vcml::reg<u8> tifr2;
  vcml::reg<u8> tccr2a;
  vcml::reg<u8> tccr2b;
  vcml::reg<u8> tcnt2;
  vcml::reg<u8> ocr2a;
  vcml::reg<u8> ocr2b;
  vcml::reg<u8> assr;
  vcml::reg<u8> timsk2;

  Timer(const sc_core::sc_module_name& nm);

  virtual void reset() override;

 private:
  SC_HAS_PROCESS(Timer);

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

  u32 ps0_;
  u32 ps1_;
  u32 ps2_;

  bool dir0_;
  bool dir1_;
  bool dir2_;

  void run();

  void write_gtccr(u8 val);
  void write_tifr0(u8 val);
  void write_tifr1(u8 val);
  void write_tifr2(u8 val);
  void write_timsk0(u8 val);
  void write_timsk1(u8 val);
  void write_timsk2(u8 val);

  u8 read_tcnt1l();
  u8 read_tcnt1h();
  void write_tcnt1h(u8 val);
  void write_tcnt1l(u8 val);

  u8 read_ocr1al();
  u8 read_ocr1ah();
  void write_ocr1ah(u8 val);
  void write_ocr1al(u8 val);

  u8 read_ocr1bl();
  u8 read_ocr1bh();
  void write_ocr1bh(u8 val);
  void write_ocr1bl(u8 val);

  u8 read_icr1l();
  u8 read_icr1h();
  void write_icr1h(u8 val);
  void write_icr1l(u8 val);

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
