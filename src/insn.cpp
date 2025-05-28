#include "insn.h"
#include "cpu.h"

#include <format>

string Insn::GetStatementString(u16 inst)
{
  return name;
}

std::function<void()> InsnRr::GenFunc(u16 insn, Cpu *cpu)
{
  assert(exec_func != nullptr);
  auto [rd, rr] = InitArgs(insn);
  return std::bind(exec_func, cpu, rd, rr);
}

std::tuple<uint, uint> InsnRr::InitArgs(u16 insn)
{
  uint rd = ExtractBits(insn, 0, 4) | (ExtractBits(insn, 8, 1) << 4);
  uint rr = ExtractBits(insn, 4, 5);

  return {rd, rr};
}

string InsnRr::GetStatementString(u16 insn)
{
  auto [rd, rr] = InitArgs(insn);
  return string(name) + " " + RegName(rd) + "," + RegName(rr);
}

std::function<void()> InsnR::GenFunc(u16 insn, Cpu *cpu)
{
  assert(exec_func != nullptr);
  auto rd = InitArgs(insn);
  return std::bind(exec_func, cpu, rd);
}

uint InsnR::InitArgs(u16 insn)
{
  uint rd = ExtractBits(insn, 4, 5);
  return rd;
}

string InsnR::GetStatementString(u16 insn)
{
  auto rd= InitArgs(insn);
  return string(name) + " " + RegName(rd);
}

std::function<void()> InsnImm6::GenFunc(u16 insn, Cpu *cpu)
{
  assert(exec_func != nullptr);
  auto [rd, imm] = InitArgs(insn);
  return std::bind(exec_func, cpu, rd, imm);
}

std::tuple<uint, uint> InsnImm6::InitArgs(u16 insn)
{
  uint rd = ExtractBits(insn, 4, 2);
  uint imm = ExtractBits(insn, 0, 4) | (ExtractBits(insn, 6, 2) << 4);

  return {rd, imm};
}

string InsnImm6::GetStatementString(u16 insn)
{
  auto [rd, imm] = InitArgs(insn);
  return string(name) + " " + RegName(rd * 2 + 24) + "," + std::to_string(imm);
}


std::function<void()> InsnImm8::GenFunc(u16 insn, Cpu *cpu)
{
  assert(exec_func != nullptr);
  auto [rd, imm] = InitArgs(insn);
  return std::bind(exec_func, cpu, rd, imm);
}

std::tuple<uint, uint> InsnImm8::InitArgs(u16 insn)
{
  uint rd = ExtractBits(insn, 4, 4);
  uint imm = ExtractBits(insn, 0, 4) | (ExtractBits(insn, 7, 4) << 4);

  return {rd, imm};
}

string InsnImm8::GetStatementString(u16 insn)
{
  auto [rd, imm] = InitArgs(insn);
  return string(name) + " " + RegName(rd) + "," + std::format("{:x}", imm);
}

Adc::Adc()
{
  exec_func = Execute;
  cycles = 1;
  name = "ADC";
}

void Adc::Execute(Cpu *cpu, uint rd, uint rr)
{
  u8 reg = cpu->rf[rd];
  u8 rs1 = cpu->rf[rr];
  u8 result = reg + rs1 + cpu->sreg.c;

  cpu->sreg.h = ((reg & 0x0F) + (rs1 & 0x0F) + cpu->sreg.c) > 0x0F;
  cpu->sreg.v = ((reg ^ result) & (rs1 ^ result)) & 0x8000;
  cpu->sreg.n = (result & 0x8000) != 0;
  cpu->sreg.z = (result == 0);
  cpu->sreg.c = (result < reg) || (result < rs1);

  cpu->rf[rd] = result;
}

Add::Add()
{
  exec_func = Execute;
  cycles = 1;
  name = "ADD";
}

void Add::Execute(Cpu *cpu, uint rd, uint rr)
{
  u8 reg = cpu->rf[rd];
  u8 rs1 = cpu->rf[rr];
  u8 result = reg + rs1;

  cpu->sreg.h = ((reg & 0x0F) + (rs1 & 0x0F)) > 0x0F;
  cpu->sreg.v = ((reg ^ result) & (rs1 ^ result)) & 0x80;
  cpu->sreg.n = (result & 0x8000) != 0;
  cpu->sreg.z = (result == 0);
  cpu->sreg.c = (result < reg) || (result < rs1);

  cpu->rf[rd] = result;
}

Adiw::Adiw()
{
  exec_func = Execute;
  cycles = 2;
  name = "ADIW";
}

void Adiw::Execute(Cpu *cpu, uint rd, uint imm)
{
  u16 reg = (u16)cpu->rf[rd * 2 + 24] | ((u16)cpu->rf[rd * 2 + 25] << 8);
  u16 result = reg + imm;

  cpu->sreg.v = ((reg ^ result) & (imm ^ result)) & 0x8000;
  cpu->sreg.n = (result & 0x8000) != 0;
  cpu->sreg.z = (result == 0);
  cpu->sreg.c = (result < reg) || (result < imm);

  cpu->rf[rd] = result;
}

And::And()
{
  exec_func = Execute;
  cycles = 1;
  name = "AND";
}

void And::Execute(Cpu *cpu, uint rd, uint rr)
{
  u8 reg = cpu->rf[rd];
  u8 rs1 = cpu->rf[rr];
  u8 result = reg & rs1;

  cpu->sreg.v = false;
  cpu->sreg.n = (result & 0x80) != 0;
  cpu->sreg.z = (result == 0);

  cpu->rf[rd] = result;
}

Andi::Andi()
{
  exec_func = Execute;
  cycles = 1;
  name = "ANDI";
}

void Andi::Execute(Cpu *cpu, uint rd, uint imm)
{
  u8 reg = cpu->rf[rd];
  u8 result = reg & imm;

  cpu->sreg.v = false;
  cpu->sreg.n = (result & 0x80) != 0;
  cpu->sreg.z = (result == 0);

  cpu->rf[rd] = result;
}

Com::Com()
{
  exec_func = Execute;
  cycles = 1;
  name = "COM";
}

void Com::Execute(Cpu *cpu, uint rd)
{
  u8 reg = cpu->rf[rd];
  u8 result = ~reg;

  cpu->sreg.v = false;
  cpu->sreg.n = (result & 0x80) != 0;
  cpu->sreg.z = (result == 0);
  cpu->sreg.c = true;

  cpu->rf[rd] = result;
}

Cp::Cp()
{
  exec_func = Execute;
  cycles = 1;
  name = "CP";
}

void Cp::Execute(Cpu *cpu, uint rd, uint rr)
{
  u8 reg = cpu->rf[rd];
  u8 rs1 = cpu->rf[rr];
  u8 result = reg - rs1;

  cpu->sreg.h = ((reg & 0x0F) - (rs1 & 0x0F)) < 0;
  cpu->sreg.v = ((reg ^ result) & (rs1 ^ result)) & 0x80;
  cpu->sreg.n = (result & 0x80) != 0;
  cpu->sreg.z = (result == 0);
  cpu->sreg.c = (result > reg) || (result > rs1);
}

Dec::Dec()
{
  exec_func = Execute;
  cycles = 1;
  name = "DEC";
}

void Dec::Execute(Cpu *cpu, uint rd)
{
  u8 reg = cpu->rf[rd];
  u8 result = reg - 1;

  cpu->sreg.v = (reg == 0x80);
  cpu->sreg.n = (result & 0x80) != 0;
  cpu->sreg.z = (result == 0);

  cpu->rf[rd] = result;
}

Eor::Eor()
{
  exec_func = Execute;
  cycles = 1;
  name = "EOR";
}

Fmul::Fmul()
{
  exec_func = Execute;
  cycles = 2;
  name = "FMUL";
}

void Fmul::Execute(Cpu *cpu, uint rd, uint rr)
{
  u8 reg = cpu->rf[rd];
  u8 rs1 = cpu->rf[rr];
  u16 result = (u16)reg * (u16)rs1;

  cpu->sreg.z = (result == 0);
  cpu->sreg.c = result & 0x8000;

  result = result << 1;
  cpu->rf[0] = result & 0xff;
  cpu->rf[1] = result >> 8;
}

void Eor::Execute(Cpu *cpu, uint rd, uint rr)
{
  u8 reg = cpu->rf[rd];
  u8 rs1 = cpu->rf[rr];
  u8 result = reg ^ rs1;

  cpu->sreg.v = false;
  cpu->sreg.n = (result & 0x80) != 0;
  cpu->sreg.z = (result == 0);

  cpu->rf[rd] = result;
}

Inc::Inc()
{
  exec_func = Execute;
  cycles = 1;
  name = "INC";
}

void Inc::Execute(Cpu *cpu, uint rd)
{
  u8 reg = cpu->rf[rd];
  u8 result = reg + 1;

  cpu->sreg.v = (reg == 0x7F);
  cpu->sreg.n = (result & 0x80) != 0;
  cpu->sreg.z = (result == 0);

  cpu->rf[rd] = result;
}

Mul::Mul()
{
  exec_func = Execute;
  cycles = 2;
  name = "MUL";
}

void Mul::Execute(Cpu *cpu, uint rd, uint rr)
{
  u8 reg = cpu->rf[rd];
  u8 rs1 = cpu->rf[rr];
  u16 result = (u16)reg * (u16)rs1;

  cpu->sreg.z = (result == 0);
  cpu->sreg.c = result & 0x8000;

  cpu->rf[0] = result & 0xff;
  cpu->rf[1] = result >> 8;
}

Mulsu::Mulsu()
{
  exec_func = Execute;
  cycles = 2;
  name = "MULSU";
}

void Mulsu::Execute(Cpu *cpu, uint rd, uint rr)
{
  u8 reg = cpu->rf[rd];
  u8 rs1 = cpu->rf[rr];
  u16 result = (i16)(i8)reg * (u16)rs1;

  cpu->sreg.z = (result == 0);
  cpu->sreg.c = result & 0x8000;

  cpu->rf[0] = result & 0xff;
  cpu->rf[1] = result >> 8;
}

Neg::Neg()
{
  exec_func = Execute;
  cycles = 1;
  name = "NEG";
}

void Neg::Execute(Cpu *cpu, uint rd)
{
  u8 reg = cpu->rf[rd];
  u8 result = -reg;

  cpu->sreg.h = (reg & 0x0F) != 0;
  cpu->sreg.v = (reg == 0x80);
  cpu->sreg.n = (result & 0x80) != 0;
  cpu->sreg.z = result == 0;
  cpu->sreg.c = result != 0;

  cpu->rf[rd] = result;
}

Nop::Nop()
{
  exec_func = Execute;
  cycles = 1;
  name = "NOP";
}

void Nop::Execute(Cpu *cpu, uint rd)
{
}

Or::Or()
{
  exec_func = Execute;
  cycles = 1;
  name = "OR";
}

void Or::Execute(Cpu *cpu, uint rd, uint rr)
{
  u8 reg = cpu->rf[rd];
  u8 rs1 = cpu->rf[rr];
  u8 result = reg | rs1;

  cpu->sreg.v = false;
  cpu->sreg.n = (result & 0x80) != 0;
  cpu->sreg.z = (result == 0);

  cpu->rf[rd] = result;
}

Ori::Ori()
{
  exec_func = Execute;
  cycles = 1;
  name = "ORI";
}

void Ori::Execute(Cpu *cpu, uint rd, uint imm)
{
  u8 reg = cpu->rf[rd];
  u8 result = reg | imm;

  cpu->sreg.v = false;
  cpu->sreg.n = (result & 0x80) != 0;
  cpu->sreg.z = (result == 0);

  cpu->rf[rd] = result;
}

Sbci::Sbci()
{
  exec_func = Execute;
  cycles = 1;
  name = "SBCI";
}

void Sbci::Execute(Cpu *cpu, uint rd, uint imm)
{
  u8 reg = cpu->rf[rd];
  u8 result = reg - imm - cpu->sreg.c;

  cpu->sreg.h = ((reg & 0x0F) - (imm & 0x0F) - cpu->sreg.c) < 0;
  cpu->sreg.v = ((reg ^ result) & (imm ^ result)) & 0x80;
  cpu->sreg.n = (result & 0x80) != 0;
  cpu->sreg.z = (result == 0);
  cpu->sreg.c = (result > reg) || (result > imm);

  cpu->rf[rd] = result;
}

Sub::Sub()
{
  exec_func = Execute;
  cycles = 1;
  name = "SUB";
}

void Sub::Execute(Cpu *cpu, uint rd, uint rr)
{
  u8 reg = cpu->rf[rd];
  u8 rs1 = cpu->rf[rr];
  u8 result = reg - rs1;

  cpu->sreg.h = ((reg & 0x0F) - (rs1 & 0x0F)) < 0;
  cpu->sreg.v = ((reg ^ result) & (rs1 ^ result)) & 0x80;
  cpu->sreg.n = (result & 0x80) != 0;
  cpu->sreg.z = (result == 0);
  cpu->sreg.c = (result > reg) || (result > rs1);

  cpu->rf[rd] = result;
}

Subi::Subi()
{
  exec_func = Execute;
  cycles = 1;
  name = "SUBI";
}

void Subi::Execute(Cpu *cpu, uint rd, uint imm)
{
  u8 reg = cpu->rf[rd];
  u8 result = reg - imm;

  cpu->sreg.h = ((reg & 0x0F) - (imm & 0x0F)) < 0;
  cpu->sreg.v = ((reg ^ result) & (imm ^ result)) & 0x80;
  cpu->sreg.n = (result & 0x80) != 0;
  cpu->sreg.z = (result == 0);
  cpu->sreg.c = (result > reg) || (result > imm);

  cpu->rf[rd] = result;
}