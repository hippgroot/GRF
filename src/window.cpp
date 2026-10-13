#include "grf/window.hpp"
#include "internal/log.hpp"
#include "internal/swapchain.hpp"

#ifndef GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_NONE
#endif

#include "GLFW/glfw3.h"

#include <algorithm>

namespace grf {

namespace {

GLFWmonitor* overlappingMonitor(GLFWwindow* window) {
  i32 count = 0;
  GLFWmonitor** monitors = glfwGetMonitors(&count);

  ivec2 windowPos, windowSize;
  glfwGetWindowPos(window, &windowPos.x, &windowPos.y);
  glfwGetWindowSize(window, &windowSize.x, &windowSize.y);

  GLFWmonitor* best = glfwGetPrimaryMonitor();
  i32 bestArea = 0;
  for (i32 i = 0; i < count; ++i) {
    const GLFWvidmode* video = glfwGetVideoMode(monitors[i]);
    if (!video) continue;

    ivec2 monitorPos;
    glfwGetMonitorPos(monitors[i], &monitorPos.x, &monitorPos.y);

    ivec2 overlap = glm::max(
      glm::min(windowPos + windowSize, monitorPos + ivec2(video->width, video->height)) - glm::max(windowPos, monitorPos),
      ivec2(0)
    );

    if (overlap.x * overlap.y > bestArea) {
      bestArea = overlap.x * overlap.y;
      best = monitors[i];
    }
  }

  return best;
}

}

u32 Window::m_instanceCount = 0;

Window::Window(const Info& info) {
  if (m_instanceCount++ == 0)
    glfwInit();

  glfwDefaultWindowHints();
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_AUTO_ICONIFY, GLFW_FALSE);

  m_windowedSize = ivec2(info.width, info.height);

  GLFWmonitor* monitor = info.mode == Mode::Borderless ? glfwGetPrimaryMonitor() : nullptr;
  const GLFWvidmode* video = monitor ? glfwGetVideoMode(monitor) : nullptr;

  if (video) {
    glfwWindowHint(GLFW_RED_BITS, video->redBits);
    glfwWindowHint(GLFW_GREEN_BITS, video->greenBits);
    glfwWindowHint(GLFW_BLUE_BITS, video->blueBits);
    glfwWindowHint(GLFW_REFRESH_RATE, video->refreshRate);

    ivec2 monitorPos;
    glfwGetMonitorPos(monitor, &monitorPos.x, &monitorPos.y);
    m_windowedPos = monitorPos + (ivec2(video->width, video->height) - m_windowedSize) / 2;
    m_mode = Mode::Borderless;

    m_window = glfwCreateWindow(video->width, video->height, info.title.c_str(), monitor, nullptr);
  } else {
    m_window = glfwCreateWindow(info.width, info.height, info.title.c_str(), nullptr, nullptr);
  }

  if (!m_window) log::error("Failed to create window (instance: {})", m_instanceCount);
  if (m_mode == Mode::Windowed) glfwGetWindowPos(m_window, &m_windowedPos.x, &m_windowedPos.y);

  int w, h;
  glfwGetFramebufferSize(m_window, &w, &h);
  m_width = w;
  m_height = h;

  glfwSetWindowUserPointer(m_window, this);

  glfwSetKeyCallback(m_window, [](GLFWwindow* p_window, i32 key, i32, i32 action, i32){
    if (key == GLFW_KEY_UNKNOWN) return;
    Window* window = (Window*)glfwGetWindowUserPointer(p_window);

    switch (action) {
    case GLFW_PRESS:
      window->m_pressed[key] = true;
      window->m_justPressed[key] = true;
      break;
    case GLFW_RELEASE:
      window->m_pressed[key] = false;
      window->m_justReleased[key] = true;
      break;
    default:
      return;
    }
  });

  glfwSetMouseButtonCallback(m_window, [](GLFWwindow* p_window, i32 button, i32 action, i32){
    Window* window = (Window*)glfwGetWindowUserPointer(p_window);

    switch (action) {
    case GLFW_PRESS:
      window->m_mousePressed[button] = true;
      window->m_mouseJustPressed[button] = true;
      break;
    case GLFW_RELEASE:
      window->m_mousePressed[button] = false;
      window->m_mouseJustReleased[button] = true;
      break;
    default:
      return;
    }
  });

  glfwSetCursorPosCallback(m_window, [](GLFWwindow* p_window, f64 x, f64 y){
    Window* window = (Window*)glfwGetWindowUserPointer(p_window);
    window->m_cursorPos = {x, y};
  });

  glfwGetCursorPos(m_window, &m_cursorPos.x, &m_cursorPos.y);
  m_lastCursorPos = m_cursorPos;

  glfwSetFramebufferSizeCallback(m_window, [](GLFWwindow* p_window, i32 width, i32 height){
    if (width == 0 || height == 0) return;

    Window* window = (Window*)glfwGetWindowUserPointer(p_window);
    for (const std::uintptr_t& addr : window->m_registeredSwapchains) {
      Swapchain::Impl* swapchain = reinterpret_cast<Swapchain::Impl*>(addr);
      swapchain->flagRecreate(width, height);
    }

    window->m_resizeCallback(width, height);
    window->m_width = width;
    window->m_height = height;
  });
}

Window::~Window() {
  for (const std::uintptr_t& addr : m_registeredSwapchains) {
    Swapchain::Impl* swapchain = reinterpret_cast<Swapchain::Impl*>(addr);
    swapchain->unregisterWindow();
  }

  glfwDestroyWindow(m_window);

  if (--m_instanceCount == 0)
    glfwTerminate();
}

bool Window::shouldClose() const {
  return glfwWindowShouldClose(m_window);
}

u32 Window::width() const {
  return m_width;
}

u32 Window::height() const {
  return m_height;
}

Window::Mode Window::mode() const {
  return m_mode;
}

void Window::setMode(Mode mode) {
  if (mode == m_mode) return;

  if (mode == Mode::Borderless) {
    GLFWmonitor* monitor = overlappingMonitor(m_window);
    const GLFWvidmode* video = monitor ? glfwGetVideoMode(monitor) : nullptr;
    if (!video) {
      log::warning("No monitor available for borderless mode");
      return;
    }

    glfwGetWindowPos(m_window, &m_windowedPos.x, &m_windowedPos.y);
    glfwGetWindowSize(m_window, &m_windowedSize.x, &m_windowedSize.y);
    glfwSetWindowMonitor(m_window, monitor, 0, 0, video->width, video->height, video->refreshRate);
  } else {
    glfwSetWindowMonitor(m_window, nullptr,
      m_windowedPos.x, m_windowedPos.y, m_windowedSize.x, m_windowedSize.y, GLFW_DONT_CARE);
  }

  m_mode = mode;
}

void Window::pollEvents() {
  m_justPressed.reset();
  m_justReleased.reset();
  m_mouseJustPressed.reset();
  m_mouseJustReleased.reset();
  m_lastCursorPos = m_cursorPos;
  glfwPollEvents();
}

bool Window::isPressed(Key key) const {
  return m_pressed[static_cast<u16>(key)];
}

bool Window::isJustPressed(Key key) const {
  return m_justPressed[static_cast<u16>(key)];
}

bool Window::isJustReleased(Key key) const {
  return m_justReleased[static_cast<u16>(key)];
}

bool Window::isPressed(MouseButton button) const {
  return m_mousePressed[static_cast<u8>(button)];
}

bool Window::isJustPressed(MouseButton button) const {
  return m_mouseJustPressed[static_cast<u8>(button)];
}

bool Window::isJustReleased(MouseButton button) const {
  return m_mouseJustReleased[static_cast<u8>(button)];
}

vec2 Window::cursorPosition() const {
  return {
    static_cast<f32>(m_cursorPos.x) / static_cast<f32>(m_width) * 2.0f - 1.0f,
    static_cast<f32>(m_cursorPos.y) / static_cast<f32>(m_height) * 2.0f - 1.0f
  };
}

vec2 Window::cursorDelta() const {
  return {
    static_cast<f32>(m_cursorPos.x - m_lastCursorPos.x) / static_cast<f32>(m_width) * 2.0f,
    static_cast<f32>(m_cursorPos.y - m_lastCursorPos.y) / static_cast<f32>(m_height) * 2.0f
  };
}

void Window::onResize(std::function<void(u32,u32)> callback) {
  m_resizeCallback = callback;
}

}