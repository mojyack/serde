#pragma once
#include <ranges>
#include <vector>

#if !defined(SERDE_NO_INCLUDE)
#include "json/json.hpp"
#endif

#include "../serde.hpp"

#include "../macro.hpp"

namespace serde {
namespace json {
// numeric
template <class T>
    requires(std::integral<T> || std::floating_point<T>)
auto serialize_element(::json::Value& value, const T& data) -> bool {
    value.emplace<::json::Number>(double(data));
    return true;
}

template <class T>
    requires(std::integral<T> || std::floating_point<T>)
auto deserialize_element(const ::json::Value& value, T& data) -> bool {
    unwrap(node, value.get<::json::Number>());
    data = node.value;
    return true;
}

// string
inline auto serialize_element(::json::Value& value, const std::string& data) -> bool {
    value.emplace<::json::String>(data);
    return true;
}

inline auto deserialize_element(const ::json::Value& value, std::string& data) -> bool {
    unwrap(node, value.get<::json::String>());
    data = node.value;
    return true;
}

// boolean
inline auto serialize_element(::json::Value& value, const bool& data) -> bool {
    value.emplace<::json::Boolean>(data);
    return true;
}

inline auto deserialize_element(const ::json::Value& value, bool& data) -> bool {
    unwrap(node, value.get<::json::Boolean>());
    data = node.value;
    return true;
}

// enum
template <enumlike T>
inline auto serialize_element(::json::Value& value, const T& data) -> bool {
    unwrap(str, to_string(data));
    value.emplace<::json::String>(std::move(str));
    return true;
}

template <enumlike T>
inline auto deserialize_element(const ::json::Value& value, T& data) -> bool {
    unwrap(node, value.get<::json::String>());
    unwrap(num, from_string<T>(node.value));
    data = num;
    return true;
}

template <class T>
concept serializable = requires(::json::Value& value, const T& data) {
    serialize_element(value, data);
};

template <class T>
concept deserializable = requires(const ::json::Value& value, T& data) {
    { deserialize_element(value, data) } -> std::same_as<bool>;
};
} // namespace json

struct JsonFormat {
    using ReadType  = const ::json::Object;
    using WriteType = ::json::Object;
};

// primitives
template <json::serializable T>
inline auto serialize(JsonFormat& /*format*/, const char* const name, ::json::Object& payload, const T& data) -> bool {
    ensure(json::serialize_element(payload[name], data));
    return true;
}

template <json::serializable T>
inline auto deserialize(JsonFormat& /*format*/, const char* const name, const ::json::Object& payload, T& data) -> bool {
    unwrap(node, payload.find(name));
    ensure(json::deserialize_element(node, data));
    return true;
}

// span
template <json::serializable T>
inline auto serialize(JsonFormat& /*format*/, const char* const name, ::json::Object& payload, const std::span<const T>& data) -> bool {
    auto array = ::json::Array();
    array.value.resize(data.size());
    for(auto&& [a, d] : std::views::zip(array.value, data)) {
        ensure(json::serialize_element(a, d));
    }
    payload[name] = ::json::Value::create<::json::Array>(std::move(array));
    return true;
}

template <json::serializable T>
inline auto deserialize(JsonFormat& /*format*/, const char* const /*name*/, const ::json::Object& /*payload*/, std::span<T>& /*data*/) -> bool {
    static_assert(false, "span is not deserializable");
    return false;
}

// array
template <json::serializable T>
inline auto serialize(JsonFormat& format, const char* const name, ::json::Object& payload, const std::vector<T>& data) -> bool {
    ensure(serialize(format, name, payload, std::span{data}));
    return true;
}

template <json::serializable T>
inline auto deserialize(JsonFormat& /*format*/, const char* const name, const ::json::Object& payload, std::vector<T>& data) -> bool {
    unwrap(array, payload.find<::json::Array>(name));
    data.resize(array.value.size());
    for(auto&& [a, d] : std::views::zip(array.value, data)) {
        ensure(json::deserialize_element(a, d));
    }
    return true;
}

// fixed-length array
template <json::serializable T, size_t len>
inline auto serialize(JsonFormat& format, const char* const name, ::json::Object& payload, const std::array<T, len>& data) -> bool {
    ensure(serialize(format, name, payload, std::span{data.data(), data.size()}));
    return true;
}

template <json::serializable T, size_t len>
inline auto deserialize(JsonFormat& /*format*/, const char* const name, const ::json::Object& payload, std::array<T, len>& data) -> bool {
    unwrap(array, payload.find<::json::Array>(name));
    ensure(len == array.value.size());
    for(auto&& [a, d] : std::views::zip(array.value, data)) {
        ensure(json::deserialize_element(a, d));
    }
    return true;
}

// object
template <serde_struct T>
inline auto serialize(JsonFormat& format, const char* const name, ::json::Object& payload, const T& data) -> bool {
    ensure(serde::impl::call_each_serialize(format, data, payload[name].emplace<::json::Object>()));
    return true;
}

template <serde_struct T>
inline auto deserialize(JsonFormat& format, const char* const name, const ::json::Object& payload, T& data) -> bool {
    unwrap(object, payload.find<::json::Object>(name));
    ensure(serde::impl::call_each_deserialize(format, data, object));
    return true;
}
} // namespace serde

#include "../macro-pop.hpp"
