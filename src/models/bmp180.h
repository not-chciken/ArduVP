#ifndef VCML_MODELS_BMP180_H
#define VCML_MODELS_BMP180_H

#include "vcml/core/model.h"
#include "vcml/core/component.h"
#include "vcml/core/systemc.h"
#include "vcml/core/types.h"
#include "vcml/protocols/i2c.h"

// Open question:
// What happens if a measurement is started while one is ongoing?
// What happens if the control register neither uses temperature (0x34) nor pressure (0x2e)?
// What happens if the oversampling bits are changed during measurement?
// Does a reset stop any ongoing measurements?
// What happens if there is a write into a non-existing/read-only register?

namespace vcml {
namespace i2c {

struct bmp180 : public vcml::component, i2c_host {
    bmp180(const sc_core::sc_module_name& nm);
    ~bmp180() = default;
    VCML_KIND(i2c::bmp180);

    i2c_target_socket in;

    property<double> pressure; // TODO: Unit?
    property<bool> pressure_noise;
    property<double> temperature;
    property<bool> temperature_noise;

    property<i16> ac1;
    property<i16> ac2;
    property<i16> ac3;
    property<i16> ac4;
    property<i16> ac5;
    property<i16> ac6;
    property<i16> b1;
    property<i16> b2;
    property<i16> mb;
    property<i16> mc;
    property<i16> md;

    virtual void reset();

protected:
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

    u32 calculate_ut();

private:
    size_t bytes_to_read_;
    u8 address_;
    i16 eeprom_[16];

    u8 out_xlsb_;
    u8 out_lsb_;
    u8 out_msb_;
    u8 ctrl_meas_;
    u8 soft_reset_;
    u8 id_;

    u8 oversampling_ratio_;
    enum measure_type {TEMPERATURE = 0, PRESSURE} measure_type_;
    enum write_state {WRITE_ADDR = 0, WRITE_DATA} write_state_;

};

} // namespace i2c
} // namespace vcml

#endif
