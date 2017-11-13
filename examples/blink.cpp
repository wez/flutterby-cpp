#include "avr_autogen.h"
#include "flutterby/EventLoop.h"
#include "flutterby/Debug.h"

using namespace flutterby;

int main() {
  __builtin_avr_cli();
#ifdef HAVE_AVR_USB_DEVICE
  UsbDevice::usbcon.clear();
  Cpu::clkpr = CpuClkprFlags::CLKPS_1;
#endif
#ifdef HAVE_AVR_WDT
  __builtin_avr_wdr();
  Wdt::wdtcsr = WdtWdtcsrFlags::WDCE | WdtWdtcsrFlags::WDE;
  Wdt::wdtcsr.clear();
  Cpu::mcusr ^= CpuMcusrFlags::WDRF;
#endif

#ifdef __AVR_ATmega32U4__
  Portc::ddrc = PortcSignalFlags::PC7;
  Portc::portc.clear();
#endif

  auto timer = make_timer(2_s, true, [] {
#ifdef __AVR_ATmega32U4__
                 Portc::portc ^= PortcSignalFlags::PC7;
#endif
                 DBG() << "CALLBACK"_P;
               }).value();
  eventloop::enable_timer(timer);
  eventloop::run_forever();

  return 0;
}
