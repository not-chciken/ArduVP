#include "max7219.h"

namespace vcml {
namespace spi {

constexpr uint CMD_NOOP = 0x0;
constexpr uint CMD_DIGIT0 = 0x1;
constexpr uint CMD_DIGIT1 = 0x2;
constexpr uint CMD_DIGIT2 = 0x3;
constexpr uint CMD_DIGIT3 = 0x4;
constexpr uint CMD_DIGIT4 = 0x5;
constexpr uint CMD_DIGIT5 = 0x6;
constexpr uint CMD_DIGIT6 = 0x7;
constexpr uint CMD_DIGIT7 = 0x8;
constexpr uint CMD_DECODE_MODE = 0x9;
constexpr uint CMD_INTENSITY = 0xa;
constexpr uint CMD_SCAN_LIMIT = 0xb;
constexpr uint CMD_SHUTDOWN = 0xc;
constexpr uint CMD_DISPLAY_TEST = 0xf;

constexpr u8 CODE_B_0 = 0b1111110u;
constexpr u8 CODE_B_1 = 0b0110000u;
constexpr u8 CODE_B_2 = 0b1101101u;
constexpr u8 CODE_B_3 = 0b1111001u;
constexpr u8 CODE_B_4 = 0b0110011u;
constexpr u8 CODE_B_5 = 0b1011011u;
constexpr u8 CODE_B_6 = 0b1011111u;
constexpr u8 CODE_B_7 = 0b1110000u;
constexpr u8 CODE_B_8 = 0b1111111u;
constexpr u8 CODE_B_9 = 0b1111011u;
constexpr u8 CODE_B_DASH = 0b0000001u;
constexpr u8 CODE_B_E = 0b1001111u;
constexpr u8 CODE_B_H = 0b0110111u;
constexpr u8 CODE_B_L = 0b0001110u;
constexpr u8 CODE_B_P = 0b1100111u;
constexpr u8 CODE_B_BLANK = 0b0000000u;

max7219::max7219(const sc_core::sc_module_name& nm):
    component(nm),
    cs("cs"),
    spi_in("spi_in"),
    spi_out("spi_out"),
    led_out("led_out"),
    led_matrix("led_matrix"),
    value_(0),
    enabled_(false),
    test_mode_(false),
    scan_limit_(0),
    decode_mode_(0),
    intensity_(0),
    digit_data() {
    for (size_t i = 0; i < NUM_LEDS; i++)
        led_out[i].bind(led_matrix.gpio_in[i]);
    clk.stub();
    rst.stub();
}

void vcml::spi::max7219::gpio_transport(const gpio_target_socket& socket,
                                        [[maybe_unused]] gpio_payload& tx) {
    if (socket == cs && cs == true) {
        trigger_command();
        update_outputs();
    }
}

void vcml::spi::max7219::spi_transport(const spi_target_socket&,
                                       spi_payload& spi) {
    if (cs == true)
        return;

    u32 dout = 0;
    for (int bit = 31; bit >= 0; bit--) {
        if (!(spi.mask & mwr::bit(bit)))
            continue;
        if (value_ & mwr::bit(15))
            dout |= mwr::bit(bit);
        value_ = (u16)((value_ << 1) | ((spi.mosi >> bit) & 1u));
    }

    if (spi_out.is_stubbed()) {
        spi.miso = dout;
        return;
    }

    spi_payload chained(dout, 0, spi.mask);
    spi_out.transport(chained);
    spi.miso = chained.miso;
}

u8 max7219::code_b_mapping(uint data) {
    data &= 0xfu;
    switch (data) {
    case 0x0:
        return CODE_B_0;
    case 0x1:
        return CODE_B_1;
    case 0x2:
        return CODE_B_2;
    case 0x3:
        return CODE_B_3;
    case 0x4:
        return CODE_B_4;
    case 0x5:
        return CODE_B_5;
    case 0x6:
        return CODE_B_6;
    case 0x7:
        return CODE_B_7;
    case 0x8:
        return CODE_B_8;
    case 0x9:
        return CODE_B_9;
    case 0xa:
        return CODE_B_DASH;
    case 0xb:
        return CODE_B_E;
    case 0xc:
        return CODE_B_H;
    case 0xd:
        return CODE_B_L;
    case 0xe:
        return CODE_B_P;
    case 0xf:
        return CODE_B_BLANK;
    default:
        log_error("unknown code b mapping");
        return 0;
    }
}

void max7219::trigger_command() {
    uint cmd = (value_ >> 8) & 0xfu;
    uint data = value_ & 0xffu;

    switch (cmd) {
    case CMD_NOOP:
        return;
    case CMD_DIGIT0:
    case CMD_DIGIT1:
    case CMD_DIGIT2:
    case CMD_DIGIT3:
    case CMD_DIGIT4:
    case CMD_DIGIT5:
    case CMD_DIGIT6:
    case CMD_DIGIT7:
        digit_data[cmd - 1] = data;
        return;
    case CMD_DECODE_MODE:
        decode_mode_ = data;
        return;
    case CMD_INTENSITY:
        intensity_ = data & 0xfu;
        return;
    case CMD_SCAN_LIMIT:
        scan_limit_ = data & 0b111u;
        return;
    case CMD_SHUTDOWN:
        enabled_ = data & 1;
        return;
    case CMD_DISPLAY_TEST:
        test_mode_ = data & 1;
        return;
    default:
        log_warn("unknown command"); // TODO: What happens on real hardware?
    }
}

void max7219::update_outputs() {
    if (test_mode_) {
        for (size_t i = 0; i < NUM_LEDS; i++)
            led_out[i] = true;
        return;
    }

    if (!enabled_) {
        for (size_t i = 0; i < NUM_LEDS; i++)
            led_out[i] = false;
        return;
    }

    for (size_t i = 0; i < NUM_DIGITS; i++) {
        if (i > scan_limit_) {
            update_digit(i, 0);
            continue;
        }

        u8 data = digit_data[i];
        if (decode_mode_ & mwr::bit(i))
            data = (data & 0x80u) | code_b_mapping(data);
        update_digit(i, data);
    }
}

void max7219::update_digit(size_t digit, u8 data) {
    for (size_t i = 0; i < LEDS_PER_DIGIT; i++)
        led_out[digit * LEDS_PER_DIGIT + i] = (bool)(data & mwr::bit(i));
}

} // namespace spi
} // namespace vcml
