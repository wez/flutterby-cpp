#include "flutterby/Progmem.h"

#pragma once
namespace flutterby {

/** FormatStream provides a way to emit formatted output to a
 * helper functor class Stream.  The Stream object is expected
 * to be callable with a byte at a time.
 */
template <typename Stream>
class FormatStream {
  Stream stream_;

 public:
  // Default construct the stream
  FormatStream() = default;
  // move in a stream
  FormatStream(Stream&& stream) : stream_(move(stream)) {}

  // write out the content from an iterator range
  template <typename Iterator>
  void write(Iterator a, Iterator b) {
    while (a != b) {
      stream_(*a);
      ++a;
    }
  }

  // Iterate the array and write out each byte
  template <typename C, size_t Size>
  void write(const ProgMemArrayInst<C, Size>& arr) {
    write(arr.begin(), arr.end());
  }

  // Iterate the range and write out each byte
  template <typename C>
  void write(const ProgMemRange<C>& arr) {
    write(arr.begin(), arr.end());
  }

  // Format an unsigned integer with the specified base and write it out
  template <typename Int>
  typename enable_if<
      numeric_traits<Int>::is_integral && !numeric_traits<Int>::is_signed,
      void>::type
  write_int(Int val, uint8_t base) {
    if (val == 0) {
      stream_('0');
      return;
    }

    char buf[numeric_traits<Int>::max_decimal_digits];
    uint8_t i = numeric_traits<Int>::max_decimal_digits - 1;

    while (val != 0) {
      uint8_t rem = val % base;
      buf[i--] = (rem > 9) ? (rem - 10 + 'a') : (rem + '0');
      val /= base;
    }
    write(buf + i + 1, buf + numeric_traits<Int>::max_decimal_digits);
  }

  // Format a signed integer with the specified base and write it out
  template <typename Int>
  typename enable_if<
      numeric_traits<Int>::is_integral && numeric_traits<Int>::is_signed,
      void>::type
  write_int(Int ival, uint8_t base) {
    if (ival < 0 && base == 10) {
      // Only handle negative numbers for base 10;
      // we assume unsigned otherwise.
      stream_('-');
      ival = -ival;
    }
    write_int(typename numeric_traits<Int>::unsigned_type(ival), base);
  }
};

/** A stream implementation that writes to the simavr console */
class SimavrConsoleStream {
 public:
  void operator()(uint8_t b) {
    SIMAVR_CONSOLE = b;
  }
};

template <typename T, typename A, size_t Size>
FormatStream<T>& operator<<(
    FormatStream<T>& stm,
    const ProgMemArrayInst<A, Size>& arr) {
  stm.write(arr);
  return stm;
}

template <typename T>
FormatStream<T>& operator<<(
    FormatStream<T>& stm,
    const ProgMemRange<char>& arr) {
  stm.write(arr);
  return stm;
}

/** Support streaming regular string literals but emit a deprecation
 * notice to remind folks to use the _P literal */
template <typename T, typename A, size_t Size>
[[deprecated("use the _P string literal suffix to save SRAM!")]] FormatStream<
    T>&
operator<<(FormatStream<T>& stm, const A (&arr)[Size]) {
  stm.write(&arr[0], &arr[Size]);
  return stm;
}

/** Stream an integral value as a decimal */
template <typename T, typename Int>
typename enable_if<numeric_traits<Int>::is_integral, FormatStream<T>>::type&
operator<<(FormatStream<T>& stm, Int ival) {
  stm.write_int(ival, 10);
  return stm;
}

/** Show the pointer value for non-stringy looking pointers */
template <typename T, typename Target>
typename enable_if<!is_same<Target, char>::value, FormatStream<T>>::type&
operator<<(FormatStream<T>& stm, const Target* ptr) {
  stm.write("0x"_P);
  stm.write_int(reinterpret_cast<size_t>(ptr), 16);
  return stm;
}
}
