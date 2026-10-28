/******************************************************************************
 * Apache License, Version 2.0
 * Copyright (c) 2026 chciken/Niko Zurstraßen
 *****************************************************************************/

#include "adc.h"

#include <cmath>

constexpr u8 ADMUX_MUX0 = mwr::bit(0);
constexpr u8 ADMUX_MUX1 = mwr::bit(1);
constexpr u8 ADMUX_MUX2 = mwr::bit(2);
constexpr u8 ADMUX_MUX3 = mwr::bit(3);
constexpr u8 ADMUX_ADLAR = mwr::bit(5);
constexpr u8 ADMUX_REFS0 = mwr::bit(6);
constexpr u8 ADMUX_REFS1 = mwr::bit(7);

constexpr u8 ADCSRB_ADTS0 = mwr::bit(0);
constexpr u8 ADCSRB_ADTS1 = mwr::bit(1);
constexpr u8 ADCSRB_ADTS2 = mwr::bit(2);
constexpr u8 ADCSRB_ACME = mwr::bit(6);

constexpr u8 ADCSRA_ADPS0 = mwr::bit(0);
constexpr u8 ADCSRA_ADPS1 = mwr::bit(1);
constexpr u8 ADCSRA_ADPS2 = mwr::bit(2);
constexpr u8 ADCSRA_ADIE = mwr::bit(3);
constexpr u8 ADCSRA_ADIF = mwr::bit(4);
constexpr u8 ADCSRA_ADATE = mwr::bit(5);
constexpr u8 ADCSRA_ADSC = mwr::bit(6);
constexpr u8 ADCSRA_ADEN = mwr::bit(7);

constexpr float TEMP_SENSOR_CODE_AT_25C = 352.0f;
constexpr float TEMP_SENSOR_LSB_PER_C = 1.0f;

Adc::Adc(const sc_core::sc_module_name& nm)
    : peripheral(nm),
      signal_host<float>(),
      volt_adc("volt_adc"),
      volt_aref("volt_aref", 5.0f),
      volt_avcc("volt_avcc", 5.0f),
      volt_1_1("volt_1_1", 1.1f),
      temp_celsius("temp_celsius", 25.0f),
      irq("irq"),
      ack("ack"),
      in("in"),
      in_ext_irq0("in_ext_irq0"),
      in_timer0_match_a("in_timer0_match_a"),
      in_timer0_overflow("in_timer0_overflow"),
      in_timer1_match_b("in_timer1_match_b"),
      in_timer1_overflow("in_timer1_overflow"),
      in_timer1_capture("in_timer1_capture"),
      in_analog_comparator("in_analog_comparator"),
      out_acme("out_acme"),
      out_aden("out_aden"),
      out_admux("out_admux"),
      adcl("adcl", 0x0),
      adch("adch", 0x1),
      adcsra("adcsra", 0x2),
      adcsrb("adcsrb", 0x3),
      admux("admux", 0x4),
      conversion_event_("conversion_event"),
      clock_prescaler_(2),
      data_blocked_(false),
      first_conversion_(true),
      conversion_in_progress_(false),
      mux_channel_(0) {
  admux.on_write(&Adc::WriteAdmux);

  adcsra.on_write(&Adc::WriteAdcsra);
  adcsra.sync_always();

  adcsrb.on_write(&Adc::WriteAdcsrb);
  adcsrb.sync_never();

  adcl.allow_read_only();
  adcl.on_read(&Adc::ReadAdcl);
  adcl.sync_always();

  adch.allow_read_only();
  adch.on_read(&Adc::ReadAdch);
  adch.sync_always();

  SC_THREAD(ConversionThread);
}

void Adc::reset() {
  peripheral::reset();
  data_blocked_ = false;
  clock_prescaler_ = 2;
  first_conversion_ = true;
  conversion_in_progress_ = false;
  mux_channel_ = 0;
  irq = false;
  out_acme = false;
  out_aden = false;
  out_admux = (u8)0;
}

void Adc::signal_transport(const vcml::signal_target_socket<float>& socket, const float& value) {
  (void)socket;
  (void)value;
}

void Adc::ConversionThread() {
  while (true) {
    sc_core::wait(conversion_event_);

    mux_channel_ = admux.get() & (ADMUX_MUX0 | ADMUX_MUX1 | ADMUX_MUX2 | ADMUX_MUX3);

    if (first_conversion_) {
      wait_clock_cycles(25 * clock_prescaler_);
    } else {
      wait_clock_cycles(13 * clock_prescaler_);
    }

    conversion_in_progress_ = false;

    if (!(adcsra & ADCSRA_ADEN)) {
      continue;
    }

    first_conversion_ = false;

    float volt_input = GetInputVoltage(mux_channel_);
    float volt_ref = GetReferenceVoltage();

    u16 volt_adc = std::min<i64>(1023ll, std::lround((volt_input * 1024.0f) / volt_ref));

    if (data_blocked_ == false) {
      if (admux & ADMUX_ADLAR) {
        adcl = (volt_adc << 6) & 0xff;
        adch = volt_adc >> 2;
      } else {
        adcl = volt_adc & 0xff;
        adch = (volt_adc >> 8) & 0x3;
      }
    }

    adcsra &= ~ADCSRA_ADSC;
    adcsra |= ADCSRA_ADIF;

    if (adcsra & ADCSRA_ADIE) {
      irq = true;
    }

    bool free_running = !(adcsrb.get() & (ADCSRB_ADTS0 | ADCSRB_ADTS1 | ADCSRB_ADTS2));
    if ((adcsra & ADCSRA_ADATE) && free_running) {
      LaunchConversion();
    }
  }
}

float Adc::GetInputVoltage(uint channel) {
  float volt_input = 0.0f;

  switch (channel) {
  case 0x0:
    volt_input = volt_adc[0];
    break;
  case 0x1:
    volt_input = volt_adc[1];
    break;
  case 0x2:
    volt_input = volt_adc[2];
    break;
  case 0x3:
    volt_input = volt_adc[3];
    break;
  case 0x4:
    volt_input = volt_adc[4];
    break;
  case 0x5:
    volt_input = volt_adc[5];
    break;
  case 0x6:
    volt_input = volt_adc[6];
    break;
  case 0x7:
    volt_input = volt_adc[7];
    break;
  case 0x8: {
    volt_input = TempToVolt();
    break;
  }
  case 0xe:
    volt_input = volt_1_1;
    break;
  case 0xf:
    volt_input = 0.0f;
    break;
  default:
    log_error("unknown voltage input");
  }
  return volt_input;
}

float Adc::GetReferenceVoltage() {
  float volt_ref = 0.0f;
  uint ref = (admux & (ADMUX_REFS0 | ADMUX_REFS1)) >> 6;

  switch (ref) {
  case 0x0:
    volt_ref = volt_aref;
    break;
  case 0x1:
    volt_ref = volt_avcc;
    break;
  case 0x2:
    log_warn("using reserved reference voltage");
    break;
  case 0x3:
    volt_ref = volt_1_1;
    break;
  default:
    log_error("unknown voltage input");
  }

  return volt_ref;
}

float Adc::TempToVolt() {
  float code = TEMP_SENSOR_CODE_AT_25C + (temp_celsius - 25.0f) * TEMP_SENSOR_LSB_PER_C;
  return code / 1024.0f * volt_1_1;
}

void Adc::LaunchConversion() {
  conversion_in_progress_ = true;
  conversion_event_.notify(sc_core::SC_ZERO_TIME);
}

void Adc::WriteAdcsra(u8 val) {
  u8 adif_bit = adcsra & ADCSRA_ADIF;
  if (val & adif_bit || !adif_bit) {
    val &= ~ADCSRA_ADIF;
    irq = false;
  } else {
    val |= adif_bit;
  }

  adcsra = val;
  out_aden = (bool)(val & ADCSRA_ADEN);

  if (!(val & ADCSRA_ADEN)) {
    first_conversion_ = true;
  }

  switch (val & (ADCSRA_ADPS0 | ADCSRA_ADPS1 | ADCSRA_ADPS2)) {
  case 0b000:
  case 0b001:
    clock_prescaler_ = 2;
    break;
  case 0b010:
    clock_prescaler_ = 4;
    break;
  case 0b011:
    clock_prescaler_ = 8;
    break;
  case 0b100:
    clock_prescaler_ = 16;
    break;
  case 0b101:
    clock_prescaler_ = 32;
    break;
  case 0b110:
    clock_prescaler_ = 64;
    break;
  case 0b111:
    clock_prescaler_ = 128;
    break;
  default:
    log_error("unknown prescaler");
  }

  if ((val & ADCSRA_ADSC) && (val & ADCSRA_ADEN)) {
    LaunchConversion();
  }
}

void Adc::WriteAdcsrb(u8 val) {
  constexpr u8 mask = ADCSRB_ADTS0 | ADCSRB_ADTS1 | ADCSRB_ADTS2 | ADCSRB_ACME;
  adcsrb = (adcsrb.get() & ~mask) | (val & mask);
  out_acme = (bool)(adcsrb.get() & ADCSRB_ACME);
}

void Adc::WriteAdmux(u8 val) {
  constexpr u8 mask = 0xef;
  admux = (admux.get() & ~mask) | (val & mask);
  out_admux = admux.get() & (ADMUX_MUX0 | ADMUX_MUX1 | ADMUX_MUX2);
}

u8 Adc::ReadAdcl() {
  data_blocked_ = true;
  return adcl;
}

u8 Adc::ReadAdch() {
  data_blocked_ = false;
  return adch;
}

void Adc::gpio_notify(const vcml::gpio_target_socket& socket, bool state) {
  if (socket == ack) {
    irq = false;
    adcsra &= ~ADCSRA_ADIF;
    return;
  }

  if (!(adcsra & ADCSRA_ADATE))
    return;

  if (state == false)
    return;

  if (socket == in_analog_comparator) {
    if (adcsrb.get() == ADCSRB_ADTS0) {
      LaunchConversion();
    }
  } else if (socket == in_ext_irq0) {
    if (adcsrb.get() == ADCSRB_ADTS1) {
      LaunchConversion();
    }
  } else if (socket == in_timer0_match_a) {
    if (adcsrb.get() == (ADCSRB_ADTS1 | ADCSRB_ADTS0)) {
      LaunchConversion();
    }
  } else if (socket == in_timer0_overflow) {
    if (adcsrb.get() == ADCSRB_ADTS2) {
      LaunchConversion();
    }
  } else if (socket == in_timer1_match_b) {
    if (adcsrb.get() == (ADCSRB_ADTS2 | ADCSRB_ADTS0)) {
      LaunchConversion();
    }
  } else if (socket == in_timer1_overflow) {
    if (adcsrb.get() == (ADCSRB_ADTS2 | ADCSRB_ADTS1)) {
      LaunchConversion();
    }
  } else if (socket == in_timer1_capture) {
    if (adcsrb.get() == (ADCSRB_ADTS2 | ADCSRB_ADTS1 | ADCSRB_ADTS0)) {
      LaunchConversion();
    }
  }
}
