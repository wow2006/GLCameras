# ${CMAKE_SOURCE_DIR}/cmake/FindImgui.cmake
if(TARGET Imgui::GLFW_OpenGL)
  return()
endif()

set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/thirdparty/imgui/)

# ========================
# IMGUI Core
# ========================
add_library(
  Imgui_core
  STATIC
)

target_sources(
  Imgui_core
  PRIVATE
  ${IMGUI_DIR}/imgui.cpp
  ${IMGUI_DIR}/imgui_draw.cpp
  ${IMGUI_DIR}/imgui_demo.cpp
  ${IMGUI_DIR}/imgui_tables.cpp
  ${IMGUI_DIR}/imgui_widgets.cpp
)

target_include_directories(
  Imgui_core
  SYSTEM PUBLIC
  ${IMGUI_DIR}
)

add_library(
  Imgui::core
  ALIAS
  Imgui_core
)

target_compile_options(
  Imgui_core
  PRIVATE
  $<$<CXX_COMPILER_ID:Clang>:-w>
  $<$<CXX_COMPILER_ID:GNU>:-w>
)

target_compile_definitions(
  Imgui_core
  PUBLIC
  IMGUI_IMPL_OPENGL_LOADER_GL3W
)

# ========================
# IMGUI win32
# ========================
if(WIN32 AND BUILD_WIN32)
    add_library(
      Imgui_win32
      STATIC
    )

    add_library(
      Imgui::Win32
      ALIAS
      Imgui_win32
    )

    target_sources(
      Imgui_win32
      PRIVATE
      ${IMGUI_DIR}/backends/imgui_impl_win32.cpp
    )

    target_include_directories(
      Imgui_win32
      SYSTEM PUBLIC
      ${IMGUI_DIR}/backends/
    )

    target_link_libraries(
      Imgui_win32
      PUBLIC
      Imgui::core
    )
endif()

# ========================
# IMGUI GLFW
# ========================
if(GLFW)
    if(NOT TARGET GLFW::glfw3)
      find_package(GLFW REQUIRED)
    endif()

    if(NOT TARGET gl3w::gl3w)
      find_package(gl3w REQUIRED)
    endif()

    add_library(
      Imgui_glfw
      STATIC
    )

    add_library(
      Imgui::GLFW
      ALIAS
      Imgui_glfw
    )

    target_sources(
      Imgui_glfw
      PRIVATE
      ${IMGUI_DIR}/examples/imgui_impl_glfw.cpp
    )

    target_include_directories(
      Imgui_glfw
      SYSTEM PUBLIC
      ${IMGUI_DIR}/examples/
    )

    target_link_libraries(
      Imgui_glfw
      PUBLIC
      Imgui::core
      GLFW::glfw3
      gl3w::gl3w
    )

    target_compile_options(
      Imgui_glfw
      PRIVATE
      $<$<CXX_COMPILER_ID:Clang>:-w>
      $<$<CXX_COMPILER_ID:GNU>:-w>
    )
endif()
# ========================
# IMGUI OpenGL
# ========================
if(NOT TARGET OpenGL::GL)
  set(OpenGL_GL_PREFERENCE GLVND)
  find_package(OpenGL REQUIRED)
endif()

if(NOT TARGET OpenGL::GL)
  find_package(gl3w REQUIRED)
endif()

add_library(
  Imgui_opengl
  STATIC
)

add_library(
  Imgui::OpenGL
  ALIAS
  Imgui_opengl
)

target_sources(
  Imgui_opengl
  PRIVATE
  ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

target_include_directories(
  Imgui_opengl
  SYSTEM PUBLIC
  ${IMGUI_DIR}/backends/
)

target_link_libraries(
  Imgui_opengl
  PUBLIC
  Imgui::core
  OpenGL::GL
  gl3w::gl3w
)

target_compile_options(
  Imgui_opengl
  PRIVATE
  $<$<CXX_COMPILER_ID:Clang>:-w>
  $<$<CXX_COMPILER_ID:GNU>:-w>
)
