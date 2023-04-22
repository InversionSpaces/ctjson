#pragma once

#include <type_traits>

namespace ctjson {

/**
 * @brief Trait to allow custom deserialization for type T
 *
 * To enable custom deserialization, instantiate this class like so:
 * @code{.cpp}
 * template<typename Tokens>
 * struct Deserializable<MyType, Tokens> : public std::true_type { // Note
 * std::true_type static ParseResult<MyType> parse(Tokens &tokens) { ... }
 * };
 * @endcode
 *
 */
template <typename T, typename Tokens>
struct Deserializable : public std::false_type {};

} // namespace ctjson