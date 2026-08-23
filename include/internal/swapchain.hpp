#pragma once

#include "grf/swapchain.hpp"

#include <vulkan/vulkan_raii.hpp>

#include <atomic>
#include <functional>

namespace vkr = vk::raii;

struct GLFWwindow;

namespace grf {

class SwapchainImage::Impl {
  friend class SwapchainImage;

public:
  struct Info {
    vk::SwapchainKHR  swapchain;
    vk::Image         image;
    u32               index;
    vk::Format        format;
    u32               width;
    u32               height;
  };

  std::function<void()> flagSwapchainForRecreation;

private:
  vk::SwapchainKHR      m_swapchain = nullptr;
  vk::Image             m_image = nullptr;
  vkr::ImageView        m_view = nullptr;
  vkr::Semaphore        m_semaphore = nullptr;
  u32                   m_index = 0;
  Format                m_format = Format::undefined;
  u32                   m_width = 0;
  u32                   m_height = 0;
  vk::ImageLayout       m_layout = vk::ImageLayout::eUndefined;

public:
  Impl(const vkr::Device& device, const Info& info);
  ~Impl() = default;

  Impl(const Impl&) = delete;
  Impl(Impl&&) = delete;
  Impl& operator=(const Impl&) = delete;
  Impl& operator=(Impl&&) = delete;

  void setLayout(vk::ImageLayout layout);

  vk::SwapchainKHR swapchain() const;
  vk::Image image() const;
  vk::ImageView view() const;
  vk::Semaphore semaphore() const;
  u32 index() const;
  vk::ImageLayout layout() const;
};

class Swapchain::Impl {
  friend class GUI;
  friend class Swapchain;

  const vkr::PhysicalDevice&  m_physicalDevice;
  const vkr::Device&          m_device;
  Window*                     m_window;

  vkr::SurfaceKHR             m_surface = nullptr;
  vkr::SwapchainKHR           m_swapchain = nullptr;
  std::vector<SwapchainImage> m_images;
  std::vector<vkr::Semaphore> m_semaphores;
  vk::SurfaceFormatKHR        m_surfaceFormat;
  vk::Extent2D                m_extent;
  u32                         m_minImageCount = 1;
  u32                         m_semaphoreIndex = 0;
  bool                        m_mailboxSupported = false;

  std::atomic<vk::PresentModeKHR> m_presentMode { vk::PresentModeKHR::eFifo };
  std::atomic<bool>               m_dirty = false;

public:
  Impl(const Device& device, Window& window, const Swapchain::Info& info);
  ~Impl();

  Impl(const Impl&) = delete;
  Impl(Impl&&) = delete;
  Impl& operator=(const Impl&) = delete;
  Impl& operator=(Impl&&) = delete;

  void recreate(u32 width, u32 height);
  void unregisterWindow();

private:
  void createSurface(const vkr::Instance& instance);
  vk::SurfaceCapabilitiesKHR processInfo(const Swapchain::Info& info);
  void createSwapchain(const vk::SurfaceCapabilitiesKHR& capabilities);
  void createImages();
};

}