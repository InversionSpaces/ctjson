#pragma once

#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace ctjson::detail {
/**
 * @brief This class maintain the path in json, updating on each token
 */
class Path {
  /**
   * @brief Path component for object
   */
  struct Object {
    std::optional<std::string> key = std::nullopt;

    /**
     * @return rendered path component
     */
    std::string render() const { return key ? "." + key.value() : ""; }
  };

  /**
   * @brief Path component for array
   */
  struct Array {
    ssize_t index = -1;

    /**
     * @return rendered path component
     */
    std::string render() const {
      return index == -1 ? "" : "[" + std::to_string(index) + "]";
    }
  };

  using PathComponent = std::variant<Object, Array>;

public:
  /**
   * Called on StartObject token
   */
  void start_object() {
    advance_array_if_needed();
    m_path.emplace_back(std::in_place_type<Object>);
  }

  /**
   * Called on Key token
   */
  void key(std::string key) {
    auto &object = std::get<Object>(m_path.back());
    object.key = std::move(key);
  }

  /**
   * Called on EndObject token
   */
  void end_object() {
    // TODO: Check that last item is Object
    m_path.pop_back();
  }

  /**
   * Called on StartArray token
   */
  void start_array() {
    advance_array_if_needed();
    m_path.emplace_back(std::in_place_type<Array>);
  }

  /**
   * Called on any Value token
   */
  void value() { advance_array_if_needed(); }

  /**
   * Called on EndArray token
   */
  void end_array() {
    // TODO: Check that last item is Array
    m_path.pop_back();
  }

  /**
   * @return Rendered path begining with "root"
   */
  std::string render() const {
    std::string result = "root";
    for (const auto &component : m_path) {
      result += std::visit([](const auto &c) { return c.render(); }, component);
    }

    return result;
  }

private:
  /**
   * Called on new object, array or value
   */
  void advance_array_if_needed() {
    if (m_path.empty() || !std::holds_alternative<Array>(m_path.back())) {
      return;
    }

    auto &array = std::get<Array>(m_path.back());
    ++array.index;
  }

private:
  std::vector<PathComponent> m_path;
};
} // namespace ctjson::detail