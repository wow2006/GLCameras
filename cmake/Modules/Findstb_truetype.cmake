if(TARGET stb::truetype)
    return()
endif()

set(stb_DIR "${CMAKE_SOURCE_DIR}/thirdparty/stb")

if(NOT EXISTS ${stb_DIR}/stb_truetype.h)
    file(MAKE_DIRECTORY ${stb_DIR})
    file(
        DOWNLOAD https://raw.githubusercontent.com/nothings/stb/master/stb_truetype.h
        ${stb_DIR}/stb_truetype.h
        SHOW_PROGRESS
        EXPECTED_HASH MD5=50f872865206783e1ff32375173b2dfd
    )
endif()

add_library(
    stb_truetype
    INTERFACE
)

add_library(
    stb::truetype
    ALIAS
    stb_truetype
)

target_include_directories(
    stb_truetype
    SYSTEM INTERFACE
    ${stb_DIR}
)

target_compile_definitions(
    stb_truetype
    INTERFACE
    STB_TRUETYPE_IMPLEMENTATION
)
