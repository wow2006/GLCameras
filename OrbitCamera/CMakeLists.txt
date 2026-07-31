# ${CMAKE_SOURCE_DIR}/OrbitCamera/CMakeLists.txt
project(OrbitCamera LANGUAGES CXX)

add_executable(OrbitCamera)

target_sources(OrbitCamera PRIVATE anim_utils.cpp main.cpp orbit_camera.cpp)

target_link_libraries(
  OrbitCamera
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

target_compile_definitions(OrbitCamera PRIVATE # OPENGL_DEBUG
                                               GLM_ENABLE_EXPERIMENTAL)

set_target_properties(
  OrbitCamera
  PROPERTIES CXX_STANDARD 17
             CXX_EXTENSIONS OFF
             CXX_STANDARD_REQUIRED ON
             C_STANDARD 11
             C_EXTENSIONS OFF
             C_STANDARD_REQUIRED ON)

target_precompile_headers(OrbitCamera PRIVATE
                          ${CMAKE_SOURCE_DIR}/utilities/precompile.hpp)

# data/debugger_small_5k.obj and data/camera.obj are optional: the demo still
# runs and reports what is missing if either is absent.
if(EXISTS ${CMAKE_CURRENT_LIST_DIR}/data)
  file(COPY ${CMAKE_CURRENT_LIST_DIR}/data
       DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/)
endif()
