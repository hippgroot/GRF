#pragma once

#include "./types.hpp"

#include <memory>

namespace grf {

class Buffer {
  friend class Allocator;

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
};

}