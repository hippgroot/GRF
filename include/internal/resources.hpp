#pragma once

#include "grf/types.hpp"

#include <vulkan/vulkan_raii.hpp>

typedef struct VmaAllocator_T* VmaAllocator;

namespace vkr = vk::raii;

namespace grf {

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
};

}