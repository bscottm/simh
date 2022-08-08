## platform_quirks.cmake
##
## This is the place where the CMake build handles various platform quirks,
## such as architecture-specific prefixes (Linux, Windows) and MacOS
## HomeBrew
##
## "scooter me fecit"

# For 64-bit builds (and this is especially true for MSVC), set the library
# architecture.
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    ## Strongly encourage (i.e., force) CMake to look in the x64 architecture
    ## directories:
    if (MSVC OR MINGW)
        set(CMAKE_C_LIBRARY_ARCHITECTURE "x64")
        set(CMAKE_LIBRARY_ARCHITECTURE "x64")
    elseif (${CMAKE_HOST_SYSTEM_NAME} MATCHES "Linux")
        ## Linux has architecture-specific subdirectories where CMake needs to
        ## search for headers. Currently, we know about x64 and ARM architecture
        ## variants.
        foreach (arch "x86_64-linux-gnu" "aarch64-linux-gnu" "arm-linux-gnueabihf")
            if (EXISTS "/usr/lib/${arch}")
                message(STATUS "CMAKE_LIBRARY_ARCHITECTURE set to ${arch}")
                set(CMAKE_C_LIBRARY_ARCHITECTURE "${arch}")
                set(CMAKE_LIBRARY_ARCHITECTURE "${arch}") 
            endif()
        endforeach()
    endif ()
endif()

if (WIN32)
    ## At some point, bring this back in to deal with MS ISO C99 deprecation.
    ## Right now, it's not in the code base and the warnings are squelched.
    ##
    ## (keep): if (MSVC_VERSION GREATER_EQUAL 1920)
    ## (keep):     add_compile_definitions(USE_ISO_C99_NAMES)
    ## (keep): endif ()

    if (MSVC)
        ## Flags enabled in the SIMH VS solution (diff redution):
        ##
        ## /EHsc: Standard C++ exception handling, extern "C" functions never
        ##        throw exceptions.
        ## /FC: Output full path name of source in diagnostics
        ## /GF: String pooling
        ## /GL: Whole program optimization
        ## /Gy: Enable function-level linking
        ## /Oi: Emit intrinsic functions
        ## /Ot: Favor fast code
        ## /Oy: Suppress generating a stack frame (??? why?)
        add_compile_options("$<$<CONFIG:Release>:/EHsc;/GF;/GL;/Gy;/Oi;/Ot;/Oy;/Zi>")
        add_compile_options("$<$<CONFIG:Debug>:/EHsc;/FC>")
        ## /LTCG: Link-Time Code Generation. Pairs with /GL above.
        add_link_options($<$<CONFIG:Release>:/LTCG>)

        ## Disable automagic add for _MBCS:
        add_definitions(-D_SBCS)
    endif ()
elseif (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    # The MSVC solution builds as 32-bit, but none of the *nix platforms do.
    #
    # If 32-bit compiles have to be added back, uncomment the following 2 lines:
    #
    # add_compile_options("-m32")
    # set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -m32")
endif ()

if (CMAKE_HOST_APPLE)
    ## Look for app bundles and frameworks after looking for Unix-style packages:
    set(CMAKE_FIND_FRAMEWORK "LAST")
    set(CMAKE_FIND_APPBUNDLE "LAST")

    if (EXISTS "/usr/local/Cellar" OR EXISTS "/opt/homebrew/Cellar")
        ## Smells like HomeBrew. Bulk add the includes and library subdirectories
        message(STATUS "Adding HomeBrew paths to library and include search")
        set(hb_topdir "/usr/local/Cellar")
        if (EXISTS "/opt/homebrew/Cellar")
            set(hb_topdir "/opt/homebrew/Cellar")
        endif()

        file(GLOB hb_lib_candidates LIST_DIRECTORIES TRUE "${hb_topdir}/*/*/lib")
        file(GLOB hb_include_candidates LIST_DIRECTORIES TRUE "${hb_topdir}/*/*/include")

        # message("@@ lib candidates ${hb_lib_candidates}")
        # message("@@ inc candidates ${hb_include_candidates}")

        set(hb_libs "")
        foreach (hb_path ${hb_lib_candidates})
            if (IS_DIRECTORY "${hb_path}")
                # message("@@ consider ${hb_path}")
                list(APPEND hb_libs "${hb_path}")
            endif()
        endforeach()

        set(hb_includes "")
        foreach (hb_path ${hb_include_candidates})
            if (IS_DIRECTORY "${hb_path}")
                # message("@@ consider ${hb_path}")
                list(APPEND hb_includes "${hb_path}")
            endif()
        endforeach()

        # message("hb_libs ${hb_libs}")
        # message("hb_includes ${hb_includes}")

        list(PREPEND CMAKE_LIBRARY_PATH ${hb_libs})
        list(PREPEND CMAKE_INCLUDE_PATH ${hb_includes})

        unset(hb_lib_candidates)
        unset(hb_include_candidates)
        unset(hb_includes)
        unset(hb_libs)
        unset(hb_path)
    elseif(EXISTS /opt/local/bin/port)
        # MacPorts
        list(PREPEND CMAKE_LIBRARY_PATH /opt/local/lib)
        list(PREPEND CMAKE_INCLUDE_PATH /opt/local/include)
    endif()
endif()
