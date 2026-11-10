
/******************************************************************************
 * Apache License, Version 2.0
 * Copyright (c) 2026 chciken/Niko Zurstraßen
 *****************************************************************************/

#pragma once

#include <vcml.h>

#include "common.h"

struct Spi : public vcml::peripheral {
  Spi(const sc_core::sc_module_name& nm);
  virtual ~Spi() = default;

  vcml::reg<u8> spcr;
  vcml::reg<u8> spsr;
  vcml::reg<u8> spdr;

  void WriteSpcr(u8 val);

  u8 ReadSpsr();
  void WriteSpsr(u8 val);

  u8 ReadSpdr();
  void WriteSpdr(u8 val);

  vcml::spi_initiator_socket spi_out;

  vcml::tlm_target_socket in;

  vcml::gpio_initiator_socket irq;
  vcml::gpio_target_socket ack;

  virtual void reset() override;

 protected:
  vcml::hz_t sck_frequency_;
  bool master_mode_;
  bool lsb_first_;
  bool irq_enabled_;
  bool enabled_;
  u8 data_read_;
  bool spif_read_;

  void UpdateSckFrequency();
  void UpdateIrq();

  virtual void gpio_notify(const vcml::gpio_target_socket& socket, bool state) override;
};
