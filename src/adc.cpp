#include "adc.h"

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

Adc::Adc(const sc_core::sc_module_name& nm)
    : peripheral(nm),
      volt_aref("volt_aref", 3.0f),
      volt_avcc("volt_avcc", 5.0f),
      volt_1_1("volt_1_1", 1.1f),
      irq("irq"),
      in("in"),
      in_ext_irq0("in_ext_irq0"),
      in_timer0_match_a("in_timer0_match_a"),
      in_timer0_overflow("in_timer0_overflow"),
      in_timer1_match_b("in_timer1_match_b"),
      in_timer1_overflow("in_timer1_overflow"),
      in_timer1_capture("in_timer1_capture"),
      adcl("adcl", 0x0),
      adch("adch", 0x1),
      adcsra("adcsra", 0x2),
      adcsrb("adcsrb", 0x3),
      admux("admux", 0x4),
      conversion_event_("conversion_event"),
      clock_prescaler_(2)
{
  admux.on_write_mask(0xef);

  adcsra.on_write(&Adc::WriteAdcsra);

  adcl.allow_read_only();
  adch.allow_read_only();

  SC_THREAD(ConversionThread);
}

void Adc::ConversionThread() {
  while (true) {
    sc_core::wait(conversion_event_);

    if (adcsra & ADCSRA_ADIE) {
      adcsra |= ADCSRA_ADIF;
      irq = true;
    }
  }
}

void Adc::WriteAdcsra(u8 val) {
  adcsra = val;

  if (val & ADCSRA_ADSC) {
    conversion_event_.notify();
  }

  switch (val & (ADCSRA_ADPS0 | ADCSRA_ADPS1 | ADCSRA_ADPS2)) {
  case 0b000:
    clock_prescaler_ = 2;
    break;
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
}

void Adc::gpio_notify(const vcml::gpio_target_socket& socket, bool state) {
  if (!(adcsra & ADCSRA_ADATE))
    return;

  // TODO: Free running mode and analog comparator.

  if (socket == in_ext_irq0) {
    if (adcsrb == ADCSRB_ADTS1) {
      conversion_event_.notify();
    }
  } else if (socket == in_timer0_match_a) {
    if (adcsrb == (ADCSRB_ADTS1 | ADCSRB_ADTS0)) {
      conversion_event_.notify();
    }
  } else if (socket == in_timer0_overflow) {
    if (adcsrb == ADCSRB_ADTS2) {
      conversion_event_.notify();
    }
  } else if (socket == in_timer1_match_b) {
    if (adcsrb == (ADCSRB_ADTS2 | ADCSRB_ADTS0)) {
      conversion_event_.notify();
    }
  } else if (socket == in_timer1_overflow) {
    if (adcsrb == (ADCSRB_ADTS2 | ADCSRB_ADTS1)) {
      conversion_event_.notify();
    }
  } else if (socket == in_timer1_capture) {
    if (adcsrb == (ADCSRB_ADTS2 | ADCSRB_ADTS1 | ADCSRB_ADTS0)) {
      conversion_event_.notify();
    }
  }
}
