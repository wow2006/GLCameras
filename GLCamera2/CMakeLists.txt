# ${CMAKE_SOURCE_DIR}/GLCamera2/CMakeLists.txt
project(GLCamera2 LANGUAGES CXX)

add_executable(
  GLCamera2
  # WIN32
)

target_sources(GLCamera2 PRIVATE camera.cpp main.cpp mathlib.cpp)

target_link_libraries(
  GLCamera2
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

target_include_directories(GLCamera2 SYSTEM PRIVATE ${Stb_INCLUDE_DIR})

target_compile_definitions(
  GLCamera2 PRIVATE # OPENGL_DEBUG
                    GLM_ENABLE_EXPERIMENTAL STB_IMAGE_IMPLEMENTATION)

set_target_properties(
  GLCamera2
  PROPERTIES CXX_STANDARD 17
             CXX_EXTENSIONS OFF
             CXX_STANDARD_REQUIRED ON
             C_STANDARD 11
             C_EXTENSIONS OFF
             C_STANDARD_REQUIRED ON)

target_precompile_headers(GLCamera2 PRIVATE
                          ${CMAKE_SOURCE_DIR}/utilities/precompile.hpp)

# TODO(Hussein): moved to resources location
download_file(
  "${CMAKE_CURRENT_LIST_DIR}/floor_color_map.jpg"
  https://docs.google.com/uc?export=download&id=1o4eLs6OOvib-PGGonjAqTA0MfhDryFz1
  1658ebf1dadbc48b97f7961be885ed68)

download_file(
  "${CMAKE_CURRENT_LIST_DIR}/floor_light_map.jpg"
  https://docs.google.com/uc?export=download&id=1ePw9OuVFflqfiqBG5WuUvYaEmvPrrwRE
  4476d38d396df90dbfdd3d8675c7e7a1)

file(COPY ${CMAKE_CURRENT_LIST_DIR}/floor_color_map.jpg
          ${CMAKE_CURRENT_LIST_DIR}/floor_light_map.jpg
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/)
