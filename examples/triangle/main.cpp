#include "grf/grf.hpp"
#include "grf/window.hpp"

int main() {
  grf::Window window;
  grf::Device device;

  while (!window.shouldClose() && !window.isJustPressed(grf::Key::Escape)) {
    window.pollEvents();
  }
}