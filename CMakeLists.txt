
set(CMAKE_VERBOSE_MAKEFILE ON)

cmake_minimum_required(VERSION 3.16)
project(wav2png)
add_compile_options (-fcolor-diagnostics)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 14)

include(CPM.cmake)

CPMAddPackage("gh:colugomusic/blahdio#b67a349")


add_executable(wav2png
  src/main.cpp
)

target_link_libraries(wav2png blahdio)