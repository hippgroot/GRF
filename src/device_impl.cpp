#include "internal/device.hpp"
#include "internal/log.hpp"

#include "GLFW/glfw3.h"

#include <algorithm>

#ifdef GRF_ENABLE_VALIDATION
#define GRF_VALIDATION_LAYER_NAME "VK_LAYER_KHRONOS_validation"
#endif

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

namespace grf {

QueueFamilies::QueueFamilies(const vkr::PhysicalDevice& physicalDevice) {
  bool separateCompute = false, asyncCompute = false;
  bool separateTransfer = false, asyncTransfer = false;
  for (u8 i = 0; const auto& property : physicalDevice.getQueueFamilyProperties()) {
    if (property.queueFlags & vk::QueueFlagBits::eGraphics) {
      if (!hasGraphics())
        m_graphics = i;

      if (!separateCompute && (property.queueFlags & vk::QueueFlagBits::eCompute)) {
        m_compute = i;
        separateCompute = m_compute != m_graphics;
      }

      if (!separateTransfer && (property.queueFlags & vk::QueueFlagBits::eTransfer)) {
        m_transfer = i;
        separateTransfer = m_transfer != m_graphics && m_transfer != m_compute;
      }
    }
    else if (property.queueFlags & vk::QueueFlagBits::eCompute) {
      if (!asyncCompute) {
        m_compute = i;
        asyncCompute = separateCompute = true;
      }

      if (!separateTransfer && (property.queueFlags & vk::QueueFlagBits::eTransfer)) {
        m_transfer = i;
        separateTransfer = m_transfer != m_compute;
      }
    }
    else if (!asyncTransfer && property.queueFlags & vk::QueueFlagBits::eTransfer) {
      m_transfer = i;
      asyncTransfer = separateTransfer = true;
    }

    ++i;
  }
}

bool QueueFamilies::hasGraphics() const {
  return m_graphics != 0xFF;
}

bool QueueFamilies::hasCompute() const {
  return m_compute != 0xFF;
}

bool QueueFamilies::hasTransfer() const {
  return m_transfer != 0xFF;
}

bool QueueFamilies::invalid() const {
  return !(hasGraphics() && hasCompute() && hasTransfer());
}

u8 QueueFamilies::graphics() const {
  return m_graphics;
}

u8 QueueFamilies::compute() const {
  return m_compute;
}

u8 QueueFamilies::transfer() const {
  return m_transfer;
}

Device::Impl::Impl(const Device::Info& info) {
  VULKAN_HPP_DEFAULT_DISPATCHER.init();

  bool queryPortability = createInstance(info);
  VULKAN_HPP_DEFAULT_DISPATCHER.init(*m_instance);

  auto [queueFamilies, apiVersion, name] = choosePhysicalDevice();

  log::general("GRF Version: {}.{}.{}",
    GRF_VERSION_MAJOR, GRF_VERSION_MINOR, GRF_VERSION_PATCH
  );
  log::general("Vulkan Version: {}.{}.{}",
    VK_VERSION_MAJOR(apiVersion), VK_VERSION_MINOR(apiVersion), VK_VERSION_PATCH(apiVersion)
  );
  log::general("Device: {}", name);
}

bool Device::Impl::createInstance(const Device::Info& info) {
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
  bool enabledPortability = false;
  for (const auto& property : m_context.enumerateInstanceExtensionProperties()) {
    if (strcmp(property.extensionName, VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME) == 0) {
      extensions.emplace_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
      flags |= vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR;
      enabledPortability = true;

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

  return enabledPortability;
}

std::tuple<QueueFamilies, u32, str> Device::Impl::choosePhysicalDevice() {
  struct PDInfo {
    vkr::PhysicalDevice physicalDevice;
    QueueFamilies       queueFamilies;
    u32                 apiVersion;
    str                 name;
    u8                  typeRank;
  };

  std::vector<PDInfo> validPhysicalDevices;
  for (const auto& physicalDevice : m_instance.enumeratePhysicalDevices()) {
    QueueFamilies queueFamilies(physicalDevice);
    if (queueFamilies.invalid()) continue;

    auto properties = physicalDevice.getProperties();
    u8 rank;
    switch (properties.deviceType) {
    case vk::PhysicalDeviceType::eDiscreteGpu:
      rank = 0;
      break;
    case vk::PhysicalDeviceType::eIntegratedGpu:
      rank = 1;
      break;
    case vk::PhysicalDeviceType::eCpu:
      rank = 2;
      break;
    case vk::PhysicalDeviceType::eVirtualGpu:
      rank = 3;
      break;
    default:
      rank = 4;
    }

    validPhysicalDevices.emplace_back(PDInfo{
      .physicalDevice = physicalDevice,
      .queueFamilies  = queueFamilies,
      .apiVersion     = properties.apiVersion,
      .name           = str(properties.deviceName),
      .typeRank       = rank
    });
  }

  if (validPhysicalDevices.empty())
    log::panic("Hardware unsupported");

  std::ranges::sort(validPhysicalDevices, std::ranges::less{}, &PDInfo::typeRank);
  auto& chosen = validPhysicalDevices[0];

  m_physicalDevice = chosen.physicalDevice;
  return { chosen.queueFamilies, chosen.apiVersion, chosen.name };
}

}