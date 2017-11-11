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
  {
    Variant<u8, float> v(123_u8);

    EXPECT(v.is<u8>());
    EXPECT(!v.is<float>());
    // EXPECT(!v.is<u16>()); won't compile because the type is mismatched

    auto v_int_ptr = v.get_ptr<u8>();
    EXPECT_EQ(*v_int_ptr, 123);

    v.set<float>(0.5);
    EXPECT(v.is<float>());
    EXPECT(*v.get_ptr<float>() == 0.5);

    static_assert(sizeof(v) == sizeof(float) + 1);

    // Note that the visitor will coerce between integer and floating
    // point types, so leaving out one of the cases here can have
    // surprising results; best to wrap things up in structs
    v.match(
        [](float fval) {
          EXPECT(fval == 0.5);
          return true;
        },
        [](u8 uval) {
          DBG() << "hmm, holding "_P << uval;
          panic("should not get here"_P);
          return false;
        });
  }

  {
    struct Foo { int foo; };
    struct Bar { int bar; };
    Variant<Foo, Bar> v(Foo{1});

    // The index values are the reverse order from the template
    // parameter list.
    EXPECT_EQ((variant::value_traits<Foo, Foo, Bar>::index), 1);
    EXPECT_EQ((variant::value_traits<Bar, Foo, Bar>::index), 0);

    EXPECT(v.is<Foo>());
    EXPECT(!v.is<Bar>());
    EXPECT_EQ(sizeof(v), 1 + sizeof(Foo));
    EXPECT(v.get_ptr<Foo>() != nullptr);

    auto match_result = v.match(
        [](Foo& f) {
          EXPECT_EQ(f.foo, 1);
          return 2;
        },
        [](Bar& b) {
          panic("should be foo"_P);
          return 0;
        });

    EXPECT_EQ(match_result, 2);
  }

  {
    Option<Variant<u8, float>> ov;
    EXPECT(ov.is_none());
    static_assert(sizeof(ov) == sizeof(float) + 1);
    ov.set_ok(42_u8);
    EXPECT_EQ(*ov.value().get_ptr<u8>(), 42);
    ov.clear();
    EXPECT(ov.is_none());
  }

  return 0;
}
