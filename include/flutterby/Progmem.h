#include <stdint.h>
#include <sys/types.h>
#include <stddef.h>
#include "flutterby/Traits.h"
#include "flutterby/SmallestInteger.h"

#pragma once
namespace flutterby {

template <typename U>
inline typename enable_if<sizeof(U) == 1, U>::type progmem_deref(const U* ptr) {
  U result;
  __asm__ __volatile__("lpm %[retval], Z;\n\t"
                       : [retval] "=r"(result), "=z"(ptr)
                       : "1"(ptr));
  return result;
}

template <typename U>
inline typename enable_if<sizeof(U) == 2, U>::type progmem_deref(const U* ptr) {
  U result;
  // using "al" as the output constraint here because, when using "r",
  // the compiler sometimes chooses to use one of the registers that
  // comprises Z which emits an asm warning (but not error!).
  // "al" gives us registers in the range 0-23.
  // http://www.atmel.com/webdoc/avrlibcreferencemanual/inline_asm_1io_ops.html
  // describes the register constraints.
  __asm__ __volatile__(
      "lpm %A[retval], Z+;\n\t"
      "lpm %B[retval], Z;\n\t"
      : [retval] "=al"(result), "=z"(ptr)
      : "1"(ptr)
      :);
  return result;
}

template <typename U>
inline typename enable_if<sizeof(U) == 3, U>::type progmem_deref(const U* ptr) {
  U result;
  __asm__ __volatile__(
      "lpm %A[retval], Z+;\n\t"
      "lpm %B[retval], Z+;\n\t"
      "lpm %C[retval], Z;\n\t"
      : [retval] "=al"(result), "=z"(ptr)
      : "1"(ptr)
      :);
  return result;
}

template <typename U>
inline typename enable_if<sizeof(U) == 4, U>::type progmem_deref(const U* ptr) {
  U result;
  __asm__ __volatile__(
      "lpm %A[retval], Z+;\n\t"
      "lpm %B[retval], Z+;\n\t"
      "lpm %C[retval], Z+;\n\t"
      "lpm %D[retval], Z;\n\t"
      : [retval] "=al"(result), "=z"(ptr)
      : "1"(ptr)
      :);
  return result;
}

extern "C" void memcpy_P(void* dest, const void* src, size_t);

template <typename U>
inline typename enable_if<sizeof(U) >= 5, U>::type progmem_deref(const U* ptr) {
  U result;
  /* I've found that trying to use the same inline asm approach
   * as above for 5+ bytes exhausts g++'s register allocator,
   * so we fall back to memcpy_P from avr-libc */
  memcpy_P(&result, ptr, sizeof(result));
  return result;
}

/** Effectively an iterator for data stored in ProgMem
 */
template <typename T>
class ProgMemPtr {
  const T* ptr_;

 public:
  constexpr ProgMemPtr(const T* ptr) : ptr_(ptr) {}

  constexpr T operator*() const {
    return progmem_deref(ptr_);
  }

  constexpr operator T() const {
    return progmem_deref(ptr_);
  }

  constexpr bool operator!=(const ProgMemPtr<T>& other) {
    return ptr_ != other.ptr_;
  }

  constexpr ProgMemPtr<T>& operator++() {
    ++ptr_;
    return *this;
  }

  constexpr ProgMemPtr<T> operator++(int)const {
    return ProgMemPtr<T>(ptr_ + 1);
  }

  constexpr ProgMemPtr<T>& operator+=(size_t n) {
    ptr_ += n;
    return *this;
  }

  constexpr ProgMemPtr<T>& operator-=(size_t n) {
    ptr_ -= n;
    return *this;
  }

  constexpr const T* raw_ptr() const {
    return ptr_;
  }
};

/** An instance of a non-array type stored in ProgMem */
template <typename T>
class ProgMemInst {
  const T value_;

 public:
  constexpr ProgMemInst(const T& value) : value_(value) {}

  /** implicit conversion to a copy of the target memory */
  constexpr operator T() const {
    if (__builtin_constant_p(value_)) {
      return value_;
    }
    return progmem_deref(&value_);
  }

  /** dereferences to a copy of the target memory */
  constexpr T operator*() const {
    if (__builtin_constant_p(value_)) {
      return value_;
    }
    return progmem_deref(&value_);
  }

  /** Taking the address of a ProgMemInst yields a ProgMemPtr */
  constexpr ProgMemPtr<T> operator&() const {
    return ProgMemPtr<T>(&value_);
  }

  /** pointer-to-member deref operator.
   * For a struct type it is desirable to only fetch the fields
   * of interest rather than copying the whole thing into SRAM
   * to access just a couple of bytes of the data.
   * The no-macro syntax for this is:
   *
   * ```
   * struct Foo {
   *    int field;
   * }
   * const ProgMem<MyFoo> __attribute__((progmem)) Foo{123};
   * ...
   *   // copy just MyFoo->field from flash
   *   auto fieldCopy = MyFoo->*(&Foo::field);
   * ```
   */
  template <typename U>
  constexpr U operator->*(U T::*ptr) const {
    return progmem_deref(member_ptr(ptr));
  }

  /** Returns a ProgMemPtr reference to a member field
   * This can be dereferenced to its value in progmem space,
   * or be passed on to some other function that accepts
   * a ProgMemPtr ref. */
  template <typename U>
  constexpr ProgMemPtr<U> ref(U T::*ptr) const {
    return ProgMemPtr<U>(member_ptr(ptr));
  }

  /** Returns the raw pointer value in program space */
  constexpr const T* raw_ptr() const {
    return &value_;
  }

  /** Returns the raw pointer address of a member field in program space */
  template <typename U>
  constexpr const U* member_ptr(U T::*ptr) const {
    return &((value_).*(ptr));
  }
};

/** A helper to deduce the correct type to hold a ProgMemInst */
template <typename T>
constexpr ProgMemInst<T> makeProgMem(const T& value) {
  return ProgMemInst<T>(value);
}

/** A macro to declare an instance of an object to be stored in ProgMem.
 * The ident parameter is the identifier to be emitted in the current
 * namespace and content is the parameter passed to its constructor. */
#define ProgMem(ident, content) \
  const auto ident __attribute__((progmem)) = makeProgMem(content)

/** Holds an array of T[Size] in ProgMem.
 * It provides begin() and end() methods that can be used in a generic
 * for loop to iterate over the contents */
template <typename T, size_t Size>
class ProgMemArrayInst {
  T array_[Size];

 public:
  constexpr ProgMemArrayInst(const T (&value)[Size]) : array_{0} {
    for (size_t i = 0; i < Size; ++i) {
      array_[i] = value[i];
    }
  }

  /** begin() returns an iterator to the start of the array,
   * enabling generic for loops */
  constexpr ProgMemPtr<T> begin() const {
    return ProgMemPtr<T>(&array_[0]);
  }
  /** end() returns an iterator to one-past-the-end of the array,
   * enabling generic for loops */
  constexpr ProgMemPtr<T> end() const {
    return ProgMemPtr<T>(&array_[Size]);
  }

  /** Returns the constant size of the array */
  constexpr typename smallest_integer_maximum<Size>::type size() const {
    return Size;
  }

  /** Returns a pointer to the specified array index.
   * ProgMemPtr is implicitly convertible to T so it behaves a bit
   * like a reference */
  constexpr ProgMemPtr<T> operator[](size_t idx) const {
    return &array_[idx];
  }
};

/** A helper to deduce the array size for a ProgMemArray */
template <typename T, size_t Size>
constexpr ProgMemArrayInst<T, Size> makeProgArray(const T (&value)[Size]) {
  return ProgMemArrayInst<T, Size>(value);
}

/** A macro to declare an instance of an array to be stored in ProgMem */
#define ProgMemArray(ident, content) \
  const auto ident __attribute__((progmem)) = makeProgArray(content)

/** ProgMemRange tracks the start and end of a range of progmem storage.
 * It is helpful when used together with the _P string literal to return
 * the bounds of the generated progmem storage. */
template <typename T>
class ProgMemRange {
  ProgMemPtr<T> s_, e_;

 public:
  constexpr ProgMemRange(const ProgMemPtr<T>& s, const ProgMemPtr<T>& e)
      : s_(s), e_(e) {}

  constexpr ProgMemPtr<T> begin() const {
    return s_;
  }
  constexpr ProgMemPtr<T> end() const {
    return e_;
  }

  /** Returns a pointer to the specified array index.
   * ProgMemPtr is implicitly convertible to T so it behaves a bit
   * like a reference.
   * Performs no bounds checks! */
  constexpr ProgMemPtr<T> operator[](size_t idx) const {
    s_ + idx;
  }
};

/** Implement a user_literal for strings that places them into
 * progmem and returns a ProgMemRange to reference them.
 * Usage:  "foo"_P
 */
template <typename Char, Char... Chars>
ProgMemRange<char> operator"" _P() {
  static const ProgMemArrayInst<char, sizeof...(Chars)> __attribute__((progmem))
  array({Chars...});
  return ProgMemRange<Char>(array.begin(), array.end());
}

using FlashString = ProgMemRange<char>;
}
