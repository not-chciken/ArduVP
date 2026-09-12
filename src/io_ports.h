#include <vcml.h>

#include "common.h"

struct IoPorts : public vcml::peripheral {
  static constexpr size_t NUM_PORTS = 8;

  IoPorts(const sc_core::sc_module_name& nm);
  ~IoPorts() = default;

  vcml::tlm_target_socket in;

  vcml::gpio_initiator_array<NUM_PORTS> out_pinb;
  vcml::gpio_initiator_array<NUM_PORTS> out_pinc;
  vcml::gpio_initiator_array<NUM_PORTS> out_pind;

  vcml::gpio_target_array<NUM_PORTS> in_pinb;
  vcml::gpio_target_array<NUM_PORTS> in_pinc;
  vcml::gpio_target_array<NUM_PORTS> in_pind;

  vcml::reg<u8> pinb;
  vcml::reg<u8> ddrb;
  vcml::reg<u8> portb;

  vcml::reg<u8> pinc;
  vcml::reg<u8> ddrc;
  vcml::reg<u8> portc;

  vcml::reg<u8> pind;
  vcml::reg<u8> ddrd;
  vcml::reg<u8> portd;

  virtual void reset() override;

 private:
  u8 ReadPinB();
  void WritePinB(u8 val);
  void WritePortB(u8 val);

  u8 ReadPinC();
  void WritePinC(u8 val);
  void WritePortC(u8 val);

  u8 ReadPinD();
  void WritePinD(u8 val);
  void WritePortD(u8 val);
};
