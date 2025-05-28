#include "insn.h"

#include <format>

#include "cpu.h"

string Insn::GetStatementString(u16 inst) {
  return name;
}

std::function<void()> Insn::GenFunc32(u32 insn, Cpu* cpu) {
  assert(false);
}

std::function<void()> Insn0::GenFunc(u16 insn, Cpu* cpu) {
  assert(exec_func != nullptr);
  return std::bind(exec_func, cpu);
}

std::function<void()> Insn32::GenFunc32(u32 insn, Cpu* cpu) {
  assert(exec_func != nullptr);
  u32 adr = InitArgs(insn);
  return std::bind(exec_func, cpu, adr);
}

u32 Insn32::InitArgs(u32 insn) {
  u32 adr = ExtractBits(insn, 0, 17) | (ExtractBits(insn, 20, 5) << 17);
  return adr;
}

string Insn32::GetStatementString(u32 insn) {
  u32 adr = InitArgs(insn);
  return string(name) + " " + std::format("0x{:x}", adr);
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
  return string(name) + " " + RegName(rr) + "," + std::to_string(b);
}

std::function<void()> InsnRr::GenFunc(u16 insn, Cpu* cpu) {
  assert(exec_func != nullptr);
  auto [rd, rr] = InitArgs(insn);
  return std::bind(exec_func, cpu, rd, rr);
}

std::tuple<uint, uint> InsnRr::InitArgs(u16 insn) {
  uint rd = ExtractBits(insn, 0, 4) | (ExtractBits(insn, 8, 1) << 4);
  uint rr = ExtractBits(insn, 4, 5);

  return {rd, rr};
}

string InsnRr::GetStatementString(u16 insn) {
  auto [rd, rr] = InitArgs(insn);
  return string(name) + " " + RegName(rd) + "," + RegName(rr);
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
  return string(name) + " " + RegName(rd);
}

std::function<void()> InsnImm6::GenFunc(u16 insn, Cpu* cpu) {
  assert(exec_func != nullptr);
  auto [rd, imm] = InitArgs(insn);
  return std::bind(exec_func, cpu, rd, imm);
}

std::tuple<uint, uint> InsnImm6::InitArgs(u16 insn) {
  uint rd = ExtractBits(insn, 4, 2);
  uint imm = ExtractBits(insn, 0, 4) | (ExtractBits(insn, 6, 2) << 4);

  return {rd, imm};
}

string InsnImm6::GetStatementString(u16 insn) {
  auto [rd, imm] = InitArgs(insn);
  return string(name) + " " + RegName(rd * 2 + 24) + "," + std::to_string(imm);
}

std::function<void()> InsnImm8::GenFunc(u16 insn, Cpu* cpu) {
  assert(exec_func != nullptr);
  auto [rd, imm] = InitArgs(insn);
  return std::bind(exec_func, cpu, rd, imm);
}

std::tuple<uint, uint> InsnImm8::InitArgs(u16 insn) {
  uint rd = ExtractBits(insn, 4, 4);
  uint imm = ExtractBits(insn, 0, 4) | (ExtractBits(insn, 7, 4) << 4);

  return {rd, imm};
}

string InsnImm8::GetStatementString(u16 insn) {
  auto [rd, imm] = InitArgs(insn);
  return string(name) + " " + RegName(rd) + "," + std::format("0x{:x}", imm);
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
  return string(name) + " " + std::format("0x{:x}", adr);
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

  cpu->rf[rd] = result;
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

Break::Break() {
  exec_func = Execute;
  cycles = 1;
  name = "BREAK";
}

void Break::Execute(Cpu* cpu) {
  cpu->break_hit = true;
}

Call::Call() {
  exec_func = Execute;
  is_32bit = true;
  cycles = 4;
  name = "CALL";
}

void Call::Execute(Cpu* cpu, u32 addr) {
  cpu->MemoryWrite<u16>(cpu->sp, cpu->pc + 2);
  cpu->sp -= 2;
  cpu->pc = addr;
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

Jmp::Jmp() {
  exec_func = Execute;
  cycles = 3;
  is_32bit = true;
  name = "JMP";
}

void Jmp::Execute(Cpu* cpu, u32 addr) {
  cpu->pc = addr;
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

Wdr::Wdr() {
  exec_func = Execute;
  cycles = 1;
  name = "WDR";
}

void Wdr::Execute(Cpu* cpu) {
  cpu->watchdog_timer = 0;
}