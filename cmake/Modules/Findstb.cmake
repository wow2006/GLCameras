foreach(component ${stb_FIND_COMPONENTS})
  string(TOUPPER ${component} _COMPONENT)
  set(STB_USE_${_COMPONENT} ON)
endforeach()

set(stb_DIR "${CMAKE_SOURCE_DIR}/thirdparty/stb")

if(STB_USE_TRUETYPE)
    if(NOT TARGET stb::image)
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

        target_compile_definitions(
            stb_image
            INTERFACE
            STBI_ONLY_JPEG
            STB_IMAGE_IMPLEMENTATION
        )
    endif()
endif()

if(STB_USE_IMAGE)
    if(NOT TARGET stb::truetype)
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
    endif()
endif()
