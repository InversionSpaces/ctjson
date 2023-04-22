#pragma once

#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>

#include <ctjson/Deserializer.hpp>
#include <ctjson/ParseResult.hpp>

#include <ctjson/detail/Field.hpp>
#include <ctjson/detail/Token.hpp>
#include <ctjson/detail/TypeUtils.hpp>
#include <ctjson/detail/Typing.hpp>

namespace ctjson {

/**
 * @brief Class to help implement deserialization from json for custom types
 *
 * Usage example:
 * @code{.cpp}
 * struct ParseClass {
 *   std::string str;
 *   int integer;
 *
 *   template<typename Tokens>
 *   static ParseResult<ParseClass> json_parse(Tokens &tokens) {
 *       ParseClass object;
 *       auto str = DeserializationHelper::Field("str", object.str); //
 * reference is saved here auto integer =
 * DeserializationHelper::Field("integer", object.integer); // and here
 *       auto result = DeserializationHelper::parse_object(tokens, str,
 * integer); if (result.is_ok()) { return
 * ParseResult<ParseClass>::result(std::move(object)); } else { return
 * ParseResult<ParseClass>::convert_error(std::move(result));
 *       }
 *   }
 * };
 * @endcode
 */
class DeserializationHelper {
  public:
    /**
     * @brief Class representing class field
     *
     * @tparam T type of field
     */
    template <typename T> class Field : public detail::FieldBase<T> {
        using Base = detail::FieldBase<T>;

      public:
        /**
         * @param name name of filed - key in json
         * @param ref reference to store parse result
         */
        Field(std::string name, T &ref)
            : Base(std::move(name), ref), m_set(false) {}

        /**
         * @return true if field is ready (set or optional)
         */
        bool is_ready() const { return m_set || Base::is_opt; }

        /**
         * @return true if field is set (already parsed)
         */
        bool is_set() const { return m_set; }

        /**
         * @param value parse result to set
         */
        void set(T value) {
            // TODO: Check that set == false
            Base::m_ref = std::move(value);
            m_set = true;
        }

      private:
        bool m_set;
    };

    /**
     * @brief Parse class with given @param fields
     *
     * @tparam Tokens type of token stream
     * @tparam Args types of fields
     * @param tokens token stream
     * @param fields references to fields
     * @return empty result if parsing was successful, error result otherwise
     */
    template <typename Tokens, typename... Args>
    static inline ParseResult<void> parse_object(Tokens &tokens,
                                                 Field<Args> &...fields) {
        const auto maybeToken = tokens.next();
        if (!maybeToken) {
            if (tokens.has_error()) {
                return ParseResult<void>::json_error(tokens.get_error(),
                                                     tokens.get_path());
            } else {
                // TODO: Provide better error
                return ParseResult<void>::parse_error(
                    Deserializer::unexpected_end_error(), tokens.get_path());
            }
        }

        const auto &token = maybeToken.value();
        if (!token.template is_of_type<detail::Token::Type::StartObject>()) {
            // TODO: Provide better error
            return ParseResult<void>::parse_error(
                Deserializer::unexpected_token_error<
                    detail::Token::Type::StartObject>(token),
                tokens.get_path());
        }

        const auto map = build_fields_map(fields...);

        while (true) {
            const auto &maybeToken = tokens.next();
            if (!maybeToken) {
                if (tokens.has_error()) {
                    return ParseResult<void>::json_error(tokens.get_error(),
                                                         tokens.get_path());
                } else {
                    // TODO: Provide better error
                    return ParseResult<void>::parse_error(
                        Deserializer::unexpected_end_error(),
                        tokens.get_path());
                }
            }

            const auto &token = maybeToken.value();
            if (token.template is_of_type<detail::Token::Type::EndObject>()) {
                // All field are set or optional
                if ((fields.is_ready() && ... && true)) {
                    return ParseResult<void>::result();
                }

                /**
                 * In case `Args = {}`, condition above would always be true
                 * We need this if just to not compile `missing_keys_error`
                 */
                if constexpr (sizeof...(Args) > 0) {
                    // TODO: Provide better error
                    return ParseResult<void>::parse_error(
                        missing_keys_error(fields...), tokens.get_path());
                }
            }

            if (!token.template is_of_type<detail::Token::Type::Key>()) {
                // TODO: Provide better error
                return ParseResult<void>::parse_error(
                    Deserializer::unexpected_token_error<
                        detail::Token::Type::Key,
                        detail::Token::Type::EndObject>(token),
                    tokens.get_path());
            }

            const auto key =
                std::move(token.template value<detail::Token::Type::Key>());
            if (map.count(key) == 0) {
                return ParseResult<void>::parse_error("Unexpected key: " + key,
                                                      tokens.get_path());
            }

            const auto index = map.at(key);
            auto field_result = parse_field(tokens, key, index, fields...);
            if (!field_result.is_ok()) {
                return field_result;
            }
        }
    }

    /**
     * @brief Parse class from value of type @tparam T
     *
     * @tparam T type of value to parse from
     * @tparam Tokens type of token stream
     * @tparam F type of conversion function (T -> your class)
     * @param tokens token stream
     * @param f conversion function
     * @return parsing result
     *
     * @note @param f could return just class if conversion is total,
     * it could also return @ref ParseResult.
     */
    template <typename T, typename Tokens, typename F>
    static inline auto parse_from(Tokens &tokens, F f) {
        using ReturnT = decltype(f(std::declval<T>()));
        constexpr bool is_parse_result = detail::is_parse_result_v<ReturnT>;
        using ResultT =
            std::conditional_t<is_parse_result, ReturnT, ParseResult<ReturnT>>;

        auto result = Deserializer::parse<T>(tokens);
        if (!result.is_ok()) {
            return ResultT::convert_error(std::move(result));
        } else if constexpr (is_parse_result) {
            return f(std::move(result).value());
        } else {
            return ResultT::result(f(std::move(result).value()));
        }
    }

  private:
    /**
     * @tparam Args types of fields
     * @param fields references to fields
     * @return map (field name -> field 0-based index in fields)
     */
    template <typename... Args>
    static std::unordered_map<std::string, size_t>
    build_fields_map(const Field<Args> &...fields) {
        std::unordered_map<std::string, size_t> result;
        size_t counter = 0;

        ([&]() { result[fields.name] = counter++; }(), ...);

        return result;
    }

    /**
     * @tparam Tokens token stream type
     * @tparam Args fields types
     * @param tokens token stream
     * @param key json key for this field
     * @param index 0-based index of field in fields
     * @param fields references to fields
     * @return successful result without value or error
     * @post if result is not error, parsed field will be set
     */
    template <typename Tokens, typename... Args>
    static ParseResult<void>
    parse_field(Tokens &tokens,
                const std::string key, // Just for error message
                const size_t index, Field<Args> &...fields) {
        std::optional<ParseResult<void>> result = std::nullopt;

        detail::call_on_nth(
            index,
            [&](auto &field) {
                using FieldType = typename std::decay_t<decltype(field)>::type;

                if (field.is_set()) {
                    result.emplace(ParseResult<void>::parse_error(
                        "Duplicate key: " + key, tokens.get_path()));
                } else {
                    auto field_result = Deserializer::parse<FieldType>(tokens);
                    if (!field_result.is_ok()) {
                        result.emplace(ParseResult<void>::convert_error(
                            std::move(field_result)));
                    } else {
                        field.set(std::move(field_result).value());
                        result.emplace(ParseResult<void>::result());
                    }
                }
            },
            fields...);

        return result.value();
    }

    /**
     * @return missing keys error message
     */
    template <typename... Args>
    static std::string missing_keys_error(const Field<Args> &...fields) {
        return (("Missing keys: " + ... +
                 (fields.is_ready() ? "" : (fields.name + ", "))) +
                "got " + detail::Token::name<detail::Token::Type::EndObject>());
    }
};
} // namespace ctjson