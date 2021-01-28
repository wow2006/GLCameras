if(TARGET stb::image)
    return()
endif()

set(stb_DIR "${CMAKE_SOURCE_DIR}/thirdparty/stb")

if(NOT EXISTS ${stb_DIR}/stb_image.h)
    file(MAKE_DIRECTORY ${stb_DIR})
    file(
        DOWNLOAD https://raw.githubusercontent.com/nothings/stb/master/stb_image.h
        ${stb_DIR}/stb_image.h
        SHOW_PROGRESS
        EXPECTED_HASH MD5=d5600754a43b0cd006d5c38dfbf93a4f
    )
endif()

add_library(
    stb_image
    INTERFACE
)

add_library(
    stb::image
    ALIAS
    stb_image
)

target_include_directories(
    stb_image
    SYSTEM INTERFACE
    ${stb_DIR}
)
