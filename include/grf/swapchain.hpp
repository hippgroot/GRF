#pragma once

#include "./types.hpp"

#include <limits>
#include <memory>

namespace grf {

class Device;
class Sync;
class Window;

class SwapchainImage {
  friend class CommandBuffer;
  friend class Queue;
  friend class Swapchain;

  class Impl;
  std::shared_ptr<Impl> m_impl;

public:
  ~SwapchainImage() = default;

  SwapchainImage(const SwapchainImage&) = default;
  SwapchainImage(SwapchainImage&&) = default;
  SwapchainImage& operator=(const SwapchainImage&) = default;
  SwapchainImage& operator=(SwapchainImage&&) = default;

  Format format() const;
  u32 width() const;
  u32 height() const;

private:
  SwapchainImage() = default;
};

class Swapchain {
  friend class GUI;
  friend class Window;

public:
  struct Info {
    Format      format;
    ColorSpace  colorspace;
    bool        vsync;
  };

private:
  class Impl;
  std::unique_ptr<Impl> m_impl;

public:
  Swapchain(const Device&, Window&, const Info& info = Info{
    .format     = Format::bgra8_srgb,
    .colorspace = ColorSpace::srgb_nonlinear,
    .vsync      = true
  });
  ~Swapchain();

  Swapchain(const Swapchain&) = delete;
  Swapchain(Swapchain&&) = delete;
  Swapchain& operator=(const Swapchain&) = delete;
  Swapchain& operator=(Swapchain&&) = delete;

  Format format() const;
  ColorSpace colorspace() const;
  bool vsync() const;
  bool vsyncOnly() const;
  void enableVsync();
  void disableVsync();
  std::pair<SwapchainImage, Sync> acquireNextImage(u64 timeout_ns = std::numeric_limits<u64>::max()) const;
};

}