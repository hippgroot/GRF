#pragma once

#include "grf/resources.hpp"
#include "grf/types.hpp"
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

class Image::Impl {
  friend class Image;

public:
  struct Info {
    const vkr::Device&  device;
    VmaAllocator        allocator;
    VmaAllocation       allocation;
    VkImage             image;
    u32                 width;
    u32                 height;
    u32                 depth;
    u32                 mips;
    vk::Format          format;
  };

private:
  ResourceManager*  m_resourceManager = nullptr;
  VmaAllocator      m_allocator = nullptr;
  VmaAllocation     m_allocation  = nullptr;
  VkImage           m_image = nullptr;
  vkr::ImageView    m_view = nullptr;
  u32               m_width;
  u32               m_height;
  u32               m_depth;
  u32               m_mips;
  vk::Format        m_format;
  vk::ImageLayout   m_layout = vk::ImageLayout::eUndefined;

public:
  Impl(const Info&);
  ~Impl();

  Impl(const Impl&) = delete;
  Impl(Impl&&) = delete;
  Impl& operator=(const Impl&) = delete;
  Impl& operator=(Impl&&) = delete;

  void setResourceManager(ResourceManager*);
  void setLayout(vk::ImageLayout);

  VkImage image() const;
  vk::ImageView view() const;
  u32 mips() const;
  vk::ImageLayout layout() const;
  vk::Format format() const;
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
  Image allocateImage(const vkr::Device&, const Image::Info&, const std::set<u32>&) const;
};

class ResourceManager {
  struct BufferStagingInfo {
    Buffer  src;
    Buffer  dst;
    u32     srcOffset;
    u32     dstOffset;
    u32     size;
  };

  struct ImageStagingInfo {
    Buffer  src;
    Image   dst;
    u32     offset;
    Layout  layout;
    u32     mip = 0;
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
  std::vector<BufferStagingInfo>  m_stagedBuffers;
  std::vector<ImageStagingInfo>   m_stagedImages;
  bool                            m_shouldWait = true;

public:
  ResourceManager() = default;
  ~ResourceManager() = default;

  ResourceManager(const ResourceManager&) = delete;
  ResourceManager(ResourceManager&&) = delete;
  ResourceManager& operator=(const ResourceManager&) = delete;
  ResourceManager& operator=(ResourceManager&&) = delete;

  void init(vkr::Device* p_device, Queue queue, Ring<Buffer> cmdRing, u32 stagingCount);
  void stage(Buffer buffer, std::span<const std::byte> data, u32 offset);
  void stage(Image image, std::span<const std::byte> bytes, Layout finalLayout, u32 mip);
  Sync flush();
};

}