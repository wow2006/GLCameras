if(TARGET tracy::tracy)
    return()
endif()

set(tracy_DIR "${CMAKE_SOURCE_DIR}/thirdparty/tracy")

if(NOT EXISTS ${tracy_DIR})
    message(FATAL_ERROR "tracy submodule is missing. please run `git submodule update --init`.")
endif()

add_library(
    tracy
    OBJECT
    ${tracy_DIR}/TracyClient.cpp
)

add_library(
    tracy::tracy
    ALIAS
    tracy
)

target_include_directories(
    tracy
    PUBLIC
    ${tracy_DIR}
)

target_compile_definitions(
    tracy
    PUBLIC
    TRACY_ENABLE
)

target_compile_options(
    tracy
    PRIVATE
    $<$<CXX_COMPILER_ID:GNU>:-w>
    $<$<CXX_COMPILER_ID:Clang>:-w>
    $<$<CXX_COMPILER_ID:MSVC>:/w>
)
