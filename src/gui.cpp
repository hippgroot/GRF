#include "grf/window.hpp"
#include "internal/device.hpp"
#include "internal/gui.hpp"
#include "internal/swapchain.hpp"
#include "internal/log.hpp"

#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

namespace grf {

GUI::GUI(const Device& device, const Swapchain& swapchain) {
  m_impl = std::make_unique<Impl>(Impl::Info{
    .instance       = device.m_impl->m_instance,
    .physicalDevice = device.m_impl->m_physicalDevice,
    .device         = device.m_impl->m_device,
    .queue          = device.graphicsQueue(),
    .window         = swapchain.m_impl->m_window->m_window,
    .format         = swapchain.m_impl->m_surfaceFormat.format,
    .minImageCount  = swapchain.m_impl->m_minImageCount,
    .imageCount     = static_cast<u32>(swapchain.m_impl->m_images.size()),
  });
}

GUI::~GUI() {}

bool GUI::ownsInput() const {
  ImGui::SetCurrentContext(m_impl->m_context);
  const auto& io = ImGui::GetIO();
  return io.WantCaptureMouse || io.WantCaptureKeyboard;
}

void GUI::newFrame() const {
  ImGui::SetCurrentContext(m_impl->m_context);
  ImGui_ImplVulkan_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();
}

void GUI::render(CommandBuffer cmd) const {
  ImGui::SetCurrentContext(m_impl->m_context);
  ImGui::Render();
  ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), *cmd.m_impl->commandBuffer());
}

GUI::DrawData GUI::snapshot() const {
  ImGui::SetCurrentContext(m_impl->m_context);
  ImGui::Render();

  ImDrawData* src = ImGui::GetDrawData();

  if (src->Textures) {
    bool pending = false;
    for (const ImTextureData* tex : *src->Textures) {
      if (tex->Status != ImTextureStatus_OK) {
        pending = true;
        break;
      }
    }

    if (pending) {
      std::lock_guard lock(m_impl->m_queue.m_impl->m_mutex);
      for (ImTextureData* tex : *src->Textures)
        ImGui_ImplVulkan_UpdateTexture(tex);
    }
  }

  DrawData drawData;
  drawData.m_impl = std::make_unique<DrawData::Impl>(src);
  return drawData;
}

void GUI::render(CommandBuffer cmd, const DrawData& drawData) const {
  if (!drawData.m_impl || !drawData.m_impl->m_valid)
    return;

  ImGui::SetCurrentContext(m_impl->m_context);

  const DrawData::Impl& src = *drawData.m_impl;

  ImDrawData data;
  data.Valid            = src.m_valid;
  data.TotalVtxCount    = src.m_totalVtxCount;
  data.TotalIdxCount    = src.m_totalIdxCount;
  data.CmdLists         = src.m_cmdLists;
  data.DisplayPos       = src.m_displayPos;
  data.DisplaySize      = src.m_displaySize;
  data.FramebufferScale = src.m_framebufferScale;
  data.OwnerViewport    = src.m_ownerViewport;
#ifndef IMGUI_DISABLE_OBSOLETE_FUNCTIONS
  data.CmdListsCount    = data.CmdLists.Size;
#endif

  ImGui_ImplVulkan_RenderDrawData(&data, *cmd.m_impl->commandBuffer());
}

GUI::DrawData::DrawData() = default;
GUI::DrawData::~DrawData() = default;
GUI::DrawData::DrawData(DrawData&&) noexcept = default;
GUI::DrawData& GUI::DrawData::operator=(DrawData&&) noexcept = default;

GUI::DrawData::Impl::Impl(const ImDrawData* src) {
  m_valid            = src->Valid;
  m_totalVtxCount    = src->TotalVtxCount;
  m_totalIdxCount    = src->TotalIdxCount;
  m_displayPos       = src->DisplayPos;
  m_displaySize      = src->DisplaySize;
  m_framebufferScale = src->FramebufferScale;
  m_ownerViewport    = src->OwnerViewport;

  m_cmdLists.reserve(src->CmdLists.Size);
  for (const ImDrawList* list : src->CmdLists)
    m_cmdLists.push_back(list->CloneOutput());
}

GUI::DrawData::Impl::~Impl() {
  for (ImDrawList* list : m_cmdLists)
    IM_DELETE(list);
}

GUI::Impl::Impl(const Info& info) {
  m_device = info.device;
  m_queue = info.queue;

  IMGUI_CHECKVERSION();
  m_context = ImGui::CreateContext();
  ImGui::SetCurrentContext(m_context);

  ImGui_ImplGlfw_InitForVulkan(info.window, true);

  ImGui_ImplVulkan_InitInfo init{
    .ApiVersion         = VK_API_VERSION_1_4,
    .Instance           = info.instance,
    .PhysicalDevice     = info.physicalDevice,
    .Device             = m_device,
    .QueueFamily        = info.queue.m_impl->m_index,
    .Queue              = *info.queue.m_impl->m_queue,
    .DescriptorPoolSize = 16,
    .MinImageCount      = info.minImageCount,
    .ImageCount         = info.imageCount,
    .PipelineInfoMain   = {
      .PipelineRenderingCreateInfo = {
        .sType                    = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
        .colorAttachmentCount     = 1,
        .pColorAttachmentFormats  = reinterpret_cast<const VkFormat*>(&info.format)
      }
    },
    .UseDynamicRendering = true,
  };
  ImGui_ImplVulkan_Init(&init);
}

GUI::Impl::~Impl() {
  m_device.waitIdle();
  ImGui::SetCurrentContext(m_context);
  ImGui_ImplVulkan_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext(m_context);
}

Profiler::Profiler(const Device& device, const Profiler::Info& info) {
  auto props = device.m_impl->m_physicalDevice.getProperties();
  m_impl = std::make_unique<Impl>(device.m_impl->m_device, props.limits.timestampPeriod, info);
}

Profiler::~Profiler() {}

void Profiler::logFrameTime(f32 frameTime) const {
  std::lock_guard lock(m_impl->m_mutex);
  m_impl->m_tracker.addTime(frameTime);
  m_impl->m_averages.addFrame(frameTime);
}

void Profiler::beginFrame(CommandBuffer cmd) const {
  m_impl->m_scopeIndex = (m_impl->m_scopeIndex + 1) % m_impl->m_flightFrames;
  u32 base = 2 * m_impl->m_scopeIndex * m_impl->m_maxGpuSamples;

  auto& scope = m_impl->m_scopes[m_impl->m_scopeIndex];
  scope.recordResults(m_impl->m_queryPool, base, m_impl->m_timestampPeriod_ns);
  {
    std::lock_guard lock(m_impl->m_mutex);
    m_impl->m_averages.addScopeTimes(scope.results());
  }
  scope.reset();

  cmd.m_impl->commandBuffer().resetQueryPool(m_impl->m_queryPool, base, 2 * m_impl->m_maxGpuSamples);
}

void Profiler::beginScope(CommandBuffer cmd, const str& name) const {
  u32 slot = m_impl->m_scopes[m_impl->m_scopeIndex].begin(name);
  if (slot >= m_impl->m_maxGpuSamples)
    log::error("Profiler exceeded maxGpuSamples ({})", m_impl->m_maxGpuSamples);

  u32 base = 2 * m_impl->m_scopeIndex * m_impl->m_maxGpuSamples;
  cmd.m_impl->commandBuffer().writeTimestamp(
    vk::PipelineStageFlagBits::eTopOfPipe, m_impl->m_queryPool, base + 2 * slot
  );
}

void Profiler::endScope(CommandBuffer cmd) const {
  u32 slot = m_impl->m_scopes[m_impl->m_scopeIndex].end();

  u32 base = 2 * m_impl->m_scopeIndex * m_impl->m_maxGpuSamples;
  cmd.m_impl->commandBuffer().writeTimestamp(
    vk::PipelineStageFlagBits::eBottomOfPipe, m_impl->m_queryPool, base + 2 * slot + 1
  );
}

void Profiler::draw() const {
  std::lock_guard lock(m_impl->m_mutex);
  const auto& tracker = m_impl->m_tracker;
  const auto& averages = m_impl->m_averages;

  ImGui::Begin("Profiler");
  ImGui::Text("Frame: %.2f ms (%.0f fps)", averages.frameTime_ms(), averages.fps());
  ImGui::PlotLines("##frametimes",
    tracker.times().data(), static_cast<int>(tracker.times().size()), static_cast<int>(tracker.offset()),
    nullptr, 0.0f, FLT_MAX, ImVec2(-1.0f, 40.0f)
  );

  const auto& scopes = averages.scopeTimes();
  if (!scopes.empty()) {
    ImGui::SeparatorText("GPU");
    for (const auto& [name, time_ms] : scopes)
      ImGui::Text("%s: %.3f ms", name.c_str(), time_ms);
  }
  ImGui::End();
}

const std::vector<std::pair<str, f32>>& ScopeList::results() const {
  return m_results;
}

u32 ScopeList::begin(const str& name) {
  u32 index = static_cast<u32>(m_names.size());
  m_names.push_back(name);
  m_open.push_back(index);
  return index;
}

u32 ScopeList::end() {
  if (m_open.empty())
    log::error("Profiler endScope called with no open scope");

  u32 index = m_open.back();
  m_open.pop_back();
  return index;
}

void ScopeList::recordResults(const vkr::QueryPool& queryPool, u32 baseIndex, f32 timestampPeriod_ns) {
  if (!m_open.empty())
    log::error("Profiler scope '{}' was never ended", m_names[m_open.back()]);

  m_results.clear();
  if (m_names.empty()) return;

  u32 count = static_cast<u32>(m_names.size());
  auto [result, timestamps] = queryPool.getResults<u64>(
    baseIndex, 2 * count, 2 * count * sizeof(u64), sizeof(u64),
    vk::QueryResultFlagBits::e64 | vk::QueryResultFlagBits::eWait
  );

  if (result != vk::Result::eSuccess) {
    log::warning("Profiler failed to read timestamp queries");
    return;
  }

  m_results.reserve(count);
  for (u32 i = 0; i < count; i++) {
    f32 time_ms = static_cast<f32>(timestamps[2 * i + 1] - timestamps[2 * i]) * timestampPeriod_ns / 1'000'000.0f;
    m_results.emplace_back(m_names[i], time_ms);
  }
}

void ScopeList::reset() {
  m_names.clear();
  m_open.clear();
}

FrametimeTracker::FrametimeTracker(u32 maxSamples)
: m_maxSamples(maxSamples) {
  m_frameTimes.resize(m_maxSamples);
}

void FrametimeTracker::addTime(f32 time) {
  m_frameTimes[m_nextIndex++] = time;
  if (m_nextIndex == m_maxSamples)
    m_nextIndex = 0;
}

const std::vector<f32>& FrametimeTracker::times() const {
  return m_frameTimes;
}

u32 FrametimeTracker::offset() const {
  return m_nextIndex;
}

AverageDisplay::AverageDisplay(f32 interval_s)
: m_interval_s(interval_s) {}

f32 AverageDisplay::frameTime_ms() const {
  return m_frameTime_ms;
}

f32 AverageDisplay::fps() const {
  return m_fps;
}

const std::vector<std::pair<str, f32>>& AverageDisplay::scopeTimes() const {
  return m_scopeTimes;
}

void AverageDisplay::addFrame(f32 dt) {
  m_elapsed_s += dt;
  m_frameCount++;

  if (m_elapsed_s < m_interval_s) return;

  m_frameTime_ms = m_elapsed_s / m_frameCount * 1000.0f;
  m_fps = m_frameCount / m_elapsed_s;

  m_scopeTimes.clear();
  m_scopeTimes.reserve(m_scopeSums.size());
  for (u32 i = 0; i < m_scopeSums.size(); i++)
    m_scopeTimes.emplace_back(m_scopeSums[i].first, m_scopeSums[i].second / m_scopeCounts[i]);

  m_elapsed_s = 0.0f;
  m_frameCount = 0u;
  m_scopeSums.clear();
  m_scopeCounts.clear();
}

void AverageDisplay::addScopeTimes(const std::vector<std::pair<str, f32>>& results) {
  for (const auto& [name, time_ms] : results) {
    u32 i = 0;
    while (i < m_scopeSums.size() && m_scopeSums[i].first != name)
      i++;

    if (i == m_scopeSums.size()) {
      m_scopeSums.emplace_back(name, time_ms);
      m_scopeCounts.push_back(1u);
    } else {
      m_scopeSums[i].second += time_ms;
      m_scopeCounts[i]++;
    }
  }
}

Profiler::Impl::Impl(const vkr::Device& device, f32 timestampPeriod_ns, const Profiler::Info& info)
: m_maxGpuSamples(info.maxGpuSamples),
  m_flightFrames(info.flightFrames),
  m_timestampPeriod_ns(timestampPeriod_ns),
  m_tracker(FrametimeTracker(info.maxSamples)),
  m_averages(AverageDisplay(1.0f))
{
  m_queryPool = device.createQueryPool(vk::QueryPoolCreateInfo{
    .queryType = vk::QueryType::eTimestamp,
    .queryCount = 2 * info.maxGpuSamples * info.flightFrames
  });

  m_scopes.resize(m_flightFrames);
}

}