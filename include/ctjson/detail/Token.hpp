#pragma once

#include <cstdint>
#include <string>
#include <type_traits>
#include <variant>

namespace ctjson::detail {

/**
 * @brief Types of token, taken from rapidjson
 */
enum class TokenType {
  Null,
  Bool,
  Int,
  Uint,
  Int64,
  Uint64,
  Double,
  RawNumber,
  String,
  StartObject,
  Key,
  EndObject,
  StartArray,
  EndArray,
};

/**
 * @brief Template base to optionally add value to Token
 *
 * @tparam T type of value or void if there is none
 */
template <typename T>
struct TokenValue {
  using value_type = T;

  T value;
};

template <>
struct TokenValue<void> {};

/**
 * @brief Class representing Token
 *
 * @tparam t_type type of token
 * @tparam T type of value token holds or void
 */
template <TokenType t_type, typename T = void>
struct ExactToken : public TokenValue<T> {
  constexpr static TokenType type = t_type;

  template <typename... Args>
  static ExactToken create(Args &&...args) noexcept {
    return {std::forward<Args>(args)...};
  }
};

// Tokens
using NullToken = ExactToken<TokenType::Null>;
using BoolToken = ExactToken<TokenType::Bool, bool>;
using IntToken = ExactToken<TokenType::Int, int>;
using UintToken = ExactToken<TokenType::Uint, unsigned>;
using Int64Token = ExactToken<TokenType::Int64, int64_t>;
using Uint64Token = ExactToken<TokenType::Uint64, uint64_t>;
using DoubleToken = ExactToken<TokenType::Double, double>;
using NumberToken = ExactToken<TokenType::RawNumber, std::string>;
using StringToken = ExactToken<TokenType::String, std::string>;
using StartObjectToken = ExactToken<TokenType::StartObject>;
using KeyToken = ExactToken<TokenType::Key, std::string>;
using EndObjectToken = ExactToken<TokenType::EndObject, unsigned>;
using StartArrayToken = ExactToken<TokenType::StartArray>;
using EndArrayToken = ExactToken<TokenType::EndArray, unsigned>;

/**
 * @brief Template magic to find token of given type
 *
 * @tparam t_type type of token to find
 * @tparam Args token types to search
 * @ref token_of_type<t_type, Tokens...>::type contains result or void
 */
template <TokenType t_type, typename... Args>
struct token_of_type;

template <TokenType t_type, typename Head, typename... Tail>
struct token_of_type<t_type, Head, Tail...> {
  using type =
      std::conditional_t<Head::type == t_type, Head,
                         typename token_of_type<t_type, Tail...>::type>;
};

template <TokenType t_type>
struct token_of_type<t_type> {
  using type = void;
};

/**
 * @brief Template magic utility to manipulate token types
 */
template <typename... Tokens>
struct TokenList {
  // Alias for variant
  using Variant = std::variant<Tokens...>;

  // Alias for token_of_type
  template <TokenType t_type>
  using of_type = typename token_of_type<t_type, Tokens...>::type;

  /**
   * @brief Template utility to concatenate TokenLists
   */
  template <typename T>
  struct cat;

  template <typename... Others>
  struct cat<TokenList<Others...>> {
    using type = TokenList<Tokens..., Others...>;
  };
};

// Alias for concatenation
template <typename L, typename R>
using cat = typename L::template cat<R>::type;

// Meta tokens
using MetaTokens = TokenList<NullToken, StartObjectToken, KeyToken,
                             EndObjectToken, StartArrayToken, EndArrayToken>;

// Value tokens
using ValueTokens =
    TokenList<BoolToken, IntToken, UintToken, Int64Token, Uint64Token,
              DoubleToken, NumberToken, StringToken>;

// All tokens
using Tokens = cat<MetaTokens, ValueTokens>;

/**
 * @brief Class representing polymorphic token
 */
class Token {
private:
  using TokenVariant = Tokens::Variant;

public:
  using Type = TokenType;

private:
  /**
   * @tparam T exact token type
   * @tparam Args types of arguments for variant constructor
   * @param args arguments for variant constructor
   */
  template <typename T, typename... Args>
  Token(std::in_place_type_t<T> tag, Args &&...args)
      : m_token(tag, std::forward<Args>(args)...) {}

public:
  /**
   * @tparam t_type token type
   * @tparam Args types of arguments for token constructor
   * @param args arguments for token constructor
   */
  template <Type t_type, typename... Args>
  static Token create(Args &&...args) noexcept {
    using ExactToken = Tokens::of_type<t_type>;
    return Token(std::in_place_type<ExactToken>,
                 ExactToken::create(std::forward<Args>(args)...));
  }

  /**
   * @tparam t_type type of token
   * @return true if this contains token of type @ref t_type
   */
  template <Type t_type>
  bool is_of_type() const {
    using ExactToken = Tokens::of_type<t_type>;
    return std::holds_alternative<ExactToken>(m_token);
  }

  /**
   * @tparam t_type type of token
   * @return value of token of type @ref t_type
   * @pre is_of_type<t_type> == true
   */
  template <Type t_type>
  auto &value() {
    using ExactToken = Tokens::of_type<t_type>;
    return std::get<ExactToken>(m_token).value;
  }

  /**
   * @tparam t_type type of token
   * @return value of token of type @ref t_type
   * @pre is_of_type<t_type> == true
   */
  template <Type t_type>
  const auto &value() const {
    using ExactToken = Tokens::of_type<t_type>;
    return std::get<ExactToken>(m_token).value;
  }

  /**
   * @tparam t_type type of token
   * @return name of token of type @ref t_type
   */
  template <Type t_type>
  static std::string name() noexcept {
    if constexpr (t_type == Type::Null) {
      return "null";
    } else if constexpr (t_type == Type::Bool) {
      return "bool";
    } else if constexpr (t_type == Type::Int) {
      return "int";
    } else if constexpr (t_type == Type::Int64) {
      return "int64";
    } else if constexpr (t_type == Type::Uint) {
      return "uint";
    } else if constexpr (t_type == Type::Uint64) {
      return "uint64";
    } else if constexpr (t_type == Type::Double) {
      return "double";
    } else if constexpr (t_type == Type::RawNumber) {
      return "number";
    } else if constexpr (t_type == Type::String) {
      return "string";
    } else if constexpr (t_type == Type::StartObject) {
      return "start object";
    } else if constexpr (t_type == Type::Key) {
      return "key";
    } else if constexpr (t_type == Type::EndObject) {
      return "end object";
    } else if constexpr (t_type == Type::StartArray) {
      return "start array";
    } else if constexpr (t_type == Type::EndArray) {
      return "end array";
    } else {
      static_assert(static_cast<int>(t_type) == -1, "Unknown token type");
    }
  }

  /**
   * @return name of token this contains
   */
  std::string name() const {
    return std::visit(
        [&](const auto &arg) noexcept {
          return Token::name<std::decay_t<decltype(arg)>::type>();
        },
        m_token);
  }

private:
  TokenVariant m_token;
};
} // namespace ctjson::detail