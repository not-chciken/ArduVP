#pragma once

#include <vcml.h>

#include "common.h"

struct Adc : public vcml::peripheral {
  vcml::property<float> volt_aref;
  vcml::property<float> volt_avcc;
  vcml::property<float> volt_1_1;

  vcml::gpio_initiator_socket irq;
  vcml::tlm_target_socket in;

  vcml::gpio_target_socket in_ext_irq0;
  vcml::gpio_target_socket in_timer0_match_a;
  vcml::gpio_target_socket in_timer0_overflow;
  vcml::gpio_target_socket in_timer1_match_b;
  vcml::gpio_target_socket in_timer1_overflow;
  vcml::gpio_target_socket in_timer1_capture;

  vcml::reg<u8> adcl;
  vcml::reg<u8> adch;
  vcml::reg<u8> adcsra;
  vcml::reg<u8> adcsrb;
  vcml::reg<u8> admux;

  Adc(const sc_core::sc_module_name& nm);

protected:
  void ConversionThread();
  sc_core::sc_event conversion_event_;

  int clock_prescaler_;

  void WriteAdcsra(u8 val);

  virtual void gpio_notify(const vcml::gpio_target_socket& socket, bool state) override;
};
