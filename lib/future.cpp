#include "flutterby/Future.h"
namespace flutterby {
namespace future {

Pollable *POLLABLES = nullptr;

Pollable::~Pollable() {}

void Pollable::spawn(Pollable *p) {
  p->next_ = POLLABLES;
  POLLABLES = p;
}

bool Pollable::poll_all() {
  bool did_any = false;

  Pollable** prev_ptr = &POLLABLES;
  auto p = POLLABLES;
  while (p) {
    if (p->poll()) {
      did_any = true;

      auto to_free = p;
      (*prev_ptr)->next_ = to_free->next_;
      p = to_free->next_;

      delete p;
      continue;
    }

    prev_ptr = &p->next_;
    p = p->next_;
  }

  return did_any;
}
}
}
