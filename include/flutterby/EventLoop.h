#pragma once
#include "flutterby/Types.h"
#include "flutterby/Heap.h"

namespace flutterby {
namespace eventloop {
static constexpr u32 kTimer1Hz = 100;
static constexpr u32 kTicksPerMs = 1000 * kTimer1Hz;
}

// Convert a number of seconds into a number of ticks
constexpr u16 operator"" _s(unsigned long long int val) {
  return val * eventloop::kTimer1Hz;
}

// Convert a number of milliseconds into a number of ticks
constexpr u16 operator"" _ms(unsigned long long int val) {
  return val * eventloop::kTimer1Hz / 1000;
}

class TimerBase {
  TimerBase* next_{nullptr};
  u16 remaining_ticks_;
  u16 repeat_;

 public:
  TimerBase(u16 remaining_ticks, bool repeat);

  virtual ~TimerBase();
  virtual void run() = 0;

  inline bool expired() const {
    return remaining_ticks_ == 0;
  }

  static void spawn(Shared<TimerBase> timer);
  static bool tick_all(u16 elapsed);
};

template <typename Func>
class Timer : public TimerBase {
  Func func_;

 public:
  Timer(u16 remaining_ticks, bool repeat, Func&& func)
      : TimerBase(remaining_ticks, repeat), func_(move(func)) {}

  void run() override {
    func_();
  }
};

template <typename Func>
Result<Shared<Timer<Func>>, Unit>
make_timer(u16 remaining_ticks, bool repeat, Func&& func) {
  return make_shared<Timer<Func>>(remaining_ticks, repeat, move(func));
}

namespace eventloop {

inline void enable_timer(Shared<TimerBase> timer) {
  TimerBase::spawn(move(timer));
}

/** Runs the event loop forever.
 * It will return when there are no more scheduled timers or registered
 * futures.  That is something that you don't really want to happen in
 * a real firmware but we allow it to make testing a bit easier in the
 * simulator. */
void run_forever();
}

}
