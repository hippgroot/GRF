#pragma once

#include <memory>

namespace grf {

class CommandBuffer;
class Device;
class Swapchain;
class Window;

class GUI {
  class Impl;
  std::unique_ptr<Impl> m_impl;

public:
  explicit GUI(const Device& device, const Swapchain& swapchain);
  ~GUI();

  GUI(const GUI&) = delete;
  GUI(GUI&&) = delete;
  GUI& operator=(const GUI&) = delete;
  GUI& operator=(GUI&&) = delete;

  bool ownsInput() const;
  void newFrame() const;
  void render(CommandBuffer cmd) const;
};

}