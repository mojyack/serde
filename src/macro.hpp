#pragma push_macro("ensure")
#pragma push_macro("unwrap")

#undef ensure
#define ensure(var, ...)                               \
    if(!(var)) {                                       \
        __VA_OPT__(std::println(stderr, __VA_ARGS__)); \
        return false;                                  \
    }

#undef unwrap
#define unwrap(var, opt)    \
    auto var##_opt = (opt); \
    ensure(var##_opt);      \
    auto& var = *var##_opt;
