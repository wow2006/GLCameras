# ${CMAKE_SOURCE_DIR}/GLThirdPersonCamera2/CMakeLists.txt
project(GLThirdPersonCamera2 LANGUAGES CXX)

add_executable(GLThirdPersonCamera2)

target_sources(GLThirdPersonCamera2 PRIVATE main.cpp third_person_camera.cpp)

target_link_libraries(
  GLThirdPersonCamera2
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

target_include_directories(GLThirdPersonCamera2 SYSTEM
                           PRIVATE ${Stb_INCLUDE_DIR})

target_compile_definitions(
  GLThirdPersonCamera2 PRIVATE # OPENGL_DEBUG
                               GLM_ENABLE_EXPERIMENTAL STB_IMAGE_IMPLEMENTATION)

set_target_properties(
  GLThirdPersonCamera2
  PROPERTIES CXX_STANDARD 17
             CXX_EXTENSIONS OFF
             CXX_STANDARD_REQUIRED ON
             C_STANDARD 11
             C_EXTENSIONS OFF
             C_STANDARD_REQUIRED ON)

target_precompile_headers(GLThirdPersonCamera2 PRIVATE
                          ${CMAKE_SOURCE_DIR}/utilities/precompile.hpp)

file(
  COPY ${CMAKE_CURRENT_LIST_DIR}/ball_color_map.jpg
       ${CMAKE_CURRENT_LIST_DIR}/floor_color_map.jpg
       ${CMAKE_CURRENT_LIST_DIR}/floor_light_map.jpg
  DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/)
