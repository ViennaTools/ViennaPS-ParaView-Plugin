#include "vtkViennaPSGeometrySource.h"
#include "vtkViennaPSUtils.h"
#include "vtkViennaPSLogger.h"
#include "vtkViennaPSDomainObject.h"

#include <vtkObjectFactory.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkPolyData.h>
#include <vtkUnstructuredGrid.h>
#include <vtkDataObject.h>
#include <vtkDemandDrivenPipeline.h>
#include <vtkStreamingDemandDrivenPipeline.h>

#include <iostream>
#include <fstream>

#include <psDomain.hpp>
#include <psUtil.hpp>
#include <materials/psMaterialMap.hpp>

#include <lsDomain.hpp>
#include <lsMakeGeometry.hpp>
#include <lsGeometries.hpp>
#include <lsVTKReader.hpp>
#include <lsFromSurfaceMesh.hpp>
#include <lsMesh.hpp>

#include <vcLogger.hpp>
#include <vcSmartPointer.hpp>

vtkStandardNewMacro(vtkViennaPSGeometrySource);

//----------------------------------------------------------------------------
vtkViennaPSGeometrySource::vtkViennaPSGeometrySource()
{
  this->FileName = nullptr;
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);

  this->PropertyManager->Initialize(true);

  auto& registry = vtkViennaPSModelRegistry::getInstance();
  if (registry.getGeometryModelNames().empty()) {
    registry.initializeBuiltinModels(true);
  }

  InitializeDynamicProperties();
}

vtkViennaPSGeometrySource::~vtkViennaPSGeometrySource()
{
  delete[] this->FileName;
}

//----------------------------------------------------------------------------
void vtkViennaPSGeometrySource::InitializeDynamicProperties()
{
  auto& registry = vtkViennaPSModelRegistry::getInstance();
  std::string modelName = registry.getGeometryModelByIndex(this->ModelType);
  
  if (!modelName.empty() && modelName != currentModelName_) {
    currentModelName_ = modelName;
    PropertyManager->OnModelChanged(modelName);
  }
}

int vtkViennaPSGeometrySource::ProcessRequest(
    vtkInformation* request,
    vtkInformationVector** inputVector,
    vtkInformationVector* outputVector)
{
  if (request->Has(vtkDemandDrivenPipeline::REQUEST_DATA_OBJECT()))
  {
    VPSLOG_DEBUG(nullptr, "GeometrySource::ProcessRequest - REQUEST_DATA_OBJECT");

    vtkInformation* outInfo = outputVector->GetInformationObject(0);
    vtkDataObject* output = outInfo->Get(vtkDataObject::DATA_OBJECT());

    if (!output || !output->IsA("vtkViennaPSDomainObject"))
    {
      vtkViennaPSDomainObject* domainObj = vtkViennaPSDomainObject::New();
      outInfo->Set(vtkDataObject::DATA_OBJECT(), domainObj);
      domainObj->Delete();

      VPSLOG_INFO(nullptr, "Created vtkViennaPSDomainObject for output");

      this->GetOutputPortInformation(0)->Set(
        vtkDataObject::DATA_EXTENT_TYPE(), domainObj->GetExtentType());
    }

    return 1;
  }

  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

int vtkViennaPSGeometrySource::FillInputPortInformation(
    int port, vtkInformation* info)
{
  if (port == 0) {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataObject");
    info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
    return 1;
  }
  return 0;
}

int vtkViennaPSGeometrySource::FillOutputPortInformation(
    int port, vtkInformation* info)
{
  if (port == 0) {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkViennaPSDomainObject");
    return 1;
  }
  return 0;
}

int vtkViennaPSGeometrySource::RequestData(
  vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector),
  vtkInformationVector* outputVector)
{
  VPSLOG_DEBUG(nullptr, "GeometrySource::RequestData");

  // Get domain object output
  vtkViennaPSDomainObject* output =
      vtkViennaPSDomainObject::SafeDownCast(vtkDataObject::GetData(outputVector, 0));

  if (!output) {
    vtkErrorMacro("No domain object output");
    return 0;
  }

  try {
    // Update model if GeometryType changed
    auto& registry = vtkViennaPSModelRegistry::getInstance();
    std::string modelName = registry.getGeometryModelByIndex(this->ModelType);
    if (!modelName.empty() && modelName != currentModelName_) {
      currentModelName_ = modelName;
      PropertyManager->OnModelChanged(modelName);
    }

    if (this->FileName && strlen(this->FileName) > 0) {
      LoadFromFile(output);
    }
    else {
      ExecuteModel(output);
    }

    VPSLOG_INFO(nullptr, "  Type: ", this->ModelType);
    VPSLOG_INFO(nullptr, "  Grid: ", this->GridDelta);
    VPSLOG_INFO(nullptr, "  Extent: ", this->XExtent, " x ", this->YExtent);
    VPSLOG_INFO(nullptr, "  Target Dimension: ", this->TargetDimension, "D");
    if (output->HasDomain()) {
      VPSLOG_INFO(nullptr, "  Created Domain Dimension: ", output->GetDimension(), "D");
    }

  } catch(const std::exception& e) {
    vtkErrorMacro("Error creating geometry: " << e.what());
    return 0;
  }

  return 1;
}

template<typename NumericType, int Dim>
std::shared_ptr<viennaps::Domain<NumericType, Dim>> ConvertVTKToDomain(
    vtkDataObject* input,
    double gridDelta,
    double xExtent,
    double yExtent,
    int importMaterialIndex = 1)
{
    if (input && (gridDelta <= 0 || xExtent <= 0 || yExtent <= 0)) {
        double bounds[6];
        if (auto* polyInput = vtkPolyData::SafeDownCast(input)) {
            polyInput->GetBounds(bounds);
        } else if (auto* ugridInput = vtkUnstructuredGrid::SafeDownCast(input)) {
            ugridInput->GetBounds(bounds);
        }

        double meshXExtent = bounds[1] - bounds[0];
        double meshYExtent = bounds[3] - bounds[2];

        if (xExtent <= 0) xExtent = meshXExtent;
        if (yExtent <= 0) yExtent = meshYExtent;

        if (gridDelta <= 0) {
            gridDelta = std::min(xExtent, yExtent) * 0.01;
            if (gridDelta <= 0) gridDelta = 0.25;
        }

        VPSLOG_INFO(nullptr, "Extracted from mesh - Grid: ", gridDelta,
                   ", XExtent: ", xExtent, ", YExtent: ", yExtent);
    }

    auto psDomain = std::make_shared<viennaps::Domain<NumericType, Dim>>(
        gridDelta, xExtent, yExtent
    );

    auto mesh = viennacore::SmartPointer<viennals::Mesh<NumericType>>::New();

    vtkPolyData* polyInput = vtkPolyData::SafeDownCast(input);
    vtkUnstructuredGrid* ugridInput = vtkUnstructuredGrid::SafeDownCast(input);

    viennals::VTKReader<NumericType> reader(mesh);
    if (polyInput) {
        VPSLOG_DEBUG(nullptr, "Reading from vtkPolyData");
        reader.setPolyData(polyInput);
    } else if (ugridInput) {
        VPSLOG_DEBUG(nullptr, "Reading from vtkUnstructuredGrid");
        reader.setUnstructuredGrid(ugridInput);
    } else {
        VPSLOG_ERROR(nullptr, "Input is neither PolyData nor UnstructuredGrid!");
        return psDomain;
    }
    reader.apply();

    auto levelSet = viennacore::SmartPointer<viennals::Domain<NumericType, Dim>>::New(
        psDomain->getGrid()
    );

    viennals::FromSurfaceMesh<NumericType, Dim> mesher;
    mesher.setMesh(mesh);
    mesher.setLevelSet(levelSet);
    mesher.apply();

    // Convert material index to viennaps::Material
    // Material list order: Undefined, Mask, Si, SiO2, Si3N4, ...
    // Index 0 = Undefined (-1), Index 1 = Mask (0), Index 2 = Si (1), etc.
    auto materialNames = ViennaPSMeta::getAllMaterialNames();
    viennaps::Material material = viennaps::Material::Si;
    if (importMaterialIndex >= 0 && importMaterialIndex < static_cast<int>(materialNames.size())) {
        material = ViennaPSMeta::getMaterialFromString(materialNames[importMaterialIndex]);
        VPSLOG_INFO(nullptr, "Using material: ", materialNames[importMaterialIndex]);
    }

    psDomain->insertNextLevelSetAsMaterial(
        levelSet,
        material
    );

    VPSLOG_DEBUG(nullptr, "VTK converted to Domain.");
    return psDomain;
}

template<int Dim>
void AddInitialLevelSet(std::shared_ptr<viennaps::Domain<ViennaPSMeta::NumericType, Dim>> domain)
{
    using NumericType = ViennaPSMeta::NumericType;
    auto levelSet = viennacore::SmartPointer<viennals::Domain<NumericType, Dim>>::New(
        domain->getGrid());

    if constexpr (Dim == 2) {
        viennals::MakeGeometry<NumericType, 2>(
            levelSet,
            viennacore::SmartPointer<viennals::Plane<NumericType, 2>>::New(
                std::array<NumericType, 2>{0., 0.},
                std::array<NumericType, 2>{0., 1.}
            )
        ).apply();
    } else {
        viennals::MakeGeometry<NumericType, 3>(
            levelSet,
            viennacore::SmartPointer<viennals::Plane<NumericType, 3>>::New(
                std::array<NumericType, 3>{0., 0., 0.},
                std::array<NumericType, 3>{0., 0., 1.}
            )
        ).apply();
    }

    domain->insertNextLevelSetAsMaterial(levelSet, viennaps::Material::Si);
}

//----------------------------------------------------------------------------
void vtkViennaPSGeometrySource::ExecuteModel(vtkViennaPSDomainObject* output)
{
  if (currentModelName_.empty()) {
    VPSLOG_ERROR(this, "No model selected");
    return;
  }

  VPSLOG_INFO(nullptr, "ExecuteModel for ", currentModelName_);

  // Validate parameters
  if (!PropertyManager->ValidateParameters()) {
    VPSLOG_ERROR(nullptr, "Parameter validation failed:\n", PropertyManager->GetValidationErrors());
    return;
  }

  int targetDimension = (this->TargetDimension > 0) ? this->TargetDimension : 3;
  VPSLOG_INFO(nullptr, "Target dimension: ", targetDimension, "D");

  auto parameters = PropertyManager->GetAllParameters();
  parameters["targetDimension"] = targetDimension;

  VPSLOG_INFO(nullptr, ViennaPSMeta::ParameterMapToString(parameters));

  std::shared_ptr<void> psDomain;

  vtkDataObject* input = this->GetInputDataObject(0, 0);

  if (targetDimension == 2) {
    std::shared_ptr<viennaps::Domain<NumericType, 2>> domain2D;

    if (auto* domainInput = vtkViennaPSDomainObject::SafeDownCast(input)) {
      auto inputDomain = domainInput->GetDomain<NumericType, 2>();
      domain2D = std::make_shared<viennaps::Domain<NumericType, 2>>(*inputDomain);
      VPSLOG_INFO(nullptr, "Deep copied 2D domain from input");
    } else if (input) {
      domain2D = ConvertVTKToDomain<NumericType, 2>(input, this->GridDelta, this->XExtent, this->YExtent, this->ImportMaterial);
      VPSLOG_INFO(nullptr, "Converted mesh to 2D domain");
    } else {
      domain2D = std::make_shared<viennaps::Domain<NumericType, 2>>(
          this->GridDelta, this->XExtent, this->YExtent);
      AddInitialLevelSet<2>(domain2D);
      VPSLOG_INFO(nullptr, "Created new 2D domain");
    }

    psDomain = domain2D;
    this->psDomain2D = domain2D;

  } else {  // targetDimension == 3
    std::shared_ptr<viennaps::Domain<NumericType, D>> domain3D;

    if (auto* domainInput = vtkViennaPSDomainObject::SafeDownCast(input)) {
      auto inputDomain = domainInput->GetDomain<NumericType, D>();
      domain3D = std::make_shared<viennaps::Domain<NumericType, D>>(*inputDomain);
      VPSLOG_INFO(nullptr, "Deep copied 3D domain from input");
    } else if (input) {
      domain3D = ConvertVTKToDomain<NumericType, D>(input, this->GridDelta, this->XExtent, this->YExtent, this->ImportMaterial);
      VPSLOG_INFO(nullptr, "Converted mesh to 3D domain");
    } else {
      domain3D = std::make_shared<viennaps::Domain<NumericType, D>>(
          this->GridDelta, this->XExtent, this->YExtent);
      AddInitialLevelSet<D>(domain3D);
      VPSLOG_INFO(nullptr, "Created new 3D domain");
    }

    psDomain = domain3D;
    this->psDomain = domain3D;
  }

  auto& registry = vtkViennaPSModelRegistry::getInstance();
  registry.executeModel(
      currentModelName_,
      psDomain,
      targetDimension,
      nullptr,  // Output handled separately
      parameters
  );

  // Set output format before SetDomain (SetDomain triggers UpdateVisualization)
  output->SetOutputFormat(this->OutputFormat);

  if (targetDimension == 2) {
    auto domain2D = std::static_pointer_cast<viennaps::Domain<NumericType, 2>>(psDomain);
    output->SetDomain<NumericType, 2>(domain2D);
    this->psDomain2D = domain2D;
    VPSLOG_INFO(nullptr, "2D Domain set to output");
  } else {
    auto domain3D = std::static_pointer_cast<viennaps::Domain<NumericType, D>>(psDomain);
    output->SetDomain<NumericType, D>(domain3D);
    this->psDomain = domain3D;
    VPSLOG_INFO(nullptr, "3D Domain set to output");
  }
}

//----------------------------------------------------------------------------
void vtkViennaPSGeometrySource::LoadFromFile(vtkViennaPSDomainObject* output)
{
  if (!this->FileName || strlen(this->FileName) == 0) {
    vtkErrorMacro("No filename specified");
    return;
  }

  viennaps::util::Parameters params;
  params.readConfigFile(this->FileName);

  this->GridDelta = params.get("gridDelta");
  this->XExtent = params.get("xExtent");
  this->YExtent = params.get("yExtent");

  VPSLOG_DEBUG(nullptr, "Loaded parameters from file: ", this->FileName);

  ExecuteModel(output);
}
