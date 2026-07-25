#include "grf/device.hpp"
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
  while (!window.shouldClose() && !window.isJustPressed(Key::Escape)) {
    window.pollEvents();

    auto& sync = syncRing.next();
    device.wait({ sync });

    auto [img, imgSync]= swapchain.acquireNextImage();
    ColorAttachment color{
      .img        = img,
      .loadOp     = LoadOp::Clear,
      .storeOp    = StoreOp::Store,
      .clearValue = { 0.0f, 0.0f, 0.0f, 0.0f }
    };

    auto cmd = cmdRing.next();

    cmd.begin();
    cmd.transition(img, Layout::ColorAttachmentOptimal);
    cmd.beginRendering(img.width(), img.height(), { color });

    cmd.bindPipeline(pipeline);
    cmd.draw(_vertexCount);

    cmd.endRendering();
    cmd.transition(img, Layout::PresentSrc);
    cmd.end();

    sync = queue.submit({ cmd }, { imgSync });
    queue.present(img, { sync });
  }

  device.waitIdle();
}