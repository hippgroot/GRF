#pragma once

#include "./types.hpp"

#include <memory>

namespace grf {

class Device;
class Window;

class Swapchain {
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
  Swapchain(const Device&, Window&, const Info& _ = Info{
    .format     = Format::bgra8_srgb,
    .colorspace = ColorSpace::srgb_nonlinear,
    .vsync      = true
  });
  ~Swapchain();

  Swapchain(const Swapchain&) = delete;
  Swapchain(Swapchain&&) = delete;
  Swapchain& operator=(const Swapchain&) = delete;
  Swapchain& operator=(Swapchain&&) = delete;
};

}