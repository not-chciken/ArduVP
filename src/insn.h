#pragma once

#include <format>

#include "common.h"
#include "decode/insn_args.h"

struct Cpu;

struct DisasContext {
  u16 next_word;
};

inline int ToRegs16To31ByOne(DisasContext*, int i) {
  return 16 + i;
}
inline int ToRegs16To23ByOne(DisasContext*, int i) {
  return 16 + i;
}
inline int ToRegs24To30ByTwo(DisasContext*, int i) {
  return 24 + 2 * i;
}
inline int ToRegs00To30ByTwo(DisasContext*, int i) {
  return 2 * i;
}
inline int NextWord(DisasContext* ctx) {
  return ctx->next_word;
}
inline int Append16(DisasContext* ctx, int high) {
  return (high << 16) | ctx->next_word;
}

struct Instruction {
  InsnArgs args;
  void (*Execute)(Cpu* cpu, const InsnArgs& args);
  string (*GetStatementString)(const InsnArgs& args);
  u8 cycles;
  bool is_32bit;
};

namespace Insn {

template <typename T>
struct InsnBase {
  static constexpr u8 kCycles = 1;
  static constexpr bool kIs32Bit = false;

  static string GetStatementString(const InsnArgs&) { return T::kName; }
};

template <typename T>
struct FmtNone : InsnBase<T> {};

template <typename T>
struct FmtRd : InsnBase<T> {
  static string GetStatementString(const InsnArgs& a) { return std::format("{} {}", T::kName, RegName(a.rd.rd)); }
};

template <typename T>
struct FmtRr : InsnBase<T> {
  static string GetStatementString(const InsnArgs& a) { return std::format("{} {}", T::kName, RegName(a.rr.rr)); }
};

template <typename T>
struct FmtRdRr : InsnBase<T> {
  static string GetStatementString(const InsnArgs& a) {
    return std::format("{} {},{}", T::kName, RegName(a.rd_rr.rd), RegName(a.rd_rr.rr));
  }
};

template <typename T>
struct FmtRdImmHex : InsnBase<T> {
  static string GetStatementString(const InsnArgs& a) {
    return std::format("{} {},0x{:x}", T::kName, RegName(a.rd_imm.rd), a.rd_imm.imm);
  }
};

template <typename T>
struct FmtRdImmDec : InsnBase<T> {
  static string GetStatementString(const InsnArgs& a) {
    return std::format("{} {},{}", T::kName, RegName(a.rd_imm.rd), a.rd_imm.imm);
  }
};

template <typename T>
struct FmtIoRd : InsnBase<T> {
  static string GetStatementString(const InsnArgs& a) {
    return std::format("{} 0x{:x},{}", T::kName, a.rd_imm.imm, RegName(a.rd_imm.rd));
  }
};

template <typename T>
struct FmtRdBit : InsnBase<T> {
  static string GetStatementString(const InsnArgs& a) {
    return std::format("{} {},{}", T::kName, RegName(a.rd_bit.rd), a.rd_bit.bit);
  }
};

template <typename T>
struct FmtRrBit : InsnBase<T> {
  static string GetStatementString(const InsnArgs& a) {
    return std::format("{} {},{}", T::kName, RegName(a.rr_bit.rr), a.rr_bit.bit);
  }
};

template <typename T>
struct FmtRegBit : InsnBase<T> {
  static string GetStatementString(const InsnArgs& a) {
    return std::format("{} 0x{:x},{}", T::kName, a.reg_bit.reg, a.reg_bit.bit);
  }
};

template <typename T>
struct FmtBit : InsnBase<T> {
  static string GetStatementString(const InsnArgs& a) { return std::format("{} {}", T::kName, a.bit.bit); }
};

template <typename T>
struct FmtBitImm : InsnBase<T> {
  static string GetStatementString(const InsnArgs& a) {
    return std::format("{} {},{}", T::kName, a.imm_bit.bit, a.imm_bit.imm);
  }
};

template <typename T>
struct FmtImmHex : InsnBase<T> {
  static string GetStatementString(const InsnArgs& a) { return std::format("{} 0x{:x}", T::kName, a.imm.imm); }
};

#define INSN(Name, Str, Cycles, Fmt)                  \
  struct Name : Fmt<Name> {                           \
    static constexpr const char* kName = Str;         \
    static constexpr u8 kCycles = Cycles;             \
    static void Execute(Cpu* cpu, const InsnArgs& a); \
  }

#define INSN_STR(Name, Str, Cycles, Fmt)                 \
  struct Name : Fmt<Name> {                              \
    static constexpr const char* kName = Str;            \
    static constexpr u8 kCycles = Cycles;                \
    static void Execute(Cpu* cpu, const InsnArgs& a);    \
    static string GetStatementString(const InsnArgs& a); \
  }

#define INSN_32(Name, Str, Cycles, Fmt)               \
  struct Name : Fmt<Name> {                           \
    static constexpr const char* kName = Str;         \
    static constexpr u8 kCycles = Cycles;             \
    static constexpr bool kIs32Bit = true;            \
    static void Execute(Cpu* cpu, const InsnArgs& a); \
  }

// Arithmetic and logic.
INSN(Adc, "ADC", 1, FmtRdRr);
INSN(Add, "ADD", 1, FmtRdRr);
INSN(Adiw, "ADIW", 2, FmtRdImmDec);
INSN(And, "AND", 1, FmtRdRr);
INSN(Andi, "ANDI", 1, FmtRdImmHex);
INSN(Com, "COM", 1, FmtRd);
INSN(Dec, "DEC", 1, FmtRd);
INSN(Eor, "EOR", 1, FmtRdRr);
INSN(Fmul, "FMUL", 2, FmtRdRr);
INSN(Fmuls, "FMULS", 2, FmtRdRr);
INSN(Fmulsu, "FMULSU", 2, FmtRdRr);
INSN(Inc, "INC", 1, FmtRd);
INSN(Mul, "MUL", 2, FmtRdRr);
INSN(Muls, "MULS", 2, FmtRdRr);
INSN(Mulsu, "MULSU", 2, FmtRdRr);
INSN(Neg, "NEG", 1, FmtRd);
INSN(Or, "OR", 1, FmtRdRr);
INSN(Ori, "ORI", 1, FmtRdImmHex);
INSN(Sbc, "SBC", 1, FmtRdRr);
INSN(Sbci, "SBCI", 1, FmtRdImmHex);
INSN(Sbiw, "SBIW", 2, FmtRdImmDec);
INSN(Sub, "SUB", 1, FmtRdRr);
INSN(Subi, "SUBI", 1, FmtRdImmHex);

// Branches, calls and skips.
INSN(Brbc, "BRBC", 1, FmtBitImm);
INSN(Brbs, "BRBS", 1, FmtBitImm);
INSN_32(Call, "CALL", 4, FmtImmHex);
INSN(Cp, "CP", 1, FmtRdRr);
INSN(Cpc, "CPC", 1, FmtRdRr);
INSN(Cpi, "CPI", 1, FmtRdImmHex);
INSN(Cpse, "CPSE", 1, FmtRdRr);
INSN(Eicall, "EICALL", 1, FmtNone);
INSN(Eijmp, "EIJMP", 1, FmtNone);
INSN(Icall, "ICALL", 3, FmtNone);
INSN(Ijmp, "IJMP", 2, FmtNone);
INSN_32(Jmp, "JMP", 3, FmtImmHex);
INSN(Rcall, "RCALL", 3, FmtImmHex);
INSN(Ret, "RET", 4, FmtNone);
INSN(Reti, "RETI", 4, FmtNone);
INSN(Rjmp, "RJMP", 2, FmtImmHex);
INSN(Sbic, "SBIC", 1, FmtRegBit);
INSN(Sbis, "SBIS", 1, FmtRegBit);
INSN(Sbrc, "SBRC", 1, FmtRrBit);
INSN(Sbrs, "SBRS", 1, FmtRrBit);

// Data transfer.
INSN(In, "IN", 1, FmtIoRd);
INSN(Lac, "LAC", 1, FmtRd);
INSN(Las, "LAS", 1, FmtRd);
INSN(Lat, "LAT", 1, FmtRd);
INSN_STR(Lddy, "LDD", 2, FmtRdImmHex);
INSN_STR(Lddz, "LDD", 2, FmtRdImmHex);
INSN(Ldi, "LDI", 1, FmtRdImmHex);
INSN_32(Lds, "LD", 2, FmtRdImmHex);
INSN_STR(Ldx1, "LD", 2, FmtRd);
INSN_STR(Ldx2, "LD", 2, FmtRd);
INSN_STR(Ldx3, "LD", 2, FmtRd);
INSN_STR(Ldy2, "LD", 2, FmtRd);
INSN_STR(Ldy3, "LD", 2, FmtRd);
INSN_STR(Ldz2, "LD", 2, FmtRd);
INSN_STR(Ldz3, "LD", 2, FmtRd);
INSN(Lpm1, "LPM", 3, FmtNone);
INSN_STR(Lpm2, "LPM", 3, FmtRd);
INSN_STR(Lpmx, "LPM", 3, FmtRd);
INSN(Mov, "MOV", 1, FmtRdRr);
INSN_STR(Movw, "MOVW", 1, FmtRdRr);
INSN(Out, "OUT", 1, FmtIoRd);
INSN(Pop, "POP", 2, FmtRd);
INSN(Push, "PUSH", 2, FmtRd);
INSN(Spm, "SPM", 1, FmtNone);
INSN(Spmx, "SPM", 1, FmtNone);
INSN_STR(Stdy, "STD", 2, FmtRdImmHex);
INSN_STR(Stdz, "STD", 2, FmtRdImmHex);
INSN_32(Sts, "STS", 2, FmtRdImmHex);
INSN_STR(Stx1, "ST", 2, FmtRr);
INSN_STR(Stx2, "ST", 2, FmtRr);
INSN_STR(Stx3, "ST", 2, FmtRr);
INSN_STR(Sty2, "ST", 2, FmtRr);
INSN_STR(Sty3, "ST", 2, FmtRr);
INSN_STR(Stz2, "ST", 2, FmtRr);
INSN_STR(Stz3, "ST", 2, FmtRr);
INSN(Xch, "XCH", 1, FmtRd);

// Bit and bit-test.
INSN(Asr, "ASR", 1, FmtRd);
INSN(Bclr, "BCLR", 1, FmtBit);
INSN(Bld, "BLD", 1, FmtRdBit);
INSN(Bset, "BSET", 1, FmtBit);
INSN(Bst, "BST", 1, FmtRdBit);
INSN(Cbi, "CBI", 2, FmtRegBit);
INSN(Lsr, "LSR", 1, FmtRd);
INSN(Ror, "ROR", 1, FmtRd);
INSN(Sbi, "SBI", 2, FmtRegBit);
INSN(Swap, "SWAP", 1, FmtRd);

// MCU control.
INSN(Break, "BREAK", 1, FmtNone);
INSN(Illegal, "ILLEGAL", 1, FmtNone);
INSN(Nop, "NOP", 1, FmtNone);
INSN(Sleep, "SLEEP", 1, FmtNone);
INSN(Wdr, "WDR", 1, FmtNone);

#undef INSN
#undef INSN_STR
#undef INSN_32

}  // namespace Insn
