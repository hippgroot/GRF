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

  GLFWwindow*                   m_window = nullptr;
  u32                           m_width = 1280;
  u32                           m_height = 720;
  std::bitset<349>              m_pressed;
  std::bitset<349>              m_justPressed;
  std::bitset<349>              m_justReleased;
  std::bitset<8>                m_mousePressed;
  std::bitset<8>                m_mouseJustPressed;
  std::bitset<8>                m_mouseJustReleased;
  dvec2                         m_cursorPos = {0.0, 0.0};
  dvec2                         m_lastCursorPos = {0.0, 0.0};
  std::set<std::uintptr_t>      m_registeredSwapchains;
  std::function<void(u32, u32)> m_resizeCallback;

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

  bool isPressed(Key key) const;
  bool isJustPressed(Key key) const;
  bool isJustReleased(Key key) const;

  bool isPressed(MouseButton button) const;
  bool isJustPressed(MouseButton button) const;
  bool isJustReleased(MouseButton button) const;

  vec2 cursorPosition() const;
  vec2 cursorDelta() const;

  void onResize(std::function<void(u32, u32)>);
};

}