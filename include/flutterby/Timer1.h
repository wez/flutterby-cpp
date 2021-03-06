#pragma once
#include "avr_autogen.h"
#include "flutterby/CriticalSection.h"
#include "flutterby/Types.h"

namespace flutterby {


class Timer1 {
 public:
  enum class ClockSource {
    None = u8(Tc1Tccr1bFlags::Tc1Tccr1bFlags::CS1_NO_CLOCK_SOURCE_STOPPED),
    Prescale1 = u8(Tc1Tccr1bFlags::Tc1Tccr1bFlags::CS1_RUNNING_NO_PRESCALING),
    Prescale8 = u8(Tc1Tccr1bFlags::Tc1Tccr1bFlags::CS1_RUNNING_CLK8),
    Prescale64 = u8(Tc1Tccr1bFlags::Tc1Tccr1bFlags::CS1_RUNNING_CLK64),
    Prescale256 = u8(Tc1Tccr1bFlags::Tc1Tccr1bFlags::CS1_RUNNING_CLK256),
    Prescale1024 = u8(Tc1Tccr1bFlags::Tc1Tccr1bFlags::CS1_RUNNING_CLK1024),
    ExternalFalling =
        u8(Tc1Tccr1bFlags::Tc1Tccr1bFlags::CS1_RUNNING_EXTCLK_TX_FALLING_EDGE),
    ExternalRising =
        u8(Tc1Tccr1bFlags::Tc1Tccr1bFlags::CS1_RUNNING_EXTCLK_TX_RISING_EDGE),
  };
  static constexpr auto A_WGM10 = 1 << 0;
  static constexpr auto A_WGM11 = 1 << 1;
  static constexpr auto B_WGM12 = (1 << 3) << 8;
  static constexpr auto B_WGM13 = (1 << 4) << 8;
  enum class WaveformGenerationMode : u16 {
    Normal = 0,
    PwmPhaseCorrect8Bit = A_WGM10,
    PwmPhaseCorrect9Bit = A_WGM11,
    PwmPhaseCorrect10Bit = A_WGM11 | A_WGM10,
    ClearOnTimerMatchOutputCompare = B_WGM12,
    FastPwm8Bit = B_WGM12 | A_WGM10,
    FastPwm9Bit = B_WGM12 | A_WGM11,
    FastPwm10Bit = B_WGM12 | A_WGM11 | A_WGM10,
    PwmPhaseAndFrequencyCorrectInputCapture = B_WGM13,
    PwmPhaseAndFrequencyCorrectOutputCompare = B_WGM13 | A_WGM10,
    PwmPhaseCorrectInputCapture = B_WGM13 | A_WGM11,
    PwmPhaseCorrectOutputCompare = B_WGM13 | A_WGM11 | A_WGM10,
    ClearOnTimerMatchInputCapture = B_WGM13 | B_WGM12,
    FastPwmInputCapture = B_WGM13 | B_WGM12 | A_WGM11,
    FastPwmOutputCompare = B_WGM13 | B_WGM12 | A_WGM11 | A_WGM10,
  };

  static inline void configure(WaveformGenerationMode wgm, u32 period_us) {
    u32 cycles = (F_CPU / 1000000) * period_us;
    static constexpr u32 kResolution = 0xffff;
    ClockSource clock;
    if (cycles < kResolution) {
      clock = ClockSource::Prescale1;
    } else if ((cycles >>= 3) < kResolution) {
      clock = ClockSource::Prescale8;
    } else if ((cycles >>= 3) < kResolution) {
      clock = ClockSource::Prescale64;
    } else if ((cycles >>= 2) < kResolution) {
      clock = ClockSource::Prescale256;
    } else if ((cycles >>= 2) < kResolution) {
      clock = ClockSource::Prescale1024;
    } else {
      panic("period_us is too large for Timer1"_P);
    }

    configure(clock, wgm, cycles-1);
  }

  static inline void
  configure(ClockSource clock, WaveformGenerationMode wgm, u16 compareA = 0) {
    auto a = bitflags<Tc1Tccr1aFlags::Tc1Tccr1aFlags, u8>::from_raw_bits(
        u16(wgm) & 0xff);
    auto b = bitflags<Tc1Tccr1bFlags::Tc1Tccr1bFlags, u8>::from_raw_bits(
        u16(clock) | (u16(wgm) >> 8));

    interrupt_free([&]() {
      Tc1::tccr1a.clear();
      Tc1::tccr1b.clear();
      Tc1::tcnt1 = 0;
      // Clear timer compare interrupt flags for channels A and B
      Tc1::tifr1 = Tc1Tifr1Flags::OCF1A;
      Tc1::tifr1 = Tc1Tifr1Flags::OCF1B;

      Tc1::tccr1a = a;
      Tc1::tccr1b = b;

      if (compareA) {
        Tc1::ocr1a = compareA;
        Tc1::timsk1 = Tc1Timsk1Flags::OCIE1A;
      } else {
        Tc1::timsk1.clear();
      }
      Tc0::gtccr.clear();
    });
  }

  static inline void setCompareA(u16 compare) {
    interrupt_free([&]() {
      Tc1::ocr1a = compare;
      if (compare) {
        Tc1::timsk1 = Tc1Timsk1Flags::OCIE1A;
      } else {
        Tc1::timsk1 &= ~Tc1Timsk1Flags::OCIE1A;
      }
    });
  }

  static inline void setCount(u16 count) {
    Tc1::tcnt1 = count;
  }

  static inline u16 getCount() {
    return Tc1::tcnt1;
  }

  static inline void configureOverflowInterrupt(bool enable) {
    if (enable) {
      Tc1::timsk1 |= Tc1Timsk1Flags::TOIE1;
    } else {
      Tc1::timsk1 &= ~Tc1Timsk1Flags::TOIE1;
    }
  }

};
}
