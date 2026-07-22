#include "grf/device.hpp"
#include "grf/swapchain.hpp"
#include "grf/window.hpp"

namespace fs = std::filesystem;
using namespace grf;

int main() {
  const u32 _flightFrames = 2;

  Window window(Window::Info{
    .width  = 1280,
    .height = 720,
    .title  = "GRF Example: Triangle"
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

  Buffer buffer = device.createBuffer(Buffer::Info{
    .size   = 3 * sizeof(u32),
    .intent = BufferIntent::GPUOnly
  });
  buffer.write(std::vector<u32>{ 0, 1, 2 });

  Image testImage;
{
  Image::Data data = Image::getDataFromFile(std::format("{}/uv_grid.png", ASSETS_DIR));
  testImage = device.createImage(Image::Info{
    .width        = data.width,
    .height       = data.height,
    .format       = data.format,
    .usage        = ImageUsage::Sampled,
    .allocateMips = true
  });
  testImage.write(data.bytes, Layout::TransferDstOptimal);
}

  auto cmdRing = queue.getCmdRing(_flightFrames);
  auto syncRing = queue.getSyncRing(_flightFrames);

  bool mipsGenerated = false;

  while (!window.shouldClose() && !window.isJustPressed(grf::Key::Escape)) {
    window.pollEvents();

    auto& sync = syncRing.next();
    device.wait({ sync });

    // flush staging before recording so tracked image layouts match what this frame's commands see
    Sync resourceSync = device.updateResources();

    auto& cmd = cmdRing.next();

    auto [scImg, imgSync] = swapchain.acquireNextImage();
    ColorAttachment renderTargetAttachment{
      .img      = scImg,
      .loadOp   = LoadOp::Clear,
      .storeOp  = StoreOp::Store
    };

    cmd.begin();

    if (!mipsGenerated) {
      cmd.generateMips(testImage, Layout::ShaderReadOptimal);
      mipsGenerated = true;
    }

    cmd.transition(scImg, Layout::ColorAttachmentOptimal);
    cmd.beginRendering(scImg.width(), scImg.height(), { renderTargetAttachment });

    cmd.bindPipeline(pipeline);
    cmd.bindIndexBuffer(buffer);
    cmd.drawIndexed(3);

    cmd.endRendering();
    cmd.transition(scImg, Layout::PresentSrc);
    cmd.end();

    sync = queue.submit({ cmd }, { imgSync, resourceSync });
    queue.present(scImg, { sync });
  }

  device.waitIdle();
}