#include "insn.h"

#include <format>

#include "cpu.h"

string Insn::GetStatementString(u16 inst) {
  return name;
}

std::function<void()> Insn::GenFunc32(u16 insn, u16 imm, Cpu* cpu) {
  assert(false);
}

std::function<void()> Insn0::GenFunc(u16 insn, Cpu* cpu) {
  assert(exec_func != nullptr);
  return std::bind(exec_func, cpu);
}

InsnImm22::InsnImm22() {
  is_32bit = true;
}

std::function<void()> InsnImm22::GenFunc32(u16 insn, u16 imm, Cpu* cpu) {
  assert(exec_func != nullptr);
  u32 adr = InitArgs(insn, imm);
  return std::bind(exec_func, cpu, adr);
}

u32 InsnImm22::InitArgs(u16 insn, u16 imm) {
  u32 adr =  (ExtractBits(insn, 4, 5) << 17) | (ExtractBits(insn, 0, 1) << 16) | imm ;
  return adr;
}

string InsnImm22::GetStatementString(u16 insn, u16 imm) {
  u32 adr = InitArgs(insn, imm);
  return name + " " + std::format("0x{:x}", adr);
}

InsnRImm16::InsnRImm16() {
  is_32bit = true;
}

std::function<void()> InsnRImm16::GenFunc32(u16 insn, u16 imm, Cpu* cpu) {
  assert(exec_func != nullptr);
  auto [rd, adr] = InitArgs(insn, imm);
  return std::bind(exec_func, cpu, rd, adr);
}

std::tuple<uint, uint> InsnRImm16::InitArgs(u16 insn, u16 imm) {
  uint rd = ExtractBits(insn, 4, 5);
  return {rd, imm};
}

string InsnRImm16::GetStatementString(u16 insn, u16 imm) {
  auto [rd, adr] = InitArgs(insn, imm);
  return name + " R" + RegName(rd) + "," + std::format("0x{:x}", adr);
}

std::function<void()> InsnRb::GenFunc(u16 insn, Cpu* cpu) {
  assert(exec_func != nullptr);
  auto [rr, b] = InitArgs(insn);
  return std::bind(exec_func, cpu, rr, b);
}

std::tuple<uint, uint> InsnRb::InitArgs(u16 insn) {
  uint b = ExtractBits(insn, 0, 3);
  uint rr = ExtractBits(insn, 4, 5);

  return {rr, b};
}

string InsnRb::GetStatementString(u16 insn) {
  auto [rr, b] = InitArgs(insn);
  return name + " " + RegName(rr) + "," + std::to_string(b);
}

std::function<void()> InsnRR::GenFunc(u16 insn, Cpu* cpu) {
  assert(exec_func != nullptr);
  auto [rd, rr] = InitArgs(insn);
  return std::bind(exec_func, cpu, rd, rr);
}

std::tuple<uint, uint> InsnRR::InitArgs(u16 insn) {
  uint rd = ExtractBits(insn, 0, 4) | (ExtractBits(insn, 8, 1) << 4);
  uint rr = ExtractBits(insn, 4, 5);

  return {rd, rr};
}

string InsnRR::GetStatementString(u16 insn) {
  auto [rd, rr] = InitArgs(insn);
  return name + " " + RegName(rd) + "," + RegName(rr);
}

std::function<void()> InsnR::GenFunc(u16 insn, Cpu* cpu) {
  assert(exec_func != nullptr);
  auto rd = InitArgs(insn);
  return std::bind(exec_func, cpu, rd);
}

uint InsnR::InitArgs(u16 insn) {
  uint rd = ExtractBits(insn, 4, 5);
  return rd;
}

string InsnR::GetStatementString(u16 insn) {
  auto rd = InitArgs(insn);
  return name + " " + RegName(rd);
}

std::function<void()> InsnRImm6::GenFunc(u16 insn, Cpu* cpu) {
  assert(exec_func != nullptr);
  auto [rd, imm] = InitArgs(insn);
  return std::bind(exec_func, cpu, rd, imm);
}

std::tuple<uint, uint> InsnRImm6::InitArgs(u16 insn) {
  uint rd = ExtractBits(insn, 4, 5);
  uint imm = ExtractBits(insn, 0, 3) | (ExtractBits(insn, 10, 2) << 3) | (ExtractBits(insn, 13, 1) << 5);

  return {rd, imm};
}

std::function<void()> InsnRIo::GenFunc(u16 insn, Cpu* cpu) {
  assert(exec_func != nullptr);
  auto [rd, io] = InitArgs(insn);
  return std::bind(exec_func, cpu, rd, io);
}

std::tuple<uint, uint> InsnRIo::InitArgs(u16 insn) {
  uint rd = ExtractBits(insn, 4, 5);
  uint io = ExtractBits(insn, 0, 4) | (ExtractBits(insn, 9, 2) << 4);

  return {rd, io};
}

std::function<void()> InsnRpImm6::GenFunc(u16 insn, Cpu* cpu) {
  assert(exec_func != nullptr);
  auto [rd, imm] = InitArgs(insn);
  return std::bind(exec_func, cpu, rd, imm);
}

std::tuple<uint, uint> InsnRpImm6::InitArgs(u16 insn) {
  uint rd = ExtractBits(insn, 4, 2);
  uint imm = ExtractBits(insn, 0, 4) | (ExtractBits(insn, 6, 2) << 4);

  return {rd, imm};
}

string InsnRpImm6::GetStatementString(u16 insn) {
  auto [rd, imm] = InitArgs(insn);
  return name + " " + RegName(rd * 2 + 24) + "," + std::to_string(imm);
}

std::function<void()> InsnRpRp::GenFunc(u16 insn, Cpu* cpu) {
  assert(exec_func != nullptr);
  auto [rd, rr] = InitArgs(insn);
  return std::bind(exec_func, cpu, rd, rr);
}

std::tuple<uint, uint> InsnRpRp::InitArgs(u16 insn) {
  uint rr = ExtractBits(insn, 0, 4);
  uint rd = ExtractBits(insn, 4, 4);

  return {rd, rr};
}

string InsnRpRp::GetStatementString(u16 insn) {
  auto [rd, rr] = InitArgs(insn);
  rd *= 2;
  rr *= 2;
  return name + " R" + RegName(rd) + "+1:R" + RegName(rd) + "," + "R" + RegName(rr) + "+1:R" + RegName(rr);
}

std::function<void()> InsnRuImm8::GenFunc(u16 insn, Cpu* cpu) {
  assert(exec_func != nullptr);
  auto [rd, imm] = InitArgs(insn);
  return std::bind(exec_func, cpu, rd, imm);
}

std::tuple<uint, uint> InsnRuImm8::InitArgs(u16 insn) {
  uint rd = ExtractBits(insn, 4, 4) + 16;
  uint imm = ExtractBits(insn, 0, 4) | (ExtractBits(insn, 7, 4) << 4);

  return {rd, imm};
}

string InsnRuImm8::GetStatementString(u16 insn) {
  auto [rd, imm] = InitArgs(insn);
  return name + " " + RegName(rd) + "," + std::format("0x{:x}", imm);
}

std::function<void()> InsnImm12::GenFunc(u16 insn, Cpu* cpu) {
  assert(exec_func != nullptr);
  auto adr = InitArgs(insn);
  return std::bind(exec_func, cpu, adr);
}

uint InsnImm12::InitArgs(u16 insn) {
  uint adr = SignExtend(ExtractBits(insn, 0, 12), 12);
  return adr;
}

string InsnImm12::GetStatementString(u16 insn) {
  uint adr = InitArgs(insn);
  return name + " " + std::format("0x{:x}", adr);
}

std::function<void()> InsnIoB::GenFunc(u16 insn, Cpu* cpu) {
  assert(exec_func != nullptr);
  auto [io, b] = InitArgs(insn);
  return std::bind(exec_func, cpu, io, b);
}

std::tuple<uint, uint> InsnIoB::InitArgs(u16 insn) {
  uint b = ExtractBits(insn, 0, 3);
  uint io = ExtractBits(insn, 3, 4);

  return {io, b};
}

string InsnIoB::GetStatementString(u16 insn) {
  auto [io, b] = InitArgs(insn);
  return name + " " + std::format("0x{:x}", io) + "," + std::to_string(b);
}

std::function<void()> InsnSImm6::GenFunc(u16 insn, Cpu* cpu) {
  assert(exec_func != nullptr);
  auto [sreg, imm] = InitArgs(insn);
  return std::bind(exec_func, cpu, sreg, imm);
}

std::tuple<uint, int> InsnSImm6::InitArgs(u16 insn) {
  uint sreg = ExtractBits(insn, 0, 3);
  int imm = SignExtend(ExtractBits(insn, 3, 6), 6);

  return {sreg, imm};
}

string InsnSImm6::GetStatementString(u16 insn) {
  auto [sreg, imm] = InitArgs(insn);
  return name + " " + std::to_string(sreg) + "," + std::to_string(imm);
}

std::function<void()> InsnS::GenFunc(u16 insn, Cpu* cpu) {
  assert(exec_func != nullptr);
  uint sreg = InitArgs(insn);
  return std::bind(exec_func, cpu, sreg);
}

uint InsnS::InitArgs(u16 insn) {
  uint sreg = ExtractBits(insn, 4, 3);
  return sreg;
}

string InsnS::GetStatementString(u16 insn) {
  auto sreg = InitArgs(insn);
  return name + " " + std::to_string(sreg);
}

Adc::Adc() {
  exec_func = Execute;
  cycles = 1;
  name = "ADC";
}

void Adc::Execute(Cpu* cpu, uint rd, uint rr) {
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

Add::Add() {
  exec_func = Execute;
  cycles = 1;
  name = "ADD";
}

void Add::Execute(Cpu* cpu, uint rd, uint rr) {
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

Adiw::Adiw() {
  exec_func = Execute;
  cycles = 2;
  name = "ADIW";
}

void Adiw::Execute(Cpu* cpu, uint rd, uint imm) {
  u16 reg = (u16)cpu->rf[rd * 2 + 24] | ((u16)cpu->rf[rd * 2 + 25] << 8);
  u16 result = reg + imm;

  cpu->sreg.v = ((reg ^ result) & (imm ^ result)) & 0x8000;
  cpu->sreg.n = (result & 0x8000) != 0;
  cpu->sreg.z = (result == 0);
  cpu->sreg.c = (result < reg) || (result < imm);

  cpu->rf[rd * 2 + 24] = result & 0xFF;
  cpu->rf[rd * 2 + 25] = (result >> 8);
}

And::And() {
  exec_func = Execute;
  cycles = 1;
  name = "AND";
}

void And::Execute(Cpu* cpu, uint rd, uint rr) {
  u8 reg = cpu->rf[rd];
  u8 rs1 = cpu->rf[rr];
  u8 result = reg & rs1;

  cpu->sreg.v = false;
  cpu->sreg.n = (result & 0x80) != 0;
  cpu->sreg.z = (result == 0);

  cpu->rf[rd] = result;
}

Andi::Andi() {
  exec_func = Execute;
  cycles = 1;
  name = "ANDI";
}

void Andi::Execute(Cpu* cpu, uint rd, uint imm) {
  u8 reg = cpu->rf[rd];
  u8 result = reg & imm;

  cpu->sreg.v = false;
  cpu->sreg.n = (result & 0x80) != 0;
  cpu->sreg.z = (result == 0);

  cpu->rf[rd] = result;
}

Asr::Asr() {
  exec_func = Execute;
  cycles = 1;
  name = "ASR";
}

void Asr::Execute(Cpu* cpu, uint rd) {
  u8 reg = cpu->rf[rd];
  u8 result = (reg >> 1) | (reg & 0x80);

  cpu->sreg.n = (result & 0x80) != 0;
  cpu->sreg.z = (result == 0);
  cpu->sreg.c = (reg & 1) != 0;
  cpu->sreg.v = cpu->sreg.v ^ cpu->sreg.c;
  cpu->sreg.s = cpu->sreg.n ^ cpu->sreg.v;

  cpu->rf[rd] = result;
}

Bclr::Bclr() {
  exec_func = Execute;
  cycles = 1;
  name = "BCLR";
}

void Bclr::Execute(Cpu* cpu, uint sreg) {
  switch (sreg) {
    case 0: cpu->sreg.c = false; break;
    case 1: cpu->sreg.z = false; break;
    case 2: cpu->sreg.n = false; break;
    case 3: cpu->sreg.v = false; break;
    case 4: cpu->sreg.s = false; break;
    case 5: cpu->sreg.h = false; break;
    case 6: cpu->sreg.t = false; break;
    case 7: cpu->sreg.i = false; break;
    default: assert(false);
  }
}

Bld::Bld() {
  exec_func = Execute;
  cycles = 1;
  name = "BLD";
}

void Bld::Execute(Cpu* cpu, uint rr, uint b) {
  u8 reg = cpu->rf[rr];
  u8 result = (cpu->sreg.t << b) | (reg & ~(1 << b));
  cpu->rf[rr] = result;
}

Brbc::Brbc() {
  exec_func = Execute;
  cycles = 1;
  name = "BRBC";
}

void Brbc::Execute(Cpu* cpu, uint sreg, int imm) {
  bool condition = false;
  switch (sreg) {
    case 0: condition = !cpu->sreg.c; break;
    case 1: condition = !cpu->sreg.z; break;
    case 2: condition = !cpu->sreg.n; break;
    case 3: condition = !cpu->sreg.v; break;
    case 4: condition = !cpu->sreg.s; break;
    case 5: condition = !cpu->sreg.h; break;
    case 6: condition = !cpu->sreg.t; break;
    case 7: condition = !cpu->sreg.i; break;
    default: assert(false);
  }
  if (condition) {
    cpu->new_pc += imm * 2;
  }
}

Brbs::Brbs() {
  exec_func = Execute;
  cycles = 1;
  name = "BRBS";
}

void Brbs::Execute(Cpu* cpu, uint sreg, int imm) {
  bool condition = false;
  switch (sreg) {
    case 0: condition = cpu->sreg.c; break;
    case 1: condition = cpu->sreg.z; break;
    case 2: condition = cpu->sreg.n; break;
    case 3: condition = cpu->sreg.v; break;
    case 4: condition = cpu->sreg.s; break;
    case 5: condition = cpu->sreg.h; break;
    case 6: condition = cpu->sreg.t; break;
    case 7: condition = cpu->sreg.i; break;
    default: assert(false);
  }
  if (condition) {
    cpu->new_pc += imm * 2;
  }
}

Bst::Bst() {
  exec_func = Execute;
  cycles = 1;
  name = "BST";
}

void Bst::Execute(Cpu* cpu, uint rr, uint b) {
  u8 reg = cpu->rf[rr];
  cpu->sreg.t = (reg >> b) & 1;
}

Break::Break() {
  exec_func = Execute;
  cycles = 1;
  name = "BREAK";
}

void Break::Execute(Cpu* cpu) {
  cpu->break_hit = true;
}

Bset::Bset() {
  exec_func = Execute;
  cycles = 1;
  name = "BSET";
}

void Bset::Execute(Cpu* cpu, uint sreg) {
  switch (sreg) {
    case 0: cpu->sreg.c = true; break;
    case 1: cpu->sreg.z = true; break;
    case 2: cpu->sreg.n = true; break;
    case 3: cpu->sreg.v = true; break;
    case 4: cpu->sreg.s = true; break;
    case 5: cpu->sreg.h = true; break;
    case 6: cpu->sreg.t = true; break;
    case 7: cpu->sreg.i = true; break;
    default: assert(false);
  }
}

Call::Call() {
  exec_func = Execute;
  cycles = 4;
  name = "CALL";
}

void Call::Execute(Cpu* cpu, u32 addr) {
  cpu->MemoryWrite<u16>(cpu->sp, cpu->pc + 2);
  cpu->sp -= 2;
  cpu->pc = addr;
}

Cbi::Cbi() {
  exec_func = Execute;
  cycles = 2;
  name = "CBI";
}

void Cbi::Execute(Cpu* cpu, uint io, uint b) {
  // TODO: Implement.
}

Com::Com() {
  exec_func = Execute;
  cycles = 1;
  name = "COM";
}

void Com::Execute(Cpu* cpu, uint rd) {
  u8 reg = cpu->rf[rd];
  u8 result = ~reg;

  cpu->sreg.v = false;
  cpu->sreg.n = (result & 0x80) != 0;
  cpu->sreg.z = (result == 0);
  cpu->sreg.c = true;

  cpu->rf[rd] = result;
}

Cp::Cp() {
  exec_func = Execute;
  cycles = 1;
  name = "CP";
}

void Cp::Execute(Cpu* cpu, uint rd, uint rr) {
  u8 reg = cpu->rf[rd];
  u8 rs1 = cpu->rf[rr];
  u8 result = reg - rs1;

  cpu->sreg.h = ((reg & 0x0F) - (rs1 & 0x0F)) < 0;
  cpu->sreg.v = ((reg ^ result) & (rs1 ^ result)) & 0x80;
  cpu->sreg.n = (result & 0x80) != 0;
  cpu->sreg.z = (result == 0);
  cpu->sreg.c = (result > reg) || (result > rs1);
}

Cpc::Cpc() {
  exec_func = Execute;
  cycles = 1;
  name = "CPC";
}

void Cpc::Execute(Cpu* cpu, uint rd, uint rr) {
  u8 reg = cpu->rf[rd];
  u8 rs1 = cpu->rf[rr];
  u8 result = reg - rs1 - cpu->sreg.c;

  cpu->sreg.h = ((reg & 0x0F) - (rs1 & 0x0F)) < 0;
  cpu->sreg.v = ((reg ^ result) & (rs1 ^ result)) & 0x80;
  cpu->sreg.n = (result & 0x80) != 0;
  cpu->sreg.z = (result == 0);
  cpu->sreg.c = (result > reg) || (result > rs1);
}

Cpi::Cpi() {
  exec_func = Execute;
  cycles = 1;
  name = "CPI";
}

void Cpi::Execute(Cpu* cpu, uint rd, uint imm) {
  u8 reg = cpu->rf[rd];
  u8 result = reg - imm;

  cpu->sreg.h = ((reg & 0x0F) - (imm & 0x0F)) < 0;
  cpu->sreg.v = ((reg ^ result) & (imm ^ result)) & 0x80;
  cpu->sreg.n = (result & 0x80) != 0;
  cpu->sreg.z = (result == 0);
  cpu->sreg.c = (result > reg) || (result > imm);
}

Cpse::Cpse() {
  exec_func = Execute;
  cycles = 1;
  name = "CPSE";
}

void Cpse::Execute(Cpu* cpu, uint rd, uint rr) {
  cpu->skip = (cpu->rf[rd] == cpu->rf[rr]);
}

Dec::Dec() {
  exec_func = Execute;
  cycles = 1;
  name = "DEC";
}

void Dec::Execute(Cpu* cpu, uint rd) {
  u8 reg = cpu->rf[rd];
  u8 result = reg - 1;

  cpu->sreg.v = (reg == 0x80);
  cpu->sreg.n = (result & 0x80) != 0;
  cpu->sreg.z = (result == 0);

  cpu->rf[rd] = result;
}

Eicall::Eicall() {
  exec_func = Execute;
  cycles = 1;
  name = "EICALL";
}

void Eicall::Execute(Cpu* cpu) {
  throw Cpu::IllegalInsnException(); // Not part of ATmega328p.
}

Eijmp::Eijmp() {
  exec_func = Execute;
  cycles = 1;
  name = "EIJMP";
}

void Eijmp::Execute(Cpu* cpu) {
  throw Cpu::IllegalInsnException(); // Not part of ATmega328p.
}

Eor::Eor() {
  exec_func = Execute;
  cycles = 1;
  name = "EOR";
}

Fmul::Fmul() {
  exec_func = Execute;
  cycles = 2;
  name = "FMUL";
}

void Fmul::Execute(Cpu* cpu, uint rd, uint rr) {
  u8 reg = cpu->rf[rd];
  u8 rs1 = cpu->rf[rr];
  u16 result = (u16)reg * (u16)rs1;

  cpu->sreg.z = (result == 0);
  cpu->sreg.c = result & 0x8000;

  result = result << 1;
  cpu->rf[0] = result & 0xff;
  cpu->rf[1] = result >> 8;
}

Fmuls::Fmuls() {
  exec_func = Execute;
  cycles = 2;
  name = "FMUL";
}

void Fmuls::Execute(Cpu* cpu, uint rd, uint rr) {
  u8 reg = cpu->rf[rd];
  u8 rs1 = cpu->rf[rr];
  u16 result = (i16)(i8)reg * (i16)(i8)rs1;

  cpu->sreg.z = (result == 0);
  cpu->sreg.c = result & 0x8000;

  result = result << 1;
  cpu->rf[0] = result & 0xff;
  cpu->rf[1] = result >> 8;
}

Fmulsu::Fmulsu() {
  exec_func = Execute;
  cycles = 2;
  name = "FMUL";
}

void Fmulsu::Execute(Cpu* cpu, uint rd, uint rr) {
  u8 reg = cpu->rf[rd];
  u8 rs1 = cpu->rf[rr];
  u16 result = (i16)(i8)reg * (u16)rs1;

  cpu->sreg.z = (result == 0);
  cpu->sreg.c = result & 0x8000;

  result = result << 1;
  cpu->rf[0] = result & 0xff;
  cpu->rf[1] = result >> 8;
}

void Eor::Execute(Cpu* cpu, uint rd, uint rr) {
  u8 reg = cpu->rf[rd];
  u8 rs1 = cpu->rf[rr];
  u8 result = reg ^ rs1;

  cpu->sreg.v = false;
  cpu->sreg.n = (result & 0x80) != 0;
  cpu->sreg.z = (result == 0);

  cpu->rf[rd] = result;
}

Icall::Icall() {
  exec_func = Execute;
  cycles = 3;
  name = "ICALL";
}

void Icall::Execute(Cpu* cpu) {
  cpu->MemoryWrite<u16>(cpu->sp, cpu->pc + 1);
  cpu->sp -= 2;
  cpu->pc = cpu->z;
}

Ijmp::Ijmp() {
  exec_func = Execute;
  cycles = 2;
  name = "IJMP";
}

void Ijmp::Execute(Cpu* cpu) {
  cpu->pc = cpu->z;
}

Inc::Inc() {
  exec_func = Execute;
  cycles = 1;
  name = "INC";
}

void Inc::Execute(Cpu* cpu, uint rd) {
  u8 reg = cpu->rf[rd];
  u8 result = reg + 1;

  cpu->sreg.v = (reg == 0x7F);
  cpu->sreg.n = (result & 0x80) != 0;
  cpu->sreg.z = (result == 0);

  cpu->rf[rd] = result;
}

In::In() {
  exec_func = Execute;
  cycles = 1;
  name = "IN";
}

void In::Execute(Cpu* cpu, uint rd, uint io) {
  // TODO: Implement.
  // cpu->rf[rd] = cpu->MemoryReadIo(io);
}

Jmp::Jmp() {
  exec_func = Execute;
  cycles = 3;
  name = "JMP";
}

void Jmp::Execute(Cpu* cpu, u32 addr) {
  cpu->pc = addr;
}

Lac::Lac() {
  exec_func = Execute;
  cycles = 1;
  name = "LAC";
}

void Lac::Execute(Cpu* cpu, uint rd) {
  throw Cpu::IllegalInsnException(); // Not part of ATmega328p.
}

Las::Las() {
  exec_func = Execute;
  cycles = 1;
  name = "LAS";
}

void Las::Execute(Cpu* cpu, uint rd) {
  throw Cpu::IllegalInsnException(); // Not part of ATmega328p.
}

Lat::Lat() {
  exec_func = Execute;
  cycles = 1;
  name = "LAT";
}

void Lat::Execute(Cpu* cpu, uint rd) {
  throw Cpu::IllegalInsnException(); // Not part of ATmega328p.
}

Lddy::Lddy() {
  exec_func = Execute;
  cycles = 2;
  name = "LDD";
}

void Lddy::Execute(Cpu* cpu, uint rd, uint imm) {
  cpu->rf[rd] = cpu->MemoryRead<u8>(cpu->y + imm);  // TODO: Is imm signed or unsigned?
}

string Lddy::GetStatementString(u16 insn) {
  auto [rd, imm] = InitArgs(insn);
  return name + " R" + std::to_string(rd) + ",Y+" + std::format("0x{:x}", imm);
}

Lddz::Lddz() {
  exec_func = Execute;
  cycles = 2;
  name = "LDD";
}

void Lddz::Execute(Cpu* cpu, uint rd, uint imm) {
  cpu->rf[rd] = cpu->MemoryRead<u8>(cpu->z + imm);  // TODO: Is imm signed or unsigned?
}

string Lddz::GetStatementString(u16 insn) {
  auto [rd, imm] = InitArgs(insn);
  return name + " R" + std::to_string(rd) + ",Z+" + std::format("0x{:x}", imm);
}

Ldi::Ldi() {
  exec_func = Execute;
  cycles = 1;
  name = "LDI";
}

void Ldi::Execute(Cpu* cpu, uint rd, uint imm) {
  cpu->rf[rd] = imm;
}

Lds::Lds() {
  exec_func = Execute;
  cycles = 2;
  name = "LD";
}

void Lds::Execute(Cpu* cpu, uint rd, uint adr) {
  cpu->rf[rd] = cpu->MemoryRead<u8>(adr);
}

Ldx1::Ldx1() {
  exec_func = Execute;
  cycles = 2;
  name = "LD";
}

void Ldx1::Execute(Cpu* cpu, uint rd) {
  cpu->rf[rd] = cpu->MemoryRead<u8>(cpu->x);
}

string Ldx1::GetStatementString(u16 insn) {
  uint rd = InitArgs(insn);
  return name + " R" + std::to_string(rd) + "X";
}

Ldx2::Ldx2() {
  exec_func = Execute;
  cycles = 2;
  name = "LD";
}

void Ldx2::Execute(Cpu* cpu, uint rd) {
  cpu->rf[rd] = cpu->MemoryRead<u8>(cpu->x);
  ++cpu->x;
}

string Ldx2::GetStatementString(u16 insn) {
  uint rd = InitArgs(insn);
  return name + " R" + std::to_string(rd) + "X+";
}

Ldx3::Ldx3() {
  exec_func = Execute;
  cycles = 2;
  name = "LD";
}

void Ldx3::Execute(Cpu* cpu, uint rd) {
  --cpu->x;
  cpu->rf[rd] = cpu->MemoryRead<u8>(cpu->x);
}

string Ldx3::GetStatementString(u16 insn) {
  uint rd = InitArgs(insn);
  return name + " R" + std::to_string(rd) + "-X";
}

Ldy2::Ldy2() {
  exec_func = Execute;
  cycles = 2;
  name = "LD";
}

void Ldy2::Execute(Cpu* cpu, uint rd) {
  cpu->rf[rd] = cpu->MemoryRead<u8>(cpu->y);
  ++cpu->y;
}

string Ldy2::GetStatementString(u16 insn) {
  uint rd = InitArgs(insn);
  return name + " R" + std::to_string(rd) + "Y+";
}

Ldy3::Ldy3() {
  exec_func = Execute;
  cycles = 2;
  name = "LD";
}

void Ldy3::Execute(Cpu* cpu, uint rd) {
  --cpu->y;
  cpu->rf[rd] = cpu->MemoryRead<u8>(cpu->y);
}

string Ldy3::GetStatementString(u16 insn) {
  uint rd = InitArgs(insn);
  return name + " R" + std::to_string(rd) + "-Y";
}

Ldz2::Ldz2() {
  exec_func = Execute;
  cycles = 2;
  name = "LD";
}

void Ldz2::Execute(Cpu* cpu, uint rd) {
  cpu->rf[rd] = cpu->MemoryRead<u8>(cpu->z);
  ++cpu->z;
}

string Ldz2::GetStatementString(u16 insn) {
  uint rd = InitArgs(insn);
  return name + " R" + std::to_string(rd) + "Z+";
}

Ldz3::Ldz3() {
  exec_func = Execute;
  cycles = 2;
  name = "LD";
}

void Ldz3::Execute(Cpu* cpu, uint rd) {
  --cpu->z;
  cpu->rf[rd] = cpu->MemoryRead<u8>(cpu->z);
}

string Ldz3::GetStatementString(u16 insn) {
  uint rd = InitArgs(insn);
  return name + " R" + std::to_string(rd) + "-Z";
}

Lpm1::Lpm1() {
  exec_func = Execute;
  cycles = 3;
  name = "LPM";
}

void Lpm1::Execute(Cpu* cpu) {
  cpu->rf[0] = cpu->ProgMemoryRead(cpu->z);
}

Lpm2::Lpm2() {
  exec_func = Execute;
  cycles = 3;
  name = "LPM";
}

void Lpm2::Execute(Cpu* cpu, uint rd) {
  cpu->rf[rd] = cpu->ProgMemoryRead(cpu->z);
}

string Lpm2::GetStatementString(u16 insn) {
  uint rd = InitArgs(insn);
  return name + " R" + std::to_string(rd) + ",Z";
}

Lpmx::Lpmx() {
  exec_func = Execute;
  cycles = 3;
  name = "LPM";
}

void Lpmx::Execute(Cpu* cpu, uint rd) {
  cpu->rf[rd] = cpu->ProgMemoryRead(cpu->z);
  ++cpu->z;
}

string Lpmx::GetStatementString(u16 insn) {
  uint rd = InitArgs(insn);
  return name + " R" + std::to_string(rd) + ",Z+";
}

Lsr::Lsr() {
  exec_func = Execute;
  cycles = 1;
  name = "LSR";
}

void Lsr::Execute(Cpu* cpu, uint rd) {
  u8 reg = cpu->rf[rd];
  u8 result = reg >> 1;

  cpu->sreg.n = false;
  cpu->sreg.z = (result == 0);
  cpu->sreg.c = (reg & 0x01) != 0;
  cpu->sreg.v = cpu->sreg.n ^ cpu->sreg.c;

  cpu->rf[rd] = result;
}

Mov::Mov() {
  exec_func = Execute;
  cycles = 1;
  name = "MOV";
}

void Mov::Execute(Cpu* cpu, uint rd, uint rr) {
  cpu->rf[rd] = cpu->rf[rr];
}

Movw::Movw() {
  exec_func = Execute;
  cycles = 1;
  name = "MOVW";
}

void Movw::Execute(Cpu* cpu, uint rd, uint rr) {
  cpu->rf[2 * rd] = cpu->rf[2 * rr];
  cpu->rf[2 * rd + 1] = cpu->rf[2 * rr + 1];
}

Mul::Mul() {
  exec_func = Execute;
  cycles = 2;
  name = "MUL";
}

void Mul::Execute(Cpu* cpu, uint rd, uint rr) {
  u8 reg = cpu->rf[rd];
  u8 rs1 = cpu->rf[rr];
  u16 result = (u16)reg * (u16)rs1;

  cpu->sreg.z = (result == 0);
  cpu->sreg.c = result & 0x8000;

  cpu->rf[0] = result & 0xff;
  cpu->rf[1] = result >> 8;
}

Muls::Muls() {
  exec_func = Execute;
  cycles = 2;
  name = "MULS";
}

void Muls::Execute(Cpu* cpu, uint rd, uint rr) {
  u8 reg = cpu->rf[rd];
  u8 rs1 = cpu->rf[rr];
  u16 result = (i16)reg * (i16)rs1;

  cpu->sreg.z = (result == 0);
  cpu->sreg.c = result & 0x8000;

  cpu->rf[0] = result & 0xff;
  cpu->rf[1] = result >> 8;
}

Mulsu::Mulsu() {
  exec_func = Execute;
  cycles = 2;
  name = "MULSU";
}

void Mulsu::Execute(Cpu* cpu, uint rd, uint rr) {
  u8 reg = cpu->rf[rd];
  u8 rs1 = cpu->rf[rr];
  u16 result = (i16)(i8)reg * (u16)rs1;

  cpu->sreg.z = (result == 0);
  cpu->sreg.c = result & 0x8000;

  cpu->rf[0] = result & 0xff;
  cpu->rf[1] = result >> 8;
}

Neg::Neg() {
  exec_func = Execute;
  cycles = 1;
  name = "NEG";
}

void Neg::Execute(Cpu* cpu, uint rd) {
  u8 reg = cpu->rf[rd];
  u8 result = -reg;

  cpu->sreg.h = (reg & 0x0F) != 0;
  cpu->sreg.v = (reg == 0x80);
  cpu->sreg.n = (result & 0x80) != 0;
  cpu->sreg.z = result == 0;
  cpu->sreg.c = result != 0;

  cpu->rf[rd] = result;
}

Nop::Nop() {
  exec_func = Execute;
  cycles = 1;
  name = "NOP";
}

void Nop::Execute(Cpu* cpu) {
}

Or::Or() {
  exec_func = Execute;
  cycles = 1;
  name = "OR";
}

void Or::Execute(Cpu* cpu, uint rd, uint rr) {
  u8 reg = cpu->rf[rd];
  u8 rs1 = cpu->rf[rr];
  u8 result = reg | rs1;

  cpu->sreg.v = false;
  cpu->sreg.n = (result & 0x80) != 0;
  cpu->sreg.z = (result == 0);

  cpu->rf[rd] = result;
}

Ori::Ori() {
  exec_func = Execute;
  cycles = 1;
  name = "ORI";
}

void Ori::Execute(Cpu* cpu, uint rd, uint imm) {
  u8 reg = cpu->rf[rd];
  u8 result = reg | imm;

  cpu->sreg.v = false;
  cpu->sreg.n = (result & 0x80) != 0;
  cpu->sreg.z = (result == 0);

  cpu->rf[rd] = result;
}

Out::Out() {
  exec_func = Execute;
  cycles = 1;
  name = "OUT";
}

void Out::Execute(Cpu* cpu, uint rr, uint io) {
  // TODO: Implement.
  // cpu->WriteIo(cpu->rf[rr], io);
}

Pop::Pop() {
  exec_func = Execute;
  cycles = 2;
  name = "POP";
}

void Pop::Execute(Cpu* cpu, uint rd) {
  cpu->sp += 1;
  cpu->rf[rd] = cpu->MemoryRead<u8>(cpu->sp);
}

Push::Push() {
  exec_func = Execute;
  cycles = 2;
  name = "PUSH";
}

void Push::Execute(Cpu* cpu, uint rd) {
  cpu->MemoryWrite<u8>(cpu->sp, cpu->rf[rd]);
  cpu->sp -= 1;
}

Rcall::Rcall() {
  exec_func = Execute;
  cycles = 3;
  name = "RCALL";
}

void Rcall::Execute(Cpu* cpu, uint addr) {
  cpu->MemoryWrite<u16>(cpu->sp, cpu->pc + 2);
  cpu->sp -= 2;
  cpu->pc += addr + 1;
}

Ret::Ret() {
  exec_func = Execute;
  cycles = 4;
  name = "RET";
}

void Ret::Execute(Cpu* cpu) {
  cpu->sp += 2;
  cpu->pc = cpu->MemoryRead<u16>(cpu->sp);
}

Reti::Reti() {
  exec_func = Execute;
  cycles = 4;
  name = "RETI";
}

void Reti::Execute(Cpu* cpu) {
  cpu->sp += 2;
  cpu->pc = cpu->MemoryRead<u16>(cpu->sp);
  cpu->sreg.i = true;
}

Rjmp::Rjmp() {
  exec_func = Execute;
  cycles = 2;
  name = "RJMP";
}

void Rjmp::Execute(Cpu* cpu, u32 addr) {
  cpu->pc += addr + 1;
}

Ror::Ror() {
  exec_func = Execute;
  cycles = 1;
  name = "ROR";
}

void Ror::Execute(Cpu* cpu, uint rd) {
  u8 reg = cpu->rf[rd];
  u8 result = (reg >> 1) | (cpu->sreg.c << 7);

  cpu->sreg.c = reg & 0x01;
  cpu->sreg.n = (result & 0x80) != 0;
  cpu->sreg.z = (result == 0);
  cpu->sreg.v = cpu->sreg.v ^ cpu->sreg.c;
  cpu->sreg.s = cpu->sreg.n ^ cpu->sreg.v;

  cpu->rf[rd] = result;
}

Sbci::Sbci() {
  exec_func = Execute;
  cycles = 1;
  name = "SBCI";
}

void Sbci::Execute(Cpu* cpu, uint rd, uint imm) {
  u8 reg = cpu->rf[rd];
  u8 result = reg - imm - cpu->sreg.c;

  cpu->sreg.h = ((reg & 0x0F) - (imm & 0x0F) - cpu->sreg.c) < 0;
  cpu->sreg.v = ((reg ^ result) & (imm ^ result)) & 0x80;
  cpu->sreg.n = (result & 0x80) != 0;
  cpu->sreg.z = (result == 0);
  cpu->sreg.c = (result > reg) || (result > imm);

  cpu->rf[rd] = result;
}

Sbc::Sbc() {
  exec_func = Execute;
  cycles = 1;
  name = "SBC";
}

void Sbc::Execute(Cpu* cpu, uint rd, uint rr) {
  u8 reg = cpu->rf[rd];
  u8 rs1 = cpu->rf[rr];
  u8 result = reg - rs1 - cpu->sreg.c;

  cpu->sreg.h = ((reg & 0x0F) - (rs1 & 0x0F) - cpu->sreg.c) < 0;
  cpu->sreg.v = ((reg ^ result) & (rs1 ^ result)) & 0x80;
  cpu->sreg.n = (result & 0x80) != 0;
  cpu->sreg.z = (result == 0);
  cpu->sreg.c = (result > reg) || (result > rs1);

  cpu->rf[rd] = result;
}

Sbi::Sbi() {
  exec_func = Execute;
  cycles = 2;
  name = "SBI";
}

void Sbi::Execute(Cpu* cpu, uint io, uint b) {
  // TODO: Implement.
}

Sbic::Sbic() {
  exec_func = Execute;
  cycles = 2;
  name = "SBIC";
}

void Sbic::Execute(Cpu* cpu, uint io, uint b) {
  // TODO: Implement.
}

Sbis::Sbis() {
  exec_func = Execute;
  cycles = 2;
  name = "SBI";
}

void Sbis::Execute(Cpu* cpu, uint io, uint b) {
  // TODO: Implement.
}

Sbiw::Sbiw() {
  exec_func = Execute;
  cycles = 2;
  name = "SBIW";
}

void Sbiw::Execute(Cpu* cpu, uint rp, uint imm) {
  u16 reg = (u16)cpu->rf[rp * 2 + 24] | ((u16)cpu->rf[rp * 2 + 25] << 8);
  u16 result = reg - imm;

  cpu->sreg.n = (result & 0x8000) != 0;
  cpu->sreg.z = (result == 0);
  cpu->sreg.c = (result > reg) || (result > imm);
  cpu->sreg.v = ((reg ^ result) & (imm ^ result)) & 0x8000;
  cpu->sreg.s = cpu->sreg.n ^ cpu->sreg.v;

  cpu->rf[rp * 2 + 24] = result;
  cpu->rf[rp * 2 + 25] = result >> 8;
}

Sbrc::Sbrc() {
  exec_func = Execute;
  cycles = 1;
  name = "SBRC";
}

void Sbrc::Execute(Cpu* cpu, uint rr, uint b) {
  u8 reg = cpu->rf[rr];
  if (!(reg & (1 << b))) {
    cpu->skip = true;
  }
}

Sbrs::Sbrs() {
  exec_func = Execute;
  cycles = 1;
  name = "SBRS";
}

void Sbrs::Execute(Cpu* cpu, uint rr, uint b) {
  u8 reg = cpu->rf[rr];
  if (reg & (1 << b)) {
    cpu->skip = true;
  }
}

Sleep::Sleep() {
  exec_func = Execute;
  cycles = 1;
  name = "SLEEP";
}

void Sleep::Execute(Cpu* cpu) {
  cpu->sleeping = true;
}

Spm::Spm() {
  exec_func = Execute;
  cycles = 1;
  name = "SPM";
}

void Spm::Execute(Cpu* cpu) {
  // TODO: Implement.
}

Spmx::Spmx() {
  exec_func = Execute;
  cycles = 1;
  name = "SPM";
}

void Spmx::Execute(Cpu* cpu) {
  // TODO: Implement.
}

Stdy::Stdy() {
  exec_func = Execute;
  cycles = 2;
  name = "STD";
}

void Stdy::Execute(Cpu* cpu, uint rr, uint imm) {
  cpu->MemoryWrite<u8>(cpu->y + imm, cpu->rf[rr]);  // TODO: Is imm signed or unsigned?
}

string Stdy::GetStatementString(u16 insn) {
  auto [rr, imm] = InitArgs(insn);
  return name + " Y+" + std::format("0x{:x}", imm) + ",R" + std::to_string(rr);
}

Stdz::Stdz() {
  exec_func = Execute;
  cycles = 2;
  name = "STD";
}

void Stdz::Execute(Cpu* cpu, uint rr, uint imm) {
  cpu->MemoryWrite<u8>(cpu->z + imm, cpu->rf[rr]);  // TODO: Is imm signed or unsigned?
}

string Stdz::GetStatementString(u16 insn) {
  auto [rr, imm] = InitArgs(insn);
  return name + " Z+" + std::format("0x{:x}", imm) + ",R" + std::to_string(rr);
}

Sts::Sts() {
  exec_func = Execute;
  cycles = 2;
  name = "STS";
}

void Sts::Execute(Cpu* cpu, uint rr, uint adr) {
  cpu->MemoryWrite<u8>(cpu->rf[rr], adr);
}

Stx1::Stx1() {
  exec_func = Execute;
  cycles = 2;
  name = "ST";
}

void Stx1::Execute(Cpu* cpu, uint rr) {
  cpu->MemoryWrite<u8>(cpu->x, cpu->rf[rr]);
}

string Stx1::GetStatementString(u16 insn) {
  uint rr = InitArgs(insn);
  return name + " X, R" + std::to_string(rr);
}

Stx2::Stx2() {
  exec_func = Execute;
  cycles = 2;
  name = "ST";
}

void Stx2::Execute(Cpu* cpu, uint rr) {
  cpu->MemoryWrite<u8>(cpu->x, cpu->rf[rr]);
  ++cpu->x;
}

string Stx2::GetStatementString(u16 insn) {
  uint rr = InitArgs(insn);
  return name + " X+, R" + std::to_string(rr);
}

Stx3::Stx3() {
  exec_func = Execute;
  cycles = 2;
  name = "ST";
}

void Stx3::Execute(Cpu* cpu, uint rr) {
  --cpu->x;
  cpu->MemoryWrite<u8>(cpu->x, cpu->rf[rr]);
}

string Stx3::GetStatementString(u16 insn) {
  uint rr = InitArgs(insn);
  return name + " X-, R" + std::to_string(rr);
}

Sty2::Sty2() {
  exec_func = Execute;
  cycles = 2;
  name = "ST";
}

void Sty2::Execute(Cpu* cpu, uint rr) {
  cpu->MemoryWrite<u8>(cpu->y, cpu->rf[rr]);
  ++cpu->y;
}

string Sty2::GetStatementString(u16 insn) {
  uint rr = InitArgs(insn);
  return name + " Y+, R" + std::to_string(rr);
}

Sty3::Sty3() {
  exec_func = Execute;
  cycles = 2;
  name = "ST";
}

void Sty3::Execute(Cpu* cpu, uint rr) {
  --cpu->y;
  cpu->MemoryWrite<u8>(cpu->y, cpu->rf[rr]);
}

string Sty3::GetStatementString(u16 insn) {
  uint rr = InitArgs(insn);
  return name + " Y-, R" + std::to_string(rr);
}

Stz2::Stz2() {
  exec_func = Execute;
  cycles = 2;
  name = "ST";
}

void Stz2::Execute(Cpu* cpu, uint rr) {
  cpu->MemoryWrite<u8>(cpu->z, cpu->rf[rr]);
  ++cpu->z;
}

string Stz2::GetStatementString(u16 insn) {
  uint rr = InitArgs(insn);
  return name + " Z+, R" + std::to_string(rr);
}

Stz3::Stz3() {
  exec_func = Execute;
  cycles = 2;
  name = "ST";
}

void Stz3::Execute(Cpu* cpu, uint rr) {
  --cpu->z;
  cpu->MemoryWrite<u8>(cpu->z, cpu->rf[rr]);
}

string Stz3::GetStatementString(u16 insn) {
  uint rr = InitArgs(insn);
  return name + " Z-, R" + std::to_string(rr);
}

Sub::Sub() {
  exec_func = Execute;
  cycles = 1;
  name = "SUB";
}

void Sub::Execute(Cpu* cpu, uint rd, uint rr) {
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

Subi::Subi() {
  exec_func = Execute;
  cycles = 1;
  name = "SUBI";
}

void Subi::Execute(Cpu* cpu, uint rd, uint imm) {
  u8 reg = cpu->rf[rd];
  u8 result = reg - imm;

  cpu->sreg.h = ((reg & 0x0F) - (imm & 0x0F)) < 0;
  cpu->sreg.v = ((reg ^ result) & (imm ^ result)) & 0x80;
  cpu->sreg.n = (result & 0x80) != 0;
  cpu->sreg.z = (result == 0);
  cpu->sreg.c = (result > reg) || (result > imm);

  cpu->rf[rd] = result;
}

Swap::Swap() {
  exec_func = Execute;
  cycles = 1;
  name = "SWAP";
}

void Swap::Execute(Cpu* cpu, uint rd) {
  u8 reg = cpu->rf[rd];
  cpu->rf[rd] = (reg << 4) | (reg >> 4);
}

Xch::Xch() {
  exec_func = Execute;
  cycles = 1;
  name = "XCH";
}

void Xch::Execute(Cpu* cpu, uint rd) {
  throw Cpu::IllegalInsnException(); // Not part of ATmega328p.
}

Wdr::Wdr() {
  exec_func = Execute;
  cycles = 1;
  name = "WDR";
}

void Wdr::Execute(Cpu* cpu) {
  cpu->watchdog_timer = 0;
}

Illegal::Illegal() {
  exec_func = Execute;
  cycles = 1;
  name = "ILLEGAL";
}

void Illegal::Execute(Cpu* cpu) {
  throw Cpu::IllegalInsnException(); // Not part of ATmega328p.
}