/******************************************************************************
 * Apache License, Version 2.0
 * Copyright (c) 2026 chciken/Niko Zurstraßen
 *****************************************************************************/

#ifndef VCML_MODELS_BMP180_H
#define VCML_MODELS_BMP180_H

#include <algorithm>
#include <cmath>
#include <random>

#include "vcml/core/model.h"
#include "vcml/core/component.h"
#include "vcml/core/systemc.h"
#include "vcml/core/types.h"
#include "vcml/protocols/i2c.h"

namespace vcml {
namespace i2c {

struct bmp180 : public vcml::component, i2c_host {
    bmp180(const sc_core::sc_module_name& nm);
    ~bmp180() = default;
    VCML_KIND(i2c::bmp180);

    i2c_target_socket in;

    property<double> pressure_hpa;
    property<bool> pressure_noise;
    property<double> temperature_c;
    property<bool> temperature_noise;
    property<double, 4> conversion_time_ms;

    property<i16> ac1;
    property<i16> ac2;
    property<i16> ac3;
    property<u16> ac4;
    property<u16> ac5;
    property<u16> ac6;
    property<i16> b1;
    property<i16> b2;
    property<i16> mb;
    property<i16> mc;
    property<i16> md;

    virtual void reset() override;

    i32 compensate_b5(i32 ut) const;
    i32 compensate_b3(i32 b5, u8 oss) const;
    i32 compensate_temperature(i32 ut) const;          // 0.1 °C
    i32 compensate_pressure(i32 up, i32 b5, u8 oss) const; // Pa

protected:
    SC_HAS_PROCESS(bmp180);

    virtual i2c_response i2c_start(const i2c_target_socket&,
                                   tlm::tlm_command) override;
    virtual i2c_response i2c_stop(const i2c_target_socket&) override;
    virtual i2c_response i2c_read(const i2c_target_socket&, u8& data) override;
    virtual i2c_response i2c_write(const i2c_target_socket&, u8 data) override;

    u8 get_reg(u8 addr);
    void set_reg(u8 addr, u8 data);

    void measure_method();
    sc_event measure_event_;

    void start_conversion();

    u16 calculate_ut(double temp);
    u32 calculate_up(double press, i32 b5, u8 oss);

private:
    u8 address_;

    u8 out_xlsb_;
    u8 out_lsb_;
    u8 out_msb_;
    u8 ctrl_meas_;
    u8 id_;

    bool converting_;
    u8 samples_oss_;
    enum write_state {WRITE_ADDR = 0, WRITE_DATA, WRITE_IGNORE} write_state_;

    std::mt19937 rng_;

    u16 calib_word(size_t idx) const;
};

} // namespace i2c
} // namespace vcml

#endif
