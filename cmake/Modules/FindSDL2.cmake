#
# SDL2_INCLUDE_DIR
# SDL2_LIBRARY
# SDL2_FOUND
#
include(FindPackageHandleStandardArgs)

if(WIN32)
    if(DEFINED CMAKE_TOOLCHAIN_FILE)
        get_filename_component(CMAKE_TOOLCHAIN_DIR ${CMAKE_TOOLCHAIN_FILE} DIRECTORY)
        get_filename_component(CMAKE_TOOLCHAIN_DIR ${CMAKE_TOOLCHAIN_DIR}/../../ ABSOLUTE)
    endif()

    find_path(
        SDL2_INCLUDE_DIR
        SDL2/SDL.h
        ${CMAKE_TOOLCHAIN_DIR}/installed/x64-windows/include
    )

    find_library(
        SDL2_LIBRARY
        SDL2
        ${CMAKE_TOOLCHAIN_DIR}/installed/x64-windows/lib
    )

    find_library(
        SDL2_LIBRARY_DEBUG
        SDL2d
        ${CMAKE_TOOLCHAIN_DIR}/installed/x64-windows/debug/lib
    )

    find_package_handle_standard_args(
        SDL2
        REQUIRED_VARS
        SDL2_LIBRARY_DEBUG
        SDL2_LIBRARY
        SDL2_INCLUDE_DIR
    )
else()
    find_path(
        SDL2_INCLUDE_DIR
        SDL2/SDL.h
        /usr/include/
        /usr/local/include/
    )

    find_library(
        SDL2_LIBRARY
        SDL2
        /usr/lib/
        /usr/lib/x86_64-linux-gnu/
        /usr/local/lib/
    )

    find_package_handle_standard_args(
        SDL2
        REQUIRED_VARS
        SDL2_LIBRARY
        SDL2_INCLUDE_DIR
    )
endif()


if(SDL2_FOUND)
  if(NOT TARGET SDL2::SDL2)
    add_library(
        SDL2::SDL2
        UNKNOWN
        IMPORTED
    )

    set_target_properties(
        SDL2::SDL2
        PROPERTIES
        IMPORTED_LOCATION "${SDL2_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${SDL2_INCLUDE_DIR}"
    )

    if(WIN32)
        set_target_properties(
            SDL2::SDL2
            PROPERTIES
            IMPORTED_LOCATION_DEBUG "${SDL2_LIBRARY_DEBUG}"
        )
    endif()
  endif()
endif()

