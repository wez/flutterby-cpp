#pragma once
#include "flutterby/Traits.h"
#include "flutterby/Result.h"
namespace flutterby {

template <typename T>
class Option {
  uint8_t empty_;
  union {
    uint8_t dummy_;
    T value_;
  };

 public:
  ~Option() {
    if (!empty_) {
      value_.~T();
    }
  }

  constexpr Option() : empty_{true} {}
  constexpr explicit Option(const T& other) : empty_{false}, value_(other) {}
  constexpr explicit Option(T&& other) : empty_{false}, value_(move(other)) {}
  constexpr explicit Option(const Option& other)
      : empty_{false}, value_(other.value_) {}
  explicit Option(Option&& other) {
    if (other.empty_) {
      empty_ = true;
    } else {
      empty_ = false;
      value_ = move(other.value_);
      other.~Option();
      other.empty_ = true;
    }
  }

  Option&operator=(const Option& other) {
    if (&other != this) {
      this->~Option();
      if (other.empty_) {
        empty_ = true;
      } else {
        empty_ = false;
        value_ = other.value_;
      }
    }
    return *this;
  }

  Option& operator=(Option&& other) {
    if (&other != this) {
      this->~Option();
      if (other.empty_) {
        empty_ = true;
      } else {
        empty_ = false;
        value_ = move(other.value_);
      }
      other.~Option();
      other.empty_ = true;
    }
    return *this;
  }

  void clear() {
    this->~Option();
    empty_ = false;
  }

  template <typename U>
  void set_ok(U&& value) {
    this->~Option();
    value_ = move(value);
    empty_ = false;
  }

  constexpr bool is_none() const {
    return empty_;
  }

  constexpr bool is_some() const {
    return !empty_;
  }
  // Get a mutable reference to the value.  If the value is
  // not assigned, a panic will be issued by panicIfEmpty().
  T& value()& {
    panicIfEmpty();
    return value_;
  }

  // Get an rvalue reference to the value.  If the value is
  // not assigned, a panic will be issued by panicIfEmpty().
  T&& value()&& {
    panicIfEmpty();
    return value_;
  }

  // Get a const reference to the value.  If the value is
  // not assigned, a panic will be issued by panicIfEmpty().
  const T& value() const & {
    panicIfEmpty();
    return value_;
  }

  void panicIfEmpty() {
    if (empty_) {
      panic("Empty Option"_P);
    }
  }
};

template <typename T>
constexpr Option<T> None() {
  return Option<T>();
}

template <typename T>
constexpr Option<T> Some(const T& value) {
  return Option<T>(value);
}

template <typename T>
constexpr Option<T> Some(T&& value) {
  return Option<T>(move(value));
}


}
