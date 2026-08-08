#pragma once

#include <systemc.h>
#include <vcml.h>

#include <iostream>

#include "adc.h"
#include "cpu.h"
#include "timer.h"
#include "usart.h"

struct ArduSystem : public vcml::system {
  ArduSystem(const sc_core::sc_module_name& nm)
      : vcml::system(nm),
        cpu(),
        timer("timer"),
        usart("usart"),
        adc("adc"),
        serial_term("serial"),
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
    bus.bind(timer.in, vcml::range(0x35, 0x37), 0x35);
    bus.bind(timer.in, vcml::range(0x43, 0x48), 0x43);
    bus.bind(timer.in, vcml::range(0x6e, 0x70), 0x6e);
    bus.bind(timer.in, vcml::range(0x80, 0x8b), 0x80);
    bus.bind(timer.in, vcml::range(0xb0, 0xb6), 0xb0);
    bus.bind(usart.in, vcml::range(0xc0, 0xc6));
    bus.bind(adc.in, vcml::range(0x78, 0x7c));

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

    serial_term.backends = "stdout";
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
    toie0_out.bin(adc.in_timer0_overflow);
    toie1_out.bin(adc.in_timer1_overflow);
    timer.icie1_out.bind(adc.in_timer1_capture);

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
  }

  virtual int run() override { return vcml::system::run(); }

  Cpu cpu;
  Timer timer;
  Usart usart;
  Adc adc;
  vcml::serial::terminal serial_term;

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
};
