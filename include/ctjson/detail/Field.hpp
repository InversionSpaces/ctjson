#pragma once

#include <string>

#include <ctjson/detail/Typing.hpp>

namespace ctjson::detail {

/**
 * @brief Base class for helper field classes
 *
 * @tparam T type of field (may be const)
 */
template <typename T>
class FieldBase {
public:
  using type = T;

public:
  /**
   * This class should not be returned or stored
   */
  FieldBase(FieldBase &&other) = delete;
  FieldBase(const FieldBase &other) = delete;

protected:
  /**
   * @param name name of filed - key in json
   * @param ref reference to field value
   */
  FieldBase(std::string name, T &ref) : name(std::move(name)), m_ref(ref) {}

public:
  const std::string name;

protected:
  static constexpr bool is_opt = detail::is_optional_v<T>;

  T &m_ref;
};

} // namespace ctjson::detail