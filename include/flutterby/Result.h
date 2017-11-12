#pragma once
#include "flutterby/Traits.h"
#include "flutterby/Types.h"
#include "flutterby/Progmem.h"
#include "flutterby/New.h"

/** The Result type borrows from its namesake in Rust.
 * It forces the programmer to consider the possibility of error.
 */

namespace flutterby {

// A type that indicates that no failures are possible
struct Infallible {};

[[noreturn]] extern void
panicImpl(FlashString file, u16 line, FlashString reason);

#define panic(reason) panicImpl(__FILE__ ""_P, __LINE__, reason)

template <typename Value, typename ErrorType>
class[[nodiscard]] Result {
  enum class State { kEmpty, kOk, kError };

  struct ErrorConstruct {};

 public:
  using value_type = Value;
  using error_type = ErrorType;

  Result() : state_(State::kEmpty) {}
  ~Result() {
    switch (state_) {
      case State::kEmpty:
        break;
      case State::kOk:
        value_.~Value();
        break;
      case State::kError:
        error_.~ErrorType();
        break;
    }
  }

  // Default construct a successful Value result
  inline static Result Ok() {
    return Result(Value());
  }

  // Copy in a successful Value result
  inline static Result Ok(const Value& value) {
    return Result(value);
  }

  // Move in a successful Value result
  inline static Result Ok(Value && value) {
    return Result(move(value));
  }

  // Default construct an error result
  inline static Result Error() {
    return Result(ErrorType(), ErrorConstruct{});
  }

  // Copy in an error result
  inline static Result Error(const ErrorType& error) {
    return Result(error, ErrorConstruct{});
  }

  // Move construct an error result
  inline static Result Error(ErrorType && error) {
    return Result(move(error), ErrorConstruct{});
  }

  // Copy a value into the result
  explicit Result(const Value& other) : state_(State::kOk), value_(other) {}

  // Move in value
  explicit Result(Value && other) : state_(State::kOk), value_(move(other)) {}

  // Copy in error
  Result(const ErrorType& error, ErrorConstruct)
      : state_(State::kError), error_(error) {}

  // Move in error
  Result(ErrorType && error, ErrorConstruct)
      : state_(State::kError), error_(move(error)) {}

  // Move construct
  Result(Result && other) noexcept : state_(other.state_) {
    switch (state_) {
      case State::kEmpty:
        break;
      case State::kOk:
        new (&value_) Value(move(other.value_));
        break;
      case State::kError:
        new (&error_) ErrorType(move(other.error_));
        break;
    }
    other.~Result();
    other.state_ = State::kEmpty;
  }

  // Move assign
  Result& operator=(Result&& other) noexcept {
    if (&other != this) {
      this->~Result();

      state_ = other.state_;
      switch (state_) {
        case State::kEmpty:
          break;
        case State::kOk:
          new (&value_) Value(move(other.value_));
          break;
        case State::kError:
          new (&error_) ErrorType(move(other.error_));
          break;
      }

      other.~Result();
      other.state_ = State::kEmpty;
    }
    return *this;
  }

  // Copy construct
  Result(const Result& other) {
    state_ = other.state_;
    switch (state_) {
      case State::kEmpty:
        break;
      case State::kOk:
        new (&value_) Value(other.value_);
        break;
      case State::kError:
        new (&error_) ErrorType(other.error_);
        break;
    }
  }

  // Copy assign
  Result& operator=(const Result& other) {
    if (&other != this) {
      this->~Result();
      state_ = other.state_;
      switch (state_) {
        case State::kEmpty:
          break;
        case State::kOk:
          new (&value_) Value(other.value_);
          break;
        case State::kError:
          new (&error_) ErrorType(other.error_);
          break;
      }
    }
    return *this;
  }

  bool is_ok() const {
    return state_ == State::kOk;
  }

  bool is_err() const {
    return state_ == State::kError;
  }

  bool is_empty() const {
    return state_ == State::kEmpty;
  }

  // If Result does not contain a valid Value, panic
  void panicIfError() const {
    switch (state_) {
      case State::kOk:
        return;
      case State::kEmpty:
        panic("Uninitialized Result"_P);
      case State::kError:
        panic("Result holds Error, not Value"_P);
    }
  }

  // Get a mutable reference to the value.  If the value is
  // not assigned, a panic will be issued by panicIfError().
  Value& value()& {
    panicIfError();
    return value_;
  }

  // Get an rvalue reference to the value.  If the value is
  // not assigned, a panic will be issued by panicIfError().
  Value&& value()&& {
    panicIfError();
    return move(value_);
  }

  // Get a const reference to the value.  If the value is
  // not assigned, a panic will be issued by panicIfError().
  const Value& value() const & {
    panicIfError();
    return value_;
  }

  template <typename T>
  void set_ok(T && value) {
    *this = move(Result<Value, ErrorType>::Ok(value));
  }

  template <typename T>
  void set_err(T && error) {
    *this = move(Result<Value, ErrorType>::Error(error));
  }

  void panicIfNotError() {
    switch (state_) {
      case State::kOk:
        panic("Result holds Value, not Error"_P);
      case State::kEmpty:
        panic("Uninitialized Result"_P);
      case State::kError:
        return;
    }
  }

  // Get a mutable reference to the error.  If the error is
  // not assigned, a panic will be issued by panicIfNotError().
  ErrorType& error()& {
    panicIfNotError();
    return error_;
  }

  // Get an rvalue reference to the error.  If the error is
  // not assigned, a panic will be issued by panicIfNotError().
  ErrorType&& error()&& {
    panicIfNotError();
    return error_;
  }

  // Get a const reference to the error.  If the error is
  // not assigned, a panic will be issued by panicIfNotError().
  const ErrorType& error() const & {
    panicIfNotError();
    return error_;
  }

 private:
  State state_;
  union {
    Value value_;
    ErrorType error_;
  };
};

template <>
class[[nodiscard]] Result<Unit, Unit> {
  enum class State : u8 { kEmpty, kOk, kError };
  State state_;

  constexpr Result(State state) : state_(state) {}

 public:
  using value_type = Unit;
  using error_type = Unit;
  constexpr Result() : state_(State::kEmpty) {}
  constexpr Result(const Result& other) : state_(other.state_) {}
  constexpr Result(Result && other) noexcept : state_(other.state_) {}
  inline Result& operator=(Result&& other) noexcept {
    state_ = other.state_;
    return *this;
  }
  inline Result& operator=(const Result& other) {
    state_ = other.state_;
    return *this;
  }

  constexpr static Result Ok() {
    return Result(State::kOk);
  }

  constexpr static Result Error() {
    return Result(State::kError);
  }

  constexpr bool is_ok() const {
    return state_ == State::kOk;
  }

  constexpr bool is_err() const {
    return state_ == State::kError;
  }

  constexpr bool is_empty() const {
    return state_ == State::kEmpty;
  }

  void set_ok() {
    state_ = State::kOk;
  }

  void set_err() {
    state_ = State::kError;
  }

  // If Result does not contain a valid Value, panic
  void panicIfError() const {
    switch (state_) {
      case State::kOk:
        return;
      case State::kEmpty:
        panic("Uninitialized Result"_P);
      case State::kError:
        panic("Result holds Error, not Value"_P);
    }
  }

  Unit value() const {
    panicIfError();
    return Unit{};
  }

  void panicIfNotError() const {
    switch (state_) {
      case State::kOk:
        panic("Result holds Value, not Error"_P);
      case State::kEmpty:
        panic("Uninitialized Result"_P);
      case State::kError:
        return;
    }
  }

  Unit error() const {
    panicIfNotError();
    return Unit{};
  }
};

template <>
class[[nodiscard]] Result<Unit, Infallible> {
 public:
  using value_type = Unit;
  using error_type = Infallible;

  constexpr static Result Ok() {
    return Result();
  }

  constexpr bool is_ok() const {
    return true;
  }

  constexpr bool is_err() const {
    return false;
  }

  constexpr bool is_empty() const {
    return false;
  }

  Unit value() {
    return Unit{};
  }

  [[noreturn]] Unit error() {
    panic("Result is infallible"_P);
  }
};

template <typename ValueType = Unit, typename ErrorType = Unit>
constexpr Result<ValueType, ErrorType> Ok() {
  return Result<ValueType, ErrorType>::Ok();
}

template <typename ValueType = Unit, typename ErrorType = Unit>
constexpr Result<ValueType, ErrorType> Error() {
  return Result<ValueType, ErrorType>::Error();
}

template <typename ValueType, typename ErrorType = Unit>
constexpr Result<ValueType, ErrorType> Ok(ValueType&& value) {
  return Result<ValueType, ErrorType>::Ok(move(value));
}

template <typename ValueType=Unit, typename ErrorType>
constexpr Result<ValueType, ErrorType> Error(ErrorType&& value) {
  return Result<ValueType, ErrorType>::Error(move(value));
}

/** Try is similar to the try! macro in rust.
 * Its purpose is to reduce some boiler plate in sections of code
 * that want to return early and bubble up error results.
 * EXPR is some expression that returns a Result type.  If that
 * result is an error then the macro will return the result immediately.
 * Otherwise the macro evaluates to the ValueType of the result.
 */
#define Try(expr)                     \
  ({                                  \
    auto __result = expr;             \
    if (__result.is_err()) {         \
      return __result; \
    }                                 \
    __result.value();                 \
  })
}
