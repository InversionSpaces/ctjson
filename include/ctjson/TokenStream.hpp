#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <type_traits>

#include <rapidjson/error/en.h>
#include <rapidjson/reader.h>

#include <ctjson/detail/Path.hpp>
#include <ctjson/detail/Token.hpp>

namespace ctjson {

namespace detail {

/**
 * @brief Class implementing Handler concept from rapidjson to retrieve tokens
 */
class TokenHandler {
public:
  bool Null() { return dispatch<Token::Type::Null>(); }
  bool Bool(bool boolean) { return dispatch<Token::Type::Bool>(boolean); }
  bool Int(int integer) { return dispatch<Token::Type::Int>(integer); }
  bool Uint(unsigned integer) { return dispatch<Token::Type::Uint>(integer); }
  bool Int64(int64_t integer) { return dispatch<Token::Type::Int64>(integer); }
  bool Uint64(uint64_t integer) {
    return dispatch<Token::Type::Uint64>(integer);
  }
  bool Double(double number) { return dispatch<Token::Type::Double>(number); }
  bool RawNumber(const char *str, unsigned len, bool copy) {
    return dispatch<Token::Type::RawNumber>(std::string(str, len));
  }
  bool String(const char *str, unsigned len, bool copy) {
    return dispatch<Token::Type::String>(std::string(str, len));
  }
  bool StartObject() { return dispatch<Token::Type::StartObject>(); }
  bool Key(const char *str, unsigned len, bool copy) {
    return dispatch<Token::Type::Key>(std::string(str, len));
  }
  bool EndObject(unsigned size) {
    return dispatch<Token::Type::EndObject>(size);
  }
  bool StartArray() { return dispatch<Token::Type::StartArray>(); }
  bool EndArray(unsigned size) { return dispatch<Token::Type::EndArray>(size); }

  /**
   * @return const reference to current token
   */
  const std::optional<Token> &peek() const { return m_token; }

  /**
   * @return current token
   * @pre has_token() == true
   * @post has_token() == false
   */
  Token token() {
    Token result = std::move(m_token.value());
    m_token.reset();

    return result;
  }

  /**
   * @return true if this handler has token
   */
  bool has_token() const { return m_token.has_value(); }

private:
  /**
   * @brief emplace current token
   *
   * @tparam t_type type of token to emplace
   * @tparam Args types of arguments to Token::create
   * @param args arguments to Token::create
   * @return true
   */
  template <Token::Type t_type, typename... Args>
  bool dispatch(Args &&...args) {
    m_token = Token::create<t_type>(args...);
    return true;
  }

private:
  std::optional<Token> m_token = std::nullopt;
};
} // namespace detail

/**
 * @brief Class converting input stream to token stream
 *
 * @tparam InputStream type of input stream
 * @tparam Derived derived class for crtp, @see advance
 */
template <typename InputStream, typename Derived = void>
class TokenStream {

public:
  /**
   * @param is input stream
   */
  TokenStream(InputStream is) : m_is(std::move(is)) {
    m_reader.IterativeParseInit();
  }

  /**
   * @return true if stream encountered error
   */
  bool has_error() const { return m_error.has_value(); }

  /**
   * @return error
   * @pre has_error() == true
   */
  std::string get_error() const { return m_error.value(); }

  /**
   * @return true if parsing is complete
   */
  bool is_complete() const {
    return m_reader.IterativeParseComplete() &&
           !m_handler.has_token(); // We could have one more token after peek
  }

  /**
   * @brief Peek next token
   * @return   std::nullopt if there is no next token,
   *           reference to next token otherwise
   */
  const std::optional<detail::Token> &peek() {
    acquire_token();

    return m_handler.peek();
  }

  /**
   * @brief Retrieve next token
   * @return   std::nullopt if there is no next token,
   *           next token otherwise
   */
  std::optional<detail::Token> next() {
    if (acquire_token()) {
      return m_handler.token();
    } else {
      return std::nullopt;
    }
  }

  /**
   * @brief Get current path in json
   *
   * This class does not maintain path
   * @return std::nullopt
   */
  std::optional<std::string> get_path() const { return std::nullopt; }

protected:
  /**
   * @brief Make sure that handler has next token or end reached or error
   * encountered;
   */
  bool acquire_token() {
    if (has_error() || is_complete()) {
      return false;
    }

    if (!m_handler.has_token()) {
      advance();
    }

    return m_handler.has_token();
  }

  /**
   * @brief Advance one token further
   */
  void advance() {
    const auto result = m_reader.IterativeParseNext<flags>(m_is, m_handler);
    if (!result) {
      handle_parse_error(m_reader.GetParseErrorCode());
    } else if (!m_handler.has_token()) {
      m_error = "Unexpected state: no token acquired, possibly a bug";
    } else if constexpr (!std::is_same_v<Derived, void>) {
      static_cast<Derived *>(this)->on_advance(m_handler.peek().value());
    }
  }

  /**
   * @brief Set error based on parse error code
   */
  void handle_parse_error(rapidjson::ParseErrorCode code) {
    std::string error = rapidjson::GetParseError_En(code);
    if (code == rapidjson::ParseErrorCode::kParseErrorTermination ||
        code == rapidjson::ParseErrorCode::kParseErrorNone) {
      error = "Unexpected error: " + error + ", possibly a bug";
    }
    m_error = std::move(error);
  }

private:
  // Parsing flags
  constexpr static unsigned flags =
      rapidjson::ParseFlag::kParseIterativeFlag |
      rapidjson::ParseFlag::kParseTrailingCommasFlag;

  InputStream m_is;
  rapidjson::Reader m_reader;
  detail::TokenHandler m_handler;

  std::optional<std::string> m_error;
};

/**
 * @brief Class to convert input stream to token stream maintaining path in json
 *
 * @tparam InputStream type of input stream
 */
template <typename InputStream>
class ContextTokenStream
    : public TokenStream<InputStream, ContextTokenStream<InputStream>> {
  using Base = TokenStream<InputStream, ContextTokenStream<InputStream>>;
  friend Base;

public:
  /**
   * @param is input stream
   */
  ContextTokenStream(InputStream is) : Base(std::move(is)) {}

  /**
   * @brief Get current path in json
   *
   * @return always current path in json
   */
  std::optional<std::string> get_path() const { return m_path.render(); }

private:
  /**
   * @brief Update path on new token
   */
  void on_advance(const detail::Token &token) {
    if (token.is_of_type<detail::Token::Type::StartObject>()) {
      m_path.start_object();
    } else if (token.is_of_type<detail::Token::Type::Key>()) {
      m_path.key(token.value<detail::Token::Type::Key>());
    } else if (token.is_of_type<detail::Token::Type::EndObject>()) {
      m_path.end_object();
    } else if (token.is_of_type<detail::Token::Type::StartArray>()) {
      m_path.start_array();
    } else if (token.is_of_type<detail::Token::Type::EndArray>()) {
      m_path.end_array();
    } else {
      m_path.value();
    }
  }

private:
  detail::Path m_path;
};
} // namespace ctjson