#pragma once
#include <vcml.h>

#include "common.h"

struct Usart : public vcml::peripheral, public vcml::serial_host {
  vcml::tlm_target_socket in;

  vcml::serial_initiator_socket serial_tx;
  vcml::serial_target_socket serial_rx;

  Usart(const sc_core::sc_module_name& nm = "usart");

  vcml::reg<u8> ucsrna;
  vcml::reg<u8> ucsrnb;
  vcml::reg<u8> ucsrnc;
  vcml::reg<u8> ubrrnl;
  vcml::reg<u8> ubrrnh;
  vcml::reg<u8> udrn;  

  vcml::gpio_initiator_socket irq_rx;
  vcml::gpio_initiator_socket irq_tx;
  vcml::gpio_initiator_socket irq_udre;

  virtual void reset() override;

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

  u8 read_ucsrna(bool dbg);
  void write_ucsrna(u8 val, bool dbg);
  void write_ucsrnb(u8 val, bool dbg);
  void write_ucsrnc(u8 val, bool dbg);
  void write_ubrrnl(u8 val, bool dbg);
  void write_ubrrnh(u8 val, bool dbg);

  u8 read_udrn(bool dbg);
  void write_udrn(u8 val, bool dbg);

  bool rx_enabled() const;
  bool tx_enabled() const;
  u16 ubrr() const;

  void set_rx_complete(bool set);
  void set_tx_complete(bool set);
  void set_udre(bool set);
};