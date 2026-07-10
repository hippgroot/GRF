#pragma once

#include "./pipeline.hpp"
#include "./queue.hpp"
#include "./types.hpp"

#include <filesystem>

namespace grf {

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
  Pipeline createComputePipeline(const Shader&) const;
  Pipeline createGraphicsPipeline(const std::vector<Shader>&, const GraphicsPipelineSettings&) const;
};

}