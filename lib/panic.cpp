#include "avr_autogen.h"
#include "flutterby/Result.h"
#include "flutterby/Debug.h"

using namespace flutterby;

extern "C"
[[noreturn]]
void exit(int result);

extern "C"[[noreturn]] void __cxa_pure_virtual(void) {
  panic("__cxa_pure_virtual"_P);
}

namespace flutterby {

void panicImpl(){
  exit(1);
}

void panicImpl(FlashString file, uint16_t line, FlashString reason) {
  DBG() << "\x1b[1;mPANIC:"_P << file << ":"_P << line << " "_P << reason
        << "\x1b[0;m"_P;

  panicImpl();
}
}
