#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <type_traits>

#include <ctjson/Serializable.hpp>

#include <ctjson/detail/Typing.hpp>

namespace ctjson {
class Serializer {
public:
  template <typename T, typename Writer>
  static inline std::enable_if_t<detail::is_json_value<T>>
  dump(const T &value, Writer &writer) {
    if constexpr (std::is_same_v<T, bool>) {
      writer.boolean(value);
    } else if constexpr (std::is_integral_v<T>) {
      writer.integer(value);
    } else if constexpr (std::is_floating_point_v<T>) {
      writer.floating(value);
    } else if constexpr (std::is_same_v<T, std::string>) {
      writer.string(value);
    } else {
      static_assert(!sizeof(T), "Unexpected value type");
    }
  }

  template <typename T, typename Writer>
  static inline std::enable_if_t<detail::is_optional_v<T>>
  dump(const T &value, Writer &writer) {
    using ValueType = typename T::value_type;

    if (value.has_value()) {
      dump<ValueType>(value.value(), writer);
    } else {
      writer.null();
    }
  }

  template <typename T, typename Writer>
  static inline std::enable_if_t<detail::is_array_like_v<T>>
  dump(const T &value, Writer &writer) {
    using ValueType = typename T::value_type;

    writer.start_array();
    for (const auto &m : value) {
      dump<ValueType>(m, writer);
    }
    writer.end_array();
  }

  template <typename T, typename Writer>
  static inline std::enable_if_t<detail::is_dict_like_v<T>>
  dump(const T &value, Writer &writer) {
    using ValueType = typename T::mapped_type;

    writer.start_object();
    for (const auto &[k, v] : value) {
      writer.key(k);
      dump<ValueType>(v, writer);
    }
    writer.end_object();
  }

  template <typename T, typename Writer>
  static inline std::enable_if_t<
      detail::has_dump_v<T, void, const T &, Writer &>>
  dump(const T &value, Writer &writer) {
    T::json_dump(value, writer);
  }

  template <typename T, typename Writer>
  static inline std::enable_if_t<Serializable<T, Writer>::value>
  dump(const T &value, Writer &writer) {
    Serializable<T, Writer>::dump(value, writer);
  }
};
}; // namespace ctjson