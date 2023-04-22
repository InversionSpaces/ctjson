#pragma once

#include <string>

template <typename C, typename Mapper>
std::string join(const C &container, Mapper mapper, const char separator = 0) {
    std::string result;
    for (const auto &item : container) {
        if (!result.empty() && separator != 0) {
            result += separator;
        }
        result += mapper(item);
    }

    return result;
}