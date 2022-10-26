#pragma once
#include <memory>
#include <vector>
#include <optional>
#include <functional>
#include <algorithm>

template<typename T>
using ptr = std::shared_ptr<T>;

template<typename T>
using opt = std::optional<T>;

#define match(expr,t,some,none) if(auto t = expr;t.has_value()) {some;} else {none;}
#define vkmc_count(arr) sizeof(arr) / sizeof(arr[0])

using u32 = uint32_t;
using u16 = uint16_t;
using u8 = uint8_t;
using i32 = int32_t;
using i16 = int16_t;
using i8 = int8_t;

constexpr u32 g_terrian_block_size = 128;

#define vkmc_assert(expr) if(!(expr)) {__debugbreak();exit(-1);}