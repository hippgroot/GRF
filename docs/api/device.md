# Device

`#include "grf/device.hpp"`

Owns the Vulkan context, allocator, bindless descriptor heap, resource manager, and shader compiler. Used to create resources, and handles resource staging internally. This object must be kept alive longer than all resources it creates.

## Construction

```cpp
Device device;
Device device(Device::Info{ ... });
```

### `Device::Info`

| Field | Default | Meaning |
| --- | --- | --- |
| `applicationName` | `"GRF Application"` | Reported to Vulkan |
| `applicationVersion` | `1` | Reported to Vulkan |
| `engineName` | `"GRF"` | Leave as-is to report GRF's version |
| `engineVersion` | — | Only used when `engineName` is changed |
| `stagingSize_MB` | `64` | Size of each staging buffer |
| `stagingCount` | `2` | Number of staging buffers in the ring |

Device selection requires Vulkan 1.4, swapchain support, and the descriptor-indexing features GRF depends on; discrete GPUs are preferred. Debug builds enable validation layers when present.

## Queues and synchronization

```cpp
const Queue& graphicsQueue() const;
const Queue& computeQueue() const;

void wait(std::span<const Sync> syncs, u64 timeout_ns = u64_max) const;
void waitIdle() const;
```

`wait` blocks the host on timeline syncs (binary syncs from `acquireNextImage` are skipped). `waitIdle` blocks until the whole device is idle. Always use `waitIdle` before `Device` goes out of scope to make sure that there is no work being done on the device before it is destroyed.

## Resource updates

```cpp
Sync updateResources() const;
```

Submits all pending staged writes on the internal transfer queue and returns the `Sync` that signals their completion. It is free when nothing is staged (returns the previous flush's sync). See [Core concepts — resource staging](../concepts.md#resource-staging).

## Shaders and pipelines

```cpp
Shader compileGlslShader(ShaderStage stage, const std::filesystem::path& path) const;
Pipeline createComputePipeline(const Shader& shader) const;
Pipeline createGraphicsPipeline(std::span<const Shader> shaders,
                                const GraphicsPipelineSettings& settings) const;
```

`compileGlslShader` compiles GLSL from file at runtime via shaderc (errors are logged with the compiler's message). Graphics pipelines take exactly one vertex shader and at most one fragment shader; see [Pipeline](pipeline.md) for `GraphicsPipelineSettings`.

## Resources

```cpp
Buffer createBuffer(const Buffer::Info& info) const;
Ring<Buffer> createBufferRing(u32 count, const Buffer::Info& info) const;
Image createImage(const Image::Info& info) const;
Sampler createSampler(const Sampler::Info& info) const;
```

`createBufferRing` is a convenience for frames-in-flight buffers (one per flight frame, typically with `BufferIntent::FrequentUpdate`). More details in [Resources](resources.md).
