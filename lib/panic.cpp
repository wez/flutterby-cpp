#include "avr_autogen.h"
#include "flutterby/Result.h"
#include "flutterby/Debug.h"

extern "C"
[[noreturn]]
void exit(int result);

namespace flutterby {

void panicImpl(FlashString file, uint16_t line, FlashString reason) {
  DBG() << "\x1b[1;mPANIC:"_P << file << ":"_P << line << " "_P << reason
        << "\x1b[0;m"_P;
  exit(1);
}
}
