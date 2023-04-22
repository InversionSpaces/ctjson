#pragma once

#include <type_traits>

namespace ctjson {

/**
 * @brief Trait to allow custom serialization for type T
 *
 * To enable custom serialization, instantiate this class like so:
 * @code{.cpp}
 * template<typename Writer>
 * struct Serializable<MyType, Writer> : public std::true_type { // Note
 * std::true_type static void dump(const MyType& value, Writer &tokens) { ... }
 * };
 * @endcode
 *
 */
template <typename T, typename Writer>
struct Serializable : public std::false_type {};

} // namespace ctjson