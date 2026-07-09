#pragma once

#include "grf/device.hpp"

#include <vulkan/vulkan_raii.hpp>

namespace vkr = vk::raii;

namespace grf {

class QueueFamilies {
  u32 m_graphics = VK_QUEUE_FAMILY_IGNORED;
  u32 m_compute = VK_QUEUE_FAMILY_IGNORED;
  u32 m_transfer = VK_QUEUE_FAMILY_IGNORED;

public:
  explicit QueueFamilies(const vkr::PhysicalDevice&);

  bool hasGraphics() const;
  bool hasCompute() const;
  bool hasTransfer() const;
  bool invalid() const;

  u32 graphics() const;
  u32 compute() const;
  u32 transfer() const;
};

class Device::Impl {
  friend class Swapchain;

  using PDOutput = std::tuple<QueueFamilies, u32, str>;

  vkr::Context        m_context;
  vkr::Instance       m_instance = nullptr;
  vkr::PhysicalDevice m_physicalDevice = nullptr;
  vkr::Device         m_device = nullptr;

public:
  explicit Impl(const Device::Info&);
  ~Impl() = default;

  Impl(const Impl&) = delete;
  Impl(Impl&&) = delete;
  Impl& operator=(const Impl&) = delete;
  Impl& operator=(Impl&&) = delete;

private:
  bool createInstance(const Device::Info&);
  PDOutput choosePhysicalDevice();
  void createDevice(const QueueFamilies&, bool);
};

}