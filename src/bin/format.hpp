#pragma once
#include <span>
#include <vector>

#include "../serde.hpp"

#include "../macro.hpp"

namespace serde {
template <std::integral SizeType = size_t>
struct BinaryFormat {
    using ReadType  = std::span<const std::byte>;
    using WriteType = std::vector<std::byte>;
};

// numeric
template <std::integral SizeType, class T>
    requires(std::is_trivially_copyable_v<T>)
inline auto serialize(BinaryFormat<SizeType>& /*format*/, const char* const /*name*/, std::vector<std::byte>& payload, const T& data) -> bool {
    const auto prev_size = payload.size();
    payload.resize(prev_size + sizeof(T));
    *std::bit_cast<T*>(payload.data() + prev_size) = data;
    return true;
}

template <std::integral SizeType, class T>
    requires(std::is_trivially_copyable_v<T>)
inline auto deserialize(BinaryFormat<SizeType>& /*format*/, const char* const /*name*/, std::span<const std::byte>& payload, T& data) -> bool {
    ensure(payload.size() >= sizeof(T));
    data    = *std::bit_cast<T*>(payload.data());
    payload = payload.subspan(sizeof(T));
    return true;
}

// string
template <std::integral SizeType>
inline auto serialize(BinaryFormat<SizeType>& format, const char* const name, std::vector<std::byte>& payload, const std::string& data) -> bool {
    ensure(data.size() <= std::numeric_limits<SizeType>::max(), "field {} is too large to serialize", name);
    ensure(serialize(format, name, payload, SizeType(data.size())));
    const auto prev_size = payload.size();
    payload.resize(prev_size + data.size());
    std::memcpy(payload.data() + prev_size, data.data(), data.size());
    return true;
}

template <std::integral SizeType>
inline auto deserialize(BinaryFormat<SizeType>& format, const char* const name, std::span<const std::byte>& payload, std::string& data) -> bool {
    auto body_size = SizeType();
    ensure(deserialize(format, name, payload, body_size));
    ensure(payload.size() >= body_size);
    data.resize(body_size);
    std::memcpy(data.data(), payload.data(), body_size);
    payload = payload.subspan(body_size);
    return true;
}

// array
template <std::integral SizeType, class T>
inline auto serialize(BinaryFormat<SizeType>& format, const char* const name, std::vector<std::byte>& payload, const std::vector<T>& data) -> bool {
    ensure(data.size() <= std::numeric_limits<SizeType>::max(), "field {} is too large to serialize", name);
    ensure(serialize(format, name, payload, SizeType(data.size())));
    for(const auto& e : data) {
        ensure(serialize(format, name, payload, e));
    }
    return true;
}

template <std::integral SizeType, class T>
inline auto deserialize(BinaryFormat<SizeType>& format, const char* const name, std::span<const std::byte>& payload, std::vector<T>& data) -> bool {
    auto body_size = SizeType();
    ensure(deserialize(format, name, payload, body_size));
    data.resize(body_size);
    for(auto& e : data) {
        ensure(deserialize(format, name, payload, e));
    }
    return true;
}

// fixed-length array
template <std::integral SizeType, class T, size_t len>
inline auto serialize(BinaryFormat<SizeType>& format, const char* const name, std::vector<std::byte>& payload, const std::array<T, len>& data) -> bool {
    for(const auto& e : data) {
        ensure(serialize(format, name, payload, e));
    }
    return true;
}

template <std::integral SizeType, class T, size_t len>
inline auto deserialize(BinaryFormat<SizeType>& format, const char* const name, std::span<const std::byte>& payload, std::array<T, len>& data) -> bool {
    for(auto& e : data) {
        ensure(deserialize(format, name, payload, e));
    }
    return true;
}

// struct
template <std::integral SizeType, class T>
    requires(serde_struct<T> && !std::is_trivially_copyable_v<T>)
inline auto serialize(BinaryFormat<SizeType>& format, const char* const /*name*/, std::vector<std::byte>& payload, const T& data) -> bool {
    ensure(serde::impl::call_each_serialize(format, data, payload));
    return true;
}

template <std::integral SizeType, class T>
    requires(serde_struct<T> && !std::is_trivially_copyable_v<T>)
inline auto deserialize(BinaryFormat<SizeType>& format, const char* const /*name*/, std::span<const std::byte>& payload, T& data) -> bool {
    ensure(serde::impl::call_each_deserialize(format, data, payload));
    return true;
}
} // namespace serde

#include "../macro-pop.hpp"
