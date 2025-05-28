#pragma once
#include "common.h"

#include <functional>

class Cpu;

struct Insn
{
  string name;
  uint cycles;
  bool is_32bit;

  virtual std::function<void()> GenFunc(u16 insn, Cpu *cpu) = 0;
  virtual std::function<void()> GenFunc32(u32 insn, Cpu *cpu);
  virtual string GetStatementString(u16 insn);
};

struct Insn0 : public Insn
{
  std::function<void(Cpu *)> exec_func;

  virtual std::function<void()> GenFunc(u16 insn, Cpu *cpu) override;
};

struct Insn32 : public Insn
{
  std::function<void(Cpu *, u32)> exec_func;
  u32 InitArgs(u32 insn);
  std::function<void()> GenFunc32(u32 insn, Cpu *cpu) override;
  virtual string GetStatementString(u32 insn);
  static void Execute(Cpu *cpu, u32 addr);
};

struct InsnRr : public Insn
{
  std::function<void(Cpu *, uint, uint)> exec_func;

  std::tuple<uint, uint> InitArgs(u16 insn);
  virtual std::function<void()> GenFunc(u16 insn, Cpu *cpu) override;
  virtual string GetStatementString(u16 insn) override;
};

struct InsnR : public Insn
{
  std::function<void(Cpu *, uint)> exec_func;

  uint InitArgs(u16 insn);
  virtual std::function<void()> GenFunc(u16 insn, Cpu *cpu) override;
  virtual string GetStatementString(u16 insn) override;
};

struct InsnRb : public Insn
{
  std::function<void(Cpu *, uint, uint)> exec_func;

  std::tuple<uint, uint> InitArgs(u16 insn);
  virtual std::function<void()> GenFunc(u16 insn, Cpu *cpu) override;
  virtual string GetStatementString(u16 insn) override;
};

struct InsnImm6 : public Insn
{
  std::function<void(Cpu *, uint, uint)> exec_func;

  std::tuple<uint, uint> InitArgs(u16 insn);
  virtual std::function<void()> GenFunc(u16 insn, Cpu *cpu) override;
  virtual string GetStatementString(u16 insn) override;
};

struct InsnImm8 : public Insn
{
  std::function<void(Cpu *, uint, uint)> exec_func;

  std::tuple<uint, uint> InitArgs(u16 insn);
  virtual std::function<void()> GenFunc(u16 insn, Cpu *cpu) override;
  virtual string GetStatementString(u16 insn) override;
};

struct InsnImm12 : public Insn
{
  std::function<void(Cpu *, uint)> exec_func;

  uint InitArgs(u16 insn);
  virtual std::function<void()> GenFunc(u16 insn, Cpu *cpu) override;
  virtual string GetStatementString(u16 insn) override;
};

struct Adc : public InsnRr
{
  Adc();
  static void Execute(Cpu *cpu, uint rd, uint rr);
};

struct Add : public InsnRr
{
  Add();
  static void Execute(Cpu *cpu, uint rd, uint rr);
};

struct Adiw : public InsnImm6
{
  Adiw();
  static void Execute(Cpu *cpu, uint rr, uint imm);
};

struct And : public InsnRr
{
  And();
  static void Execute(Cpu *cpu, uint rd, uint rr);
};

struct Andi : public InsnImm8
{
  Andi();
  static void Execute(Cpu *cpu, uint rr, uint imm);
};

struct Break : public Insn0
{
  Break();
  static void Execute(Cpu *cpu);
};

struct Call : public Insn32
{
  Call();
  static void Execute(Cpu *cpu, u32 addr);
};

struct Com : public InsnR
{
  Com();
  static void Execute(Cpu *cpu, uint rd);
};

struct Cp : public InsnRr
{
  Cp();
  static void Execute(Cpu *cpu, uint rd, uint rr);
};

struct Cpc : public InsnRr
{
  Cpc();
  static void Execute(Cpu *cpu, uint rd, uint rr);
};

struct Cpse : public InsnRr
{
  Cpse();
  static void Execute(Cpu *cpu, uint rd, uint rr);
};

struct Dec : public InsnR
{
  Dec();
  static void Execute(Cpu *cpu, uint rd);
};

struct Eor : public InsnRr
{
  Eor();
  static void Execute(Cpu *cpu, uint rd, uint rr);
};

struct Fmul : public InsnRr
{
  Fmul();
  static void Execute(Cpu *cpu, uint rd, uint rr);
};

struct Fmuls : public InsnRr
{
  Fmuls();
  static void Execute(Cpu *cpu, uint rd, uint rr);
};

struct Fmulsu : public InsnRr
{
  Fmulsu();
  static void Execute(Cpu *cpu, uint rd, uint rr);
};

struct Inc : public InsnR
{
  Inc();
  static void Execute(Cpu *cpu, uint rd);
};

struct Jmp : public Insn32
{
  Jmp();
  static void Execute(Cpu *cpu, u32 addr);
};

struct Mov : public InsnRr
{
  Mov();
  static void Execute(Cpu *cpu, uint rd, uint rr);
};

struct Mul : public InsnRr
{
  Mul();
  static void Execute(Cpu *cpu, uint rd, uint rr);
};

struct Mulsu : public InsnRr
{
  Mulsu();
  static void Execute(Cpu *cpu, uint rd, uint rr);
};

struct Neg : public InsnR
{
  Neg();
  static void Execute(Cpu *cpu, uint rd);
};

struct Nop : public Insn0
{
  Nop();
  static void Execute(Cpu *cpu);
};

struct Or : public InsnRr
{
  Or();
  static void Execute(Cpu *cpu, uint rd, uint rr);
};

struct Ori : public InsnImm8
{
  Ori();
  static void Execute(Cpu *cpu, uint rr, uint imm);
};

struct Pop : public InsnR
{
  Pop();
  static void Execute(Cpu *cpu, uint rd);
};

struct Push : public InsnR
{
  Push();
  static void Execute(Cpu *cpu, uint rd);
};

struct Rcall : public InsnImm12
{
  Rcall();
  static void Execute(Cpu *cpu, uint addr);
};

struct Ret : public Insn0
{
  Ret();
  static void Execute(Cpu *cpu);
};

struct Reti : public Insn0
{
  Reti();
  static void Execute(Cpu *cpu);
};

struct Rjmp : public InsnImm12
{
  Rjmp();
  static void Execute(Cpu *cpu, uint addr);
};

struct Ror : public InsnR
{
  Ror();
  static void Execute(Cpu *cpu, uint rd);
};

struct Sbci : public InsnImm8
{
  Sbci();
  static void Execute(Cpu *cpu, uint rr, uint imm);
};

struct Sbrc : public InsnRb
{
  Sbrc();
  static void Execute(Cpu *cpu, uint rr, uint imm);
};

struct Sbrs : public InsnRb
{
  Sbrs();
  static void Execute(Cpu *cpu, uint rr, uint imm);
};

struct Sleep : public Insn0
{
  Sleep();
  static void Execute(Cpu *cpu);
};

struct Sub : public InsnRr
{
  Sub();
  static void Execute(Cpu *cpu, uint rd, uint rr);
};

struct Subi : public InsnImm6
{
  Subi();
  static void Execute(Cpu *cpu, uint rr, uint imm);
};

struct Swap : public InsnR
{
  Swap();
  static void Execute(Cpu *cpu, uint rd);
};


// TODO:
// Ijmp
// Icall

// CPI
// SBIC
// SBIS
// BRBS
// BRBC
// BREQ
// BRNE
// BRCS
// BRCC
// BRSH
// BRLO
// BRMI
// BRPL
// BRGE
// BRLT
// BRHS
// BRHC
// BRTS
// BRTC
// BRVS
// BRVC
// BRIE
// BRID

// SBI
// CBI
// LSL
// LSR
// ROL
// ASR
// BSET
// BCLR
// BST
// BLD
// CLC
// SEN

// CLN
// CLZ
// SEI
// CLI
// CLS
// CLV
// CLT
// SEH
// CLH

// MOVW
// LDI
// LD
// LDD
// LDD
// LDS
// STD
// ST
// STS
// LPM
// LPM
// LPM
// SPM
// IN
// OUT
// WDR