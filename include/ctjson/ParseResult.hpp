#pragma once

#include <optional>
#include <string>
#include <type_traits>

namespace ctjson {

namespace detail {
/**
 * @brief Base class for parse results
 *
 * It makes @ref ErrorType and @ref Error common classes
 * for any templated parse result
 */
class ParseResultBase {
protected:
  enum class ErrorType {
    NO_ERROR,
    JSON_ERROR,  // Error in json structure
    PARSE_ERROR, // Error while parsing to domain
  };

public:
  /**
   * @brief Error information
   */
  struct Error {
    std::string error;               // Error message
    std::optional<std::string> path; // Path in json

    std::string render() const {
      auto result = error;
      if (path) {
        result += " at " + path.value();
      }

      return result;
    }
  };

protected:
  ParseResultBase() = default;
  ParseResultBase(ErrorType error_type, Error error)
      : m_error_type(error_type), m_error(std::move(error)) {}

protected:
  const ErrorType m_error_type = ErrorType::NO_ERROR;
  std::optional<Error> m_error = std::nullopt;
};

/**
 * @brief Base class for parse results holding value
 */
template <typename T>
class ParseResultValue {
protected:
  ParseResultValue() = default;
  ParseResultValue(T value) : m_value(std::move(value)) {}

public:
  /**
   * @return value of this result
   */
  T value() && {
    auto result = std::move(m_value.value());
    return result;
  }

protected:
  std::optional<T> m_value = std::nullopt;
};

/**
 * @brief Specialization for void - no value
 */
template <>
class ParseResultValue<void> {};
} // namespace detail

/**
 * @brief Representation of parse result - error or value
 */
template <typename T>
class ParseResult : public detail::ParseResultBase,
                    public detail::ParseResultValue<T> {
  template <typename U>
  friend class ParseResult;

  /**
   * @brief Error constructor
   */
  ParseResult(ErrorType error_type, Error error)
      : detail::ParseResultBase(error_type, std::move(error)),
        detail::ParseResultValue<T>() {}

  /**
   * @brief Value constructor, enabled only if T != void
   */
  template <typename U = T,
            typename = std::enable_if_t<!std::is_same_v<U, void>>>
  ParseResult(std::enable_if_t<std::is_same_v<U, T>, U> value)
      : detail::ParseResultBase(), detail::ParseResultValue<T>(
                                       std::move(value)) {}

  /**
   * @brief Value constructor, enabled only if T == void
   */
  template <typename U = T,
            typename = std::enable_if_t<std::is_same_v<U, void>>>
  ParseResult() : detail::ParseResultBase(), detail::ParseResultValue<T>() {}

public:
  /**
   * @brief    Method for creating result with value,
   *           enabled if T == void
   *
   * @return result
   */
  template <typename U = T,
            typename = std::enable_if_t<std::is_same_v<U, void>>>
  static ParseResult result() {
    static_assert(std::is_same_v<U, T>,
                  "Do not substitute template arguments!");
    return ParseResult();
  }

  /**
   * @brief    Method for creating result with value,
   *           enabled if T != void
   *
   * @return result containing value
   */
  template <typename U = T,
            typename = std::enable_if_t<!std::is_same_v<U, void>>>
  static ParseResult result(std::enable_if_t<std::is_same_v<U, T>, U> value) {
    static_assert(std::is_same_v<U, T>,
                  "Do not substitute template arguments!");
    return ParseResult(std::move(value));
  }

  /**
   * @brief Method for creating result with error in json parsing
   *
   * @param error error message
   * @param path optional path in json where error occured
   * @return result containing json error
   */
  static ParseResult
  json_error(std::string error,
             std::optional<std::string> path = std::nullopt) {
    return ParseResult(ErrorType::JSON_ERROR,
                       {.error = std::move(error), .path = std::move(path)});
  }

  /**
   * @brief Method for creating result with error in parsing to domain
   *
   * @param error error message
   * @param path optional path in json where error occured
   * @return result containing parse error
   */
  static ParseResult
  parse_error(std::string error,
              std::optional<std::string> path = std::nullopt) {
    return ParseResult(ErrorType::PARSE_ERROR,
                       {.error = std::move(error), .path = std::move(path)});
  }

  /**
   * @brief Method for converting error result of another type
   *
   * @param other result to convert
   * @return result with the same error as @ref other
   * @pre other.is_ok() == false
   */
  template <typename U>
  static ParseResult convert_error(ParseResult<U> &&other) {
    return ParseResult(other.m_error_type, std::move(other.m_error.value()));
  }

  /**
   * @return true if this contains json error
   */
  bool is_json_error() const { return m_error_type == ErrorType::JSON_ERROR; }

  /**
   * @return true if this contains parse error
   */
  bool is_parse_error() const { return m_error_type == ErrorType::PARSE_ERROR; }

  /**
   * @return true if this contains value
   */
  bool is_ok() const { return m_error_type == ErrorType::NO_ERROR; }

  /**
   * @return Error information
   * @pre this.is_ok() == false
   */
  Error error() && {
    auto result = std::move(m_error.value());
    return result;
  }
};
} // namespace ctjson