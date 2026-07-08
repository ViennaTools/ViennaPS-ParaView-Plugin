cmake_minimum_required(VERSION 3.20 FATAL_ERROR)
project(ViennaPSPluginModule LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(BUILD_SHARED_LIBS "Build shared libraries" ON)

include(GNUInstallDirs)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_BINDIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR}")

find_package(ParaView REQUIRED)

find_package(OpenMP)
if(OpenMP_CXX_FOUND)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
endif()

include(cmake/cpm.cmake)

CPMAddPackage(
  NAME ViennaLS
  VERSION 5.8.3
  GIT_REPOSITORY "https://github.com/ViennaTools/ViennaLS"
  GIT_TAG b0c35942d477b72c2439d06669276ac653caa470
  OPTIONS "VIENNALS_USE_VTK ON" "VIENNALS_VTK_RENDERING OFF")

CPMAddPackage(
  NAME ViennaPS
  VERSION 4.6.1
  GIT_REPOSITORY "https://github.com/ViennaTools/ViennaPS"
  GIT_TAG 2956ed587984c6dc38be24c6e2390e10c9b2f0a7
  OPTIONS "VIENNAPS_USE_GPU OFF")

add_compile_definitions(VIENNARAY_USE_RAY_MASKING)

enable_testing()

set("_paraview_plugin_default_${PROJECT_NAME}" ON)
paraview_plugin_scan(PLUGIN_FILES "${CMAKE_CURRENT_SOURCE_DIR}/Plugin/paraview.plugin"
                     PROVIDES_PLUGINS plugins REQUIRES_MODULES required_modules)

foreach(module IN LISTS required_modules)
  if(NOT TARGET "${module}")
    message(WARNING "Skipping plugin ${PROJECT_NAME}: missing required module: ${module}")
    return()
  endif()
endforeach()

paraview_plugin_build(
  RUNTIME_DESTINATION
  "${CMAKE_INSTALL_BINDIR}"
  LIBRARY_DESTINATION
  "${CMAKE_INSTALL_LIBDIR}"
  LIBRARY_SUBDIRECTORY
  "${PARAVIEW_PLUGIN_SUBDIR}"
  PLUGINS
  ${plugins})
