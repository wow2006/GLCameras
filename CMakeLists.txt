# ${CMAKE_SOURCE_DIR}/CMakeLists.txt
cmake_minimum_required(VERSION 3.16 FATAL_ERROR)
project(GLCameras VERSION 2.0.0 LANGUAGES CXX)

list(
    APPEND CMAKE_MODULE_PATH
    ${CMAKE_SOURCE_DIR}/cmake
    ${CMAKE_SOURCE_DIR}/cmake/Modules
)
include(options)
include(download_file)

# TODO(Hussein): Removed when use SDL2
set(BUILD_WIN32 ON)

find_package(fmt    REQUIRED)
find_package(glm    REQUIRED)
find_package(gl3w   REQUIRED)
find_package(imgui  REQUIRED)
find_package(OpenGL REQUIRED)
find_package(tracy  REQUIRED)
find_package(stb    REQUIRED COMPONENTS image truetype)

add_subdirectory(GLCamera1)
#add_subdirectory(GLCamera2)
#add_subdirectory(GLCamera3)
#add_subdirectory(GLThirdPersonCamera1)
#add_subdirectory(GLThirdPersonCamera2)
#add_subdirectory(OrbitCamera)
#add_subdirectory(Trackball)
