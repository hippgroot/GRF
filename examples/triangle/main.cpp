#include "grf/device.hpp"
#include "grf/queue.hpp"
#include "grf/swapchain.hpp"
#include "grf/window.hpp"

namespace fs = std::filesystem;
using namespace grf;

int main() {
  const u32 _flightFrames = 2;

  Window window(Window::Info{
    .width = 1280,
    .height = 720,
    .title = "GRF Example: Triangle"
  });
  Device device;
  Swapchain swapchain(device, window);
  Queue queue = device.graphicsQueue();

  Shader vertShader = device.compileGlslShader(
    ShaderStage::Vertex, fs::path(SHADER_DIR) / "vert.glsl"
  );

  Shader fragShader = device.compileGlslShader(
    ShaderStage::Fragment, fs::path(SHADER_DIR) / "frag.glsl"
  );

  Pipeline pipeline = device.createGraphicsPipeline({ vertShader, fragShader },
    GraphicsPipelineSettings{
      .colorFormats = { swapchain.format() },
      .cullMode = CullMode::None,
      .blends = { BlendState{
        .enable         = true,
        .srcColorFactor = BlendFactor::SrcAlpha,
        .dstColorFactor = BlendFactor::OneMinusSrcAlpha
      }}
    }
  );

  auto cmdRing = queue.getCmdRing(_flightFrames);
  auto syncRing = queue.getSyncRing(_flightFrames);

  while (!window.shouldClose() && !window.isJustPressed(grf::Key::Escape)) {
    window.pollEvents();

    auto& sync = syncRing.next();
    device.wait({ sync });

    auto& cmd = cmdRing.next();

    auto [renderTarget, imgSync] = swapchain.acquireNextImage();
    ColorAttachment renderTargetAttachment{
      .img      = renderTarget,
      .loadOp   = LoadOp::Clear,
      .storeOp  = StoreOp::Store
    };

    cmd.begin();
    cmd.transition(renderTarget, Layout::Undefined, Layout::ColorAttachmentOptimal);
    cmd.beginRendering(renderTarget.width(), renderTarget.height(), { renderTargetAttachment });

    cmd.bindPipeline(pipeline);
    cmd.draw(3);

    cmd.endRendering();
    cmd.transition(renderTarget, Layout::ColorAttachmentOptimal, Layout::PresentSrc);
    cmd.end();

    sync = queue.submit({ cmd }, { imgSync });
    queue.present(renderTarget, { sync });
  }

  device.waitIdle();
}