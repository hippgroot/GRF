cmake_minimum_required(VERSION 4.2)

option(GRF_MAKE_EXAMPLES "Compile the GRF examples" ON)

project(GrootRenderingFramework
  VERSION 0.1.0
  LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED On)

set(GRF_SOURCE_DIR ${CMAKE_SOURCE_DIR}/src)
set(GRF_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/include)

add_library(grf
  ${GRF_INCLUDE_DIR}/grf/grf.hpp
  ${GRF_SOURCE_DIR}/grf.cpp
)

target_include_directories(grf PUBLIC ${GRF_INCLUDE_DIR})

if(GRF_MAKE_EXAMPLES)
  add_subdirectory(${CMAKE_SOURCE_DIR}/examples)
endif()
