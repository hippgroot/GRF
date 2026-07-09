#pragma once

#include "grf/grf.hpp"

#include <vulkan/vulkan_raii.hpp>

namespace vkr = vk::raii;

namespace grf {

class QueueFamilies {
  u8 m_graphics = 0xFF;
  u8 m_compute = 0xFF;
  u8 m_transfer = 0xFF;

public:
  explicit QueueFamilies(const vkr::PhysicalDevice&);

  bool hasGraphics() const;
  bool hasCompute() const;
  bool hasTransfer() const;
  bool invalid() const;

  u8 graphics() const;
  u8 compute() const;
  u8 transfer() const;
};

class Device::Impl {
  vkr::Context        m_context;
  vkr::Instance       m_instance = nullptr;
  vkr::PhysicalDevice m_physicalDevice = nullptr;

public:
  explicit Impl(const Device::Info&);
  ~Impl() = default;

  Impl(const Impl&) = delete;
  Impl(Impl&&) = delete;
  Impl& operator=(const Impl&) = delete;
  Impl& operator=(Impl&&) = delete;

private:
  bool createInstance(const Device::Info&);
  std::tuple<QueueFamilies, u32, str> choosePhysicalDevice();
};

}