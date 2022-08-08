if (${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
  message("")
  message("*** Do NOT build or generate CMake artifacts in the SIMH source directory! ***")
  message("")
  message("Create a subdirectory and build in that subdirectory, e.g.:")
  message("")
  message("  $ mkdir cmake-build")
  message("  $ cd cmake-build")
  message("  $ cmake -G \"your generator here\" ..")
  message("")
  message(FATAL_ERROR "Preventing in-tree source build.")
endif ()

## Note: pthreads4w requires 3.14 or better.
cmake_minimum_required(VERSION 3.14)

## Respect MSVC_RUNTIME_LIBRARY's setting. Has to be set before project(),
## otherwise it'd have been put into platform_quirks.
##
## Note: See cmake\build_dep_matrix.cmake to see how this is propagated down
## into the dependency libraries.
##
if (POLICY CMP0091)
    cmake_policy(SET CMP0091 NEW)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    set(MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

set(SIMH_VERSION "4.0.0")
project(simh VERSION "${SIMH_VERSION}" LANGUAGES C CXX)

include(GNUInstallDirs)

## Provide a default CMAKE_BUILD_TYPE if CMAKE_CONFIGURATION_TYPES is empty or not defined.
if (NOT CMAKE_CONFIGURATION_TYPES)
    if (NOT CMAKE_BUILD_TYPE)
        set(CMAKE_BUILD_TYPE "Release")
        message(STATUS "CMAKE_BUILD_TYPE defaulted to ${CMAKE_BUILD_TYPE}")
    else (NOT CMAKE_BUILD_TYPE)
        message(STATUS "CMAKE_BUILD_TYPE is ${CMAKE_BUILD_TYPE}")
    endif (NOT CMAKE_BUILD_TYPE)
endif ()

# Places to look for CMake modules/includes
set(SIMH_INCLUDE_PATH_LIST ${CMAKE_SOURCE_DIR}/cmake)
list(APPEND CMAKE_MODULE_PATH ${SIMH_INCLUDE_PATH_LIST})
message(STATUS "CMAKE_MODULE_PATH: ${CMAKE_MODULE_PATH}")

## Deal with platform quirkiness
include(platform_quirks)

# SIMH_SYSTEM_ID: Roughly analogous to the autoconf system triple. Used (almost exclusively)
# as part of the depedencies' top-level directory name. 
set(SIMH_SYSTEM_ID ${CMAKE_SYSTEM_NAME})
string(REPLACE "." ";" version_list ${CMAKE_SYSTEM_VERSION})
list(GET version_list 0 version_major)
string(APPEND SIMH_SYSTEM_ID "-" ${version_major})

if (CMAKE_C_LIBRARY_ARCHITECTURE)
    string(APPEND SIMH_SYSTEM_ID -${CMAKE_C_LIBRARY_ARCHITECTURE})
endif (CMAKE_C_LIBRARY_ARCHITECTURE)
string(APPEND SIMH_SYSTEM_ID -${CMAKE_C_COMPILER_ID})
string(REPLACE "." ";" version_list ${CMAKE_C_COMPILER_VERSION})
list(GET version_list 0 version_major)
list(GET version_list 1 version_minor)
if (NOT version_minor)
  set(version_minor 0)
endif (NOT version_minor)
string(APPEND SIMH_SYSTEM_ID "-${version_major}.${version_minor}")

# SIMH_DEP_TOPDIR: This is the top-level directory where dependencies live. It's cached so that
# scripts (e.g., cmake-builder.ps1) can update PATH to find shared objects and DLLs.
#
# Only add to search paths if NO_DEP_BUILD is False
set(SIMH_DEP_TOPDIR
    ${CMAKE_SOURCE_DIR}/cmake/dependencies/${SIMH_SYSTEM_ID}
    CACHE PATH "Top-level directory for SIMH library dependencies")

# SIMH_DEP_PATCHES: This is where various external package patches live. Also cached just in case
# it's needed by a script.
set(SIMH_DEP_PATCHES ${CMAKE_SOURCE_DIR}/cmake/patches
        CACHE PATH "Top-level directory for external dependency patches")

##-- Options --##
if (NOT DEFINED NO_DEP_BUILD)
    ## For Windows, always build dependencies
    if (WIN32 AND NOT DEFINED ENV{MSYSTEM})
        message(STATUS "Setting NO_DEP_BUILD to FALSE, will BUILD missing dependencies")
        set(NO_DEP_BUILD_OPTVAL FALSE)
    else ()
        message(STATUS "Setting NO_DEP_BUILD to TRUE, will NOT BUILD missing dependencies")
        set(NO_DEP_BUILD_OPTVAL TRUE)
    endif ()
else ()
    set(NO_DEP_BUILD_OPTVAL ${NO_DEP_BUILD})
    if (NO_DEP_BUILD_OPTVAL)
        message(STATUS "Missing dependencies WILL NOT BE BUILT.")
    else ()
        message(STATUS "Missing dependencies WILL BE BUILT.")
    endif ()
endif ()

option(NO_DEP_BUILD
       "Enable (=1)/disable (=0) Dependency library builds (def: enabled)"
       ${NO_DEP_BUILD_OPTVAL})
option(BUILD_SHARED_DEPS
       "Enable (=1)/disable (=0) building dependencies as shared libraries/DLLs (def: disabled)"
       FALSE)
option(WITH_ASYNC
       "Enable (=1)/disable (=0) Asynchronous I/O and threading."
       TRUE)
option(WITH_REGEX
       "Enable (=1)/disable (=0) PCRE/PCRE2 regular expression support (def: enabled)"
       TRUE)
option(PREFER_PCRE
       "Prefer (=1)/ignore (=0) Prefer PCRE over PCRE2 (def: ignore)"
       TRUE)
option(WITH_NETWORK
       "Enable (=1)/disable (=0) simulator networking support. (def: enabled)"
       TRUE)
option(WITH_PCAP
       "Enable (=1)/disable (=0) libpcap (packet capture) support. (def: enabled)"
       TRUE)
option(WITH_SLIRP
       "Enable (=1)/disable (=0) SLIRP network support. (def: enabled)"
       TRUE)
option(WITH_VDE
       "Enable (=1)/disable (=0) VDE2/VDE4 network support. (def: enabled)"
       TRUE)
option(WITH_TAP
       "Enable (=1)/disable (=0) TAP/TUN device network support. (def: enabled)"
       TRUE)
option(WITH_VIDEO
       "Enable (=1)/disable (=0) simulator display and graphics support (def: enabled)"
       TRUE)
option(PANDA_LIGHTS
       "Enable (=1)/disable (=0) KA-10/KI-11 simulator's Panda display. (def: disabled)"
       FALSE)
option(DONT_USE_ROMS
       "Enable (=1)/disable (=0) building support ROMs. (def: disabled)"
       FALSE)
option(ENABLE_CPPCHECK
       "Enable (=1)/disable (=0) 'cppcheck' static code analysis. (def: disabled.)"
       FALSE)
option(WINAPI_DEPRECATION
       "Show (=1)/mute (=0) Various WinAPI deprecation warning (def: mute.)"
       FALSE)

# Places where CMake should look for dependent package configuration fragments and artifacts:
set(SIMH_PREFIX_PATH_LIST)
if (NOT NO_DEP_BUILD)
    list(APPEND CMAKE_INCLUDE_PATH ${SIMH_DEP_TOPDIR})
    list(APPEND SIMH_PREFIX_PATH_LIST ${SIMH_DEP_TOPDIR})
    list(APPEND CMAKE_PREFIX_PATH ${SIMH_PREFIX_PATH_LIST})
endif ()

## Additional command line arguments for dependencies. Need this because Powershell will report
## an error if anything is sent to stderr and $ErrorDefaultAction is set to "Stop".
set(DEP_CMAKE_ARGS "-Wno-dev" "--no-warn-unused-cli")

## build-stage directory hierarchy for dependency installs:
if (NOT NO_DEP_BUILD AND NOT EXISTS ${SIMH_DEP_TOPDIR})
    message(STATUS "Creating dependency library directory hierarchy")
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E make_directory ${SIMH_DEP_TOPDIR} ${SIMH_DEP_TOPDIR}/include ${SIMH_DEP_TOPDIR}/lib 
                    ${SIMH_DEP_TOPDIR}/bin
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )
endif ()

## Default install location is ${CMAKE_SOURCE_DIR}/BIN if not otherwise set
if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  set(CMAKE_INSTALL_PREFIX "${CMAKE_SOURCE_DIR}/" CACHE PATH "${CMAKE_SOURCE_DIR} default install prefix" FORCE)
  set(CMAKE_INSTALL_BINDIR BIN CACHE PATH "BIN default binary install directory" FORCE)

  if (CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_INSTALL_BINDIR "BIN/Win32/$<CONFIG>" CACHE PATH "BIN/Win32/<config> default binary install directory" FORCE)
  endif ()
endif()

if (NOT NO_DEP_BUILD)
    # Make sure we can include and link from the dependency directory
    list(APPEND CMAKE_LIBRARY_PATH ${SIMH_DEP_TOPDIR}/lib)
    link_directories(${SIMH_DEP_TOPDIR}/lib)
    list(APPEND CMAKE_INCLUDE_PATH ${SIMH_DEP_TOPDIR}/include)
    include_directories(${CMAKE_SOURCE_DIR} ${SIMH_DEP_TOPDIR}/include)
endif ()

set(EXTRA_CFLAGS "")

# Tweak compiler flags
if (MSVC)
    list(APPEND EXTRA_CFLAGS "$<$<CONFIG:Debug>:/W3>" "$<$<CONFIG:Debug>:/w34744>" "$<$<CONFIG:Debug>:/w34777>")
    # set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /verbose:lib")
elseif (CMAKE_C_COMPILER_ID STREQUAL "GNU" OR CMAKE_C_COMPILER_ID MATCHES ".*Clang")
    # include(fpintrin)

    # Turn on warnings about strict overflow/potential overflows.
    ## LIST(APPEND EXTRA_CFLAGS "-Wall" "-fno-inline" "-fstrict-overflow" "-Wstrict-overflow=3")
    LIST(APPEND EXTRA_CFLAGS "-Wformat" "-U__STRICT_ANSI__")

    # 'O3' optimization and strict overflow cause all kinds of simulator issues, especially inside
    # the VAX simulators. Reduce optimization and ensure strict overflow is turned off.
    message(STATUS "Replacing '-O3' with '-O2'")
    string(REGEX REPLACE "-O3" "-O2" CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}")
    string(REGEX REPLACE "-O3" "-O2" CMAKE_C_FLAGS_MINSIZEREL "${CMAKE_C_FLAGS_MINSIZEREL}")

    if (CMAKE_C_COMPILER_ID STREQUAL "GNU")
        message(STATUS "Adding GNU-specific optimizations to CMAKE_C_FLAGS_RELEASE")
        list(APPEND opt_flags "-finline-functions" "-fgcse-after-reload" "-fpredictive-commoning"
                              "-fipa-cp-clone" "-fno-unsafe-loop-optimizations" "-fno-strict-overflow")
    elseif (CMAKE_C_COMPILER_ID MATCHES ".*Clang")
        message(STATUS "Adding Clang-specific optimizations to CMAKE_C_FLAGS_RELEASE")
        list(APPEND opt_flags "-fno-strict-overflow")
    endif()

    foreach (opt_flag ${opt_flags})
        message(STATUS "    ${opt_flag}")
        string(REGEX REPLACE "${opt_flag}[ \t\r\n]*" "" CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}")
        string(APPEND CMAKE_C_FLAGS_RELEASE " ${opt_flag}")
        string(REGEX REPLACE "${opt_flag}[ \t\r\n]*" "" CMAKE_C_FLAGS_MINSIZEREL "${CMAKE_C_FLAGS_MINSIZEREL}")
        string(APPEND CMAKE_C_FLAGS_MINSIZEREL " ${opt_flag}")
    endforeach ()
else ()
    message(STATUS "Not touching CMAKE_C_FLAGS_RELEASE on ${CMAKE_C_COMPILER_ID}")
endif (MSVC)

## CMake policy tweaks

if (POLICY CMP0135)
    ## Until I figure out a way to add DOWNLOAD_EXTRACT_TIMESTAMP to the call to
    ## ExternalProject_Add cleanly..
    cmake_policy(SET CMP0135 NEW)
endif()

# Find packages, arrange for dependency download/compile/install:
#
# SIMH_BUILD_DEPS is the list of the dependecies' names, for pretty-printing.
# SIMH_DEP_TARGETS is the list of dependency targets' names that we'll actually build.

set(SIMH_BUILD_DEPS)
set(SIMH_DEP_TARGETS)

set(THREADING_PKG_STATUS "unknown")
set(ZLIB_PKG_STATUS "unknown")
set(PCRE_PKG_STATUS "unknown")
set(VIDEO_PKG_STATUS)
set(NETWORK_PKG_STATUS)

## "Standard" includes
include(FeatureSummary)
include(CheckCSourceCompiles)
include(CheckCCompilerFlag)
include(CheckIncludeFile)
include(CheckSymbolExists)
include(CheckTypeSize)
include(ExternalProject)
include(FindPackageHandleStandardArgs)

# SIMH local:
include(git-commit-id)
include(build_dep_matrix)
include(os-features)
include(dep-locate)
include(dep-targets)

if (VIDEO_PKG_STATUS)
    string(REPLACE ";" ", " VIDEO_PKG_STATUS "${VIDEO_PKG_STATUS}")
else (VIDEO_PKG_STATUS)
    set(VIDEO_PKG_STATUS "unknown")
endif (VIDEO_PKG_STATUS)

if (NETWORK_PKG_STATUS)
    string(REPLACE ";" ", " NETWORK_PKG_STATUS "${NETWORK_PKG_STATUS}")
else (NETWORK_PKG_STATUS)
    set(NETWORK_PKG_STATUS "unknown")
endif (NETWORK_PKG_STATUS)

set(CPPCHECK_STATUS "disabled.")

find_program(cppcheck_cmd NAMES cppcheck)
if (cppcheck_cmd)
    list(APPEND cppcheck_cmd
         "--language=c"
         "--enable=warning,style,performance,portability,information,missingInclude"
         "--suppress=missingIncludeSystem"
         "--inline-suppr"
         "--std=c99"
         "--force")
    set(CPPCHECK_STATUS "enabled.")
    if (WIN32)
        if(CMAKE_SIZEOF_VOID_P EQUAL 8)
            list(APPEND cppcheck_cmd
                 "--platform=win64")
            set(CPPCHECK_STATUS "enabled, Win64 platform.")
        else ()
            list(APPEND cppcheck_cmd
                 "--platform=win32A")
            set(CPPCHECK_STATUS "enabled, Win32 ASCII platform.")
        endif ()
    endif ()
else (cppcheck_cmd)
    set(CPPCHECK_STATUS "'cppcheck' not installed.")
endif ()

set(_feature_text "Libraries and features:\n")
string(APPEND _feature_text "\n    * Build with video/graphics support. ${BUILD_WITH_VIDEO}")
string(APPEND _feature_text "\n    * Build with networking support .... ${BUILD_WITH_NETWORK}")
string(APPEND _feature_text "\n    * Build internal ROMS .............. ")
if (DONT_SET_ROMS)
    string(APPEND _feature_text "No")
else ()
    string(APPEND _feature_text "Yes")
endif ()
string(APPEND _feature_text "\n    * Thread support ................... ${THREADING_PKG_STATUS}")
string(APPEND _feature_text "\n    * zlib ............................. ${ZLIB_PKG_STATUS}")
string(APPEND _feature_text "\n    * Perl-Compatible RegExps........... ${PCRE_PKG_STATUS}")
string(APPEND _feature_text "\n    * PNG, Freetype, SDL2, SDL2_ttf .... ${VIDEO_PKG_STATUS}")
string(APPEND _feature_text "\n    * Network support .................. ${NETWORK_PKG_STATUS}")

get_target_property(OS_FEATURE_DEFS os_features INTERFACE_COMPILE_DEFINITIONS)
list(LENGTH OS_FEATURE_DEFS len_os_features)
string(APPEND _feature_text "\n    * OS Feature definitions")
if (OS_FEATURE_DEFS)
    string(APPEND _feature_text ":")
    foreach (feature ${OS_FEATURE_DEFS})
        string(APPEND _feature_text "\n      .. ${feature}")
    endforeach()
else ()
    string(APPEND _feature_text " ........... None defined.")
endif ()

get_target_property(OS_FEATURE_LIBS os_features INTERFACE_LINK_LIBRARIES)
list(LENGTH OS_FEATURE_LIBS len_os_features)
string(APPEND _feature_text "\n    * OS Feature libraries")
if (OS_FEATURE_LIBS)
    string(APPEND _feature_text ":")
    foreach (feature ${OS_FEATURE_LIBS})
        string(APPEND _feature_text "\n      .. ${feature}")
    endforeach ()
else ()
    string(APPEND _feature_text " ............. None required.")
endif ()

string(APPEND _feature_text "\n    * 'cppcheck' ....................... ${CPPCHECK_STATUS}")
string(APPEND _feature_text "\n")

message(STATUS ${_feature_text})
unset(_feature_text)

## feature_summary(INCLUDE_QUIET_PACKAGES WHAT ALL)

unset(ROM_STATUS)

if (NO_DEP_BUILD AND SIMH_BUILD_DEPS)
    ## Don't build dependencies. Just wail about them.
    message("")
    message("Missing SIMH dependencies:")
    foreach (dep ${SIMH_BUILD_DEPS})
        message(STATUS "    ${dep}")
    endforeach()
    message("")
    message("Please install the above dependencies via your platform package management or")
    message("software installation system and reconfigure.")
    message("")
    message("Also see the .travis/deps.h file for Brew and apt packages installed during")
    message("github.com workflow actions.")
    message(FATAL_ERROR "Missing dependencies, cannot continue.")

    ## TODO: Assume that these dependencies are optional?
endif ()

if (NOT DEFINED DO_DEPENDENCY_BUILD OR SIMH_BUILD_DEPS)
    if (DEFINED DO_DEPENDENCY_BUILD AND NOT DO_DEPENDENCY_BUILD AND SIMH_BUILD_DEPS)
        message(FATAL_ERROR "Dependency libraries did not build successfully!!??")
    endif()

    if (SIMH_BUILD_DEPS)
        message(STATUS "Building dependency libraries as a superbuild")
        set(DO_DEPENDENCY_BUILD ON CACHE BOOL "Superbuild flag" FORCE)
    else ()
        set(DO_DEPENDENCY_BUILD OFF CACHE BOOL "Superbuild flag" FORCE)
    endif ()
else ()
    set(DO_DEPENDENCY_BUILD ${DO_DEPENDENCY_BUILD} CACHE BOOL "Superbuild flag" FORCE)
endif ()

if (NOT DO_DEPENDENCY_BUILD)
    include (add_simulator)
    if (WITH_SLIRP)
      add_subdirectory(slirp)
    endif (WITH_SLIRP)

## Don't delete yet ##    set(Python_ADDITIONAL_VERSIONS 3)
## Don't delete yet ##    include(FindPythonInterp)
## Don't delete yet ##    if (PYTHONINTERP_FOUND AND PYTHON_VERSION_MAJOR GREATER_EQUAL 3)
## Don't delete yet ##        add_custom_command(OUTPUT ${CMAKE_SOURCE_DIR}/cmake/simh-simulators.cmake
## Don't delete yet ##          COMMAND ${PYTHON_EXECUTABLE} "-m" generate "--srcdir" "${CMAKE_SOURCE_DIR}"
## Don't delete yet ##            MAIN_DEPENDENCY ${CMAKE_SOURCE_DIR}/makefile
## Don't delete yet ##            COMMENT "Updating cmake/simh-simulators.cmake"
## Don't delete yet ##            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/cmake
## Don't delete yet ##            COMMAND_EXPAND_LISTS)
## Don't delete yet ##        message(STATUS "Added makefile update rule.")
## Don't delete yet ##    endif (PYTHONINTERP_FOUND AND PYTHON_VERSION_MAJOR GREATER_EQUAL 3)
## Don't delete yet ##
## Don't delete yet ##    if (${CMAKE_SOURCE_DIR}/makefile IS_NEWER_THAN ${CMAKE_SOURCE_DIR}/cmake/simh-simulators.cmake)
## Don't delete yet ##        if (NOT PYTHONINTERP_FOUND OR PYTHON_VERSION_MAJOR LESS 3)
## Don't delete yet ##            if (NOT PYTHONINTERP_FOUND)
## Don't delete yet ##                message("!! Python not found")
## Don't delete yet ##            elseif (PYTHON_VERSION_MAJOR LESS 3)
## Don't delete yet ##                message("!! Python is not python3")
## Don't delete yet ##            endif (NOT PYTHONINTERP_FOUND)
## Don't delete yet ##
## Don't delete yet ##            message("!!")
## Don't delete yet ##            message("!! Cannot update cmake/simh-simulators.cmake, using older version.")
## Don't delete yet ##            message("!! Proceed with caution: This build might not succeed.")
## Don't delete yet ##            message("!!")
## Don't delete yet ##            message(STATUS "Keeping existing cmake/simh-simulators.cmake")
## Don't delete yet ##        else (NOT PYTHONINTERP_FOUND OR PYTHON_VERSION_MAJOR LESS 3)
## Don't delete yet ##            message(STATUS "Updating cmake/simh-simulators.cmake")
## Don't delete yet ##            execute_process(COMMAND ${PYTHON_EXECUTABLE} "-m" generate "--srcdir" "${CMAKE_SOURCE_DIR}"
## Don't delete yet ##                WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/cmake)
## Don't delete yet ##        endif (NOT PYTHONINTERP_FOUND OR PYTHON_VERSION_MAJOR LESS 3)
## Don't delete yet ##    endif ()

    message(STATUS "Adding simulators")
    include(simh-simulators)

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
    include(CPack)
else ()
    message(STATUS "")
    message(STATUS "Building the following dependency libraries:")
    message(STATUS "")
    foreach(dep ${SIMH_BUILD_DEPS})
        message(STATUS "    ${dep}")
    endforeach ()
    message(STATUS "")

    ExternalProject_Add (simh_superbuild
        DEPENDS
            ${SIMH_DEP_TARGETS}
        SOURCE_DIR
            ${CMAKE_SOURCE_DIR}
        CMAKE_ARGS
            -DDO_DEPENDENCY_BUILD:BOOL=OFF
            -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
            -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
            -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
        INSTALL_COMMAND ""
        BINARY_DIR
            ${CMAKE_CURRENT_BINARY_DIR}
        USES_TERMINAL_CONFIGURE TRUE
        USES_TERMINAL_BUILD     TRUE
    )

    ## Ensure that the next build rechecks the dependency
    ## libraries that were just built and finds them.
    unset(Freetype_FOUND CACHE)
    unset(PCAP_FOUND CACHE)
    unset(PCRE_FOUND CACHE)
    unset(PCRE2_FOUND CACHE)
    unset(PNG_FOUND CACHE)
    unset(PTW_FOUND CACHE)
    unset(SDL2_FOUND CACHE)
    unset(SDL2_ttf_FOUND CACHE)
    unset(ZLIB_FOUND CACHE)
endif ()
