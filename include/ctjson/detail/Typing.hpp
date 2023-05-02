#pragma once

#include <map>
#include <optional>
#include <set>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <ctjson/ParseResult.hpp>

namespace ctjson::detail {

/**
 * @brief Is @tparam T basic json value (number or string)
 */
template <typename T>
constexpr bool is_json_value =
    std::is_arithmetic_v<T> || std::is_same_v<std::string, T>;

template <typename T>
struct is_optional : std::false_type {};

template <typename T>
struct is_optional<std::optional<T>> : std::true_type {};

/**
 * @brief Is @tparam T std::optional of some other type
 */
template <typename T>
constexpr static bool is_optional_v = is_optional<T>::value;

template <typename T>
struct is_array_like : std::false_type {};

template <typename T>
struct is_array_like<std::vector<T>> : std::true_type {
  template <typename... Args>
  static inline void emplace(std::vector<T> &v, Args &&...args) {
    v.emplace_back(std::forward<Args>(args)...);
  }
};

template <typename T>
struct is_array_like<std::set<T>> : std::true_type {
  template <typename... Args>
  static inline void emplace(std::set<T> &s, Args &&...args) {
    s.emplace(std::forward<Args>(args)...);
  }
};

template <typename T>
struct is_array_like<std::unordered_set<T>> : std::true_type {
  template <typename... Args>
  static inline void emplace(std::unordered_set<T> &s, Args &&...args) {
    s.emplace(std::forward<Args>(args)...);
  }
};

/**
 * @brief Is @tparam T array like from some other type (std::vector, std::set,
 * std::unordered_set)
 */
template <typename T>
constexpr static bool is_array_like_v = is_array_like<T>::value;

template <typename T>
struct is_dict_like : std::false_type {};

template <typename T>
struct is_dict_like<std::map<std::string, T>> : std::true_type {
  template <typename... Args>
  static inline void emplace(std::map<std::string, T> &m, Args &&...args) {
    m.emplace(std::forward<Args>(args)...);
  }
};

template <typename T>
struct is_dict_like<std::unordered_map<std::string, T>> : std::true_type {
  template <typename... Args>
  static inline void emplace(std::unordered_map<std::string, T> &m,
                             Args &&...args) {
    m.emplace(std::forward<Args>(args)...);
  }
};

/**
 * @brief Is @tparam T dict like from some other type (std::map<std::string,
 * ...>, std::unordered_map<std::string, ...>)
 */
template <typename T>
constexpr static bool is_dict_like_v = is_dict_like<T>::value;

template <typename C, typename Ret, typename... Args>
class has_parse {
  template <typename T>
  static constexpr auto check(T *) ->
      typename std::is_same<decltype(T::json_parse(std::declval<Args>()...)),
                            Ret>::type;

  template <typename>
  static constexpr std::false_type check(...);

  using type = decltype(check<C>(0));

public:
  static constexpr bool value = type::value;
};

/**
 * @brief Does @tparam C has static method called `json_parse` with signature
 * Ret(Args...)
 */
template <typename C, typename Ret, typename... Args>
constexpr bool has_parse_v = has_parse<C, Ret, Args...>::value;

template <typename C, typename Ret, typename... Args>
class has_dump {
  template <typename T>
  static constexpr auto check(T *) ->
      typename std::is_same<decltype(T::json_dump(std::declval<Args>()...)),
                            Ret>::type;

  template <typename>
  static constexpr std::false_type check(...);

  using type = decltype(check<C>(0));

public:
  static constexpr bool value = type::value;
};

/**
 * @brief Does @tparam C has static method called `json_dump` with signature
 * Ret(Args...)
 */
template <typename C, typename Ret, typename... Args>
constexpr bool has_dump_v = has_dump<C, Ret, Args...>::value;

template <typename T>
struct is_parse_result : std::false_type {};

template <typename T>
struct is_parse_result<ParseResult<T>> : std::true_type {};

template <typename T>
constexpr static bool is_parse_result_v = is_parse_result<T>::value;

} // namespace ctjson::detail