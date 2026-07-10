#pragma once

#include "./types.hpp"

#include <memory>

namespace grf {


class Sync {
  friend class Device;
  friend class Queue;

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
  ~CommandBuffer() = default;

  CommandBuffer(const CommandBuffer&) = default;
  CommandBuffer(CommandBuffer&&) = default;
  CommandBuffer& operator=(const CommandBuffer&) = default;
  CommandBuffer& operator=(CommandBuffer&&) = default;

private:
  CommandBuffer() = default;
};

class Queue {
  friend class Device;

  class Impl;
  std::shared_ptr<Impl> m_impl;

public:
  ~Queue() = default;

  Queue(const Queue&) = default;
  Queue(Queue&&) = default;
  Queue& operator=(const Queue&) = default;
  Queue& operator=(Queue&&) = default;

  Ring<CommandBuffer> getCmdRing(u32) const;
  Ring<Sync> getSyncRing(u32) const;
  Sync submit(const std::vector<CommandBuffer>&, const std::vector<Sync>& waits = {}) const;

private:
  Queue() = default;
};

extern template class Ring<CommandBuffer>;
extern template class Ring<Sync>;

}