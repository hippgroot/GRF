add_executable(triangle
  ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
)

target_link_libraries(triangle PRIVATE grf)
