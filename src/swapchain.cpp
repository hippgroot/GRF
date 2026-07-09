#include "internal/swapchain.hpp"

namespace grf {

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

}