#pragma once

#include "grf/swapchain.hpp"

#include <vulkan/vulkan_raii.hpp>

namespace vkr = vk::raii;

class GLFWwindow;

namespace grf {

class Swapchain::Impl {
  const vkr::PhysicalDevice&  m_physicalDevice;
  const vkr::Device&          m_device;
  Window*                     m_window;

  vkr::SurfaceKHR             m_surface = nullptr;
  vkr::SwapchainKHR           m_swapchain = nullptr;
  std::vector<vk::Image>      m_images;
  std::vector<vkr::ImageView> m_views;
  vk::SurfaceFormatKHR        m_surfaceFormat;
  vk::PresentModeKHR          m_presentMode;
  vk::Extent2D                m_extent;
  u32                         m_minImageCount;

public:
  Impl(const Device&, Window&, const Swapchain::Info&);
  ~Impl();

  Impl(const Impl&) = delete;
  Impl(Impl&&) = delete;
  Impl& operator=(const Impl&) = delete;
  Impl& operator=(Impl&&) = delete;

  void recreate(u32, u32);
  void unregisterWindow();

private:
  void createSurface(const vkr::Instance&);
  vk::SurfaceCapabilitiesKHR processInfo(const Swapchain::Info&);
  void createSwapchain(const vk::SurfaceCapabilitiesKHR&);
  void createViews();
};

}