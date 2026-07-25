# Queue, CommandBuffer, Sync

`#include "grf/queue.hpp"`

## Queue

Obtained from `device.graphicsQueue()` / `device.computeQueue()`. Copyable handle.

```cpp
Ring<CommandBuffer> getCmdRing(u32 count) const;
Ring<Sync> getSyncRing(u32 count) const;

Sync submit(std::span<const CommandBuffer> cmds, std::span<const Sync> waits = {}) const;
void present(const SwapchainImage& image, std::span<const Sync> waits) const;
```

- `getCmdRing` / `getSyncRing` allocate the per-frame rings for the frames-in-flight pattern (see [Core concepts](../concepts.md#frames-in-flight-pattern)).
- `submit` executes command buffers after all `waits` have signaled and returns the `Sync` that signals when they finish.
- `present` waits on `waits`, then queues the image for presentation.

## Sync

An opaque synchronization point. Produced by `submit`, `acquireNextImage`, and `updateResources`; consumed by `submit`/`present` wait lists and `device.wait`. Copyable (copies will refer to the same point).

## ColorAttachment / DepthAttachment

```cpp
struct ColorAttachment {
  std::variant<SwapchainImage, Image> img;
  LoadOp               loadOp     = LoadOp::Load;
  StoreOp              storeOp    = StoreOp::Store;
  std::array<float, 4> clearValue = { 0.0f, 0.0f, 0.0f, 1.0f };
};

struct DepthAttachment {
  Image   img;
  LoadOp  loadOp     = LoadOp::Load;
  StoreOp storeOp    = StoreOp::Store;
  f32     clearValue = 1.0f;
};
```

Color attachments accept a swapchain image or an offscreen `Image` (created with `ImageUsage::ColorAttachment`). Depth attachments take an `Image` with `ImageUsage::DepthAttachment` and a depth `Format`. Attachments must already be in the matching layout (`ColorAttachmentOptimal` / `DepthAttachmentOptimal`) — `beginRendering` uses each image's tracked layout.

## CommandBuffer

Opaque, copyable handle. Not thread-safe — record from one thread at a time.

### Recording

```cpp
void begin() const;
void end() const;
```

`begin` resets then begins the command buffer and then binds the descriptor heap.

### Rendering

```cpp
void beginRendering(u32 width, u32 height,
                    std::span<const ColorAttachment> colors,
                    std::optional<DepthAttachment> depth = std::nullopt) const;
void endRendering() const;
```

Dynamic rendering methods. `width` and `height` are the viewport/scissor dimensions.

### Binding and drawing

```cpp
void bindPipeline(Pipeline pipeline) const;
void bindIndexBuffer(Buffer buffer, IndexType type = IndexType::u32) const;
void pushConstants(std::span<const std::byte> data, u32 offset = 0) const;

void draw(u32 vertexCount, u32 instanceCount = 1, u32 firstVertex = 0, u32 firstInstance = 0) const;
void drawIndexed(u32 indexCount, u32 instanceCount = 1, u32 firstIndex = 0,
                 i32 vertexOffset = 0, u32 firstInstance = 0) const;
void drawIndirect(Buffer buffer, u32 offset = 0, u32 drawCount = 1,
                  u32 stride = sizeof(DrawIndirectCommand)) const;
void drawIndexedIndirect(Buffer buffer, u32 offset = 0, u32 drawCount = 1,
                         u32 stride = sizeof(DrawIndexedIndirectCommand)) const;

void dispatch(u32 x, u32 y = 1, u32 z = 1) const;
void dispatchIndirect(Buffer buffer, u32 offset = 0) const;
```

Push constants are visible to all stages (total push constant size <= 128 bytes). Indirect commands use the structs in `grf/types.hpp`.

### Barriers and transitions

```cpp
void transition(SwapchainImage image, Layout layout) const;
void transition(Image image, Layout layout) const;
void barrier(Buffer buffer, BufferStage src = BufferStage::All,
             BufferStage dst = BufferStage::All) const;
void barrier() const;
```

`transition` records a barrier from the image's tracked layout and updates the tracking at record time. See [Core concepts — image layouts](../concepts.md#image-layouts).

`barrier()` is a full wait on all resources. Use sparingly and only when necessary.

### Transfers

```cpp
void copyBuffer(Buffer src, Buffer dst, u32 srcOffset, u32 dstOffset, u32 size) const;
void bufferImageCopy(Buffer src, Image dst, u32 bufferOffset = 0, u32 mip = 0) const;
void generateMips(Image image, Layout finalLayout) const;
```

Most uploads should go through `Buffer::write` / `Image::write` + `device.updateResources()` instead; these are the manual escape hatches.

`bufferImageCopy` requires `dst` to be in `TransferDstOptimal`.

`generateMips` blits the full chain from mip 0 and leaves the image in `finalLayout`.
