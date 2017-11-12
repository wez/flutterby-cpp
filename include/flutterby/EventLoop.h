#pragma once
#include "flutterby/Types.h"
#include "flutterby/Heap.h"

namespace flutterby {
namespace eventloop {
static constexpr float kTimer1Hz = 50.0;
static constexpr u32 kPrescaler = 1024;
static constexpr u16 kCompareA =
    u16((float(F_CPU) / (kTimer1Hz * float(kPrescaler))) - 1);
static constexpr u16 kTicksPerMs = u16(1000.0 / kTimer1Hz);
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
