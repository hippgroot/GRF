#include "internal/device.hpp"
#include "internal/log.hpp"
#include "internal/queue.hpp"

namespace fs = std::filesystem;

namespace grf {

Device::Device(const Info& info) : m_impl(std::make_unique<Impl>(info)) {}

Device::~Device() {}

const Queue& Device::graphicsQueue() const {
  return m_impl->m_graphicsQueue;
}

const Queue& Device::computeQueue() const {
  return m_impl->m_computeQueue;
}

void Device::wait(std::span<const Sync> syncs, u64 timeout) const {
  if (syncs.empty()) {
    log::warning("submitted 0 syncs to wait on");
    return;
  }

  std::vector<vk::Semaphore> semaphores;
  semaphores.reserve(syncs.size());

  std::vector<u64> values;
  values.reserve(syncs.size());

  for (const auto& sync : syncs) {
    if (sync.m_impl->type() != SyncType::Timeline) continue;

    semaphores.emplace_back(sync.m_impl->semaphore());
    values.emplace_back(sync.m_impl->value());
  }

  if (semaphores.empty()) return;

  vk::Result res = m_impl->m_device.waitSemaphores(vk::SemaphoreWaitInfo{
    .semaphoreCount = static_cast<u32>(semaphores.size()),
    .pSemaphores    = semaphores.data(),
    .pValues        = values.data()
  }, timeout);

  if (res != vk::Result::eSuccess)
    log::error("Hung waiting for syncs");
}

void Device::waitIdle() const {
  m_impl->m_device.waitIdle();
}

Sync Device::updateResources() const {
  return m_impl->m_resourceManager.flush();
}

Shader Device::compileGlslShader(ShaderStage stage, const fs::path& path) const {
  return m_impl->m_shaderManager.compile(m_impl->m_device, stage, path);
}

Pipeline Device::createComputePipeline(const Shader& shader) const {
  if (shader.m_impl->m_stage != vk::ShaderStageFlagBits::eCompute) {
    log::error("Tried to use {} shader to create compute pipeline",
      log::to_string(static_cast<ShaderStage>(shader.m_impl->m_stage))
    );
  }

  Pipeline pipeline;
  pipeline.m_impl = std::make_shared<Pipeline::Impl>(PipelineType::Compute,
    m_impl->m_device.createComputePipeline(nullptr, vk::ComputePipelineCreateInfo{
      .stage = {
        .stage  = shader.m_impl->m_stage,
        .module = shader.m_impl->m_module,
        .pName  = "main"
      },
      .layout = m_impl->m_pipelineLayout
    })
  );

  return pipeline;
}

Pipeline Device::createGraphicsPipeline(
  std::span<const Shader> shaders, const GraphicsPipelineSettings& settings
) const {
  if (shaders.empty())
    log::error("graphics pipeline creation received 0 shaders");

  if (settings.colorFormats.empty())
    log::error("Graphics pipeline must be given at least one color format");

  if (settings.blends.size() != settings.colorFormats.size()) {
    log::error(
      "Graphics pipeline BlendState count ({}) must be equal to the color format count ({})",
      settings.blends.size(), settings.colorFormats.size()
    );
  }

  std::vector<vk::PipelineShaderStageCreateInfo> stages;
  stages.reserve(shaders.size());

  u8 vertexCount = 0, fragmentCount = 0;
  for (const auto& shader : shaders) {
    ShaderStage stage = static_cast<ShaderStage>(shader.m_impl->m_stage);
    switch (stage) {
    case ShaderStage::Vertex:
      ++vertexCount;
      break;
    case ShaderStage::Fragment:
      ++fragmentCount;
      break;
    default:
      log::error("Tried to use {} shader to create graphics pipeline", log::to_string(stage));
    }

    stages.emplace_back(vk::PipelineShaderStageCreateInfo{
      .stage  = shader.m_impl->m_stage,
      .module = shader.m_impl->m_module,
      .pName  = "main"
    });
  }

  if (vertexCount == 0)
    log::error("graphics pipelines require 1 vertex shader");

  if (vertexCount > 1)
    log::error("graphics pipeline must only have 1 vertex shader");

  if (fragmentCount > 1)
    log::error("graphics pipeline must only have 1 fragment shader");

  vk::PipelineVertexInputStateCreateInfo vertexInput;

  vk::PipelineInputAssemblyStateCreateInfo inputAssembly{
    .topology = static_cast<vk::PrimitiveTopology>(settings.topology)
  };

  vk::PipelineViewportStateCreateInfo viewport{
    .viewportCount  = 1,
    .scissorCount   = 1
  };

  vk::PipelineRasterizationStateCreateInfo rasterization{
    .polygonMode  = static_cast<vk::PolygonMode>(settings.polygonMode),
    .cullMode     = static_cast<vk::CullModeFlagBits>(settings.cullMode),
    .frontFace    = static_cast<vk::FrontFace>(settings.frontFace),
    .lineWidth    = 1.0f
  };

  vk::PipelineMultisampleStateCreateInfo multisample{
    .rasterizationSamples = static_cast<vk::SampleCountFlagBits>(settings.sampleCount)
  };

  const bool hasDepth = settings.depthFormat != Format::undefined;
  vk::PipelineDepthStencilStateCreateInfo depthStencil{
    .depthTestEnable  = hasDepth && (settings.depthTest || settings.depthWrite),
    .depthWriteEnable = hasDepth && settings.depthWrite,
    .depthCompareOp   = static_cast<vk::CompareOp>(settings.depthCompareOp)
  };
  if (!hasDepth && (settings.depthTest || settings.depthWrite))
    log::warning("Depth test/write enabled but no format was given");
  if (hasDepth && !settings.depthTest && settings.depthWrite)
    log::warning("Depth test is disabled but depth write is enabled. Enabled depth testing to fix");

  std::vector<vk::PipelineColorBlendAttachmentState> blendAttachments(settings.colorFormats.size());
  for (std::size_t i = 0; i < blendAttachments.size(); ++i) {
    const auto& b = settings.blends[i];
    blendAttachments[i] = vk::PipelineColorBlendAttachmentState{
      .blendEnable          = b.enable,
      .srcColorBlendFactor  = static_cast<vk::BlendFactor>(b.srcColorFactor),
      .dstColorBlendFactor  = static_cast<vk::BlendFactor>(b.dstColorFactor),
      .colorBlendOp         = static_cast<vk::BlendOp>(b.colorOp),
      .srcAlphaBlendFactor  = static_cast<vk::BlendFactor>(b.srcAlphaFactor),
      .dstAlphaBlendFactor  = static_cast<vk::BlendFactor>(b.dstAlphaFactor),
      .alphaBlendOp         = static_cast<vk::BlendOp>(b.alphaOp),
      .colorWriteMask       = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                              vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA
    };
  }

  vk::PipelineColorBlendStateCreateInfo colorBlend{
    .attachmentCount  = static_cast<uint32_t>(blendAttachments.size()),
    .pAttachments     = blendAttachments.data()
  };

  std::array<vk::DynamicState, 2> dynamicStates = {
    vk::DynamicState::eViewport,
    vk::DynamicState::eScissor
  };
  vk::PipelineDynamicStateCreateInfo dynamic{
    .dynamicStateCount  = static_cast<uint32_t>(dynamicStates.size()),
    .pDynamicStates     = dynamicStates.data()
  };

  std::vector<vk::Format> colorFormats;
  colorFormats.reserve(settings.colorFormats.size());
  for (auto f : settings.colorFormats)
    colorFormats.emplace_back(static_cast<vk::Format>(f));

  vk::StructureChain<vk::GraphicsPipelineCreateInfo, vk::PipelineRenderingCreateInfo> chain = {
    vk::GraphicsPipelineCreateInfo{
      .stageCount           = static_cast<uint32_t>(stages.size()),
      .pStages              = stages.data(),
      .pVertexInputState    = &vertexInput,
      .pInputAssemblyState  = &inputAssembly,
      .pViewportState       = &viewport,
      .pRasterizationState  = &rasterization,
      .pMultisampleState    = &multisample,
      .pDepthStencilState   = &depthStencil,
      .pColorBlendState     = &colorBlend,
      .pDynamicState        = &dynamic,
      .layout               = m_impl->m_pipelineLayout
    },
    vk::PipelineRenderingCreateInfo{
      .colorAttachmentCount     = static_cast<uint32_t>(colorFormats.size()),
      .pColorAttachmentFormats  = colorFormats.data(),
      .depthAttachmentFormat    = static_cast<vk::Format>(settings.depthFormat)
    }
  };

  Pipeline pipeline;
  pipeline.m_impl = std::make_shared<Pipeline::Impl>(PipelineType::Graphics,
    m_impl->m_device.createGraphicsPipeline(nullptr, chain.get<vk::GraphicsPipelineCreateInfo>())
  );

  return pipeline;
}

Buffer Device::createBuffer(const Buffer::Info& info) const {
  Buffer buffer = m_impl->m_allocator.allocateBuffer(info, m_impl->m_uniqueIndices);

  buffer.m_impl->setResourceManager(&m_impl->m_resourceManager);
  buffer.m_impl->setAddress(m_impl->m_device.getBufferAddress({ .buffer = buffer.m_impl->buffer() }));

  return buffer;
}

Ring<Buffer> Device::createBufferRing(u32 count, const Buffer::Info& info) const {
  std::vector<Buffer> buffers;
  buffers.reserve(count);

  for (u32 i = 0; i < count; ++i)
    buffers.emplace_back(createBuffer(info));

  return Ring<Buffer>(std::move(buffers));
}

Image Device::createImage(const Image::Info& info) const {
  Image image = m_impl->m_allocator.allocateImage(m_impl->m_device, info, m_impl->m_uniqueIndices);
  m_impl->m_descriptorHeap.registerImage(image);

  image.m_impl->setResourceManager(&m_impl->m_resourceManager);
  image.m_impl->setDescriptorHeap(&m_impl->m_descriptorHeap);

  return image;
}

Sampler Device::createSampler(const Sampler::Info& info) const {
  auto properties = m_impl->m_physicalDevice.getProperties();

  Sampler sampler;
  sampler.m_impl = std::make_shared<Sampler::Impl>(m_impl->m_device.createSampler(vk::SamplerCreateInfo{
    .magFilter        = static_cast<vk::Filter>(info.magFilter),
    .minFilter        = static_cast<vk::Filter>(info.minFilter),
    .mipmapMode       = static_cast<vk::SamplerMipmapMode>(info.mipmapMode),
    .addressModeU     = static_cast<vk::SamplerAddressMode>(info.addressModeU),
    .addressModeV     = static_cast<vk::SamplerAddressMode>(info.addressModeV),
    .addressModeW     = static_cast<vk::SamplerAddressMode>(info.addressModeW),
    .mipLodBias       = std::min(info.mipLodBias, properties.limits.maxSamplerLodBias),
    .anisotropyEnable = info.anisotropyEnable,
    .maxAnisotropy    = properties.limits.maxSamplerAnisotropy,
    .compareEnable    = info.compareEnable,
    .compareOp        = static_cast<vk::CompareOp>(info.compareOp),
    .minLod           = 0.0,
    .maxLod           = vk::LodClampNone,
    .borderColor      = vk::BorderColor::eFloatTransparentBlack
  }));

  m_impl->m_descriptorHeap.registerSampler(sampler);
  sampler.m_impl->setDescriptorHeap(&m_impl->m_descriptorHeap);

  return sampler;
}

}