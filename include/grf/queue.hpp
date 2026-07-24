#pragma once

#include "./resources.hpp"
#include "./swapchain.hpp"

#include <memory>
#include <span>

namespace grf {

class Pipeline;

struct ColorAttachment {
  SwapchainImage        img;
  LoadOp                loadOp     = LoadOp::Load;
  StoreOp               storeOp    = StoreOp::Store;
  std::array<float, 4>  clearValue = { 0.0f, 0.0f, 0.0f, 1.0f };
};

struct DepthAttachment {
  Image   img;
  LoadOp  loadOp     = LoadOp::Load;
  StoreOp storeOp    = StoreOp::Store;
  f32     clearValue = 1.0f;
};

class Sync {
  friend class Device;
  friend class Queue;
  friend class ResourceManager;
  friend class Swapchain;

  class Impl;
  std::shared_ptr<Impl> m_impl;

public:
  ~Sync() = default;

  Sync(const Sync&) = default;
  Sync(Sync&&) = default;
  Sync& operator=(const Sync&) = default;
  Sync& operator=(Sync&&) = default;

private:
  Sync() = default;
};

class CommandBuffer {
  friend class Queue;

  class Impl;
  std::shared_ptr<Impl> m_impl;

public:
  CommandBuffer() = default;
  ~CommandBuffer() = default;

  CommandBuffer(const CommandBuffer&) = default;
  CommandBuffer(CommandBuffer&&) = default;
  CommandBuffer& operator=(const CommandBuffer&) = default;
  CommandBuffer& operator=(CommandBuffer&&) = default;

  void begin() const;
  void end() const;
  void beginRendering(u32 width, u32 height, std::span<const ColorAttachment> colors, std::optional<DepthAttachment> depth = std::nullopt) const;
  void endRendering() const;

  void bindIndexBuffer(Buffer buffer, IndexType type = IndexType::u32) const;
  void bindPipeline(Pipeline pipeline) const;
  void pushConstants(std::span<const std::byte> data, u32 offset = 0) const;
  void draw(u32 vertexCount, u32 instanceCount = 1, u32 firstVertex = 0, u32 firstInstance = 0) const;
  void drawIndirect(Buffer buffer, u32 offset = 0, u32 drawCount = 1, u32 stride = sizeof(DrawIndirectCommand)) const;
  void drawIndexed(u32 indexCount, u32 instanceCount = 1, u32 firstIndex = 0, i32 vertexOffset = 0, u32 firstInstance = 0) const;
  void drawIndexedIndirect(Buffer buffer, u32 offset = 0, u32 drawCount = 1, u32 stride = sizeof(DrawIndexedIndirectCommand)) const;
  void dispatch(u32 x, u32 y = 1, u32 z = 1) const;
  void dispatchIndirect(Buffer buffer, u32 offset = 0) const;

  void transition(SwapchainImage image, Layout layout) const;
  void transition(Image image, Layout layout) const;
  void barrier(Buffer buffer, BufferStage src = BufferStage::All, BufferStage dst = BufferStage::All) const;
  void barrier() const;
  void copyBuffer(Buffer src, Buffer dst, u32 srcOffset, u32 dstOffset, u32 size) const;
  void bufferImageCopy(Buffer src, Image dst, u32 bufferOffset = 0, u32 mip = 0) const;
  void generateMips(Image image, Layout finalLayout) const;
};

class Queue {
  friend class Device;

  class Impl;
  std::shared_ptr<Impl> m_impl;

public:
  Queue() = default;
  ~Queue() = default;

  Queue(const Queue&) = default;
  Queue(Queue&&) = default;
  Queue& operator=(const Queue&) = default;
  Queue& operator=(Queue&&) = default;

  Ring<CommandBuffer> getCmdRing(u32) const;
  Ring<Sync> getSyncRing(u32) const;
  Sync submit(std::span<const CommandBuffer>, std::span<const Sync> waits = {}) const;
  void present(const SwapchainImage&, std::span<const Sync> waits) const;
};

extern template class Ring<CommandBuffer>;
extern template class Ring<Sync>;

}