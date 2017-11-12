#pragma once
#include "flutterby/Result.h"

/** Heap functions.
 * It's convenient to use dynamically allocated memory in a variety
 * of situations, even though such practice is generally frowned upon
 * in embedded applications.  It's ok to use the heap so long as you
 * have considered the cardinality of the heap allocations.  The best
 * practice is to heap allocate a small number of things at the start
 * of the application and then try to avoid bursting over that volume
 * of heap allocated data so that you avoid running out of RAM later
 * in the life of the application.
 *
 * This module provides Unique<T> and Shared<T> to track unique and
 * shared ownership of pointer values respectively.
 * They are allocated via make_unique() and make_shared() which return
 * a Result<T> holding the result.  If you are allocating heap memory
 * later in the life of your application, it is strongly recommended
 * that you propagate allocation failures using the Try() macro to
 * avoid panicking the system.
 */

extern "C" {
void free(void*);
void* malloc(unsigned int);
}

inline void* operator new(unsigned int size) {
  return malloc(size);
}

inline void operator delete(void* ptr, unsigned int) {
  free(ptr);
}

namespace flutterby {

/** Unique represents a heap managed item with a sole owner.
 * When Unique is destroyed, it will delete the owned item.
 */
template <typename T>
class Unique {
  T* ptr_={nullptr};

 public:
  using value_type = T;
  Unique() = default;
  Unique(T* ptr) : ptr_(ptr) {}
  ~Unique() {
    if (ptr_) {
      delete ptr_;
    }
  }

  /** No copying */
  Unique(const Unique&) =delete;
  Unique& operator=(const Unique&) = delete;

  /** Moves are allowed; they transfer ownership */
  Unique(Unique&& other) : ptr_(other.ptr_) {
    other.ptr_ = nullptr;
  }
  Unique& operator=(Unique&&other) {
    if (&other != this) {
      reset();
      ptr_ = other.ptr_;
      other.ptr_ = nullptr;
    }
    return *this;
  }

  /** Returns the raw pointer.
   * The lifetime is NOT extended; it is unsafe to retain
   * the raw pointer unless you take your own measures to
   * ensure that the Unique lives long enough */
  inline T* get() const {
    return ptr_;
  }

  /** Transfer ownership of the pointer from Unique to the
   * caller.  Unique will no longer attempt to delete it.
   * The caller is now responsible for deleting the pointer
   * at the appropriate time. */
  T* release() {
    auto result = ptr_;
    ptr_ = nullptr;
    return result;
  }

  /** Deref to the stored item */
  T& operator*() const {
    return get();
  }

  T* operator->() const {
    return get();
  }

  /** Clear the pointer, deleting it and resetting it to nullptr */
  void reset() {
    if (ptr_) {
      delete ptr_;
      ptr_ = nullptr;
    }
  }
};

/** Helper to construct a Unique.
 * Returns a Result to indicate the success of the operation. */
template <typename T, typename ...Args>
Result<Unique<T>, Unit> make_unique(Args&&...args) {
  auto ptr = new T(forward<Args>(args)...);
  if (ptr) {
    return Ok(Unique<T>(ptr));
  } else {
    return Error<Unique<T>>();
  }
}

template <typename T>
struct Shared {
  struct Control {
    uint8_t ref;
  };
  Control* control_{nullptr};

  inline Shared(Control* control) : control_(control) {}

 public:
  using value_type = T;

  Shared() = default;

  ~Shared() {
    if (control_) {
      if (--control_->ref == 0) {
        get()->~T();
        free(control_);
      }
    }
  }

  /** Construct Shared<T> from Shared<U>, but only if U derives from T */
  template <
      typename U,
      typename enable_if<is_base_of<T, U>::value, int>::type = 0>
  Shared(const Shared<U>& other)
      : control_(reinterpret_cast<Control*>(
            const_cast<typename Shared<U>::Control*>(other.control_))) {
    if (control_) {
      ++control_->ref;
    }
  }

  Shared(const Shared& other) : control_(other.control_) {
    if (control_) {
      ++control_->ref;
    }
  }
  Shared& operator=(const Shared&other) {
    if (&other != this) {
      reset();
      control_ = other.control_;
      if (control_) {
        ++control_->ref;
      }
    }
    return *this;
  }

  Shared(Shared&& other) : control_(other.control_) {
    other.control_ = nullptr;
  }
  Shared& operator=(Shared&& other) {
    if (&other != this) {
      reset();
      control_ = other.control_;
      other.control_ = nullptr;
    }
    return *this;
  }

  /** Returns the raw pointer.
   * The lifetime is NOT extended; it is unsafe to retain
   * the raw pointer unless you take your own measures to
   * ensure that the Shared lives long enough */
  inline T* get() const {
    return control_ ? (T*)(&control_->ref + 1) : nullptr;
  }

  /** Deref to the stored item */
  T& operator*() const {
    return *get();
  }

  T* operator->() const {
    return get();
  }

  /** Clear the pointer, deleting it (if we were the last ref)
   * and resetting it to nullptr */
  void reset() {
    if (control_) {
      if (--control_->ref == 0) {
        get()->~T();
        free(control_);
        control_ = nullptr;
      }
    }
  }

  /** Claim the current reference to the pointer from this Shared instance.
   * The caller is responsible for managing the lifetime of the pointer;
   * the only valid way to do so is to call from_raw() with this pointer
   * at a later time. */
  T* into_raw() {
    if (!control_) {
      panic("into_raw called on empty Shared"_P);
    }
    auto ptr = get();
    control_ = nullptr;
    return ptr;
  }

  /** Restore a Shared instanced from a pointer that was previously returned
   * from into_raw().  It is invalid to pass any other pointer to this
   * method. */
  static Shared from_raw(T* ptr) {
    return Shared((Control*)(((u8*)ptr) - 1));
  }

  /* helper for making a new one.  You probably want the
   * free function make_shared instead of this, as it can
   * deduce the argument and return types more ergonomically. */
  template <typename... Args>
  static inline Result<Shared<T>, Unit> make(Args&&... args) {
    auto control = (Control*)malloc(sizeof(Control) + sizeof(T));

    if (!control) {
      return Error<Shared<T>>();
    }
    control->ref = 1;
    new (&control->ref + 1) T(forward<Args>(args)...);

    return Ok(Shared<T>(Shared{control}));
  }
};

template <typename T, typename... Args>
static inline Result<Shared<T>, Unit> make_shared(Args&&... args) {
  return Shared<T>::make(forward<Args>(args)...);
}

}
