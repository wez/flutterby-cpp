#include "atmega32u4.h"
#include <string.h>
#include "flutterby/Debug.h"
#include "flutterby/Option.h"
#include "flutterby/Progmem.h"
#include "flutterby/Test.h"
#include "flutterby/Types.h"
#include "flutterby/Variant.h"

using namespace flutterby;

int main() {
  Variant<u8, float> v(123_u8);

  EXPECT(v.is<u8>());
  EXPECT(!v.is<float>());
  // EXPECT(!v.is<u16>()); won't compile because the type is mismatched

  auto v_int_ptr = v.get_ptr<u8>();
  EXPECT_EQ(*v_int_ptr, 123);

  v.set<float>(0.5);
  EXPECT(v.is<float>());
  EXPECT(*v.get_ptr<float>() == 0.5);

  static_assert(sizeof(v) == sizeof(float)+1);

  Option<Variant<u8, float>> ov;
  EXPECT(ov.is_none());
  static_assert(sizeof(ov) == sizeof(float)+1);
  ov.set_ok(42_u8);
  EXPECT_EQ(*ov.value().get_ptr<u8>(), 42);
  ov.clear();
  EXPECT(ov.is_none());

  return 0;
}
