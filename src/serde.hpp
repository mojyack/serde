#pragma once
#include <concepts>
#include <optional>
#include <print>

namespace serde {
template <class T>
concept stream_format = requires {
    typename T::ReadType;
    typename T::WriteType;
};

template <class T>
concept serde_struct = requires {
    std::is_same_v<decltype(T::_serde_fields_begin), int>;
    std::is_same_v<decltype(T::_serde_fields_end), int>;
};

template <class T>
concept optlike = requires(const T& value) {
    typename T::value_type;
    { value.value() };
    { value.has_value() } -> std::same_as<bool>;
};

template <class T>
concept enumlike = std::is_enum_v<T>;

template <class Enum>
auto to_string(const Enum /*num*/) -> std::optional<std::string> {
    static_assert(false, "no string-to-enum converter defined");
}

template <class Enum>
auto from_string(const std::string_view /*str*/) -> std::optional<Enum> {
    static_assert(false, "no string-to-enum converter defined");
}

namespace impl {
template <size_t line>
struct Tag {};

template <class Format, class T, size_t line>
concept callable = stream_format<Format> &&
                   requires(Format& format, T& value, Format::WriteType& payload) {
                       value._serialize(format, payload, Tag<line>());
                   } && requires(Format& format, T& value, Format::ReadType& payload) {
                       value._deserialize(format, payload, Tag<line>());
                   };

// bulk invokers
template <stream_format Format, class T, size_t line = T::_serde_fields_begin>
auto call_each_serialize(Format& format, const T& self, typename Format::WriteType& payload) -> bool {
    if constexpr(line >= T::_serde_fields_end) {
        return true;
    } else {
        if constexpr(callable<Format, T, line>) {
            if(!self._serialize(format, payload, Tag<line>())) {
                return false;
            }
        }
        return call_each_serialize<Format, T, line + 1>(format, self, payload);
    }
}

template <stream_format Format, class T, size_t line = T::_serde_fields_begin>
auto call_each_deserialize(Format& format, T& self, typename Format::ReadType& payload) -> bool {
    if constexpr(line >= T::_serde_fields_end) {
        return true;
    } else {
        if constexpr(callable<Format, T, line>) {
            if(!self._deserialize(format, payload, Tag<line>())) {
                return false;
            }
        }
        return call_each_deserialize<Format, T, line + 1>(format, self, payload);
    }
}

// optlike support
template <stream_format Format, class T>
auto serialize_entry(Format& format, const char* const key, typename Format::WriteType& payload, const T& field) -> bool {
    return serialize(format, key, payload, field);
}

template <stream_format Format, optlike T>
auto serialize_entry(Format& format, const char* const key, typename Format::WriteType& payload, const T& field) -> bool {
    if(!field) {
        return true;
    }
    return serialize(format, key, payload, *field);
}

template <stream_format Format, class T>
auto deserialize_entry(Format& format, const char* const key, typename Format::ReadType& payload, T& field) -> bool {
    return deserialize(format, key, payload, field);
}

template <stream_format Format, optlike T>
auto deserialize_entry(Format& format, const char* const key, typename Format::ReadType& payload, T& field) -> bool {
    auto storage = typename T::value_type();
    if(deserialize(format, key, payload, storage)) {
        field = std::move(storage);
    }
    return true;
}

// macro definitions
#define SerdeFieldsBegin \
    constexpr static auto _serde_fields_begin = __LINE__

#define SerdeFieldsEnd                                                                                                         \
    constexpr static auto _serde_fields_end = __LINE__;                                                                        \
                                                                                                                               \
    template <serde::stream_format Format>                                                                                     \
    auto dump(typename Format::WriteType result = {}, Format format = {}) const -> std::optional<typename Format::WriteType> { \
        if(serde::impl::call_each_serialize<Format>(format, *this, result)) {                                                  \
            return std::move(result);                                                                                          \
        } else {                                                                                                               \
            return std::nullopt;                                                                                               \
        }                                                                                                                      \
    }

#define SerdeNamedField(name, key, ...)                                                                     \
    name __VA_OPT__(= __VA_ARGS__);                                                                         \
    template <serde::stream_format Format>                                                                  \
    auto _serialize(Format& format, Format::WriteType& payload, serde::impl::Tag<__LINE__>) const -> bool { \
        if(!serde::impl::serialize_entry(format, key, payload, name)) {                                     \
            std::println(stderr, R"(failed to serialize field "{}"(key="{}"))", #name, key);                \
            return false;                                                                                   \
        }                                                                                                   \
        return true;                                                                                        \
    }                                                                                                       \
                                                                                                            \
    template <serde::stream_format Format>                                                                  \
    auto _deserialize(Format& format, Format::ReadType& payload, serde::impl::Tag<__LINE__>) -> bool {      \
        if(!serde::impl::deserialize_entry(format, key, payload, name)) {                                   \
            std::println(stderr, R"(failed to deserialize field "{}"(key="{}"))", #name, key);              \
            return false;                                                                                   \
        }                                                                                                   \
        return true;                                                                                        \
    }

#define SerdeField(name, ...) SerdeNamedField(name, #name, __VA_ARGS__)
} // namespace impl

// load object from data
template <stream_format Format, serde_struct T>
auto load(typename Format::ReadType payload, T result = {}, Format format = Format()) -> std::optional<T> {
    if(impl::call_each_deserialize(format, result, payload)) {
        return result;
    } else {
        return std::nullopt;
    }
}
} // namespace serde
