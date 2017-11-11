#pragma once
#include "flutterby/New.h"
#include "flutterby/Traits.h"


/** A (slightly) simple Variant implementation.
 * How this is implemented is a bit of a difficult read but thankfully
 * knowing how it works isn't required to use it.  The purpose of the
 * Variant type is a type-safe alternative to a plain union.
 * https://chadaustin.me/2015/07/sum-types/ has more information on
 * this concept.
 *
 * To use:
 *
 * ```
 * struct Foo { int foo; };
 * struct Bar { bool flag; };
 * Variant<Foo, Bar> v(Foo{123});
 * ```
 *
 * This particular Variant implementation does not allow absence of values;
 * it will always contain one of the member types.  If you need to support
 * absence of a value you can use `Option<Variant<>>`; there is a specialization
 * of Option when used with variant that doesn't take up any additional space.
 *
 * This Variant implementation has a hard limit of 254 member types.
 */
namespace flutterby {

namespace variant {

template <size_t arg1, size_t... others>
struct static_max;

template <size_t arg>
struct static_max<arg> {
  static const size_t value = arg;
};

template <size_t arg1, size_t arg2, size_t... others>
struct static_max<arg1, arg2, others...> {
  static const size_t value = arg1 >= arg2 ? static_max<arg1, others...>::value
                                           : static_max<arg2, others...>::value;
};

static constexpr u8 kDestroyed = 0xff;

template <u8 I, class T>
struct tuple_element;

template <class Head, class... Tail>
struct tuple {};

// recursive case
template <u8 I, class Head, class... Tail>
struct tuple_element<I, tuple<Head, Tail...>>
    : tuple_element<I - 1, tuple<Tail...>> {};

// base case
template <class Head, class... Tail>
struct tuple_element<0, tuple<Head, Tail...>> {
  typedef Head type;
};

// Helper for resolving T to its index value
template <typename T, typename... Types>
struct type_to_index;

template <typename T, typename First, typename... Types>
struct type_to_index<T, First, Types...> {
  static constexpr u8 index = is_same<T, First>::value
      ? sizeof...(Types)
      : type_to_index<T, Types...>::index;
};

// Any other type is invalid (kDestroyed)
template <typename T>
struct type_to_index<T> {
  static constexpr u8 index = kDestroyed;
};

template <typename T, typename... Types>
struct value_traits {
  // Resolve T to its index within Types.
  using value_type =
      typename remove_const<typename remove_reference<T>::type>::type;
  static constexpr u8 index = type_to_index<value_type, Types...>::index;
  static constexpr bool is_valid = index != kDestroyed;

  // tuple_index is used to resolve T to its stored type.  index 0 resolves
  // to the void type and is used to match incompatible types and cause
  // an error at compilation time.
  static constexpr u8 tuple_index = is_valid ? sizeof...(Types) - index : 0;
  using target_type =
      typename tuple_element<tuple_index, tuple<void, Types...>>::type;
};

template <typename... Types>
struct recursive_helper;

template <typename First, typename... Types>
struct recursive_helper<First, Types...> {
  static inline void destroy(const u8 idx, void* data) {
    if (idx == sizeof...(Types)) {
      reinterpret_cast<First*>(data)->~First();
      return;
    }
    recursive_helper<Types...>::destroy(idx, data);
  }

  static inline void move(const u8 idx, void* src, void* data) {
    if (idx == sizeof...(Types)) {
      new (data) First(flutterby::move(*reinterpret_cast<First*>(src)));
      return;
    }
    recursive_helper<Types...>::move(idx, src, data);
  }

  static inline void copy(const u8 idx, void* src, void* data) {
    if (idx == sizeof...(Types)) {
      new (data) First(*reinterpret_cast<First*>(src));
      return;
    }
    recursive_helper<Types...>::move(idx, src, data);
  }
};

template <>
struct recursive_helper<> {
  static inline void destroy(const u8 idx, void* data) {}
  static inline void move(const u8 idx, void* src, void* data) {}
  static inline void copy(const u8 idx, void* src, void* data) {}
};
}

template <typename First, typename... Types>
class Variant {
 public:
  static constexpr size_t storage_size =
      1 + variant::static_max<sizeof(First), sizeof(Types)...>::value;

 private:
  using helper_type = variant::recursive_helper<First, Types...>;

  u8 storage_[storage_size];

 public:
  // Only explicit initialization is allowed; use Option
  // to express the absence of the value.
  Variant() = delete;

  ~Variant() {
    helper_type::destroy(storage_[0], (void*)&storage_[1]);
  }

  constexpr Variant(const Variant& other) {
    storage_[0] = other.storage_[0];
    helper_type::copy(
        storage_[0], (void*)other.storage_[1], (void*)&storage_[1]);
  }

  Variant& operator=(const Variant& other) {
    if (&other != this) {
      helper_type::destroy(storage_[0], (void*)&storage_[1]);
      storage_[0] = other.storage_[0];
      helper_type::copy(
          storage_[0], (void*)other.storage_[1], (void*)&storage_[1]);
    }
    return *this;
  }

  constexpr Variant(Variant&& other) {
    storage_[0] = other.storage_[0];
    helper_type::move(
        storage_[0], (void*)other.storage_[1], (void*)&storage_[1]);
  }

  Variant& operator=(Variant&& other) {
    if (&other != this) {
      helper_type::destroy(storage_[0], (void*)&storage_[1]);
      storage_[0] = other.storage_[0];
      helper_type::move(
          storage_[0], (void*)other.storage_[1], (void*)&storage_[1]);
    }
    return *this;
  }

  // Only allow explicit initialization from one of the member types
  template <
      typename T,
      typename Traits = variant::value_traits<T, First, Types...>,
      typename Enable = typename enable_if<Traits::is_valid>::type>
  constexpr Variant(T&& val) {
    storage_[0] = Traits::index;
    new (&storage_[1]) typename Traits::target_type(forward<T>(val));
  }

  template <
      typename T,
      typename enable_if<
          (variant::type_to_index<T, First, Types...>::index !=
           variant::kDestroyed)>::type* = nullptr>
  inline bool is() const {
    return storage_[0] == variant::type_to_index<T, First, Types...>::index;
  }

  template <
      typename T,
      typename enable_if<
          (variant::type_to_index<T, First, Types...>::index !=
           variant::kDestroyed)>::type* = nullptr>
  inline T* get_ptr() {
    if (storage_[0] == variant::type_to_index<T, First, Types...>::index) {
      return reinterpret_cast<T*>(&storage_[1]);
    }
    return nullptr;
  }

  template <
      typename T,
      typename V = T,
      typename enable_if<
          (variant::type_to_index<T, First, Types...>::index !=
           variant::kDestroyed)>::type* = nullptr>
  void set(V&& v) {
    helper_type::destroy(storage_[0], (void*)&storage_[1]);
    storage_[0] = variant::type_to_index<T, First, Types...>::index;
    helper_type::move(storage_[0], (void*)&v, (void*)&storage_[1]);
  }
};
}
