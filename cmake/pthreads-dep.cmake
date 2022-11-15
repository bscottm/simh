#~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=
# Manage the pthreads dependency
#
# (a) Try to locate the system's installed pthreads library, which is very
#     platform dependent (MSVC -> Pthreads4w, MinGW -> pthreads, *nix -> pthreads.)
# (b) MSVC: Build Pthreads4w as a dependent
#~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=

add_library(thread_lib INTERFACE)

if (WITH_ASYNC)
    if (MSVC)
        find_package(PThreads4W)
        target_link_libraries(thread_lib INTERFACE PThreads4W::PThreads4W)
    else ()
        # Let CMake determine which threading library ought be used.
        set(THREADS_PREFER_PTHREAD_FLAG On)
        find_package(Threads)
        if (THREADS_FOUND)
          target_link_libraries(thread_lib INTERFACE Threads::Threads)
        endif (THREADS_FOUND)

        set(THREADING_PKG_STATUS "Platform-detected threading support")
    endif (MSVC)

    if (THREADS_FOUND OR PThreads4W_FOUND OR PTHREADS4W_FOUND)
        message(STATUS "Reader thread and SIM_ASYNC_IO enabled.")
        target_compile_definitions(thread_lib INTERFACE USE_READER_THREAD SIM_ASYNCH_IO)
    else ()
        message(STATUS "Reader thread and SIM_ASYNC_IO disabled (DONT_USE_READER_THREAD).")
        target_compile_definitions(thread_lib INTERFACE DONT_USE_READER_THREAD)
    endif ()
else (WITH_ASYNC)
    target_compile_definitions(thread_lib INTERFACE DONT_USE_READER_THREAD)
    set(THREADING_PKG_STATUS "asynchronous I/O disabled.")
endif (WITH_ASYNC)
