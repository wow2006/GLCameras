# ${CMAKE_SOURCE_DIR}/utilities/CMakeLists.txt
add_library(
  utilities
  STATIC
  input.hpp
  input.cpp
)

add_library(
  camera::utilities
  ALIAS
  utilities
)

target_include_directories(
  utilities
  PUBLIC
  ${CMAKE_CURRENT_LIST_DIR}
)

target_link_libraries(
  utilities
  PUBLIC
  fmt::fmt
  glm::glm
  SDL2::SDL2
)