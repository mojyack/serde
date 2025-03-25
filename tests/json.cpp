#include <array>
#include <vector>

#include "serde/json/format.hpp"

#include "macro.hpp"

#include "enum.hpp"

// primitives
struct Primitives {
    SerdeFieldsBegin;
    int          SerdeField(num);
    unsigned int SerdeField(unum);
    double       SerdeField(floating);
    std::string  SerdeField(string);
    bool         SerdeField(boolean);
    Enum         SerdeField(enumerator);
    SerdeFieldsEnd;
};

auto primitives() -> int {
    const auto str = R"({
        "num": -1,
        "unum": 1,
        "floating": 3.14,
        "string": "hello world",
        "boolean": true,
        "enumerator": "a"
    })";

    unwrap(node_pre, json::parse(str));
    unwrap(obj_pre, (serde::load<serde::JsonFormat, Primitives>(node_pre)));
    unwrap(node, obj_pre.dump<serde::JsonFormat>());
    unwrap(obj, (serde::load<serde::JsonFormat, Primitives>(node)));

    ensure(obj.num == -1);
    ensure(obj.unum == 1);
    ensure(obj.floating == 3.14);
    ensure(obj.string == "hello world");
    ensure(obj.boolean == true);
    ensure(obj.enumerator == Enum::A);
    return 0;
}

// containers
struct SubSubStruct {
    SerdeFieldsBegin;
    int SerdeField(num);
    SerdeFieldsEnd;
};

struct SubStruct {
    SerdeFieldsBegin;
    int          SerdeField(num);
    SubSubStruct SerdeField(child);
    SerdeFieldsEnd;
};

struct Containers {
    SerdeFieldsBegin;

    std::vector<int>   SerdeField(vector);
    std::array<int, 4> SerdeField(array);
    SubStruct          SerdeField(child);

    SerdeFieldsEnd;
};

auto containers() -> int {
    const auto str = R"({
        "vector": [1,2,3,4,5],
        "array": [6,7,8,9],
        "child": {
            "num": 10,
            "child": {
                "num": 20
            }
        }
    })";

    unwrap(node_pre, json::parse(str));
    unwrap(obj_pre, (serde::load<serde::JsonFormat, Containers>(node_pre)));
    unwrap(node, obj_pre.dump<serde::JsonFormat>());
    unwrap(obj, (serde::load<serde::JsonFormat, Containers>(node)));

    ensure((obj.vector == std::vector{1, 2, 3, 4, 5}));
    ensure((obj.array == std::array{6, 7, 8, 9}));
    ensure((obj.child.num == 10));
    ensure((obj.child.child.num == 20));
    return 0;
}

// serde features
struct Features {
    SerdeFieldsBegin;
    std::optional<int> SerdeField(onum1);                   // optional not exists
    std::optional<int> SerdeField(onum2);                   // optional exists
    int                SerdeNamedField(num_num, "num-num"); // another key
    int                SerdeField(num_def, -1);             // default value
    SerdeFieldsEnd;
};

auto features() -> int {
    const auto str = R"({
        "onum2": 2,
        "num-num": 3,
        "num_def": 4,
        "ignored": 5
    })";

    unwrap(node_pre, json::parse(str));
    unwrap(obj_pre, (serde::load<serde::JsonFormat, Features>(node_pre)));
    unwrap(node, obj_pre.dump<serde::JsonFormat>());
    unwrap(obj, (serde::load<serde::JsonFormat, Features>(node)));

    ensure(!obj.onum1.has_value());
    ensure(obj.onum2.has_value() && obj.onum2.value() == 2);
    ensure((obj.num_num == 3));
    ensure(Features().num_def == -1);
    ensure((obj.num_def == 4));
    return 0;
}

// missing field
struct MissingField {
    SerdeFieldsBegin;
    int SerdeField(a);
    int SerdeField(b);
    int SerdeField(c);
    SerdeFieldsEnd;
};

auto missing_field() -> int {
    const auto str = R"({
        "a": 1,
        "c": 1
    })";

    unwrap(node, json::parse(str));
    ensure(!(serde::load<serde::JsonFormat, MissingField>(node)).has_value());
    return 0;
}

// mismatched array length
struct MismatchedArrayLength {
    SerdeFieldsBegin;
    std::array<int, 3> SerdeField(a);
    SerdeFieldsEnd;
};

auto mismatched_array_length() -> int {
    const auto str = R"({
        "a": [1,2,3,4]
    })";

    unwrap(node, json::parse(str));
    ensure(!(serde::load<serde::JsonFormat, MismatchedArrayLength>(node)).has_value());
    return 0;
}

auto main() -> int {
    ensure(primitives() == 0);
    ensure(containers() == 0);
    ensure(features() == 0);
    ensure(missing_field() == 0);
    ensure(mismatched_array_length() == 0);
    std::println("pass");
    return 0;
}
