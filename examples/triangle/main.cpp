#include "grf/device.hpp"
#include "grf/swapchain.hpp"
#include "grf/window.hpp"

int main() {
  grf::Window window(grf::Window::Info{
    .width = 1280,
    .height = 720,
    .title = "GRF Example: Triangle"
  });
  grf::Device device;
  grf::Swapchain swapchain(device, window);

  while (!window.shouldClose() && !window.isJustPressed(grf::Key::Escape)) {
    window.pollEvents();
  }
}