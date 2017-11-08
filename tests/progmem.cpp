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

ProgMem(Food, Foo(123,321));

int main() {
  // We should have nothing to copy to SRAM
  EXPECT(data_segment_size() == 0);

  // Assert that deref has the correct type
  Foo local = *Food;

  // Dereference yields a copy of the object with the correct values
  EXPECT(local.baz == 321);
  EXPECT(local.bar == 123);

  // Taking the address yields something that derefs to the right contents
  auto ptr = &Food;
  local = *ptr;
  EXPECT(local.baz == 321);
  EXPECT(local.bar == 123);

  // pointer to member referencing
  auto localRef = Food.ref(&Foo::baz);
  EXPECT(*localRef == 321);
  EXPECT((Food->*(&Foo::baz)) == 123);

  return 0;
}
