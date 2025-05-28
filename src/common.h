#pragma once

#include <cassert>
#include <cmath>
#include <cstdint>
#include <string>
#include <stdfloat>

using string = std::string;

using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

using i8 = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;

using f32 = float;
using f64 = double;

constexpr u64 Bitmask(size_t width)
{
  return ((1ull << width) - 1);
}

constexpr u64 ExtractBits(u64 value, uint start, uint n)
{
  u64 mask = ((1ul << n) - 1) << start;
  return (value & mask) >> start;
}

template <typename T>
constexpr T SignExtend(T value, int bits)
{
  T m = (T)1 << (bits - 1);
  return (value ^ m) - m;
}

namespace Avr
{
  enum Regs
  {
    R0 = 0,
    R1,
    R2,
    R3,
    R4,
    R5,
    R6,
    R7,
    R8,
    R9,
    R10,
    R11,
    R12,
    R13,
    R14,
    R15,
    R16,
    R17,
    R18,
    R19,
    R20,
    R21,
    R22,
    R23,
    R24,
    R25,
    R26,
    R27,
    R28,
    R29,
    R30,
    R31
  };
}

inline string RegName(size_t ind)
{
  static string names[32] = {
      "R0", "R1", "R2", "R3", "R4", "R5", "R6", "R7",
      "R8", "R9", "R10", "R11", "R12", "R13", "R14", "R15",
      "R16", "R17", "R18", "R19", "R20", "R21", "R22", "R23",
      "R24", "R25", "R26", "R27", "R28", "R29", "R30", "R31"};

  return names[ind];
}
