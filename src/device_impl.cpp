#include "internal/device.hpp"
#include "internal/log.hpp"

#include "GLFW/glfw3.h"

#ifdef GRF_ENABLE_VALIDATION
#define GRF_VALIDATION_LAYER_NAME "VK_LAYER_KHRONOS_validation"
#endif

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

namespace grf {

Device::Impl::Impl(const Device::Info& info) {
  VULKAN_HPP_DEFAULT_DISPATCHER.init();
  createInstance(info);
  VULKAN_HPP_DEFAULT_DISPATCHER.init(*m_instance);

  log::general("GRF {}.{}.{}", GRF_VERSION_MAJOR, GRF_VERSION_MINOR, GRF_VERSION_PATCH);
}

void Device::Impl::createInstance(const Device::Info& info) {
  u32 engineVersion = info.engineName == "GRF" ?
    VK_MAKE_API_VERSION(0, GRF_VERSION_MAJOR, GRF_VERSION_MINOR, GRF_VERSION_PATCH)
  : info.engineVersion;

  vk::ApplicationInfo appInfo{
    .pApplicationName   = info.applicationName.c_str(),
    .applicationVersion = info.applicationVersion,
    .pEngineName        = info.engineName.c_str(),
    .engineVersion      = engineVersion,
    .apiVersion         = VK_API_VERSION_1_4
  };

  std::vector<const char *> layers;
#ifdef GRF_ENABLE_VALIDATION
  for (const auto& property : m_context.enumerateInstanceLayerProperties()) {
    if (strcmp(property.layerName, GRF_VALIDATION_LAYER_NAME) == 0) {
      layers.emplace_back(GRF_VALIDATION_LAYER_NAME);
      break;
    }
  }
  if (layers.empty())
    log::warning("Debug builds enable validation layers yet none were found");
#endif

  u32 count = 0;
  (void)glfwGetRequiredInstanceExtensions(&count);
  const char ** glfwExtensions = glfwGetRequiredInstanceExtensions(&count);

  std::vector<const char *> extensions(glfwExtensions, glfwExtensions + count);
  vk::InstanceCreateFlags flags;
  for (const auto& property : m_context.enumerateInstanceExtensionProperties()) {
    if (strcmp(property.extensionName, VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME) == 0) {
      extensions.emplace_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
      flags |= vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR;
      log::general("Portability enabled");
      break;
    }
  }

  m_instance = m_context.createInstance(vk::InstanceCreateInfo{
    .flags                    = flags,
    .pApplicationInfo         = &appInfo,
    .enabledLayerCount        = static_cast<u32>(layers.size()),
    .ppEnabledLayerNames      = layers.data(),
    .enabledExtensionCount    = static_cast<u32>(extensions.size()),
    .ppEnabledExtensionNames  = extensions.data(),
  });
}

}