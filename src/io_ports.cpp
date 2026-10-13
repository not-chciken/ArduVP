#include "io_ports.h"

IoPorts::IoPorts(const sc_core::sc_module_name& nm)
    : vcml::peripheral(nm),
      in("in"),
      out_pinb("out_pinb"),
      out_pinc("out_pinc"),
      out_pind("out_pind"),
      in_pinb("in_pinb"),
      in_pinc("in_pinc"),
      in_pind("in_pind"),
      pinb("pinb", 0x00),
      ddrb("ddrb", 0x01),
      portb("portb", 0x02),
      pinc("pinc", 0x03),
      ddrc("ddrc", 0x04),
      portc("portc", 0x05),
      pind("pind", 0x06),
      ddrd("ddrd", 0x07),
      portd("portd", 0x08) {
  pinb.allow_read_write();
  pinb.on_read(&IoPorts::ReadPinB);
  pinb.on_write(&IoPorts::WritePinB);
  pinb.sync_always();

  ddrb.allow_read_write();
  ddrb.sync_always();

  portb.allow_read_write();
  portb.on_write(&IoPorts::WritePortB);
  portb.sync_always();

  pinc.allow_read_write();
  pinc.on_read(&IoPorts::ReadPinC);
  pinc.on_write(&IoPorts::WritePinC);
  pinc.sync_always();

  ddrc.allow_read_write();
  ddrc.sync_always();

  portc.allow_read_write();
  portc.on_write(&IoPorts::WritePortC);
  portc.sync_always();

  pind.allow_read_write();
  pind.on_read(&IoPorts::ReadPinD);
  pind.on_write(&IoPorts::WritePinD);
  pind.sync_always();

  ddrd.allow_read_write();
  ddrd.sync_always();

  portd.allow_read_write();
  portd.on_write(&IoPorts::WritePortD);
  portd.sync_always();
}

u8 IoPorts::ReadPinB() {
  u8 val = 0;
  for (size_t i = 0; i < NUM_PORTS; ++i) {
    if (ddrb.get() & mwr::bit(i)) {
      val |= (u8)out_pinb[i] << i;
    } else {
      val |= (u8)in_pinb[i] << i;
    }
  }
  pinb = val;
  return pinb;
}

void IoPorts::WritePinB(u8 val) {
  WritePortB(portb.get() ^ val);
}

void IoPorts::WritePortB(u8 val) {
  portb = val;
  for (size_t i = 0; i < NUM_PORTS; ++i) {
    out_pinb[i] = (val >> i) & 1;
  }
}

u8 IoPorts::ReadPinC() {
  u8 val = 0;
  for (size_t i = 0; i < NUM_PORTS; ++i) {
    if (ddrc.get() & mwr::bit(i)) {
      val |= (u8)out_pinc[i] << i;
    } else {
      val |= (u8)in_pinc[i] << i;
    }
  }
  pinc = val;
  return pinc;
}

void IoPorts::WritePinC(u8 val) {
  WritePortC(portc.get() ^ val);
}

void IoPorts::WritePortC(u8 val) {
  portc = val;
  for (size_t i = 0; i < NUM_PORTS; ++i) {
    out_pinc[i] = (val >> i) & 1;
  }
}

u8 IoPorts::ReadPinD() {
  u8 val = 0;
  for (size_t i = 0; i < NUM_PORTS; ++i) {
    if (ddrd.get() & mwr::bit(i)) {
      val |= (u8)out_pind[i] << i;
    } else {
      val |= (u8)in_pind[i] << i;
    }
  }
  pind = val;
  return pind;
}

void IoPorts::WritePinD(u8 val) {
  WritePortD(portd.get() ^ val);
}

void IoPorts::WritePortD(u8 val) {
  portd = val;
  for (size_t i = 0; i < NUM_PORTS; ++i) {
    out_pind[i] = (val >> i) & 1;
  }
}

void IoPorts::reset() {
  peripheral::reset();
  for (size_t i = 0; i < NUM_PORTS; ++i) {
    out_pinb[i] = false;
    out_pinc[i] = false;
    out_pind[i] = false;
  }
}
