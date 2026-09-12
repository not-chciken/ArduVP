#include "insn.h"

#include "cpu.h"

namespace Insn {

void Adc::Execute(Cpu* cpu, const InsnArgs& a) {
  const uint rd = a.rd_rr.rd;
  const uint rr = a.rd_rr.rr;

  u8 reg = cpu->rf[rd];
  u8 rs1 = cpu->rf[rr];
  u8 carry_in = cpu->sreg.c;
  u16 sum = (u16)reg + (u16)rs1 + carry_in;
  u8 result = sum;

  cpu->sreg.h = ((reg & 0x0f) + (rs1 & 0x0f) + carry_in) > 0x0f;
  cpu->sreg.n = (result & 0x80) != 0;
  cpu->sreg.z = (result == 0);
  cpu->sreg.c = sum > 0xff;
  cpu->sreg.v = (((reg ^ result) & (rs1 ^ result)) & 0x80) != 0;
  cpu->sreg.s = cpu->sreg.n ^ cpu->sreg.v;

  cpu->rf[rd] = result;
}

void Add::Execute(Cpu* cpu, const InsnArgs& a) {
  const uint rd = a.rd_rr.rd;
  const uint rr = a.rd_rr.rr;

  u8 reg = cpu->rf[rd];
  u8 rs1 = cpu->rf[rr];
  u8 result = reg + rs1;

  cpu->sreg.h = ((reg & 0x0f) + (rs1 & 0x0f)) > 0x0f;
  cpu->sreg.v = (bool)(((reg ^ result) & (rs1 ^ result)) & 0x80);
  cpu->sreg.n = (bool)(result & 0x80);
  cpu->sreg.z = (result == 0);
  cpu->sreg.c = (result < reg) || (result < rs1);
  cpu->sreg.s = cpu->sreg.n ^ cpu->sreg.v;

  cpu->rf[rd] = result;
}

void Adiw::Execute(Cpu* cpu, const InsnArgs& a) {
  const uint rd = a.rd_imm.rd;
  const uint imm = a.rd_imm.imm;

  u16 reg = (u16)cpu->rf[rd] | ((u16)cpu->rf[rd + 1] << 8);
  u16 result = reg + imm;

  cpu->sreg.v = ((reg ^ result) & (imm ^ result)) & 0x8000;
  cpu->sreg.n = (result & 0x8000) != 0;
  cpu->sreg.z = (result == 0);
  cpu->sreg.c = (result < reg) || (result < imm);
  cpu->sreg.s = cpu->sreg.n ^ cpu->sreg.v;

  cpu->rf[rd] = result & 0xff;
  cpu->rf[rd + 1] = (result >> 8);
}

void And::Execute(Cpu* cpu, const InsnArgs& a) {
  const uint rd = a.rd_rr.rd;
  const uint rr = a.rd_rr.rr;

  u8 reg = cpu->rf[rd];
  u8 rs1 = cpu->rf[rr];
  u8 result = reg & rs1;

  cpu->sreg.v = false;
  cpu->sreg.n = (result & 0x80) != 0;
  cpu->sreg.z = (result == 0);
  cpu->sreg.s = cpu->sreg.n ^ cpu->sreg.v;

  cpu->rf[rd] = result;
}

void Andi::Execute(Cpu* cpu, const InsnArgs& a) {
  const uint rd = a.rd_imm.rd;
  const uint imm = a.rd_imm.imm;

  u8 reg = cpu->rf[rd];
  u8 result = reg & imm;

  cpu->sreg.v = false;
  cpu->sreg.n = (result & 0x80) != 0;
  cpu->sreg.z = (result == 0);
  cpu->sreg.s = cpu->sreg.n ^ cpu->sreg.v;

  cpu->rf[rd] = result;
}

void Asr::Execute(Cpu* cpu, const InsnArgs& a) {
  const uint rd = a.rd.rd;

  u8 reg = cpu->rf[rd];
  u8 result = (reg >> 1) | (reg & 0x80);

  cpu->sreg.n = (result & 0x80) != 0;
  cpu->sreg.z = (result == 0);
  cpu->sreg.c = (reg & 1) != 0;
  cpu->sreg.v = cpu->sreg.n ^ cpu->sreg.c;
  cpu->sreg.s = cpu->sreg.n ^ cpu->sreg.v;

  cpu->rf[rd] = result;
}

void Bclr::Execute(Cpu* cpu, const InsnArgs& a) {
  const uint sreg = a.bit.bit;

  switch (sreg) {
  case 0:
    cpu->sreg.c = false;
    break;
  case 1:
    cpu->sreg.z = false;
    break;
  case 2:
    cpu->sreg.n = false;
    break;
  case 3:
    cpu->sreg.v = false;
    break;
  case 4:
    cpu->sreg.s = false;
    break;
  case 5:
    cpu->sreg.h = false;
    break;
  case 6:
    cpu->sreg.t = false;
    break;
  case 7:
    cpu->sreg.i = false;
    break;
  default:
    assert(false);
  }
}

void Bld::Execute(Cpu* cpu, const InsnArgs& a) {
  const uint rr = a.rd_bit.rd;
  const uint b = a.rd_bit.bit;

  u8 reg = cpu->rf[rr];
  u8 result = (cpu->sreg.t << b) | (reg & ~(1 << b));
  cpu->rf[rr] = result;
}

void Brbc::Execute(Cpu* cpu, const InsnArgs& a) {
  const uint sreg = a.imm_bit.bit;
  const int imm = a.imm_bit.imm;

  bool condition = false;
  switch (sreg) {
  case 0:
    condition = !cpu->sreg.c;
    break;
  case 1:
    condition = !cpu->sreg.z;
    break;
  case 2:
    condition = !cpu->sreg.n;
    break;
  case 3:
    condition = !cpu->sreg.v;
    break;
  case 4:
    condition = !cpu->sreg.s;
    break;
  case 5:
    condition = !cpu->sreg.h;
    break;
  case 6:
    condition = !cpu->sreg.t;
    break;
  case 7:
    condition = !cpu->sreg.i;
    break;
  default:
    assert(false);
  }
  if (condition) {
    cpu->new_pc = cpu->pc + imm + 1;
    cpu->branch_taken = true;
  }
}

void Brbs::Execute(Cpu* cpu, const InsnArgs& a) {
  const uint sreg = a.imm_bit.bit;
  const int imm = a.imm_bit.imm;

  bool condition = false;
  switch (sreg) {
  case 0:
    condition = cpu->sreg.c;
    break;
  case 1:
    condition = cpu->sreg.z;
    break;
  case 2:
    condition = cpu->sreg.n;
    break;
  case 3:
    condition = cpu->sreg.v;
    break;
  case 4:
    condition = cpu->sreg.s;
    break;
  case 5:
    condition = cpu->sreg.h;
    break;
  case 6:
    condition = cpu->sreg.t;
    break;
  case 7:
    condition = cpu->sreg.i;
    break;
  default:
    assert(false);
  }
  if (condition) {
    cpu->new_pc = cpu->pc + imm + 1;
    cpu->branch_taken = true;
  }
}

void Bst::Execute(Cpu* cpu, const InsnArgs& a) {
  const uint rr = a.rd_bit.rd;
  const uint b = a.rd_bit.bit;

  u8 reg = cpu->rf[rr];
  cpu->sreg.t = (reg >> b) & 1;
}

void Break::Execute(Cpu* cpu, const InsnArgs&) {
  cpu->break_hit = true;
}

void Bset::Execute(Cpu* cpu, const InsnArgs& a) {
  const uint sreg = a.bit.bit;

  switch (sreg) {
  case 0:
    cpu->sreg.c = true;
    break;
  case 1:
    cpu->sreg.z = true;
    break;
  case 2:
    cpu->sreg.n = true;
    break;
  case 3:
    cpu->sreg.v = true;
    break;
  case 4:
    cpu->sreg.s = true;
    break;
  case 5:
    cpu->sreg.h = true;
    break;
  case 6:
    cpu->sreg.t = true;
    break;
  case 7:
    if (!cpu->sreg.i) {
      cpu->sreg.i = true;
      cpu->irq_sei_delay = true;
    }
    break;
  default:
    assert(false);
  }
}

void Call::Execute(Cpu* cpu, const InsnArgs& a) {
  const u32 addr = a.imm.imm;

  u16 old_pc = cpu->pc + 2;
  cpu->MemoryWrite(cpu->sp, old_pc >> 8);
  --cpu->sp;
  cpu->MemoryWrite(cpu->sp, old_pc & 0xff);
  --cpu->sp;
  cpu->new_pc = addr;
}

void Cbi::Execute(Cpu* cpu, const InsnArgs& a) {
  const uint io = a.reg_bit.reg;
  const uint b = a.reg_bit.bit;

  cpu->IoWrite(io, cpu->IoRead(io) & ~(1 << b));
}

void Com::Execute(Cpu* cpu, const InsnArgs& a) {
  const uint rd = a.rd.rd;

  u8 reg = cpu->rf[rd];
  u8 result = ~reg;

  cpu->sreg.v = false;
  cpu->sreg.n = (result & 0x80) != 0;
  cpu->sreg.z = (result == 0);
  cpu->sreg.c = true;
  cpu->sreg.s = cpu->sreg.n ^ cpu->sreg.v;

  cpu->rf[rd] = result;
}

void Cp::Execute(Cpu* cpu, const InsnArgs& a) {
  const uint rd = a.rd_rr.rd;
  const uint rr = a.rd_rr.rr;

  u8 reg = cpu->rf[rd];
  u8 rs1 = cpu->rf[rr];
  u8 result = reg - rs1;

  cpu->sreg.h = ((int)(reg & 0x0f) - (int)(rs1 & 0x0f)) < 0;
  cpu->sreg.v = ((reg ^ rs1) & (reg ^ result)) & 0x80;
  cpu->sreg.n = (result & 0x80) != 0;
  cpu->sreg.z = (result == 0);
  cpu->sreg.c = (rs1 > reg);
  cpu->sreg.s = cpu->sreg.n ^ cpu->sreg.v;
}

void Cpc::Execute(Cpu* cpu, const InsnArgs& a) {
  const uint rd = a.rd_rr.rd;
  const uint rr = a.rd_rr.rr;

  u8 reg = cpu->rf[rd];
  u8 rs1 = cpu->rf[rr];
  u8 carry_in = cpu->sreg.c;
  u8 result = reg - rs1 - carry_in;

  cpu->sreg.h = ((int)(reg & 0x0f) - (int)(rs1 & 0x0f) - (int)carry_in) < 0;
  cpu->sreg.v = ((reg ^ rs1) & (reg ^ result)) & 0x80;
  cpu->sreg.n = (result & 0x80) != 0;
  cpu->sreg.z = (result == 0) ? cpu->sreg.z : false;
  cpu->sreg.c = ((uint)rs1 + carry_in) > reg;
  cpu->sreg.s = cpu->sreg.n ^ cpu->sreg.v;
}

void Cpi::Execute(Cpu* cpu, const InsnArgs& a) {
  const uint rd = a.rd_imm.rd;
  const uint imm = a.rd_imm.imm;

  u8 reg = cpu->rf[rd];
  u8 result = reg - imm;

  cpu->sreg.h = ((int)(reg & 0x0f) - (int)(imm & 0x0f)) < 0;
  cpu->sreg.v = ((reg ^ imm) & (reg ^ result)) & 0x80;
  cpu->sreg.n = (result & 0x80) != 0;
  cpu->sreg.z = (result == 0);
  cpu->sreg.c = imm > reg;
  cpu->sreg.s = cpu->sreg.n ^ cpu->sreg.v;
}

void Cpse::Execute(Cpu* cpu, const InsnArgs& a) {
  const uint rd = a.rd_rr.rd;
  const uint rr = a.rd_rr.rr;

  cpu->skip = (cpu->rf[rd] == cpu->rf[rr]);
}

void Dec::Execute(Cpu* cpu, const InsnArgs& a) {
  const uint rd = a.rd.rd;

  u8 reg = cpu->rf[rd];
  u8 result = reg - 1;

  cpu->sreg.v = (reg == 0x80);
  cpu->sreg.n = (result & 0x80) != 0;
  cpu->sreg.z = (result == 0);
  cpu->sreg.s = cpu->sreg.n ^ cpu->sreg.v;

  cpu->rf[rd] = result;
}

void Eicall::Execute(Cpu* cpu, const InsnArgs&) {
  throw Cpu::IllegalInsnException();  // Not part of ATmega328p.
}

void Eijmp::Execute(Cpu* cpu, const InsnArgs&) {
  throw Cpu::IllegalInsnException();  // Not part of ATmega328p.
}

void Eor::Execute(Cpu* cpu, const InsnArgs& a) {
  const uint rd = a.rd_rr.rd;
  const uint rr = a.rd_rr.rr;

  u8 reg = cpu->rf[rd];
  u8 rs1 = cpu->rf[rr];
  u8 result = reg ^ rs1;

  cpu->sreg.v = false;
  cpu->sreg.n = (result & 0x80) != 0;
  cpu->sreg.z = (result == 0);
  cpu->sreg.s = cpu->sreg.n ^ cpu->sreg.v;

  cpu->rf[rd] = result;
}

void Fmul::Execute(Cpu* cpu, const InsnArgs& a) {
  const uint rd = a.rd_rr.rd;
  const uint rr = a.rd_rr.rr;

  u8 reg = cpu->rf[rd];
  u8 rs1 = cpu->rf[rr];
  u16 result = (u16)reg * (u16)rs1;

  cpu->sreg.z = (result == 0);
  cpu->sreg.c = (result & 0x8000) != 0;

  result = result << 1;
  cpu->rf[0] = result & 0xff;
  cpu->rf[1] = result >> 8;
}

void Fmuls::Execute(Cpu* cpu, const InsnArgs& a) {
  const uint rd = a.rd_rr.rd;
  const uint rr = a.rd_rr.rr;

  u8 reg = cpu->rf[rd];
  u8 rs1 = cpu->rf[rr];
  u16 result = (i16)(i8)reg * (i16)(i8)rs1;

  cpu->sreg.z = (result == 0);
  cpu->sreg.c = (result & 0x8000) != 0;

  result = result << 1;
  cpu->rf[0] = result & 0xff;
  cpu->rf[1] = result >> 8;
}

void Fmulsu::Execute(Cpu* cpu, const InsnArgs& a) {
  const uint rd = a.rd_rr.rd;
  const uint rr = a.rd_rr.rr;

  u8 reg = cpu->rf[rd];
  u8 rs1 = cpu->rf[rr];
  u16 result = (i16)(i8)reg * (u16)rs1;

  cpu->sreg.z = (result == 0);
  cpu->sreg.c = (result & 0x8000) != 0;

  result = result << 1;
  cpu->rf[0] = result & 0xff;
  cpu->rf[1] = result >> 8;
}

void Icall::Execute(Cpu* cpu, const InsnArgs&) {
  auto new_pc = cpu->pc + 1;
  cpu->MemoryWrite(cpu->sp, new_pc >> 8);
  cpu->sp -= 1;
  cpu->MemoryWrite(cpu->sp, new_pc & 0xff);
  cpu->sp -= 1;
  cpu->new_pc = cpu->z;
}

void Ijmp::Execute(Cpu* cpu, const InsnArgs&) {
  cpu->new_pc = cpu->z;
}

void Inc::Execute(Cpu* cpu, const InsnArgs& a) {
  const uint rd = a.rd.rd;

  u8 reg = cpu->rf[rd];
  u8 result = reg + 1;

  cpu->sreg.v = (reg == 0x7f);
  cpu->sreg.n = (result & 0x80) != 0;
  cpu->sreg.z = (result == 0);
  cpu->sreg.s = cpu->sreg.n ^ cpu->sreg.v;

  cpu->rf[rd] = result;
}

void In::Execute(Cpu* cpu, const InsnArgs& a) {
  const uint rd = a.rd_imm.rd;
  const uint io = a.rd_imm.imm;

  cpu->rf[rd] = cpu->IoRead(io);
}

void Jmp::Execute(Cpu* cpu, const InsnArgs& a) {
  const u32 addr = a.imm.imm;

  cpu->new_pc = addr;
}

void Lac::Execute(Cpu* cpu, const InsnArgs&) {
  throw Cpu::IllegalInsnException();  // Not part of ATmega328p.
}

void Las::Execute(Cpu* cpu, const InsnArgs&) {
  throw Cpu::IllegalInsnException();  // Not part of ATmega328p.
}

void Lat::Execute(Cpu* cpu, const InsnArgs&) {
  throw Cpu::IllegalInsnException();  // Not part of ATmega328p.
}

void Lddy::Execute(Cpu* cpu, const InsnArgs& a) {
  const uint rd = a.rd_imm.rd;
  const uint imm = a.rd_imm.imm;

  cpu->rf[rd] = cpu->MemoryRead(cpu->y + imm);  // TODO: Is imm signed or unsigned?
}

string Lddy::GetStatementString(const InsnArgs& a) {
  return std::format("{} R{},Y+0x{:x}", kName, a.rd_imm.rd, a.rd_imm.imm);
}

void Lddz::Execute(Cpu* cpu, const InsnArgs& a) {
  const uint rd = a.rd_imm.rd;
  const uint imm = a.rd_imm.imm;

  cpu->rf[rd] = cpu->MemoryRead(cpu->z + imm);  // TODO: Is imm signed or unsigned?
}

string Lddz::GetStatementString(const InsnArgs& a) {
  return std::format("{} R{},Z+0x{:x}", kName, a.rd_imm.rd, a.rd_imm.imm);
}

void Ldi::Execute(Cpu* cpu, const InsnArgs& a) {
  const uint rd = a.rd_imm.rd;
  const uint imm = a.rd_imm.imm;

  cpu->rf[rd] = imm;
}

void Lds::Execute(Cpu* cpu, const InsnArgs& a) {
  const uint rd = a.rd_imm.rd;
  const uint adr = a.rd_imm.imm;

  cpu->rf[rd] = cpu->MemoryRead(adr);
}

void Ldx1::Execute(Cpu* cpu, const InsnArgs& a) {
  const uint rd = a.rd.rd;

  cpu->rf[rd] = cpu->MemoryRead(cpu->x);
}

string Ldx1::GetStatementString(const InsnArgs& a) {
  return std::format("{} R{},X", kName, a.rd.rd);
}

void Ldx2::Execute(Cpu* cpu, const InsnArgs& a) {
  const uint rd = a.rd.rd;

  cpu->rf[rd] = cpu->MemoryRead(cpu->x);
  ++cpu->x;
}

string Ldx2::GetStatementString(const InsnArgs& a) {
  return std::format("{} R{},X+", kName, a.rd.rd);
}

void Ldx3::Execute(Cpu* cpu, const InsnArgs& a) {
  const uint rd = a.rd.rd;

  --cpu->x;
  cpu->rf[rd] = cpu->MemoryRead(cpu->x);
}

string Ldx3::GetStatementString(const InsnArgs& a) {
  return std::format("{} R{},-X", kName, a.rd.rd);
}

void Ldy2::Execute(Cpu* cpu, const InsnArgs& a) {
  const uint rd = a.rd.rd;

  cpu->rf[rd] = cpu->MemoryRead(cpu->y);
  ++cpu->y;
}

string Ldy2::GetStatementString(const InsnArgs& a) {
  return std::format("{} R{},Y+", kName, a.rd.rd);
}

void Ldy3::Execute(Cpu* cpu, const InsnArgs& a) {
  const uint rd = a.rd.rd;

  --cpu->y;
  cpu->rf[rd] = cpu->MemoryRead(cpu->y);
}

string Ldy3::GetStatementString(const InsnArgs& a) {
  return std::format("{} R{},-Y", kName, a.rd.rd);
}

void Ldz2::Execute(Cpu* cpu, const InsnArgs& a) {
  const uint rd = a.rd.rd;

  cpu->rf[rd] = cpu->MemoryRead(cpu->z);
  ++cpu->z;
}

string Ldz2::GetStatementString(const InsnArgs& a) {
  return std::format("{} R{},Z+", kName, a.rd.rd);
}

void Ldz3::Execute(Cpu* cpu, const InsnArgs& a) {
  const uint rd = a.rd.rd;

  --cpu->z;
  cpu->rf[rd] = cpu->MemoryRead(cpu->z);
}

string Ldz3::GetStatementString(const InsnArgs& a) {
  return std::format("{} R{},-Z", kName, a.rd.rd);
}

void Lpm1::Execute(Cpu* cpu, const InsnArgs&) {
  cpu->rf[0] = cpu->ProgMemoryRead(cpu->z);
}

void Lpm2::Execute(Cpu* cpu, const InsnArgs& a) {
  const uint rd = a.rd.rd;

  cpu->rf[rd] = cpu->ProgMemoryRead(cpu->z);
}

string Lpm2::GetStatementString(const InsnArgs& a) {
  return std::format("{} R{},Z", kName, a.rd.rd);
}

void Lpmx::Execute(Cpu* cpu, const InsnArgs& a) {
  const uint rd = a.rd.rd;

  cpu->rf[rd] = cpu->ProgMemoryRead(cpu->z);
  ++cpu->z;
}

string Lpmx::GetStatementString(const InsnArgs& a) {
  return std::format("{} R{},Z+", kName, a.rd.rd);
}

void Lsr::Execute(Cpu* cpu, const InsnArgs& a) {
  const uint rd = a.rd.rd;

  u8 reg = cpu->rf[rd];
  u8 result = reg >> 1;

  cpu->sreg.n = false;
  cpu->sreg.z = (result == 0);
  cpu->sreg.c = (reg & 0x01) != 0;
  cpu->sreg.v = cpu->sreg.n ^ cpu->sreg.c;
  cpu->sreg.s = cpu->sreg.n ^ cpu->sreg.v;

  cpu->rf[rd] = result;
}

void Mov::Execute(Cpu* cpu, const InsnArgs& a) {
  const uint rd = a.rd_rr.rd;
  const uint rr = a.rd_rr.rr;

  cpu->rf[rd] = cpu->rf[rr];
}

void Movw::Execute(Cpu* cpu, const InsnArgs& a) {
  const uint rd = a.rd_rr.rd;
  const uint rr = a.rd_rr.rr;

  cpu->rf[rd] = cpu->rf[rr];
  cpu->rf[rd + 1] = cpu->rf[rr + 1];
}

string Movw::GetStatementString(const InsnArgs& a) {
  return std::format("{} {}+1:{},{}+1:{}", kName, RegName(a.rd_rr.rd), RegName(a.rd_rr.rd), RegName(a.rd_rr.rr),
                     RegName(a.rd_rr.rr));
}

void Mul::Execute(Cpu* cpu, const InsnArgs& a) {
  const uint rd = a.rd_rr.rd;
  const uint rr = a.rd_rr.rr;

  u8 reg = cpu->rf[rd];
  u8 rs1 = cpu->rf[rr];
  u16 result = (u16)reg * (u16)rs1;

  cpu->sreg.z = (result == 0);
  cpu->sreg.c = (result & 0x8000) != 0;

  cpu->rf[0] = result & 0xff;
  cpu->rf[1] = result >> 8;
}

void Muls::Execute(Cpu* cpu, const InsnArgs& a) {
  const uint rd = a.rd_rr.rd;
  const uint rr = a.rd_rr.rr;

  u8 reg = cpu->rf[rd];
  u8 rs1 = cpu->rf[rr];
  u16 result = (i16)(i8)reg * (i16)(i8)rs1;

  cpu->sreg.z = (result == 0);
  cpu->sreg.c = (result & 0x8000) != 0;

  cpu->rf[0] = result & 0xff;
  cpu->rf[1] = result >> 8;
}

void Mulsu::Execute(Cpu* cpu, const InsnArgs& a) {
  const uint rd = a.rd_rr.rd;
  const uint rr = a.rd_rr.rr;

  u8 reg = cpu->rf[rd];
  u8 rs1 = cpu->rf[rr];
  u16 result = (i16)(i8)reg * (u16)rs1;

  cpu->sreg.z = (result == 0);
  cpu->sreg.c = (result & 0x8000) != 0;

  cpu->rf[0] = result & 0xff;
  cpu->rf[1] = result >> 8;
}

void Neg::Execute(Cpu* cpu, const InsnArgs& a) {
  const uint rd = a.rd.rd;

  u8 reg = cpu->rf[rd];
  u8 result = -reg;

  cpu->sreg.h = (reg & 0x0f) != 0;
  cpu->sreg.v = (reg == 0x80);
  cpu->sreg.n = (result & 0x80) != 0;
  cpu->sreg.z = result == 0;
  cpu->sreg.c = result != 0;
  cpu->sreg.s = cpu->sreg.n ^ cpu->sreg.v;

  cpu->rf[rd] = result;
}

void Nop::Execute(Cpu* cpu, const InsnArgs&) {
}

void Or::Execute(Cpu* cpu, const InsnArgs& a) {
  const uint rd = a.rd_rr.rd;
  const uint rr = a.rd_rr.rr;

  u8 reg = cpu->rf[rd];
  u8 rs1 = cpu->rf[rr];
  u8 result = reg | rs1;

  cpu->sreg.v = false;
  cpu->sreg.n = (result & 0x80) != 0;
  cpu->sreg.z = (result == 0);
  cpu->sreg.s = cpu->sreg.n ^ cpu->sreg.v;

  cpu->rf[rd] = result;
}

void Ori::Execute(Cpu* cpu, const InsnArgs& a) {
  const uint rd = a.rd_imm.rd;
  const uint imm = a.rd_imm.imm;

  u8 reg = cpu->rf[rd];
  u8 result = reg | imm;

  cpu->sreg.v = false;
  cpu->sreg.n = (result & 0x80) != 0;
  cpu->sreg.z = (result == 0);
  cpu->sreg.s = cpu->sreg.n ^ cpu->sreg.v;

  cpu->rf[rd] = result;
}

void Out::Execute(Cpu* cpu, const InsnArgs& a) {
  const uint rr = a.rd_imm.rd;
  const uint io = a.rd_imm.imm;

  cpu->IoWrite(io, cpu->rf[rr]);
}

void Pop::Execute(Cpu* cpu, const InsnArgs& a) {
  const uint rd = a.rd.rd;

  cpu->sp += 1;
  cpu->rf[rd] = cpu->MemoryRead(cpu->sp);
}

void Push::Execute(Cpu* cpu, const InsnArgs& a) {
  const uint rd = a.rd.rd;

  cpu->MemoryWrite(cpu->sp, cpu->rf[rd]);
  cpu->sp -= 1;
}

void Rcall::Execute(Cpu* cpu, const InsnArgs& a) {
  const uint addr = a.imm.imm;

  cpu->MemoryWrite(cpu->sp, (cpu->pc + 1) >> 8);
  cpu->sp -= 1;
  cpu->MemoryWrite(cpu->sp, (cpu->pc + 1) & 0xff);
  cpu->sp -= 1;
  cpu->new_pc = cpu->pc + addr + 1;
}

void Ret::Execute(Cpu* cpu, const InsnArgs&) {
  cpu->sp += 1;
  u16 new_pc = cpu->MemoryRead(cpu->sp);
  cpu->sp += 1;
  new_pc |= ((u16)cpu->MemoryRead(cpu->sp)) << 8;
  cpu->new_pc = new_pc;
}

void Reti::Execute(Cpu* cpu, const InsnArgs&) {
  cpu->sp += 1;
  u16 new_pc = cpu->MemoryRead(cpu->sp);
  cpu->sp += 1;
  new_pc |= cpu->MemoryRead(cpu->sp) << 8;
  cpu->new_pc = new_pc;
  cpu->sreg.i = true;
  cpu->irq_sei_delay = false;
}

void Rjmp::Execute(Cpu* cpu, const InsnArgs& a) {
  const u32 addr = a.imm.imm;

  cpu->new_pc = cpu->pc + addr + 1;
}

void Ror::Execute(Cpu* cpu, const InsnArgs& a) {
  const uint rd = a.rd.rd;

  u8 reg = cpu->rf[rd];
  u8 result = (reg >> 1) | (cpu->sreg.c << 7);

  cpu->sreg.c = reg & 0x01;
  cpu->sreg.n = (result & 0x80) != 0;
  cpu->sreg.z = (result == 0);
  cpu->sreg.v = cpu->sreg.n ^ cpu->sreg.c;
  cpu->sreg.s = cpu->sreg.n ^ cpu->sreg.v;

  cpu->rf[rd] = result;
}

void Sbci::Execute(Cpu* cpu, const InsnArgs& a) {
  const uint rd = a.rd_imm.rd;
  const uint imm = a.rd_imm.imm;

  u8 reg = cpu->rf[rd];
  u8 result = reg - imm - cpu->sreg.c;

  cpu->sreg.h = ((int)(reg & 0x0f) - (int)(imm & 0x0f) - (int)cpu->sreg.c) < 0;
  cpu->sreg.v = ((reg ^ imm) & (reg ^ result)) & 0x80;
  cpu->sreg.n = (result & 0x80) != 0;
  cpu->sreg.z = (result == 0) ? cpu->sreg.z : false;
  cpu->sreg.c = ((uint)imm + cpu->sreg.c) > reg;
  cpu->sreg.s = cpu->sreg.n ^ cpu->sreg.v;

  cpu->rf[rd] = result;
}

void Sbc::Execute(Cpu* cpu, const InsnArgs& a) {
  const uint rd = a.rd_rr.rd;
  const uint rr = a.rd_rr.rr;

  u8 reg = cpu->rf[rd];
  u8 rs1 = cpu->rf[rr];
  u8 result = reg - rs1 - cpu->sreg.c;

  cpu->sreg.h = ((int)(reg & 0x0f) - (int)(rs1 & 0x0f) - (int)cpu->sreg.c) < 0;
  cpu->sreg.v = ((reg ^ rs1) & (reg ^ result)) & 0x80;
  cpu->sreg.n = (result & 0x80) != 0;
  cpu->sreg.z = (result == 0) ? cpu->sreg.z : false;
  cpu->sreg.c = ((uint)rs1 + cpu->sreg.c) > reg;
  cpu->sreg.s = cpu->sreg.n ^ cpu->sreg.v;

  cpu->rf[rd] = result;
}

void Sbi::Execute(Cpu* cpu, const InsnArgs& a) {
  const uint io = a.reg_bit.reg;
  const uint b = a.reg_bit.bit;

  cpu->IoWrite(io, cpu->IoRead(io) | (1 << b));
}

void Sbic::Execute(Cpu* cpu, const InsnArgs& a) {
  const uint io = a.reg_bit.reg;
  const uint b = a.reg_bit.bit;

  u8 reg = cpu->IoRead(io);
  if (!(reg & (1 << b))) {
    cpu->skip = true;
  }
}

void Sbis::Execute(Cpu* cpu, const InsnArgs& a) {
  const uint io = a.reg_bit.reg;
  const uint b = a.reg_bit.bit;

  u8 reg = cpu->IoRead(io);
  if (reg & (1 << b)) {
    cpu->skip = true;
  }
}

void Sbiw::Execute(Cpu* cpu, const InsnArgs& a) {
  const uint rp = a.rd_imm.rd;
  const uint imm = a.rd_imm.imm;

  u16 reg = (u16)cpu->rf[rp] | ((u16)cpu->rf[rp + 1] << 8);
  u16 result = reg - imm;

  cpu->sreg.n = (result & 0x8000) != 0;
  cpu->sreg.z = (result == 0);
  cpu->sreg.c = imm > reg;
  cpu->sreg.v = ((reg ^ imm) & (reg ^ result)) & 0x8000;
  cpu->sreg.s = cpu->sreg.n ^ cpu->sreg.v;

  cpu->rf[rp] = result;
  cpu->rf[rp + 1] = result >> 8;
}

void Sbrc::Execute(Cpu* cpu, const InsnArgs& a) {
  const uint rr = a.rr_bit.rr;
  const uint b = a.rr_bit.bit;

  u8 reg = cpu->rf[rr];
  if (!(reg & (1 << b))) {
    cpu->skip = true;
  }
}

void Sbrs::Execute(Cpu* cpu, const InsnArgs& a) {
  const uint rr = a.rr_bit.rr;
  const uint b = a.rr_bit.bit;

  u8 reg = cpu->rf[rr];
  if (reg & (1 << b)) {
    cpu->skip = true;
  }
}

void Sleep::Execute(Cpu* cpu, const InsnArgs&) {
  cpu->sleeping = true;
}

void Spm::Execute(Cpu* cpu, const InsnArgs&) {
  log_error("SPM executed");
  // TODO: Implement.
}

void Spmx::Execute(Cpu* cpu, const InsnArgs&) {
  log_error("SPMX executed");
  // TODO: Implement.
}

void Stdy::Execute(Cpu* cpu, const InsnArgs& a) {
  const uint rr = a.rd_imm.rd;
  const uint imm = a.rd_imm.imm;

  cpu->MemoryWrite(cpu->y + imm, cpu->rf[rr]);  // TODO: Is imm signed or unsigned?
}

string Stdy::GetStatementString(const InsnArgs& a) {
  return std::format("{} Y+0x{:x},R{}", kName, a.rd_imm.imm, a.rd_imm.rd);
}

void Stdz::Execute(Cpu* cpu, const InsnArgs& a) {
  const uint rr = a.rd_imm.rd;
  const uint imm = a.rd_imm.imm;

  cpu->MemoryWrite(cpu->z + imm, cpu->rf[rr]);  // TODO: Is imm signed or unsigned?
}

string Stdz::GetStatementString(const InsnArgs& a) {
  return std::format("{} Z+0x{:x},R{}", kName, a.rd_imm.imm, a.rd_imm.rd);
}

void Sts::Execute(Cpu* cpu, const InsnArgs& a) {
  const uint rr = a.rd_imm.rd;
  const uint adr = a.rd_imm.imm;

  cpu->MemoryWrite(adr, cpu->rf[rr]);
}

void Stx1::Execute(Cpu* cpu, const InsnArgs& a) {
  const uint rr = a.rr.rr;

  cpu->MemoryWrite(cpu->x, cpu->rf[rr]);
}

string Stx1::GetStatementString(const InsnArgs& a) {
  return std::format("{} X, R{}", kName, a.rr.rr);
}

void Stx2::Execute(Cpu* cpu, const InsnArgs& a) {
  const uint rr = a.rr.rr;

  cpu->MemoryWrite(cpu->x, cpu->rf[rr]);
  ++cpu->x;
}

string Stx2::GetStatementString(const InsnArgs& a) {
  return std::format("{} X+, R{}", kName, a.rr.rr);
}

void Stx3::Execute(Cpu* cpu, const InsnArgs& a) {
  const uint rr = a.rr.rr;

  --cpu->x;
  cpu->MemoryWrite(cpu->x, cpu->rf[rr]);
}

string Stx3::GetStatementString(const InsnArgs& a) {
  return std::format("{} X-, R{}", kName, a.rr.rr);
}

void Sty2::Execute(Cpu* cpu, const InsnArgs& a) {
  const uint rr = a.rr.rr;

  cpu->MemoryWrite(cpu->y, cpu->rf[rr]);
  ++cpu->y;
}

string Sty2::GetStatementString(const InsnArgs& a) {
  return std::format("{} Y+, R{}", kName, a.rr.rr);
}

void Sty3::Execute(Cpu* cpu, const InsnArgs& a) {
  const uint rr = a.rr.rr;

  --cpu->y;
  cpu->MemoryWrite(cpu->y, cpu->rf[rr]);
}

string Sty3::GetStatementString(const InsnArgs& a) {
  return std::format("{} Y-, R{}", kName, a.rr.rr);
}

void Stz2::Execute(Cpu* cpu, const InsnArgs& a) {
  const uint rr = a.rr.rr;

  cpu->MemoryWrite(cpu->z, cpu->rf[rr]);
  ++cpu->z;
}

string Stz2::GetStatementString(const InsnArgs& a) {
  return std::format("{} Z+, R{}", kName, a.rr.rr);
}

void Stz3::Execute(Cpu* cpu, const InsnArgs& a) {
  const uint rr = a.rr.rr;

  --cpu->z;
  cpu->MemoryWrite(cpu->z, cpu->rf[rr]);
}

string Stz3::GetStatementString(const InsnArgs& a) {
  return std::format("{} Z-, R{}", kName, a.rr.rr);
}

void Sub::Execute(Cpu* cpu, const InsnArgs& a) {
  const uint rd = a.rd_rr.rd;
  const uint rr = a.rd_rr.rr;

  u8 reg = cpu->rf[rd];
  u8 rs1 = cpu->rf[rr];
  u8 result = reg - rs1;

  cpu->sreg.h = ((int)(reg & 0x0f) - (int)(rs1 & 0x0f)) < 0;
  cpu->sreg.v = ((reg ^ rs1) & (reg ^ result)) & 0x80;
  cpu->sreg.n = (result & 0x80) != 0;
  cpu->sreg.z = (result == 0);
  cpu->sreg.c = rs1 > reg;
  cpu->sreg.s = cpu->sreg.n ^ cpu->sreg.v;

  cpu->rf[rd] = result;
}

void Subi::Execute(Cpu* cpu, const InsnArgs& a) {
  const uint rd = a.rd_imm.rd;
  const uint imm = a.rd_imm.imm;

  u8 reg = cpu->rf[rd];
  u8 result = reg - imm;

  cpu->sreg.h = ((int)(reg & 0x0f) - (int)(imm & 0x0f)) < 0;
  cpu->sreg.v = ((reg ^ imm) & (reg ^ result)) & 0x80;
  cpu->sreg.n = (result & 0x80) != 0;
  cpu->sreg.z = (result == 0);
  cpu->sreg.c = imm > reg;
  cpu->sreg.s = cpu->sreg.n ^ cpu->sreg.v;

  cpu->rf[rd] = result;
}

void Swap::Execute(Cpu* cpu, const InsnArgs& a) {
  const uint rd = a.rd.rd;

  u8 reg = cpu->rf[rd];
  cpu->rf[rd] = (reg << 4) | (reg >> 4);
}

void Xch::Execute(Cpu* cpu, const InsnArgs&) {
  throw Cpu::IllegalInsnException();  // Not part of ATmega328p.
}

void Wdr::Execute(Cpu* cpu, const InsnArgs&) {
  cpu->watchdog_timer = 0;
}

void Illegal::Execute(Cpu* cpu, const InsnArgs&) {
  throw Cpu::IllegalInsnException();  // Not part of ATmega328p.
}

}  // namespace Insn
