/******************************************************************************
 * Apache License, Version 2.0
 * Copyright (c) 2026 chciken/Niko Zurstraßen
 *****************************************************************************/

#pragma once

#include <vcml.h>

#include "common.h"

struct Twi : public vcml::peripheral, public vcml::i2c_host {
  Twi(const sc_core::sc_module_name& nm);
  ~Twi() = default;

  vcml::reg<u8> twbr;
  vcml::reg<u8> twcr;
  vcml::reg<u8> twsr;
  vcml::reg<u8> twdr;
  vcml::reg<u8> twar;
  vcml::reg<u8> twamr;

  vcml::i2c_initiator_socket out_i2c;
  vcml::i2c_target_socket in_i2c;
  vcml::gpio_initiator_socket irq;
  vcml::gpio_target_socket ack;
  vcml::tlm_target_socket in;

  virtual void reset() override;
  virtual void i2c_transport(vcml::i2c_target_socket&, vcml::i2c_payload&) override;

 private:
  bool master_active_;
  bool master_start_pending_;
  vcml::tlm_command master_dir_;

  bool slave_addressed_;
  bool slave_general_call_;
  vcml::tlm_command slave_dir_;

  void WriteTwcr(u8 val);
  void WriteTwsr(u8 val);
  void WriteTwdr(u8 val);
  void WriteTwar(u8 val);
  void WriteTwamr(u8 val);

  void DoMasterContinue(bool twea);
  void AbortTransaction();

  void SetStatus(u8 code);
  void UpdateIrq();

  void SlaveStart(vcml::i2c_payload& tx);
  void SlaveData(vcml::i2c_payload& tx);
  void SlaveStop(vcml::i2c_payload& tx);
};
