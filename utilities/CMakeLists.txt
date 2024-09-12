# ${CMAKE_SOURCE_DIR}/utilities/CMakeLists.txt
add_library(utilities STATIC input.hpp input.cpp shaders.hpp shaders.cpp)

add_library(camera::utilities ALIAS utilities)

target_include_directories(utilities PUBLIC ${CMAKE_CURRENT_LIST_DIR})

target_link_libraries(utilities PUBLIC fmt::fmt glbinding::glbinding glm::glm
                                       OpenGL::GL SDL2::SDL2)
