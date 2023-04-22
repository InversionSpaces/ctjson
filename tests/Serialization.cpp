#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <algorithm>
#include <cmath>
#include <sstream>
#include <string>
#include <type_traits>

#include <rapidjson/stringbuffer.h>

#include <ctjson/Json.hpp>
#include <ctjson/Serializable.hpp>
#include <ctjson/SerializationHelper.hpp>

#include "Utils.hpp"

using namespace ctjson;
using namespace Catch::Matchers;

TEST_CASE("Bool is serialized", "[Serialization]") {
    const auto test = [](const bool val) {
        const std::string json = val ? "true" : "false";
        auto result = dump<bool>(val);

        REQUIRE(result == json);
    };

    test(true);
    test(false);
}

template <typename Number> static inline void test_number() {
    const auto test = [](const Number val) {
        // To avoid reading chars
        using ReadType =
            std::conditional_t<sizeof(Number) < sizeof(int), int, Number>;

        const auto json = dump<Number>(val);

        std::stringstream ss;
        ss << json;
        ReadType result = 0;
        ss >> result;

        if constexpr (std::is_floating_point_v<Number>) {
            REQUIRE_THAT(result, WithinRel(val) || WithinAbs(val, 1e-6));
        } else {
            REQUIRE(result == val);
        }
    };

    test(0);
    test(std::numeric_limits<Number>::max());
    test(std::numeric_limits<Number>::min());
    test(std::numeric_limits<Number>::max() / 2);
    test(std::numeric_limits<Number>::min() / 2);
    if constexpr (std::is_floating_point_v<Number>) {
        test(1e-6);
        test(-1e-6);
    }
}

TEST_CASE("Numbers are serialized", "[Serialization]") {
    test_number<std::int8_t>();
    test_number<std::int16_t>();
    test_number<std::int32_t>();
    test_number<std::int64_t>();
    test_number<std::uint8_t>();
    test_number<std::uint16_t>();
    test_number<std::uint32_t>();
    test_number<std::uint64_t>();
    test_number<std::float_t>();
    test_number<std::double_t>();
}

TEST_CASE("String is serialized", "[Serialization]") {
    const std::string val = "example";

    auto result = dump<std::string>(val);

    REQUIRE(result == "\"" + val + "\"");
}

TEST_CASE("Optional is serialized", "[Serialization]") {
    {
        const std::optional<std::string> val = "example";

        auto json = dump<std::optional<std::string>>(val);

        REQUIRE(json == "\"" + val.value() + "\"");
    }
    {
        const std::optional<std::string> val = std::nullopt;

        auto json = dump<std::optional<std::string>>(val);

        REQUIRE(json == "null");
    }
}

template <class Container> void test_array(const size_t size) {
    Container arr;
    for (size_t i = 0; i < size; ++i) {
        if constexpr (std::is_same_v<Container, std::vector<int>>) {
            arr.emplace_back(i);
        } else {
            arr.emplace(i);
        }
    }

    const auto elems = join(
        arr, [](const auto i) { return std::to_string(i); }, ',');

    const auto compare = [&arr](const std::string json) {
        auto result = dump(arr);

        auto nend = std::remove(result.begin(), result.end(), ' ');
        result.erase(nend, result.end());

        REQUIRE(result == json);
    };

    compare("[" + elems + "]");
}

TEST_CASE("Arrays are serialized", "[Serialization]") {
    test_array<std::vector<int>>(0);
    test_array<std::vector<int>>(1);
    test_array<std::vector<int>>(2);
    test_array<std::vector<int>>(42);
    test_array<std::set<int>>(0);
    test_array<std::set<int>>(1);
    test_array<std::set<int>>(2);
    test_array<std::set<int>>(42);
    test_array<std::unordered_set<int>>(0);
    test_array<std::unordered_set<int>>(1);
    test_array<std::unordered_set<int>>(2);
    test_array<std::unordered_set<int>>(42);
}

template <class Container> void test_dict(const size_t size) {
    Container dict;
    for (size_t i = 0; i < size; ++i) {
        std::string key = i % 2 == 0 ? "even" : "odd";
        key += std::to_string(i);
        dict.emplace(std::move(key), i);
    }

    const auto elems = join(
        dict,
        [](const auto e) {
            const auto &[key, value] = e;
            return "\"" + key + "\":" + std::to_string(value);
        },
        ',');

    const auto compare = [&dict](const std::string json) {
        auto result = dump<Container>(dict);

        auto nend = std::remove(result.begin(), result.end(), ' ');
        result.erase(nend, result.end());

        REQUIRE(result == json);
    };

    compare("{" + elems + "}");
}

TEST_CASE("Dicts are serialized", "[Serialization]") {
    test_dict<std::map<std::string, int>>(0);
    test_dict<std::map<std::string, int>>(1);
    test_dict<std::map<std::string, int>>(2);
    test_dict<std::map<std::string, int>>(42);
    test_dict<std::unordered_map<std::string, int>>(0);
    test_dict<std::unordered_map<std::string, int>>(1);
    test_dict<std::unordered_map<std::string, int>>(2);
    test_dict<std::unordered_map<std::string, int>>(42);
}

struct DumpClass {
    std::string str;
    int integer;

    template <typename Writer>
    static void json_dump(const DumpClass &value, Writer &writer) {
        auto str = SerializationHelper::Field("str", value.str);
        auto integer = SerializationHelper::Field("integer", value.integer);
        SerializationHelper::dump(writer, str, integer);
    }
};

TEST_CASE("Class with `dump` method is serialized", "[Serialization]") {
    DumpClass val = {.str = "example", .integer = 42};

    auto result = dump<DumpClass>(val);

    auto nend = std::remove(result.begin(), result.end(), ' ');
    result.erase(nend, result.end());

    REQUIRE(result == "{\"str\":\"example\",\"integer\":42}");
}

struct SerialazableClass {
    bool boolean;
    std::optional<int> oint;
};

template <typename Writer>
struct Serializable<SerialazableClass, Writer> : public std::true_type {
    static void dump(const SerialazableClass &value, Writer &writer) {
        auto boolean = SerializationHelper::Field("boolean", value.boolean);
        auto oint = SerializationHelper::Field("oint", value.oint);
        SerializationHelper::dump(writer, boolean, oint);
    }
};

TEST_CASE("Class with `Serializable` instance is serialized",
          "[Serialization]") {
    SerialazableClass val = {.boolean = false, .oint = 42};

    auto result = dump(val);

    auto nend = std::remove(result.begin(), result.end(), ' ');
    result.erase(nend, result.end());

    REQUIRE(result == "{\"boolean\":false,\"oint\":42}");
}

struct InnerClass {
    std::string str;
    std::optional<int> oint;

    template <typename Writer>
    static void json_dump(const InnerClass &value, Writer &writer) {
        auto str = SerializationHelper::Field("str", value.str);
        auto oint = SerializationHelper::Field("oint", value.oint);
        SerializationHelper::dump(writer, str, oint);
    }
};

struct OuterClass {
    bool boolean;
    std::string str;
    std::vector<InnerClass> inners;

    template <typename Writer>
    static void json_dump(const OuterClass &value, Writer &writer) {
        auto boolean = SerializationHelper::Field("boolean", value.boolean);
        auto str = SerializationHelper::Field("str", value.str);
        auto inners = SerializationHelper::Field("inners", value.inners);
        SerializationHelper::dump(writer, boolean, str, inners);
    }
};

TEST_CASE("Nested classes are serialized", "[Serialization]") {
    const auto val =
        OuterClass{.boolean = false,
                   .str = "example",
                   .inners = {InnerClass{.str = "one", .oint = 1},
                              InnerClass{.str = "none", .oint = std::nullopt}}};

    const auto inners_json =
        "[" +
        join(
            val.inners,
            [](const auto &i) {
                return std::string("{\"str\":\"") + i.str + "\",\"oint\":" +
                       (i.oint ? std::to_string(i.oint.value()) : "null") + "}";
            },
            ',') +
        "]";
    const auto json =
        std::string("{\"boolean\":false,\"str\":\"example\",\"inners\":") +
        inners_json + "}";

    auto result = dump<OuterClass>(val);

    auto nend = std::remove(result.begin(), result.end(), ' ');
    result.erase(nend, result.end());

    REQUIRE(result == json);
}