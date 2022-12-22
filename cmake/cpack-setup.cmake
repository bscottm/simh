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

if (SIMH_PACKAGE_SUFFIX)
    set(buildSuffix "${SIMH_PACKAGE_SUFFIX}")
else ()
    message(STATUS "No SIMH_PACKAGE_SUFFIX supplied, manufacturing a default.")
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

## When applicable (e.g., NSIS Windows), install under the SIMH-x.y directory:
set(CPACK_PACKAGE_INSTALL_DIRECTORY "SIMH-${SIMH_VERSION_MAJOR}.${SIMH_VERSION_MINOR}")
## License file:
set(CPACK_RESOURCE_FILE_LICENSE ${CMAKE_SOURCE_DIR}/LICENSE.txt)

set(CPACK_PACKAGE_CONTACT     "open-simh@nowhere.org")
set(CPACK_PACKAGE_MAINTAINER "open-simh@nowhere.org")

## CPack generator-specific configs:

##+
## NSIS Windows installer.
##-
set(CPACK_NSIS_PACKAGE_NAME ${CPACK_PACKAGE_INSTALL_DIRECTORY})
configure_file(${CMAKE_SOURCE_DIR}/cmake/installers/NSIS.template.in
    ${CMAKE_BINARY_DIR}/NSIS.template
    @ONLY)

###+
### WIX MSI Windows installer.
###
###
### Upgrade GUID shouldn't really change.
###-
set(CPACK_WIX_UPGRADE_GUID "ed5dba4c-7c9e-4af8-ac36-37e14c637696")

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


include(CPack)
