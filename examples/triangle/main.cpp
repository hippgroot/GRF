#include "grf/device.hpp"
#include "grf/swapchain.hpp"
#include "grf/window.hpp"

namespace fs = std::filesystem;
using namespace grf;

struct Vertex {
  float x, y;
  float u, v;
};

struct PC {
  u64 vertBufAddr;
  u32 texIndex;
  u32 samplerIndex;
};

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

  Buffer vertexBuffer = device.createBuffer(Buffer::Info{
    .size   = 3 * sizeof(Vertex),
    .intent = BufferIntent::GPUOnly
  });
  vertexBuffer.write(std::array<Vertex, 3>{
    Vertex{ -0.5f, 0.5f, 0.0f, 1.0f },
    Vertex{ 0.5f,  0.5f, 1.0f, 1.0f },
    Vertex{ 0.0f, -0.5f, 0.5f, 0.0f }
  });

  Buffer indexBuffer = device.createBuffer(Buffer::Info{
    .size   = 3 * sizeof(u32),
    .intent = BufferIntent::GPUOnly
  });
  indexBuffer.write(std::array<u32, 3>{ 0, 1, 2 });

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

  Sampler sampler = device.createSampler({});

  auto cmdRing = queue.getCmdRing(_flightFrames);
  auto syncRing = queue.getSyncRing(_flightFrames);

  bool mipsGenerated = false;

  while (!window.shouldClose() && !window.isJustPressed(grf::Key::Escape)) {
    window.pollEvents();

    auto& sync = syncRing.next();
    device.wait({ sync });

    Sync resourceSync = device.updateResources();

    auto& cmd = cmdRing.next();

    auto [scImg, imgSync] = swapchain.acquireNextImage();
    ColorAttachment scImgAttachment{
      .img      = scImg,
      .loadOp   = LoadOp::Clear,
      .storeOp  = StoreOp::Store
    };

    cmd.begin();

    if (!mipsGenerated) [[unlikely]] {
      cmd.generateMips(testImage, Layout::ShaderReadOptimal);
      mipsGenerated = true;
    }

    cmd.transition(scImg, Layout::ColorAttachmentOptimal);
    cmd.beginRendering(scImg.width(), scImg.height(), { scImgAttachment });

    PC pc{
      .vertBufAddr  = vertexBuffer.address(),
      .texIndex     = testImage.sampledHeapIndex(),
      .samplerIndex = sampler.heapIndex()
    };

    cmd.bindPipeline(pipeline);
    cmd.bindIndexBuffer(indexBuffer);
    cmd.pushConstants(std::as_bytes(std::span{&pc, 1}));
    cmd.drawIndexed(3);

    cmd.endRendering();
    cmd.transition(scImg, Layout::PresentSrc);
    cmd.end();

    sync = queue.submit({ cmd }, { imgSync, resourceSync });
    queue.present(scImg, { sync });
  }

  device.waitIdle();
}