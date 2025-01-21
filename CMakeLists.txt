set(CMAKE_VERBOSE_MAKEFILE ON)

cmake_minimum_required(VERSION 3.16)
project(
  wav2png
  VERSION 1.0
  LANGUAGES CXX)
add_compile_options(-fcolor-diagnostics)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 14)

option(BUILD_STANDALONE "Should there be a standalone of the application?" OFF)

if(BUILD_STANDALONE)
  include(CPM.cmake)
  cpmaddpackage("gh:colugomusic/blahdio#b67a349")
  add_executable(wav2png src/main.cpp)
  target_include_directories(wav2png
                             PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
  target_link_libraries(wav2png blahdio)
else()

  include(GNUInstallDirs)

  add_library(wav2png INTERFACE)
  target_include_directories(
    wav2png INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
                      $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

  include(CPM.cmake)

  install(TARGETS wav2png EXPORT wav2pngTargets)
  install(
    EXPORT wav2pngTargets
    FILE wav2pngConfig.cmake
    NAMESPACE wav2png::
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/wav2png)
  install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

  include(CMakePackageConfigHelpers)
  write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/wav2pngConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion)
  install(FILES "${CMAKE_CURRENT_BINARY_DIR}/wav2pngConfigVersion.cmake"
          DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/wav2png)
endif()
