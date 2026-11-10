/******************************************************************************
 * Apache License, Version 2.0
 * Copyright (c) 2026 chciken/Niko Zurstraßen
 *****************************************************************************/

#include "spi.h"

constexpr u8 SPCR_SPR = mwr::bit(0) | mwr::bit(1);
constexpr u8 SPCR_CPHA = mwr::bit(2);
constexpr u8 SPCR_CPOL = mwr::bit(3);
constexpr u8 SPCR_MSTR = mwr::bit(4);
constexpr u8 SPCR_DORD = mwr::bit(5);
constexpr u8 SPCR_SPE = mwr::bit(6);
constexpr u8 SPCR_SPIE = mwr::bit(7);

constexpr u8 SPSR_SPI2X = mwr::bit(0);
constexpr u8 SPSR_WCOL = mwr::bit(6);
constexpr u8 SPSR_SPIF = mwr::bit(7);

Spi::Spi(const sc_core::sc_module_name& nm)
    : vcml::peripheral(nm),
      spcr("spcr", 0x0, 0),
      spsr("spsr", 0x1, 0),
      spdr("spdr", 0x2, 0),
      spi_out("spi_out"),
      in("in"),
      irq("irq"),
      ack("ack"),
      sck_frequency_(0),
      master_mode_(false),
      lsb_first_(false),
      irq_enabled_(false),
      enabled_(false),
      data_read_(0),
      spif_read_(false) {
  spcr.allow_read_write();
  spcr.sync_always();
  spcr.on_write(&Spi::WriteSpcr);

  spsr.allow_read_write();
  spsr.sync_always();
  spsr.on_read(&Spi::ReadSpsr);
  spsr.on_write(&Spi::WriteSpsr);

  spdr.allow_read_write();
  spdr.sync_always();
  spdr.on_read(&Spi::ReadSpdr);
  spdr.on_write(&Spi::WriteSpdr);
}

void Spi::reset() {
  peripheral::reset();

  master_mode_ = false;
  lsb_first_ = false;
  irq_enabled_ = false;
  enabled_ = false;
  data_read_ = 0;
  spif_read_ = false;

  irq = false;
  UpdateSckFrequency();
}

void Spi::UpdateSckFrequency() {
  static constexpr uint kDividers[] = { 4, 16, 64, 128 };
  uint divider = kDividers[spcr & SPCR_SPR];
  if (spsr & SPSR_SPI2X)
    divider /= 2;
  sck_frequency_ = clk.get_hz() / divider;
}

void Spi::UpdateIrq() {
  irq = irq_enabled_ && (spsr & SPSR_SPIF);
}

void Spi::gpio_notify(const vcml::gpio_target_socket& socket, [[maybe_unused]] bool state) {
  if (socket == ack) {
    spsr &= ~SPSR_SPIF;
    spif_read_ = false;
    UpdateIrq();
  }
}

void Spi::WriteSpcr(u8 val) {
  spcr = val;
  master_mode_ = SPCR_MSTR & val;
  lsb_first_ = SPCR_DORD & val;
  enabled_ = SPCR_SPE & val;
  irq_enabled_ = SPCR_SPIE & val;
  UpdateSckFrequency();
  UpdateIrq();
}

u8 Spi::ReadSpsr() {
  if (spsr & SPSR_SPIF)
    spif_read_ = true;
  return spsr;
}

void Spi::WriteSpsr(u8 val) {
  spsr = (spsr & ~SPSR_SPI2X) | (val & SPSR_SPI2X);
  UpdateSckFrequency();
}

u8 Spi::ReadSpdr() {
  if (spif_read_) {
    spsr &= ~SPSR_SPIF;
    spif_read_ = false;
    UpdateIrq();
  }

  spdr = data_read_;
  return spdr;
}

void Spi::WriteSpdr(u8 val) {
  if (spif_read_) {
    spsr &= ~SPSR_SPIF;
    spif_read_ = false;
  }

  spdr = val;

  if (enabled_ && master_mode_) {
    vcml::spi_payload payload(val);
    spi_out.transport(payload);
    data_read_ = payload.miso;
    spsr |= SPSR_SPIF;
  }

  UpdateIrq();
}
