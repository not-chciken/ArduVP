#include "usart.h"

#include <algorithm>

static constexpr u8 UCSRNA_MPCMN = mwr::bit(0);
static constexpr u8 UCSRNA_U2XN = mwr::bit(1);
static constexpr u8 UCSRNA_UPEN = mwr::bit(2);
static constexpr u8 UCSRNA_DORN = mwr::bit(3);
static constexpr u8 UCSRNA_FEN = mwr::bit(4);
static constexpr u8 UCSRNA_UDREN = mwr::bit(5);
static constexpr u8 UCSRNA_TXCN = mwr::bit(6);
static constexpr u8 UCSRNA_RXCN = mwr::bit(7);

static constexpr u8 UCSRNB_TXB8N = mwr::bit(0);
static constexpr u8 UCSRNB_RXB8N = mwr::bit(1);
static constexpr u8 UCSRNB_UCSZN2 = mwr::bit(2);
static constexpr u8 UCSRNB_TXENN = mwr::bit(3);
static constexpr u8 UCSRNB_RXENN = mwr::bit(4);
static constexpr u8 UCSRNB_UDRIEN = mwr::bit(5);
static constexpr u8 UCSRNB_TXCIEN = mwr::bit(6);
static constexpr u8 UCSRNB_RXCIEN = mwr::bit(7);

static constexpr u8 UCSRNC_UCPOLN = mwr::bit(0);
static constexpr u8 UCSRNC_UCSZ0N = mwr::bit(1);
static constexpr u8 UCSRNC_UCSZN1 = mwr::bit(2);
static constexpr u8 UCSRNC_USBSN = mwr::bit(3);
static constexpr u8 UCSRNC_UPM0N = mwr::bit(4);
static constexpr u8 UCSRNC_UPMN1 = mwr::bit(5);
static constexpr u8 UCSRNC_UMSEL0N = mwr::bit(6);
static constexpr u8 UCSRNC_UMSELN1 = mwr::bit(7);

Usart::Usart(const sc_core::sc_module_name& nm)
    : peripheral(nm),
      serial_host(),
      ucsrna("ucsrna", 0x0, UCSRNA_UDREN),
      ucsrnb("ucsrnb", 0x1, 0x00),
      ucsrnc("ucsrnc", 0x2, UCSRNC_UCSZ0N | UCSRNC_UCSZN1),
      ubrrnl("ubrrnl", 0x4, 0x00),
      ubrrnh("ubrrnh", 0x5, 0x00),
      udrn("udrn", 0x6, 0x00),
      in("in"),
      serial_tx("serial_tx"),
      serial_rx("serial_rx"),
      irq_rx("irq_rx"),
      irq_tx("irq_tx"),
      irq_udre("irq_udre"),
      rxc_ack("rxc_ack"),
      udre_ack("udre_ack"),
      tx_ack("tx_ack"),
      untimed("untimed", false),
      tx_ev_("tx_ev"),
      rx_pending_(false),
      rx_data_(0) {
  set_little_endian();

  ucsrna.sync_always();
  ucsrna.allow_read_write();
  ucsrna.on_read(&Usart::ReadUcsrna);
  ucsrna.on_write(&Usart::WriteUcsrna);

  ucsrnb.sync_always();
  ucsrnb.allow_read_write();
  ucsrnb.on_write(&Usart::WriteUcsrnb);

  ucsrnc.sync_always();
  ucsrnc.allow_read_write();
  ucsrnc.on_write(&Usart::WriteUcsrnc);

  ubrrnl.sync_always();
  ubrrnl.allow_read_write();
  ubrrnl.on_write(&Usart::WriteUbrrnl);

  ubrrnh.sync_always();
  ubrrnh.allow_read_write();
  ubrrnh.on_write(&Usart::WriteUbrrnh);

  udrn.sync_always();
  udrn.allow_read_write();
  udrn.on_read(&Usart::ReadUdrn);
  udrn.on_write(&Usart::WriteUdrn);

  UpdateSerialFormat();
  UpdateSerialBaud();

  SC_THREAD(TxThread);
}

void Usart::reset() {
  peripheral::reset();

  rx_pending_ = false;
  rx_data_ = 0;

  irq_rx = false;
  irq_tx = false;
  irq_udre = false;

  UpdateSerialFormat();
  UpdateSerialBaud();
}

bool Usart::RxEnabled() const {
  return (ucsrnb & UCSRNB_RXENN) != 0;
}

bool Usart::TxEnabled() const {
  return (ucsrnb & UCSRNB_TXENN) != 0;
}

u16 Usart::Ubrr() const {
  return (u16)((u16)(ubrrnh & 0x0f) << 8) | ubrrnl;
}

void Usart::SetRxComplete(bool set) {
  if (set)
    ucsrna = (u8)(ucsrna | UCSRNA_RXCN);
  else
    ucsrna = (u8)(ucsrna & ~UCSRNA_RXCN);
}

void Usart::SetTxComplete(bool set) {
  if (set) {
    ucsrna = (u8)(ucsrna | UCSRNA_TXCN);
    if (ucsrnb & UCSRNB_TXCIEN)
      irq_tx = true;
  } else {
    ucsrna = (u8)(ucsrna & ~UCSRNA_TXCN);
    irq_tx = false;
  }
}

void Usart::SetUdre(bool set) {
  if (set) {
    ucsrna = (u8)(ucsrna | UCSRNA_UDREN);
    if (ucsrnb & UCSRNB_RXCIEN)
      irq_rx = true;
    if (ucsrna & UCSRNB_UDRIEN)
      irq_udre = true;
  } else {
    ucsrna = (u8)(ucsrna & ~UCSRNA_UDREN);
  }
}

void Usart::gpio_notify(const vcml::gpio_target_socket& socket, bool state) {
  if (!state)
    return;

  if (socket == tx_ack)
    SetTxComplete(false);
}

void Usart::UpdateSerialBaud() {
  u32 divisor = (ucsrna & UCSRNA_U2XN) ? 8u : 16u;
  u16 n = (u16)(Ubrr() + 1u);
  mwr::hz_t fosc = clock_hz();
  mwr::hz_t baud = (n == 0 || divisor == 0) ? vcml::SERIAL_9600BD : (fosc / (divisor * n));

  serial_tx.set_baud(std::max<mwr::hz_t>(baud, 1));
}

void Usart::UpdateSerialFormat() {
  u8 umsel = (u8)((ucsrnc & (UCSRNC_UMSEL0N | UCSRNC_UMSELN1)) >> 6);
  (void)umsel;

  u8 upm = (u8)((ucsrnc & (UCSRNC_UPM0N | UCSRNC_UPMN1)) >> 4);
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

  bool two_stop = (ucsrnc & UCSRNC_USBSN) != 0;
  vcml::serial_stop stop = two_stop ? vcml::SERIAL_STOP_2 : vcml::SERIAL_STOP_1;

  u8 ucsz = (u8)(((ucsrnb & UCSRNB_UCSZN2) ? 1u : 0u) << 2) | (u8)((ucsrnc & (UCSRNC_UCSZ0N | UCSRNC_UCSZN1)) >> 1);

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
    break;
  }

  serial_tx.set_parity(parity);
  serial_tx.set_stop_bits(stop);
  serial_tx.set_data_width(bits);
}

u8 Usart::ReadUcsrna() {
  if (rx_pending_)
    ucsrna = (u8)(ucsrna | UCSRNA_RXCN);
  else
    ucsrna = (u8)(ucsrna & ~UCSRNA_RXCN);
  return ucsrna;
}

void Usart::WriteUcsrna(u8 val) {
  u8 keep_ro = (u8)(ucsrna & (UCSRNA_UPEN | UCSRNA_DORN | UCSRNA_FEN | UCSRNA_UDREN | UCSRNA_RXCN));
  u8 new_rw = (u8)(val & (UCSRNA_MPCMN | UCSRNA_U2XN));
  ucsrna = (u8)(keep_ro | new_rw | (ucsrna & UCSRNA_TXCN));

  if (val & UCSRNA_TXCN)
    SetTxComplete(false);

  UpdateSerialBaud();
}

void Usart::WriteUcsrnb(u8 val) {
  ucsrnb = val;
  UpdateSerialFormat();
}

void Usart::WriteUcsrnc(u8 val) {
  ucsrnc = val;
  UpdateSerialFormat();
}

void Usart::WriteUbrrnl(u8 val) {
  ubrrnl = val;
  UpdateSerialBaud();
}

void Usart::WriteUbrrnh(u8 val) {
  ubrrnh = (u8)(val & 0x0f);
  UpdateSerialBaud();
}

u8 Usart::ReadUdrn(bool dbg) {
  if (dbg) {
    return rx_data_;
  }

  if (!rx_pending_) {
    return udrn;
  }

  const u8 val = rx_data_;
  udrn = val;
  rx_pending_ = false;
  SetRxComplete(false);
  ucsrna = (u8)(ucsrna & ~(UCSRNA_FEN | UCSRNA_DORN | UCSRNA_UPEN));
  return val;
}

void Usart::WriteUdrn(u8 val) {
  if (!(ucsrna & UCSRNA_UDREN))
    return;

  udrn = val;

  if (!TxEnabled()) {
    return;
  }

  SetUdre(false);
  SetTxComplete(false);
  tx_ev_.notify(sc_core::SC_ZERO_TIME);
}

void Usart::TxThread() {
  while (true) {
    wait(tx_ev_);

    if (!TxEnabled()) {
      SetUdre(true);
      continue;
    }

    const u8 data = udrn;
    serial_tx.send(data);

    if (!untimed)
      wait(serial_tx.cycle());

    SetUdre(true);
    SetTxComplete(true);
  }
}

void Usart::serial_receive(const vcml::serial_target_socket& socket, vcml::serial_payload& tx) {
  if (!RxEnabled())
    return;

  if (rx_pending_) {
    ucsrna = (u8)(ucsrna | UCSRNA_DORN);
    return;
  }

  if (tx.parity != serial_tx.parity()) {
    log_error("USART parity mismatch");
  }

  vcml::serial_parity par = serial_tx.parity();

  if (par == vcml::SERIAL_PARITY_ODD || par == vcml::SERIAL_PARITY_EVEN) {
    vcml::serial_payload check = tx;
    if (!vcml::serial_test_parity(check)) {
      ucsrna = (u8)(ucsrna | UCSRNA_UPEN);
    } else {
      ucsrna &= ~UCSRNA_UPEN;
    }
  }

  rx_pending_ = true;
  rx_data_ = (u8)(tx.data & vcml::serial_mask(serial_tx.data_width()));
  SetRxComplete(true);
}
