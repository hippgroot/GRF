#include "grf/device.hpp"
#include "grf/swapchain.hpp"
#include "grf/window.hpp"

namespace fs = std::filesystem;

int main() {
  grf::Window window(grf::Window::Info{
    .width = 1280,
    .height = 720,
    .title = "GRF Example: Triangle"
  });
  grf::Device device;
  grf::Swapchain swapchain(device, window);

  grf::Shader vertShader = device.compileGlslShader(
    grf::ShaderStage::Vertex, fs::path(SHADER_DIR) / "vert.glsl"
  );

  grf::Shader fragShader = device.compileGlslShader(
    grf::ShaderStage::Fragment, fs::path(SHADER_DIR) / "frag.glsl"
  );

  while (!window.shouldClose() && !window.isJustPressed(grf::Key::Escape)) {
    window.pollEvents();
  }
}