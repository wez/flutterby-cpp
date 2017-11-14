#pragma once
namespace flutterby {

/** bitflags is a helper for working with bitfields in a type-safe manner.
 * Usage: is via the somewhat clunky initialization:
 *
 * ```
 * namespace Foo {
 *   enum class Foo : uint8_t {
 *     Foo1,
 *     Foo2,
 *   }
 *   static const constexpr bitflags<Foo, uint8_t> Foo1(Foo::Foo1);
 *   static const constexpr bitflags<Foo, uint8_t> Foo2(Foo::Foo2);
 * }
 *
 * auto foo = Foo::Foo1 | Foo::Foo2;
 * ```
 *
 * This prevents accidental assignment of an arbitrary integer value
 * to the bitflags type while retaining a convenient way to refer
 * to the discrete variants of the type.
 *
 * Since this type is used for IO access, care is taken in the method
 * implementations to allow for both const and volatile overloads so
 * that we don't accidentally emit IO load/store opcodes for temporary
 * values, and vice versa.
 */
template <typename T, typename Int> class bitflags {
  Int value_;

  constexpr bitflags(Int bits) : value_{bits} {}

public:
  using Bitflags = bitflags<T, Int>;
  using Enum = T;
  using Integer = Int;

  constexpr bitflags(T bits) : value_{Int(bits)} {}
  constexpr bitflags() : value_{0} {}
  bitflags(const volatile Bitflags &other) : value_{other.value_} {}
  bitflags(const Bitflags &other) : value_{other.value_} {}

  /** Escape hatch for specifying the raw bit pattern.
   * Having this as a static method makes this action explicit. */
  static constexpr Bitflags from_raw_bits(Int bits) {
    return bitflags(bits);
  }

  void set_raw_bits(Int bits) {
    value_ = bits;
  }
  void set_raw_bits(Int bits) volatile {
    value_ = bits;
  }

  void clear() {
    value_ = 0;
  }
  void clear() volatile {
    value_ = 0;
  }

  /** Yield the raw bits */
  constexpr Int raw_bits() const {
    return value_;
  }

  constexpr Int raw_bits() const volatile {
    return value_;
  }

  constexpr operator bool() {
    return value_ != 0;
  }
  constexpr operator bool() volatile {
    return value_ != 0;
  }

  constexpr bool operator==(const Bitflags&other) {
    return value_ == other.value_;
  }
  constexpr bool operator==(const Bitflags&other) volatile {
    return value_ == other.value_;
  }
  constexpr bool operator!=(const Bitflags&other) {
    return value_ != other.value_;
  }
  constexpr bool operator!=(const Bitflags&other) volatile {
    return value_ != other.value_;
  }

  constexpr Bitflags &operator=(const Bitflags &other) {
    value_ = other.value_;
  }
  constexpr Bitflags &operator=(const Bitflags &other) volatile {
    value_ = other.value_;
  }

#define _FLUT_BINOP(op)                                                        \
  constexpr Bitflags operator op(const Bitflags &other) const {                \
    return Bitflags(value_ op other.value_);                                   \
  }                                                                            \
  constexpr Bitflags operator op(const Bitflags &other) const volatile {       \
    return Bitflags(value_ op other.value_);                                   \
  }

  /* operators such as |= return void to avoid triggering the warning:
   * "implicit dereference will not access object" for the volatile
   * overload */
#define _FLUT_BITASS(opeq)                                                     \
  void operator opeq(const Bitflags &other) volatile {                         \
    value_ opeq other.value_;                                                  \
  }                                                                            \
  Bitflags &operator opeq(const Bitflags &other) {                             \
    value_ opeq other.value_;                                                  \
    return *this;                                                              \
  }

  _FLUT_BINOP(|)
  _FLUT_BITASS(|=)

  _FLUT_BINOP(&)
  _FLUT_BITASS(&=)

  _FLUT_BINOP(^)
  _FLUT_BITASS(^=)

#undef _FLUT_BINOP
#undef _FLUT_BITASS

  constexpr Bitflags operator~() const { return Bitflags(~value_); }
  constexpr Bitflags operator~() const volatile { return Bitflags(~value_); }

  /* operator- clears the bits of the rhs from the lhs */
  constexpr Bitflags operator-(const Bitflags &other) const {
    return Bitflags(value_ & ~other.value_);
  }
  constexpr Bitflags operator-(const Bitflags &other) const volatile {
    return Bitflags(value_ & ~other.value_);
  }

} __attribute__((packed));
}
