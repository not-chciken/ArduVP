#include "eeprom_24xx256.h"

namespace vcml {
namespace i2c {

eeprom_24xx256::eeprom_24xx256(const sc_core::sc_module_name& nm, u8 a_pins_)
    : component(nm),
      i2c_in("i2c_in"),
      a_pins("a_pins", a_pins_),
      wp("wp", false),
      m_state(STATE_ADDR_MSB),
      m_data_ptr(0),
      m_write_event("write_event"),
      m_data(new u8[EEPROM_SIZE]),
      m_write_buffer(),
      m_buffer_count(0) {
  u8 address = 0b1010000 | (a_pins.get() & 0x7u);
  i2c_in.set_address(address);
  clk.stub();
  SC_THREAD(write_thread);
  memset(m_data, ERASED, EEPROM_SIZE);
}

eeprom_24xx256::~eeprom_24xx256() {
  delete[] m_data;
}

u8* eeprom_24xx256::get_data() {
  return m_data;
}

void eeprom_24xx256::reset() {
  m_state = STATE_ADDR_MSB;
  m_data_ptr = 0;
  m_buffer_count = 0;
  m_write_event.cancel();
  memset(m_write_buffer, 0, sizeof(m_write_buffer));
}

i2c_response eeprom_24xx256::i2c_start(const i2c_target_socket&, tlm::tlm_command) {
  if (m_state == STATE_WRITING)
    return I2C_NACK;

  m_buffer_count = 0;
  m_state = STATE_ADDR_MSB;
  return I2C_ACK;
}

i2c_response eeprom_24xx256::i2c_stop(const i2c_target_socket&) {
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

i2c_response eeprom_24xx256::i2c_read(const i2c_target_socket&, u8& data) {
  if (m_state == STATE_WRITING)
    return I2C_NACK;

  data = m_data[m_data_ptr];
  m_data_ptr = (m_data_ptr + 1) % EEPROM_SIZE;
  return I2C_ACK;
}

i2c_response eeprom_24xx256::i2c_write(const i2c_target_socket&, u8 data) {
  if (m_state == STATE_WRITING)
    return I2C_NACK;

  switch (m_state) {
  case STATE_ADDR_MSB:
    m_data_ptr = (u16)(data & 0x7f) << 8;
    m_state = STATE_ADDR_LSB;
    break;
  case STATE_ADDR_LSB:
    m_data_ptr |= (u16)data;
    m_state = STATE_DATA;
    break;
  case STATE_DATA:
    if (m_buffer_count == PAGE_SIZE)
      log_warn("page buffer wraparound");
    m_write_buffer[(m_data_ptr + m_buffer_count) % PAGE_SIZE] = data;
    m_buffer_count++;
    break;
  default:
    log_error("unknown write state");
    return I2C_NACK;
  }

  return I2C_ACK;
}

void eeprom_24xx256::write_thread() {
  while (true) {
    wait(m_write_event);

    size_t count = min(m_buffer_count, PAGE_SIZE);
    u16 page_base = m_data_ptr / PAGE_SIZE * PAGE_SIZE;
    for (size_t i = 0; i < count; i++) {
      size_t offset = (m_data_ptr + i) % PAGE_SIZE;
      m_data[page_base + offset] = m_write_buffer[offset];
    }

    m_data_ptr = page_base + (m_data_ptr + count) % PAGE_SIZE;
    m_buffer_count = 0;
    m_state = STATE_ADDR_MSB;
  }
}

VCML_EXPORT_MODEL(vcml::i2c::eeprom_24xx256, name, args) {
    u8 a_pins = 0;
    if (!args.empty())
        a_pins = from_string<u8>(args[0]);
  return new eeprom_24xx256(name, a_pins);
}

}  // namespace i2c
}  // namespace vcml
