set(interfaces)
set(qt_headers
  ViennaPSPlugin/pqViennaPSWidget.h)

qt6_wrap_cpp(moc_sources ${qt_headers})

set(widget_sources
  ViennaPSPlugin/pqViennaPSWidget.cxx
  ${moc_sources})

paraview_plugin_add_property_widget(
  KIND WIDGET
  TYPE pqViennaPSWidget
  CLASS_NAME pqViennaPSWidget
  INTERFACES widget_interface
  SOURCES widget_interface_sources)

list(APPEND interfaces ${widget_interface})
list(APPEND sources ${widget_interface_sources} ${widget_sources})

list(APPEND sources ViennaPSPluginInitializer.cxx)

paraview_add_plugin(ViennaPSPluginModule
  REQUIRED_ON_CLIENT
  REQUIRED_ON_SERVER
  VERSION "1.0"
  UI_INTERFACES ${interfaces}
  MODULES ViennaPSPlugin
  MODULE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/ViennaPSPlugin/vtk.module"
  SERVER_MANAGER_XML ViennaPSPlugin.xml
  SOURCES ${sources})

option(BUILD_TESTING "Build Plugin Testing" OFF)
if (BUILD_TESTING AND BUILD_SHARED_LIBS)
  add_subdirectory(Testing)
endif()
