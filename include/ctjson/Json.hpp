#pragma once

#include <rapidjson/stringbuffer.h>

#include <ctjson/Deserializer.hpp>
#include <ctjson/Serializer.hpp>
#include <ctjson/SimpleWriter.hpp>
#include <ctjson/TokenStream.hpp>

namespace ctjson {

/**
 * @brief Convinient function to parse json from string
 * @tparam T type of value to parse
 * @param json json string
 * @return parse result
 */
template <typename T> inline ParseResult<T> parse(const std::string &json) {
    rapidjson::StringStream ss(json.c_str());
    ContextTokenStream<rapidjson::StringStream> tokens(std::move(ss));

    return Deserializer::parse<T>(tokens);
}

/**
 * @brief Convinient function to dump value to json string
 * @tparam T type of value
 * @param value value to dump
 * @return json string
 */
template <typename T> inline std::string dump(const T &value) {
    rapidjson::StringBuffer sb;
    SimpleWriter<rapidjson::StringBuffer> writer(sb);

    Serializer::dump(value, writer);

    return sb.GetString();
}
} // namespace ctjson