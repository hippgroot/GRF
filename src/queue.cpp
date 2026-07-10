#include "internal/log.hpp"
#include "internal/queue.hpp"

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

CommandBuffer::Impl::Impl(vkr::CommandBuffer&& cmd) : m_cmd(std::move(cmd)) {}

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
    ringData[i].m_impl = std::make_shared<CommandBuffer::Impl>(std::move(cmds[i]));

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

Sync Queue::submit(const std::vector<CommandBuffer>& cmds, const std::vector<Sync>& syncs) const {
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