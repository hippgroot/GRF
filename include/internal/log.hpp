#pragma once

#include "grf/types.hpp"

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
[[noreturn]] void error(std::format_string<Args...> fmt, Args&&... args) {
  std::println(stderr, "[GRF] ERROR: {}", std::format(fmt, std::forward<Args>(args)...));
  std::abort();
}

inline str to_string(ShaderStage stage) {
  switch(stage) {
  case ShaderStage::Vertex:   return "vertex";
  case ShaderStage::Fragment: return "fragment";
  case ShaderStage::Compute:  return "compute";
  default:
    log::error("invalid shader stage (in to_string internal debugging method)");
  }
}

}