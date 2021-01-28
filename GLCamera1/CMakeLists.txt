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
    GL_ARB_multitexture.cpp
    WGL_ARB_multisample.cpp
    bitmap.cpp
    camera.cpp
    gl_font.cpp
    input.cpp
    mathlib.cpp
)

target_link_libraries(
    GLCamera1
    PRIVATE
    OpenGL::GL
    OpenGL::GLU
    tracy::tracy
    options::options
)

file(
    COPY ${CMAKE_CURRENT_LIST_DIR}/floor_color_map.jpg
         ${CMAKE_CURRENT_LIST_DIR}/floor_light_map.jpg
    DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/
)
