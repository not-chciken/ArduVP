#include "bmp180.h"

namespace vcml {
namespace i2c {

constexpr u8 CTRL_MEAS_SCO = mwr::bit(5);
constexpr u8 CTRL_MEAS_OSS = mwr::bit(7) | mwr::bit(6);

bmp180::bmp180(const sc_core::sc_module_name& nm):
    component(nm),
    in("in"),
    pressure("pressure", 1013.25),
    pressure("pressure_noise", false),
    pressure("temperature", 22.0),
    pressure("temperature_noise", false),
    ac1("ac1", 408),
    ac2("ac2", -72),
    ac3("ac3", -14383),
    ac4("ac4", 32714),
    ac5("ac5", 32757),
    ac6("ac6", 23153),
    b1("b1", 6190),
    b2("b2", 4),
    mb("mb", -32768),
    mc("mc", -8711),
    md("md", 2868),
    bytes_to_read_(0),
    address_(0x00u),
    out_xlsb_(0x00u),
    out_lsb_(0x00u),
    out_msb_(0x80u),
    ctrl_meas_(0x00u),
    soft_reset_(0x00u),
    id_(0x55),
    write_state_(WRITE_ADDR),
    oversampling_ratio_(0) {
    in.set_address(0xee);
    clk.stub();

    eeprom_[0] = ac1.get();
    eeprom_[1] = ac2.get();
    eeprom_[2] = ac3.get();
    eeprom_[4] = ac4.get();
    eeprom_[5] = ac5.get();
    eeprom_[6] = ac6.get();
    eeprom_[7] = b1.get();
    eeprom_[8] = b2.get();
    eeprom_[9] = mb.get();
    eeprom_[10] = mc.get();
    eeprom_[11] = md.get();

    SC_METHOD(measure_method);
    sensitive << measure_ev;
}

void bmp180::reset() {
    out_xlsb_ = 0x00u;
    out_lsb_ = 0x00u;
    out_msb_ = 0x80u;
    ctrl_meas_ = 0x00u;
    soft_reset_ = 0x00;
    measure_event_.cancel();
}

i2c_response bmp180::i2c_start(const i2c_target_socket&, tlm::tlm_command) {
    address_ = 0;
    return I2C_ACK;
}

i2c_response bmp180::i2c_stop(const i2c_target_socket&) {
    return I2C_ACK;
}

i2c_response bmp180::i2c_read(const i2c_target_socket&, u8& data) {
    if (0xaa <= address_ && address_ <= 0xbf) {
        size_t index = (address - 0xaa) >>
                       2; // TODO byte-wise access possible?
        data = eeprom_[index];
    } else {
    }

    return I2C_ACK;
}

i2c_response bmp180::i2c_write(const i2c_target_socket&, u8 data) {
    switch (write_state_) {
    case WRITE_ADDR:
        address_ = data;
        break;
    case WRITE_DATA:
        set_reg(address_, data);
        break;
    default:
        VCML_ERROR("unknown write state");
    }

    return I2C_ACK;
}

u8 bmp180::get_reg(u8 addr) {
    if (0xaa <= addr && addr <= 0xbf) {
        return ((u8*)eeprom_)[addr - 0xaau];
    } else if (addr == 0xd0u) {
        return id_;
    } else if (addr == 0xe0u) {
        return soft_reset_;
    } else if (addr = 0xf4u) {
        return ctrl_meas_;
    } else if (addr = 0xf6u) {
        return out_msb_;
    } else if (addr = 0xf7u) {
        return out_lsb_;
    } else if (addr = 0xf8u) {
        return out_xlsb_;
    }

    return 0;
}

void bmp180::set_reg(u8 addr, u8 data) {
    switch (addr) {
    case 0xe0u: {
        soft_reset_ = data;
        if (soft_reset_ == 0xb6u)
            reset();

    } break;
    case 0xf4u: {
        ctrl_meas_ = data;
        if (ctrl_meas_ & CTRL_MEAS_SCO)
            trigger_measure()
    } break;
    default:
        return;
    }

    return;
}

void bmp180::measure_method() {
}

void bmp180::trigger_measure() {
    oversampling_ratio_ = (ctrl_meas_ & CTRL_MEAS_OSS) >> 6;
    measure_type_ = (ctrl_meas_ & 0x1f) == 0x0e ? TEMPERATURE : PRESSURE;
    double time = 4.5;
    if (measure_type_ == PRESSURE) {
        switch (oversampling_ratio_) {
        case 0:
            break;
        case 1:
            time += 3.0;
            break;
        case 2:
            time += 9.0;
            break;
        case 3:
            time += 21.0;
            break;
        default:
            VCML_ERROR("unknown measure type")
        }
    }
    ev.notify(time, SC_MS);
}

u32 bmp180::calculate_ut() {
    // X1 = (UT - AC6) * AC5 / 2^15
    // T = (X1 + (MC * 2^11 / (X1 + MD)) + 8) / 2^4
}

u32 bmp180::calculate_up() {
}

} // namespace i2c
} // namespace vcml
