#include "internal/device.hpp"

namespace grf {

Device::Device(const Info& info) : m_impl(std::make_unique<Impl>(info)) {}

Device::~Device() {}

const Queue& Device::graphicsQueue() const {
  return m_impl->m_graphicsQueue;
}

const Queue& Device::computeQueue() const {
  return m_impl->m_computeQueue;
}

}