/******************************************************************************
 * Apache License, Version 2.0
 * Copyright (c) 2026 chciken/Niko Zurstraßen
 *****************************************************************************/

#include "bmp180.h"

namespace vcml {
namespace i2c {

constexpr u8 BMP180_I2C_ADDR = 0x77u;

constexpr u8 REG_CALIB_FIRST = 0xaau;
constexpr u8 REG_CALIB_LAST = 0xbfu;
constexpr u8 REG_ID = 0xd0u;
constexpr u8 REG_SOFT_RESET = 0xe0u;
constexpr u8 REG_CTRL_MEAS = 0xf4u;
constexpr u8 REG_OUT_MSB = 0xf6u;
constexpr u8 REG_OUT_LSB = 0xf7u;
constexpr u8 REG_OUT_XLSB = 0xf8u;

constexpr u8 SOFT_RESET_CMD = 0xb6u;

constexpr u8 CTRL_MEAS_SCO = mwr::bit(5);
constexpr u8 CTRL_MEAS_OSS = mwr::bit(7) | mwr::bit(6);
constexpr u8 CTRL_MEAS_MC = 0x1fu;
constexpr u8 CTRL_MEAS_MC_TEMPERATURE = 0x0eu;
constexpr u8 CTRL_MEAS_MC_PRESSURE = 0x14u;
constexpr u8 CTRL_MEAS_MC_SEL_TEMPERATURE = mwr::bit(3);
constexpr u8 CTRL_MEAS_MC_SEL_PRESSURE = mwr::bit(4);
constexpr u8 CTRL_MEAS_UNUSED = mwr::bit(2);

constexpr double PRESSURE_NOISE_HPA[] = {0.06, 0.05, 0.04, 0.03};
constexpr double TEMPERATURE_NOISE_C = 0.007;

constexpr i64 ADC_MID_SCALE = 0x8000;
constexpr u32 OUT_MID_SCALE = 0x800000u;

bmp180::bmp180(const sc_core::sc_module_name& nm):
    component(nm),
    in("in"),
    pressure_hpa("pressure_hpa", 1013.25),
    pressure_noise("pressure_noise", false),
    temperature_c("temperature_c", 22.0),
    temperature_noise("temperature_noise", false),
    conversion_time_ms("conversion_time_ms", {4.5, 7.5, 13.5, 25.5}),
    ac1("ac1", 408),
    ac2("ac2", -72),
    ac3("ac3", -14383),
    ac4("ac4", 32741),
    ac5("ac5", 32757),
    ac6("ac6", 23153),
    b1("b1", 6190),
    b2("b2", 4),
    mb("mb", -32768),
    mc("mc", -8711),
    md("md", 2868),
    address_(0x00u),
    out_xlsb_(0x00u),
    out_lsb_(0x00u),
    out_msb_(0x80u),
    ctrl_meas_(0x00u),
    id_(0x55),
    converting_(false),
    samples_oss_(0),
    write_state_(WRITE_ADDR),
    rng_()
    {
    in.set_address(BMP180_I2C_ADDR);
    clk.stub();

    SC_METHOD(measure_method);
    sensitive << measure_event_;
    dont_initialize();
}

void bmp180::reset() {
    out_xlsb_ = 0x00u;
    out_lsb_ = 0x00u;
    out_msb_ = 0x80u;
    ctrl_meas_ = 0x00u;
    converting_ = false;
    measure_event_.cancel();
}

i2c_response bmp180::i2c_start(const i2c_target_socket&,
                               tlm::tlm_command cmd) {
    if (cmd == tlm::TLM_WRITE_COMMAND)
        write_state_ = WRITE_ADDR;
    return I2C_ACK;
}

i2c_response bmp180::i2c_stop(const i2c_target_socket&) {
    return I2C_ACK;
}

i2c_response bmp180::i2c_read(const i2c_target_socket&, u8& data) {
    data = get_reg(address_++);
    return I2C_ACK;
}

i2c_response bmp180::i2c_write(const i2c_target_socket&, u8 data) {
    switch (write_state_) {
    case WRITE_ADDR:
        address_ = data;
        write_state_ = WRITE_DATA;
        break;
    case WRITE_DATA:
        set_reg(address_, data);
        write_state_ = WRITE_IGNORE;
        break;
    case WRITE_IGNORE:
        log_debug("ignoring burst write 0x%02hhx", data);
        break;
    default:
        VCML_ERROR("unknown write state");
    }

    return I2C_ACK;
}

u16 bmp180::calib_word(size_t idx) const {
    switch (idx) {
    case 0: return (u16)ac1.get();
    case 1: return (u16)ac2.get();
    case 2: return (u16)ac3.get();
    case 3: return ac4.get();
    case 4: return ac5.get();
    case 5: return ac6.get();
    case 6: return (u16)b1.get();
    case 7: return (u16)b2.get();
    case 8: return (u16)mb.get();
    case 9: return (u16)mc.get();
    case 10: return (u16)md.get();
    default:
        VCML_ERROR("invalid calibration word %zu", idx);
    }
}

u8 bmp180::get_reg(u8 addr) {
    if (REG_CALIB_FIRST <= addr && addr <= REG_CALIB_LAST) {
        u8 offset = addr - REG_CALIB_FIRST;
        u16 word = calib_word(offset / 2);
        return (offset % 2 == 0) ? (u8)(word >> 8) : (u8)word;
    }

    switch (addr) {
    case REG_ID:
        return id_;
    case REG_CTRL_MEAS:
        return ctrl_meas_ | (converting_ ? CTRL_MEAS_SCO : 0x00u);
    case REG_OUT_MSB:
        return out_msb_;
    case REG_OUT_LSB:
        return out_lsb_;
    case REG_OUT_XLSB:
        return out_xlsb_;
    default:
        return 0;
    }
}

void bmp180::set_reg(u8 addr, u8 data) {
    switch (addr) {
    case REG_SOFT_RESET:
        if (data == SOFT_RESET_CMD)
            reset();
        break;
    case REG_CTRL_MEAS:
        ctrl_meas_ = data & ~(CTRL_MEAS_SCO | CTRL_MEAS_UNUSED);
        if (converting_)
            log_warn("ctrl_meas 0x%02hhx written during conversion", data);
        else if (data & CTRL_MEAS_SCO)
            start_conversion();
        break;
    default:
        break;
    }
}

void bmp180::measure_method() {
    u8 mc = ctrl_meas_ & CTRL_MEAS_MC;
    u8 oss = (ctrl_meas_ & CTRL_MEAS_OSS) >> 6;
    i64 samples = 1 << samples_oss_;

    i64 sum = 0;
    switch (mc & (CTRL_MEAS_MC_SEL_TEMPERATURE | CTRL_MEAS_MC_SEL_PRESSURE)) {
    case CTRL_MEAS_MC_SEL_TEMPERATURE:
        for (i64 i = 0; i < samples; i++) {
            double temp = temperature_c;
            if (temperature_noise) {
                temp += std::normal_distribution<double>(
                    0.0, TEMPERATURE_NOISE_C)(rng_);
            }
            sum += (i64)calculate_ut(temp) - ADC_MID_SCALE;
        }
        break;
    case CTRL_MEAS_MC_SEL_PRESSURE: {
        double press = pressure_hpa;
        if (pressure_noise) {
            double sigma = PRESSURE_NOISE_HPA[samples_oss_];
            press += std::normal_distribution<double>(0.0, sigma)(rng_);
        }

        i32 b5 = compensate_b5(calculate_ut(temperature_c));
        u32 up = calculate_up(press, b5, samples_oss_);
        sum = (i64)up - samples * ADC_MID_SCALE;
        break;
    }
    case 0:
        sum = -samples * ADC_MID_SCALE;
        break;
    default:
        sum = 0;
        break;
    }

    u32 raw = (u32)(OUT_MID_SCALE + ((sum * 256) >> oss)) & 0xffffffu;
    out_msb_ = (u8)(raw >> 16);
    out_lsb_ = (u8)(raw >> 8);
    out_xlsb_ = (u8)raw;

    converting_ = false;
}

void bmp180::start_conversion() {
    u8 mc = ctrl_meas_ & CTRL_MEAS_MC;
    if (mc != (CTRL_MEAS_MC_TEMPERATURE & ~CTRL_MEAS_UNUSED) &&
        mc != (CTRL_MEAS_MC_PRESSURE & ~CTRL_MEAS_UNUSED))
        log_warn("unsupported measurement control 0x%02hhx", ctrl_meas_);

    samples_oss_ = (ctrl_meas_ & CTRL_MEAS_OSS) >> 6;
    converting_ = true;
    measure_event_.notify(conversion_time_ms[samples_oss_], SC_MS);
}

i32 bmp180::compensate_b5(i32 ut) const {
    i32 x1 = (i32)(((i64)ut - ac6) * ac5 >> 15);
    i32 x2 = ((i32)mc << 11) / (x1 + md);
    return x1 + x2;
}

i32 bmp180::compensate_temperature(i32 ut) const {
    return (compensate_b5(ut) + 8) >> 4;
}

i32 bmp180::compensate_b3(i32 b5, u8 oss) const {
    i32 b6 = b5 - 4000;
    i32 x1 = (b2 * ((b6 * b6) >> 12)) >> 11;
    i32 x2 = (ac2 * b6) >> 11;
    i32 x3 = x1 + x2;
    return (((ac1 * 4 + x3) << oss) + 2) >> 2;
}

i32 bmp180::compensate_pressure(i32 up, i32 b5, u8 oss) const {
    i32 b6 = b5 - 4000;
    i32 b3 = compensate_b3(b5, oss);
    i32 x1 = (ac3 * b6) >> 13;
    i32 x2 = (b1 * ((b6 * b6) >> 12)) >> 16;
    i32 x3 = ((x1 + x2) + 2) >> 2;
    u32 b4 = (ac4 * (u32)(x3 + 32768)) >> 15;
    u32 b7 = ((u32)up - b3) * (50000 >> oss);

    i32 p;
    if (b7 < 0x80000000u)
        p = (b7 << 1) / b4;
    else
        p = (b7 / b4) << 1;

    x1 = (p >> 8) * (p >> 8);
    x1 = (x1 * 3038) >> 16;
    x2 = (-7357 * p) >> 16;
    return p + ((x1 + x2 + 3791) >> 4);
}

u16 bmp180::calculate_ut(double temp) {
    double b5 = temp * 160.0;
    double p = b5 + md;
    double disc = std::max(p * p - 4.0 * mc * (1 << 11), 0.0);
    double x1 = (p + std::sqrt(disc)) / 2.0 - md;
    double ut = x1 * (1 << 15) / ac5 + ac6;
    i32 guess = (i32)std::clamp(std::round(ut), 0.0, 65535.0);

    i32 target = (i32)std::round(b5);
    i32 best = guess;
    for (i32 cand = std::max(guess - 4, 0); cand <= std::min(guess + 4, 65535);
         cand++) {
        if (std::abs(compensate_b5(cand) - target) <
            std::abs(compensate_b5(best) - target))
            best = cand;
    }

    return (u16)best;
}

u32 bmp180::calculate_up(double press, i32 b5, u8 oss) {
    i32 target = (i32)std::round(press * 100.0);
    i32 lo = std::max(compensate_b3(b5, oss), 0);
    i32 hi = (1 << (16 + oss)) - 1;
    if (lo > hi)
        return hi;

    while (lo < hi) {
        i32 mid = lo + (hi - lo) / 2;
        if (compensate_pressure(mid, b5, oss) < target)
            lo = mid + 1;
        else
            hi = mid;
    }

    i32 b3 = std::max(compensate_b3(b5, oss), 0);
    if (lo > b3 && std::abs(compensate_pressure(lo - 1, b5, oss) - target) <
                       std::abs(compensate_pressure(lo, b5, oss) - target))
        lo--;

    return (u32)lo;
}

VCML_EXPORT_MODEL(vcml::i2c::bmp180, name, args) {
    (void) args;
    return new bmp180(name);
}

} // namespace i2c
} // namespace vcml
