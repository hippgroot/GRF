#pragma once

#include "grf/resources.hpp"
#include "internal/queue.hpp"

#include <vulkan/vulkan_raii.hpp>

#include <set>

typedef struct VmaAllocator_T* VmaAllocator;
typedef struct VmaAllocation_T* VmaAllocation;

namespace vkr = vk::raii;

namespace grf {

class ResourceManager;

class Buffer::Impl {
  friend class Buffer;

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
  ResourceManager*  m_resourceManager = nullptr;
  VmaAllocator      m_allocator = nullptr;
  VmaAllocation     m_allocation = nullptr;
  VkBuffer          m_buffer = nullptr;
  void*             m_map = nullptr;
  u32               m_size = 0;
  bool              m_readable = false;

public:
  explicit Impl(const Info&);
  ~Impl();

  Impl(const Impl&) = delete;
  Impl(Impl&&) = delete;
  Impl& operator=(const Impl&) = delete;
  Impl& operator=(Impl&&) = delete;

  void setResourceManager(ResourceManager*);

  VkBuffer buffer() const;
  void* map() const;
  u32 size() const;
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
  Buffer allocateBuffer(const Buffer::Info&, const std::set<u32>&) const;
};

class ResourceManager {
  struct BufferStagingInfo {
    Buffer  src;
    Buffer  dst;
    u32     srcOffset;
    u32     dstOffset;
    u32     size;
  };

  vkr::Device*                    m_device;
  Queue                           m_queue;
  Ring<CommandBuffer>             m_cmdRing;
  Ring<Sync>                      m_syncRing;
  Ring<Buffer>                    m_bufferRing;
  Sync*                           m_currSync;
  Sync                            m_latestSync;
  Buffer                          m_buffer;
  u32                             m_offset = 0;
  std::vector<BufferStagingInfo>  m_staged;
  bool                            m_shouldWait = true;

public:
  ResourceManager() = default;
  ~ResourceManager() = default;

  ResourceManager(const ResourceManager&) = delete;
  ResourceManager(ResourceManager&&) = delete;
  ResourceManager& operator=(const ResourceManager&) = delete;
  ResourceManager& operator=(ResourceManager&&) = delete;

  void init(vkr::Device*, Queue, Ring<Buffer>, u32);
  void stage(Buffer, std::span<const std::byte>, u32 offset);
  Sync flush();
};

}