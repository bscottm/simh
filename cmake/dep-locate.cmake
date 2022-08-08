##=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=
## dep-locate.cmake
##
## Consolidated list of runtime dependencies for simh, probed/found via
## CMake's find_package() and pkg_check_modules() when 'pkgconfig' is
## available.
##=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=

include(FindPkgConfig)

if (WITH_REGEX)
    find_package(PCRE)
    ## TODO: Add PCRE2 support later:
    ## find_package(PCRE2)
endif ()

if (WITH_REGEX OR WITH_VIDEO)
    if (CMAKE_VERSION VERSION_GREATER_EQUAL 3.24)
        set(ZLIB_USE_STATIC_LIBS ON)
        find_package(ZLIB)
    else ()
        find_package(ZLIB_STATIC)
    endif ()
endif ()

if (WITH_VIDEO)
    find_package(BZip2)
    find_package(PNG)
    find_package(BrotliDec)
    find_package(Harfbuzz)
    find_package(Freetype)
    find_package(SDL2 NAMES sdl2 SDL2)
    find_package(SDL2_ttf NAMES sdl2_ttf SDL2_ttf)
endif (WITH_VIDEO)

if (WITH_NETWORK)
    if (WITH_VDE)
        find_package(VDE)
    endif ()
endif (WITH_NETWORK)

## pcap is special: Headers only and dynamically loaded.
if (WITH_NETWORK)
    if (WITH_PCAP)
        find_package(PCAP)
    endif (WITH_PCAP)
endif (WITH_NETWORK)

find_package(PkgConfig)
if (PKG_CONFIG_FOUND)
    if (WITH_REGEX)
        if (PREFER_PCRE AND NOT PCRE_FOUND)
            pkg_check_modules(PCRE IMPORTED_TARGET libpcre)
        elseif (NOT PREFER_PCRE AND NOT PCRE2_FOUND)
            pkg_check_modules(PCRE IMPORTED_TARGET libpcre2-8)
        endif ()
    endif (WITH_REGEX)

    if (WITH_REGEX OR WITH_VIDEO)
        if (NOT ZLIB_FOUND)
            pkg_check_modules(ZLIB IMPORTED_TARGET zlib)
        endif ()
    endif ()

    if (WITH_VIDEO)
        if (NOT BZIP2_FOUND)
            pkg_check_modules(BZIP2 IMPORTED_TARGET bzip2)
        endif ()
        if (NOT PNG_FOUND)
            pkg_check_modules(PNG IMPORTED_TARGET libpng16)
        endif ()
        if (NOT HARFBUZZ_FOUND)
            pkg_check_modules(HARFBUZZ IMPORTED_TARGET harfbuzz)
        endif ()
        if (NOT FREETYPE_FOUND)
            pkg_check_modules(Freetype IMPORTED_TARGET freetype2)
        endif ()
        if (NOT SDL2_FOUND)
            pkg_check_modules(SDL2 IMPORTED_TARGET sdl2)
            if (NOT SDL2_FOUND)
                pkg_check_modules(SDL2 IMPORTED_TARGET SDL2)
            endif ()
        endif ()

        if (NOT SDL2_ttf_FOUND)
            pkg_check_modules(SDL2_ttf IMPORTED_TARGET SDL2_ttf)
            if (NOT SDL2_ttf_FOUND)
                pkg_check_modules(SDL2_ttf IMPORTED_TARGET sdl2_ttf)
            endif ()
        endif ()
    endif (WITH_VIDEO)

    if (WITH_NETWORK)
        if (WITH_VDE AND NOT VDE_FOUND)
            pkg_check_modules(VDE IMPORTED_TARGET vdeplug)
        endif ()
    endif (WITH_NETWORK)
endif ()
