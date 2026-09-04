/******************************************************************************
 *                                                                            *
 * Copyright (C) 2026 MachineWare GmbH                                        *
 * All Rights Reserved                                                        *
 *                                                                            *
 * This work is licensed under the terms described in the LICENSE file found  *
 * in the root directory of this source tree.                                 *
 *                                                                            *
 ******************************************************************************/

#ifndef VCML_MODELS_EEPROM_24XX_H
#define VCML_MODELS_EEPROM_24XX_H

#include "vcml/core/component.h"
#include "vcml/core/model.h"
#include "vcml/core/systemc.h"
#include "vcml/core/types.h"
#include "vcml/models/generic/memory.h"
#include "vcml/protocols/i2c.h"
#include "vcml/ui/console.h"

namespace vcml {
namespace i2c {

struct eeprom_24xx : public vcml::generic::memory, i2c_host {
  eeprom_24xx(const sc_core::sc_module_name& nm, u8 a_pins = 0, u64 size = 32 * KiB, u64 buffer_size = 64);

  VCML_KIND(i2c::eeprom_24xx);

  i2c_target_socket i2c_in;

  property<u8> a_pins;
  property<bool> wp;
  property<u64> buffer_size;

  virtual void reset() override;
  virtual void start_of_simulation() override;

 protected:
  static constexpr double WRITE_CYCLE_MS = 5.0;
  static constexpr u8 ERASED = 0xff;

  enum state {
    STATE_ADDR_MSB,
    STATE_ADDR_LSB,
    STATE_DATA,
    STATE_WRITING,
  } m_state;

  u16 m_data_ptr;
  sc_core::sc_event m_write_event;
  vector<u8> m_write_buffer;
  size_t m_buffer_count;

  SC_HAS_PROCESS(eeprom_24xx);

  virtual i2c_response i2c_start(const i2c_target_socket&, tlm::tlm_command) override;
  virtual i2c_response i2c_stop(const i2c_target_socket&) override;
  virtual i2c_response i2c_read(const i2c_target_socket&, u8& data) override;
  virtual i2c_response i2c_write(const i2c_target_socket&, u8 data) override;

  void write_thread();
};

}  // namespace i2c
}  // namespace vcml

#endif
