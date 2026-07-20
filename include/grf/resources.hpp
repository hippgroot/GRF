#pragma once

#include "./types.hpp"

#include <memory>
#include <span>

namespace grf {

class Buffer {
  friend class Allocator;
  friend class CommandBuffer;
  friend class Device;
  friend class ResourceManager;

public:
  struct Info {
    u32           size;
    BufferIntent  intent;
  };

private:
  class Impl;
  std::shared_ptr<Impl> m_impl;

public:
  Buffer() = default;
  ~Buffer() = default;

  Buffer(const Buffer&) = default;
  Buffer(Buffer&&) = default;
  Buffer& operator=(const Buffer&) = default;
  Buffer& operator=(Buffer&&) = default;

  template <typename T>
    requires std::is_trivially_copyable_v<T> && (!std::ranges::contiguous_range<T>)
  void write(const T& data, u32 offset = 0u) const {
    writeData(std::as_bytes(std::span{&data, 1}), offset);
  }

  template <std::ranges::contiguous_range R>
      requires std::is_trivially_copyable_v<std::ranges::range_value_t<R>>
  void write(R&& data, u32 offset = 0) const {
    writeData(std::as_bytes(std::span{data}), offset);
  }

private:
  void writeData(std::span<const std::byte>, u32) const;
};

}