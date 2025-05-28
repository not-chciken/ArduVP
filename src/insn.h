#pragma once
#include "common.h"

#include <functional>

class Cpu;

struct Insn
{
  string name;
  uint cycles;

  virtual std::function<void()> GenFunc(u16 insn, Cpu *cpu) = 0;
  virtual string GetStatementString(u16 insn);
};

struct InsnRr : public Insn {
  std::function<void(Cpu*, uint, uint)> exec_func;

  std::tuple<uint, uint> InitArgs(u16 insn);
  virtual std::function<void()> GenFunc(u16 insn, Cpu* cpu) override;
  virtual string GetStatementString(u16 insn) override;
};

struct InsnR : public Insn {
  std::function<void(Cpu*, uint)> exec_func;

  uint InitArgs(u16 insn);
  virtual std::function<void()> GenFunc(u16 insn, Cpu* cpu) override;
  virtual string GetStatementString(u16 insn) override;
};

struct InsnImm6 : public Insn {
  std::function<void(Cpu*, uint, uint)> exec_func;

  std::tuple<uint, uint> InitArgs(u16 insn);
  virtual std::function<void()> GenFunc(u16 insn, Cpu* cpu) override;
  virtual string GetStatementString(u16 insn) override;
};

struct InsnImm8 : public Insn {
  std::function<void(Cpu*, uint, uint)> exec_func;

  std::tuple<uint, uint> InitArgs(u16 insn);
  virtual std::function<void()> GenFunc(u16 insn, Cpu* cpu) override;
  virtual string GetStatementString(u16 insn) override;
};

//InsnR
//InsnFlag

struct Adc : public InsnRr
{
  Adc();
  static void Execute(Cpu *cpu, uint rr, uint rd);
};

struct Add : public InsnRr
{
  Add();
  static void Execute(Cpu *cpu, uint rr, uint rd);
};

struct Adiw : public InsnImm6
{
  Adiw();
  static void Execute(Cpu *cpu, uint rr, uint imm);
};

struct And : public InsnRr
{
  And();
  static void Execute(Cpu *cpu, uint rr, uint rd);
};

struct Andi : public InsnImm8
{
  Andi();
  static void Execute(Cpu *cpu, uint rr, uint imm);
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

struct Inc : public InsnR
{
  Inc();
  static void Execute(Cpu *cpu, uint rd);
};

struct Mul : public InsnRr
{
  Mul();
  static void Execute(Cpu *cpu, uint rr, uint rd);
};

struct Mulsu : public InsnRr
{
  Mulsu();
  static void Execute(Cpu *cpu, uint rr, uint rd);
};

struct Neg : public InsnR
{
  Neg();
  static void Execute(Cpu *cpu, uint rd);
};

struct Nop : public InsnR
{
  Nop();
  static void Execute(Cpu *cpu, uint rd);
};

struct Or : public InsnRr
{
  Or();
  static void Execute(Cpu *cpu, uint rr, uint rd);
};

struct Ori : public InsnImm8
{
  Ori();
  static void Execute(Cpu *cpu, uint rr, uint imm);
};

struct Sub : public InsnRr
{
  Sub();
  static void Execute(Cpu *cpu, uint rr, uint rd);
};

struct Subi : public InsnImm6
{
  Subi();
  static void Execute(Cpu *cpu, uint rr, uint imm);
};

struct Sbci : public InsnImm8
{
  Sbci();
  static void Execute(Cpu *cpu, uint rr, uint imm);
};


// TODO:
// Fmul
// Fmuls
// Fmulsu

// Rjmp
// Ijmp
// Jmp
// Rcall
// Icall
// CALL
// RET
// RETI
// CPSE

// CPC
// CPI
// SBRC
// SBRS
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
// ROR
// ASR
// SWAP
// BSET
// BCLR
// BST
// BLD
// SEC
// CLC
// SEN

// CLN
// SEZ
// CLZ
// SEI
// CLI
// SES
// CLS
// SEV
// CLV
// SET
// CLT
// SEH
// CLH

// MOV
// MOVW
// LDI
// LD
// LD
// LD
// LD
// LD
// LD
// LDD
// LD
// LD
// LD
// LDD
// LDS
// ST
// ST
// ST
// ST
// ST
// ST
// STD
// ST
// ST
// ST
// STD
// STS
// LPM
// LPM
// LPM
// SPM
// IN
// OUT
// PUSH
// POP
// MCU
// SLEEP
// WDR
// BREAK