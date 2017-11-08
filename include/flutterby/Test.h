#pragma once
extern "C" void exit(int) __attribute__((noreturn));
namespace flutterby {

#define EXPECT(expr)                       \
  do {                                     \
    if (!expr) {                           \
      FormatStream<SimavrConsoleStream> s; \
      s << "FAIL: "_P << #expr ""_P        \
        << " at "_P << __FILE__ ""_P       \
        << ":"_P << __LINE__ << "\r\n"_P;  \
      exit(1);                             \
    }                                      \
  } while (0)

extern "C" {
uint8_t* __data_load_start;
uint8_t* __data_load_end;
}

inline size_t data_segment_size() {
  return __data_load_end - __data_load_start;
}

}
