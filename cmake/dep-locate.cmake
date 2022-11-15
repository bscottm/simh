##=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=
## dep-locate.cmake
##
## Consolidated list of runtime dependencies for simh, probed/found via
## CMake's find_package() and pkg_check_modules() when 'pkgconfig' is
## available.
##=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=

include(FindPkgConfig)

if (WITH_REGEX)
    if (WIN32)
        find_package(unofficial-pcre CONFIG)
    elseif (PREFER_PCRE)
        find_package(PCRE REQUIRED)
    else ()
        find_package(PCRE2 REQUIRED)
    endif ()
endif ()

if (WITH_VIDEO)
    find_package(PNG REQUIRED)
    ## find_package(Freetype CONFIG REQUIRED)
    find_package(SDL2 CONFIG QUIET)
    find_package(SDL2_ttf CONFIG QUIET)
endif (WITH_VIDEO)

if (WITH_NETWORK)
    if (WITH_VDE)
        find_package(VDE)
    endif ()
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
        if (NOT ZLIB_FOUND AND NOT WIN32)
            pkg_check_modules(ZLIB IMPORTED_TARGET zlib)
        endif ()
    endif ()

    if (WITH_VIDEO)
        if (NOT PNG_FOUND)
            pkg_check_modules(PNG IMPORTED_TARGET libpng16)
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
