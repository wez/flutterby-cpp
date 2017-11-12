#include "avr_autogen.h"
#include "flutterby/Test.h"
#include "flutterby/EventLoop.h"

using namespace flutterby;

int main() {
  bool done = false;

  {
    auto timer = make_timer(10_u16, false, [&done] { done = true; }).value();
    eventloop::enable_timer(timer);

    EXPECT(!done);

    TimerBase::tick_all(1);
    EXPECT(!done);
    TimerBase::tick_all(5);
    EXPECT(!done);
    TimerBase::tick_all(5);
    EXPECT(done);

    TimerBase::tick_all(5);
    EXPECT(true); // didn't fault
  }

  {
    done = false;
    auto timer = make_timer(10_u16, false, [&done] { done = true; }).value();
    eventloop::enable_timer(timer);
    eventloop::run_forever();

    EXPECT(done);
  }
  return 0;
}
