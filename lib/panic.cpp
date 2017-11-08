#include "atmega32u4.h"
#include "flutterby/Result.h"
#include "flutterby/Debug.h"

extern "C"
[[noreturn]]
void exit(int result);

namespace flutterby {

void panicImpl(FlashString file, uint16_t line, FlashString reason) {
  FormatStream<SimavrConsoleStream> s;
  s << "PANIC:"_P << file << ":"_P << line << " "_P << reason << "\r\n"_P;
  exit(1);
}
}
