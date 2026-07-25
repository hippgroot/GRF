# Getting Started

## Prerequisites

- Vulkan 1.4+ with shaderc
- CMake ≥ 4.2
- A C++26 compiler

## Building

```sh
git clone --recursive https://github.com/hippgroot/GRF.git
cd GRF
cmake -B build
cmake --build build
```

Debug builds enable Vulkan validation layers when they are installed. Examples build by default (`-DGRF_BUILD_EXAMPLES=OFF` to skip); binaries land in `build/`.

To use GRF from your own project, either add it as a subdirectory:

```cmake
add_subdirectory(grf)
target_link_libraries(app PRIVATE grf)
```

or install it and use the package:

```sh
cmake --build build --target install    # remove later with --target uninstall
```

```cmake
find_package(grf REQUIRED CONFIG)
target_link_libraries(app PRIVATE GRF::grf)
```

## Application skeleton

Every GRF application follows the same shape. Create the core objects:

```cpp
#include "grf/device.hpp"
#include "grf/swapchain.hpp"
#include "grf/window.hpp"

using namespace grf;

int main() {
  Window window(Window::Info{ .width = 1280, .height = 720, .title = "App" });
  Device device;
  Swapchain swapchain(device, window);
}
```

Compile shaders and build a pipeline:

```cpp

...
Shader vert = device.compileGlslShader(ShaderStage::Vertex, "shaders/vert.glsl");
Shader frag = device.compileGlslShader(ShaderStage::Fragment, "shaders/frag.glsl");

Pipeline pipeline = device.createGraphicsPipeline({ vert, frag },
  GraphicsPipelineSettings{
    .colorFormats = { swapchain.format() },
    .blends       = { BlendState{} }
  }
);
```

Create resources and write data into them (writes to `GPUOnly` memory are staged — see [Core concepts](concepts.md#resource-staging)):

```cpp
Buffer vertexBuffer = device.createBuffer({ .size = size, .intent = BufferIntent::GPUOnly });
vertexBuffer.write(vertices);
```

Then run the frame loop: wait for the frame's previous work, flush staged resource updates, record, submit, present.

```cpp
const u32 _flightFrames = 2;
Queue queue = device.graphicsQueue();
auto cmdRing  = queue.getCmdRing(_flightFrames);
auto syncRing = queue.getSyncRing(_flightFrames);

while (!window.shouldClose()) {
  window.pollEvents();

  auto& sync = syncRing.next();
  device.wait({ sync });

  auto& cmd = cmdRing.next();
  auto [img, imgSync] = swapchain.acquireNextImage();

  cmd.begin();
  cmd.transition(img, Layout::ColorAttachmentOptimal);
  cmd.beginRendering(img.width(), img.height(), {
    ColorAttachment{ .img = img, .loadOp = LoadOp::Clear, .storeOp = StoreOp::Store }
  });

  // bind pipeline, push constants, draw ...

  cmd.endRendering();
  cmd.transition(img, Layout::PresentSrc);
  cmd.end();

  Sync resourceSync = device.updateResources();

  sync = queue.submit({ cmd }, { imgSync, resourceSync });
  queue.present(img, { sync });
}

device.waitIdle();
```

The [triangle example](../examples/triangle) is a full example to get acclimated with the GRF syntax.

## Shaders under GRF

GRF has no vertex input state and no per-material descriptor sets. Shaders receive everything through two mechanisms:

- **Push constants** (128-byte budget) carry small per-draw data like buffer device addresses and heap indices.
- **The descriptor heap** exposes all sampled images (binding 0), storage images (binding 1), and samplers (binding 2) as unsized arrays, indexed by the values resources report from `sampledHeapIndex()` / `storageHeapIndex()` / `heapIndex()`.

A minimal vertex shader pulls its vertices through a buffer reference:

```glsl
#version 460
#extension GL_EXT_buffer_reference : require

struct Vertex { vec2 pos; vec2 uv; };

layout(buffer_reference, std430) readonly buffer VertexBuffer {
  Vertex vertices[];
};

layout(push_constant) uniform PC {
  VertexBuffer vertexBuffer;
} _Constants;

void main() {
  Vertex v = _Constants.vertexBuffer.vertices[gl_VertexIndex];
  gl_Position = vec4(v.pos, 0.0, 1.0);
}
```

and a fragment shader samples through the heap:

```glsl
#version 460
#extension GL_EXT_nonuniform_qualifier : require

layout(binding = 0) uniform texture2D _Textures[];
layout(binding = 2) uniform sampler _Samplers[];
```

See [Core concepts](concepts.md) for the contracts behind each step.
