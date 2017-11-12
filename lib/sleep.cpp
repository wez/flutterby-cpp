#include "flutterby/Sleep.h"
#include "avr_autogen.h"

namespace flutterby {

void set_sleep_mode(SleepMode mode) {
  auto flags = CpuSmcrFlags::SM_IDLE;
  switch (mode) {
    case SleepMode::Idle:
      flags = CpuSmcrFlags::SM_IDLE;
      break;
    case SleepMode::ADCNoiseReduction:
      flags = CpuSmcrFlags::SM_ADC_NOISE_REDUCTION_IF_AVAILABLE;
      break;
    case SleepMode::PowerDown:
      flags = CpuSmcrFlags::SM_POWER_DOWN;
    case SleepMode::PowerSave:
      flags = CpuSmcrFlags::SM_POWER_SAVE;
    case SleepMode::StandyBy:
      flags = CpuSmcrFlags::SM_STANDBY;
    case SleepMode::ExtendedStandBy:
      flags = CpuSmcrFlags::SM_EXTENDED_STANDBY;
    default:
      return;
  }

  // Dont flip the sleep enable bit; just set the mode flags
  Cpu::smcr = (Cpu::smcr & CpuSmcrFlags::SE) | flags;
}

static volatile uint8_t PENDING = 0;
void set_event_pending() {
  PENDING = 1;
}

inline void sleep_enable() {
  Cpu::smcr |= CpuSmcrFlags::SE;
}

inline void sleep_disable() {
  Cpu::smcr &= ~CpuSmcrFlags::SE;
}

void wait_for_event(SleepMode mode) {
  set_sleep_mode(mode);
  __builtin_avr_cli();
  if (!PENDING) {
    sleep_enable();
    __builtin_avr_sei();
    __builtin_avr_sleep();
    sleep_disable();
  }
  PENDING = 0;
  __builtin_avr_sei();
}

}
