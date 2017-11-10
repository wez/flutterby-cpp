#include "atmega32u4.h"
#include "flutterby/Progmem.h"
#include <string.h>
#include "flutterby/Debug.h"
#include "flutterby/Test.h"
#include "flutterby/Result.h"
#include "flutterby/Future.h"

using namespace flutterby;

int main() {
  auto f = make_future(Ok());
  auto status = f.poll();
  EXPECT(status.is_some());
  EXPECT(status.value().is_ok());

  EXPECT_EQ(
      make_future(Ok(2))
          .and_then([](int value) { return value * 2; })
          .and_then([](int value) { return Ok(value * 2); })
          .and_then([](int value) { return make_future(Ok(value * 2)); })
          .poll()
          .value()
          .value(),
      16);

  EXPECT_EQ(
      make_future(Error<int>())
          .and_then([](int value) { return 0xbad; })
          .or_else([](Unit) { return Ok(42); })
          .poll()
          .value()
          .value(),
      42);

  EXPECT_EQ(
      make_future(Error<int>())
          .or_else([](Unit) { return make_future(Ok(42)); })
          .poll()
          .value()
          .value(),
      42);

  EXPECT(make_future(Error<int>())
             .or_else([](Unit) { return 42; })
             .poll()
             .value()
             .is_err());

  EXPECT_EQ(
      make_future(Error<int>())
          .or_else([](Unit) { return Ok(42); })
          .and_then([](int value) { return value * 2; })
          .poll()
          .value()
          .value(),
      84);

  return 0;
}
