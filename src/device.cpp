#include "internal/device.hpp"

namespace fs = std::filesystem;

namespace grf {

Device::Device(const Info& info) : m_impl(std::make_unique<Impl>(info)) {}

Device::~Device() {}

const Queue& Device::graphicsQueue() const {
  return m_impl->m_graphicsQueue;
}

const Queue& Device::computeQueue() const {
  return m_impl->m_computeQueue;
}

Shader Device::compileGlslShader(ShaderStage stage, const fs::path& path) const {
  return m_impl->m_shaderManager.compile(m_impl->m_device, stage, path);
}

}