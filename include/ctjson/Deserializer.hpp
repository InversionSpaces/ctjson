#pragma once

#include <cstdint>
#include <optional>
#include <string>

#include <ctjson/Deserializable.hpp>
#include <ctjson/ParseResult.hpp>

#include <ctjson/detail/Token.hpp>
#include <ctjson/detail/TypeUtils.hpp>
#include <ctjson/detail/Typing.hpp>

namespace ctjson {

/**
 * @brief Class for parsing token stream to domain classes
 *
 * Usage example:
 * @code{.cpp}
 * auto tokens = ...;
 * auto result = Deserializer::parse<MyType>(tokens);
 * if (result.is_ok()) {
 *   auto value = std::move(result).value();
 * } else {
 *   auto error = std::move(result).error();
 * }
 * @endcode
 */
class Deserializer {
public:
  /**
   * @brief Specialization for values: numbers and strings (numbers,
   * std::string)
   */
  template <typename T, typename Tokens>
  static inline std::enable_if_t<detail::is_json_value<T>, ParseResult<T>>
  parse(Tokens &tokens) {
    auto maybeToken = tokens.next();
    if (!maybeToken) {
      if (tokens.has_error()) {
        return ParseResult<T>::json_error(tokens.get_error(),
                                          tokens.get_path());
      } else {
        return ParseResult<T>::parse_error(unexpected_end_error(),
                                           tokens.get_path());
      }
    }

    auto &token = maybeToken.value();

    return parse_value<T>(token, tokens.get_path());
  }

  /**
   * @brief Specialization for optional: null or T (std::optional<T>)
   */
  template <typename T, typename Tokens>
  static inline std::enable_if_t<detail::is_optional_v<T>, ParseResult<T>>
  parse(Tokens &tokens) {
    using ValueType = typename T::value_type;

    const auto &maybeToken = tokens.peek();
    if (!maybeToken) {
      if (tokens.has_error()) {
        return ParseResult<T>::json_error(tokens.get_error(),
                                          tokens.get_path());
      } else {
        // TODO: Provide better error
        return ParseResult<T>::parse_error(unexpected_end_error(),
                                           tokens.get_path());
      }
    }

    const auto &token = maybeToken.value();
    if (token.template is_of_type<detail::Token::Type::Null>()) {
      tokens.next();

      return ParseResult<T>::result(std::nullopt);
    }

    auto result = parse<ValueType>(tokens);
    if (result.is_ok()) {
      return ParseResult<T>::result(std::move(result).value());
    }

    return ParseResult<T>::convert_error(std::move(result));
  }

  /**
   * @brief Specialization for arrays (std::vector, std::set,
   * std::unordered_set)
   */
  template <typename T, typename Tokens>
  static inline std::enable_if_t<detail::is_array_like_v<T>, ParseResult<T>>
  parse(Tokens &tokens) {
    using ValueType = typename T::value_type;

    T result = {};

    const auto maybeToken = tokens.next();
    if (!maybeToken) {
      if (tokens.has_error()) {
        return ParseResult<T>::json_error(tokens.get_error(),
                                          tokens.get_path());
      } else {
        // TODO: Provide better error
        return ParseResult<T>::parse_error(unexpected_end_error(),
                                           tokens.get_path());
      }
    }

    const auto &token = maybeToken.value();
    if (!token.template is_of_type<detail::Token::Type::StartArray>()) {
      // TODO: Provide better error
      return ParseResult<T>::parse_error(
          unexpected_token_error<detail::Token::Type::StartArray>(token),
          tokens.get_path());
    }

    while (true) {
      const auto &maybeToken = tokens.peek();
      if (!maybeToken) {
        if (tokens.has_error()) {
          return ParseResult<T>::json_error(tokens.get_error(),
                                            tokens.get_path());
        } else {
          // TODO: Provide better error
          return ParseResult<T>::parse_error(unexpected_end_error(),
                                             tokens.get_path());
        }
      }

      const auto &token = maybeToken.value();
      if (token.template is_of_type<detail::Token::Type::EndArray>()) {
        tokens.next();

        return ParseResult<T>::result(std::move(result));
      }

      auto member_result = parse<ValueType>(tokens);
      if (!member_result.is_ok()) {
        return ParseResult<T>::convert_error(std::move(member_result));
      }

      detail::is_array_like<T>::emplace(result,
                                        std::move(member_result).value());
    }
  }

  /**
   * @brief Specialization for dicts (std::map, std::unordered_map)
   */
  template <typename T, typename Tokens>
  static inline std::enable_if_t<detail::is_dict_like_v<T>, ParseResult<T>>
  parse(Tokens &tokens) {
    using ValueType = typename T::mapped_type;

    T result = {};

    const auto maybeToken = tokens.next();
    if (!maybeToken) {
      if (tokens.has_error()) {
        return ParseResult<T>::json_error(tokens.get_error(),
                                          tokens.get_path());
      } else {
        // TODO: Provide better error
        return ParseResult<T>::parse_error(unexpected_end_error(),
                                           tokens.get_path());
      }
    }

    const auto &token = maybeToken.value();
    if (!token.template is_of_type<detail::Token::Type::StartObject>()) {
      // TODO: Provide better error
      return ParseResult<T>::parse_error(
          unexpected_token_error<detail::Token::Type::StartObject>(token),
          tokens.get_path());
    }

    while (true) {
      const auto &maybeToken = tokens.next();
      if (!maybeToken) {
        if (tokens.has_error()) {
          return ParseResult<T>::json_error(tokens.get_error(),
                                            tokens.get_path());
        } else {
          // TODO: Provide better error
          return ParseResult<T>::parse_error(unexpected_end_error(),
                                             tokens.get_path());
        }
      }

      const auto &token = maybeToken.value();
      if (token.template is_of_type<detail::Token::Type::EndObject>()) {
        return ParseResult<T>::result(std::move(result));
      }

      if (!token.template is_of_type<detail::Token::Type::Key>()) {
        // TODO: Provide better error
        return ParseResult<T>::parse_error(
            unexpected_token_error<detail::Token::Type::Key,
                                   detail::Token::Type::EndObject>(token),
            tokens.get_path());
      }

      auto key = std::move(token.template value<detail::Token::Type::Key>());

      auto member_result = parse<ValueType>(tokens);
      if (!member_result.is_ok()) {
        return ParseResult<T>::convert_error(std::move(member_result));
      }

      detail::is_dict_like<T>::emplace(result, std::move(key),
                                       std::move(member_result).value());
    }
  }

  /**
   * @brief Specialization for classes that implement json_parse
   */
  template <typename T, typename Tokens>
  static inline std::enable_if_t<
      detail::has_parse_v<T, ParseResult<T>, Tokens &>, ParseResult<T>>
  parse(Tokens &tokens) {
    return T::json_parse(tokens);
  }

  /**
   * @brief Specialization for classes which has Deserializable instantiated
   */
  template <typename T, typename Tokens>
  static inline std::enable_if_t<Deserializable<T, Tokens>::value,
                                 ParseResult<T>>
  parse(Tokens &tokens) {
    return Deserializable<T, Tokens>::parse(tokens);
  }

  /**
   * @tparam t_types types of expected tokens
   * @param token unexpected token
   * @return error message for unexpected token
   */
  template <detail::Token::Type... t_types>
  static inline std::string unexpected_token_error(const detail::Token &token) {
    return "Expected " + ((detail::Token::name<t_types>() + ",") + ...) +
           " got " + token.name();
  }

  /**
   * @return error message for unexpected end of json
   */
  static inline std::string unexpected_end_error() {
    return "Unexpected end of json";
  }

private:
  template <typename T>
  static inline ParseResult<T>
  parse_value(detail::Token &token,
              std::optional<std::string> path = std::nullopt) {
    return parse_value_impl<T>(token, std::move(path),
                               std::in_place_type<detail::ValueTokens>);
  }

  /**
   * @brief Implementation of parsing json value from token
   *
   * @param token token to parse from
   * @param path optional path in json of token (for error message)
   */
  template <typename T, typename Head, typename... Tail>
  static inline ParseResult<T>
  parse_value_impl(detail::Token &token, std::optional<std::string> path,
                   std::in_place_type_t<detail::TokenList<Head, Tail...>>) {
    using ValueType = typename Head::value_type;
    constexpr auto t_type = Head::type;

    if (!token.is_of_type<t_type>()) {
      if constexpr (sizeof...(Tail) > 0) {
        return parse_value_impl<T, Tail...>(
            token, std::move(path),
            std::in_place_type<detail::TokenList<Tail...>>);
      } else {
        // TODO: Provide better error
        return ParseResult<T>::parse_error("Unexpected " + token.name(),
                                           std::move(path));
      }
    }

    constexpr bool t_is_bool = std::is_same_v<T, bool>;
    constexpr bool v_is_bool = std::is_same_v<ValueType, bool>;
    constexpr bool t_is_integer =
        !t_is_bool && std::numeric_limits<T>::is_integer;
    constexpr bool v_is_integer =
        !v_is_bool && std::numeric_limits<ValueType>::is_integer;
    constexpr bool t_is_floating = std::is_floating_point_v<T>;
    constexpr bool v_is_floating = std::is_floating_point_v<ValueType>;

    constexpr bool is_bool = t_is_bool && v_is_bool;
    constexpr bool is_integer = t_is_integer && v_is_integer;
    constexpr bool is_floating =
        (v_is_integer || v_is_floating) && t_is_floating;
    constexpr bool is_string = std::is_same_v<ValueType, std::string> &&
                               std::is_same_v<T, std::string>;

    auto &value = token.value<t_type>();
    if constexpr (is_bool) {
      return ParseResult<T>::result(value);
    } else if constexpr (is_integer) {
      if (detail::in_range<T>(value)) {
        return ParseResult<T>::result(static_cast<T>(value));
      } else {
        // TODO: Provide better error
        return ParseResult<T>::parse_error("Integer value not in range",
                                           std::move(path));
      }
    } else if constexpr (is_floating) {
      return ParseResult<T>::result(static_cast<T>(value));
    } else if constexpr (is_string) {
      return ParseResult<T>::result(std::move(value));
    } else {
      // TODO: Provide better error
      return ParseResult<T>::parse_error("Unexpected " + token.name(),
                                         std::move(path));
    }
  }
};
} // namespace ctjson