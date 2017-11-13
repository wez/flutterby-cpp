#include "avr_autogen.h"
#include "flutterby/EventLoop.h"
#include "flutterby/Debug.h"
#include "flutterby/Serial0.h"

/** This is intended to drive an atmega328p in the Solder Time Desk Clock */

using namespace flutterby;

int main() {
  __builtin_avr_cli();
  Cpu::clkpr = CpuClkprFlags::CLKPCE;
  Cpu::clkpr = CpuClkprFlags::CLKPS_1;
#ifdef HAVE_AVR_WDT
  __builtin_avr_wdr();
  Wdt::wdtcsr = WdtWdtcsrFlags::WDCE | WdtWdtcsrFlags::WDE;
  Wdt::wdtcsr.clear();
  Cpu::mcusr ^= CpuMcusrFlags::WDRF;
#endif

  // Weirdness; both here and in the Arduino IDE I find that the baud
  // rate is off by a factor of 2.  The math looks right; both the
  // code in Serial0.h and Arduino match up to the datasheet, but
  // I find that I have to specify half the speed set here when connecting
  // to the device with a serial monitor via an FTDI cable.
  Serial0::configure(9600);

  auto timer = make_timer(2_s, true, [] { TXSER() << "Boop"_P; }).value();
  eventloop::enable_timer(timer);
  eventloop::run_forever();

  return 0;
}
