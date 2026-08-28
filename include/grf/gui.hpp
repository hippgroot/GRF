#pragma once

#include "./types.hpp"

#include <memory>

namespace grf {

class CommandBuffer;
class Device;
class Swapchain;
class Window;

class GUI {
  class Impl;
  std::unique_ptr<Impl> m_impl;

public:
  class DrawData {
    friend class GUI;

    class Impl;
    std::unique_ptr<Impl> m_impl;

  public:
    DrawData();
    ~DrawData();

    DrawData(const DrawData&) = delete;
    DrawData(DrawData&&) noexcept;
    DrawData& operator=(const DrawData&) = delete;
    DrawData& operator=(DrawData&&) noexcept;
  };

  explicit GUI(const Device& device, const Swapchain& swapchain);
  ~GUI();

  GUI(const GUI&) = delete;
  GUI(GUI&&) = delete;
  GUI& operator=(const GUI&) = delete;
  GUI& operator=(GUI&&) = delete;

  bool ownsInput() const;
  void newFrame() const;
  DrawData snapshot() const;
  void render(CommandBuffer cmd) const;
  void render(CommandBuffer cmd, const DrawData& drawData) const;
};

class Profiler {
public:
  struct Info {
    u32 maxSamples = 240;
    u32 maxGpuSamples = 16;
    u32 flightFrames = 2;
  };

private:
  class Impl;
  std::unique_ptr<Impl> m_impl;

public:
  explicit Profiler(const Device& device, const Info& info = Info{
    .maxSamples         = 240u,
    .maxGpuSamples      = 16u,
    .flightFrames       = 2u
  });
  ~Profiler();

  Profiler(const Profiler&) = delete;
  Profiler(Profiler&&) = delete;
  Profiler& operator=(const Profiler&) = delete;
  Profiler& operator=(Profiler&&) = delete;

  void logFrameTime(f32 time) const;
  void beginFrame(CommandBuffer cmd) const;
  void beginScope(CommandBuffer cmd, const str& name) const;
  void endScope(CommandBuffer cmd) const;

  void draw() const;
};

}