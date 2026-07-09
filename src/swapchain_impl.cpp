#include "grf/window.hpp"
#include "internal/device.hpp"
#include "internal/log.hpp"
#include "internal/swapchain.hpp"

#ifndef GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_NONE
#endif

#include "GLFW/glfw3.h"

#include <limits>

namespace grf {

Swapchain::Impl::Impl(const Device& device, Window& window, const Swapchain::Info& info)
: m_physicalDevice(device.m_impl->m_physicalDevice),
  m_device(device.m_impl->m_device),
  m_window(&window)
{
  createSurface(device.m_impl->m_instance);
  auto capabilities = processInfo(info);
  createSwapchain(capabilities);
  createViews();

  m_window->m_registeredSwapchains.emplace(reinterpret_cast<std::uintptr_t>(this));
}

Swapchain::Impl::~Impl() {
  if (m_window)
    m_window->m_registeredSwapchains.erase(reinterpret_cast<std::uintptr_t>(this));
}

void Swapchain::Impl::recreate(u32 width, u32 height) {
  m_device.waitIdle();

  m_views.clear();
  m_images.clear();

  auto capabilities = m_physicalDevice.getSurfaceCapabilitiesKHR(*m_surface);

  m_extent.width = std::clamp(width,
    capabilities.minImageExtent.width, capabilities.maxImageExtent.width
  );
  m_extent.height = std::clamp(height,
    capabilities.minImageExtent.height, capabilities.maxImageExtent.height
  );

  createSwapchain(capabilities);
  createViews();
}

void Swapchain::Impl::unregisterWindow() {
  m_window = nullptr;
}

void Swapchain::Impl::createSurface(const vkr::Instance& instance) {
  vk::SurfaceKHR::CType cSurface;
  if (glfwCreateWindowSurface(*instance, m_window->m_window, nullptr, &cSurface) != VK_SUCCESS)
    log::panic("Failed to create window surface");

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
  m_presentMode = vk::PresentModeKHR::eFifo;
  if (!info.vsync) {
    for (const auto& mode : m_physicalDevice.getSurfacePresentModesKHR(*m_surface)) {
      if (mode == vk::PresentModeKHR::eMailbox) m_presentMode = mode;
    }
    if (m_presentMode != vk::PresentModeKHR::eMailbox)
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
  m_images = m_swapchain.getImages();
}

void Swapchain::Impl::createViews() {
  m_views.reserve(m_images.size());
  for (const auto& image : m_images) {
    m_views.emplace_back(m_device.createImageView(vk::ImageViewCreateInfo{
      .image = image,
      .viewType = vk::ImageViewType::e2D,
      .format   = m_surfaceFormat.format,
      .subresourceRange = {
        .aspectMask = vk::ImageAspectFlagBits::eColor,
        .levelCount = 1,
        .layerCount = 1
      }
    }));
  }
}

}