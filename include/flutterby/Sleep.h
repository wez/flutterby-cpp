#pragma once
#include "flutterby/Types.h"

namespace flutterby {

// http://microchipdeveloper.com/8avr:avrsleep
// has more information on sleep modes.
enum class SleepMode {
  Idle,
  ADCNoiseReduction,
  PowerDown,
  PowerSave,
  StandyBy,
  ExtendedStandBy,
};

/** Configures the sleep mode, but doesn't sleep the CPU */
void set_sleep_mode(SleepMode mode);

/** Intended to be called from a ISR that is queuing up work or otherwise
 * setting a flag for work to be done in the main non-interrupt context.
 * Setting pending status will avoid a race between the start of the
 * decision to initiate a sleep and an interrupt coming in while we
 * are setting up to sleep. */
static inline void set_event_pending() {
  extern volatile u8 PENDING;
  PENDING = 1;
}

/** Put the CPU into sleep mode, blocking until an interrupt occurs.
 * Clears any pending event state that may have been set by set_event_pending()
 */
void wait_for_event(SleepMode mode);

}
