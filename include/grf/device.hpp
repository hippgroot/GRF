#pragma once

#include "./types.hpp"

#include <memory>
#include <filesystem>

namespace grf {

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

private:
  Queue() = default;
};

class Shader {
  friend class ShaderManager;

  class Impl;
  std::shared_ptr<Impl> m_impl;

public:
  ~Shader() = default;

  Shader(const Shader&) = default;
  Shader(Shader&&) = default;
  Shader& operator=(const Shader&) = default;
  Shader& operator=(Shader&&) = default;

private:
  Shader() = default;
};

class Device {
  friend class Swapchain;

public:
  struct Info{
    str applicationName;
    u32 applicationVersion;
    str engineName;
    u32 engineVersion;
  };

private:
  class Impl;
  std::unique_ptr<Impl> m_impl;

public:
  explicit Device(const Info& info = Info{
    .applicationName    = "GRF Application",
    .applicationVersion = 1,
    .engineName         = "GRF"
  });
  ~Device();

  Device(const Device&) = delete;
  Device(Device&&) = delete;
  Device& operator=(const Device&) = delete;
  Device& operator=(Device&&) = delete;

  const Queue& graphicsQueue() const;
  const Queue& computeQueue() const;

  Shader compileGlslShader(ShaderStage, const std::filesystem::path&) const;
};

}