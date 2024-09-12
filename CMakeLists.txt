# ${CMAKE_SOURCE_DIR}/CMakeLists.txt
cmake_minimum_required(VERSION 3.16 FATAL_ERROR)
project(
  GLCameras
  VERSION 2.0.0
  LANGUAGES CXX)

if(MSVC AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 19.14)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /experimental:external /external:W0 ")
  set(CMAKE_INCLUDE_SYSTEM_FLAG_CXX "/external:I ")
endif()

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake
     ${CMAKE_SOURCE_DIR}/cmake/Modules)
include(options)
include(download_file)

find_package(OpenGL REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(glbinding CONFIG REQUIRED)
find_package(Imgui REQUIRED)
find_package(SDL2 CONFIG REQUIRED)
find_package(Stb REQUIRED)

add_subdirectory(utilities)
add_subdirectory(GLCamera1)
add_subdirectory(GLCamera2)
# add_subdirectory(GLCamera3) add_subdirectory(GLThirdPersonCamera1)
# add_subdirectory(GLThirdPersonCamera2) add_subdirectory(OrbitCamera)
# add_subdirectory(Trackball)
