#include "flutterby/Heap.h"
#include "flutterby/Future.h"
namespace flutterby {
namespace future {

Pollable *POLLABLES = nullptr;

Pollable::~Pollable() {}

void Pollable::spawn(Pollable *p) {
  p->next_ = POLLABLES;
  POLLABLES = p;
}

bool Pollable::have_pollables() {
  return POLLABLES != nullptr;
}

bool Pollable::poll_all() {
  bool did_any = false;

  Pollable** prev_next = &POLLABLES;
  auto p = *prev_next;
  while (p) {
    if (p->poll()) {
      did_any = true;

      auto to_free = p;
      *prev_next = to_free->next_;
      p = to_free->next_;

      delete to_free;
      continue;
    }

    prev_next = &p->next_;
    p = p->next_;
  }

  return did_any;
}
}
}
