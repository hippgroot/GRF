#include "grf/gui.hpp"
#include "grf/queue.hpp"
#include "grf/types.hpp"

#include <vulkan/vulkan_raii.hpp>

namespace vkr = vk::raii;

struct GLFWwindow;
struct ImGuiContext;

namespace grf {

class GUI::Impl {
  friend class GUI;

public:
  struct Info {
    vk::Instance        instance;
    vk::PhysicalDevice  physicalDevice;
    vk::Device          device;
    Queue               queue;
    GLFWwindow*         window;
    vk::Format          format;
    u32                 minImageCount;
    u32                 imageCount;
  };

private:
  vk::Device    m_device = nullptr;
  ImGuiContext* m_context = nullptr;

public:
  explicit Impl(const Info& info);
  ~Impl();

  Impl(const Impl&) = delete;
  Impl(Impl&&) = delete;
  Impl& operator=(const Impl&) = delete;
  Impl& operator=(Impl&&) = delete;
};

class ScopeList {
  std::vector<str> m_names;
  std::vector<u32> m_open;
  std::vector<std::pair<str, f32>> m_results;

public:
  ScopeList() = default;
  ~ScopeList() = default;

  const std::vector<std::pair<str, f32>>& results() const;

  u32 begin(const str& name);
  u32 end();
  void recordResults(const vkr::QueryPool& queryPool, u32 baseIndex, f32 timestampPeriod_ns);
  void reset();
};

class FrametimeTracker {
  const u32         m_maxSamples;

  std::vector<f32>  m_frameTimes;
  u32               m_nextIndex = 0u;

public:
  explicit FrametimeTracker(u32 maxSamples);
  ~FrametimeTracker() = default;

  void addTime(f32 time);

  const std::vector<f32>& times() const;
  u32 offset() const;
};

class AverageDisplay {
  const f32 m_interval_s;

  f32 m_elapsed_s = 0.0f;
  u32 m_frameCount = 0u;
  std::vector<std::pair<str, f32>> m_scopeSums;
  std::vector<u32> m_scopeCounts;

  f32 m_frameTime_ms = 0.0f;
  f32 m_fps = 0.0f;
  std::vector<std::pair<str, f32>> m_scopeTimes;

public:
  explicit AverageDisplay(f32 interval_s);
  ~AverageDisplay() = default;

  f32 frameTime_ms() const;
  f32 fps() const;
  const std::vector<std::pair<str, f32>>& scopeTimes() const;

  void addFrame(f32 dt);
  void addScopeTimes(const std::vector<std::pair<str, f32>>& results);
};

class Profiler::Impl {
  friend class Profiler;

  const u32               m_maxGpuSamples;
  const u32               m_flightFrames;
  const f32               m_timestampPeriod_ns;

  vkr::QueryPool          m_queryPool = nullptr;
  FrametimeTracker        m_tracker;
  AverageDisplay          m_averages;
  std::vector<ScopeList>  m_scopes;
  u32                     m_scopeIndex = 0;

public:
  Impl(const vkr::Device& device, f32 timestampPeriod_ns, const Profiler::Info& info);
  ~Impl() = default;

  Impl(const Impl&) = delete;
  Impl(Impl&&) = delete;
  Impl& operator=(const Impl&) = delete;
  Impl& operator=(Impl&&) = delete;
};

}