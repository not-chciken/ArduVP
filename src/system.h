#pragma once

#include <systemc.h>
#include <vcml.h>

#include <chrono>
#include <iostream>

#include "atm328p.h"
#include "models/ssd1306.h"
#include "models/eeprom_24xx.h"

struct ArduSystem : public vcml::system {
  static constexpr size_t NUM_ANALOG_PINS = 6;

  ArduSystem(const sc_core::sc_module_name& nm)
      : vcml::system(nm),
        volt_pin_a("volt_pin_a", 0.0f),
        pin_a("pin_a"),
        pin_d("pin_d"),
        atm328p("atm328p"),
        console("console"),
        ads1015("ads1015", 0x48),
        eeprom_24xx256("eeprom_24xx256"),
        i2c_bus("i2c_bus"),
        ssd1306("ssd1306"),
        builtin_led("builtin_led"),
        buttons("buttons"),
        serial_term("serial"),
        clock("clock", 16 * vcml::MHz),
        reset("reset"),
        throttle("throttle") {
    i2c_bus.bind(atm328p.twi.out_i2c);
    i2c_bus.bind(ssd1306.in);
    i2c_bus.bind(ads1015.i2c_in);
    i2c_bus.bind(eeprom_24xx256.i2c_in);
    atm328p.twi.in_i2c.stub();

    clock.clk.bind(atm328p.clk);
    reset.rst.bind(atm328p.rst);

    reset.rst.bind(ssd1306.rst);
    reset.rst.bind(eeprom_24xx256.rst);
    reset.rst.bind(i2c_bus.rst);

    atm328p.usart.serial_tx.bind(serial_term.serial_rx);
    serial_term.serial_tx.bind(atm328p.usart.serial_rx);

    clock.clk.bind(i2c_bus.clk);

    pin_d[6].bind(atm328p.comparator.volt_ain[0]);
    pin_d[7].bind(atm328p.comparator.volt_ain[1]);

    for (size_t i = 0; i < NUM_ANALOG_PINS; ++i) {
      pin_a[i].bind(atm328p.adc.volt_adc[i]);
      pin_a[i] = volt_pin_a[i];
    }

    atm328p.io_ports.out_pinb[5].bind(builtin_led.gpio_in[0]);
    buttons.gpio_in[0].bind(atm328p.io_ports.in_pind[2]);

    for (size_t i = 0; i < 8; ++i) {
      if (!atm328p.io_ports.in_pinb[i].is_stubbed())
        atm328p.io_ports.in_pinb[i].stub();
      if (!atm328p.io_ports.in_pinc[i].is_stubbed())
        atm328p.io_ports.in_pinc[i].stub();
      if (!atm328p.io_ports.in_pind[i].is_stubbed())
        atm328p.io_ports.in_pind[i].stub();

      if (!atm328p.io_ports.out_pinb[i].is_stubbed())
        atm328p.io_ports.out_pinb[i].stub();
      if (!atm328p.io_ports.out_pinc[i].is_stubbed())
        atm328p.io_ports.out_pinc[i].stub();
      if (!atm328p.io_ports.out_pind[i].is_stubbed())
        atm328p.io_ports.out_pind[i].stub();
    }
  }

  virtual int run() override { return vcml::system::run(); }
  virtual void start_of_simulation() override { sim_start_time = std::chrono::steady_clock::now(); }
  virtual void end_of_simulation() override {
    auto elapsed = std::chrono::steady_clock::now() - sim_start_time;
    double elapsed_wall_clock_time = std::chrono::duration<double>(elapsed).count();
    double elapsed_virtual_time = vcml::time_stamp_ms() / 1000.0;

    log_info("Elapsed wall-clock time: %.3f seconds", elapsed_wall_clock_time);
    log_info("Elapsed virtual time:    %.3f seconds", elapsed_virtual_time);
    log_info("Real-time factor:        %.3f", elapsed_virtual_time / elapsed_wall_clock_time);
  }

  vcml::property<float, NUM_ANALOG_PINS> volt_pin_a;

  vcml::signal_initiator_array<float, NUM_ANALOG_PINS> pin_a;
  vcml::signal_initiator_array<float, 8> pin_d;

  Atm328p atm328p;

  vcml::ui::console console;

  vcml::i2c::ads1015 ads1015;
  vcml::i2c::bus i2c_bus;
  vcml::i2c::eeprom_24xx eeprom_24xx256;
  vcml::i2c::ssd1306 ssd1306;

  vcml::gpio::leds builtin_led;
  vcml::gpio::buttons buttons;

  vcml::serial::terminal serial_term;

  vcml::generic::clock clock;
  vcml::generic::reset reset;

  vcml::meta::throttle throttle;

  std::chrono::steady_clock::time_point sim_start_time;
};
