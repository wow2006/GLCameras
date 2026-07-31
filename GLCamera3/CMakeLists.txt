# ${CMAKE_SOURCE_DIR}/GLCamera3/CMakeLists.txt
project(GLCamera3 LANGUAGES CXX)

add_executable(
  GLCamera3
  # WIN32
)

target_sources(GLCamera3 PRIVATE camera.cpp main.cpp)

target_link_libraries(
  GLCamera3
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

target_include_directories(GLCamera3 SYSTEM PRIVATE ${Stb_INCLUDE_DIR})

target_compile_definitions(
  GLCamera3 PRIVATE # OPENGL_DEBUG
                    GLM_ENABLE_EXPERIMENTAL STB_IMAGE_IMPLEMENTATION)

set_target_properties(
  GLCamera3
  PROPERTIES CXX_STANDARD 17
             CXX_EXTENSIONS OFF
             CXX_STANDARD_REQUIRED ON
             C_STANDARD 11
             C_EXTENSIONS OFF
             C_STANDARD_REQUIRED ON)

target_precompile_headers(GLCamera3 PRIVATE
                          ${CMAKE_SOURCE_DIR}/utilities/precompile.hpp)

file(COPY ${CMAKE_CURRENT_LIST_DIR}/content/textures/floor_color_map.tga
          ${CMAKE_CURRENT_LIST_DIR}/content/textures/floor_light_map.tga
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/)

file(COPY ${CMAKE_CURRENT_LIST_DIR}/content/models/
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/content/models/)
