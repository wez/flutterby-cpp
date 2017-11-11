#pragma once
namespace flutterby {

using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;

using i8 = int8_t;
using i16 = int16_t;
using i32 = int32_t;

// Convenience for declaring small literals.
// Note that we cannot use this for _i8 without building a new type
// with a unary minus operator.  You'll just need to use i8(-123)
// for those cases.
constexpr u8 operator"" _u8(unsigned long long int val) {
  return val;
}

constexpr u16 operator"" _u16(unsigned long long int val) {
  return val;
}

// A stand-in for the void type, but easier to match in templates
struct Unit {
  // Lift void -> Unit if T is void, else T
  template <typename T>
  struct Lift : conditional<is_same<T, void>::value, Unit, T> {};
};


}
