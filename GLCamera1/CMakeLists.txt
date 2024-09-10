# ${CMAKE_SOURCE_DIR}/GLCamera1/CMakeLists.txt
project(GLCamera1 LANGUAGES CXX)

add_executable(
    GLCamera1
    WIN32
)

target_sources(
    GLCamera1
    PRIVATE
    main.cpp
    camera.cpp
    shaders.cpp
    input.cpp
)

target_link_libraries(
    GLCamera1
    PRIVATE
    fmt::fmt
    glbinding::glbinding
    glm::glm
    Imgui::OpenGL
    Imgui::SDL2
    OpenGL::GL
    options::options
    SDL2::SDL2
    SDL2::SDL2main
    stb::image
)

target_compile_definitions(
    GLCamera1
    PRIVATE
    #OPENG_DEBUG
    GLM_ENABLE_EXPERIMENTAL
)

set_target_properties(
    GLCamera1
    PROPERTIES
    CXX_STANDARD 17
    CXX_EXTENSIONS OFF
    CXX_STANDARD_REQUIRED ON
    C_STANDARD 11
    C_EXTENSIONS OFF
    C_STANDARD_REQUIRED ON
)

target_precompile_headers(
  GLCamera1
  PRIVATE
  ${CMAKE_SOURCE_DIR}/utilities/precompile.hpp
)

download_file(
  "${CMAKE_CURRENT_LIST_DIR}/floor_color_map.jpg"
  https://docs.google.com/uc?export=download&id=1o4eLs6OOvib-PGGonjAqTA0MfhDryFz1
  1658ebf1dadbc48b97f7961be885ed68
)

download_file(
  "${CMAKE_CURRENT_LIST_DIR}/floor_light_map.jpg"
  https://docs.google.com/uc?export=download&id=1ePw9OuVFflqfiqBG5WuUvYaEmvPrrwRE
  4476d38d396df90dbfdd3d8675c7e7a1
)

file(
    COPY ${CMAKE_CURRENT_LIST_DIR}/floor_color_map.jpg
         ${CMAKE_CURRENT_LIST_DIR}/floor_light_map.jpg
    DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/
)
