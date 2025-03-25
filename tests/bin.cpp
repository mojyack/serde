#include <ranges>

#include "serde/bin/format.hpp"

#include "macro.hpp"

#include "enum.hpp" // note that binary format does not use {to,from}_string(Enum)

// primitives
struct Primitives {
    SerdeFieldsBegin;
    int         SerdeField(integer);
    double      SerdeField(floating);
    std::string SerdeField(string);
    Enum        SerdeField(enumerator);
    SerdeFieldsEnd;
};

auto primitives() -> int {
    const auto a = Primitives{
        .integer    = 1,
        .floating   = 2.0,
        .string     = "string",
        .enumerator = Enum::A,
    };

    unwrap(bin, a.dump<serde::BinaryFormat<>>());
    unwrap(b, (serde::load<serde::BinaryFormat<>, Primitives>(bin)));
    ensure(a.integer == b.integer);
    ensure(a.floating == b.floating);
    ensure(a.string == b.string);
    ensure(a.enumerator == b.enumerator);
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
    std::array<SubSubStruct, 3> SerdeField(subs);
    SerdeFieldsEnd;
};

struct Containers {
    SerdeFieldsBegin;
    std::vector<int>           SerdeField(ivec);
    std::array<int, 3>         SerdeField(iarray);
    std::array<std::string, 2> SerdeField(sarray);
    SubSubStruct               SerdeField(child);
    std::array<SubStruct, 2>   SerdeField(children);
    SerdeFieldsEnd;
};

auto containers() -> int {
    const auto a = Containers{
        .ivec     = {1, 2, 3},
        .iarray   = {4, 5, 6},
        .sarray   = {"hello", "world"},
        .child    = {1},
        .children = {{
            {.subs = {{{11}, {12}, {13}}}},
            {.subs = {{{21}, {22}, {23}}}},
        }},
    };

    unwrap(bin, a.dump<serde::BinaryFormat<>>());
    unwrap(b, (serde::load<serde::BinaryFormat<>, Containers>(bin)));
    ensure(a.ivec == b.ivec);
    ensure(a.iarray == b.iarray);
    ensure(a.sarray == b.sarray);
    ensure(a.child.num == b.child.num);
    for(const auto& [c1, c2] : std::views::zip(a.children, b.children)) {
        for(const auto& [s1, s2] : std::views::zip(c1.subs, c2.subs)) {
            ensure(s1.num == s2.num);
        }
    }
    return 0;
}

// use short size type for saving binary size
struct ShortSize {
    SerdeFieldsBegin;
    std::vector<uint8_t> SerdeField(bytes);
    std::string          SerdeField(string);
    SerdeFieldsEnd;

    auto fill(const size_t count) -> void {
        bytes.resize(count);
        string.resize(count);
        auto i = 0;
        for(auto&& [b, c] : std::views::zip(bytes, string)) {
            b = i;
            c = i;
            i += 1;
        }
    }
};

auto short_size() -> int {
    auto a = ShortSize{};
    a.fill(0xff);
    unwrap(small, a.dump<serde::BinaryFormat<uint8_t>>());
    unwrap(big, a.dump<serde::BinaryFormat<size_t>>());
    ensure(small.size() < big.size());
    unwrap(b, (serde::load<serde::BinaryFormat<uint8_t>, ShortSize>(small)));
    ensure(a.bytes == b.bytes);
    ensure(a.string == b.string);
    return 0;
}

auto too_large() -> int {
    auto a = ShortSize{};
    a.fill(0xff + 1);
    ensure(!a.dump<serde::BinaryFormat<uint8_t>>());
    return 0;
}

auto main() -> int {
    ensure(primitives() == 0);
    ensure(containers() == 0);
    ensure(short_size() == 0);
    ensure(too_large() == 0);
    std::println("pass");
    return 0;
}
