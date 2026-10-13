#pragma once

#include <vcml.h>

#include "common.h"

struct Timer : public vcml::peripheral {
  Timer(const sc_core::sc_module_name& nm);
  ~Timer() = default;

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

  vcml::gpio_target_socket ocie0a_ack;
  vcml::gpio_target_socket ocie0b_ack;
  vcml::gpio_target_socket toie0_ack;

  vcml::gpio_target_socket ocie1a_ack;
  vcml::gpio_target_socket ocie1b_ack;
  vcml::gpio_target_socket toie1_ack;
  vcml::gpio_target_socket icie1_ack;

  vcml::gpio_target_socket ocie2a_ack;
  vcml::gpio_target_socket ocie2b_ack;
  vcml::gpio_target_socket toie2_ack;

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

  sc_core::sc_event resume_ev_;
  sc_core::sc_event div0_ev_;
  sc_core::sc_event div1_ev_;
  sc_core::sc_event div2_ev_;

  sc_core::sc_time wait_start0_;
  sc_core::sc_time wait_start1_;
  sc_core::sc_time wait_start2_;

  void RunTimer0();
  void RunTimer1();
  void RunTimer2();
  void RunTimer(u32 (Timer::*div_fn)() const, void (Timer::*tick_fn)(), u32& ps, sc_core::sc_time& wait_start,
                sc_core::sc_event& div_ev);

  u32 ElapsedCycles(const sc_core::sc_time& start) const;

  void WriteGtccr(u8 val);
  void WriteTifr0(u8 val);
  void WriteTifr1(u8 val);
  void WriteTifr2(u8 val);
  void WriteTimsk0(u8 val);
  void WriteTimsk1(u8 val);
  void WriteTimsk2(u8 val);

  void WriteTccr0b(u8 val);
  void WriteTccr1b(u8 val);
  void WriteTccr2b(u8 val);

  u8 ReadTcnt1l();
  u8 ReadTcnt1h();
  void WriteTcnt1h(u8 val);
  void WriteTcnt1l(u8 val);

  u8 ReadOcr1al();
  u8 ReadOcr1ah();
  void WriteOcr1ah(u8 val);
  void WriteOcr1al(u8 val);

  u8 ReadOcr1bl();
  u8 ReadOcr1bh();
  void WriteOcr1bh(u8 val);
  void WriteOcr1bl(u8 val);

  u8 ReadIcr1l();
  u8 ReadIcr1h();
  void WriteIcr1h(u8 val);
  void WriteIcr1l(u8 val);

  bool TimersStopped() const;

  u32 Timer0Div() const;
  u32 Timer1Div() const;
  u32 Timer2Div() const;

  void TickTimer0();
  void TickTimer1();
  void TickTimer2();

  void SetTifr0(u8 mask);
  void SetTifr1(u8 mask);
  void SetTifr2(u8 mask);

  void CheckInterrupts();

  void Sync16bitRegsFromState();

  virtual void gpio_notify(const vcml::gpio_target_socket& socket, bool state) override;
};
