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

template <typename T, T v> struct integral_constant {
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
}
