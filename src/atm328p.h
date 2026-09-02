#pragma once

#include <systemc.h>
#include <vcml.h>

#include "adc.h"
#include "comparator.h"
#include "cpu.h"
#include "io_ports.h"
#include "timer.h"
#include "twi.h"
#include "usart.h"

struct Atm328p : public vcml::component {
  Atm328p(const sc_core::sc_module_name& nm)
      : vcml::component(nm),
        mem_range_program(vcml::range(0, 0x7fff)),
        mem_range_sram(vcml::range(0x100, 0x8ff)),
        mem_range_sram_elf(vcml::range(0x800000, 0x8007ff)),
        adc("adc"),
        comparator("comparator"),
        cpu(),
        io_ports("io_ports"),
        timer("timer"),
        twi("twi"),
        usart("usart"),
        loader("loader"),
        bus("bus"),
        prog_bus("prog_bus"),
        sram_bus("sram_bus"),
        program_mem("program_mem", mem_range_program.length()),
        sram_mem("sram_mem", mem_range_sram.length()) {
    bus.bind(cpu.data);
    bus.bind(sram_bus.in[0], mem_range_sram);
    bus.bind(io_ports.in, vcml::range(0x23, 0x2b));
    bus.bind(timer.in, vcml::range(0x35, 0x37), 0x35);
    bus.bind(timer.in, vcml::range(0x43, 0x48), 0x43);
    bus.bind(timer.in, vcml::range(0x6e, 0x70), 0x6e);
    bus.bind(timer.in, vcml::range(0x80, 0x8b), 0x80);
    bus.bind(timer.in, vcml::range(0xb0, 0xb6), 0xb0);
    bus.bind(usart.in, vcml::range(0xc0, 0xc6));
    bus.bind(twi.in, vcml::range(0xb8, 0xbd), 0xb8);
    bus.bind(adc.in, vcml::range(0x78, 0x7c));
    bus.bind(comparator.in, vcml::range(0x50, 0x50));

    prog_bus.bind(sram_bus.in[1], mem_range_sram_elf);
    prog_bus.bind(program_mem.in, mem_range_program);
    prog_bus.bind(loader.insn);
    prog_bus.bind(loader.data);
    prog_bus.bind(cpu.insn);

    sram_bus.bind(sram_mem.in, vcml::range(0, mem_range_sram.length()));

    clk.bind(adc.clk);
    clk.bind(comparator.clk);
    clk.bind(cpu.clk);
    clk.bind(io_ports.clk);
    clk.bind(timer.clk);
    clk.bind(twi.clk);
    clk.bind(usart.clk);
    clk.bind(bus.clk);
    clk.bind(prog_bus.clk);
    clk.bind(sram_bus.clk);
    clk.bind(sram_mem.clk);
    clk.bind(program_mem.clk);

    rst.bind(adc.rst);
    rst.bind(comparator.rst);
    rst.bind(cpu.rst);
    rst.bind(io_ports.rst);
    rst.bind(timer.rst);
    rst.bind(twi.rst);
    rst.bind(usart.rst);
    rst.bind(bus.rst);
    rst.bind(prog_bus.rst);
    rst.bind(sram_bus.rst);
    rst.bind(sram_mem.rst);
    rst.bind(program_mem.rst);

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
    twi.irq.bind(cpu.irq[17]);

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
    cpu.irq_ack[17].bind(twi.ack);
  }

  const vcml::range mem_range_program;
  const vcml::range mem_range_sram;
  const vcml::range mem_range_sram_elf;

  Adc adc;
  Comparator comparator;
  Cpu cpu;
  IoPorts io_ports;
  Timer timer;
  Twi twi;
  Usart usart;

  vcml::meta::loader loader;
  vcml::generic::bus bus;
  vcml::generic::bus prog_bus;
  vcml::generic::bus sram_bus;
  vcml::generic::memory program_mem;
  vcml::generic::memory sram_mem;
};
