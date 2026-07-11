#pragma once

#include <systemc.h>
#include <vcml.h>

#include <iostream>

#include "cpu.h"
#include "timer.h"
#include "usart.h"

struct ArduSystem : public vcml::system {
  ArduSystem(const sc_core::sc_module_name& nm)
      : vcml::system(nm),
        cpu(),
        timer("timer"),
        usart("usart"),
        serial_term("serial"),
        loader("loader"),
        mem_range_program("mem_range_program", vcml::range(0, 0x7fff)),
        mem_range_sram("mem_range_sram", vcml::range(0x100, 0x8ff)),
        mem_range_sram_elf("mem_range_sram_elf_", vcml::range(0x800000, 0x8007ff)),
        bus("bus"),
        prog_bus("prog_bus"),
        clock("clock", 16 * vcml::MHz),
        program_mem("program_mem", mem_range_program.get().length()),
        sram_mem("sram_mem", mem_range_sram.get().length()),
        reset("reset") {
    bus.bind(sram_mem.in, mem_range_sram);
    bus.bind(timer.in, vcml::range(0x35, 0x37), 0x35);
    bus.bind(timer.in, vcml::range(0x43, 0x48), 0x43);
    bus.bind(timer.in, vcml::range(0x6e, 0x70), 0x6e);
    bus.bind(timer.in, vcml::range(0x80, 0x8b), 0x80);
    bus.bind(timer.in, vcml::range(0xb0, 0xb6), 0xb0);
    bus.bind(usart.in, vcml::range(0xc0, 0xc6), 0xc0);

    // prog_bus.bind(sram_mem.in, mem_range_sram_elf);
    prog_bus.bind(program_mem.in, mem_range_program);
    prog_bus.bind(loader.insn);
    prog_bus.bind(loader.data);
    prog_bus.bind(cpu.insn);

    bus.bind(cpu.data);
    clock.clk.bind(cpu.clk);
    reset.rst.bind(cpu.rst);

    clock.clk.bind(timer.clk);
    reset.rst.bind(timer.rst);

    clock.clk.bind(usart.clk);
    reset.rst.bind(usart.rst);

    serial_term.backends = "stdout";
    usart.serial_tx.bind(serial_term.serial_rx);
    serial_term.serial_tx.bind(usart.serial_rx);

    clock.clk.bind(bus.clk);
    clock.clk.bind(prog_bus.clk);
    clock.clk.bind(sram_mem.clk);
    clock.clk.bind(program_mem.clk);

    reset.rst.bind(bus.rst);
    reset.rst.bind(prog_bus.rst);
    reset.rst.bind(sram_mem.rst);
    reset.rst.bind(program_mem.rst);

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
  }

  virtual int run() override { return vcml::system::run(); }

  Cpu cpu;
  Timer timer;
  Usart usart;
  vcml::serial::terminal serial_term;

  vcml::meta::loader loader;

  vcml::property<vcml::range> mem_range_program;
  vcml::property<vcml::range> mem_range_sram;
  vcml::property<vcml::range> mem_range_sram_elf;

  vcml::generic::bus bus;
  vcml::generic::bus prog_bus;
  vcml::generic::clock clock;
  vcml::generic::memory program_mem;
  vcml::generic::memory sram_mem;
  vcml::generic::reset reset;
};