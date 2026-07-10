#include "internal/queue.hpp"

namespace grf {

template class Ring<CommandBuffer>;

CommandBuffer::Impl::Impl(vkr::CommandBuffer&& cmd) : m_cmd(std::move(cmd)) {}

Ring<CommandBuffer> Queue::getCmdRing(u32 count) const {
  std::lock_guard lock(m_impl->m_mutex);

  vkr::CommandBuffers cmds = m_impl->m_device.allocateCommandBuffers(vk::CommandBufferAllocateInfo{
    .commandPool = m_impl->m_cmdPool,
    .commandBufferCount = count
  });

  std::vector<CommandBuffer> ringData(count, CommandBuffer());
  for (u32 i = 0; i < count; ++i)
    ringData[i].m_impl = std::make_shared<CommandBuffer::Impl>(std::move(cmds[i]));

  return Ring<CommandBuffer>(std::move(ringData));
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

}