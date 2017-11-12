#pragma once
#include "avr_autogen.h"
#include "flutterby/Traits.h"
#include "flutterby/Types.h"

namespace flutterby {

class CriticalSection {
  bitflags<CpuSregFlags::CpuSregFlags, u8> sreg_;

  public:
   CriticalSection() : sreg_(Cpu::sreg) {
     __builtin_avr_cli();
   }

   ~CriticalSection() {
     // Restore prior status register.  This has the effect
     // of enabling interrupts again if they were enabled prior
     // to our CLI call above, or leaving them disabled if we
     // were a nested critical section acquisition
     Cpu::sreg = sreg_;
     __asm__ __volatile__("" ::: "memory");
   }

   template <typename Func>
   auto run(Func&& func) {
     return func();
   }
};

template <typename Func>
auto interrupt_free(Func&&func) {
  CriticalSection cs;
  return cs.run(move(func));
}

}
