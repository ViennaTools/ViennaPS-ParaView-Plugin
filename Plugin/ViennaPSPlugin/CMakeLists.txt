find_package(Qt6 REQUIRED COMPONENTS Core Widgets)

set(classes
  vtkViennaPSGeometrySource
  vtkViennaPSProcessFilter
  vtkViennaPSDomainObject
  vtkViennaPSUtils
  vtkViennaPSModelRegistry
  vtkViennaPSPropertyManager
  vtkViennaPSParameterInterface
  vtkViennaPSLogger)

set(sources
  vtkViennaPSBuiltinModels.cpp
  vtkViennaPSGeometryModels.cpp
  vtkViennaPSEtchingModels.cpp
  vtkViennaPSDepositionModels.cpp
  vtkViennaPSDistributionModels.cpp)

vtk_module_add_module(ViennaPSPlugin
  FORCE_STATIC
  CLASSES ${classes}
  SOURCES ${sources})

target_link_libraries(ViennaPSPlugin
  PRIVATE
    Qt6::Core
    Qt6::Widgets
    ParaView::pqApplicationComponents
    ParaView::RemotingViews
    ParaView::RemotingServerManager)

target_sources(ViennaPSPlugin PRIVATE ${widget_interface_sources})
