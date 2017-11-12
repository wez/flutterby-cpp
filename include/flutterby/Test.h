#pragma once
#include "flutterby/Debug.h"
#include "flutterby/Progmem.h"

extern "C" void exit(int) __attribute__((noreturn));
namespace flutterby {

#define EXPECT(expr)                              \
  do {                                            \
    if (!(expr)) {                                \
      DBG() << "\x1b[1;mFAIL: "_P << #expr ""_P   \
            << " at "_P << __FILE__ ""_P          \
            << ":"_P << __LINE__ << "\x1b[0;m"_P; \
      exit(1);                                    \
    }                                             \
  } while (0)

#define EXPECT_EQ(lhs, rhs)                                     \
  do {                                                          \
    auto __lhs = (lhs);                                         \
    auto __rhs = (rhs);                                         \
    if (__lhs != __rhs) {                                       \
      DBG() << "\x1b[1;mFAIL: "_P << __lhs << " != "_P << __rhs \
            << " (" #lhs " != " #rhs ") at "_P << __FILE__ ""_P \
            << ":"_P << __LINE__ << "\x1b[0;m"_P;               \
      exit(1);                                                  \
    }                                                           \
  } while (0)

extern "C" {
uint8_t* __data_load_start;
uint8_t* __data_load_end;
}

inline size_t data_segment_size() {
  return __data_load_end - __data_load_start;
}

}
