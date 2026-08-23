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

set(GRF_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(GRF_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake
  @ONLY
)
if(NOT TARGET uninstall)
  add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake
  )
endif()

set(GLFW_BUILD_DOCS OFF)
set(GLFW_BUILD_TESTS OFF)
set(GLFW_BUILD_EXAMPLES OFF)
set(GLFW_INSTALL OFF)
add_subdirectory(external/glfw)

add_subdirectory(external/vma)

add_library(glm INTERFACE)
target_include_directories(glm SYSTEM INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/external/glm>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/grf>
)
target_compile_definitions(glm INTERFACE
  GLM_FORCE_RADIANS
  GLM_FORCE_DEPTH_ZERO_TO_ONE
)

add_library(stb INTERFACE)
target_include_directories(stb SYSTEM INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/external/stb)

add_library(grf
  ${GRF_INCLUDE_DIR}/grf/device.hpp
  ${GRF_INCLUDE_DIR}/grf/gui.hpp
  ${GRF_INCLUDE_DIR}/grf/pipeline.hpp
  ${GRF_INCLUDE_DIR}/grf/queue.hpp
  ${GRF_INCLUDE_DIR}/grf/resources.hpp
  ${GRF_INCLUDE_DIR}/grf/swapchain.hpp
  ${GRF_INCLUDE_DIR}/grf/types.hpp
  ${GRF_INCLUDE_DIR}/grf/window.hpp

  ${GRF_INCLUDE_DIR}/internal/device.hpp
  ${GRF_INCLUDE_DIR}/internal/gui.hpp
  ${GRF_INCLUDE_DIR}/internal/log.hpp
  ${GRF_INCLUDE_DIR}/internal/pipeline.hpp
  ${GRF_INCLUDE_DIR}/internal/queue.hpp
  ${GRF_INCLUDE_DIR}/internal/resources.hpp
  ${GRF_INCLUDE_DIR}/internal/swapchain.hpp

  ${GRF_SOURCE_DIR}/device.cpp
  ${GRF_SOURCE_DIR}/device_impl.cpp
  ${GRF_SOURCE_DIR}/gui.cpp
  ${GRF_SOURCE_DIR}/pipeline.cpp
  ${GRF_SOURCE_DIR}/queue.cpp
  ${GRF_SOURCE_DIR}/resources.cpp
  ${GRF_SOURCE_DIR}/swapchain.cpp
  ${GRF_SOURCE_DIR}/window.cpp

  ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui/imgui.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui/imgui_draw.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui/imgui_tables.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui/imgui_widgets.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui/backends/imgui_impl_glfw.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui/backends/imgui_impl_vulkan.cpp
)

set_source_files_properties(
  ${GRF_SOURCE_DIR}/resources.cpp
  PROPERTIES COMPILE_FLAGS "-Wno-nullability-completeness"
)

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui/imgui.h
  ${CMAKE_CURRENT_BINARY_DIR}/include/grf/imgui.h
  COPYONLY
)
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui/imconfig.h
  ${CMAKE_CURRENT_BINARY_DIR}/include/grf/imconfig.h
  COPYONLY
)

target_include_directories(grf
  PUBLIC
    $<BUILD_INTERFACE:${GRF_INCLUDE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui
    ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui/backends
    ${CMAKE_CURRENT_SOURCE_DIR}/external/stb
    ${CMAKE_CURRENT_SOURCE_DIR}/external/glm
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

target_link_libraries(grf
  PUBLIC glm
  PRIVATE
    Vulkan::Vulkan
    Vulkan::shaderc_combined
    $<BUILD_INTERFACE:glfw>
    $<BUILD_INTERFACE:GPUOpen::VulkanMemoryAllocator>
)

find_package(Threads REQUIRED)
get_target_property(GLFW_LINK_DEPS glfw INTERFACE_LINK_LIBRARIES)
target_link_libraries(grf INTERFACE ${GLFW_LINK_DEPS})

if(APPLE)
  add_custom_command(TARGET grf POST_BUILD
    COMMAND libtool -static -no_warning_for_no_symbols
      -o $<TARGET_FILE:grf>.merged $<TARGET_FILE:grf> $<TARGET_FILE:glfw>
    COMMAND ${CMAKE_COMMAND} -E rename $<TARGET_FILE:grf>.merged $<TARGET_FILE:grf>
    VERBATIM
  )
elseif(MSVC)
  add_custom_command(TARGET grf POST_BUILD
    COMMAND ${CMAKE_AR} /OUT:$<TARGET_FILE:grf>.merged
      $<TARGET_FILE:grf> $<TARGET_FILE:glfw>
    COMMAND ${CMAKE_COMMAND} -E rename $<TARGET_FILE:grf>.merged $<TARGET_FILE:grf>
    VERBATIM
  )
else()
  file(GENERATE OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/grf-merge.mri CONTENT
"create $<TARGET_FILE:grf>.merged
addlib $<TARGET_FILE:grf>
addlib $<TARGET_FILE:glfw>
save
end
")
  add_custom_command(TARGET grf POST_BUILD
    COMMAND ${CMAKE_COMMAND}
      -DAR=${CMAKE_AR}
      -DMRI=${CMAKE_CURRENT_BINARY_DIR}/grf-merge.mri
      -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/merge-archives.cmake
    COMMAND ${CMAKE_COMMAND} -E rename $<TARGET_FILE:grf>.merged $<TARGET_FILE:grf>
    VERBATIM
  )
endif()

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

install(TARGETS grf glm EXPORT grf-targets
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(DIRECTORY ${GRF_INCLUDE_DIR}/grf
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/external/glm/glm
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/grf
  FILES_MATCHING
    PATTERN "*.hpp"
    PATTERN "*.h"
    PATTERN "*.inl"
)

install(FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui/imgui.h
  ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui/imconfig.h
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/grf
)

install(EXPORT grf-targets
  FILE grf-targets.cmake
  NAMESPACE GRF::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/grf
)

configure_package_config_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/cmake/grf-config.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/grf-config.cmake
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/grf
)

write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/grf-config-version.cmake
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMinorVersion
)

install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/grf-config.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/grf-config-version.cmake
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/grf
)

if(GRF_BUILD_EXAMPLES)
  add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/examples)
endif()
