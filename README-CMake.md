# Build `simh` using CMake


- [Build `simh` using CMake](#build-simh-using-cmake)
  - [Why CMake?](#why-cmake)
  - [Building `simh` With CMake](#building-simh-with-cmake)
    - [Prerequisites](#prerequisites)
      - [Supported C Compilers](#supported-c-compilers)
      - [Packages and Dependencies](#packages-and-dependencies)
    - [Quickstart for the Impatient](#quickstart-for-the-impatient)
    - [Building `simh` via `cmake` manually](#building-simh-via-cmake-manually)
      - [Linux/Unix/Unix-like platform example](#linuxunixunix-like-platform-example)
    - [Notes for Windows Visual Studio](#notes-for-windows-visual-studio)
    - [Configuration Options](#configuration-options)
  - [`CMake` Generators](#cmake-generators)
  - [Developer Notes](#developer-notes)
    - [`add_simulator`: Compiling simulators](#add_simulator-compiling-simulators)


## Why CMake?

[CMake][cmake] is a cross-platform meta-build system that provides similar
functionality to GNU _autotools_ within a more integrated and platform-agnostic
framework. A sample of the supported build environments include:

  - Unix Makefiles
  - [MinGW Makefiles][mingw64]
  - [Ninja][ninja]
  - MS Visual Studio solutions (2015, 2017, 2019, 2022)
  - IDE build wrappers ([Sublime Text][sublime] and [CodeBlocks][codeblocks])

Some of the [CMake][cmake] infrastructure objectives include:

  - Support a wider variety of platforms and compiler combinations
  - Better cross-platform SIMH support
  - Compile the simulator support code (`scp.c`, `sim_*.c` and friends) as
    static libraries that link to each simulator, vice repeatedly compiling them
    with each simulator.
  - Better IDE integration.

## Building `simh` With CMake

### Prerequisites

#### Supported C Compilers

| Compiler                 | Notes                                                 |
| ------------------------ | ----------------------------------------------------- |
| _GNU C Compiler (gcc)_   | This is one of two compilers against which `simh` is routinely compiled. `gcc` is the default compiler for many Linux and Unix/Unix-like platforms; it can also be used for [Mingw-w64][mingw64]-based builds on Windows. |
| _Microsoft Visual C/C++_ | This is the other compiler against which `simh` is routinely compiled. |
| _CLang/LLVM_             | `clang` is the default compiler on MacOS and is known to build `simh` successfully. `clang` is untested on Linux and Unix/Unix-like platforms. `clang` is broken on Windows while building the `libpng` library as a dependency. |

[CMake][cmake] "success" reports on platforms other than Linux, MacOS,
Unix/Unix-like and Windows systems are happily accepted. Patches to the
[CMake][cmake] infrastructure are gratefully accepted.

#### Packages and Dependencies

- For Linux `apt`-based distributions, these are the installed dependencies for the Github CI/CD workflow:

  ```
  sudo apt-get update -yqqm
  sudo apt-get install -ym pkg-config
  sudo apt-get install -ym libegl1-mesa-dev libgles2-mesa-dev
  sudo apt-get install -ym libsdl2-dev libpcap-dev libvdeplug-dev
  sudo apt-get install -ym libsdl2-ttf-dev
  sudo apt-get install -ym cmake cmake-data
  ```

- For the MacOS `brew` package manager, these are the installed dependencies for the Github CI/CD workflow:

  ```
  brew update
  brew install pkg-config
  brew install sdl2
  brew install sdl2_ttf
  brew install vde
  brew install cmake gnu-getopt coreutils
  ```

- Windows

  - The preferred compiler is Microsoft's Visual Studio C/C++.

    - [MinGW-w32 and MinGW-w64][mingw64] will work, if you manually edit
      `sim_disk.c` and change all occurances of `CreateVirtualDisk` to
      `CreateVDisk` (or something other than `CreateVirtualDisk`.)

    - Clang for Windows support is awaiting a merge request for the `libpng`
      library, which experiences a library name clash.

  - Install `cmake` and `git` using one of the several Windows package managers
    ([Scoop][scoop], [Chocolatey][chocolatey], ...)

    ```
    PS> scoop install cmake git
    ```

  - The [CMake][cmake] build infrastructure currently builds required dependency
    libraries as a "superbuild" -- `cmake` will download and build libraries
    that it cannot find or detect, then re-runs `cmake` configuration and builds
    the simulators.

  - _TODO_: Download and install dependency libraries using the `vcpkg` package
    manager.

### Quickstart for the Impatient

There are two scripts, relative to the `simh` source directory, that build `simh` using [CMake][cmake].

```bash
# Linux/Unix/Unix-like:
$ git clone https://github.com/open-simh/simh.git
$ cd simh
$ cmake/cmake-builder.sh
# List the supported command line flags:
$ cmake/cmake-builder.sh --help
```

```PowerShell
# Windows PowerShell
PS> git clone https://github.com/open-simh/simh.git
PS> cd simh
PS> cmake/cmake-builder.ps1
# List the supported command line flags:
PS> cmake/cmake-builder.ps1 -help
```

### Building `simh` via `cmake` manually

Building `simh` by invoking the `cmake` command follows the steps below:

1. Clone the `simh` Git repository, if not already cloned.

2. Create a subdirectory in which you will build the simulators.
 
     - This [CMake][cmake] configuration **will not allow you** to configure,
       build or compile `simh` in the source tree's top level directory. This will not work:

        ```
        $ git clone https://github.com/open-simh/simh.git
        $ cd simh
        $ cmake -G "Unix Makefiles" -S .
 
        *** Do NOT build or generate CMake artifacts in the SIMH source directory! ***
        
        Create a subdirectory and build in that subdirectory, e.g.:
        
          $ mkdir cmake-build
          $ cd cmake-build
          $ cmake -G \"your generator here\" ..
        
        Preventing in-tree source build.
        ```

     - Building `simh` simulators with [CMake][cmake] ***must*** be done in a
       separate directory, which is usually a subdirectory within the source tree.

     - The `cmake/cmake-builder.sh` and `cmake/cmake-builder.ps1` scripts create
       subdirectories under the `cmake` directory with the prefix `build-`, e.g.,
       `build-unix` for Unix `Makefile` builds, `build-ninja` for Ninja-based
       builds, and `build-vs2022` for Visual C/C++ 2022.

     - The `cmake` build directory doesn't have to be within the `simh` source
       tree. It could be in `/tmp`.

3. Configure the build environment using a [CMake generator][#cmake-generators]
  
     - (Windows) Create a superbuild for missing dependencies

     - (Non-windows) `cmake` will output a diagnostic message for missing
       dependencies, such as [SDL2][SDL2].

4. Compile the simulators

#### Linux/Unix/Unix-like platform example

```bash
# Clone the simh repository (if you haven't done so already)
$ git clone https://github.com/open-simh/simh.git
$ cd simh

# Make a build directory and generate Unix Makefiles
$ mkdir cmake/build-unix
$ cd cmake/build-unix
# Note: Use "Debug" instead of "Release" for a debug build. If you
# need to switch from one configuration to another, remove
# CMakeCache.txt and the CMakeFiles subdirectory
$ cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release -S ../..

# Build the simulators (see "Debug" note above)
$ cmake --build .
# Need to build a specific simulator, such as b5500?
$ cmake --build . --target b5500
# Since cmake generated a Makefile and we're in the same directory as
# the Makefile:
$ make b5500
# Run the tests with a 5 minute timeout per test (most tests only require 2
# minutes, SEL32 is a notable exception)
$ ctest --build-config Release --output-on-failure --timeout 300

# Alternative build: Ninja
$ cd ..
$ mkdir build-ninja; cd build-ninja
# Generate Ninja infrastructure
$ cmake -G Ninja -DCMAKE_BUILD_TYPE=Release -S ../..
# Build all of the simulators
$ cmake --build .
# Build a specific simulator
$ cmake --build . --target 3b2
# Build the 3b2 simulator using ninja if we're in the same subdirectory
# as build.ninja:
$ ninja 3b2
# Run the tests
$ ctest --build-config Release --output-on-failure --timeout 300

# Install will install to the top level `BIN` directory inside the source tree:
$ cmake --install .
```

### Notes for Windows Visual Studio

The source tree versions of the Visual Studio project files/solutions build a
32-bit executable. To do this with [CMake][cmake], you have to specify the target
architecture at configuration time as follows:

```PowerShell
# Create a Visual Studio build directory
PS> mkdir cmake/build-vstudio
PS> cd cmake/build-vstudio

# Visual Studio 2019 and 2022 have an architecture argument, "-A", and defaults
# to a 64-bit architecture:
PS> cmake -G "Visual Studio 17 2022" -A Win32 -S ../..
PS> cmake -G "Visual Studio 16 2019" -A Win32 -S ../..

# Prior Visual Studios don't and default to Win32
PS> cmake -G "Visual Studio 15 2017" -S ../..
PS> cmake -G "Visual Studio 14 2015" -S ../..
PS> cmake -G "Visual Studio 12 2013" -S ../..
PS> cmake -G "Visual Studio 11 2012" -S ../..
PS> cmake -G "Visual Studio 10 2010" -S ../..
PS> cmake -G "Visual Studio 9 2008" -S ../..
```

The `cmake` Visual Studio generators also create the solution file, which you
can open from within Visual Studio. In the above example, look for the `.sln`
file underneath the `cmake/build-vstudio` subdirectory.

The PowerShell example follows the same basic flow as the Linux example with one
important difference: Visual Studio determines which compile configuration to
follow at compile time (Debug vs. Release). Consequently, `CMAKE_BUILD_TYPE` is
not set on the command line, as it is for the Linux example. Instead, `--config`
specifies the compile configuration when building the simulators.

```PowerShell
# Clone the simh repository (if you haven't done so already)
PS> git clone https://github.com/open-simh/simh.git
PS> cd simh

# Make a build directory
PS> mkdir cmake/build-vs2022
PS> cd cmake/build-vs2022
# Generate the VS structure (look for the VS solution here after generating
# is done.)
PS> cmake -G "Visual Studio 17 2022" -A Win32 -T host=x64 -S ../..

# Build the simulators (could use "Debug" instead of "Release")
PS> cmake --build . --config Release
# Run the tests with a 5 minute timeout per test (most tests only require 2
# minutes, SEL32 is a notable exception)
PS> ctest --build-config Release --output-on-failure --timeout 300

# Install will install to the top level `BIN\Win32\<Debug|Release>` directory
# inside the source tree:
PS> cmake --install .
```

### Configuration Options

The default `simh` [CMake][cmake] configuration is _"Batteries Included"_: all
options are enabled. The configuration options generally mirror those in the
original `simh` `makefile`:

* `WITH_NETWORK`: Enable (=1)/disable (=0) simulator networking support. (def: enabled)
* `WITH_PCAP`: Enable (=1)/disable (=0) libpcap (packet capture) support. (def: enabled)
* `WITH_SLIRP`: Enable (=1)/disable (=0) SLIRP network support. (def: enabled)
* `WITH_VIDEO`: Enable (=1)/disable (=0) simulator display and graphics support (def: enabled)
* `WITH_ASYNC`: Enable (=1)/disable (=0) simulator asynchronous I/O (def: enabled)
* `PANDA_LIGHTS`: Enable (=1)/disable (=0) KA-10/KI-11 simulator's Panda display. (def: disabled)
* `DONT_USE_ROMS`: Enable (=1)/disable (=0) building hardcoded support ROMs. (def: disabled)
* `ENABLE_CPPCHECK`: Enable (=1)/disable (=0) [cppcheck][cppcheck] static code checking rules.

[CMake][cmake] enables (or disables) options at configuration time:

```bash
# Assuming that you are in the cmake/build-unix build subdirectory already.
# Remove the CMakeCache.txt file and CMakeFiles subdirectory if you are
# reconfiguring your build system:
$ rm -rf CMakeCache.txt CMakeFiles/

# Then reconfigure:
$ cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release -DWITH_NETWORK=Off -DENABLE_CPPCHECK=Off

# Alteratively ("0" and "Off" are equivalent)
$ cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release -DWITH_NETWORK=0 -DENABLE_CPPCHECK=0
```

## `CMake` Generators

[CMake][cmake] generates environments for a wide variety of build systems. The available list of build systems in your installed [CMake][cmake] is always available via:

```shell
$ cmake --help
# (Some help text elided here for brevity...)

Generators

The following generators are available on this platform (* marks default):
* Visual Studio 16 2019        = Generates Visual Studio 2019 project files.
                                 Use -A option to specify architecture.
  Visual Studio 15 2017 [arch] = Generates Visual Studio 2017 project files.
                                 Optional [arch] can be "Win64" or "ARM".
  Visual Studio 14 2015 [arch] = Generates Visual Studio 2015 project files.
                                 Optional [arch] can be "Win64" or "ARM".
  Visual Studio 12 2013 [arch] = Generates Visual Studio 2013 project files.
                                 Optional [arch] can be "Win64" or "ARM".
  Visual Studio 11 2012 [arch] = Generates Visual Studio 2012 project files.
                                 Optional [arch] can be "Win64" or "ARM".
  Visual Studio 10 2010 [arch] = Generates Visual Studio 2010 project files.
                                 Optional [arch] can be "Win64" or "IA64".
  Visual Studio 9 2008 [arch]  = Generates Visual Studio 2008 project files.
                                 Optional [arch] can be "Win64" or "IA64".
  Borland Makefiles            = Generates Borland makefiles.
  NMake Makefiles              = Generates NMake makefiles.
  NMake Makefiles JOM          = Generates JOM makefiles.
  MSYS Makefiles               = Generates MSYS makefiles.
  MinGW Makefiles              = Generates a make file for use with
                                 mingw32-make.
  Unix Makefiles               = Generates standard UNIX makefiles.
  Green Hills MULTI            = Generates Green Hills MULTI files
                                 (experimental, work-in-progress).
  Ninja                        = Generates build.ninja files.
  Watcom WMake                 = Generates Watcom WMake makefiles.
  CodeBlocks - MinGW Makefiles = Generates CodeBlocks project files.
  CodeBlocks - NMake Makefiles = Generates CodeBlocks project files.
  CodeBlocks - NMake Makefiles JOM
                               = Generates CodeBlocks project files.
  CodeBlocks - Ninja           = Generates CodeBlocks project files.
  CodeBlocks - Unix Makefiles  = Generates CodeBlocks project files.
  CodeLite - MinGW Makefiles   = Generates CodeLite project files.
  CodeLite - NMake Makefiles   = Generates CodeLite project files.
  CodeLite - Ninja             = Generates CodeLite project files.
  CodeLite - Unix Makefiles    = Generates CodeLite project files.
  Sublime Text 2 - MinGW Makefiles
                               = Generates Sublime Text 2 project files.
  Sublime Text 2 - NMake Makefiles
                               = Generates Sublime Text 2 project files.
  Sublime Text 2 - Ninja       = Generates Sublime Text 2 project files.
  Sublime Text 2 - Unix Makefiles
                               = Generates Sublime Text 2 project files.
  Kate - MinGW Makefiles       = Generates Kate project files.
  Kate - NMake Makefiles       = Generates Kate project files.
  Kate - Ninja                 = Generates Kate project files.
  Kate - Unix Makefiles        = Generates Kate project files.
  Eclipse CDT4 - NMake Makefiles
                               = Generates Eclipse CDT 4.0 project files.
  Eclipse CDT4 - MinGW Makefiles
                               = Generates Eclipse CDT 4.0 project files.
  Eclipse CDT4 - Ninja         = Generates Eclipse CDT 4.0 project files.
  Eclipse CDT4 - Unix Makefiles= Generates Eclipse CDT 4.0 project files.
```
## Developer Notes

### `add_simulator`: Compiling simulators

If you hack the simulators and add (or remove) source files, you will have to
update the affected simulator's `CMakeLists.txt`. 

The `add_simulator` function sets up the individual simulator executable. For
example, in the `3B2/CMakeLists.txt`, the 3b2 simulator's executable
`add_simulator` looks like:

```cmake
add_simulator(3b2
    SOURCES
        3b2_cpu.c
        3b2_sys.c
        3b2_rev2_sys.c
        3b2_rev2_mmu.c
        3b2_rev2_mau.c
        3b2_rev2_csr.c
        3b2_rev2_timer.c
        3b2_stddev.c
        3b2_mem.c
        3b2_iu.c
        3b2_if.c
        3b2_id.c
        3b2_dmac.c
        3b2_io.c
        3b2_ports.c
        3b2_ctc.c
        3b2_ni.c
    INCLUDES
        ${CMAKE_CURRENT_SOURCE_DIR}
    DEFINES
        REV2
    FEATURE_FULL64
    LABEL 3B2
    TEST 3b2)
```

- `add_simulator`'s first argument is the simulator's executable name: `3b2`.
  This generates an executable named `3b2` on Unix platforms or `3b2.exe` on
  Windows platforms.
  
- Argument list keywords: `SOURCES`, `INCLUDES`, `DEFINES`

    - `SOURCES`: The source files that comprise the simulator. The file names
      are relative to the simulator's source directory. In the `3b2`'s case,
      this is relative to the `3B2/` subdirectory where `3B2/CMakeLists.txt` is
      located. [CMake][cmake] sets the variable `CMAKE_CURRENT_SOURCE_DIR` to
      the same directory from which `CMakeLists.txt` is being read.

    - `INCLUDES`: The include directories where the header files needed by the
      simulator are located, i.e., subdirectories that follow the compiler's
      `-I` flag). These subdirectories are relative to the top level `simh`
      directory.

    - `DEFINES`: Command line manifest constants, i.e., values that follow the
      compiler's `-D` flags.

- Option keywords:
  - `FEATURE_INT64`: 64-bit integers, 32-bit pointers
  - `FEATURE_FULL64`: 64-bit integers, 64-bit pointers
  - `BUILDROMS`: Simulator depends on the `BuildROMs` utility to build the
    built-in boot ROMs.
  - `FEATURE_VIDEO`: Simulator video support.
  - `FEATURE_DISPLAY`: Video display support.

Putting it all together:

```cmake
add_simulator(my_simulator
    SOURCES
        my_sim.c
        my_sim_devs.c
        my_sim_support.c
    INCLUDES
        my_simulator                # Relative to the top-level source directory
        ${CMAKE_SOURCE_DIR}/PDP8    # Add top-level PDP8 subdirectory, explicit path
    DEFINES
        VM_SIMH_SIMULATOR
        SPECIAL_VALUE=0xdeadbeef
    FEATURE_FULL64                  # 64-bit integer, 64-bit pointer option
    BUILDROMS                       # Boot ROM is built-in header file
    FEATURE_VIDEO                   # Video support
)
```


[cmake]: https://cmake.org
[cppcheck]: http://cppcheck.sourceforge.net/
[ninja]: https://ninja-build.org/
[scoop]: https://scoop.sh/
[gitscm]: https://git-scm.com/
[bison]: https://www.gnu.org/software/bison/
[flex]: https://github.com/westes/flex
[npcap]: https://nmap.org/npcap/
[zlib]: https://www.zlib.net
[pcre2]: https://pcre.org
[libpng]: http://www.libpng.org/pub/png/libpng.html
[FreeType]: https://www.freetype.org/
[libpcap]: https://www.tcpdump.org/
[SDL2]: https://www.libsdl.org/
[SDL2_ttf]: https://www.libsdl.org/projects/SDL_ttf/
[mingw64]: https://mingw-w64.org/
[winflexbison]: https://github.com/lexxmark/winflexbison
[pthreads4w]: https://github.com/jwinarske/pthreads4w
[chocolatey]: https://chocolatey.org/
[sublime]: https://www.sublimetext.com
[codeblocks]: http://www.codeblocks.org
[coreutils]: https://www.gnu.org/software/coreutils/coreutils.html
[util-linux]: https://git.kernel.org/pub/scm/utils/util-linux/util-linux.git/
