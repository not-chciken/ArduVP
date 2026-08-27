#include "ssd1306.h"

#include <cstring>

namespace vcml {
namespace i2c {

constexpr uint kSlaveAddress = 0x3cu;
constexpr uint kSlaveAddressAlt = 0x3du;

constexpr u32 kDefaultOnColor = 0x66ccffu;
constexpr u32 kDefaultOffColor = 0x000000u;

ssd1306::ssd1306(const sc_core::sc_module_name& nm)
    : component(nm),
      in("in"),
      alternate_address("alternate_address", false),
      on_color("on_color", kDefaultOnColor),
      off_color("off_color", kDefaultOffColor),
      m_console() {
  in.set_address(alternate_address ? kSlaveAddressAlt : kSlaveAddress);
  clk.stub();
  reset();
}

void ssd1306::end_of_elaboration() {
  m_console.setup(ui::videomode(PIXEL_FORMAT, kWidth, kHeight), m_video);
}

void ssd1306::end_of_simulation() {
  m_console.shutdown();
}

bool ssd1306::is_display_on() const {
  return m_display_on;
}

bool ssd1306::is_inverse() const {
  return m_inverse;
}

u8 ssd1306::get_contrast() const {
  return m_contrast;
}

bool ssd1306::read_pixel(u32 x, u32 y) const {
  return (m_console.read_pixel(x, y) != kDefaultOffColor);
}

void ssd1306::reset() {
  memset(m_gddram, 0, sizeof(m_gddram));
  fill_all_pixels(off_color.get());

  m_await_control = true;
  m_stream_data = false;
  m_data_mode = false;

  m_cmd_opcode = 0;
  m_cmd_args_needed = 0;
  m_cmd_args_have = 0;

  m_addr_mode = addressing_mode::PAGE;
  m_col = 0;
  m_page = 0;
  m_col_start = 0;
  m_col_end = kWidth - 1;
  m_page_start = 0;
  m_page_end = kPages - 1;
  m_page_mode_col_start = 0;
  m_contrast = 0x7f;

  m_display_on = false;
  m_inverse = false;
  m_entire_display_on = false;
  m_segment_remap = false;
  m_com_scan_remap = false;

  m_console.render();
}

i2c_response ssd1306::i2c_start(const i2c_target_socket&, tlm::tlm_command) {
  m_await_control = true;
  m_stream_data = false;
  m_cmd_args_needed = 0;
  m_cmd_args_have = 0;
  return I2C_ACK;
}

i2c_response ssd1306::i2c_stop(const i2c_target_socket&) {
  m_console.render();
  return I2C_ACK;
}

i2c_response ssd1306::i2c_read(const i2c_target_socket&, u8& data) {
  data = m_display_on ? 0x00 : 0x40;
  return I2C_ACK;
}

i2c_response ssd1306::i2c_write(const i2c_target_socket&, u8 data) {
  if (m_await_control) {
    handle_control_byte(data);
    return I2C_ACK;
  }

  if (m_data_mode)
    write_gddram(data);
  else
    handle_command_byte(data);

  if (!m_stream_data)
    m_await_control = true;

  return I2C_ACK;
}

void ssd1306::handle_control_byte(u8 byte) {
  bool continuation = byte & 0x80;
  m_data_mode = byte & 0x40;

  m_await_control = false;
  m_stream_data = !continuation;
}

void ssd1306::handle_command_byte(u8 byte) {
  if (m_cmd_args_needed > 0) {
    m_cmd_args[m_cmd_args_have++] = byte;
    if (m_cmd_args_have == m_cmd_args_needed) {
      execute_command(m_cmd_opcode, m_cmd_args);
      m_cmd_args_needed = 0;
      m_cmd_args_have = 0;
    }
    return;
  }

  m_cmd_opcode = byte;
  m_cmd_args_have = 0;

  switch (byte) {
  case 0x20:  // set memory addressing mode
  case 0x81:  // set contrast control
  case 0x8d:  // charge pump (not in this datasheet revision, but ubiquitous)
  case 0xa8:  // set multiplex ratio
  case 0xd3:  // set display offset
  case 0xd5:  // set display clock divide ratio / oscillator frequency
  case 0xd9:  // set pre-charge period
  case 0xda:  // set COM pins hardware configuration
  case 0xdb:  // set VCOMH deselect level
    m_cmd_args_needed = 1;
    return;
  case 0x21:  // set column address
  case 0x22:  // set page address
  case 0xa3:  // set vertical scroll area
    m_cmd_args_needed = 2;
    return;
  case 0x29:  // continuous vertical and horizontal scroll setup
  case 0x2a:
    m_cmd_args_needed = 5;
    return;
  case 0x26:  // horizontal scroll setup
  case 0x27:
    m_cmd_args_needed = 6;
    return;
  default:
    execute_command(byte, nullptr);
    return;
  }
}

void ssd1306::execute_command(u8 opcode, const u8* args) {
  if (opcode <= 0x0f) {
    m_col = (m_col & 0xf0) | (opcode & 0x0f);
    m_page_mode_col_start = m_col;
    return;
  }

  if (opcode >= 0x10 && opcode <= 0x1f) {
    m_col = (m_col & 0x0f) | ((opcode & 0x0f) << 4);
    m_page_mode_col_start = m_col;
    return;
  }

  if (opcode >= 0x40 && opcode <= 0x7f)
    return;

  if (opcode >= 0xb0 && opcode <= 0xb7) {
    m_page = opcode & 0x07;
    return;
  }

  switch (opcode) {
  case 0x20:
    switch (args[0] & 0x3) {
    case 0x0:
      m_addr_mode = addressing_mode::HORIZONTAL;
      break;
    case 0x1:
      m_addr_mode = addressing_mode::VERTICAL;
      break;
    case 0x2:
      m_addr_mode = addressing_mode::PAGE;
      break;
    default:
      log_warn("invalid memory addressing mode 0x%x", args[0] & 0x3);
      break;
    }
    break;
  case 0x21:
    m_col_start = args[0] & 0x7f;
    m_col_end = args[1] & 0x7f;
    m_col = m_col_start;
    break;
  case 0x22:
    m_page_start = args[0] & 0x7;
    m_page_end = args[1] & 0x7;
    m_page = m_page_start;
    break;
  case 0x81:
    m_contrast = args[0];
    break;
  case 0xa0:
    m_segment_remap = false;
    break;
  case 0xa1:
    m_segment_remap = true;
    break;
  case 0xa4:
    m_entire_display_on = false;
    redraw();
    break;
  case 0xa5:
    m_entire_display_on = true;
    redraw();
    break;
  case 0xa6:
    m_inverse = false;
    redraw();
    break;
  case 0xa7:
    m_inverse = true;
    redraw();
    break;
  case 0xae:
    m_display_on = false;
    redraw();
    break;
  case 0xaf:
    m_display_on = true;
    redraw();
    break;
  case 0xc0:
    m_com_scan_remap = false;
    break;
  case 0xc8:
    m_com_scan_remap = true;
    break;
  default:
    log_warn("executing unknown command with opcode %hhu", opcode);
    break;
  }
}

void ssd1306::write_gddram(u8 byte) {
  m_gddram[m_page][m_col] = byte;
  redraw_column(m_page, m_col);
  advance_pointer();
}

void ssd1306::advance_pointer() {
  switch (m_addr_mode) {
  case addressing_mode::PAGE:
    if (++m_col > kWidth - 1)
      m_col = m_page_mode_col_start;
    break;

  case addressing_mode::HORIZONTAL:
    if (++m_col > m_col_end) {
      m_col = m_col_start;
      if (++m_page > m_page_end)
        m_page = m_page_start;
    }
    break;

  case addressing_mode::VERTICAL:
    if (++m_page > m_page_end) {
      m_page = m_page_start;
      if (++m_col > m_col_end)
        m_col = m_col_start;
    }
    break;
  }
}

void ssd1306::set_pixel(u32 row, u32 col, bool on) {
  u32 color = on ? on_color : off_color;
  u8* p = &m_video[(row * kWidth + col) * 3];
  p[0] = mwr::extract(color, 16, 8);
  p[1] = mwr::extract(color, 8, 8);
  p[2] = mwr::extract(color, 0, 8);
}

void ssd1306::fill_all_pixels(u32 color) {
  u8 r = mwr::extract(color, 16, 8);
  u8 g = mwr::extract(color, 8, 8);
  u8 b = mwr::extract(color, 0, 8);

  for (size_t i = 0; i < kWidth * kHeight; i++) {
    m_video[i * 3 + 0] = r;
    m_video[i * 3 + 1] = g;
    m_video[i * 3 + 2] = b;
  }
}

void ssd1306::redraw_column(uint page, uint col) {
  u8 byte = m_gddram[page][col];
  u32 out_col = m_segment_remap ? (kWidth - 1 - col) : col;

  for (uint bit = 0; bit < 8; bit++) {
    bool set = (byte >> bit) & 1;
    bool on_logical = m_entire_display_on || (set != m_inverse);
    bool on = m_display_on && on_logical;

    u32 row = page * 8 + bit;
    u32 out_row = m_com_scan_remap ? (kHeight - 1 - row) : row;
    set_pixel(out_row, out_col, on);
  }
}

void ssd1306::redraw() {
  for (uint page = 0; page < kPages; page++)
    for (uint col = 0; col < kWidth; col++)
      redraw_column(page, col);
  m_console.render();
}

VCML_EXPORT_MODEL(vcml::i2c::ssd1306, name, args) {
  return new ssd1306(name);
}

}  // namespace i2c
}  // namespace vcml
