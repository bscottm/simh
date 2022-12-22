## CPack setup -- sets the CPACK_* variables for the sundry installers
##
## Author: B. Scott Michel (scooter.phd@gmail.com)
## "scooter me fecit"


# Install the DLLs alongside the binaries
if (WIN32 AND BUILD_SHARED_DEPS AND EXISTS ${SIMH_DEP_TOPDIR}/bin)
   file(GLOB SIMH_DLLS ${SIMH_DEP_TOPDIR}/bin/*.dll)
   install(FILES ${SIMH_DLLS} RUNTIME)
endif ()

# After we know where everything will install, let CPack figure out
# how to assemble it into a package file.
set(CPACK_PACKAGE_VENDOR "The Open-SIMH project")

if (SIMH_BUILD_SUFFIX)
    set(buildSuffix ${SIMH_BUILD_SUFFIX})
else ()
    set(buildSuffix "")
    if (WIN32)
        if (CMAKE_SIZEOF_VOID_P EQUAL 8)
            list(APPEND buildSuffix "win64")
        else ()
            list(APPEND buildSuffix "win32")
        endif ()

        list(APPEND buildSuffix "\${CPACK_BUILD_CONFIG}")
        ## If using Visual Studio, append the compiler and toolkit:
        if (CMAKE_GENERATOR MATCHES "Visual Studio 17 .*")
            list(APPEND buildSuffix "vs2022")
        elseif (CMAKE_GENERATOR MATCHES "Visual Studio 16 .*")
            list(APPEND buildSuffix "vs2019")
        elseif (CMAKE_GENERATOR MATCHES "Visual Studio 15 .*")
            list(APPEND buildSuffix "vs2017")
        elseif (CMAKE_GENERATOR MATCHES "Visual Studio 14 .*")
            list(APPEND buildSuffix "vs2015")
        endif ()
        if (CMAKE_GENERATOR_TOOLSET MATCHES "v[0-9][0-9][0-9]_xp")
            string(APPEND buildSuffix "xp")
        endif ()
    else ()
        list(APPEND buildSuffix ${CMAKE_SYSTEM_NAME})
    endif ()

    list(JOIN buildSuffix "-" buildSuffix)
endif ()

string(JOIN "-" CPACK_PACKAGE_FILE_NAME
    "${CMAKE_PROJECT_NAME}"
    "${CMAKE_PROJECT_VERSION}"
    "${buildSuffix}"
)

message(STATUS "CPack output file name: ${CPACK_PACKAGE_FILE_NAME}")
unset(buildSuffix)

## CPack generator-specific configs:

##+
## Debian:
##-

list(APPEND CPACK_DEBIAN_PACKAGE_DEPENDS
    libsdl2
    libsdl2-ttf
    libpcap
    libvdeplug
    libedit
)

set(CPACK_PACKAGE_CONTACT     "open-simh@nowhere.org")
set(CPACK_PACKAGE_MAINTAINER "open-simh@nowhere.org")

include(CPack)
