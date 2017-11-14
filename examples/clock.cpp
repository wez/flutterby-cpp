#include "avr_autogen.h"
#include "flutterby/EventLoop.h"
#include "flutterby/Debug.h"
#include "flutterby/Serial0.h"

/** This is intended to drive an atmega328p in the Solder Time Desk Clock */

using namespace flutterby;

int main() {
  __builtin_avr_cli();
#ifdef HAVE_AVR_WDT
  __builtin_avr_wdr();
  Wdt::wdtcsr = WdtWdtcsrFlags::WDCE | WdtWdtcsrFlags::WDE;
  Wdt::wdtcsr.clear();
  Cpu::mcusr ^= CpuMcusrFlags::WDRF;
#endif

  Serial0::configure(9600);

  auto timer = make_timer(2_s, true, [] { TXSER() << "Boop"_P; }).value();
  eventloop::enable_timer(timer);
  eventloop::run_forever();

  return 0;
}
