#pragma once

#include <vcml.h>

#include "common.h"

struct Comparator : public vcml::peripheral, public vcml::signal_host<float>, public vcml::signal_host<u8> {
  // Analog inputs, driven from outside (e.g. by whoever models the board's
  // analog signals). ADC0..7 are shared with the Adc peripheral's own pins.
  vcml::signal_target_array<float, 8> volt_adc;
  vcml::signal_target_array<float, 2> volt_ain;
  vcml::property<float> volt_bandgap;

  vcml::gpio_initiator_socket aco;
  vcml::gpio_initiator_socket acic;
  vcml::gpio_initiator_socket irq;

  vcml::gpio_target_socket in_acme;
  vcml::gpio_target_socket in_aden;
  vcml::signal_target_socket<u8> in_admux;

  vcml::tlm_target_socket in;

  vcml::reg<u8> acsr;

  Comparator(const sc_core::sc_module_name& nm);

  virtual void signal_transport(const vcml::signal_target_socket<float>& socket, const float& value) override;
  virtual void signal_transport(const vcml::signal_target_socket<u8>& socket, const u8& value) override;

 private:
  void DoComparison();

  void WriteAcsr(u8 val);
};
