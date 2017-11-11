#pragma once
#include "flutterby/New.h"
#include "flutterby/Result.h"
#include "flutterby/Traits.h"
#include "flutterby/Types.h"
#include "flutterby/Variant.h"

namespace flutterby {

template <typename T>
class Option {
  u8 empty_;
  union {
    u8 dummy_;
    T value_;
  };

 public:
  ~Option() {
    if (!empty_) {
      value_.~T();
    }
  }

  constexpr Option() : empty_{true} {}
  constexpr Option(const T& other) : empty_{false}, value_(other) {}
  constexpr Option(T&& other) : empty_{false}, value_(move(other)) {}
  constexpr Option(const Option& other) : empty_{false}, value_(other.value_) {}
  Option(Option&& other) noexcept {
    if (other.empty_) {
      empty_ = true;
    } else {
      empty_ = false;
      new (&value_) T(move(other.value_));
      // We define the move of an option to cause the source to be emptied
      other.clear();
    }
  }

  Option& operator=(const Option& other) {
    if (&other != this) {
      this->~Option();
      if (other.empty_) {
        empty_ = true;
      } else {
        empty_ = false;
        new (&value_) T(other.value_);
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
        new (&value_) T(move(other.value_));
      }
      // We define the move of an option to cause the source to be emptied
      other.clear();
    }
    return *this;
  }

  void clear() {
    this->~Option();
    empty_ = true;
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
  T& value() & {
    panicIfEmpty();
    return value_;
  }

  // Get an rvalue reference to the value.  If the value is
  // not assigned, a panic will be issued by panicIfEmpty().
  T&& value() && {
    panicIfEmpty();
    return move(value_);
  }

  // Get a const reference to the value.  If the value is
  // not assigned, a panic will be issued by panicIfEmpty().
  const T& value() const & {
    panicIfEmpty();
    return value_;
  }

  void panicIfEmpty() const {
    if (empty_) {
      panic("Empty Option"_P);
    }
  }

  static constexpr Option None() {
    return Option();
  }
};

/** Specialization of Option<Variant> so that we can save 1 byte of RAM.
 * This takes advantage of intimate knowledge of Variant and simply stores
 * a variant inside itself.  We use the kDestroyed state of the variant
 * to represent the empty state of the variant. */
template <>
template <typename ...Types>
class Option<Variant<Types...>> {
  using VariantType = Variant<Types...>;
  u8 storage_[VariantType::storage_size];

 public:
  ~Option() {
    reinterpret_cast<VariantType*>(this)->~VariantType();
  }

  constexpr Option() {
    storage_[0] = variant::kDestroyed;
  }
  constexpr Option(const VariantType& other) {
    new (storage_) VariantType(other);
  }
  constexpr Option(VariantType&& other) {
    new (storage_) VariantType(move(other));
  }

  constexpr Option(const Option& other) {
    new (storage_) VariantType(*reinterpret_cast<VariantType*>(other.storage_));
  }

  Option(Option&& other) noexcept {
    new (storage_)
        VariantType(move(*reinterpret_cast<VariantType*>(other.storage_)));
    // We define the move of an option to cause the source to be emptied
    other.clear();
  }

  Option& operator=(const Option& other) {
    if (&other != this) {
      this->~Option();
      new (storage_)
          VariantType(*reinterpret_cast<VariantType*>(other.storage_));
    }
    return *this;
  }

  Option& operator=(Option&& other) {
    if (&other != this) {
      this->~Option();
      new (storage_)
          VariantType(move(*reinterpret_cast<VariantType*>(other.storage_)));
      // We define the move of an option to cause the source to be emptied
      other.clear();
    }
    return *this;
  }

  void clear() {
    this->~Option();
    storage_[0] = variant::kDestroyed;
  }

  template <typename U>
  void set_ok(U&& value) {
    this->~Option();
    new (storage_) VariantType(move(value));
  }

  constexpr bool is_none() const {
    return storage_[0] == variant::kDestroyed;
  }

  constexpr bool is_some() const {
    return storage_[0] != variant::kDestroyed;
  }

  // Get a mutable reference to the value.  If the value is
  // not assigned, a panic will be issued by panicIfEmpty().
  VariantType& value() & {
    panicIfEmpty();
    return *reinterpret_cast<VariantType*>(storage_);
  }

  // Get an rvalue reference to the value.  If the value is
  // not assigned, a panic will be issued by panicIfEmpty().
  VariantType&& value() && {
    panicIfEmpty();
    return move(*reinterpret_cast<VariantType*>(storage_));
  }

  // Get a const reference to the value.  If the value is
  // not assigned, a panic will be issued by panicIfEmpty().
  const VariantType& value() const & {
    panicIfEmpty();
    return *reinterpret_cast<VariantType*>(storage_);
  }

  void panicIfEmpty() const {
    if (storage_[0] == variant::kDestroyed) {
      panic("Empty Option"_P);
    }
  }

  static constexpr Option None() {
    return Option();
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
