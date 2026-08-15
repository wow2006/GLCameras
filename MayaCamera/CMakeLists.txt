# ${CMAKE_SOURCE_DIR}/MayaCamera/CMakeLists.txt
project(MayaCamera LANGUAGES CXX)

add_executable(MayaCamera)

target_sources(MayaCamera PRIVATE main.cpp maya_camera.cpp)

target_link_libraries(
  MayaCamera
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

target_compile_definitions(MayaCamera PRIVATE # OPENGL_DEBUG
                                              GLM_ENABLE_EXPERIMENTAL)

set_target_properties(
  MayaCamera
  PROPERTIES CXX_STANDARD 17
             CXX_EXTENSIONS OFF
             CXX_STANDARD_REQUIRED ON
             C_STANDARD 11
             C_EXTENSIONS OFF
             C_STANDARD_REQUIRED ON)

target_precompile_headers(MayaCamera PRIVATE
                          ${CMAKE_SOURCE_DIR}/utilities/precompile.hpp)

# The demo shares OrbitCamera's two models rather than keeping a second copy of
# them. Both are optional: the demo still runs and reports what is missing.
if(EXISTS ${CMAKE_SOURCE_DIR}/OrbitCamera/data)
  file(COPY ${CMAKE_SOURCE_DIR}/OrbitCamera/data
       DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/)
endif()
