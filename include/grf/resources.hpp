#pragma once

#include "./types.hpp"

#include <filesystem>
#include <memory>
#include <span>
#include <type_traits>

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

  template <typename T>
    requires std::is_trivially_copyable_v<T> && std::is_default_constructible_v<T>
  T read(u32 offset = 0) const {
    T out{};
    readData(std::as_writable_bytes(std::span{&out, 1}), offset);
    return out;
  }

private:
  void writeData(std::span<const std::byte>, u32) const;
  void readData(std::span<std::byte>, u32) const;
};

class Image {
  friend class Allocator;
  friend class CommandBuffer;
  friend class DescriptorHeap;
  friend class Device;
  friend class ResourceManager;

public:
  struct Info {
    u32         width;
    u32         height;
    u32         depth = 1;
    Format      format;
    ImageUsage  usage = ImageUsage::None;
    bool        allocateMips = false;
  };

  struct Data {
    u32                     width;
    u32                     height;
    Format                  format;
    std::vector<std::byte>  bytes;
  };

private:
  class Impl;
  std::shared_ptr<Impl> m_impl;

public:
  Image() = default;
  ~Image() = default;

  Image(const Image&) = default;
  Image(Image&&) = default;
  Image& operator=(const Image&) = default;
  Image& operator=(Image&&) = default;

  static Data getDataFromFile(const std::filesystem::path& path, bool srgb = true);

  u32 width() const;
  u32 height() const;
  u32 depth() const;

  void write(std::span<const std::byte> bytes, Layout finalLayout, u32 mip = 0) const;
};

}