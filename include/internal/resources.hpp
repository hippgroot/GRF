#pragma once

#include "grf/resources.hpp"
#include "grf/types.hpp"

#include <vulkan/vulkan_raii.hpp>

typedef struct VmaAllocator_T* VmaAllocator;
typedef struct VmaAllocation_T* VmaAllocation;

namespace vkr = vk::raii;

namespace grf {

class Buffer::Impl {
public:
  struct Info {
    VmaAllocator  allocator;
    VmaAllocation allocation;
    VkBuffer      buffer;
    void*         map;
    u32           size;
    bool          readable;
  };

private:
  VmaAllocator  m_allocator = nullptr;
  VmaAllocation m_allocation = nullptr;
  VkBuffer      m_buffer = nullptr;
  void*         m_map = nullptr;
  u32           m_size = 0;
  bool          m_readable = false;

public:
  explicit Impl(const Info&);
  ~Impl();

  Impl(const Impl&) = delete;
  Impl(Impl&&) = delete;
  Impl& operator=(const Impl&) = delete;
  Impl& operator=(Impl&&) = delete;
};

class Allocator {
public:
  struct Info {
    const vkr::Instance&        instance;
    const vkr::PhysicalDevice&  physicalDevice;
    const vkr::Device&          device;
    u32                         apiVersion;
  };

private:
  VmaAllocator m_allocator = nullptr;

public:
  Allocator() = default;
  ~Allocator();

  Allocator(const Allocator&) = delete;
  Allocator(Allocator&&) = delete;
  Allocator& operator=(const Allocator&) = delete;
  Allocator& operator=(Allocator&&) = delete;

  void init(const Info&);
  Buffer allocateBuffer(const Buffer::Info&) const;
};

}