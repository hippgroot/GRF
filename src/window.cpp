#include "grf/window.hpp"
#include "internal/log.hpp"
#include "internal/swapchain.hpp"

#ifndef GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_NONE
#endif

#include "GLFW/glfw3.h"

namespace grf {

u32 Window::m_instanceCount = 0;

Window::Window(const Info& info) {
  if (m_instanceCount++ == 0)
    glfwInit();

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

  m_window = glfwCreateWindow(info.width, info.height, info.title.c_str(), nullptr, nullptr);
  if (!m_window) log::panic("Failed to create window (instance: {})", m_instanceCount);

  m_width = info.width;
  m_height = info.height;

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

  glfwSetWindowSizeCallback(m_window, [](GLFWwindow* p_window, i32 width, i32 height){
    if (width == 0 || height == 0) return;
    Window* window = (Window*)glfwGetWindowUserPointer(p_window);
    window->m_width = width;
    window->m_height = height;
  });

  glfwSetFramebufferSizeCallback(m_window, [](GLFWwindow* p_window, i32 width, i32 height){
    if (width == 0 || height == 0) return;
    Window* window = (Window*)glfwGetWindowUserPointer(p_window);
    for (const std::uintptr_t& addr : window->m_registeredSwapchains) {
      Swapchain::Impl* swapchain = reinterpret_cast<Swapchain::Impl*>(addr);
      swapchain->recreate(width, height);
    }
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

bool Window::isPressed(Key key) const {
  return m_pressed[static_cast<u16>(key)];
}

bool Window::isJustPressed(Key key) const {
  return m_justPressed[static_cast<u16>(key)];
}

bool Window::isJustReleased(Key key) const {
  return m_justReleased[static_cast<u16>(key)];
}

void Window::pollEvents() {
  m_justPressed.reset();
  m_justReleased.reset();
  glfwPollEvents();
}

}