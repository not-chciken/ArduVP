#pragma once
#include <vcml.h>

#include "common.h"

struct Usart : public vcml::peripheral, public vcml::serial_host {
  vcml::tlm_target_socket in;

  vcml::serial_initiator_socket serial_tx;
  vcml::serial_target_socket serial_rx;

  Usart(const sc_core::sc_module_name& nm = "usart");

  vcml::reg<u8> ucsr0a; // 0xc0
  vcml::reg<u8> ucsr0b; // 0xc1
  vcml::reg<u8> ucsr0c; // 0xc2
  vcml::reg<u8> ubrr0l; // 0xc4
  vcml::reg<u8> ubrr0h; // 0xc5
  vcml::reg<u8> udr0;   // 0xc6

  virtual void reset() override;

  // serial_host
  virtual void serial_receive(u8 data) override;

  virtual ~Usart() = default;

 private:
  SC_HAS_PROCESS(Usart);

  sc_core::sc_event tx_ev_;

  bool rx_pending_;
  u8 rx_data_;

  void tx_thread();

  void update_serial_format();
  void update_serial_baud();

  u8 read_ucsr0a(bool dbg);
  void write_ucsr0a(u8 val, bool dbg);
  void write_ucsr0b(u8 val, bool dbg);
  void write_ucsr0c(u8 val, bool dbg);
  void write_ubrr0l(u8 val, bool dbg);
  void write_ubrr0h(u8 val, bool dbg);

  u8 read_udr0(bool dbg);
  void write_udr0(u8 val, bool dbg);

  // helpers
  bool rx_enabled() const;
  bool tx_enabled() const;
  u16 ubrr() const;

  void set_rx_complete(bool set);
  void set_tx_complete(bool set);
  void set_udre(bool set);
};