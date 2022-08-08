#!/bin/sh

install_osx() {
    brew update
    brew install pkg-config
    brew install sdl2
    brew install sdl2_ttf
    brew install vde
    brew install cmake gnu-getopt coreutils
}

install_linux() {
    sudo apt-get update -yqqm
    sudo apt-get install -ym pkg-config
    sudo apt-get install -ym libegl1-mesa-dev libgles2-mesa-dev
    sudo apt-get install -ym libsdl2-dev libpcap-dev libvdeplug-dev
    sudo apt-get install -ym libsdl2-ttf-dev
    sudo apt-get install -ym cmake cmake-data
    sudo apt-get install -ym libedit-dev
}

install_mingw64() {
    pacman -S --needed mingw-w64-x86_64-ninja \
        mingw-w64-x86_64-toolchain mingw-w64-x86_64-make \
        mingw-w64-x86_64-pcre mingw-w64-x86_64-freetype \
        mingw-w64-x86_64-SDL2 mingw-w64-x86_64-SDL2_ttf
}

install_ucrt64() {
    pacman -S --needed mingw-w64-ucrt-x86_64-ninja \
        mingw-w64-ucrt-x86_64-toolchain mingw-w64-ucrt-x86_64-make \
        mingw-w64-ucrt-x86_64-pcre mingw-w64-ucrt-x86_64-freetype \
        mingw-w64-ucrt-x86_64-SDL2 mingw-w64-ucrt-x86_64-SDL2_ttf
}

install_"$1"
