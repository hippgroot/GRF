#pragma once

#include "grf/pipeline.hpp"
#include "grf/types.hpp"

#include <shaderc/shaderc.hpp>
#include <vulkan/vulkan_raii.hpp>

#include <filesystem>

namespace vkr = vk::raii;
namespace fs = std::filesystem;

namespace grf {

enum class PipelineType {
  Graphics,
  Compute
};

class Shader::Impl {
  friend class Device;
  friend class Shader;

  vk::ShaderStageFlagBits m_stage;
  vkr::ShaderModule       m_module;

public:
  Impl(vk::ShaderStageFlagBits, vkr::ShaderModule&&);
  ~Impl() = default;

  Impl(const Impl&) = delete;
  Impl(Impl&&) = delete;
  Impl& operator=(const Impl&) = delete;
  Impl& operator=(Impl&&) = delete;
};

class Pipeline::Impl {
  friend class Pipeline;

  vkr::Pipeline m_pipeline = nullptr;
  PipelineType  m_type = PipelineType::Graphics;

public:
  explicit Impl(PipelineType, vkr::Pipeline&&);
  ~Impl() = default;

  Impl(const Impl&) = delete;
  Impl(Impl&&) = delete;
  Impl& operator=(const Impl&) = delete;
  Impl& operator=(Impl&&) = delete;

  PipelineType type() const;
  vk::Pipeline pipeline() const;
};

class ShaderManager {
  shaderc::Compiler m_compiler;
  shaderc::CompileOptions m_opts;

public:
  ShaderManager();
  ~ShaderManager() = default;

  ShaderManager(const ShaderManager&) = delete;
  ShaderManager(ShaderManager&&) = delete;
  ShaderManager& operator=(const ShaderManager&) = delete;
  ShaderManager& operator=(ShaderManager&&) = delete;

  Shader compile(const vkr::Device&, ShaderStage, const fs::path&) const;

private:
  str stringify(const fs::path&) const;
};

}