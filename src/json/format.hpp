#pragma once
#include <limits>
#include <ranges>
#include <vector>

#include "json/json.hpp"

#include "../serde.hpp"

#include "../macro.hpp"

namespace serde {
struct JsonFormat {
    using ReadType  = const ::json::Object;
    using WriteType = ::json::Object;
};

namespace json {
// integral
template <std::integral T>
auto serialize_element(JsonFormat& /*format*/, ::json::Value& value, const T& data) -> bool {
    ensure(T(double(data)) == data);
    value.emplace<::json::Number>(double(data));
    return true;
}

template <std::integral T>
auto deserialize_element(JsonFormat& /*format*/, const ::json::Value& value, T& data) -> bool {
    unwrap(node, value.get<::json::Number>());
    ensure(T(node.value) == node.value);
    data = node.value;
    return true;
}

// floating point
template <std::floating_point T>
auto serialize_element(JsonFormat& /*format*/, ::json::Value& value, const T& data) -> bool {
    ensure(data <= std::numeric_limits<double>::max() && data >= std::numeric_limits<double>::lowest());
    value.emplace<::json::Number>(double(data));
    return true;
}

template <std::floating_point T>
auto deserialize_element(JsonFormat& /*format*/, const ::json::Value& value, T& data) -> bool {
    unwrap(node, value.get<::json::Number>());
    ensure(node.value <= std::numeric_limits<T>::max() && node.value >= std::numeric_limits<T>::lowest());
    data = node.value;
    return true;
}

// string
inline auto serialize_element(JsonFormat& /*format*/, ::json::Value& value, const std::string& data) -> bool {
    value.emplace<::json::String>(data);
    return true;
}

inline auto deserialize_element(JsonFormat& /*format*/, const ::json::Value& value, std::string& data) -> bool {
    unwrap(node, value.get<::json::String>());
    data = node.value;
    return true;
}

// boolean
inline auto serialize_element(JsonFormat& /*format*/, ::json::Value& value, const bool& data) -> bool {
    value.emplace<::json::Boolean>(data);
    return true;
}

inline auto deserialize_element(JsonFormat& /*format*/, const ::json::Value& value, bool& data) -> bool {
    unwrap(node, value.get<::json::Boolean>());
    data = node.value;
    return true;
}

// enum
template <enumlike T>
auto serialize_element(JsonFormat& /*format*/, ::json::Value& value, const T& data) -> bool {
    unwrap(str, to_string(data));
    value.emplace<::json::String>(std::move(str));
    return true;
}

template <enumlike T>
auto deserialize_element(JsonFormat& /*format*/, const ::json::Value& value, T& data) -> bool {
    unwrap(node, value.get<::json::String>());
    unwrap(num, from_string<T>(node.value));
    data = num;
    return true;
}

// forward declarations
template <class T, size_t len>
auto serialize_element(JsonFormat& /*format*/, ::json::Value& payload, const std::array<T, len>& data) -> bool;

template <class T, size_t len>
auto deserialize_element(JsonFormat& /*format*/, const ::json::Value& payload, std::array<T, len>& data) -> bool;

template <serde_struct T>
auto serialize_element(JsonFormat& /*format*/, ::json::Value& payload, const T& data) -> bool;

template <serde_struct T>
auto deserialize_element(JsonFormat& /*format*/, const ::json::Value& payload, T& data) -> bool;

// span
template <class T>
auto serialize_element(JsonFormat& format, ::json::Value& payload, const std::span<const T>& data) -> bool {
    auto array = ::json::Array();
    array.value.resize(data.size());
    for(auto&& [a, d] : std::views::zip(array.value, data)) {
        ensure(json::serialize_element(format, a, d));
    }
    payload = ::json::Value::create<::json::Array>(std::move(array));
    return true;
}

template <class T>
auto deserialize_element(JsonFormat& /*format*/, const ::json::Value& /*payload*/, std::span<T>& /*data*/) -> bool {
    static_assert(false, "span is not deserializable");
    return false;
}

// array
template <class T>
auto serialize_element(JsonFormat& format, ::json::Value& payload, const std::vector<T>& data) -> bool {
    return serialize_element(format, payload, std::span(data));
}

template <class T>
auto deserialize_element(JsonFormat& format, const ::json::Value& payload, std::vector<T>& data) -> bool {
    unwrap(array, payload.get<::json::Array>());
    data.resize(array.value.size());
    for(auto&& [a, d] : std::views::zip(array.value, data)) {
        ensure(json::deserialize_element(format, a, d));
    }
    return true;
}

// fixed-length array
template <class T, size_t len>
auto serialize_element(JsonFormat& format, ::json::Value& payload, const std::array<T, len>& data) -> bool {
    return serialize_element(format, payload, std::span<const T>(data));
}

template <class T, size_t len>
auto deserialize_element(JsonFormat& format, const ::json::Value& payload, std::array<T, len>& data) -> bool {
    unwrap(array, payload.get<::json::Array>());
    ensure(len == array.value.size());
    for(auto&& [a, d] : std::views::zip(array.value, data)) {
        ensure(json::deserialize_element(format, a, d));
    }
    return true;
}

// object
template <serde_struct T>
auto serialize_element(JsonFormat& format, ::json::Value& payload, const T& data) -> bool {
    ensure(serde::impl::call_each_serialize(format, data, payload.emplace<::json::Object>()));
    return true;
}

template <serde_struct T>
auto deserialize_element(JsonFormat& format, const ::json::Value& payload, T& data) -> bool {
    unwrap(node, payload.get<::json::Object>());
    ensure(serde::impl::call_each_deserialize(format, data, node));
    return true;
}

// concept
template <class T>
concept serializable = requires(JsonFormat& format, ::json::Value& value, const T& data) {
    serialize_element(format, value, data);
};

template <class T>
concept deserializable = requires(JsonFormat& format, const ::json::Value& value, T& data) {
    { deserialize_element(format, value, data) } -> std::same_as<bool>;
};
} // namespace json

template <json::serializable T>
auto serialize(JsonFormat& format, const char* const name, ::json::Object& payload, const T& data) -> bool {
    ensure(json::serialize_element(format, payload[name], data));
    return true;
}

template <json::deserializable T>
auto deserialize(JsonFormat& format, const char* const name, const ::json::Object& payload, T& data) -> bool {
    unwrap(node, payload.find(name));
    ensure(json::deserialize_element(format, node, data));
    return true;
}
} // namespace serde

#include "../macro-pop.hpp"
