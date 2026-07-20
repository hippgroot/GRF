#include "internal/log.hpp"
#include "internal/resources.hpp"

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

namespace grf {

void Buffer::writeData(std::span<const std::byte> data, u32 offset) const {
  if (data.size() > m_impl->m_size) {
    log::warning(
      "Data size ({}) being written is greater than buffer size ({}). Skipping write",
      data.size(), m_impl->m_size
    );
    return;
  }

  if (data.size() + offset > m_impl->m_size) {
    log::warning(
      "Data (size {}) being written to offset ({}) overflows buffer. Skipping write.",
      data.size(), offset
    );
    return;
  }

  if (m_impl->m_map) {
    std::memcpy((std::byte*)m_impl->m_map + offset, data.data(), data.size());
    return;
  }

  m_impl->m_resourceManager->stage(*this, data, offset);
}

void Buffer::readData(std::span<std::byte> data, u32 offset) const {
  if (!m_impl->m_readable || !m_impl->m_map) {
    log::warning("Buffer readback initiated on unreadable buffer. Make sure to create buffer with BufferIntent::Readable.");
    return;
  }

  if (data.size() + offset > m_impl->m_size) {
    log::warning("Readback size at offset ({} + {} = {}) exceeds buffer size ({}). Readback will produce no results.",
      data.size(),
      offset,
      data.size() + offset,
      m_impl->m_size
    );
    return;
  }

  std::memcpy(data.data(), (std::byte*)m_impl->m_map + offset, data.size());
}

Buffer::Impl::Impl(const Info& info)
: m_allocator(info.allocator),
  m_allocation(info.allocation),
  m_buffer(info.buffer),
  m_map(info.map),
  m_size(info.size),
  m_readable(info.readable)
{}

Buffer::Impl::~Impl() {
  vmaDestroyBuffer(m_allocator, m_buffer, m_allocation);
}

void Buffer::Impl::setResourceManager(ResourceManager* resourceManager) {
  m_resourceManager = resourceManager;
}

VkBuffer Buffer::Impl::buffer() const {
  return m_buffer;
}

void* Buffer::Impl::map() const {
  return m_map;
}

u32 Buffer::Impl::size() const {
  return m_size;
}

Allocator::~Allocator() {
  vmaDestroyAllocator(m_allocator);
}

void Allocator::init(const Info& info) {
  VmaVulkanFunctions functions{
    .vkGetInstanceProcAddr = VULKAN_HPP_DEFAULT_DISPATCHER.vkGetInstanceProcAddr,
    .vkGetDeviceProcAddr = VULKAN_HPP_DEFAULT_DISPATCHER.vkGetDeviceProcAddr
  };

  VmaAllocatorCreateInfo createInfo{
    .physicalDevice   = *info.physicalDevice,
    .device           = *info.device,
    .pVulkanFunctions = &functions,
    .instance         = *info.instance,
    .vulkanApiVersion = info.apiVersion
  };

  vk::Result res = vk::Result(vmaCreateAllocator(&createInfo, &m_allocator));
  if (res != vk::Result::eSuccess)
    log::error("Failed to create allocator: {}", vk::to_string(res));
}

Buffer Allocator::allocateBuffer(const Buffer::Info& info, const std::set<u32>& uniqueIndices) const {
  std::vector<u32> indices(uniqueIndices.begin(), uniqueIndices.end());

  vk::BufferCreateInfo bufferCreateInfo{
    .size   = info.size,
    .usage  = vk::BufferUsageFlagBits::eStorageBuffer   |
              vk::BufferUsageFlagBits::eIndexBuffer     |
              vk::BufferUsageFlagBits::eIndirectBuffer  |
              vk::BufferUsageFlagBits::eTransferSrc     |
              vk::BufferUsageFlagBits::eTransferDst,
    .sharingMode = indices.size() == 1 ? vk::SharingMode::eExclusive : vk::SharingMode::eConcurrent,
    .queueFamilyIndexCount  = static_cast<u32>(indices.size()),
    .pQueueFamilyIndices    = indices.data()
  };

  VmaAllocationCreateInfo allocCreateInfo{};
  switch(info.intent) {
  case BufferIntent::GPUOnly:
    allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
    break;
  case BufferIntent::FrequentUpdate:
    allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
    allocCreateInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                      VMA_ALLOCATION_CREATE_MAPPED_BIT;
    break;
  case BufferIntent::Readable:
    allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
    allocCreateInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT |
                            VMA_ALLOCATION_CREATE_MAPPED_BIT;
  }

  VkBuffer buffer;
  VmaAllocation allocation;
  VmaAllocationInfo allocInfo{};
  vk::Result res = vk::Result(vmaCreateBuffer(
    m_allocator,
    reinterpret_cast<VkBufferCreateInfo*>(&bufferCreateInfo),
    &allocCreateInfo,
    &buffer,
    &allocation,
    &allocInfo
  ));

  if (res != vk::Result::eSuccess)
    log::error("Failed to create buffer: {}", vk::to_string(res));

  Buffer buf;
  buf.m_impl = std::make_shared<Buffer::Impl>(Buffer::Impl::Info{
    .allocator  = m_allocator,
    .allocation = allocation,
    .buffer     = buffer,
    .map        = allocInfo.pMappedData,
    .size       = info.size,
    .readable   = info.intent == BufferIntent::Readable
  });

  return buf;
}

void ResourceManager::init(
  vkr::Device* device, Queue queue, Ring<Buffer> buffers, u32 stagingCount
) {
  m_device = device;
  m_queue = queue;

  m_cmdRing = queue.getCmdRing(stagingCount);

  m_syncRing = queue.getSyncRing(stagingCount);
  m_currSync = &m_syncRing.next();
  m_latestSync = *m_currSync;

  m_bufferRing = buffers;
  m_buffer = m_bufferRing.next();
}

void ResourceManager::stage(Buffer dst, std::span<const std::byte> data, u32 offset) {
  if (data.size() > m_buffer.m_impl->size())
    log::error("Data upload size is greater than staging buffer size. Chunking is not implemented yet so consider increasing the staging buffer size in your Device::Info struct");

  if (data.size() + m_offset > m_buffer.m_impl->size())
    (void)flush();

  if (m_shouldWait) {
    m_currSync = &m_syncRing.next();
    vk::Semaphore semaphore = m_currSync->m_impl->semaphore();
    u64 value = m_currSync->m_impl->value();

    auto res = m_device->waitSemaphores(vk::SemaphoreWaitInfo{
      .semaphoreCount = 1,
      .pSemaphores    = &semaphore,
      .pValues        = &value
    }, std::numeric_limits<u64>::max());

    if (res != vk::Result::eSuccess)
      log::error("Hung waiting for next staging buffer");

    m_shouldWait = false;
  }

  std::byte* map = (std::byte*)m_buffer.m_impl->map();
  std::memcpy(map + m_offset, data.data(), data.size());

  m_staged.emplace_back(BufferStagingInfo{
    .src        = m_buffer,
    .dst        = dst,
    .srcOffset  = m_offset,
    .dstOffset  = offset,
    .size       = static_cast<u32>(data.size())
  });

  m_offset += data.size();
}

Sync ResourceManager::flush() {
  if (m_staged.empty()) return m_latestSync;

  auto& cmd = m_cmdRing.next();
  cmd.begin();

  for (const auto& info : m_staged)
    cmd.copyBuffer(info.src, info.dst, info.srcOffset, info.dstOffset, info.size);

  cmd.end();

  *m_currSync = m_latestSync = m_queue.submit({ cmd });

  m_staged.clear();
  m_buffer = m_bufferRing.next();
  m_offset = 0;
  m_shouldWait = true;

  return m_latestSync;
}

}