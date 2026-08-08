#pragma once
#include <functional>

#include "common.h"

class Cpu;

namespace Insn {

struct InsnBase {
  string name;
  uint cycles;
  bool is_32bit;

  virtual std::function<void()> GenFunc(u16 insn, Cpu* cpu) = 0;
  virtual std::function<void()> GenFunc32(u16 insn, u16 imm, Cpu* cpu);
  virtual string GetStatementString(u16 insn);
};

// 0000 0000 0000 0000
struct Insn0 : public InsnBase {
  std::function<void(Cpu*)> exec_func;

  virtual std::function<void()> GenFunc(u16 insn, Cpu* cpu) override;
};

// 1001 010k kkkk 111k
// kkkk kkkk kkkk kkkk
struct InsnImm22 : public InsnBase {
  InsnImm22();
  std::function<void(Cpu*, u32)> exec_func;
  u32 InitArgs(u16 insn, u16 imm);
  virtual std::function<void()> GenFunc(u16 insn, Cpu* cpu) override{ return {}; };
  std::function<void()> GenFunc32(u16 insn, u16 imm, Cpu* cpu) override;
  virtual string GetStatementString(u16 insn, u16 imm);
  static void Execute(Cpu* cpu, u32 addr);
};

// 0000 000d dddd 0000
// kkkk kkkk kkkk kkkk
struct InsnRImm16 : public InsnBase {
  InsnRImm16();
  std::function<void(Cpu*, uint, uint)> exec_func;
  std::tuple<uint, uint> InitArgs(u16 insn, u16 imm);
  virtual std::function<void()> GenFunc(u16 insn, Cpu* cpu) override { return {}; };
  std::function<void()> GenFunc32(u16 insn, u16 imm, Cpu* cpu) override;
  virtual string GetStatementString(u16 insn, u16 imm);
  static void Execute(Cpu* cpu, uint rd, uint adr);
};

// 0000 00rd dddd rrrr
struct InsnRR : public InsnBase {
  std::function<void(Cpu*, uint, uint)> exec_func;

  std::tuple<uint, uint> InitArgs(u16 insn);
  virtual std::function<void()> GenFunc(u16 insn, Cpu* cpu) override;
  virtual string GetStatementString(u16 insn) override;
};

// 0000 000d dddd 0000
struct InsnR : public InsnBase {
  std::function<void(Cpu*, uint)> exec_func;

  uint InitArgs(u16 insn);
  virtual std::function<void()> GenFunc(u16 insn, Cpu* cpu) override;
  virtual string GetStatementString(u16 insn) override;
};

// 0000 000d dddd 0bbb
struct InsnRb : public InsnBase {
  std::function<void(Cpu*, uint, uint)> exec_func;

  std::tuple<uint, uint> InitArgs(u16 insn);
  virtual std::function<void()> GenFunc(u16 insn, Cpu* cpu) override;
  virtual string GetStatementString(u16 insn) override;
};

// 0000 0000 KKdd KKKK
struct InsnRpImm6 : public InsnBase {
  std::function<void(Cpu*, uint, uint)> exec_func;

  std::tuple<uint, uint> InitArgs(u16 insn);
  virtual std::function<void()> GenFunc(u16 insn, Cpu* cpu) override;
  virtual string GetStatementString(u16 insn) override;
};

// 0000 0000 dddd rrrr
struct InsnRpRp : public InsnBase {
  std::function<void(Cpu*, uint, uint)> exec_func;

  std::tuple<uint, uint> InitArgs(u16 insn);
  virtual std::function<void()> GenFunc(u16 insn, Cpu* cpu) override;
  virtual string GetStatementString(u16 insn) override;
};

// 00q0 qq0d dddd 0qqq
struct InsnRImm6 : public InsnBase {
  std::function<void(Cpu*, uint, uint)> exec_func;

  std::tuple<uint, uint> InitArgs(u16 insn);
  virtual std::function<void()> GenFunc(u16 insn, Cpu* cpu) override;
};

// 1011 1AAr rrrr AAAA
struct InsnRIo : public InsnBase {
  std::function<void(Cpu*, uint, uint)> exec_func;

  std::tuple<uint, uint> InitArgs(u16 insn);
  virtual std::function<void()> GenFunc(u16 insn, Cpu* cpu) override;
};

// 0111 KKKK dddd KKKK
struct InsnRuImm8 : public InsnBase {
  std::function<void(Cpu*, uint, uint)> exec_func;

  std::tuple<uint, uint> InitArgs(u16 insn);
  virtual std::function<void()> GenFunc(u16 insn, Cpu* cpu) override;
  virtual string GetStatementString(u16 insn) override;
};

// 0000 kkkk kkkk kkkk
struct InsnImm12 : public InsnBase {
  std::function<void(Cpu*, uint)> exec_func;

  uint InitArgs(u16 insn);
  virtual std::function<void()> GenFunc(u16 insn, Cpu* cpu) override;
  virtual string GetStatementString(u16 insn) override;
};

// 0000 0000 AAAA Abbb
struct InsnIoB : public InsnBase {
  std::function<void(Cpu*, uint, uint)> exec_func;

  std::tuple<uint, uint> InitArgs(u16 insn);
  virtual std::function<void()> GenFunc(u16 insn, Cpu* cpu) override;
  virtual string GetStatementString(u16 insn) override;
};

// 0000 00kk kkkk ksss
struct InsnSImm6 : public InsnBase {
  std::function<void(Cpu*, uint, uint)> exec_func;

  std::tuple<uint, int> InitArgs(u16 insn);
  virtual std::function<void()> GenFunc(u16 insn, Cpu* cpu) override;
  virtual string GetStatementString(u16 insn) override;
};

// 0000 0000 0sss 0000
struct InsnS : public InsnBase {
  std::function<void(Cpu*, uint)> exec_func;

  uint InitArgs(u16 insn);
  virtual std::function<void()> GenFunc(u16 insn, Cpu* cpu) override;
  virtual string GetStatementString(u16 insn) override;
};

struct Adc : public InsnRR {
  Adc();
  static void Execute(Cpu* cpu, uint rd, uint rr);
};

struct Add : public InsnRR {
  Add();
  static void Execute(Cpu* cpu, uint rd, uint rr);
};

struct Adiw : public InsnRpImm6 {
  Adiw();
  static void Execute(Cpu* cpu, uint rr, uint imm);
};

struct And : public InsnRR {
  And();
  static void Execute(Cpu* cpu, uint rd, uint rr);
};

struct Andi : public InsnRuImm8 {
  Andi();
  static void Execute(Cpu* cpu, uint rr, uint imm);
};

struct Asr : public InsnR {
  Asr();
  static void Execute(Cpu* cpu, uint rd);
};

struct Bclr : public InsnS {
  Bclr();
  static void Execute(Cpu* cpu, uint sreg);
};

struct Bld : public InsnRb {
  Bld();
  static void Execute(Cpu* cpu, uint rr, uint imm);
};

struct Brbc : public InsnSImm6 {
  Brbc();
  static void Execute(Cpu* cpu, uint sreg, int imm);
};

struct Brbs : public InsnSImm6 {
  Brbs();
  static void Execute(Cpu* cpu, uint sreg, int imm);
};

struct Break : public Insn0 {
  Break();
  static void Execute(Cpu* cpu);
};

struct Bset : public InsnS {
  Bset();
  static void Execute(Cpu* cpu, uint sreg);
};

struct Bst : public InsnRb {
  Bst();
  static void Execute(Cpu* cpu, uint rr, uint imm);
};

struct Call : public InsnImm22 {
  Call();
  static void Execute(Cpu* cpu, u32 addr);
};

struct Cbi : public InsnIoB {
  Cbi();
  static void Execute(Cpu* cpu, uint io, uint b);
};

struct Com : public InsnR {
  Com();
  static void Execute(Cpu* cpu, uint rd);
};

struct Cp : public InsnRR {
  Cp();
  static void Execute(Cpu* cpu, uint rd, uint rr);
};

struct Cpc : public InsnRR {
  Cpc();
  static void Execute(Cpu* cpu, uint rd, uint rr);
};

struct Cpi : public InsnRuImm8 {
  Cpi();
  static void Execute(Cpu* cpu, uint rr, uint imm);
};

struct Cpse : public InsnRR {
  Cpse();
  static void Execute(Cpu* cpu, uint rd, uint rr);
};

struct Dec : public InsnR {
  Dec();
  static void Execute(Cpu* cpu, uint rd);
};

struct Eicall : public Insn0 {
  Eicall();
  static void Execute(Cpu* cpu);
};

struct Eijmp : public Insn0 {
  Eijmp();
  static void Execute(Cpu* cpu);
};

struct Eor : public InsnRR {
  Eor();
  static void Execute(Cpu* cpu, uint rd, uint rr);
};

struct Fmul : public InsnRR {
  Fmul();
  static void Execute(Cpu* cpu, uint rd, uint rr);
};

struct Fmuls : public InsnRR {
  Fmuls();
  static void Execute(Cpu* cpu, uint rd, uint rr);
};

struct Fmulsu : public InsnRR {
  Fmulsu();
  static void Execute(Cpu* cpu, uint rd, uint rr);
};

struct Icall : public Insn0 {
  Icall();
  static void Execute(Cpu* cpu);
};

struct Ijmp : public Insn0 {
  Ijmp();
  static void Execute(Cpu* cpu);
};

struct Inc : public InsnR {
  Inc();
  static void Execute(Cpu* cpu, uint rd);
};

struct In : public InsnRIo {
  In();
  static void Execute(Cpu* cpu, uint rd, uint io);
};

struct Jmp : public InsnImm22 {
  Jmp();
  static void Execute(Cpu* cpu, u32 addr);
};

struct Lac : public InsnR {
  Lac();
  static void Execute(Cpu* cpu, uint rd);
};

struct Las : public InsnR {
  Las();
  static void Execute(Cpu* cpu, uint rd);
};

struct Lat : public InsnR {
  Lat();
  static void Execute(Cpu* cpu, uint rd);
};

struct Lddy : public InsnRImm6 {
  Lddy();
  static void Execute(Cpu* cpu, uint rd, uint imm);
  virtual string GetStatementString(u16 insn) override;
};

struct Lddz : public InsnRImm6 {
  Lddz();
  static void Execute(Cpu* cpu, uint rd, uint imm);
  virtual string GetStatementString(u16 insn) override;
};

struct Ldi : public InsnRuImm8 {
  Ldi();
  static void Execute(Cpu* cpu, uint rd, uint imm);
};

struct Lds : public InsnRImm16 {
  Lds();
  static void Execute(Cpu* cpu, uint rd, uint adr);
};

struct Ldx1 : public InsnR {
  Ldx1();
  static void Execute(Cpu* cpu, uint rd);
  virtual string GetStatementString(u16 insn) override;
};

struct Ldx2 : public InsnR {
  Ldx2();
  static void Execute(Cpu* cpu, uint rd);
  virtual string GetStatementString(u16 insn) override;
};

struct Ldx3 : public InsnR {
  Ldx3();
  static void Execute(Cpu* cpu, uint rd);
  virtual string GetStatementString(u16 insn) override;
};

struct Ldy2 : public InsnR {
  Ldy2();
  static void Execute(Cpu* cpu, uint rd);
  virtual string GetStatementString(u16 insn) override;
};

struct Ldy3 : public InsnR {
  Ldy3();
  static void Execute(Cpu* cpu, uint rd);
  virtual string GetStatementString(u16 insn) override;
};

struct Ldz2 : public InsnR {
  Ldz2();
  static void Execute(Cpu* cpu, uint rd);
  virtual string GetStatementString(u16 insn) override;
};

struct Ldz3 : public InsnR {
  Ldz3();
  static void Execute(Cpu* cpu, uint rd);
  virtual string GetStatementString(u16 insn) override;
};

struct Lpm1 : public Insn0 {
  Lpm1();
  static void Execute(Cpu* cpu);
};

struct Lpm2 : public InsnR {
  Lpm2();
  static void Execute(Cpu* cpu, uint rd);
  virtual string GetStatementString(u16 insn) override;
};

struct Lpmx : public InsnR {
  Lpmx();
  static void Execute(Cpu* cpu, uint rd);
  virtual string GetStatementString(u16 insn) override;
};

struct Lsr : public InsnR {
  Lsr();
  static void Execute(Cpu* cpu, uint rd);
};

struct Mov : public InsnRR {
  Mov();
  static void Execute(Cpu* cpu, uint rd, uint rr);
};

struct Movw : public InsnRpRp {
  Movw();
  static void Execute(Cpu* cpu, uint rd, uint rr);
};

struct Mul : public InsnRR {
  Mul();
  static void Execute(Cpu* cpu, uint rd, uint rr);
};

struct Muls : public InsnRR {
  Muls();
  static void Execute(Cpu* cpu, uint rd, uint rr);
};

struct Mulsu : public InsnRR {
  Mulsu();
  static void Execute(Cpu* cpu, uint rd, uint rr);
};

struct Neg : public InsnR {
  Neg();
  static void Execute(Cpu* cpu, uint rd);
};

struct Nop : public Insn0 {
  Nop();
  static void Execute(Cpu* cpu);
};

struct Or : public InsnRR {
  Or();
  static void Execute(Cpu* cpu, uint rd, uint rr);
};

struct Ori : public InsnRuImm8 {
  Ori();
  static void Execute(Cpu* cpu, uint rr, uint imm);
};

struct Out : public InsnRIo {
  Out();
  static void Execute(Cpu* cpu, uint rr, uint io);
};

struct Pop : public InsnR {
  Pop();
  static void Execute(Cpu* cpu, uint rd);
};

struct Push : public InsnR {
  Push();
  static void Execute(Cpu* cpu, uint rd);
};

struct Rcall : public InsnImm12 {
  Rcall();
  static void Execute(Cpu* cpu, uint addr);
};

struct Ret : public Insn0 {
  Ret();
  static void Execute(Cpu* cpu);
};

struct Reti : public Insn0 {
  Reti();
  static void Execute(Cpu* cpu);
};

struct Rjmp : public InsnImm12 {
  Rjmp();
  static void Execute(Cpu* cpu, uint addr);
};

struct Ror : public InsnR {
  Ror();
  static void Execute(Cpu* cpu, uint rd);
};

struct Sbc : public InsnRR {
  Sbc();
  static void Execute(Cpu* cpu, uint rd, uint rr);
};

struct Sbci : public InsnRuImm8 {
  Sbci();
  static void Execute(Cpu* cpu, uint rr, uint imm);
};

struct Sbi : public InsnIoB {
  Sbi();
  static void Execute(Cpu* cpu, uint io, uint b);
};

struct Sbic : public InsnIoB {
  Sbic();
  static void Execute(Cpu* cpu, uint io, uint b);
};

struct Sbis : public InsnIoB {
  Sbis();
  static void Execute(Cpu* cpu, uint io, uint b);
};

struct Sbiw : public InsnRpImm6 {
  Sbiw();
  static void Execute(Cpu* cpu, uint rp, uint imm);
};

struct Sbrc : public InsnRb {
  Sbrc();
  static void Execute(Cpu* cpu, uint rr, uint imm);
};

struct Sbrs : public InsnRb {
  Sbrs();
  static void Execute(Cpu* cpu, uint rr, uint imm);
};

struct Sleep : public Insn0 {
  Sleep();
  static void Execute(Cpu* cpu);
};

struct Spm : public Insn0 {
  Spm();
  static void Execute(Cpu* cpu);
};

struct Spmx : public Insn0 {
  Spmx();
  static void Execute(Cpu* cpu);
};

struct Stdy : public InsnRImm6 {
  Stdy();
  static void Execute(Cpu* cpu, uint rr, uint imm);
  virtual string GetStatementString(u16 insn) override;
};

struct Stdz : public InsnRImm6 {
  Stdz();
  static void Execute(Cpu* cpu, uint rr, uint imm);
  virtual string GetStatementString(u16 insn) override;
};

struct Sts : public InsnRImm16 {
  Sts();
  static void Execute(Cpu* cpu, uint rr, uint adr);
};

struct Stx1 : public InsnR {
  Stx1();
  static void Execute(Cpu* cpu, uint rr);
  virtual string GetStatementString(u16 insn) override;
};

struct Stx2 : public InsnR {
  Stx2();
  static void Execute(Cpu* cpu, uint rr);
  virtual string GetStatementString(u16 insn) override;
};

struct Stx3 : public InsnR {
  Stx3();
  static void Execute(Cpu* cpu, uint rr);
  virtual string GetStatementString(u16 insn) override;
};

struct Sty2 : public InsnR {
  Sty2();
  static void Execute(Cpu* cpu, uint rr);
  virtual string GetStatementString(u16 insn) override;
};

struct Sty3 : public InsnR {
  Sty3();
  static void Execute(Cpu* cpu, uint rr);
  virtual string GetStatementString(u16 insn) override;
};

struct Stz2 : public InsnR {
  Stz2();
  static void Execute(Cpu* cpu, uint rr);
  virtual string GetStatementString(u16 insn) override;
};

struct Stz3 : public InsnR {
  Stz3();
  static void Execute(Cpu* cpu, uint rr);
  virtual string GetStatementString(u16 insn) override;
};

struct Sub : public InsnRR {
  Sub();
  static void Execute(Cpu* cpu, uint rd, uint rr);
};

struct Subi : public InsnRuImm8 {
  Subi();
  static void Execute(Cpu* cpu, uint rr, uint imm);
};

struct Swap : public InsnR {
  Swap();
  static void Execute(Cpu* cpu, uint rd);
};

struct Wdr : public Insn0 {
  Wdr();
  static void Execute(Cpu* cpu);
};

struct Xch : public InsnR {
  Xch();
  static void Execute(Cpu* cpu, uint rd);
};

struct Illegal : public Insn0 {
  Illegal();
  static void Execute(Cpu* cpu);
};

} // namespace insn

// TODO:
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