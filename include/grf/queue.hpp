#pragma once

#include "./swapchain.hpp"

#include <memory>
#include <span>

namespace grf {

class Buffer;
class Pipeline;
class Image;

struct ColorAttachment {
  SwapchainImage        img;
  LoadOp                loadOp     = LoadOp::Load;
  StoreOp               storeOp    = StoreOp::Store;
  std::array<float, 4>  clearValue = { 0.0f, 0.0f, 0.0f, 1.0f };
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
  void beginRendering(u32 width, u32 height, std::span<const ColorAttachment> attachments) const;
  void endRendering() const;

  void bindIndexBuffer(const Buffer& buffer, IndexType type = IndexType::u32) const;
  void bindPipeline(const Pipeline& pipeline) const;
  void pushConstants(std::span<const std::byte> data, u32 offset) const;
  void draw(u32 vertexCount) const;
  void drawIndexed(u32 indexCount) const;

  void transition(SwapchainImage image, Layout layout) const;
  void transition(Image image, Layout layout) const;
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