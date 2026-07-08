#pragma once

#include "grf/grf.hpp"

#include <vulkan/vulkan_raii.hpp>

namespace vkr = vk::raii;

namespace grf {

class Device::Impl {
  vkr::Context  m_context;
  vkr::Instance m_instance = nullptr;

public:
  explicit Impl(const Device::Info&);
  ~Impl() = default;

  Impl(const Impl&) = delete;
  Impl(Impl&&) = delete;
  Impl& operator=(const Impl&) = delete;
  Impl& operator=(Impl&&) = delete;

private:
  void createInstance(const Device::Info&);
};

}