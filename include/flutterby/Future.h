#pragma once
#include "flutterby/Option.h"

/** Futures
 * The Future<> type indicates the Result of some work that may complete
 * in the future.
 * The primary method on Future<> is Future<>::poll() which is used to determine
 * if the result is ready.
 * In the context of an event driven microcontroller running concurrent tasks
 * we want to avoid blocking on any single Future so there are no conveniences
 * for doing so.  Instead, the intent is that you spawn() the future into
 * a manager that will take responsibility for tracking all of the futures
 * and driving them until completion.
 *
 * The intended usage is something along the lines of:
 *
 * ```
 *    spawn(button_pressed().and_then([] {
 *        panic("the button was pressed"_P);
 *        }))
 * ```
 *
 * where `button_pressed` returns a Future that completes when a button is
 * pressed.
 */

namespace flutterby {

template <typename ValueType, typename ErrorType, typename Impl>
class[[nodiscard]] Future;

// A detail namespace for future related things
namespace future {

// Extracts the return type of a functor call
template <typename F, typename... Args>
using resultOf = decltype(declval<F>()(declval<Args>()...));

template <typename ValueType, typename ErrorType>
class ResultFuture {
  Result<ValueType, ErrorType> result_;

 public:
  ResultFuture(Result<ValueType, ErrorType>&& result) : result_(move(result)) {}

  Option<Result<ValueType, ErrorType>> operator()() {
    if (result_.is_empty()) {
      panic("Future already complete"_P);
    }
    return Some(move(result_));
  }
};

// a trait to test whether something is a Future
template <typename T>
struct isFuture : false_type {};

// a trait to test whether something is a Result
template <typename T>
struct isResult : false_type {};

// if we match a future, make it convenient to extract associated types
template <typename ValueType, typename ErrorType, typename Impl>
struct isFuture<Future<ValueType, ErrorType, Impl>> : true_type {
  using result_type = Result<ValueType, ErrorType>;
  using future_type = Future<ValueType, ErrorType, Impl>;
};

// if we match a result, make it convenient to extract associated types
template <typename ValueType, typename ErrorType>
struct isResult<Result<ValueType, ErrorType>> : true_type {
  using value_type = ValueType;
  using error_type = ErrorType;
  using result_type = Result<ValueType, ErrorType>;
  using future_type = Future<ValueType, ErrorType, ResultFuture<ValueType, ErrorType>>;
};

// A helper to encapsulate the "and_then" combinator logic.
// The intent is: if the LHS is successful, invoke Func with the Ok data.
// Otherwise the error result from the LHS is wrapped up in the Result type
// that matches the return type of Func and propagated.
template <
    typename NextResult,
    typename PriorFuture,
    typename NextFuture,
    typename Func,
    typename Apply>
auto poll_and_then(
    Option<PriorFuture>& prior,
    Option<NextFuture>& next,
    Func& func,
    Apply&& apply) {
  if (prior.is_some()) {
    auto status = prior.value().poll();

    if (!status.is_some()) {
      return Option<NextResult>::None();
    }

    auto& prior_result = status.value();
    if (prior_result.is_ok()) {
      // map the Ok value
      next = Some(apply(func(move(prior_result.value()))));
      prior.clear();
    } else {
      // Propagate the Error value
      prior.clear();
      next.clear();
      return Some(NextResult::Error(move(prior_result.error())));
    }
  }
  if (next.is_some()) {
    auto status = next.value().poll();

    if (!status.is_some()) {
      return Option<NextResult>::None();
    }

    next.clear();
    return status;
  }

  return Option<NextResult>::None();
}

// A helper to encapsulate the "or_else" combinator logic.
// The intent is: if the LHS is error, invoke Func with the Error data.
// Otherwise the Ok result from the LHS is wrapped up in the Result type
// that matches the return type of Func and propagated.
template <
    typename NextResult,
    typename PriorFuture,
    typename NextFuture,
    typename Func,
    typename Apply>
auto poll_or_else(
    Option<PriorFuture>& prior,
    Option<NextFuture>& next,
    Func& func,
    Apply&& apply) {
  if (prior.is_some()) {
    auto status = prior.value().poll();

    if (!status.is_some()) {
      return Option<NextResult>::None();
    }

    auto& prior_result = status.value();

    if (prior_result.is_err()) {
      // map the Error value
      next = Some(apply(func(move(prior_result.error()))));
      prior.clear();
    } else {
      // Propagate the Ok value
      prior.clear();
      next.clear();
      return Some(NextResult::Ok(move(prior_result.value())));
    }
  }
  if (next.is_some()) {
    auto status = next.value().poll();

    if (!status.is_some()) {
      return Option<NextResult>::None();
    }

    next.clear();
    return status;
  }

  return Option<NextResult>::None();
}

} // namespace detail

template <typename ValueType, typename ErrorType, typename Impl>
class[[nodiscard]] Future {
  Option<Impl> impl_;

 public:
  using value_type = ValueType;
  using error_type = ErrorType;
  using result_type = Result<ValueType, ErrorType>;
  using poll_type = Option<result_type>;

  explicit Future(Impl && impl) : impl_(Some(move(impl))) {}

  Option<Result<ValueType, ErrorType>> poll() {
    if (impl_.is_none()) {
      panic("Future already complete"_P);
    }
    auto& impl = impl_.value();

    auto status = impl();
    if (status.is_some()) {
      impl_.clear();
    }
    return status;
  }

  /** Future.and_then([](ValueType) -> NextValueType */
  template <
      typename Func,
      typename enable_if<
          !future::isResult<future::resultOf<Func, ValueType&&>>::value &&
          !future::isFuture<future::resultOf<Func, ValueType&&>>::value,
          int>::type = 0>
  auto and_then(Func && func)&& {
    using NextValue = typename future::resultOf<Func, ValueType&&>;
    using PriorFuture = Future<ValueType, ErrorType, Impl>;
    using NextFuture = Future<
        NextValue,
        ErrorType,
        future::ResultFuture<NextValue, ErrorType>>;
    using NextResult = Result<NextValue, ErrorType>;
    struct ThenValue {
      Option<PriorFuture> prior_;
      Option<NextFuture> next_;
      Func func_;

      Option<NextResult> operator()() {
        return future::poll_and_then<NextResult>(
            prior_, next_, func_, [](ValueType&& result) {
              return NextFuture(NextResult(move(result)));
            });
      }
    };
    return Future<ValueType, ErrorType, ThenValue>(
        ThenValue{move(*this), Option<NextFuture>::None(), move(func)});
  }

  /** Future.and_then([](ValueType) -> Result<NextValue, ErrorType>) */
  template <
      typename Func,
      typename enable_if<
          future::isResult<future::resultOf<Func, ValueType&&>>::value,
          int>::type = 0>
  auto and_then(Func && func)&& {
    using ResultTrait = future::isResult<future::resultOf<Func, ValueType&&>>;
    using NextValue = typename ResultTrait::value_type;
    using PriorFuture = Future<ValueType, ErrorType, Impl>;
    using NextFuture = typename ResultTrait::future_type;
    using NextResult = typename ResultTrait::result_type;
    struct ThenResult {
      Option<PriorFuture> prior_;
      Option<NextFuture> next_;
      Func func_;

      Option<NextResult> operator()() {
        return future::poll_and_then<NextResult>(
            prior_,
            next_,
            func_,
            [](typename ResultTrait::result_type&& result) {
              return NextFuture(move(result));
            });
      }
    };
    return Future<ValueType, ErrorType, ThenResult>(
        ThenResult{move(*this), Option<NextFuture>::None(), move(func)});
  }

  /** Future.and_then([](ValueType) -> Future<NextValue, ErrorType>) */
  template <
      typename Func,
      typename enable_if<
          future::isFuture<future::resultOf<Func, ValueType&&>>::value,
          int>::type = 0>
  auto and_then(Func && func)&& {
    using Trait = future::isFuture<future::resultOf<Func, ValueType&&>>;
    using PriorFuture = Future<ValueType, ErrorType, Impl>;
    using NextFuture = typename Trait::future_type;
    using NextResult = typename Trait::result_type;
    struct ThenFuture {
      Option<PriorFuture> prior_;
      Option<NextFuture> next_;
      Func func_;

      typename NextFuture::poll_type operator()() {
        return future::poll_and_then<NextResult>(
            prior_, next_, func_, [](NextFuture&& result) { return result; });
      }
    };
    return Future<ValueType, ErrorType, ThenFuture>(
        ThenFuture{move(*this), Option<NextFuture>::None(), move(func)});
  }


  /** Future.or_else([](ErrorType) -> NextErrorType */
  template <
      typename Func,
      typename enable_if<
          !future::isResult<future::resultOf<Func, ErrorType&&>>::value &&
          !future::isFuture<future::resultOf<Func, ErrorType&&>>::value,
          int>::type = 0>
  auto or_else(Func && func)&& {
    using NextError = typename future::resultOf<Func, ErrorType&&>;
    using PriorFuture = Future<ValueType, ErrorType, Impl>;
    using NextFuture = Future<
        ValueType,
        NextError,
        future::ResultFuture<ValueType, NextError>>;
    using NextResult = Result<ValueType, NextError>;
    struct ElseValue {
      Option<PriorFuture> prior_;
      Option<NextFuture> next_;
      Func func_;

      Option<NextResult> operator()() {
        return future::poll_or_else<NextResult>(
            prior_, next_, func_, [](NextError&& result) {
              return NextFuture(NextResult::Error(move(result)));
            });
      }
    };
    return Future<ValueType, NextError, ElseValue>(
        ElseValue{move(*this), Option<NextFuture>::None(), move(func)});
  }

  /** Future.or_else([](ErrorType) -> Result<NextValue, ErrorType>) */
  template <
      typename Func,
      typename enable_if<
          future::isResult<future::resultOf<Func, ErrorType&&>>::value,
          int>::type = 0>
  auto or_else(Func && func)&& {
    using ResultTrait = future::isResult<future::resultOf<Func, ErrorType&&>>;
    using NextValue = typename ResultTrait::value_type;
    using PriorFuture = Future<ValueType, ErrorType, Impl>;
    using NextFuture = typename ResultTrait::future_type;
    using NextResult = typename ResultTrait::result_type;
    struct ElseResult {
      Option<PriorFuture> prior_;
      Option<NextFuture> next_;
      Func func_;

      Option<NextResult> operator()() {
        return future::poll_or_else<NextResult>(
            prior_,
            next_,
            func_,
            [](typename ResultTrait::result_type&& result) {
              return NextFuture(move(result));
            });
      }
    };
    return Future<ValueType, ErrorType, ElseResult>(
        ElseResult{move(*this), Option<NextFuture>::None(), move(func)});
  }

  /** Future.or_else([](ErrorType) -> Future<NextValue, ErrorType>) */
  template <
      typename Func,
      typename enable_if<
          future::isFuture<future::resultOf<Func, ErrorType&&>>::value,
          int>::type = 0>
  auto or_else(Func && func)&& {
    using Trait = future::isFuture<future::resultOf<Func, ErrorType&&>>;
    using PriorFuture = Future<ValueType, ErrorType, Impl>;
    using NextFuture = typename Trait::future_type;
    using NextResult = typename Trait::result_type;
    struct ElseFuture {
      Option<PriorFuture> prior_;
      Option<NextFuture> next_;
      Func func_;

      typename NextFuture::poll_type operator()() {
        return future::poll_or_else<NextResult>(
            prior_, next_, func_, [](NextFuture&& result) { return result; });
      }
    };
    return Future<ValueType, ErrorType, ElseFuture>(
        ElseFuture{move(*this), Option<NextFuture>::None(), move(func)});
  }
};

namespace future {

/** Pollable is used by spawn() to box up a Future and track it.
 * You are not supposed to create instances of this type for yourself. */
class Pollable {
  Pollable* next_;

 public:
  virtual ~Pollable();
  virtual bool poll() = 0;

  static void spawn(Pollable* p);
  static bool poll_all();
};
}

/** Given a Result, construct a Future<> instance that is immediately ready */
template <typename ValueType = Unit, typename ErrorType = Unit>
auto make_future(Result<ValueType, ErrorType>&& result) {
  return Future<
      ValueType,
      ErrorType,
      future::ResultFuture<ValueType, ErrorType>>(
      future::ResultFuture<ValueType, ErrorType>(move(result)));
}

/** Given some Future<Unit, Unit>, track and drive it until it is complete.
 * Note that the ValueType and ErrorType are both Unit; you are expected
 * to use combinators on your chain of Futures to consume and handle any
 * value or error produced by its computation.
 * The futures are driven by the run_loop() function.
 */
template <typename Impl>
void spawn(Future<Unit, Unit, Impl>&& fut) {
  class PollFuture : public future::Pollable {
    Future<Unit, Unit, Impl> fut_;

   public:
    PollFuture(Future<Unit, Unit, Impl>&& fut) : fut_(move(fut)) {}
    bool poll() override {
      return fut_.poll().is_some();
    }
  };
  auto poll = new PollFuture(move(fut));
  future::Pollable::spawn(poll);
}

/** Drive all futures.
 * Does not return; you'll need to spawn() at least one future before
 * calling this for it to be meaningful */
[[noreturn]]
extern void run_loop();

}
