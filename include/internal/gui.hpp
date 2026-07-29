#include "grf/gui.hpp"
#include "grf/queue.hpp"
#include "grf/types.hpp"

#include <vulkan/vulkan_raii.hpp>

class GLFWwindow;
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

}