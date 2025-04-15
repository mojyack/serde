#pragma once
#include <ranges>
#include <span>

#include "util/charconv.hpp"
#include "xml/xml.hpp"

#include "../serde.hpp"

#include "../macro.hpp"

namespace serde {
struct XmlFormat {
    using ReadType  = const ::xml::Node;
    using WriteType = ::xml::Node;
};

namespace xml {
template <class T>
concept xml_node = std::same_as<decltype(T::data), std::string> && serde::serde_struct<T>;
} // namespace xml

// string attribute
inline auto serialize(XmlFormat& /*format*/, const char* const name, ::xml::Node& payload, const std::string& data) -> bool {
    payload[name] = data;
    return true;
}

inline auto deserialize(XmlFormat& /*format*/, const char* const name, const ::xml::Node& payload, std::string& data) -> bool {
    unwrap(attr, payload.find_attr(name));
    data = attr;
    return true;
}

// numeric attribute
template <class T>
    requires(std::integral<T> || std::floating_point<T>)
inline auto serialize(XmlFormat& /*format*/, const char* const name, ::xml::Node& payload, const T& data) -> bool {
    payload[name] = std::to_string(data);
    return true;
}

template <class T>
    requires(std::integral<T> || std::floating_point<T>)
inline auto deserialize(XmlFormat& /*format*/, const char* const name, const ::xml::Node& payload, T& data) -> bool {
    unwrap(attr, payload.find_attr(name));
    unwrap(num, from_chars<T>(attr));
    data = num;
    return true;
}

// enum attribute
template <enumlike T>
inline auto serialize(XmlFormat& /*format*/, const char* const name, ::xml::Node& payload, const T& data) -> bool {
    unwrap(str, to_string(data));
    payload[name] = std::move(str);
    return true;
}

template <enumlike T>
inline auto deserialize(XmlFormat& /*format*/, const char* const name, const ::xml::Node& payload, T& data) -> bool {
    unwrap(attr, payload.find_attr(name));
    unwrap(num, from_string<T>(attr));
    data = num;
    return true;
}

// span
template <serde::serde_struct T>
inline auto serialize(XmlFormat& format, const char* const name, ::xml::Node& payload, const std::span<const T>& data) -> bool {
    for(const auto& e : data) {
        auto& child = payload.children.emplace_back();
        child.name  = name;
        if constexpr(xml::xml_node<T>) {
            child.data = e.data;
        }
        ensure(serde::impl::call_each_serialize(format, e, child));
    }
    return true;
}

template <serde::serde_struct T>
inline auto deserialize(XmlFormat& /*format*/, const char* const /*name*/, const ::xml::Node& /*payload*/, std::span<T>& /*data*/) -> bool {
    static_assert(false, "span is not deserializable");
    return false;
}

// children
template <serde::serde_struct T>
inline auto serialize(XmlFormat& format, const char* const name, ::xml::Node& payload, const std::vector<T>& data) -> bool {
    ensure(serialize(format, name, payload, std::span{data}));
    return true;
}

template <serde::serde_struct T>
inline auto deserialize(XmlFormat& format, const char* const name, const ::xml::Node& payload, std::vector<T>& data) -> bool {
    for(const auto& c : payload.children) {
        if(c.name != name) {
            continue;
        }
        auto child = T();
        if constexpr(xml::xml_node<T>) {
            child.data = c.data;
        }
        ensure(serde::impl::call_each_deserialize(format, child, c));
        data.emplace_back(std::move(child));
    }
    return true;
}

// fixed-length children
template <serde::serde_struct T, size_t len>
inline auto serialize(XmlFormat& format, const char* const name, ::xml::Node& payload, const std::array<T, len>& data) -> bool {
    ensure(serialize(format, name, payload, std::span{data.data(), data.size()}));
    return true;
}

template <serde::serde_struct T, size_t len>
inline auto deserialize(XmlFormat& format, const char* const name, const ::xml::Node& payload, std::array<T, len>& data) -> bool {
    auto buf = std::vector<T>();
    ensure(deserialize(format, name, payload, buf));
    ensure(buf.size() == len);
    for(auto&& [src, dst] : std::views::zip(buf, data)) {
        dst = std::move(src);
    }
    return true;
}
} // namespace serde

#include "../macro-pop.hpp"
