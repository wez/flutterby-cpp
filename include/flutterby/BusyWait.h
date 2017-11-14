#pragma once
#include "flutterby/Types.h"

namespace flutterby {

inline void busy_wait_us(u32 us) __attribute__((__always_inline__));
inline void busy_wait_us(u32 us)  {
  __builtin_avr_delay_cycles(us * (F_CPU / 1000000));
}

inline void busy_wait_ms(u32 us) __attribute__((__always_inline__));
inline void busy_wait_ms(u32 us) {
  __builtin_avr_delay_cycles(us * (F_CPU / 1000));
}

}
