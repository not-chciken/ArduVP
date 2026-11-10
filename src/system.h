/******************************************************************************
 * Apache License, Version 2.0
 * Copyright (c) 2026 chciken/Niko Zurstraßen
 *****************************************************************************/

#pragma once

#include <systemc.h>
#include <vcml.h>

#include <iostream>

#include "atm328p.h"
#include "models/ssd1306.h"
#include "models/eeprom_24xx.h"
#include "models/bmp180.h"

struct ArduSystem : public vcml::system {
  static constexpr size_t NUM_ANALOG_PINS = 6;

  ArduSystem(const sc_core::sc_module_name& nm)
      : vcml::system(nm),
        volt_pin_a("volt_pin_a", 0.0f),
        pin_a("pin_a"),
        pin_d("pin_d"),
        atm328p("atm328p"),
        i2c_bus("i2c_bus"),
        ads1015("ads1015", 0x48),
        eeprom_24xx256("eeprom_24xx256"),
        ssd1306("ssd1306"),
        bmp180("bmp180"),
        max31855("max31855"),
        builtin_led("builtin_led"),
        buttons("buttons"),
        clock("clock", 16 * vcml::MHz),
        reset("reset"),
        console("console"),
        serial_term("serial"),
        throttle("throttle") {
    i2c_bus.bind(atm328p.twi.out_i2c);
    i2c_bus.bind(ssd1306.in);
    i2c_bus.bind(bmp180.in);
    i2c_bus.bind(ads1015.i2c_in);
    i2c_bus.bind(eeprom_24xx256.i2c_in);
    atm328p.twi.in_i2c.stub();

    clock.clk.bind(atm328p.clk);
    clock.clk.bind(i2c_bus.clk);

    reset.rst.bind(atm328p.rst);
    reset.rst.bind(ssd1306.rst);
    reset.rst.bind(bmp180.rst);
    reset.rst.bind(eeprom_24xx256.rst);
    reset.rst.bind(i2c_bus.rst);

    atm328p.usart.serial_tx.bind(serial_term.serial_rx);
    serial_term.serial_tx.bind(atm328p.usart.serial_rx);

    pin_d[6].bind(atm328p.comparator.volt_ain[0]);
    pin_d[7].bind(atm328p.comparator.volt_ain[1]);

    for (size_t i = 0; i < NUM_ANALOG_PINS; ++i) {
      pin_a[i].bind(atm328p.adc.volt_adc[i]);
      pin_a[i] = volt_pin_a[i];
    }

    atm328p.spi.spi_out.bind(max31855.spi_in);
    max31855.bind(atm328p.io_ports.out_pinb[2], false);

    atm328p.io_ports.out_pinb[5].bind(builtin_led.gpio_in[0]);
    buttons.gpio_in[0].bind(atm328p.io_ports.in_pind[2]);

    for (size_t i = 0; i < IoPorts::NUM_PORTS; ++i) {
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

  vcml::property<float, NUM_ANALOG_PINS> volt_pin_a;

  vcml::signal_initiator_array<float, NUM_ANALOG_PINS> pin_a;
  vcml::signal_initiator_array<float, 8> pin_d;

  Atm328p atm328p;

  vcml::i2c::bus i2c_bus;

  vcml::i2c::ads1015 ads1015;
  vcml::i2c::eeprom_24xx eeprom_24xx256;
  vcml::i2c::ssd1306 ssd1306;
  vcml::i2c::bmp180 bmp180;

  vcml::spi::max31855 max31855;

  vcml::gpio::leds builtin_led;
  vcml::gpio::buttons buttons;

  vcml::generic::clock clock;
  vcml::generic::reset reset;

  vcml::ui::console console;

  vcml::serial::terminal serial_term;

  vcml::meta::throttle throttle;
};
