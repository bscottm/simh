##+=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~
## vcpkg setup for MSVC. MinGW builds should use 'pacman' to install
## required dependency libraries.
##-=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~

if (WIN32 AND CMAKE_GENERATOR MATCHES "Visual Studio .*")
    ## Set vcpkg's environment -- needs to be done before (!!) vcpkg-bootstrap.bat executes:

    ## Set the target triplet:
    ## Default to x64, unless otherwise directed:
    set(VCPKG_ARCH "x64")
    if(CMAKE_GENERATOR_PLATFORM MATCHES "Win32")
        set(VCPKG_ARCH "x86")
    elseif(CMAKE_GENERATOR_PLATFORM MATCHES "ARM")
        set(VCPKG_ARCH "arm")
    elseif(CMAKE_GENERATOR_PLATFORM MATCHES "ARM64")
        set(VCPKG_ARCH "arm64")
    endif()

    set(VCPKG_RUNTIME "static")
    set(VCPKG_PLATFORM "windows")

## The "_xp" patches didn't get merged into vcpkg. Keeping this code just
## in case they do, at some point in time, to remember how it's done. Also
## have to add a local triplet file and set the overlay triplets environment
## variable.
##
## x86-winxp-static triplet file:
##    set(VCPKG_TARGET_ARCHITECTURE x86)
##    set(VCPKG_CRT_LINKAGE static)
##    set(VCPKG_LIBRARY_LINKAGE static)
##    ## "v141_xp" was the proposed toolset version in vcpkg to
##    ## setup XP using VS2017:
##    set(VCPKG_PLATFORM_TOOLSET "v141_xp")
##
#
#    if (CMAKE_GENERATOR MATCHES "Visual Studio 16 .*")
#        ## VS 2019
#        if (CMAKE_GENERATOR_TOOLSET MATCHES "v[0-9][0-9][0-9]_xp")
#            set(VCPKG_PLATFORM "winxp")
#        endif ()
#    elseif (CMAKE_GENERATOR MATCHES "Visual Studio 15 .*")
#        ## VS 2017
#        if (CMAKE_GENERATOR_TOOLSET MATCHES "v[0-9][0-9][0-9]_xp")
#            set(VCPKG_PLATFORM "winxp")
#        endif ()
#    elseif (CMAKE_GENERATOR MATCHES "Visual Studio 14 .*")
#        ## VS 2015
#    endif()
#    ## And our local triplets configurations:
#    set(ENV{VCPKG_OVERLAY_TRIPLETS} ${CMAKE_SOURCE_DIR}/cmake/local-triplets)

    ## Set the default triplet in the environment; older vcpkg installs on
    ## appveyor don't support the "--triplet" command line argument.
    set(VCPKG_TARGET_TRIPLET "${VCPKG_ARCH}-${VCPKG_PLATFORM}-${VCPKG_RUNTIME}" CACHE STRING "target triplet" FORCE)
    set(ENV{VCPKG_DEFAULT_TRIPLET} ${VCPKG_TARGET_TRIPLET})

    ## Status:
    message(STATUS "CMAKE_GENERATOR          ${CMAKE_GENERATOR}")
    message(STATUS "CMAKE_GENERATOR_PLATFORM ${CMAKE_GENERATOR_PLATFORM}")
    message(STATUS "vcpkg target triplet     ${VCPKG_TARGET_TRIPLET}")

    ## Locate the existing vcpkg:
    find_path(VCPKG_LOCAL vcpkg.exe DOC "Locally installed vcpkg.exe")

    ## Not locally installed or user did not set VCPKG_ROOT -- install a SIMH-local copy
    ## and work with it.
    if (NOT VCPKG_LOCAL OR NOT DEFINED ENV{VCPKG_ROOT})
        set(VCPKG_URL "https://github.com/microsoft/vcpkg/archive/refs/tags/2022.09.27.zip")
        string(REGEX MATCH "/tags/(.*)\.zip" vcpkg_version ${VCPKG_URL})
        set(vcpkg_version ${CMAKE_MATCH_1})
        set(VCPKG_DIR "${CMAKE_BINARY_DIR}/vcpkg-${vcpkg_version}")

        if (NOT EXISTS ${VCPKG_DIR})
            set(vcpkg_msg "")
            string(APPEND vcpkg_msg "Downloading and extracting vcpkg\n")
            string(APPEND vcpkg_msg "\n")
            string(APPEND vcpkg_msg "   * Did not find a locally installed 'vcpkg' package manager\n")
            string(APPEND vcpkg_msg "   * VCPKG_ROOT is not set in the environment.\n")
            string(APPEND vcpkg_msg "\n")
            string(APPEND vcpkg_msg "OPEN-SIMH relies on 'vcpkg' to manage dependency libraries used by\n")
            string(APPEND vcpkg_msg "simulators: pcre, libpng, sdl2, sdl2-ttf and pthreads.\n")
            string(APPEND vcpkg_msg "\n")
            string(APPEND vcpkg_msg "Installing Microsoft 'vcpkg' package manager in ${VCPKG_DIR}\n")
            string(APPEND vcpkg_msg "Downloading from ${VCPKG_URL}\n")
            string(APPEND vcpkg_msg "\n")
            string(APPEND vcpkg_msg "\n")
            message(STATUS ${vcpkg_msg})
            unset(vcpkg_msg)

            execute_process(
                COMMAND ${CMAKE_COMMAND} -E make_directory ${VCPKG_DIR}
                RESULT_VARIABLE VCPKG_MKDIR
            )

            file(DOWNLOAD ${VCPKG_URL} "${CMAKE_BINARY_DIR}/vcpkg.zip"
                STATUS VCPKG_DOWNLOAD
            )
            list(GET VCPKG_DOWNLOAD 0 VCPKG_DL_STATUS)
            if (NOT (${VCPKG_DL_STATUS} EQUAL 0))
                list(GET VCPKG_DOWNLOAD 1 VCPKG_DL_ERROR)
                message(FATAL_ERROR "Download failed: ${VCPKG_DL_ERROR}")
            endif (NOT (${VCPKG_DL_STATUS} EQUAL 0))

            execute_process(
                COMMAND ${CMAKE_COMMAND} -E tar xf "${CMAKE_BINARY_DIR}/vcpkg.zip"
                WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
                RESULT_VARIABLE VCPKG_EXTRACT
            )
            if (NOT (${VCPKG_EXTRACT} EQUAL 0))
                message(FATAL_ERROR "Extract failed.")
            endif (NOT (${VCPKG_EXTRACT} EQUAL 0))
        endif ()
    elseif (DEFINED ENV{VCPKG_ROOT})
        ## User specified a local vcpkg installlation:
        set(VCPKG_DIR $ENV{VCPKG_ROOT})
        message(STATUS "Using vcpkg installation in $ENV{VCPKG_ROOT}.")
    elseif (VCPKG_LOCAL)
        ## Found a local vcpkg installation. vcpkg.exe exists in the top of the
        ## directory hierarchy.
        file(REAL_PATH ${VCPKG_LOCAL} VCPKG_LOCAL)
        get_filename_component(VCPKG_DIR ${VCPKG_LOCAL} DIRECTORY)
        message(STATUS "Using vcpkg installation in $VCPKG_DIR.")
        if (NOT EXISTS ${VCPKG_DIR}/scripts/buildsystems/vcpkg.cmake)
            message(FATAL "Expected to find ${VCPKG_DIR}/scripts/buildsystems/vcpkg.cmake")
        endif ()
    else ()
        message(FATAL "Didn't download vcpkg, didn't find VCPKG_ROOT set and didn't find vcpkg.")
    endif ()

    if (EXISTS ${VCPKG_DIR})
        if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
            set(CMAKE_TOOLCHAIN_FILE "${VCPKG_DIR}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
        endif()

        if (NOT EXISTS ${VCPKG_DIR}/vcpkg.exe)
            message(STATUS "Bootstrapping vcpkg")
            execute_process(
                COMMAND "cmd" "/c" "bootstrap-vcpkg.bat" "-disableMetrics"
                WORKING_DIRECTORY ${VCPKG_DIR}
                RESULT_VARIABLE VCPKG_BOOTSTRAP
            )
            if (NOT VCPKG_BOOTSTRAP EQUAL 0)
                if (EXISTS ${VCPKG_DIR})
                    file(REMOVE_RECURSE ${VCPKG_DIR})
                endif ()
                message(FATAL_ERROR "vcpkg did not bootstrap. Exiting.")
            endif ()
        endif ()

        message(STATUS "Installing vcpkg-based dependencies: pthreads pcre libpng sdl2 sdl2-ttf")
        execute_process(
            COMMAND ".\\vcpkg.exe" "install" "pthreads" "pcre" "libpng" "sdl2" "sdl2-ttf"
            WORKING_DIRECTORY ${VCPKG_DIR}
            RESULT_VARIABLE VCPKG_RESULT
        )
        if (NOT VCPKG_RESULT EQUAL 0)
            message(FATAL_ERROR "'vcpkg install' failed. Exiting.")
        endif ()
    else ()
        message(FATAL "vcpkg installation expected at ${VCPKG_DIR}")
    endif()
endif ()
