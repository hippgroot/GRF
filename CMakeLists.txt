cmake_minimum_required(VERSION 4.2)

option(GRF_BUILD_EXAMPLES "Compile the GRF examples" ON)

project(GrootRenderingFramework
  VERSION 0.1.0
  LANGUAGES CXX
)

find_package(Vulkan REQUIRED COMPONENTS shaderc_combined)

include(GNUInstallDirs)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED On)

set(GRF_SOURCE_DIR ${CMAKE_SOURCE_DIR}/src)
set(GRF_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/include)

configure_file(
  ${CMAKE_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in
  ${CMAKE_BINARY_DIR}/cmake_uninstall.cmake
  @ONLY
)
add_custom_target(uninstall
  COMMAND ${CMAKE_COMMAND} -P ${CMAKE_BINARY_DIR}/cmake_uninstall.cmake
)

set(GLFW_BUILD_DOCS OFF)
set(GLFW_BUILD_TESTS OFF)
set(GLFW_BUILD_EXAMPLES OFF)
set(GLFW_INSTALL ON)
add_subdirectory(external/glfw)

add_subdirectory(external/vma)

add_library(stb INTERFACE)
target_include_directories(stb SYSTEM INTERFACE ${CMAKE_SOURCE_DIR}/external/stb)

add_library(grf
  ${GRF_INCLUDE_DIR}/grf/device.hpp
  ${GRF_INCLUDE_DIR}/grf/pipeline.hpp
  ${GRF_INCLUDE_DIR}/grf/queue.hpp
  ${GRF_INCLUDE_DIR}/grf/resources.hpp
  ${GRF_INCLUDE_DIR}/grf/swapchain.hpp
  ${GRF_INCLUDE_DIR}/grf/types.hpp
  ${GRF_INCLUDE_DIR}/grf/window.hpp

  ${GRF_INCLUDE_DIR}/internal/device.hpp
  ${GRF_INCLUDE_DIR}/internal/log.hpp
  ${GRF_INCLUDE_DIR}/internal/pipeline.hpp
  ${GRF_INCLUDE_DIR}/internal/queue.hpp
  ${GRF_INCLUDE_DIR}/internal/resources.hpp
  ${GRF_INCLUDE_DIR}/internal/swapchain.hpp

  ${GRF_SOURCE_DIR}/device.cpp
  ${GRF_SOURCE_DIR}/device_impl.cpp
  ${GRF_SOURCE_DIR}/pipeline.cpp
  ${GRF_SOURCE_DIR}/queue.cpp
  ${GRF_SOURCE_DIR}/resources.cpp
  ${GRF_SOURCE_DIR}/swapchain.cpp
  ${GRF_SOURCE_DIR}/window.cpp
)

set_source_files_properties(
  ${GRF_SOURCE_DIR}/resources.cpp
  PROPERTIES COMPILE_FLAGS "-Wno-nullability-completeness"
)

target_include_directories(grf
  PUBLIC
    $<BUILD_INTERFACE:${GRF_INCLUDE_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  PRIVATE ${CMAKE_SOURCE_DIR}/external/stb
)

target_compile_definitions(grf PRIVATE
  VULKAN_HPP_NO_CONSTRUCTORS=1
  VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1
  VMA_STATIC_VULKAN_FUNCTIONS=0
  VMA_DYNAMIC_VULKAN_FUNCTIONS=1
  GRF_VERSION_MAJOR=${CMAKE_PROJECT_VERSION_MAJOR}
  GRF_VERSION_MINOR=${CMAKE_PROJECT_VERSION_MINOR}
  GRF_VERSION_PATCH=${CMAKE_PROJECT_VERSION_PATCH}
  $<$<CONFIG:Debug>:GRF_ENABLE_VALIDATION>
)

target_link_libraries(grf PRIVATE
  Vulkan::Vulkan
  Vulkan::shaderc_combined
  $<BUILD_INTERFACE:glfw>
  $<INSTALL_INTERFACE:glfw>
  $<BUILD_INTERFACE:GPUOpen::VulkanMemoryAllocator>
)

if(MINGW)
  target_link_libraries(grf PRIVATE
    glslang
    glslang-default-resource-limits
    SPIRV
    SPIRV-Tools-opt
    SPIRV-Tools
  )
endif()

include(CMakePackageConfigHelpers)

install(TARGETS grf EXPORT grf-targets
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(DIRECTORY ${GRF_INCLUDE_DIR}/grf
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(EXPORT grf-targets
  FILE grf-targets.cmake
  NAMESPACE GRF::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/grf
)

configure_package_config_file(
  ${CMAKE_SOURCE_DIR}/cmake/grf-config.cmake.in
  ${CMAKE_BINARY_DIR}/grf-config.cmake
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/grf
)

write_basic_package_version_file(
  ${CMAKE_BINARY_DIR}/grf-config-version.cmake
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMinorVersion
)

install(FILES
  ${CMAKE_BINARY_DIR}/grf-config.cmake
  ${CMAKE_BINARY_DIR}/grf-config-version.cmake
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/grf
)

if(GRF_BUILD_EXAMPLES)
  add_subdirectory(${CMAKE_SOURCE_DIR}/examples)
endif()
