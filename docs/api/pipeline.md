# Shader, Pipeline

`#include "grf/pipeline.hpp"`
> included with the `grf/device.hpp` module

Both are opaque, copyable handles created by `Device`.

## Shader

```cpp
Shader vert = device.compileGlslShader(ShaderStage::Vertex,   "path/to/shader.glsl");
Shader frag = device.compileGlslShader(ShaderStage::Fragment, "path/to/shader.glsl");
Shader comp = device.compileGlslShader(ShaderStage::Compute,  "path/to/shader.glsl");
```

Shaders are compiled at runtime using shaderc.

## Pipeline

```cpp
Pipeline compute  = device.createComputePipeline(comp);
Pipeline graphics = device.createGraphicsPipeline({ vert, frag }, settings);
```

Bind with `cmd.bindPipeline(pipeline)`. The bind point (graphics/compute) is inferred.

Pipelines are given 128 bytes of push constant space (Vulkan guaranteed minimum) and use the layout given by the descriptor heap.

### `GraphicsPipelineSettings`

| Field | Default | Notes |
| --- | --- | --- |
| `colorFormats` | — | one per color attachment; at least one is required |
| `depthFormat` | `Format::undefined` | set to a depth format to enable a depth attachment |
| `topology` | `TriangleList` | |
| `polygonMode` | `Fill` | |
| `cullMode` | `Back` | |
| `frontFace` | `CounterClockwise` | |
| `depthTest` | `false` | requires `depthFormat` |
| `depthWrite` | `false` | enabling it force-enables `depthTest` |
| `depthCompareOp` | `Less` | |
| `blends` | — | one `BlendState` per color attachment; count must match `colorFormats` |
| `sampleCount` | `e1` | |

`BlendState` defaults to blending disabled.

There is intentionally no vertex input state. Instead, store vertices in a buffer and give it to the shader via its address (see [Core concepts](../concepts.md#the-bindless-heap)).
