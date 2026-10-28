/******************************************************************************
 * Apache License, Version 2.0
 * Copyright (c) 2026 chciken/Niko Zurstraßen
 *****************************************************************************/

#pragma once

#include <vcml.h>

#include "common.h"

struct Usart : public vcml::peripheral, public vcml::serial_host {
  Usart(const sc_core::sc_module_name& nm);
  virtual ~Usart() = default;

  vcml::property<bool> untimed;

  vcml::reg<u8> ucsrna;
  vcml::reg<u8> ucsrnb;
  vcml::reg<u8> ucsrnc;
  vcml::reg<u8> ubrrnl;
  vcml::reg<u8> ubrrnh;
  vcml::reg<u8> udrn;

  vcml::tlm_target_socket in;
  vcml::serial_initiator_socket serial_tx;
  vcml::serial_target_socket serial_rx;

  vcml::gpio_initiator_socket irq_rx;
  vcml::gpio_initiator_socket irq_tx;
  vcml::gpio_initiator_socket irq_udre;

  vcml::gpio_target_socket rxc_ack;
  vcml::gpio_target_socket udre_ack;
  vcml::gpio_target_socket tx_ack;

  virtual void reset() override;
  virtual void serial_receive(const vcml::serial_target_socket& socket, vcml::serial_payload& tx) override;
  virtual void gpio_notify(const vcml::gpio_target_socket& socket, bool state) override;

 private:
  SC_HAS_PROCESS(Usart);

  sc_core::sc_event tx_ev_;
  bool rx_pending_;
  u8 rx_data_;

  void TxThread();

  void UpdateSerialFormat();
  void UpdateSerialBaud();

  u8 ReadUcsrna();
  void WriteUcsrna(u8 val);
  void WriteUcsrnb(u8 val);
  void WriteUcsrnc(u8 val);
  void WriteUbrrnl(u8 val);
  void WriteUbrrnh(u8 val);

  u8 ReadUdrn(bool dbg);
  void WriteUdrn(u8 val);

  bool RxEnabled() const;
  bool TxEnabled() const;
  u16 Ubrr() const;

  void SetRxComplete(bool set);
  void SetTxComplete(bool set);
  void SetUdre(bool set);
};
