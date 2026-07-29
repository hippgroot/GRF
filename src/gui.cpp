#include "grf/window.hpp"
#include "imgui.h"
#include "imgui_impl_vulkan.h"
#include "internal/device.hpp"
#include "internal/gui.hpp"
#include "internal/swapchain.hpp"

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

GUI::Impl::Impl(const Info& info) {
  m_device = info.device;

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

}