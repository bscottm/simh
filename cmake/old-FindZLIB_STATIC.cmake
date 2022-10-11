# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

#[=======================================================================[.rst:
FindZLIBStatic
--------------

Find the native ZLIB includes and library. Under Windows, this usually finds the
dynamic library; this explicitly looks for the static version.

Adapted from the FindZLIB.cmake distributed with CMake.

#]=======================================================================]

set(_ZLIB_SEARCHES)

# Search ZLIB_ROOT first if it is set.
if(ZLIB_ROOT)
  set(_ZLIB_SEARCH_ROOT PATHS ${ZLIB_ROOT} NO_DEFAULT_PATH)
  list(APPEND _ZLIB_SEARCHES _ZLIB_SEARCH_ROOT)
endif()

# Normal search.
set(_ZLIB_x86 "(x86)")
set(_ZLIB_SEARCH_NORMAL
    PATHS "[HKEY_LOCAL_MACHINE\\SOFTWARE\\GnuWin32\\Zlib;InstallPath]"
          "$ENV{ProgramFiles}/zlib"
          "$ENV{ProgramFiles${_ZLIB_x86}}/zlib")
unset(_ZLIB_x86)
list(APPEND _ZLIB_SEARCHES _ZLIB_SEARCH_NORMAL)

# Just the static libraries
set(ZLIB_NAMES zlibstatic zlibstat zlib z)
set(ZLIB_NAMES_DEBUG zlibstaticd zlibstatd zlibd zd)

# Try each search configuration.
foreach(search ${_ZLIB_SEARCHES})
  find_path(ZLIB_STATIC_INCLUDE_DIR NAMES zlib.h ${${search}} PATH_SUFFIXES include)
endforeach()

# Allow ZLIB_STATIC_LIBRARY to be set manually, as the location of the zlib library
if(NOT ZLIB_STATIC_LIBRARY)
  set(_zlib_ORIG_CMAKE_FIND_LIBRARY_PREFIXES ${CMAKE_FIND_LIBRARY_PREFIXES})
  set(_zlib_ORIG_CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_FIND_LIBRARY_SUFFIXES})
  # Prefix/suffix of the win32/Makefile.gcc build
  if(WIN32)
    list(APPEND CMAKE_FIND_LIBRARY_PREFIXES "" "lib")
    list(APPEND CMAKE_FIND_LIBRARY_SUFFIXES ".dll.a")
  endif()
  # Support preference of static libs by adjusting CMAKE_FIND_LIBRARY_SUFFIXES
  if(WIN32)
    set(CMAKE_FIND_LIBRARY_SUFFIXES .lib .a ${CMAKE_FIND_LIBRARY_SUFFIXES})
  else()
    set(CMAKE_FIND_LIBRARY_SUFFIXES .a)
  endif()

  foreach(search ${_ZLIB_SEARCHES})
    find_library(ZLIB_STATIC_LIBRARY_RELEASE NAMES ${ZLIB_NAMES} NAMES_PER_DIR ${${search}} PATH_SUFFIXES lib)
    find_library(ZLIB_STATIC_LIBRARY_DEBUG NAMES ${ZLIB_NAMES_DEBUG} NAMES_PER_DIR ${${search}} PATH_SUFFIXES lib)
  endforeach()

  # Restore the original find library ordering
  set(CMAKE_FIND_LIBRARY_SUFFIXES ${_zlib_ORIG_CMAKE_FIND_LIBRARY_SUFFIXES})
  set(CMAKE_FIND_LIBRARY_PREFIXES ${_zlib_ORIG_CMAKE_FIND_LIBRARY_PREFIXES})

  include(SelectLibraryConfigurations)
  select_library_configurations(ZLIB_STATIC)
endif()

unset(ZLIB_NAMES)
unset(ZLIB_NAMES_DEBUG)

mark_as_advanced(ZLIB_STATIC_INCLUDE_DIR)

if(ZLIB_STATIC_INCLUDE_DIR AND EXISTS "${ZLIB_INCLUDE_DIR}/zlib.h")
    file(STRINGS "${ZLIB_INCLUDE_DIR}/zlib.h" ZLIB_H REGEX "^#define ZLIB_VERSION \"[^\"]*\"$")

    string(REGEX REPLACE "^.*ZLIB_VERSION \"([0-9]+).*$" "\\1" ZLIB_VERSION_MAJOR "${ZLIB_H}")
    string(REGEX REPLACE "^.*ZLIB_VERSION \"[0-9]+\\.([0-9]+).*$" "\\1" ZLIB_VERSION_MINOR  "${ZLIB_H}")
    string(REGEX REPLACE "^.*ZLIB_VERSION \"[0-9]+\\.[0-9]+\\.([0-9]+).*$" "\\1" ZLIB_VERSION_PATCH "${ZLIB_H}")
    set(ZLIB_VERSION_STRING "${ZLIB_VERSION_MAJOR}.${ZLIB_VERSION_MINOR}.${ZLIB_VERSION_PATCH}")

    # only append a TWEAK version if it exists:
    set(ZLIB_VERSION_TWEAK "")
    if( "${ZLIB_H}" MATCHES "ZLIB_VERSION \"[0-9]+\\.[0-9]+\\.[0-9]+\\.([0-9]+)")
        set(ZLIB_VERSION_TWEAK "${CMAKE_MATCH_1}")
        string(APPEND ZLIB_VERSION_STRING ".${ZLIB_VERSION_TWEAK}")
    endif()

    set(ZLIB_MAJOR_VERSION "${ZLIB_VERSION_MAJOR}")
    set(ZLIB_MINOR_VERSION "${ZLIB_VERSION_MINOR}")
    set(ZLIB_PATCH_VERSION "${ZLIB_VERSION_PATCH}")
endif()

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(ZLIB_STATIC REQUIRED_VARS ZLIB_STATIC_LIBRARY ZLIB_STATIC_INCLUDE_DIR
                                       VERSION_VAR ZLIB_VERSION_STRING)

if(ZLIB_STATIC_FOUND)
    set(ZLIB_STATIC_INCLUDE_DIRS ${ZLIB_STATIC_INCLUDE_DIR})

    if(NOT ZLIB_STATIC_LIBRARIES)
        set(ZLIB_STATIC_LIBRARIES ${ZLIB_STATIC_LIBRARY})
    endif()

    if(NOT TARGET ZLIBstatic::ZLIBstatic)
      add_library(ZLIBstatic::ZLIBstatic UNKNOWN IMPORTED)
      set_target_properties(ZLIBstatic::ZLIBstatic PROPERTIES
          INTERFACE_INCLUDE_DIRECTORIES "${ZLIB_STATIC_INCLUDE_DIRS}")

      if(ZLIB_STATIC_LIBRARY_RELEASE)
        set_property(TARGET ZLIBstatic::ZLIBstatic APPEND PROPERTY
          IMPORTED_CONFIGURATIONS RELEASE)
        set_target_properties(ZLIBstatic::ZLIBstatic PROPERTIES
            IMPORTED_LOCATION_RELEASE "${ZLIB_STATIC_LIBRARY_RELEASE}")
      endif()

      if(ZLIB_STATIC_LIBRARY_DEBUG)
        set_property(TARGET ZLIBstatic::ZLIBstatic APPEND PROPERTY
            IMPORTED_CONFIGURATIONS DEBUG)
        set_target_properties(ZLIBstatic::ZLIBstatic PROPERTIES
            IMPORTED_LOCATION_DEBUG "${ZLIB_STATIC_LIBRARY_DEBUG}")
      endif()

      if(NOT ZLIB_STATIC_LIBRARY_RELEASE AND NOT ZLIB_STATIC_LIBRARY_DEBUG)
        set_property(TARGET ZLIBstatic::ZLIBstatic APPEND PROPERTY
            IMPORTED_LOCATION "${ZLIB_STATIC_LIBRARY}")
      endif()
    endif()

    if (NOT TARGET ZLIB::ZLIB)
      if(CMAKE_VERSION VERSION_LESS "3.18")
        # FIXME: Aliasing local targets is not supported on CMake < 3.18, so make it global.
        add_library(ZLIB::ZLIB INTERFACE IMPORTED)
        set_target_properties(ZLIB::ZLIB PROPERTIES INTERFACE_LINK_LIBRARIES "ZLIBstatic::ZLIBstatic")
      else()
        add_library(ZLIB::ZLIB ALIAS ZLIBstatic::ZLIBstatic)
      endif ()
    endif ()
endif()
