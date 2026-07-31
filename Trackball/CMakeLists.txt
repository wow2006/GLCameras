# ${CMAKE_SOURCE_DIR}/Trackball/CMakeLists.txt
project(Trackball LANGUAGES CXX)

add_executable(Trackball)

target_sources(Trackball PRIVATE main.cpp trackball.cpp)

target_link_libraries(
  Trackball
  PRIVATE fmt::fmt
          glbinding::glbinding
          glm::glm
          OpenGL::GL
          options::options
          SDL2::SDL2
          SDL2::SDL2main
          Imgui::OpenGL
          Imgui::SDL2
          camera::utilities)

target_compile_definitions(Trackball PRIVATE # OPENGL_DEBUG
                                             GLM_ENABLE_EXPERIMENTAL)

set_target_properties(
  Trackball
  PROPERTIES CXX_STANDARD 17
             CXX_EXTENSIONS OFF
             CXX_STANDARD_REQUIRED ON
             C_STANDARD 11
             C_EXTENSIONS OFF
             C_STANDARD_REQUIRED ON)

target_precompile_headers(Trackball PRIVATE
                          ${CMAKE_SOURCE_DIR}/utilities/precompile.hpp)

# data/debugger_small_5k.obj is optional: the demo falls back to a generated
# wire torus if it is not present.
if(EXISTS ${CMAKE_CURRENT_LIST_DIR}/data)
  file(COPY ${CMAKE_CURRENT_LIST_DIR}/data
       DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/)
endif()
