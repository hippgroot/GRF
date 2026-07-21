#include "internal/device.hpp"
#include "internal/log.hpp"
#include "internal/queue.hpp"

#include "GLFW/glfw3.h"
#include <vulkan/vulkan_beta.h>

#include <algorithm>
#include <set>

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

      if (!separateTransfer) {
        m_transfer = i;
        separateTransfer = m_transfer != m_graphics && m_transfer != m_compute;
      }
    }
    else if (property.queueFlags & vk::QueueFlagBits::eCompute) {
      if (!asyncCompute) {
        m_compute = i;
        asyncCompute = separateCompute = true;
      }

      if (!separateTransfer) {
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
  return m_graphics != VK_QUEUE_FAMILY_IGNORED;
}

bool QueueFamilies::hasCompute() const {
  return m_compute != VK_QUEUE_FAMILY_IGNORED;
}

bool QueueFamilies::hasTransfer() const {
  return m_transfer != VK_QUEUE_FAMILY_IGNORED;
}

bool QueueFamilies::invalid() const {
  return !(hasGraphics() && hasCompute() && hasTransfer());
}

u32 QueueFamilies::graphics() const {
  return m_graphics;
}

u32 QueueFamilies::compute() const {
  return m_compute;
}

u32 QueueFamilies::transfer() const {
  return m_transfer;
}

Device::Impl::Impl(const Device::Info& info) {
  VULKAN_HPP_DEFAULT_DISPATCHER.init();

  bool queryPortability = createInstance(info);
  VULKAN_HPP_DEFAULT_DISPATCHER.init(*m_instance);

  auto [queueFamilies, apiVersion, name] = choosePhysicalDevice();
  createDevice(queueFamilies, queryPortability);
  VULKAN_HPP_DEFAULT_DISPATCHER.init(*m_device);

  m_graphicsQueue.m_impl = std::make_shared<Queue::Impl>(m_device, queueFamilies.graphics());
  m_computeQueue.m_impl = std::make_shared<Queue::Impl>(m_device, queueFamilies.compute());
  m_transferQueue.m_impl = std::make_shared<Queue::Impl>(m_device, queueFamilies.transfer());

  m_uniqueIndices = std::set<u32>{
    queueFamilies.graphics(), queueFamilies.compute(), queueFamilies.transfer()
  };

  createLayouts();

  m_allocator.init(Allocator::Info{
    .instance       = m_instance,
    .physicalDevice = m_physicalDevice,
    .device         = m_device,
    .apiVersion     = apiVersion
  });

  std::vector<Buffer> buffers;
  buffers.reserve(info.stagingCount);
  for (u32 i = 0; i < info.stagingCount; ++i) {
    Buffer buffer = m_allocator.allocateBuffer(Buffer::Info{
      .size   = info.stagingSize_MB * 1024u * 1024u,
      .intent = BufferIntent::FrequentUpdate
    }, m_uniqueIndices);
    buffer.m_impl->setResourceManager(&m_resourceManager);
    buffers.emplace_back(buffer);
  }
  m_resourceManager.init(&m_device, m_transferQueue, Ring<Buffer>(buffers), info.stagingCount);

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

  std::vector<const char*> layers;
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
  const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&count);

  std::vector<const char*> extensions(glfwExtensions, glfwExtensions + count);
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

Device::Impl::PDOutput Device::Impl::choosePhysicalDevice() {
  struct PDInfo {
    vkr::PhysicalDevice physicalDevice;
    QueueFamilies       queueFamilies;
    u32                 apiVersion;
    str                 name;
    u8                  typeRank;
  };

  std::vector<PDInfo> validPhysicalDevices;
  for (const auto& physicalDevice : m_instance.enumeratePhysicalDevices()) {
    auto properties = physicalDevice.getProperties();
    if (properties.apiVersion < VK_API_VERSION_1_4) continue;

    bool hasSwapchainSupport = false;
    for (const auto& property : physicalDevice.enumerateDeviceExtensionProperties()) {
      if (strcmp(property.extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0) {
        hasSwapchainSupport = true;
        break;
      }
    }
    if (!hasSwapchainSupport) continue;

    QueueFamilies queueFamilies(physicalDevice);
    if (queueFamilies.invalid()) continue;

    auto features = physicalDevice.getFeatures2<
      vk::PhysicalDeviceFeatures2,
      vk::PhysicalDeviceVulkan12Features
    >();

    auto f2 = features.get<vk::PhysicalDeviceFeatures2>();
    auto f12 = features.get<vk::PhysicalDeviceVulkan12Features>();

    if (
      !f2.features.shaderStorageImageReadWithoutFormat  ||
      !f2.features.shaderStorageImageWriteWithoutFormat ||
      !f12.descriptorIndexing                           ||
      !f12.shaderStorageImageArrayNonUniformIndexing    ||
      !f12.descriptorBindingVariableDescriptorCount
    ) continue;

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
    log::error("Hardware unsupported");

  std::ranges::sort(validPhysicalDevices, std::ranges::less{}, &PDInfo::typeRank);
  auto& chosen = validPhysicalDevices[0];

  m_physicalDevice = chosen.physicalDevice;
  return { chosen.queueFamilies, chosen.apiVersion, chosen.name };
}

void Device::Impl::createDevice(const QueueFamilies& queueFamilies, bool queryPortability) {
  std::set<u32> indices = { queueFamilies.graphics(), queueFamilies.compute(), queueFamilies.transfer() };
  std::vector<vk::DeviceQueueCreateInfo> queueInfos;
  f32 priority = 1.0f;
  for (u32 index : indices) {
    queueInfos.emplace_back(vk::DeviceQueueCreateInfo{
      .queueFamilyIndex = index,
      .queueCount       = 1,
      .pQueuePriorities = &priority
    });
  }

  std::vector<const char*> extensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
  if (queryPortability) {
    for (const auto& property : m_physicalDevice.enumerateDeviceExtensionProperties()) {
      if (strcmp(property.extensionName, VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME) == 0) {
        extensions.emplace_back(VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME);
        break;
      }
    }
  }

  vk::StructureChain<
    vk::DeviceCreateInfo,
    vk::PhysicalDeviceFeatures2,
    vk::PhysicalDeviceVulkan12Features,
    vk::PhysicalDeviceVulkan13Features,
    vk::PhysicalDeviceVulkan14Features
  > createChain{
    vk::DeviceCreateInfo{
      .queueCreateInfoCount     = static_cast<u32>(queueInfos.size()),
      .pQueueCreateInfos        = queueInfos.data(),
      .enabledExtensionCount    = static_cast<u32>(extensions.size()),
      .ppEnabledExtensionNames  = extensions.data(),
    },
    vk::PhysicalDeviceFeatures2{
      .features = {
        .shaderStorageImageReadWithoutFormat  = true,
        .shaderStorageImageWriteWithoutFormat = true,
      }
    },
    vk::PhysicalDeviceVulkan12Features{
      .descriptorIndexing                           = true,
      .shaderSampledImageArrayNonUniformIndexing    = true,
      .shaderStorageImageArrayNonUniformIndexing    = true,
      .descriptorBindingSampledImageUpdateAfterBind = true,
      .descriptorBindingStorageImageUpdateAfterBind = true,
      .descriptorBindingUpdateUnusedWhilePending    = true,
      .descriptorBindingPartiallyBound              = true,
      .descriptorBindingVariableDescriptorCount     = true,
      .runtimeDescriptorArray                       = true,
      .shaderSubgroupExtendedTypes                  = true,
      .hostQueryReset                               = true,
      .timelineSemaphore                            = true,
      .bufferDeviceAddress                          = true,
    },
    vk::PhysicalDeviceVulkan13Features{
      .shaderDemoteToHelperInvocation = true,
      .synchronization2               = true,
      .dynamicRendering               = true,
    },
    vk::PhysicalDeviceVulkan14Features{
      .dynamicRenderingLocalRead = true,
    }
  };

  m_device = m_physicalDevice.createDevice(createChain.get<vk::DeviceCreateInfo>());
}

void Device::Impl::createLayouts() {
  m_setLayout = m_device.createDescriptorSetLayout(vk::DescriptorSetLayoutCreateInfo{
    .bindingCount = 0,
    .pBindings    = nullptr
  });

  m_pipelineLayout = m_device.createPipelineLayout(vk::PipelineLayoutCreateInfo{
    .setLayoutCount         = 1,
    .pSetLayouts            = &*m_setLayout,
    .pushConstantRangeCount = 0,
    .pPushConstantRanges    = nullptr
  });
}

}