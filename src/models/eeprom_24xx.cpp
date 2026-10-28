/******************************************************************************
 * Apache License, Version 2.0
 * Copyright (c) 2026 chciken/Niko Zurstraßen
 *****************************************************************************/

#include "eeprom_24xx.h"

namespace vcml {
namespace i2c {

eeprom_24xx::eeprom_24xx(const sc_core::sc_module_name& nm, u8 a_pins_,
                         u64 size_, u64 buffer_size_):
    memory(nm, size_),
    i2c_in("i2c_in"),
    a_pins("a_pins", a_pins_),
    wp("wp", false),
    buffer_size("buffer_size", buffer_size_),
    m_state(STATE_ADDR_MSB),
    m_data_ptr(0),
    m_write_event("write_event"),
    m_write_buffer(buffer_size.get()),
    m_buffer_count(0) {
    VCML_ERROR_ON(buffer_size == 0u, "buffer size cannot be 0");

    u8 address = 0b1010000 | (a_pins.get() & 0x7u);
    i2c_in.set_address(address);
    in.stub();
    clk.stub();

    if (shared.get().empty())
        memset(data(), ERASED, size.get());

    SC_THREAD(write_thread);
}

void eeprom_24xx::start_of_simulation() {
    memory::start_of_simulation();
    load_images(images);
}

void eeprom_24xx::reset() {
    m_state = STATE_ADDR_MSB;
    m_data_ptr = 0;
    m_buffer_count = 0;
    m_write_event.cancel();
    memset(m_write_buffer.data(), 0, m_write_buffer.size());
}

i2c_response eeprom_24xx::i2c_start(const i2c_target_socket&,
                                    tlm::tlm_command) {
    if (m_state == STATE_WRITING)
        return I2C_NACK;

    m_buffer_count = 0;
    m_state = STATE_ADDR_MSB;
    return I2C_ACK;
}

i2c_response eeprom_24xx::i2c_stop(const i2c_target_socket&) {
    if (m_state == STATE_WRITING)
        return I2C_NACK;

    if (m_buffer_count > 0 && !wp.get()) {
        m_state = STATE_WRITING;
        m_write_event.notify(sc_core::sc_time(WRITE_CYCLE_MS, sc_core::SC_MS));
    } else {
        if (m_buffer_count > 0)
            log_warn("write to %04hx ignored", m_data_ptr);

        m_buffer_count = 0;
        m_state = STATE_ADDR_MSB;
    }

    return I2C_ACK;
}

i2c_response eeprom_24xx::i2c_read(const i2c_target_socket&, u8& data) {
    if (m_state == STATE_WRITING)
        return I2C_NACK;

    data = (*this)[m_data_ptr];
    m_data_ptr = (m_data_ptr + 1) % size.get();
    return I2C_ACK;
}

i2c_response eeprom_24xx::i2c_write(const i2c_target_socket&, u8 data) {
    if (m_state == STATE_WRITING)
        return I2C_NACK;

    switch (m_state) {
    case STATE_ADDR_MSB:
        m_data_ptr = (u16)data << 8;
        m_state = STATE_ADDR_LSB;
        break;
    case STATE_ADDR_LSB:
        m_data_ptr = (m_data_ptr | (u16)data) % size.get();
        m_state = STATE_DATA;
        break;
    case STATE_DATA:
        if (m_buffer_count == buffer_size.get())
            log_warn("page buffer wraparound");
        m_write_buffer[(m_data_ptr + m_buffer_count) %
                       buffer_size.get()] = data;
        m_buffer_count++;
        break;
    default:
        log_error("unknown write state");
        return I2C_NACK;
    }

    return I2C_ACK;
}

void eeprom_24xx::write_thread() {
    while (true) {
        wait(m_write_event);

        size_t bufsz = buffer_size.get();
        size_t count = min(m_buffer_count, bufsz);
        u16 page_base = m_data_ptr / bufsz * bufsz;
        for (size_t i = 0; i < count; i++) {
            size_t offset = (m_data_ptr + i) % bufsz;
            (*this)[page_base + offset] = m_write_buffer[offset];
        }

        m_data_ptr = page_base + (m_data_ptr + count) % bufsz;
        m_buffer_count = 0;
        m_state = STATE_ADDR_MSB;
    }
}

VCML_EXPORT_MODEL(vcml::i2c::eeprom_24xx, name, args) {
    u8 a_pins = 0;
    u64 size = 32 * KiB;
    u64 buffer_size = 64;
    if (args.size() > 0)
        a_pins = from_string<u8>(args[0]);
    if (args.size() > 1)
        size = from_string<u64>(args[1]);
    if (args.size() > 2)
        buffer_size = from_string<u64>(args[2]);
    return new eeprom_24xx(name, a_pins, size, buffer_size);
}

} // namespace i2c
} // namespace vcml
