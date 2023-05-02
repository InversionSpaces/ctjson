#pragma once

#include <initializer_list>
#include <limits>
#include <tuple>
#include <type_traits>

namespace ctjson::detail {

template <typename I, I... ints, typename F>
void compile_switch(I i, std::integer_sequence<I, ints...>, F f) {
  // This code is believed to be compiled to effecient assembly
  std::initializer_list<int>{
      (i == ints ? (f(std::integral_constant<I, ints>{}), 0) : 0)...};
}

template <typename F, typename... Args>
void call_on_nth(std::size_t n, F f, Args &&...args) {
  compile_switch(n, std::index_sequence_for<Args...>{}, [&](auto i) {
    f(std::get<i>(std::forward_as_tuple(std::forward<Args>(args)...)));
  });
}

/**
 * C++20 helpers, see [https://en.cppreference.com/w/cpp/utility/intcmp]
 */
template <class T, class U>
constexpr bool cmp_equal(T t, U u) noexcept {
  using UT = std::make_unsigned_t<T>;
  using UU = std::make_unsigned_t<U>;
  if constexpr (std::is_signed_v<T> == std::is_signed_v<U>)
    return t == u;
  else if constexpr (std::is_signed_v<T>)
    return t < 0 ? false : UT(t) == u;
  else
    return u < 0 ? false : t == UU(u);
}

template <class T, class U>
constexpr bool cmp_not_equal(T t, U u) noexcept {
  return !cmp_equal(t, u);
}

template <class T, class U>
constexpr bool cmp_less(T t, U u) noexcept {
  using UT = std::make_unsigned_t<T>;
  using UU = std::make_unsigned_t<U>;
  if constexpr (std::is_signed_v<T> == std::is_signed_v<U>)
    return t < u;
  else if constexpr (std::is_signed_v<T>)
    return t < 0 ? true : UT(t) < u;
  else
    return u < 0 ? false : t < UU(u);
}

template <class T, class U>
constexpr bool cmp_greater(T t, U u) noexcept {
  return cmp_less(u, t);
}

template <class T, class U>
constexpr bool cmp_less_equal(T t, U u) noexcept {
  return !cmp_greater(t, u);
}

template <class T, class U>
constexpr bool cmp_greater_equal(T t, U u) noexcept {
  return !cmp_less(t, u);
}

/**
 * C++20 helper, see [https://en.cppreference.com/w/cpp/utility/in_range]
 */
template <class R, class T>
constexpr bool in_range(T t) noexcept {
  return cmp_greater_equal(t, std::numeric_limits<R>::min()) &&
         cmp_less_equal(t, std::numeric_limits<R>::max());
}

} // namespace ctjson::detail