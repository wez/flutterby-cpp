#pragma once
#include <inttypes.h>
#include <stdint.h>

namespace flutterby {

template <class T>
struct remove_reference {
  typedef T type;
};
template <class T>
struct remove_reference<T&> {
  typedef T type;
};
template <class T>
struct remove_reference<T&&> {
  typedef T type;
};

template <class T>
typename remove_reference<T>::type&& move(T&& t) {
  return static_cast<typename remove_reference<T>::type&&>(t);
}

template <class T>
struct remove_const {
  typedef T type;
};
template <class T>
struct remove_const<const T> {
  typedef T type;
};

template <class T>
struct remove_volatile {
  typedef T type;
};
template <class T>
struct remove_volatile<volatile T> {
  typedef T type;
};

template <class T>
struct remove_cv {
  typedef typename remove_volatile<typename remove_const<T>::type>::type type;
};

template <bool Condition, class IfTrue, class IfFalse>
struct conditional {
  using type = IfTrue;
};

template <class IfTrue, class IfFalse>
struct conditional<false, IfTrue, IfFalse> {
  using type = IfFalse;
};

template <bool B, class T = void>
struct enable_if {};

template <class T>
struct enable_if<true, T> {
  using type = T;
};

template <typename T, T v>
struct integral_constant {
  static constexpr const T value = v;
};
struct false_type : integral_constant<bool, false> {};
struct true_type : integral_constant<bool, true> {};

template <typename T, typename U>
struct is_same final : false_type {};

template <typename T>
struct is_same<T, T> final : true_type {};

template <typename T, typename U>
constexpr const bool is_same_v = is_same<T, U>::value;

template <class T>
struct is_void : public false_type {};

template <>
struct is_void<void> : public true_type {};
template <>
struct is_void<const void> : public true_type {};
template <>
struct is_void<const volatile void> : public true_type {};
template <>
struct is_void<volatile void> : public true_type {};

template <class T>
struct is_rvalue_reference : public false_type {};
template <class T>
struct is_rvalue_reference<T&&> : public true_type {};

template <class T>
struct is_lvalue_reference : public false_type {};
template <class T>
struct is_lvalue_reference<T&> : public true_type {};

template <class T>
struct is_reference
    : public integral_constant<
          bool,
          is_lvalue_reference<T>::value || is_rvalue_reference<T>::value> {};

template <typename T, bool b>
struct add_rvalue_reference_helper {
  using type = T;
};

template <typename T>
struct add_rvalue_reference_helper<T, true> {
  using type = T&&;
};

template <typename T>
struct add_rvalue_reference_imp {
  using type = typename add_rvalue_reference_helper<
      T,
      (is_void<T>::value == false && is_reference<T>::value == false)>::type;
};

template <class T>
struct add_rvalue_reference {
  typedef typename add_rvalue_reference_imp<T>::type type;
};

template <typename T>
typename add_rvalue_reference<T>::type
declval() noexcept; // as unevaluated operand

template <class T>
constexpr inline T&& forward(typename remove_reference<T>::type& t) noexcept {
  return static_cast<T&&>(t);
}

template <class T>
constexpr inline T&& forward(typename remove_reference<T>::type&& t) noexcept {
  static_assert(
      !is_lvalue_reference<T>::value,
      "Can not forward an rvalue as an lvalue.");
  return static_cast<T&&>(t);
}

template <class T>
struct remove_extent {
  typedef T type;
};

template <class T>
struct remove_extent<T[]> {
  typedef T type;
};

template <class T, size_t N>
struct remove_extent<T[N]> {
  typedef T type;
};

// primary template
template <class>
struct is_function : false_type {};

// specialization for regular functions
template <class Ret, class... Args>
struct is_function<Ret(Args...)> : true_type {};

// specialization for variadic functions such as printf
template <class Ret, class... Args>
struct is_function<Ret(Args......)> : true_type {};

// specialization for function types that have cv-qualifiers
template <class Ret, class... Args>
struct is_function<Ret(Args...) const> : true_type {};
template <class Ret, class... Args>
struct is_function<Ret(Args...) volatile> : true_type {};
template <class Ret, class... Args>
struct is_function<Ret(Args...) const volatile> : true_type {};
template <class Ret, class... Args>
struct is_function<Ret(Args......) const> : true_type {};
template <class Ret, class... Args>
struct is_function<Ret(Args......) volatile> : true_type {};
template <class Ret, class... Args>
struct is_function<Ret(Args......) const volatile> : true_type {};

// specialization for function types that have ref-qualifiers
template <class Ret, class... Args>
struct is_function<Ret(Args...)&> : true_type {};
template <class Ret, class... Args>
struct is_function<Ret(Args...) const &> : true_type {};
template <class Ret, class... Args>
struct is_function<Ret(Args...) volatile&> : true_type {};
template <class Ret, class... Args>
struct is_function<Ret(Args...) const volatile&> : true_type {};
template <class Ret, class... Args>
struct is_function<Ret(Args......)&> : true_type {};
template <class Ret, class... Args>
struct is_function<Ret(Args......) const &> : true_type {};
template <class Ret, class... Args>
struct is_function<Ret(Args......) volatile&> : true_type {};
template <class Ret, class... Args>
struct is_function<Ret(Args......) const volatile&> : true_type {};
template <class Ret, class... Args>
struct is_function<Ret(Args...) &&> : true_type {};
template <class Ret, class... Args>
struct is_function<Ret(Args...) const &&> : true_type {};
template <class Ret, class... Args>
struct is_function<Ret(Args...) volatile&&> : true_type {};
template <class Ret, class... Args>
struct is_function<Ret(Args...) const volatile&&> : true_type {};
template <class Ret, class... Args>
struct is_function<Ret(Args......) &&> : true_type {};
template <class Ret, class... Args>
struct is_function<Ret(Args......) const &&> : true_type {};
template <class Ret, class... Args>
struct is_function<Ret(Args......) volatile&&> : true_type {};
template <class Ret, class... Args>
struct is_function<Ret(Args......) const volatile&&> : true_type {};

namespace detail {
template <class T, bool is_function_type = false>
struct add_pointer {
  using type = typename remove_reference<T>::type*;
};

template <class T>
struct add_pointer<T, true> {
  using type = T;
};

template <class T, class... Args>
struct add_pointer<T(Args...), true> {
  using type = T (*)(Args...);
};

template <class T, class... Args>
struct add_pointer<T(Args..., ...), true> {
  using type = T (*)(Args..., ...);
};

} // namespace detail

template <class T>
struct add_pointer : detail::add_pointer<T, is_function<T>::value> {};

template <class T>
struct is_array : false_type {};

template <class T>
struct is_array<T[]> : true_type {};

template <class T, size_t N>
struct is_array<T[N]> : true_type {};

template <class T>
struct decay {
 private:
  typedef typename remove_reference<T>::type U;

 public:
  typedef typename conditional<
      is_array<U>::value,
      typename remove_extent<U>::type*,
      typename conditional<
          is_function<U>::value,
          typename add_pointer<U>::type,
          typename remove_cv<U>::type>::type>::type type;
};

// Extracts the return type of a functor call
template <typename F, typename... Args>
using result_of_functor = decltype(declval<F>()(declval<Args>()...));

}
