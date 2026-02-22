#include "usart.h"

#include <algorithm>
// #include <mwr/types.h>

namespace {

// UCSR0A bits
static constexpr u8 UCSR0A_MPCM0 = 1u << 0;
static constexpr u8 UCSR0A_U2X0 = 1u << 1;
static constexpr u8 UCSR0A_UPE0 = 1u << 2;
static constexpr u8 UCSR0A_DOR0 = 1u << 3;
static constexpr u8 UCSR0A_FE0 = 1u << 4;
static constexpr u8 UCSR0A_UDRE0 = 1u << 5;
static constexpr u8 UCSR0A_TXC0 = 1u << 6;
static constexpr u8 UCSR0A_RXC0 = 1u << 7;

// UCSR0B bits
static constexpr u8 UCSR0B_TXB80 = 1u << 0;
static constexpr u8 UCSR0B_RXB80 = 1u << 1;
static constexpr u8 UCSR0B_UCSZ02 = 1u << 2;
static constexpr u8 UCSR0B_TXEN0 = 1u << 3;
static constexpr u8 UCSR0B_RXEN0 = 1u << 4;
static constexpr u8 UCSR0B_UDRIE0 = 1u << 5;
static constexpr u8 UCSR0B_TXCIE0 = 1u << 6;
static constexpr u8 UCSR0B_RXCIE0 = 1u << 7;

// UCSR0C bits
static constexpr u8 UCSR0C_UCPOL0 = 1u << 0;
static constexpr u8 UCSR0C_UCSZ00 = 1u << 1;
static constexpr u8 UCSR0C_UCSZ01 = 1u << 2;
static constexpr u8 UCSR0C_USBS0 = 1u << 3;
static constexpr u8 UCSR0C_UPM00 = 1u << 4;
static constexpr u8 UCSR0C_UPM01 = 1u << 5;
static constexpr u8 UCSR0C_UMSEL00 = 1u << 6;
static constexpr u8 UCSR0C_UMSEL01 = 1u << 7;

}  // namespace

Usart::Usart(const sc_core::sc_module_name& nm)
    : peripheral(nm),
      serial_host(),
      in("in"),
      serial_tx("serial_tx"),
      serial_rx("serial_rx"),
      ucsr0a("ucsr0a", 0xc0, 0x20),
      ucsr0b("ucsr0b", 0xc1, 0x00),
      ucsr0c("ucsr0c", 0xc2, 0x06),
      ubrr0l("ubrr0l", 0xc4, 0x00),
      ubrr0h("ubrr0h", 0xc5, 0x00),
      udr0("udr0", 0xc6, 0x00),
      tx_ev_("tx_ev"),
      rx_pending_(false),
      rx_data_(0) {
  set_little_endian();

  // UCSR0A: status + U2X0 + MPCM0. TXC0 is cleared by writing 1.
  ucsr0a.sync_always();
  ucsr0a.allow_read_write();
  ucsr0a.on_read(&Usart::read_ucsr0a, this);
  ucsr0a.on_write(&Usart::write_ucsr0a, this);

  // UCSR0B: control bits.
  ucsr0b.sync_always();
  ucsr0b.allow_read_write();
  ucsr0b.on_write(&Usart::write_ucsr0b, this);

  // UCSR0C: frame format.
  ucsr0c.sync_always();
  ucsr0c.allow_read_write();
  ucsr0c.on_write(&Usart::write_ucsr0c, this);

  ubrr0l.sync_always();
  ubrr0l.allow_read_write();
  ubrr0l.on_write(&Usart::write_ubrr0l, this);

  ubrr0h.sync_always();
  ubrr0h.allow_read_write();
  ubrr0h.on_write(&Usart::write_ubrr0h, this);

  udr0.sync_always();
  udr0.allow_read_write();
  udr0.on_read(&Usart::read_udr0, this);
  udr0.on_write(&Usart::write_udr0, this);

  // Default serial settings.
  update_serial_format();
  update_serial_baud();

  SC_THREAD(tx_thread);
}

void Usart::reset() {
  peripheral::reset();

  // Reset values (ATmega328P):
  // UCSR0A: UDRE0=1, TXC0=1 after reset; keep U2X0=0, MPCM0=0.
  ucsr0a = (u8)(UCSR0A_UDRE0 | UCSR0A_TXC0);
  ucsr0b = 0x00;
  ucsr0c = 0x06;  // async, no parity, 1 stop, 8-bit.
  ubrr0l = 0x00;
  ubrr0h = 0x00;
  udr0 = 0x00;

  rx_pending_ = false;
  rx_data_ = 0;

  update_serial_format();
  update_serial_baud();
}

bool Usart::rx_enabled() const {
  return (ucsr0b & UCSR0B_RXEN0) != 0;
}

bool Usart::tx_enabled() const {
  return (ucsr0b & UCSR0B_TXEN0) != 0;
}

u16 Usart::ubrr() const {
  return (u16)((u16)(ubrr0h & 0x0f) << 8) | ubrr0l;
}

void Usart::set_rx_complete(bool set) {
  if (set)
    ucsr0a = (u8)(ucsr0a | UCSR0A_RXC0);
  else
    ucsr0a = (u8)(ucsr0a & ~UCSR0A_RXC0);
}

void Usart::set_tx_complete(bool set) {
  if (set)
    ucsr0a = (u8)(ucsr0a | UCSR0A_TXC0);
  else
    ucsr0a = (u8)(ucsr0a & ~UCSR0A_TXC0);
}

void Usart::set_udre(bool set) {
  if (set)
    ucsr0a = (u8)(ucsr0a | UCSR0A_UDRE0);
  else
    ucsr0a = (u8)(ucsr0a & ~UCSR0A_UDRE0);
}

void Usart::update_serial_baud() {
  // AVR baud rate generation (async normal: fosc/(16*(ubrr+1)), double: /8).
  const u32 divisor = (ucsr0a & UCSR0A_U2X0) ? 8u : 16u;
  const u16 n = (u16)(ubrr() + 1u);
  const mwr::hz_t fosc = clock_hz();
  const mwr::hz_t baud = (n == 0 || divisor == 0) ? vcml::SERIAL_9600BD : (fosc / (divisor * n));

  // VCML sockets take hz_t as baud. Clamp to something sane.
  serial_tx.set_baud(std::max<mwr::hz_t>(baud, 1));
}

void Usart::update_serial_format() {
  // Only asynchronous mode is modeled; sync/MSPI are ignored.
  const u8 umsel = (u8)((ucsr0c & (UCSR0C_UMSEL00 | UCSR0C_UMSEL01)) >> 6);
  (void)umsel;

  const u8 upm = (u8)((ucsr0c & (UCSR0C_UPM00 | UCSR0C_UPM01)) >> 4);
  vcml::serial_parity parity = vcml::SERIAL_PARITY_NONE;
  switch (upm) {
  case 0:
    parity = vcml::SERIAL_PARITY_NONE;
    break;
  case 2:
    parity = vcml::SERIAL_PARITY_EVEN;
    break;
  case 3:
    parity = vcml::SERIAL_PARITY_ODD;
    break;
  default:
    parity = vcml::SERIAL_PARITY_NONE;
    break;
  }

  const bool two_stop = (ucsr0c & UCSR0C_USBS0) != 0;
  vcml::serial_stop stop = two_stop ? vcml::SERIAL_STOP_2 : vcml::SERIAL_STOP_1;

  // Character size (UCSZ02:0)
  const u8 ucsz =
      (u8)(((ucsr0b & UCSR0B_UCSZ02) ? 1u : 0u) << 2) | (u8)((ucsr0c & (UCSR0C_UCSZ00 | UCSR0C_UCSZ01)) >> 1);

  vcml::serial_bits bits = vcml::SERIAL_8_BITS;
  switch (ucsz) {
  case 0:
    bits = vcml::SERIAL_5_BITS;
    break;
  case 1:
    bits = vcml::SERIAL_6_BITS;
    break;
  case 2:
    bits = vcml::SERIAL_7_BITS;
    break;
  case 3:
    bits = vcml::SERIAL_8_BITS;
    break;
  default:
    bits = vcml::SERIAL_8_BITS;
    break;  // 9-bit not modeled
  }

  serial_tx.set_parity(parity);
  serial_tx.set_stop_bits(stop);
  serial_tx.set_data_width(bits);
}

u8 Usart::read_ucsr0a(bool dbg) {
  (void)dbg;
  // Status bits are stored in ucsr0a, but RXC0 must reflect pending RX.
  if (rx_pending_)
    ucsr0a = (u8)(ucsr0a | UCSR0A_RXC0);
  else
    ucsr0a = (u8)(ucsr0a & ~UCSR0A_RXC0);
  return ucsr0a;
}

void Usart::write_ucsr0a(u8 val, bool dbg) {
  (void)dbg;

  // Writable bits: MPCM0, U2X0. TXC0 is cleared by writing a 1.
  const u8 keep_ro = (u8)(ucsr0a & (UCSR0A_UPE0 | UCSR0A_DOR0 | UCSR0A_FE0 | UCSR0A_UDRE0 | UCSR0A_RXC0));
  const u8 new_rw = (u8)(val & (UCSR0A_MPCM0 | UCSR0A_U2X0));
  ucsr0a = (u8)(keep_ro | new_rw | (ucsr0a & UCSR0A_TXC0));

  // Clear TXC0 on write-1.
  if (val & UCSR0A_TXC0)
    set_tx_complete(false);

  update_serial_baud();
}

void Usart::write_ucsr0b(u8 val, [[maybe_unused]] bool dbg) {
  ucsr0b = val;
  update_serial_format();
}

void Usart::write_ucsr0c(u8 val, [[maybe_unused]] bool dbg) {
  (void)dbg;
  update_serial_format();
}

void Usart::write_ubrr0l(u8 val, [[maybe_unused]] bool dbg) {
  ubrr0l = val;
  update_serial_baud();
}

void Usart::write_ubrr0h(u8 val, [[maybe_unused]] bool dbg) {
  // only lower 4 bits exist
  ubrr0h = (u8)(val & 0x0f);
  update_serial_baud();
}

u8 Usart::read_udr0(bool dbg) {
  (void)dbg;
  if (!rx_pending_) {
    // reading with no data returns last value; keep udr0 as-is
    return udr0;
  }

  // Consume the pending RX byte.
  const u8 val = rx_data_;
  udr0 = val;
  rx_pending_ = false;
  set_rx_complete(false);
  // Clear error flags for this simplified model.
  ucsr0a = (u8)(ucsr0a & ~(UCSR0A_FE0 | UCSR0A_DOR0 | UCSR0A_UPE0));
  return val;
}

void Usart::write_udr0(u8 val, bool dbg) {
  (void)dbg;
  udr0 = val;

  if (!tx_enabled()) {
    // If TX disabled, behave as a write to a holding register without output.
    return;
  }

  // Writing UDR0 starts a transmission: buffer becomes non-empty.
  set_udre(false);
  set_tx_complete(false);
  tx_ev_.notify(sc_core::SC_ZERO_TIME);
}

void Usart::tx_thread() {
  while (true) {
    wait(tx_ev_);

    if (!tx_enabled()) {
      // If TX got disabled, consider buffer empty again.
      set_udre(true);
      continue;
    }

    // Send current UDR0 byte.
    const u8 data = udr0;
    serial_tx.send(data);

    // Approximate transmitter timing.
    wait(serial_tx.cycle());

    // Data register empty + transmission complete.
    set_udre(true);
    set_tx_complete(true);
  }
}

void Usart::serial_receive(u8 data) {
  // Incoming byte on serial_rx.
  if (!rx_enabled())
    return;

  if (rx_pending_) {
    // Data overrun (simplified): drop new byte and set DOR0.
    ucsr0a = (u8)(ucsr0a | UCSR0A_DOR0);
    return;
  }

  rx_pending_ = true;
  rx_data_ = data;
  set_rx_complete(true);
}