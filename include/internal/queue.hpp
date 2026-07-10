#pragma once

#include "grf/queue.hpp"

#include <vulkan/vulkan_raii.hpp>

#include <mutex>

namespace vkr = vk::raii;

namespace grf {

enum class SyncType {
  Timeline,
  Binary
};

class Sync::Impl {
  friend class Sync;

  vk::Semaphore m_semaphore = nullptr;
  u64           m_value = 0;
  SyncType      m_type = SyncType::Timeline;

public:
  explicit Impl(vk::Semaphore);
  Impl(vk::Semaphore, u64);
  ~Impl() = default;

  Impl(const Impl&) = delete;
  Impl(Impl&&) = delete;
  Impl& operator=(const Impl&) = delete;
  Impl& operator=(Impl&&) = delete;

  vk::Semaphore semaphore() const;
  u64 value() const;
  SyncType type() const;
};

class CommandBuffer::Impl {
  friend class CommandBuffer;

  vkr::CommandBuffer m_cmd = nullptr;

public:
  explicit Impl(vkr::CommandBuffer&&);
  ~Impl() = default;

  Impl(const Impl&) = delete;
  Impl(Impl&&) = delete;
  Impl& operator=(const Impl&) = delete;
  Impl& operator=(Impl&&) = delete;

  const vkr::CommandBuffer& commandBuffer() const;
};

class Queue::Impl {
  friend class Queue;

  const vkr::Device&  m_device;

  std::mutex          m_mutex = std::mutex{};
  u32                 m_index = VK_QUEUE_FAMILY_IGNORED;
  vkr::Queue          m_queue = nullptr;
  vkr::Semaphore      m_timeline = nullptr;
  vkr::CommandPool    m_cmdPool = nullptr;
  u64                 m_value = 0;

public:
  Impl(const vkr::Device&, u32);
  ~Impl() = default;

  Impl(const Impl&) = delete;
  Impl(Impl&&) = delete;
  Impl& operator=(const Impl&) = delete;
  Impl& operator=(Impl&&) = delete;
};

}