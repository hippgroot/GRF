#include "grf/device.hpp"
#include "grf/gui.hpp"
#include "grf/imgui.h"
#include "grf/swapchain.hpp"
#include "grf/window.hpp"

using namespace grf;

int main() {
  Window window(Window::Info{
    .width = 1280,
    .height = 720,
    .title = "GRF Triangle"
  });
  Device device;
  Swapchain swapchain(device, window);

  GUI gui(device, swapchain);
  auto& io = ImGui::GetIO();
  io.IniFilename = nullptr;

  Shader vertShader = device.compileGlslShader(ShaderStage::Vertex,
    std::format("{}/vert.glsl", SHADER_DIR)
  );

  Shader fragShader = device.compileGlslShader(ShaderStage::Fragment,
    std::format("{}/frag.glsl", SHADER_DIR)
  );

  Pipeline pipeline = device.createGraphicsPipeline({ vertShader, fragShader },
    GraphicsPipelineSettings{
      .colorFormats = { swapchain.format() },
      .cullMode = CullMode::None,
      .blends = { BlendState{} }
    }
  );

  const u32 _flightFrames = 2;
  Queue queue = device.graphicsQueue();
  Ring<CommandBuffer> cmdRing = queue.getCmdRing(_flightFrames);
  Ring<Sync> syncRing = queue.getSyncRing(_flightFrames);

  const u32 _vertexCount = 3;
  bool showDemoWindow = true;
  while (!window.shouldClose() && !window.isJustPressed(Key::Escape)) {
    window.pollEvents();

    auto& sync = syncRing.next();
    device.wait({ sync });
    auto& cmd = cmdRing.next();

    auto [img, imgSync]= swapchain.acquireNextImage();

    cmd.begin();
    cmd.transition(img, Layout::ColorAttachmentOptimal);
    cmd.beginRendering(img.width(), img.height(), { ColorAttachment{
      .img        = img,
      .loadOp     = LoadOp::Clear,
      .storeOp    = StoreOp::Store
    }});

    cmd.bindPipeline(pipeline);
    cmd.draw(_vertexCount);

    cmd.endRendering();
    cmd.beginRendering(img.width(), img.height(), { ColorAttachment{
      .img = img,
      .loadOp = LoadOp::Load,
      .storeOp = StoreOp::Store,
    }});

    gui.newFrame();
    ImGui::Begin("Test Window");
      ImGui::TextUnformatted("Groot");
      ImGui::TextUnformatted("Rendering");
      ImGui::TextUnformatted("Framework");
    ImGui::End();
    gui.render(cmd);

    cmd.endRendering();

    cmd.transition(img, Layout::PresentSrc);
    cmd.end();

    sync = queue.submit({ cmd }, { imgSync });
    queue.present(img, { sync });
  }

  device.waitIdle();
}