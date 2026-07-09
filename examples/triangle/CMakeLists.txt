add_executable(triangle
  ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
)

target_compile_definitions(triangle PRIVATE
  SHADER_DIR="${CMAKE_CURRENT_SOURCE_DIR}/shaders"
)

target_link_libraries(triangle PRIVATE grf)
