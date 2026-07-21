#include "internal/log.hpp"
#include "internal/resources.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

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

Image::Data Image::getDataFromFile(const std::filesystem::path& path, bool srgb) {
  i32 width, height, channels;
  Format format = Format::undefined;
  std::byte* bytes = nullptr;
  std::size_t size = 0;

  if (stbi_is_hdr(path.string().c_str())) {
    bytes = (std::byte*)stbi_loadf(path.string().c_str(), &width, &height, &channels, 4);
    format = Format::rgba32_sfloat;
    size = sizeof(f32) * 4 * width * height;
  }
  else if (stbi_is_16_bit(path.string().c_str())) {
    bytes = (std::byte*)stbi_load_16(path.string().c_str(), &width, &height, &channels, 4);
    format = Format::rgba16_unorm;
    size = sizeof(u16) * 4 * width * height;
  }
  else {
    bytes = (std::byte*)stbi_load(path.string().c_str(), &width, &height, &channels, 4);
    format = srgb ? Format::rgba8_srgb : Format::rgba8_unorm;
    size = sizeof(u8) * 4 * width * height;
  }

  if (!bytes)
    log::error("Failed to read {}: {}", path.string(), stbi_failure_reason());

  Image::Data data{
    .width  = static_cast<u32>(width),
    .height = static_cast<u32>(height),
    .format = format,
    .bytes  = std::vector<std::byte>(bytes, bytes + size)
  };

  stbi_image_free(bytes);

  return data;
}

u32 Image::width() const {
  return m_impl->m_width;
}

u32 Image::height() const {
  return m_impl->m_height;
}

u32 Image::depth() const {
  return m_impl->m_depth;
}

Image::Impl::Impl(const Info& info)
: m_allocator(info.allocator),
  m_allocation(info.allocation),
  m_image(info.image),
  m_width(info.width),
  m_height(info.height),
  m_depth(info.depth),
  m_mips(info.mips),
  m_format(info.format)
{
  bool isDepth = m_format >= vk::Format::eD16Unorm && m_format <= vk::Format::eD32SfloatS8Uint;
  m_view = info.device.createImageView(vk::ImageViewCreateInfo{
    .image     = m_image,
    .viewType = info.depth > 1 ? vk::ImageViewType::e3D : vk::ImageViewType::e2D,
    .format   = m_format,
    .subresourceRange = {
      .aspectMask     = isDepth ? vk::ImageAspectFlagBits::eDepth : vk::ImageAspectFlagBits::eColor,
      .baseMipLevel   = 0,
      .levelCount     = m_mips,
      .baseArrayLayer = 0,
      .layerCount     = 1,
    }
  });
}

Image::Impl::~Impl() {
  vmaDestroyImage(m_allocator, m_image, m_allocation);
}

void Image::Impl::setResourceManager(ResourceManager* resourceManager) {
  m_resourceManager = resourceManager;
}

VkImage Image::Impl::image() const {
  return m_image;
}

vk::ImageView Image::Impl::view() const {
  return *m_view;
}

u32 Image::Impl::mips() const {
  return m_mips;
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

Image Allocator::allocateImage(const vkr::Device& device, const Image::Info& info, const std::set<u32>& uniqueIndices) const {
  std::vector<u32> indices(uniqueIndices.begin(), uniqueIndices.end());
  u32 mips = info.allocateMips ? static_cast<u32>(std::floor(std::log2(std::max({info.width, info.height, info.depth})))) + 1u : 1u;

  vk::ImageCreateInfo imageCreateInfo{
    .imageType              = info.depth > 1 ? vk::ImageType::e3D : vk::ImageType::e2D,
    .format                 = static_cast<vk::Format>(info.format),
    .extent                 = { info.width, info.height, info.depth },
    .mipLevels              = mips,
    .arrayLayers            = 1,
    .samples                = vk::SampleCountFlagBits::e1,
    .tiling                 = vk::ImageTiling::eOptimal,
    .usage                  = vk::ImageUsageFlags(static_cast<u32>(info.usage)) | vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst,
    .sharingMode            = indices.size() == 1 ? vk::SharingMode::eExclusive : vk::SharingMode::eConcurrent,
    .queueFamilyIndexCount  = static_cast<u32>(indices.size()),
    .pQueueFamilyIndices    = indices.data()
  };

  VmaAllocationCreateInfo allocCreateInfo{
    .usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE
  };

  VkImage image;
  VmaAllocation allocation;
  vk::Result res = vk::Result(vmaCreateImage(
    m_allocator,
    reinterpret_cast<VkImageCreateInfo*>(&imageCreateInfo),
    &allocCreateInfo,
    &image,
    &allocation,
    nullptr
  ));
  if (res != vk::Result::eSuccess)
    log::error("Failed to create image: {}", vk::to_string(res));

  Image img;
  img.m_impl = std::make_shared<Image::Impl>(Image::Impl::Info{
    .device     = device,
    .allocator  = m_allocator,
    .allocation = allocation,
    .image      = image,
    .width      = info.width,
    .height     = info.height,
    .depth      = info.depth,
    .mips       = mips,
    .format     = imageCreateInfo.format
  });

  return img;
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