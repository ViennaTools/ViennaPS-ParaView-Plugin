set(classes
    vtkViennaPSGeometrySource
    vtkViennaPSProcessFilter
    vtkViennaPSDomainObject
    vtkViennaPSUtils
    vtkViennaPSModelRegistry
    vtkViennaPSPropertyManager
    vtkViennaPSParameterInterface
    vtkViennaPSLogger)

set(sources vtkViennaPSBuiltinModels.cpp vtkViennaPSGeometryModels.cpp vtkViennaPSEtchingModels.cpp
            vtkViennaPSDepositionModels.cpp vtkViennaPSDistributionModels.cpp)

vtk_module_add_module(ViennaPSPlugin FORCE_STATIC CLASSES ${classes} SOURCES ${sources})

target_link_libraries(
  ViennaPSPlugin
  PRIVATE Qt::Core Qt::Widgets ParaView::pqApplicationComponents ParaView::RemotingViews
          ParaView::RemotingServerManager
  PUBLIC ViennaPS)

target_sources(ViennaPSPlugin PRIVATE ${widget_interface_sources})
