#include "atmega32u4.h"
#include "flutterby/Progmem.h"
#include <string.h>
#include "flutterby/Debug.h"
#include "flutterby/Test.h"
#include "flutterby/Option.h"

using namespace flutterby;

int main() {
  auto a = Some(132);
  EXPECT(!a.is_none());
  EXPECT(a.is_some());
  EXPECT(a.value() == 132);

  // Could do with some tests to prove that destruction is
  // handled correctly
  return 0;
}
