#pragma once
#include "avr_autogen.h"
#include "flutterby/Progmem.h"

#ifdef HAVE_SIMAVR
# define HAVE_DBG 1
#else
# define HAVE_DBG 0
#endif

namespace flutterby {

/** FormatStream provides a way to emit formatted output to a
 * helper functor class Stream.  The Stream object is expected
 * to be callable with a byte at a time.
 */
template <typename Stream, bool AddNewLine>
class FormatStream {
  Stream stream_;

 public:
  // Default construct the stream
  FormatStream() = default;
  // move in a stream
  FormatStream(Stream&& stream) : stream_(move(stream)) {}

  ~FormatStream() {
#if HAVE_DBG
    if constexpr (AddNewLine) {
      write("\r"_P);
    }
#endif
  }

  // write out the content from an iterator range
  template <typename Iterator>
  void write(Iterator a, Iterator b) {
#if HAVE_DBG
    while (a != b) {
      stream_(*a);
      ++a;
    }
#endif
  }

  // Iterate the array and write out each byte
  template <typename C, size_t Size>
  void write(const ProgMemArrayInst<C, Size>& arr) {
#if HAVE_DBG
    write(arr.begin(), arr.end());
#endif
  }

  // Iterate the range and write out each byte
  template <typename C>
  void write(const ProgMemRange<C>& arr) {
#if HAVE_DBG
    write(arr.begin(), arr.end());
#endif
  }

  // Format an unsigned integer with the specified base and write it out
  template <typename Int>
  typename enable_if<
      numeric_traits<Int>::is_integral && !numeric_traits<Int>::is_signed,
      void>::type
  write_int(Int val, uint8_t base) {
#if HAVE_DBG
    if (val == 0) {
      stream_('0');
      return;
    }

    char buf[numeric_traits<Int>::max_decimal_digits];
    // Careful not to underflow i when we have max_decimal_digits to print
    uint8_t i = numeric_traits<Int>::max_decimal_digits;

    while (val != 0) {
      uint8_t rem = val % base;
      buf[--i] = (rem > 9) ? (rem - 10 + 'a') : (rem + '0');
      val /= base;
    }
    write(buf + i , buf + numeric_traits<Int>::max_decimal_digits);
#endif
  }

  // Format a signed integer with the specified base and write it out
  template <typename Int>
  typename enable_if<
      numeric_traits<Int>::is_integral && numeric_traits<Int>::is_signed,
      void>::type
  write_int(Int ival, uint8_t base) {
#if HAVE_DBG
    if (ival < 0 && base == 10) {
      // Only handle negative numbers for base 10;
      // we assume unsigned otherwise.
      stream_('-');
      ival = -ival;
    }
    write_int(typename numeric_traits<Int>::unsigned_type(ival), base);
#endif
  }

  FormatStream& stream() {
    return *this;
  }
};

/** A stream implementation that writes to the simavr console */
class SimavrConsoleStream {
 public:
  void operator()(uint8_t b) {
#if HAVE_SIMAVR
    SIMAVR_CONSOLE = b;
#endif
  }
};

#define DBG_NO_NL() FormatStream<SimavrConsoleStream, false>().stream()
#define DBG() FormatStream<SimavrConsoleStream, true>().stream()


template <typename T, bool NL, typename A, size_t Size>
FormatStream<T, NL>& operator<<(
    FormatStream<T, NL>& stm,
    const ProgMemArrayInst<A, Size>& arr) {
#if HAVE_DBG
  stm.write(arr);
#endif
  return stm;
}

template <typename T, bool NL>
FormatStream<T, NL>& operator<<(
    FormatStream<T, NL>& stm,
    const ProgMemRange<char>& arr) {
#if HAVE_DBG
  stm.write(arr);
#endif
  return stm;
}

/** Support streaming regular string literals but emit a deprecation
 * notice to remind folks to use the _P literal */
template <typename T, bool NL, typename A, size_t Size>
[[deprecated(
    "use the _P string literal suffix to save SRAM!")]] FormatStream<T, NL>&
operator<<(FormatStream<T, NL>& stm, const A (&arr)[Size]) {
#if HAVE_DBG
  stm.write(&arr[0], &arr[Size]);
#endif
  return stm;
}

/** Stream an integral value as a decimal */
template <typename T, bool NL,typename Int>
typename enable_if<numeric_traits<Int>::is_integral, FormatStream<T, NL>>::type&
operator<<(FormatStream<T, NL>& stm, Int ival) {
#if HAVE_DBG
  stm.write_int(ival, 10);
#endif
  return stm;
}

/** Show the pointer value for non-stringy looking pointers */
template <typename T, bool NL, typename Target>
typename enable_if<!is_same<Target, char>::value, FormatStream<T, NL>>::type&
operator<<(FormatStream<T, NL>& stm, const Target* ptr) {
#if HAVE_DBG
  stm.write("0x"_P);
  stm.write_int(reinterpret_cast<size_t>(ptr), 16);
#endif
  return stm;
}

// Sometimes you just have to resort to blinking an LED to figure things out
inline void debugBlink(uint8_t times) {
#ifdef __AVR_ATmega32U4__
  times *= 2;
    Portc::portc &= ~PortcSignalFlags::PC7;
  while (times-- > 0) {
    Portc::portc ^= PortcSignalFlags::PC7;
    __builtin_avr_delay_cycles(1000000);
  }
    Portc::portc &= ~PortcSignalFlags::PC7;
    __builtin_avr_delay_cycles(10000000);
#endif
}
}
