#include "grf/queue.hpp"
#include "internal/log.hpp"
#include "internal/pipeline.hpp"
#include "internal/queue.hpp"
#include "internal/resources.hpp"
#include "internal/swapchain.hpp"

namespace grf {

template class Ring<CommandBuffer>;
template class Ring<Sync>;

Sync::Impl::Impl(vk::Semaphore semaphore)
: m_semaphore(semaphore), m_value(0), m_type(SyncType::Binary)
{}

Sync::Impl::Impl(vk::Semaphore semaphore, u64 value)
: m_semaphore(semaphore), m_value(value), m_type(SyncType::Timeline)
{}

vk::Semaphore Sync::Impl::semaphore() const {
  return m_semaphore;
}

u64 Sync::Impl::value() const {
  return m_value;
}

SyncType Sync::Impl::type() const {
  return m_type;
}

void CommandBuffer::begin() const {
  m_impl->m_cmd.reset();
  m_impl->m_cmd.begin({});

  if (!m_impl->m_set || !m_impl->m_layout) return;

  m_impl->m_cmd.bindDescriptorSets2(vk::BindDescriptorSetsInfo{
    .stageFlags         = m_impl->m_stageFlags,
    .layout             = m_impl->m_layout,
    .firstSet           = 0,
    .descriptorSetCount = 1,
    .pDescriptorSets    = &m_impl->m_set
  });
}

void CommandBuffer::end() const {
  m_impl->m_cmd.end();
}

void CommandBuffer::beginRendering(
  u32 width, u32 height, std::span<const ColorAttachment> colors
) const {
  std::vector<vk::RenderingAttachmentInfo> colorInfos;
  colorInfos.reserve(colors.size());
  for (const auto& c : colors) {
    vk::ClearValue cv{ .color = vk::ClearColorValue(c.clearValue) };
    colorInfos.push_back(vk::RenderingAttachmentInfo{
      .imageView   = c.img.m_impl->view(),
      .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
      .loadOp      = static_cast<vk::AttachmentLoadOp>(c.loadOp),
      .storeOp     = static_cast<vk::AttachmentStoreOp>(c.storeOp),
      .clearValue  = cv
    });
  }

  vk::Rect2D area{
    .offset = { 0, 0 },
    .extent = { width, height }
  };

  m_impl->m_cmd.beginRendering(vk::RenderingInfo{
    .renderArea           = area,
    .layerCount           = 1,
    .colorAttachmentCount = static_cast<uint32_t>(colorInfos.size()),
    .pColorAttachments    = colorInfos.data(),
    .pDepthAttachment     = nullptr
  });

  vk::Viewport viewport{
    .x        = 0.0f,
    .y        = 0.0f,
    .width    = static_cast<f32>(width),
    .height   = static_cast<f32>(height),
    .minDepth = 0.0f,
    .maxDepth = 1.0f
  };

  m_impl->m_cmd.setViewport(0, viewport);
  m_impl->m_cmd.setScissor(0, area);
}

void CommandBuffer::endRendering() const {
  m_impl->m_cmd.endRendering();
}

void CommandBuffer::bindIndexBuffer(const Buffer& buffer, IndexType type) const {
  m_impl->m_cmd.bindIndexBuffer(buffer.m_impl->buffer(), 0, static_cast<vk::IndexType>(type));
}

void CommandBuffer::bindPipeline(const Pipeline& pipeline) const {
  vk::PipelineBindPoint bindPoint = pipeline.m_impl->type() == PipelineType::Graphics ?
    vk::PipelineBindPoint::eGraphics : vk::PipelineBindPoint::eCompute;

  m_impl->m_cmd.bindPipeline(
    bindPoint,
    pipeline.m_impl->pipeline()
  );
}

void CommandBuffer::draw(u32 vertexCount) const {
  m_impl->m_cmd.draw(vertexCount, 1, 0, 0);
}

void CommandBuffer::drawIndexed(u32 indexCount) const {
  m_impl->m_cmd.drawIndexed(indexCount, 1, 0, 0, 0);
}

void CommandBuffer::transition(SwapchainImage image, Layout layout) const {
  vk::ImageLayout to = static_cast<vk::ImageLayout>(layout);

  vk::ImageMemoryBarrier2 barrier{
    .srcStageMask     = vk::PipelineStageFlagBits2::eAllCommands,
    .srcAccessMask    = vk::AccessFlagBits2::eMemoryRead | vk::AccessFlagBits2::eMemoryWrite,
    .dstStageMask     = vk::PipelineStageFlagBits2::eAllCommands,
    .dstAccessMask    = vk::AccessFlagBits2::eMemoryRead | vk::AccessFlagBits2::eMemoryWrite,
    .oldLayout        = image.m_impl->layout(),
    .newLayout        = to,
    .image            = image.m_impl->image(),
    .subresourceRange = {
      .aspectMask = vk::ImageAspectFlagBits::eColor,
      .levelCount = 1,
      .layerCount = 1
    }
  };

  m_impl->m_cmd.pipelineBarrier2(vk::DependencyInfo{
    .imageMemoryBarrierCount  = 1,
    .pImageMemoryBarriers     = &barrier
  });

  image.m_impl->setLayout(to);
}

void CommandBuffer::transition(Image image, Layout layout) const  {
  vk::ImageLayout to = static_cast<vk::ImageLayout>(layout);

  vk::ImageMemoryBarrier2 barrier{
    .srcStageMask     = vk::PipelineStageFlagBits2::eAllCommands,
    .srcAccessMask    = vk::AccessFlagBits2::eMemoryRead | vk::AccessFlagBits2::eMemoryWrite,
    .dstStageMask     = vk::PipelineStageFlagBits2::eAllCommands,
    .dstAccessMask    = vk::AccessFlagBits2::eMemoryRead | vk::AccessFlagBits2::eMemoryWrite,
    .oldLayout        = image.m_impl->layout(),
    .newLayout        = to,
    .image            = image.m_impl->image(),
    .subresourceRange = {
      .aspectMask = vk::ImageAspectFlagBits::eColor,
      .levelCount = vk::RemainingMipLevels,
      .layerCount = 1
    }
  };

  m_impl->m_cmd.pipelineBarrier2(vk::DependencyInfo{
    .imageMemoryBarrierCount  = 1,
    .pImageMemoryBarriers     = &barrier
  });

  image.m_impl->setLayout(to);
}

void CommandBuffer::copyBuffer(Buffer src, Buffer dst, u32 srcOffset, u32 dstOffset, u32 size) const {
  m_impl->m_cmd.copyBuffer(src.m_impl->buffer(), dst.m_impl->buffer(), vk::BufferCopy{
    .srcOffset  = srcOffset,
    .dstOffset  = dstOffset,
    .size       = size
  });
}

void CommandBuffer::bufferImageCopy(Buffer src, Image dst, u32 bufferOffset, u32 mip) const {
  bool isDepth = dst.m_impl->format() >= vk::Format::eD16Unorm && dst.m_impl->format() <= vk::Format::eD32SfloatS8Uint;

  vk::BufferImageCopy2 region{
    .bufferOffset     = bufferOffset,
    .imageSubresource = vk::ImageSubresourceLayers{
      .aspectMask     = isDepth ? vk::ImageAspectFlagBits::eDepth : vk::ImageAspectFlagBits::eColor,
      .mipLevel       = mip,
      .baseArrayLayer = 0,
      .layerCount     = 1
    },
    .imageExtent = { std::max(1u, dst.width() >> mip), std::max(1u, dst.height() >> mip), std::max(1u, dst.depth() >> mip) }
  };

  m_impl->m_cmd.copyBufferToImage2(vk::CopyBufferToImageInfo2{
    .srcBuffer      = src.m_impl->buffer(),
    .dstImage       = dst.m_impl->image(),
    .dstImageLayout = dst.m_impl->layout(),
    .regionCount    = 1,
    .pRegions       = &region
  });
}

void CommandBuffer::generateMips(Image image, Layout finalLayout) const {
  u32 mips = image.m_impl->mips();

  if (mips == 1) {
    transition(image, finalLayout);
    return;
  }

  vk::Image img = image.m_impl->image();

  auto barrier = [&](
    u32 baseMip, u32 count,
    vk::ImageLayout from, vk::ImageLayout to,
    vk::PipelineStageFlags2 srcStage, vk::AccessFlags2 srcAccess,
    vk::PipelineStageFlags2 dstStage, vk::AccessFlags2 dstAccess
  ) {
    vk::ImageMemoryBarrier2 b{
      .srcStageMask     = srcStage,
      .srcAccessMask    = srcAccess,
      .dstStageMask     = dstStage,
      .dstAccessMask    = dstAccess,
      .oldLayout        = from,
      .newLayout        = to,
      .image            = img,
      .subresourceRange = {
        .aspectMask   = vk::ImageAspectFlagBits::eColor,
        .baseMipLevel = baseMip,
        .levelCount   = count,
        .layerCount   = 1
      }
    };

    m_impl->m_cmd.pipelineBarrier2(vk::DependencyInfo{
      .imageMemoryBarrierCount = 1,
      .pImageMemoryBarriers    = &b
    });
  };

  auto extent = [&](u32 mip) {
    return vk::Offset3D{
      static_cast<i32>(std::max(1u, image.width()  >> mip)),
      static_cast<i32>(std::max(1u, image.height() >> mip)),
      static_cast<i32>(std::max(1u, image.depth()  >> mip))
    };
  };

  vk::ImageLayout curr = image.m_impl->layout();

  barrier(0, 1, curr, vk::ImageLayout::eTransferSrcOptimal,
    vk::PipelineStageFlagBits2::eAllCommands,
    vk::AccessFlagBits2::eMemoryRead | vk::AccessFlagBits2::eMemoryWrite,
    vk::PipelineStageFlagBits2::eTransfer,
    vk::AccessFlagBits2::eTransferRead);

  barrier(1, mips - 1, curr, vk::ImageLayout::eTransferDstOptimal,
    vk::PipelineStageFlagBits2::eAllCommands,
    vk::AccessFlagBits2::eMemoryRead | vk::AccessFlagBits2::eMemoryWrite,
    vk::PipelineStageFlagBits2::eTransfer,
    vk::AccessFlagBits2::eTransferWrite
  );

  for (u32 i = 1; i < mips; ++i) {
    vk::ImageBlit2 region{
      .srcSubresource = {
        .aspectMask = vk::ImageAspectFlagBits::eColor,
        .mipLevel   = i - 1,
        .layerCount = 1
      },
      .srcOffsets = std::array{ vk::Offset3D{}, extent(i - 1) },
      .dstSubresource = {
        .aspectMask = vk::ImageAspectFlagBits::eColor,
        .mipLevel   = i,
        .layerCount = 1
      },
      .dstOffsets = std::array{ vk::Offset3D{}, extent(i) }
    };

    m_impl->m_cmd.blitImage2(vk::BlitImageInfo2{
      .srcImage       = img,
      .srcImageLayout = vk::ImageLayout::eTransferSrcOptimal,
      .dstImage       = img,
      .dstImageLayout = vk::ImageLayout::eTransferDstOptimal,
      .regionCount    = 1,
      .pRegions       = &region,
      .filter         = vk::Filter::eLinear
    });

    barrier(i, 1,
      vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eTransferSrcOptimal,
      vk::PipelineStageFlagBits2::eTransfer, vk::AccessFlagBits2::eTransferWrite,
      vk::PipelineStageFlagBits2::eTransfer, vk::AccessFlagBits2::eTransferRead
    );
  }

  barrier(0, mips,
    vk::ImageLayout::eTransferSrcOptimal, static_cast<vk::ImageLayout>(finalLayout),
    vk::PipelineStageFlagBits2::eTransfer, vk::AccessFlagBits2::eTransferWrite,
    vk::PipelineStageFlagBits2::eAllCommands,
    vk::AccessFlagBits2::eMemoryRead | vk::AccessFlagBits2::eMemoryWrite
  );

  image.m_impl->setLayout(static_cast<vk::ImageLayout>(finalLayout));
}

CommandBuffer::Impl::Impl(vkr::CommandBuffer&& cmd, vk::DescriptorSet set, vk::PipelineLayout layout, vk::ShaderStageFlags stageFlags)
: m_cmd(std::move(cmd)), m_set(set), m_layout(layout), m_stageFlags(stageFlags)
{}

const vkr::CommandBuffer& CommandBuffer::Impl::commandBuffer() const {
  return m_cmd;
}

Ring<CommandBuffer> Queue::getCmdRing(u32 count) const {
  if (count == 0)
    log::error("tried to create command buffer ring of size 0");

  std::lock_guard lock(m_impl->m_mutex);

  vkr::CommandBuffers cmds = m_impl->m_device.allocateCommandBuffers(vk::CommandBufferAllocateInfo{
    .commandPool = m_impl->m_cmdPool,
    .commandBufferCount = count
  });

  std::vector<CommandBuffer> ringData(count, CommandBuffer());
  for (u32 i = 0; i < count; ++i)
    ringData[i].m_impl = std::make_shared<CommandBuffer::Impl>(std::move(cmds[i]), m_impl->m_set, m_impl->m_layout, m_impl->m_stageFlags);

  return Ring<CommandBuffer>(std::move(ringData));
}

Ring<Sync> Queue::getSyncRing(u32 count) const {
  if (count == 0)
    log::error("tried to create sync ring of size 0");

  std::lock_guard lock(m_impl->m_mutex);

  std::vector<Sync> syncs(count, Sync());
  for (u32 i = 0; i < count; ++i)
    syncs[i].m_impl = std::make_shared<Sync::Impl>(*m_impl->m_timeline, m_impl->m_value);

  return Ring<Sync>(std::move(syncs));
}

Sync Queue::submit(std::span<const CommandBuffer> cmds, std::span<const Sync> syncs) const {
  if (cmds.empty()) {
    log::warning("Submitted 0 command buffers to queue");
    Sync err;
    err.m_impl = std::make_shared<Sync::Impl>(*m_impl->m_timeline, m_impl->m_value);
    return err;
  }

  std::vector<vk::CommandBufferSubmitInfo> cmdInfos;
  cmdInfos.reserve(cmds.size());
  for (const auto& cmd : cmds) {
    cmdInfos.emplace_back(vk::CommandBufferSubmitInfo{
      .commandBuffer = *cmd.m_impl->commandBuffer()
    });
  }

  std::vector<vk::SemaphoreSubmitInfo> waitInfos;
  waitInfos.reserve(syncs.size());
  for (const auto& sync : syncs) {
    waitInfos.emplace_back(vk::SemaphoreSubmitInfo{
      .semaphore  = sync.m_impl->semaphore(),
      .value      = sync.m_impl->value(),
      .stageMask  = vk::PipelineStageFlagBits2::eAllCommands
    });
  }

  std::lock_guard lock(m_impl->m_mutex);

  Sync signal;
  signal.m_impl = std::make_shared<Sync::Impl>(*m_impl->m_timeline, ++m_impl->m_value);

  vk::SemaphoreSubmitInfo signalInfo{
    .semaphore  = signal.m_impl->semaphore(),
    .value      = signal.m_impl->value(),
    .stageMask  = vk::PipelineStageFlagBits2::eAllCommands
  };

  m_impl->m_queue.submit2(vk::SubmitInfo2{
    .waitSemaphoreInfoCount   = static_cast<u32>(waitInfos.size()),
    .pWaitSemaphoreInfos      = waitInfos.data(),
    .commandBufferInfoCount   = static_cast<u32>(cmdInfos.size()),
    .pCommandBufferInfos      = cmdInfos.data(),
    .signalSemaphoreInfoCount = 1,
    .pSignalSemaphoreInfos    = &signalInfo
  });

  return signal;
}

void Queue::present(const SwapchainImage& image, std::span<const Sync> syncs) const {
  std::vector<vk::SemaphoreSubmitInfo> waitInfos;
  waitInfos.reserve(syncs.size());
  for (const auto& sync : syncs) {
    waitInfos.emplace_back(vk::SemaphoreSubmitInfo{
      .semaphore  = sync.m_impl->semaphore(),
      .value      = sync.m_impl->value(),
      .stageMask  = vk::PipelineStageFlagBits2::eAllCommands
    });
  }

  vk::SemaphoreSubmitInfo signalInfo{
    .semaphore  = image.m_impl->semaphore(),
    .stageMask  = vk::PipelineStageFlagBits2::eAllCommands
  };

  vk::Semaphore semaphore = image.m_impl->semaphore();
  vk::SwapchainKHR swapchain = image.m_impl->swapchain();
  u32 index = image.m_impl->index();

  std::lock_guard lock(m_impl->m_mutex);

  m_impl->m_queue.submit2(vk::SubmitInfo2{
    .waitSemaphoreInfoCount   = static_cast<u32>(waitInfos.size()),
    .pWaitSemaphoreInfos      = waitInfos.data(),
    .signalSemaphoreInfoCount = 1,
    .pSignalSemaphoreInfos    = &signalInfo
  });

  auto res = m_impl->m_queue.presentKHR(vk::PresentInfoKHR{
    .waitSemaphoreCount = 1,
    .pWaitSemaphores    = &semaphore,
    .swapchainCount     = 1,
    .pSwapchains        = &swapchain,
    .pImageIndices      = &index
  });

  if (res == vk::Result::eSuboptimalKHR)
    image.m_impl->flagSwapchainForRecreation();
}

Queue::Impl::Impl(const vkr::Device& device, u32 index)
: m_device(device), m_index(index) {
  m_queue = m_device.getQueue(m_index, 0);

  m_cmdPool = m_device.createCommandPool(vk::CommandPoolCreateInfo{
    .flags            = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
    .queueFamilyIndex = index
  });

  vk::StructureChain<vk::SemaphoreCreateInfo, vk::SemaphoreTypeCreateInfo> chain{
    vk::SemaphoreCreateInfo{},
    vk::SemaphoreTypeCreateInfo{
      .semaphoreType  = vk::SemaphoreType::eTimeline,
      .initialValue   = m_value
    }
  };

  m_timeline = m_device.createSemaphore(chain.get<vk::SemaphoreCreateInfo>());
}

void Queue::Impl::setDescriptorHeapInfo(vk::DescriptorSet set, vk::PipelineLayout layout, vk::ShaderStageFlags stageFlags) {
  m_set = set;
  m_layout = layout;
  m_stageFlags = stageFlags;
}

}