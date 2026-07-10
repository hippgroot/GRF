#pragma once

#include <memory>

namespace grf {

class Shader {
  friend class Device;
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

class Pipeline {
  friend class CommandBuffer;
  friend class Device;

  class Impl;
  std::shared_ptr<Impl> m_impl;

public:
  ~Pipeline() = default;

  Pipeline(const Pipeline&) = default;
  Pipeline(Pipeline&&) = default;
  Pipeline& operator=(const Pipeline&) = default;
  Pipeline& operator=(Pipeline&&) = default;

private:
  Pipeline() = default;
};

}