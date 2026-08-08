#include "comparator.h"

constexpr u8 ACSR_ACIS0 = mwr::bit(0);
constexpr u8 ACSR_ACIS1 = mwr::bit(1);
constexpr u8 ACSR_ACIC = mwr::bit(2);
constexpr u8 ACSR_ACIE = mwr::bit(3);
constexpr u8 ACSR_ACI = mwr::bit(4);
constexpr u8 ACSR_ACO = mwr::bit(5);
constexpr u8 ACSR_ACBG = mwr::bit(6);
constexpr u8 ACSR_ACD = mwr::bit(7);

constexpr u8 ADCSRB_ACME = mwr::bit(6);

constexpr u8 ADCSRA_ADEN = mwr::bit(7);

Comparator::Comparator(const sc_core::sc_module_name& nm)
    : peripheral(nm),
      signal_host<float>(),
      signal_host<u8>(),
      volt_adc("volt_adc"),
      volt_ain("volt_ain"),
      volt_bandgap("volt_bandgap", 1.1f),
      aco("aco"),
      acic("acic"),
      irq("irq"),
      in_acme("in_acme"),
      in_aden("in_aden"),
      in_admux("in_admux"),
      in("in"),
      acsr("acsr", 0x0) {
  acsr.allow_read_write();
  acsr.on_write_mask(0xdf);
  acsr.on_write(&Comparator::WriteAcsr);
  acsr.sync_always();
}

void Comparator::signal_transport(const vcml::signal_target_socket<float>& socket, const float& value) {
  DoComparison();
}

void Comparator::signal_transport(const vcml::signal_target_socket<u8>& socket, const u8& value) {
  DoComparison();
}

void Comparator::gpio_notify(const vcml::gpio_target_socket& socket, bool state) {
  DoComparison();
}

void Comparator::DoComparison() {
  if (acsr.get() & ACSR_ACD) {
    return;
  }

  float input_positive = volt_ain[0].read();
  float input_negative = volt_ain[1].read();

  if (acsr.get() & ACSR_ACBG) {
    input_positive = volt_bandgap;
  }

  if ((in_acme == true) && (in_aden == false)) {
    input_negative = volt_adc[in_admux.read() & 0b111];
  }

  if (input_positive > input_negative) {
    switch (acsr.get() & (ACSR_ACIS1 | ACSR_ACIS0)) {
    case 0:
      if (aco == false) {
        acsr |= ACSR_ACI;
      }
      break;
    case 1:
      log_error("unknown mode");
      break;
    case 2:
      break;
    case 3:
      if (aco == false) {
        acsr |= ACSR_ACI;
      }
      break;
    default:
      log_error("unknown mode");
    }

    aco = true;
    acsr |= ACSR_ACO;

    if ((acsr.get() & ACSR_ACI) && (acsr.get() & ACSR_ACIE)) {
      irq = true;
    }
  } else {
    switch (acsr.get() & (ACSR_ACIS1 | ACSR_ACIS0)) {
    case 0:
      if (aco == true) {
        acsr |= ACSR_ACI;
      }
      break;
    case 1:
      log_error("unknown mode");
      break;
    case 2:
      if (aco == true) {
        acsr |= ACSR_ACI;
      }
      break;
    case 3:
      break;
    default:
      log_error("unknown mode");
    }

    aco = false;
    acsr &= ~ACSR_ACO;

    if ((acsr.get() & ACSR_ACI) && (acsr.get() & ACSR_ACIE)) {
      irq = true;
    }
  }

  acic = (acsr.get() & ACSR_ACIC) ? (bool)aco : false;
}

void Comparator::WriteAcsr(u8 val) {
  u8 aci_bit = acsr & ACSR_ACI;
  if (val & aci_bit || !aci_bit) {
    val &= ~ACSR_ACI;
    irq = false;
  } else {
    val |= aci_bit;
  }

  acsr = val;

  DoComparison();
}
