#pragma once

#include "grf/device.hpp"
#include "internal/resources.hpp"
#include "internal/pipeline.hpp"

#include <chrono>

namespace vkr = vk::raii;
namespace chr = std::chrono;

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
  friend class Device;
  friend class Swapchain;

  using PDOutput = std::tuple<QueueFamilies, u32, str>;
  using Clock = chr::high_resolution_clock;
  using TimePoint = Clock::time_point;

  vkr::Context              m_context;
  vkr::Instance             m_instance = nullptr;
  vkr::PhysicalDevice       m_physicalDevice = nullptr;
  vkr::Device               m_device = nullptr;
  Queue                     m_graphicsQueue;
  Queue                     m_computeQueue;
  Queue                     m_transferQueue;
  std::set<u32>             m_uniqueIndices;
  vkr::PipelineLayout       m_pipelineLayout = nullptr;

  Allocator                 m_allocator;
  ResourceManager           m_resourceManager;
  DescriptorHeap            m_descriptorHeap;
  ShaderManager             m_shaderManager;

  TimePoint                 m_frameStart;

public:
  explicit Impl(const Device::Info& info);
  ~Impl() = default;

  Impl(const Impl&) = delete;
  Impl(Impl&&) = delete;
  Impl& operator=(const Impl&) = delete;
  Impl& operator=(Impl&&) = delete;

private:
  bool createInstance(const Device::Info& info);
  PDOutput choosePhysicalDevice();
  void createDevice(const QueueFamilies& queueFamilies, bool queryPortability);
  void createLayouts();
};

}