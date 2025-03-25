#pragma once
#include "serde/serde.hpp"

enum class Enum {
    A,
    B,
    C,
};

namespace serde {
template <>
inline auto to_string(const Enum data) -> std::optional<std::string> {
    switch(data) {
    case Enum::A:
        return "a";
    case Enum::B:
        return "b";
    case Enum::C:
        return "c";
    default:
        return std::nullopt;
    }
};

template <>
inline auto from_string(const std::string_view str) -> std::optional<Enum> {
    if(str == "a") return Enum::A;
    if(str == "b") return Enum::B;
    if(str == "c") return Enum::C;
    return std::nullopt;
}
} // namespace serde

