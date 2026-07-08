#include "internal/device.hpp"

namespace grf {

Device::Device(const Info& info) : m_impl(std::make_unique<Impl>(info)) {}
Device::~Device() {}

}