# ${CMAKE_SOURCE_DIR}/CMakeLists.txt
cmake_minimum_required(VERSION 3.16 FATAL_ERROR)
project(GLCameras VERSION 2.0.0 LANGUAGES CXX)

list(
    APPEND CMAKE_MODULE_PATH
    ${CMAKE_SOURCE_DIR}/cmake
    ${CMAKE_SOURCE_DIR}/cmake/Modules
)
include(options)

find_package(OpenGL REQUIRED)
find_package(tracy  REQUIRED)

add_subdirectory(GLCamera1)
#add_subdirectory(GLCamera2)
#add_subdirectory(GLCamera3)
#add_subdirectory(GLThirdPersonCamera1)
#add_subdirectory(GLThirdPersonCamera2)
#add_subdirectory(OrbitCamera)
#add_subdirectory(Trackball)
