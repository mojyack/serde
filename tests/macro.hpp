#pragma once
#include <print>

#define ensure(var)                                            \
    if(!(var)) {                                               \
        std::println("assertion failed at line {}", __LINE__); \
        return -1;                                             \
    }

#define unwrap(var, opt)    \
    auto var##_opt = (opt); \
    ensure(var##_opt);      \
    auto& var = *var##_opt;
