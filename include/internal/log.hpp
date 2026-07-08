#pragma once

#include <print>
#include <format>

namespace grf::log {

template <typename... Args>
void general(std::format_string<Args...> fmt, Args&&... args) {
  std::println(stderr, "[GRF] {}", std::format(fmt, std::forward<Args>(args)...));
}

template <typename... Args>
void warning(std::format_string<Args...> fmt, Args&&... args) {
  std::println(stderr, "[GRF] Warning: {}", std::format(fmt, std::forward<Args>(args)...));
}

template <typename... Args>
void error(std::format_string<Args...> fmt, Args&&... args) {
  std::println(stderr, "[GRF] Error: {}", std::format(fmt, std::forward<Args>(args)...));
}

template <typename... Args>
[[noreturn]] void panic(std::format_string<Args...> fmt, Args&&... args) {
  std::println(stderr, "[GRF] PANIC: {}", std::format(fmt, std::forward<Args>(args)...));
  std::abort();
}

}