cmake_minimum_required(VERSION 4.2)

option(GRF_BUILD_EXAMPLES "Compile the GRF examples" ON)

project(GrootRenderingFramework
  VERSION 0.1.0
  LANGUAGES CXX
)

find_package(Vulkan REQUIRED)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED On)

set(GRF_SOURCE_DIR ${CMAKE_SOURCE_DIR}/src)
set(GRF_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/include)

set(GLFW_BUILD_DOCS OFF)
set(GLFW_BUILD_TESTS OFF)
set(GLFW_BUILD_EXAMPLES OFF)
add_subdirectory(external/glfw)

add_library(grf
  ${GRF_INCLUDE_DIR}/grf/grf.hpp
  ${GRF_INCLUDE_DIR}/grf/types.hpp
  ${GRF_INCLUDE_DIR}/grf/window.hpp

  ${GRF_INCLUDE_DIR}/internal/device.hpp

  ${GRF_SOURCE_DIR}/device.cpp
  ${GRF_SOURCE_DIR}/device_impl.cpp
  ${GRF_SOURCE_DIR}/window.cpp
)

target_include_directories(grf PUBLIC ${GRF_INCLUDE_DIR})

target_compile_definitions(grf PRIVATE
  VULKAN_HPP_NO_CONSTRUCTORS
  VULKAN_HPP_DISPATCH_LOADER_DYNAMIC
  GRF_VERSION_MAJOR=${CMAKE_PROJECT_VERSION_MAJOR}
  GRF_VERSION_MINOR=${CMAKE_PROJECT_VERSION_MINOR}
  GRF_VERSION_PATCH=${CMAKE_PROJECT_VERSION_PATCH}
  $<$<CONFIG:Debug>:GRF_ENABLE_VALIDATION>
)

target_link_libraries(grf PRIVATE
  Vulkan::Vulkan
  glfw
)

if(GRF_BUILD_EXAMPLES)
  add_subdirectory(${CMAKE_SOURCE_DIR}/examples)
endif()
