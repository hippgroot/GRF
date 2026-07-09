#pragma once

#include "grf/device.hpp"

#include <shaderc/shaderc.hpp>
#include <vulkan/vulkan_raii.hpp>

#include <filesystem>

namespace vkr = vk::raii;
namespace fs = std::filesystem;

namespace grf {

class Shader::Impl {
  ShaderStage       m_stage;
  vkr::ShaderModule m_module;

public:
  Impl(ShaderStage, vkr::ShaderModule&&);
  ~Impl() = default;

  Impl(const Impl&) = delete;
  Impl(Impl&&) = delete;
  Impl& operator=(const Impl&) = delete;
  Impl& operator=(Impl&&) = delete;
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