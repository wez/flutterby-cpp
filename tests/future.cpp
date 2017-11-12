#include "avr_autogen.h"
#include "flutterby/Progmem.h"
#include <string.h>
#include "flutterby/Debug.h"
#include "flutterby/Test.h"
#include "flutterby/Result.h"
#include "flutterby/Future.h"

using namespace flutterby;

// Why isn't this a method of Future?   We don't want the real firmware
// code to busy loop on a single future, so we don't want to make it
// too easy to do this.
template <typename Future>
auto busy_wait_future(Future&&f) {
  while (true) {
    auto status = f.template poll();
    if (status.is_some()) {
      return status;
    }
  }
}

int main() {
  auto f = make_future(Ok());
  auto status = f.poll();
  EXPECT(status.is_some());
  EXPECT(status.value().is_ok());

  EXPECT_EQ(
      busy_wait_future(
          make_future(Ok(2))
              .and_then([](int value) { return value * 2; })
              .and_then([](int value) { return Ok(value * 2); })
              .and_then([](int value) { return make_future(Ok(value * 2)); }))
          .value()
          .value(),
      16);

  EXPECT_EQ(
      busy_wait_future(make_future(Error<int>())
                           .and_then([](int value) { return 0xbad; })
                           .or_else([](Unit) { return Ok(42); }))
          .value()
          .value(),
      42);

  EXPECT_EQ(
      busy_wait_future(make_future(Error<int>()).or_else([](Unit) {
        return make_future(Ok(42));
      }))
          .value()
          .value(),
      42);

  EXPECT(busy_wait_future(
             make_future(Error<int>()).or_else([](Unit) { return 42; }))
             .value()
             .is_err());

  EXPECT_EQ(
      busy_wait_future(make_future(Error<int>())
                           .or_else([](Unit) { return Ok(42); })
                           .and_then([](int value) { return value * 2; }))
          .value()
          .value(),
      84);

  return 0;
}
