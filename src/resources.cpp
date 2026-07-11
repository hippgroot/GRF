#include "internal/log.hpp"
#include "vulkan/vulkan_hpp_macros.hpp"
#include "internal/resources.hpp"

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

namespace grf {

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

}