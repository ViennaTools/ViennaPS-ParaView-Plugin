cmake_minimum_required(VERSION 3.8)
project(ViennaPSPluginModule)

find_package(ParaView REQUIRED)

set(PARAVIEW_SOURCE_DIR "~/BachelorThesis/paraview")
set(PARAVIEW_BUILD_DIR "~/BachelorThesis/paraview/build")

include_directories(
  ${PARAVIEW_BUILD_DIR}/Remoting/ClientServerStream
  ${PARAVIEW_BUILD_DIR}/Remoting/ServerManager
  ${PARAVIEW_BUILD_DIR}/VTK/Filters/General
  ${PARAVIEW_BUILD_DIR}/VTK/Common/Core
  ${PARAVIEW_BUILD_DIR}/VTK/Common/DataModel
  ${PARAVIEW_BUILD_DIR}/VTK/Common/ExecutionModel
  ${PARAVIEW_BUILD_DIR}/VTK/IO/XML
  ${PARAVIEW_BUILD_DIR}/VTK/IO/Core
  ${PARAVIEW_BUILD_DIR}/VTK/IO/Legacy
  ${PARAVIEW_BUILD_DIR}/VTK/Filters/Core
  ${PARAVIEW_BUILD_DIR}/VTK/Filters/Sources
  ${PARAVIEW_BUILD_DIR}/VTK/Filters/Geometry
  ${PARAVIEW_BUILD_DIR}/VTK/Rendering/Core
  ${PARAVIEW_BUILD_DIR}/VTK

  ${PARAVIEW_SOURCE_DIR}/Remoting/ClientServerStream
  ${PARAVIEW_SOURCE_DIR}/Remoting/ServerManager
  ${PARAVIEW_SOURCE_DIR}/VTK/Common/Core
  ${PARAVIEW_SOURCE_DIR}/VTK/Common/DataModel
  ${PARAVIEW_SOURCE_DIR}/VTK/Common/ExecutionModel
  ${PARAVIEW_SOURCE_DIR}/VTK/IO/XML
  ${PARAVIEW_SOURCE_DIR}/VTK/IO/Core
  ${PARAVIEW_SOURCE_DIR}/VTK/IO/Legacy
  ${PARAVIEW_SOURCE_DIR}/VTK/Filters/Core
  ${PARAVIEW_SOURCE_DIR}/VTK/Filters/General
  ${PARAVIEW_SOURCE_DIR}/VTK/Filters/Sources
  ${PARAVIEW_SOURCE_DIR}/VTK/Filters/Geometry
  ${PARAVIEW_SOURCE_DIR}/VTK/Rendering/Core
  ${PARAVIEW_SOURCE_DIR}/VTK/Utilities/KWIML

  ${PARAVIEW_BUILD_DIR}
)

add_compile_definitions(VIENNARAY_EMBREE_VERSION=4)

# Enable VTK support in ViennaLS
add_compile_definitions(VIENNALS_USE_VTK=1)
add_compile_definitions(VIENNARAY_USE_RAY_MASKING)

set(VIENNAHRLE_INCLUDE_NAME "viennahrle-1.0.0")
set(EMBREE_INCLUDE_NAME "embree4")
set(VIENNACS_INCLUDE_NAME "viennacs-2.0.1")

include_directories("~/BachelorThesis/ViennaPS/include/viennaps")
include_directories("~/BachelorThesis/ViennaCore/include/viennacore")
include_directories("/usr/local/include/${VIENNAHRLE_INCLUDE_NAME}")
include_directories("~/BachelorThesis/ViennaLS/include/viennals")
include_directories("/usr/local/include/${EMBREE_INCLUDE_NAME}")
include_directories("~/BachelorThesis/ViennaRay/include/viennaray")
include_directories("/usr/local/include/${VIENNACS_INCLUDE_NAME}")

find_library(EMBREE_LIBRARY
        NAMES embree4 embree
        PATHS "/usr/local/lib"
        NO_DEFAULT_PATH
    )
if(EMBREE_LIBRARY)
  message(STATUS "Found Embree library: ${EMBREE_LIBRARY}")
  link_libraries(${EMBREE_LIBRARY})
endif()

include_directories(${VTK_INCLUDE_DIRS})

find_package(OpenMP)
if(OpenMP_CXX_FOUND)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BUILD_SHARED_LIBS "Build shared libraries" ON)
enable_testing()

include(GNUInstallDirs)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_BINDIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR}")

set("_paraview_plugin_default_${PROJECT_NAME}" ON)
paraview_plugin_scan(
  PLUGIN_FILES      "${CMAKE_CURRENT_SOURCE_DIR}/Plugin/paraview.plugin"
  PROVIDES_PLUGINS  plugins
  REQUIRES_MODULES  required_modules)

foreach (module IN LISTS required_modules)
  if (NOT TARGET "${module}")
    message(#FATAL_ERROR
      "Skipping example ${PROJECT_NAME}: Missing required module: "
      "${module}")
    return ()
  endif ()
endforeach ()

paraview_plugin_build(
  RUNTIME_DESTINATION "${CMAKE_INSTALL_BINDIR}"
  LIBRARY_DESTINATION "${CMAKE_INSTALL_LIBDIR}"
  LIBRARY_SUBDIRECTORY "${PARAVIEW_PLUGIN_SUBDIR}"
  PLUGINS ${plugins})
