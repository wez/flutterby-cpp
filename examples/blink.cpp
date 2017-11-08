#include "atmega32u4.h"
#include "flutterby/Progmem.h"
#include <string.h>
#include "flutterby/Debug.h"
#include "flutterby/Test.h"

using namespace flutterby;

struct Foo {
  int bar = 0;
  uint16_t baz = 0;

  constexpr Foo() = default;
  constexpr Foo(int a, uint16_t b) : bar{a}, baz{b} {}
  constexpr Foo(const Foo& other) = default;
};

//ProgMem(Woot, 123);
ProgMem(Food, Foo(123,321));
//ProgMemArray(Wut2, "hello world\r\n");

#if 0
const ProgMem(bool) bfoo(false);

__attribute__ ((noinline))
bool thingy(const ProgMemInst<bool>& bval) {
  return bval;
}
#endif

int main() {
  /*
  auto mcusr = Cpu::mcusr | CpuMcusrFlags::WDRF;
  auto other = mcusr ;//| CpuMcusrFlags::WDRF;
  Cpu::mcusr = other;
  Cpu::mcusr |= CpuMcusrFlags::WDRF - CpuMcusrFlags::BORF;
  */

//  Portd::pind |= PortdSignalFlags::PD0;

  FormatStream<SimavrConsoleStream> s;
#if 0
  // FIXME: int16 here generates more code
  s << uint32_t{123} << "\r\n"_P;
  s << Wut2 << " "_P << -123 << " woah there\r\n"_P;
#endif

  EXPECT(0 == 1);

#if 1
  s<< "pointer-to-member-deref"_P <<Food->*(&Foo::baz) << "\r\n"_P;
  auto local = *Food;
  s << "local.bar="_P << local.bar << " local.baz="_P << local.baz << "\r\n"_P;

  auto localRef = Food.ref(&Foo::baz);

  s << "Food "_P << Food.raw_ptr() << " localRef "_P << localRef.raw_ptr()
    << "\r\n"_P;
  s << *localRef << "\r\n"_P;

//  return local.bar + local.baz;
#endif


  /*
  if (thingy(&bfoo)) {
  return 1;
  }
  */

  return 0;
}
