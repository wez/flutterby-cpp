#pragma once
// Integer sequences are useful in template metaprogramming
#include <stddef.h>

namespace flutterby {

template <class T, T... Ints>
struct integer_sequence {
  using value_type = T;

  static constexpr size_t size() noexcept {
    return sizeof...(Ints);
  }
};

template <size_t... Ints>
using index_sequence = flutterby::integer_sequence<size_t, Ints...>;

namespace detail {
template <size_t N, size_t... Ints>
struct make_index_sequence
    : detail::make_index_sequence<N - 1, N - 1, Ints...> {};

template <size_t... Ints>
struct make_index_sequence<0, Ints...> : flutterby::index_sequence<Ints...> {};
}

template <size_t N>
using make_index_sequence = detail::make_index_sequence<N>;
}
