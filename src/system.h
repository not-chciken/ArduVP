#pragma once

#include <systemc.h>
#include <vcml.h>

#include <chrono>
#include <iostream>

#include "adc.h"
#include "comparator.h"
#include "cpu.h"
#include "io_ports.h"
#include "timer.h"
#include "usart.h"

struct ArduSystem : public vcml::system {
  static constexpr size_t NUM_ANALOG_PINS = 6;

  ArduSystem(const sc_core::sc_module_name& nm)
      : vcml::system(nm),
        cpu(),
        timer("timer"),
        usart("usart"),
        adc("adc"),
        comparator("comparator"),
        io_ports("io_ports"),
        builtin_led("builtin_led"),
        serial_term("serial"),
        pin_d("pin_d"),
        pin_a("pin_a"),
        volt_pin_a("volt_pin_a", 0.0f),
        loader("loader"),
        mem_range_program("mem_range_program", vcml::range(0, 0x7fff)),
        mem_range_sram("mem_range_sram", vcml::range(0x100, 0x8ff)),
        mem_range_sram_elf("mem_range_sram_elf_", vcml::range(0x800000, 0x8007ff)),
        bus("bus"),
        prog_bus("prog_bus"),
        sram_bus("sram_bus"),
        clock("clock", 16 * vcml::MHz),
        program_mem("program_mem", mem_range_program.get().length()),
        sram_mem("sram_mem", mem_range_sram.get().length()),
        reset("reset") {
    bus.bind(sram_bus.in[0], mem_range_sram);
    bus.bind(io_ports.in, vcml::range(0x23, 0x2b));
    bus.bind(timer.in, vcml::range(0x35, 0x37), 0x35);
    bus.bind(timer.in, vcml::range(0x43, 0x48), 0x43);
    bus.bind(timer.in, vcml::range(0x6e, 0x70), 0x6e);
    bus.bind(timer.in, vcml::range(0x80, 0x8b), 0x80);
    bus.bind(timer.in, vcml::range(0xb0, 0xb6), 0xb0);
    bus.bind(usart.in, vcml::range(0xc0, 0xc6));
    bus.bind(adc.in, vcml::range(0x78, 0x7c));
    bus.bind(comparator.in, vcml::range(0x50, 0x50));

    prog_bus.bind(sram_bus.in[1], mem_range_sram_elf);
    prog_bus.bind(program_mem.in, mem_range_program);
    prog_bus.bind(loader.insn);
    prog_bus.bind(loader.data);
    prog_bus.bind(cpu.insn);

    sram_bus.bind(sram_mem.in, vcml::range(0, mem_range_sram.get().length()));

    bus.bind(cpu.data);
    clock.clk.bind(cpu.clk);
    reset.rst.bind(cpu.rst);

    clock.clk.bind(timer.clk);
    reset.rst.bind(timer.rst);

    clock.clk.bind(usart.clk);
    reset.rst.bind(usart.rst);

    clock.clk.bind(adc.clk);
    reset.rst.bind(adc.rst);

    clock.clk.bind(comparator.clk);
    reset.rst.bind(comparator.rst);

    clock.clk.bind(io_ports.clk);
    reset.rst.bind(io_ports.rst);

    usart.serial_tx.bind(serial_term.serial_rx);
    serial_term.serial_tx.bind(usart.serial_rx);

    clock.clk.bind(bus.clk);
    clock.clk.bind(prog_bus.clk);
    clock.clk.bind(sram_bus.clk);
    clock.clk.bind(sram_mem.clk);
    clock.clk.bind(program_mem.clk);

    reset.rst.bind(bus.rst);
    reset.rst.bind(prog_bus.rst);
    reset.rst.bind(sram_bus.rst);
    reset.rst.bind(sram_mem.rst);
    reset.rst.bind(program_mem.rst);

    adc.in_ext_irq0.stub();
    timer.ocie0a_out.bind(adc.in_timer0_match_a);
    timer.ocie1b_out.bind(adc.in_timer1_match_b);
    timer.toie0_out.bind(adc.in_timer0_overflow);
    timer.toie1_out.bind(adc.in_timer1_overflow);
    timer.icie1_out.bind(adc.in_timer1_capture);
    comparator.aco.bind(adc.in_analog_comparator);

    adc.out_acme.bind(comparator.in_acme);
    adc.out_aden.bind(comparator.in_aden);
    adc.out_admux.bind(comparator.in_admux);

    comparator.acic.stub();

    pin_d[6].bind(comparator.volt_ain[0]);
    pin_d[7].bind(comparator.volt_ain[1]);

    for (size_t i = 0; i < NUM_ANALOG_PINS; ++i) {
      pin_a[i].bind(adc.volt_adc[i]);
      pin_a[i] = volt_pin_a[i];
    }

    io_ports.out_pinb[5].bind(builtin_led.gpio_in[0]);

    for (size_t i = 0; i < 8; ++i) {
      io_ports.in_pinb[i].stub();
      io_ports.in_pinc[i].stub();
      io_ports.in_pind[i].stub();
      io_ports.out_pinc[i].stub();
      io_ports.out_pind[i].stub();

      if (i == 5) {
        continue;
      }

      io_ports.out_pinb[i].stub();
    }

    timer.ocie2a_out.bind(cpu.irq[0]);
    timer.ocie2b_out.bind(cpu.irq[1]);
    timer.toie2_out.bind(cpu.irq[2]);
    timer.icie1_out.bind(cpu.irq[3]);
    timer.ocie1a_out.bind(cpu.irq[4]);
    timer.ocie1b_out.bind(cpu.irq[5]);
    timer.toie1_out.bind(cpu.irq[6]);
    timer.ocie0a_out.bind(cpu.irq[7]);
    timer.ocie0b_out.bind(cpu.irq[8]);
    timer.toie0_out.bind(cpu.irq[9]);
    usart.irq_rx.bind(cpu.irq[11]);
    usart.irq_udre.bind(cpu.irq[12]);
    usart.irq_tx.bind(cpu.irq[13]);
    adc.irq.bind(cpu.irq[14]);
    comparator.irq.bind(cpu.irq[16]);

    cpu.irq_ack[0].bind(timer.ocie2a_ack);
    cpu.irq_ack[1].bind(timer.ocie2b_ack);
    cpu.irq_ack[2].bind(timer.toie2_ack);
    cpu.irq_ack[3].bind(timer.icie1_ack);
    cpu.irq_ack[4].bind(timer.ocie1a_ack);
    cpu.irq_ack[5].bind(timer.ocie1b_ack);
    cpu.irq_ack[6].bind(timer.toie1_ack);
    cpu.irq_ack[7].bind(timer.ocie0a_ack);
    cpu.irq_ack[8].bind(timer.ocie0b_ack);
    cpu.irq_ack[9].bind(timer.toie0_ack);
    cpu.irq_ack[11].bind(usart.rxc_ack);
    cpu.irq_ack[12].bind(usart.udre_ack);
    cpu.irq_ack[13].bind(usart.tx_ack);
    cpu.irq_ack[14].bind(adc.ack);
    cpu.irq_ack[16].bind(comparator.ack);
  }

  virtual int run() override { return vcml::system::run(); }
  virtual void start_of_simulation() override { sim_start_time = std::chrono::steady_clock::now(); }
  virtual void end_of_simulation() override {
    auto elapsed = std::chrono::steady_clock::now() - sim_start_time;
    double elapsed_wall_clock_time = std::chrono::duration<double>(elapsed).count();
    double elapsed_virtual_time = vcml::time_stamp_ms() / 1000.0;

    log_info("Elapsed wall-clock time: %.3f seconds", elapsed_wall_clock_time);
    log_info("Elapsed virtual time:    %.3f seconds", elapsed_virtual_time);
    log_info("Real-time factor:        %.3f", elapsed_virtual_time / elapsed_wall_clock_time);
  }

  Cpu cpu;
  Timer timer;
  Usart usart;
  Adc adc;
  Comparator comparator;
  IoPorts io_ports;

  vcml::gpio::leds builtin_led;

  vcml::serial::terminal serial_term;

  vcml::signal_initiator_array<float, 8> pin_d;
  vcml::signal_initiator_array<float, NUM_ANALOG_PINS> pin_a;
  vcml::property<float, NUM_ANALOG_PINS> volt_pin_a;

  vcml::meta::loader loader;

  vcml::property<vcml::range> mem_range_program;
  vcml::property<vcml::range> mem_range_sram;
  vcml::property<vcml::range> mem_range_sram_elf;

  vcml::generic::bus bus;
  vcml::generic::bus prog_bus;
  vcml::generic::bus sram_bus;
  vcml::generic::clock clock;
  vcml::generic::memory program_mem;
  vcml::generic::memory sram_mem;
  vcml::generic::reset reset;

  std::chrono::steady_clock::time_point sim_start_time;
};
