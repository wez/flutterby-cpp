#include "flutterby/Debug.h"
#include "flutterby/EventLoop.h"
#include "flutterby/Future.h"
#include "flutterby/Sleep.h"
#include "flutterby/Timer1.h"

namespace flutterby {

namespace eventloop {
TimerBase* TIMERS = nullptr;
volatile u16 TICKS = 0;
}

TimerBase::~TimerBase() {}

TimerBase::TimerBase(u16 remaining_ticks, bool repeat)
    : remaining_ticks_(remaining_ticks ? remaining_ticks : 1),
      repeat_(repeat ? remaining_ticks_ : 0) {}

void TimerBase::spawn(Shared<TimerBase> timer) {
  // Convert the Shared instance into a raw pointer; we promise
  // to keep track of it until it has finished its countdown
  auto* timer_ptr = timer.into_raw();
  timer_ptr->next_ = eventloop::TIMERS;
  eventloop::TIMERS = timer_ptr;
}

bool TimerBase::tick_all(u16 elapsed) {
  bool did_any = false;
  TimerBase** prev_next = &eventloop::TIMERS;

  auto p = *prev_next;
  while (p) {
    if (p->remaining_ticks_ && elapsed >= p->remaining_ticks_) {
      p->run();
      did_any = true;

      if (p->repeat_ != 0) {
        p->remaining_ticks_ = p->repeat_;
      } else {
        // Convert back to Shared so that we can safely release our ref
        auto owned = Shared<TimerBase>::from_raw(p);
        // Unlink from the list
        *prev_next = p->next_;
        // `owned` falls out of scope here and releases its ref
        p = p->next_;
        continue;
      }
    } else {
      p->remaining_ticks_ -= elapsed;
    }

    prev_next = &p->next_;
    p = p->next_;
  }

  return did_any;
}

namespace eventloop {

IRQ_TIMER1_COMPA {
//  DBG() << "COMPA:"_P << TICKS;
  ++TICKS;
  set_event_pending();
}

static void setup_timer() {
  Timer1::configure(
      Timer1::WaveformGenerationMode::ClearOnTimerMatchOutputCompare,
      1000000 / kTimerHz);
  // Some bootloaders let us get this far without interrupts enabled;
  // ensure that they are turned on for the remainder of operation
  __builtin_avr_sei();
}

void run_forever() {
  setup_timer();

  auto last_tick = TICKS;
  while (TIMERS || future::Pollable::have_pollables()) {
    auto now_tick = TICKS;
    auto elapsed_ticks = now_tick - last_tick;
    last_tick = now_tick;

    bool did_any = false;

    if (TimerBase::tick_all(elapsed_ticks)) {
      did_any = true;
    }
    if (future::Pollable::poll_all()) {
      did_any = true;
    }

    if (did_any) {
      continue;
    }

    wait_for_event(SleepMode::Idle);
  }

#ifdef HAVE_SIMAVR
  // Turn off the timer so that we don't trigger a wakeup in exit() and cause
  // the test harness to loop.  If we do somehow get here in a real device then
  // we want to allow the interrupt to reset the device so let's only do this
  // when building for the simulator
  Timer1::configure(
      Timer1::ClockSource::None, Timer1::WaveformGenerationMode::Normal);
#endif
}
}
}
