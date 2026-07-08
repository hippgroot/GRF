#pragma once

#include "./types.hpp"

#include <memory>

namespace grf {

class Device {
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
};

}