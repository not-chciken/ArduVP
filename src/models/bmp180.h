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

// Open question:
// What happens if a measurement is started while one is ongoing?
//   -> currently restarts the conversion with the new settings.
// What happens if the control register neither uses temperature (0x34) nor pressure (0x2e)?
//   -> currently no conversion is started, a warning is logged and sco is cleared.
// What happens if the oversampling bits are changed during measurement?
//   -> currently the settings latched at the start of the conversion are used.
// Does a reset stop any ongoing measurements?
//   -> currently yes.
// What happens if there is a write into a non-existing/read-only register?
//   -> currently ignored.
// What is read from a non-existing register or from the write-only soft reset register?
//   -> currently 0x00.
// Does the register address auto-increment on burst writes (it does on reads)?
//   -> currently yes.
// What does out_xlsb contain after a temperature conversion?
//   -> currently 0x00.
// What do the data registers contain during a conversion?
//   -> currently the result of the previous conversion.
// What is the RMS noise of the temperature measurement (not in datasheet)?
//   -> currently 0.1 °C, i.e. the output resolution.

namespace vcml {
namespace i2c {

struct bmp180 : public vcml::component, i2c_host {
    bmp180(const sc_core::sc_module_name& nm);
    ~bmp180() = default;
    VCML_KIND(i2c::bmp180);

    i2c_target_socket in;

    property<double> pressure;    // hPa
    property<bool> pressure_noise;
    property<double> temperature; // °C
    property<bool> temperature_noise;

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

    void trigger_measure();

    u16 calculate_ut(double temp);
    u32 calculate_up(double press, i32 b5, u8 oss);

private:
    u8 address_;

    u8 out_xlsb_;
    u8 out_lsb_;
    u8 out_msb_;
    u8 ctrl_meas_;
    u8 id_;

    u8 oversampling_ratio_;
    enum measure_type {TEMPERATURE = 0, PRESSURE} measure_type_;
    enum write_state {WRITE_ADDR = 0, WRITE_DATA} write_state_;

    std::mt19937 rng_;

    u16 calib_word(size_t idx) const;
};

} // namespace i2c
} // namespace vcml

#endif
