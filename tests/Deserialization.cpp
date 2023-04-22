#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <cmath>
#include <cstdint>
#include <limits>
#include <type_traits>

#include <rapidjson/error/en.h>
#include <rapidjson/rapidjson.h>
#include <rapidjson/reader.h>

#include <ctjson/DeserializationHelper.hpp>
#include <ctjson/Json.hpp>

#include "Utils.hpp"

using namespace ctjson;
using namespace Catch::Matchers;

template <typename T>
void expect_result(const std::string &json, const T &value) {
    auto result = parse<T>(json);
    if (!result.is_ok()) {
        FAIL("Result is: " << std::move(result).error().render()
                           << ", while parsing " << json);
    } else {
        REQUIRE(std::move(result).value() == value);
    }
}

TEST_CASE("Bool is deserialized", "[Deserialization]") {
    const auto test = [](const bool val) {
        expect_result<bool>(val ? "true" : "false", val);
    };

    test(true);
    test(false);
}

template <typename Number> static inline void test_number() {
    const auto test = [](const Number val) {
        auto json = std::to_string(val);
        auto result = parse<Number>(json);
        REQUIRE(result.is_ok());
        if constexpr (std::is_floating_point_v<Number>) {
            REQUIRE_THAT(std::move(result).value(),
                         WithinRel(val) || WithinAbs(val, 1e-6));
        } else {
            REQUIRE(std::move(result).value() == val);
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

TEST_CASE("Numbers are deserialized", "[Deserialization]") {
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

TEST_CASE("String is deserialized", "[Deserialization]") {
    expect_result<std::string>("\"example\"", "example");
}

TEST_CASE("Optional is deserialized", "[Deserialization]") {
    expect_result<std::optional<std::string>>("\"example\"", "example");
    expect_result<std::optional<std::string>>("null", std::nullopt);
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
        expect_result<Container>(json, arr);
    };

    compare("[" + elems + "]");
    if (size > 0) {
        compare("[" + elems + ",]");
    }
}

TEST_CASE("Arrays are deserialized", "[Deserialization]") {
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
            return "\"" + key + "\": " + std::to_string(value);
        },
        ',');

    const auto compare = [&dict](const std::string json) {
        expect_result<Container>(json, dict);
    };

    compare("{" + elems + "}");
    if (size > 0) {
        compare("{" + elems + ",}");
    }
}

TEST_CASE("Dicts are deserialized", "[Deserialization]") {
    test_dict<std::map<std::string, int>>(0);
    test_dict<std::map<std::string, int>>(1);
    test_dict<std::map<std::string, int>>(2);
    test_dict<std::map<std::string, int>>(42);
    test_dict<std::unordered_map<std::string, int>>(0);
    test_dict<std::unordered_map<std::string, int>>(1);
    test_dict<std::unordered_map<std::string, int>>(2);
    test_dict<std::unordered_map<std::string, int>>(42);
}

struct ParseClass {
    std::string str;
    int integer;

    bool operator==(const ParseClass &other) const {
        return str == other.str && integer == other.integer;
    }

    template <typename Tokens>
    static ParseResult<ParseClass> json_parse(Tokens &tokens) {
        ParseClass object;
        auto str = DeserializationHelper::Field("str", object.str);
        auto integer = DeserializationHelper::Field("integer", object.integer);
        auto result = DeserializationHelper::parse_object(tokens, str, integer);
        if (result.is_ok()) {
            return ParseResult<ParseClass>::result(std::move(object));
        } else {
            return ParseResult<ParseClass>::convert_error(std::move(result));
        }
    }
};

TEST_CASE("Object with `parse` method is deserialized", "[Deserialization]") {
    {
        const auto object = ParseClass{.str = "meaning", .integer = 42};

        expect_result("{\"str\": \"meaning\", \"integer\": 42}", object);
    }
    {
        auto result = parse<ParseClass>("{\"integer\": 42}");
        REQUIRE(result.is_parse_error());
    }
    {
        auto result = parse<ParseClass>(
            "{\"str\": \"meaning\", \"integer\": 42, \"add\": 100}");
        REQUIRE(result.is_parse_error());
    }
}

struct DeserializableClass {
    bool boolean;
    int integer;

    bool operator==(const DeserializableClass &other) const {
        return boolean == other.boolean && integer == other.integer;
    }
};

template <typename Tokens>
struct Deserializable<DeserializableClass, Tokens> : public std::true_type {
    static ParseResult<DeserializableClass> parse(Tokens &tokens) {
        DeserializableClass object;
        auto boolean = DeserializationHelper::Field("boolean", object.boolean);
        auto integer = DeserializationHelper::Field("integer", object.integer);
        auto result =
            DeserializationHelper::parse_object(tokens, boolean, integer);
        if (result.is_ok()) {
            return ParseResult<DeserializableClass>::result(std::move(object));
        } else {
            return ParseResult<DeserializableClass>::convert_error(
                std::move(result));
        }
    }
};

TEST_CASE("Object with `Deserializable` instance is deserialized",
          "[Deserialization]") {
    {
        const auto object =
            DeserializableClass{.boolean = false, .integer = 42};

        expect_result("{\"boolean\": false, \"integer\": 42}", object);
    }
    {
        auto result = parse<DeserializableClass>("{\"integer\": 42}");
        REQUIRE(result.is_parse_error());
    }
    {
        auto result = parse<DeserializableClass>(
            "{\"boolean\": false, \"integer\": 42, \"add\": 100}");
        REQUIRE(result.is_parse_error());
    }
}

struct FromStringClass {
    std::string str;

    bool operator==(const FromStringClass &other) const {
        return str == other.str;
    }

    template <typename Tokens>
    static ParseResult<FromStringClass> json_parse(Tokens &tokens) {
        return DeserializationHelper::parse_from<std::string>(
            tokens, [](std::string str) {
                return FromStringClass{.str = std::move(str)};
            });
    }
};

TEST_CASE("Object is deserialized from mapping", "[Deserialization]") {
    const auto object = FromStringClass{.str = "example"};

    expect_result("\"example\"", object);
}

struct FromStringResultClass {
    std::string str;

    bool operator==(const FromStringResultClass &other) const {
        return str == other.str;
    }

    template <typename Tokens>
    static ParseResult<FromStringResultClass> json_parse(Tokens &tokens) {
        return DeserializationHelper::parse_from<std::string>(
            tokens, [](std::string str) {
                const std::string prefix = "custom_";
                if (str.compare(0, prefix.length(), prefix) == 0) {
                    return ParseResult<FromStringResultClass>::result(
                        FromStringResultClass{.str = std::move(str)});
                } else {
                    return ParseResult<FromStringResultClass>::parse_error(
                        "Expected string with prefix: " + prefix);
                }
            });
    }
};

TEST_CASE("Object is deserialized from mapping that could fail",
          "[Deserialization]") {
    const auto object = FromStringResultClass{.str = "custom_example"};

    expect_result("\"custom_example\"", object);

    auto result = parse<FromStringResultClass>("\"example\"");
    REQUIRE(result.is_parse_error());
}

struct InnerClass {
    std::string str;
    std::optional<int> oint;

    bool operator==(const InnerClass &other) const {
        return str == other.str && oint == other.oint;
    }

    template <typename Tokens>
    static ParseResult<InnerClass> json_parse(Tokens &tokens) {
        InnerClass object;
        auto str = DeserializationHelper::Field("str", object.str);
        auto oint = DeserializationHelper::Field("oint", object.oint);
        auto result = DeserializationHelper::parse_object(tokens, str, oint);
        if (result.is_ok()) {
            return ParseResult<InnerClass>::result(std::move(object));
        } else {
            return ParseResult<InnerClass>::convert_error(std::move(result));
        }
    }
};

struct OuterClass {
    bool boolean;
    std::string str;
    std::optional<InnerClass> opt;
    std::vector<InnerClass> arr;
    std::map<std::string, InnerClass> map;

    bool operator==(const OuterClass &other) const {
        return str == other.str && boolean == other.boolean &&
               opt == other.opt && arr == other.arr && map == other.map;
    }

    template <typename Tokens>
    static ParseResult<OuterClass> json_parse(Tokens &tokens) {
        OuterClass object;
        auto boolean = DeserializationHelper::Field("boolean", object.boolean);
        auto str = DeserializationHelper::Field("str", object.str);
        auto opt = DeserializationHelper::Field("opt", object.opt);
        auto arr = DeserializationHelper::Field("arr", object.arr);
        auto map = DeserializationHelper::Field("map", object.map);
        auto result = DeserializationHelper::parse_object(tokens, boolean, str,
                                                          opt, arr, map);
        if (result.is_ok()) {
            return ParseResult<OuterClass>::result(std::move(object));
        } else {
            return ParseResult<OuterClass>::convert_error(std::move(result));
        }
    }
};

TEST_CASE("Nested classes are deserialized", "[Deserialization]") {
    {
        const std::string json = "{\
            \"boolean\": false, \
            \"str\": \"example\", \
            \"opt\": null, \
            \"arr\": [{\"str\": \"one\", \"oint\": 1}, {\"str\": \"none\",}], \
            \"map\": {}, \
        }";

        const auto object =
            OuterClass{.boolean = false,
                       .str = "example",
                       .opt = std::nullopt,
                       .arr = {InnerClass{.str = "one", .oint = 1},
                               InnerClass{.str = "none", .oint = std::nullopt}},
                       .map = {}};

        expect_result(json, object);
    }
    {
        const std::string json = "{\
            \"boolean\": false, \
            \"str\": \"example\", \
            \"opt\": {\"str\": \"none\", \"oint\": null}, \
            \"arr\": [],\
            \"map\": {\"test\": {\"str\": \"one\", \"oint\": 1}},\
        }";

        const auto object =
            OuterClass{.boolean = false,
                       .str = "example",
                       .opt = InnerClass{.str = "none", .oint = std::nullopt},
                       .arr = {},
                       .map = {{"test", InnerClass{.str = "one", .oint = 1}}}};

        expect_result(json, object);
    }
}

struct InnerClassError {
    std::string str;
    int integer;

    bool operator==(const InnerClassError &other) const {
        return str == other.str && integer == other.integer;
    }

    template <typename Tokens>
    static ParseResult<InnerClassError> json_parse(Tokens &tokens) {
        InnerClassError object;
        auto str = DeserializationHelper::Field("str", object.str);
        auto integer = DeserializationHelper::Field("integer", object.integer);
        auto result = DeserializationHelper::parse_object(tokens, str, integer);
        if (result.is_ok()) {
            return ParseResult<InnerClassError>::result(std::move(object));
        } else {
            return ParseResult<InnerClassError>::convert_error(
                std::move(result));
        }
    }
};

struct OuterClassError {
    double number;
    std::vector<InnerClassError> inners;

    bool operator==(const OuterClassError &other) const {
        return number == other.number && inners == other.inners;
    }

    template <typename Tokens>
    static ParseResult<OuterClassError> json_parse(Tokens &tokens) {
        OuterClassError object;
        auto number = DeserializationHelper::Field("number", object.number);
        auto inners = DeserializationHelper::Field("inners", object.inners);
        auto result =
            DeserializationHelper::parse_object(tokens, number, inners);
        if (result.is_ok()) {
            return ParseResult<OuterClassError>::result(std::move(object));
        } else {
            return ParseResult<OuterClassError>::convert_error(
                std::move(result));
        }
    }
};

template <typename... Args> std::string get_json_path(Args... args) {
    const auto render = [](auto el) {
        using T = std::decay_t<decltype(el)>;
        if constexpr (std::is_same_v<T, const char *> ||
                      std::is_same_v<T, std::string>) {
            return "." + std::string(el);
        } else if constexpr (std::is_integral_v<T>) {
            return "[" + std::to_string(el) + "]";
        } else {
            static_assert(!sizeof(T), "Unexpected type");
        }
    };

    return ("root" + ... + render(args));
}

TEST_CASE("Parsing errors are correct", "[Deserialization]") {
    const auto test = [](const char *json, const std::string &path) {
        auto result = parse<OuterClassError>(json);
        INFO("json is " << json);
        REQUIRE(result.is_parse_error());
        auto error = std::move(result).error();
        REQUIRE(error.path.has_value());
        REQUIRE(error.path.value() == path);
    };

    test("{\
        \"number\": false,\
    }",
         get_json_path("number"));
    test("{\
        \"number\": 1.0,\
        \"inners\": {}\
    }",
         get_json_path("inners"));
    test("{\
        \"number\": 1.0,\
        \"inners\": [true]\
    }",
         get_json_path("inners", 0));
    test("{\
        \"number\": 1.0,\
        \"inners\": [{}]\
    }",
         get_json_path("inners", 0));
    test("{\
        \"number\": 1.0,\
        \"inners\": [{\"str\": true}]\
    }",
         get_json_path("inners", 0, "str"));
    test("{\
        \"number\": 1.0,\
        \"inners\": [{\"str\": {}}]\
    }",
         get_json_path("inners", 0, "str"));
    test("{\
        \"number\": 1.0,\
        \"inners\": [{\"str\": []}]\
    }",
         get_json_path("inners", 0, "str"));
    test("{\
        \"number\": 1.0,\
        \"inners\": [{\"str\": \"example\"}]\
    }",
         get_json_path("inners", 0));
    test("{\
        \"number\": 1.0,\
        \"inners\": [{\"str\": \"example\", \"integer\": true}]\
    }",
         get_json_path("inners", 0, "integer"));
    test("{\
        \"number\": 1.0,\
        \"inners\": [{\"str\": \"example\", \"integer\": 42, \"dup\": true}]\
    }",
         get_json_path("inners", 0, "dup"));
    test("{\
        \"number\": 1.0,\
        \"inners\": [{\"str\": \"example\", \"integer\": 42}, true]\
    }",
         get_json_path("inners", 1));
    test("{\
        \"number\": 1.0,\
        \"inners\": [{\"str\": \"example\", \"integer\": 42}, []]\
    }",
         get_json_path("inners", 1));
    test("{\
        \"number\": 1.0,\
        \"inners\": [{\"str\": \"example\", \"integer\": 42}, {}]\
    }",
         get_json_path("inners", 1));
}

TEST_CASE("Json parsing errors are correct", "[Deserialization]") {
    const auto test = [](const char *json) {
        auto result = parse<OuterClassError>(json);
        INFO("json is " << json);
        REQUIRE(result.is_json_error());
        auto error = std::move(result).error();
        REQUIRE(error.path.has_value());
    };

    test("{\
        \"number\", false,\
    }");
    test("{\
        \"number\": 1.0\
        \"inners\": {}\
    }");
    test("{\
        \"number\": 1.0 \"test\",\
        \"inners\": [true]\
    }");
    test("{\
        \"number\": 1.0,\
        \"inners\": [{]\
    }");
    test("{\
        \"number\": 1.0,\
        \"inners\": [{str: true}]\
    }");
    test("{\
        \"number\": 1.0,\
        \"inners\": [{\"str\": ,}]\
    }");
    test("{\
        \"number\": 1.0,\
        \"inners\": [{\"str\": \"42\"]}]\
    }");
    test("{\
        \"number\": 1.0,\
        \"inners\": [{\"str\": \"example\" 42}]\
    }");
    test("{\
        \"number\": 1.0,\
        \"inners\": [{\"str\": \"example\", \"integer\": ] true,}]\
    }");
    test("{\
        \"number\": 1.0,\
        \"inners\": [{\"str\": \"exa\" \"mple\", \"integer\": 42, \"dup\": true}]\
    }");
    test("{\
        \"number\": 1.0,\
        \"inners\": [{\"str\": \"example\", \"integer\" - 42}, true]\
    }");
    test("{\
        \"number\": 1.0,\
        \"inners\": [{\"str\": \"example\", \"integer\" [] 42}, []]\
    }");
    test("{\
        \"number\": 1.0,\
        \"inners\": [{\"str\": \"example\" {} \"integer\": 42}, {}]\
    }");
}