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
  IMGUI_IMPL_OPENGL_LOADER_GLBINDING3
)

# ========================
# IMGUI win32
# ========================
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

# ========================
# IMGUI OpenGL
# ========================
if(NOT TARGET OpenGL::GL)
  set(OpenGL_GL_PREFERENCE GLVND)
  find_package(OpenGL REQUIRED)
endif()

if(NOT TARGET glbinding::glbinding)
    find_package(glbinding CONFIG REQUIRED)
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
  glbinding::glbinding
)

target_compile_options(
  Imgui_opengl
  PRIVATE
  $<$<CXX_COMPILER_ID:Clang>:-w>
  $<$<CXX_COMPILER_ID:GNU>:-w>
)
