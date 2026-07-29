#pragma once

#include "./pipeline.hpp"
#include "./queue.hpp"
#include "./resources.hpp"

#include <filesystem>
#include <limits>
#include <span>

namespace grf {

class Device {
  friend class GUI;
  friend class Swapchain;

public:
  struct Info{
    str applicationName = "GRF Application";
    u32 applicationVersion = 1;
    str engineName = "GRF";
    u32 engineVersion;
    u32 stagingSize_MB = 64;
    u32 stagingCount = 2;
  };

private:
  class Impl;
  std::unique_ptr<Impl> m_impl;

public:
  explicit Device(const Info& info = Info{
    .applicationName    = "GRF Application",
    .applicationVersion = 1,
    .engineName         = "GRF",
    .stagingSize_MB     = 64,
    .stagingCount       = 2
  });
  ~Device();

  Device(const Device&) = delete;
  Device(Device&&) = delete;
  Device& operator=(const Device&) = delete;
  Device& operator=(Device&&) = delete;

  const Queue& graphicsQueue() const;
  const Queue& computeQueue() const;

  f32 beginFrame() const;

  void wait(std::span<const Sync> syncs, u64 timeout_ns = std::numeric_limits<u64>::max()) const;
  void waitIdle() const;
  Sync updateResources() const;

  Shader compileGlslShader(ShaderStage stage, const std::filesystem::path& path) const;
  Pipeline createComputePipeline(const Shader& shader) const;
  Pipeline createGraphicsPipeline(std::span<const Shader> shaders, const GraphicsPipelineSettings& settings) const;

  Buffer createBuffer(const Buffer::Info& info) const;
  Ring<Buffer> createBufferRing(u32, const Buffer::Info& info) const;

  Image createImage(const Image::Info& info) const;

  Sampler createSampler(const Sampler::Info& info) const;
};

}