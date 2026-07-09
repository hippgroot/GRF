#include "internal/swapchain.hpp"

namespace grf {

Swapchain::Swapchain(const Device& device, Window& window, const Swapchain::Info& info)
: m_impl(std::make_unique<Swapchain::Impl>(device, window, info))
{}

Swapchain::~Swapchain() {}

}