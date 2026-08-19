#pragma once

#include <vcml.h>

#include "common.h"

struct Adc : public vcml::peripheral, public vcml::signal_host<float> {
  vcml::signal_target_array<float, 8> volt_adc;

  vcml::property<float> volt_aref;
  vcml::property<float> volt_avcc;
  vcml::property<float> volt_1_1;
  vcml::property<float> temp_celsius;

  vcml::gpio_initiator_socket irq;
  vcml::gpio_target_socket ack;
  vcml::tlm_target_socket in;

  vcml::gpio_target_socket in_ext_irq0;
  vcml::gpio_target_socket in_timer0_match_a;
  vcml::gpio_target_socket in_timer0_overflow;
  vcml::gpio_target_socket in_timer1_match_b;
  vcml::gpio_target_socket in_timer1_overflow;
  vcml::gpio_target_socket in_timer1_capture;
  vcml::gpio_target_socket in_analog_comparator;

  vcml::gpio_initiator_socket out_acme;
  vcml::gpio_initiator_socket out_aden;
  vcml::signal_initiator_socket<u8> out_admux;

  vcml::reg<u8> adcl;
  vcml::reg<u8> adch;
  vcml::reg<u8> adcsra;
  vcml::reg<u8> adcsrb;
  vcml::reg<u8> admux;

  Adc(const sc_core::sc_module_name& nm);

  virtual void reset() override;

  virtual void signal_transport(const vcml::signal_target_socket<float>& socket, const float& value) override;

 protected:
  void ConversionThread();
  sc_core::sc_event conversion_event_;

  float GetInputVoltage(uint channel);
  float GetReferenceVoltage();
  float TempToVolt();

  void LaunchConversion();

  int clock_prescaler_;
  bool data_blocked_;
  bool first_conversion_;
  bool conversion_in_progress_;
  uint mux_channel_;

  void WriteAdcsra(u8 val);
  void WriteAdcsrb(u8 val);
  void WriteAdmux(u8 val);

  u8 ReadAdcl();
  u8 ReadAdch();

  virtual void gpio_notify(const vcml::gpio_target_socket& socket, bool state) override;
};
