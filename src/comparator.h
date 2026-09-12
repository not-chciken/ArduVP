#pragma once

#include <vcml.h>

#include "common.h"

struct Comparator : public vcml::peripheral, public vcml::signal_host<float>, public vcml::signal_host<u8> {
  Comparator(const sc_core::sc_module_name& nm);
  ~Comparator() = default;

  vcml::property<float> volt_bandgap;

  vcml::reg<u8> acsr;

  vcml::signal_target_array<float, 8> volt_adc;
  vcml::signal_target_array<float, 2> volt_ain;
  vcml::gpio_initiator_socket aco;
  vcml::gpio_initiator_socket acic;
  vcml::gpio_initiator_socket irq;
  vcml::gpio_target_socket ack;
  vcml::gpio_target_socket in_acme;
  vcml::gpio_target_socket in_aden;
  vcml::signal_target_socket<u8> in_admux;
  vcml::tlm_target_socket in;

  virtual void signal_transport(const vcml::signal_target_socket<float>& socket, const float& value) override;
  virtual void signal_transport(const vcml::signal_target_socket<u8>& socket, const u8& value) override;
  virtual void gpio_notify(const vcml::gpio_target_socket& socket, bool state) override;

 private:
  void DoComparison();

  void WriteAcsr(u8 val);
};
