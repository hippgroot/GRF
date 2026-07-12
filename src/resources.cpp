#include "internal/log.hpp"
#include "vulkan/vulkan_hpp_macros.hpp"
#include "internal/resources.hpp"

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

namespace grf {

Buffer::Impl::Impl(const Info& info)
: m_allocator(info.allocator),
  m_allocation(info.allocation),
  m_buffer(info.buffer),
  m_size(info.size)
{}

Buffer::Impl::~Impl() {
  vmaDestroyBuffer(m_allocator, m_buffer, m_allocation);
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

Buffer Allocator::allocateBuffer(const Buffer::Info& info) const {
  vk::BufferCreateInfo bufferCreateInfo{
    .size   = info.size,
    .usage  = vk::BufferUsageFlagBits::eStorageBuffer   |
              vk::BufferUsageFlagBits::eIndexBuffer     |
              vk::BufferUsageFlagBits::eIndirectBuffer  |
              vk::BufferUsageFlagBits::eTransferSrc     |
              vk::BufferUsageFlagBits::eTransferDst
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

}