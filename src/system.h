#pragma once

#include <systemc.h>
#include <vcml.h>

#include <iostream>

#include "cpu.h"

class ArduSystem : public vcml::system {
 public:
  ArduSystem(const sc_core::sc_module_name& nm)
      : vcml::system(nm),
        cpu(),
        hex_file("hex_file",""),
        mem_range_program_("mem_range_program", vcml::range(0, 0x7fff)),
        mem_range_sram_("mem_range_sram", vcml::range(0x100, 0x8ff)),
        mem_range_ext_io_("mem_range_ext_io", vcml::range(0x60, 0xff)),
        mem_range_io_("mem_range_io", vcml::range(0x20, 0x5f)),
        bus_("bus"),
        clock_("clock", 1 * vcml::GHz),
        program_mem_("program_mem", mem_range_program_.get().length()),
        sram_mem_("sram_mem", mem_range_sram_.get().length()),
        ext_io_mem_("ext_io_mem", mem_range_ext_io_.get().length()),
        io_mem_("io_mem", mem_range_io_.get().length()),
        reset_("reset")
      {

    bus_.bind(io_mem_.in, mem_range_io_);
    bus_.bind(ext_io_mem_.in, mem_range_ext_io_);
    bus_.bind(sram_mem_.in, mem_range_sram_);

    cpu.insn.bind(program_mem_.in);

    bus_.bind(cpu.data);
    clock_.clk.bind(cpu.clk);
    reset_.rst.bind(cpu.rst);

    clock_.clk.bind(bus_.clk);
    clock_.clk.bind(sram_mem_.clk);
    clock_.clk.bind(program_mem_.clk);
    clock_.clk.bind(ext_io_mem_.clk);
    clock_.clk.bind(io_mem_.clk);
    reset_.rst.bind(bus_.rst);
    reset_.rst.bind(sram_mem_.rst);
    reset_.rst.bind(program_mem_.rst);
    reset_.rst.bind(io_mem_.rst);
    reset_.rst.bind(ext_io_mem_.rst);

    if (!hex_file.get().empty()) {
        program_mem_.load_image(hex_file, 0, vcml::debugging::image_type::IMAGE_IHEX);
    }
  }

  virtual int run() override {
    return vcml::system::run();
  }

 public:
  Cpu cpu;

  vcml::property<std::string> hex_file;

  vcml::property<vcml::range> mem_range_program_;
  vcml::property<vcml::range> mem_range_sram_;
  vcml::property<vcml::range> mem_range_ext_io_;
  vcml::property<vcml::range> mem_range_io_;

  vcml::generic::bus bus_;
  vcml::generic::clock clock_;
  vcml::generic::memory program_mem_;
  vcml::generic::memory sram_mem_;
  vcml::generic::memory ext_io_mem_;
  vcml::generic::memory io_mem_;
  vcml::generic::reset reset_;
};