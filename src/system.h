#pragma once

#include <systemc.h>
#include <vcml.h>

#include <iostream>

#include "cpu.h"
#include "timer.h"
#include "usart.h"

class ArduSystem : public vcml::system {
 public:
  ArduSystem(const sc_core::sc_module_name& nm)
      : vcml::system(nm),
        cpu(),
        timer_("timer"),
        usart_("usart"),
        serial_term_("serial"),
        loader_("loader"),
        mem_range_program_("mem_range_program", vcml::range(0, 0x7fff)),
        mem_range_sram_("mem_range_sram", vcml::range(0x100, 0x8ff)),
        mem_range_sram_elf_("mem_range_sram_elf_", vcml::range(0x800000, 0x8007ff)),
        bus_("bus"),
        prog_bus_("prog_bus"),
        clock_("clock", 16 * vcml::MHz),
        program_mem_("program_mem", mem_range_program_.get().length()),
        sram_mem_("sram_mem", mem_range_sram_.get().length()),
        reset_("reset") {
    bus_.bind(sram_mem_.in, mem_range_sram_);
    bus_.bind(timer_.in, vcml::range(0x35, 0x37), 0x35);
    bus_.bind(timer_.in, vcml::range(0x43, 0x48), 0x43);
    bus_.bind(timer_.in, vcml::range(0x6e, 0x70), 0x6e);
    bus_.bind(timer_.in, vcml::range(0x80, 0x8b), 0x80);
    bus_.bind(timer_.in, vcml::range(0xb0, 0xb6), 0xb0);
    bus_.bind(usart_.in, vcml::range(0xc0, 0xc6), 0xc0);

    //prog_bus_.bind(sram_mem_.in, mem_range_sram_elf_);
    prog_bus_.bind(program_mem_.in, mem_range_program_);
    prog_bus_.bind(loader_.insn);
    prog_bus_.bind(loader_.data);
    prog_bus_.bind(cpu.insn);

    bus_.bind(cpu.data);
    clock_.clk.bind(cpu.clk);
    reset_.rst.bind(cpu.rst);

    clock_.clk.bind(timer_.clk);
    reset_.rst.bind(timer_.rst);

    clock_.clk.bind(usart_.clk);
    reset_.rst.bind(usart_.rst);

    serial_term_.backends = "stdout";
    usart_.serial_tx.bind(serial_term_.serial_rx);
    serial_term_.serial_tx.bind(usart_.serial_rx);

    clock_.clk.bind(bus_.clk);
    clock_.clk.bind(prog_bus_.clk);
    clock_.clk.bind(sram_mem_.clk);
    clock_.clk.bind(program_mem_.clk);

    reset_.rst.bind(bus_.rst);
    reset_.rst.bind(prog_bus_.rst);
    reset_.rst.bind(sram_mem_.rst);
    reset_.rst.bind(program_mem_.rst);
  }

  virtual int run() override { return vcml::system::run(); }

 public:
  Cpu cpu;
  Timer timer_;
  Usart usart_;
  vcml::serial::terminal serial_term_;

  vcml::meta::loader loader_;

  vcml::property<vcml::range> mem_range_program_;
  vcml::property<vcml::range> mem_range_sram_;
  vcml::property<vcml::range> mem_range_sram_elf_;

  vcml::generic::bus bus_;
  vcml::generic::bus prog_bus_;
  vcml::generic::clock clock_;
  vcml::generic::memory program_mem_;
  vcml::generic::memory sram_mem_;
  vcml::generic::reset reset_;
};