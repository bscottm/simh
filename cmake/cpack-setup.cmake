# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
# THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
# IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
# CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
# 
# Except as contained in this notice, the names of The Authors shall not be
# used in advertising or otherwise to promote the sale, use or other dealings
# in this Software without prior written authorization from the Authors.

## CPack setup -- sets the CPACK_* variables for the sundry installers
##
## Author: B. Scott Michel (scooter.phd@gmail.com)
## "scooter me fecit"


# Install the DLLs alongside the binaries
if (WIN32 AND BUILD_SHARED_DEPS AND EXISTS ${SIMH_DEP_TOPDIR}/bin)
   file(GLOB SIMH_DLLS ${SIMH_DEP_TOPDIR}/bin/*.dll)
   install(FILES ${SIMH_DLLS}
       DESTINATION ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}
       COMPONENT RUNTIME
   )
endif ()

# After we know where everything will install, let CPack figure out
# how to assemble it into a package file.
set(CPACK_PACKAGE_VENDOR "The Open-SIMH project")

## Do the "special" CPack thing for system names
set(systemSuffix ${CMAKE_SYSTEM_NAME})

if (CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(systemSuffix "win64")
else ()
    set(systemSuffix "win32")
endif ()

string(JOIN "-" CPACK_PACKAGE_FILE_NAME
    "${CMAKE_PROJECT_NAME}"
    "${CMAKE_PROJECT_VERSION}"
    "${systemSuffix}"
)

set(buildSuffix "")

## Distinguish between release and debug build artifacts where we can:
if (DEFINED CMAKE_BUILD_TYPE)
    if (CMAKE_BUILD_TYPE EQUAL "Debug")
        set(buildSuffix "debug")
    endif ()
else ()
    set(buildSuffix "\${CPACK_BUILD_CONFIG}")
endif ()
if (buildSuffix)
    string(APPEND CPACK_PACKAGE_FILE_NAME "-${buildSuffix}")
endif ()

## If using Visual Studio, append the compiler and toolkit:
set(buildSuffix "")
if (CMAKE_GENERATOR MATCHES "Visual Studio 17 .*")
    set(buildSuffix "vs2022")
elseif (CMAKE_GENERATOR MATCHES "Visual Studio 16 .*")
    set(buildSuffix "vs2019")
elseif (CMAKE_GENERATOR MATCHES "Visual Studio 15 .*")
    set(buildSuffix "vs2017")
elseif (CMAKE_GENERATOR MATCHES "Visual Studio 14 .*")
    set(buildSuffix "vs2015")
endif ()
if (CMAKE_GENERATOR_TOOLSET MATCHES "v[0-9][0-9][0-9]_xp")
    string(APPEND buildSuffix "xp")
endif ()

if (buildSuffix)
    string(APPEND CPACK_PACKAGE_FILE_NAME "-${buildSuffix}")
endif ()
unset(buildSuffix)
message(STATUS "CPack output file name: ${CPACK_PACKAGE_FILE_NAME}")

## CPack generator-specific configs:

##+
## Debian:
##-

list(APPEND CPACK_DEBIAN_PACKAGE_DEPENDS
    libegl1-mesa
    libgles2-mesa
    libsdl2
    libsdl2-ttf
    libpcap
    libvdeplug
    libedit
)

set(CPACK_PACKAGE_CONTACT     "open-simh@nowhere.org")
set(CPACK_PACKAGE_MAINTAINER "open-simh@nowhere.org")

include(CPack)
