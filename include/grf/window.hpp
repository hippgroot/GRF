#pragma once

#include "./types.hpp"

#include <bitset>
#include <set>

class GLFWwindow;

namespace grf {

class Window {
  friend class Swapchain;

public:
  struct Info {
    u32 width;
    u32 height;
    str title;
  };

private:
  static u32  m_instanceCount;

  GLFWwindow*               m_window = nullptr;
  u32                       m_width = 1280;
  u32                       m_height = 720;
  std::bitset<349>          m_pressed;
  std::bitset<349>          m_justPressed;
  std::bitset<349>          m_justReleased;
  std::set<std::uintptr_t>  m_registeredSwapchains;

public:
  explicit Window(const Info& _ = Info{
    .width = 1280,
    .height = 720,
    .title = "GRF Application"
  });
  ~Window();

  Window(const Window&) = delete;
  Window(Window&&) = delete;
  Window& operator=(const Window&) = delete;
  Window& operator=(Window&&) = delete;

  bool shouldClose() const;
  u32 width() const;
  u32 height() const;

  void pollEvents();
  bool isPressed(Key) const;
  bool isJustPressed(Key) const;
  bool isJustReleased(Key) const;
};

}