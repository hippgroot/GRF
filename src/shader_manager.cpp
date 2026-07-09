#include "internal/shader_manager.hpp"
#include "internal/log.hpp"

#include <fstream>

namespace grf {

Shader::Impl::Impl(ShaderStage stage, vkr::ShaderModule&& module)
: m_stage(stage), m_module(std::move(module))
{}

ShaderManager::ShaderManager() {
  m_opts.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_4);
  m_opts.SetTargetSpirv(shaderc_spirv_version_1_6);

#ifdef GRF_ENABLE_VALIDATION
  m_opts.SetOptimizationLevel(shaderc_optimization_level_zero);
  m_opts.SetGenerateDebugInfo();
#else
  m_opts.SetOptimizationLevel(shaderc_optimization_level_performance);
#endif
}

Shader ShaderManager::compile(const vkr::Device& device, ShaderStage stage, const fs::path& path) const {
  str source = stringify(path);

  shaderc_shader_kind kind;
  switch (stage) {
  case ShaderStage::Vertex:
    kind = shaderc_shader_kind::shaderc_glsl_vertex_shader;
    break;
  case ShaderStage::Fragment:
    kind = shaderc_shader_kind::shaderc_fragment_shader;
    break;
  case ShaderStage::Compute:
    kind = shaderc_shader_kind::shaderc_compute_shader;
    break;
  case ShaderStage::Mesh:
    kind = shaderc_shader_kind::shaderc_mesh_shader;
    break;
  default:
    log::panic("Invalid shader stage given to {}", path.string());
  }

  auto result = m_compiler.CompileGlslToSpv(source, kind, path.c_str(), m_opts);

  if (result.GetCompilationStatus() != shaderc_compilation_status::shaderc_compilation_status_success)
    log::panic("Failed to compile {}:\n{}", path.string(), result.GetErrorMessage());


  if (result.GetNumWarnings() > 0)
    log::warning("Compiled {} with warnings:\n{}", path.string(), result.GetErrorMessage());
  else
    log::general("Compiled {}", path.string());

  std::vector<u32> code(result.cbegin(), result.cend());

  Shader s;
  s.m_impl = std::make_unique<Shader::Impl>(stage, device.createShaderModule(vk::ShaderModuleCreateInfo{
    .codeSize = code.size() * sizeof(u32),
    .pCode = code.data()
  }));

  return s;
}

str ShaderManager::stringify(const fs::path& path) const {
  std::ifstream file(path.string());
  if (!file)
    log::panic("Failed to open {}", path.string());

  str source = "";
  str line;
  while(std::getline(file, line))
    source += std::format("{}\n", line);

  return source;
}

}
