/******************************************************************************
 * Apache License, Version 2.0
 * Copyright (c) 2026 chciken/Niko Zurstraßen
 *****************************************************************************/

#ifndef VCML_MODELS_MAX7219_H
#define VCML_MODELS_MAX7219_H

#include <vcml.h>

namespace vcml {
namespace spi {

class max7219 : public vcml::component, public vcml::spi_host
{
public:
    static constexpr uint NUM_DIGITS = 8;
    static constexpr uint LEDS_PER_DIGIT = 8;
    static constexpr uint NUM_LEDS = NUM_DIGITS * LEDS_PER_DIGIT;

    max7219(const sc_core::sc_module_name& nm);
    virtual ~max7219() = default;

    vcml::gpio_target_socket cs;
    virtual void gpio_transport(const gpio_target_socket& socket,
                                gpio_payload& tx) override;

    vcml::spi_target_socket spi_in;
    vcml::spi_initiator_socket spi_out;
    virtual void spi_transport(const spi_target_socket& socket,
                               spi_payload& spi) override;

    vcml::gpio_initiator_array<NUM_LEDS> led_out;
    vcml::gpio::leds led_matrix;

private:
    u16 value_;
    bool enabled_;
    bool test_mode_;
    uint scan_limit_;
    uint decode_mode_;
    uint intensity_;
    u8 digit_data[NUM_DIGITS];

    u8 code_b_mapping(uint data);
    void trigger_command();
    void update_outputs();
    void update_digit(size_t digit, u8 data);
};

} // namespace spi
} // namespace vcml

#endif
