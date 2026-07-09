#pragma once

#include "internal/shader_manager.hpp"

#include <vulkan/vulkan_raii.hpp>
#include <shaderc/shaderc.hpp>

#include <mutex>

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

class Queue::Impl {
  friend class Queue;

  std::mutex      m_mutex = std::mutex{};
  u32             m_index = VK_QUEUE_FAMILY_IGNORED;
  vkr::Queue      m_queue = nullptr;
  vkr::Semaphore  m_timeline = nullptr;
  u64             m_value = 0;

public:
  Impl(const vkr::Device&, u32);
  ~Impl() = default;

  Impl(const Impl&) = delete;
  Impl(Impl&&) = delete;
  Impl& operator=(const Impl&) = delete;
  Impl& operator=(Impl&&) = delete;
};

class Device::Impl {
  friend class Device;
  friend class Swapchain;

  using PDOutput = std::tuple<QueueFamilies, u32, str>;

  vkr::Context        m_context;
  vkr::Instance       m_instance = nullptr;
  vkr::PhysicalDevice m_physicalDevice = nullptr;
  vkr::Device         m_device = nullptr;
  Queue               m_graphicsQueue;
  Queue               m_computeQueue;
  Queue               m_transferQueue;

  ShaderManager       m_shaderManager;

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