#include "grf/window.hpp"
#include "internal/device.hpp"
#include "internal/log.hpp"
#include "internal/queue.hpp"
#include "internal/swapchain.hpp"

#ifndef GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_NONE
#endif

#include "GLFW/glfw3.h"

#include <limits>

namespace grf {

Format SwapchainImage::format() const {
  return m_impl->m_format;
}

u32 SwapchainImage::width() const {
  return m_impl->m_width;
}

u32 SwapchainImage::height() const {
  return m_impl->m_height;
}

SwapchainImage::Impl::Impl(const vkr::Device& device, const Info& info)
: m_swapchain(info.swapchain),
  m_image(info.image),
  m_index(info.index),
  m_format(static_cast<Format>(info.format)),
  m_width(info.width),
  m_height(info.height)
{
  m_view = device.createImageView(vk::ImageViewCreateInfo{
    .image    = m_image,
    .viewType = vk::ImageViewType::e2D,
    .format   = info.format,
    .subresourceRange = {
      .aspectMask = vk::ImageAspectFlagBits::eColor,
      .levelCount = 1,
      .layerCount = 1
    }
  });
  m_semaphore = device.createSemaphore({});
}

void SwapchainImage::Impl::setLayout(vk::ImageLayout layout) {
  m_layout = layout;
}

vk::SwapchainKHR SwapchainImage::Impl::swapchain() const {
  return m_swapchain;
}

vk::Image SwapchainImage::Impl::image() const {
  return m_image;
}

vk::ImageView SwapchainImage::Impl::view() const {
  return *m_view;
}

vk::Semaphore SwapchainImage::Impl::semaphore() const {
  return *m_semaphore;
}

u32 SwapchainImage::Impl::index() const {
  return m_index;
}

vk::ImageLayout SwapchainImage::Impl::layout() const {
  return m_layout;
}

Swapchain::Swapchain(const Device& device, Window& window, const Swapchain::Info& info)
: m_impl(std::make_unique<Swapchain::Impl>(device, window, info))
{}

Swapchain::~Swapchain() {}

Format Swapchain::format() const {
  return static_cast<Format>(m_impl->m_surfaceFormat.format);
}

ColorSpace Swapchain::colorspace() const {
  return static_cast<ColorSpace>(m_impl->m_surfaceFormat.colorSpace);
}

bool Swapchain::vsync() const {
  return m_impl->m_presentMode == vk::PresentModeKHR::eFifo;
}

bool Swapchain::vsyncOnly() const {
  return !m_impl->m_mailboxSupported;
}

void Swapchain::enableVsync() {
  if (m_impl->m_presentMode == vk::PresentModeKHR::eFifo)
    return;

  m_impl->m_presentMode = vk::PresentModeKHR::eFifo;
  m_impl->m_dirty = true;
}

void Swapchain::disableVsync() {
  if (!m_impl->m_mailboxSupported) {
    log::warning("Uncapped presentation is unsupported. Using VSync");
    return;
  }
  if (m_impl->m_presentMode == vk::PresentModeKHR::eMailbox)
    return;

  m_impl->m_presentMode = vk::PresentModeKHR::eMailbox;
  m_impl->m_dirty = true;
}

std::pair<SwapchainImage, Sync> Swapchain::acquireNextImage(u64 timeout) const {
  if (m_impl->m_dirty) {
    i32 width, height;
    glfwGetFramebufferSize(m_impl->m_window->m_window, &width, &height);
    m_impl->recreate(width, height);
    m_impl->m_dirty = false;
  }

  vk::Semaphore semaphore = *m_impl->m_semaphores[m_impl->m_semaphoreIndex];
  m_impl->m_semaphoreIndex = (m_impl->m_semaphoreIndex + 1) % m_impl->m_semaphores.size();

  auto [res, index] = m_impl->m_swapchain.acquireNextImage(timeout, semaphore);
  if (res == vk::Result::eSuboptimalKHR)
    m_impl->m_dirty = true;

  Sync sync;
  sync.m_impl = std::make_shared<Sync::Impl>(semaphore);

  return std::make_pair(m_impl->m_images[index], sync);
}

Swapchain::Impl::Impl(const Device& device, Window& window, const Swapchain::Info& info)
: m_physicalDevice(device.m_impl->m_physicalDevice),
  m_device(device.m_impl->m_device),
  m_window(&window)
{
  createSurface(device.m_impl->m_instance);
  auto capabilities = processInfo(info);
  createSwapchain(capabilities);
  createImages();

  for (u32 i = 0; i < m_images.size(); ++i)
    m_semaphores.emplace_back(m_device.createSemaphore({}));

  m_window->m_registeredSwapchains.emplace(reinterpret_cast<std::uintptr_t>(this));
}

Swapchain::Impl::~Impl() {
  if (m_window)
    m_window->m_registeredSwapchains.erase(reinterpret_cast<std::uintptr_t>(this));
}

void Swapchain::Impl::recreate(u32 width, u32 height) {
  m_device.waitIdle();
  m_images.clear();

  auto capabilities = m_physicalDevice.getSurfaceCapabilitiesKHR(*m_surface);

  m_extent.width = std::clamp(width,
    capabilities.minImageExtent.width, capabilities.maxImageExtent.width
  );
  m_extent.height = std::clamp(height,
    capabilities.minImageExtent.height, capabilities.maxImageExtent.height
  );

  createSwapchain(capabilities);
  createImages();
}

void Swapchain::Impl::unregisterWindow() {
  m_window = nullptr;
}

void Swapchain::Impl::createSurface(const vkr::Instance& instance) {
  vk::SurfaceKHR::CType cSurface;
  if (glfwCreateWindowSurface(*instance, m_window->m_window, nullptr, &cSurface) != VK_SUCCESS)
    log::error("Failed to create window surface");

  m_surface = vkr::SurfaceKHR(instance, cSurface);
}

vk::SurfaceCapabilitiesKHR Swapchain::Impl::processInfo(const Swapchain::Info& info) {
  vk::Format desiredFormat = static_cast<vk::Format>(info.format);
  vk::ColorSpaceKHR desiredColorspace = static_cast<vk::ColorSpaceKHR>(info.colorspace);

  auto surfaceFormats = m_physicalDevice.getSurfaceFormatsKHR(*m_surface);
  m_surfaceFormat = surfaceFormats[0];
  for (const auto& surfaceFormat : surfaceFormats) {
    if (surfaceFormat.format == desiredFormat && surfaceFormat.colorSpace == desiredColorspace) {
      m_surfaceFormat = vk::SurfaceFormatKHR{
        .format     = desiredFormat,
        .colorSpace = desiredColorspace
      };
      break;
    }
  }
  if (m_surfaceFormat.format != desiredFormat || m_surfaceFormat.colorSpace != desiredColorspace)
    log::warning("Desired format and colorspace unsupported. Using defaults.");

  auto capabilities = m_physicalDevice.getSurfaceCapabilitiesKHR(*m_surface);

  for (const auto& mode : m_physicalDevice.getSurfacePresentModesKHR(*m_surface)) {
    if (mode == vk::PresentModeKHR::eMailbox) m_mailboxSupported = true;
  }

  m_presentMode = vk::PresentModeKHR::eFifo;
  if (!info.vsync) {
    if (m_mailboxSupported)
      m_presentMode = vk::PresentModeKHR::eMailbox;
    else
      log::warning("Uncapped presentation is unsupported. Using VSync");
  }

  i32 windowWidth, windowHeight;
  glfwGetFramebufferSize(m_window->m_window, &windowWidth, &windowHeight);

  m_extent = capabilities.currentExtent;
  if (m_extent.width == std::numeric_limits<u32>::max()) {
     m_extent.width = std::clamp(static_cast<u32>(windowWidth),
       capabilities.minImageExtent.width, capabilities.maxImageExtent.width
     );
     m_extent.height = std::clamp(static_cast<u32>(windowHeight),
       capabilities.minImageExtent.height, capabilities.maxImageExtent.height
     );
  }

  m_minImageCount = capabilities.minImageCount + 1;
  if (capabilities.maxImageCount > 0 && m_minImageCount > capabilities.maxImageCount)
    m_minImageCount = capabilities.maxImageCount;

  return capabilities;
}

void Swapchain::Impl::createSwapchain(const vk::SurfaceCapabilitiesKHR& capabilities) {
  m_swapchain = m_device.createSwapchainKHR(vk::SwapchainCreateInfoKHR{
    .surface          = *m_surface,
    .minImageCount    = m_minImageCount,
    .imageFormat      = m_surfaceFormat.format,
    .imageColorSpace  = m_surfaceFormat.colorSpace,
    .imageExtent      = m_extent,
    .imageArrayLayers = 1,
    .imageUsage       = vk::ImageUsageFlagBits::eColorAttachment,
    .preTransform     = capabilities.currentTransform,
    .presentMode      = m_presentMode,
    .clipped          = true,
    .oldSwapchain     = *m_swapchain
  });
}

void Swapchain::Impl::createImages() {
  std::vector<vk::Image> images = m_swapchain.getImages();
  m_images.reserve(images.size());

  for (u32 index = 0; auto image : images) {
    SwapchainImage scImg;
    scImg.m_impl = std::make_shared<SwapchainImage::Impl>(m_device, SwapchainImage::Impl::Info{
      .swapchain = *m_swapchain,
      .image      = image,
      .index      = index++,
      .format     = m_surfaceFormat.format,
      .width      = m_extent.width,
      .height     = m_extent.height
    });
    scImg.m_impl->flagSwapchainForRecreation = [this](){ m_dirty = true; };

    m_images.emplace_back(std::move(scImg));
  }
}

}