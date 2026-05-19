#include "vtkViennaPSProcessFilter.h"
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

// ViennaPS includes
#include <psDomain.hpp>
#include <materials/psMaterialMap.hpp>
#include <models/psSingleParticleProcess.hpp>
#include <models/psSF6O2Etching.hpp>
#include <models/psFluorocarbonEtching.hpp>

// ViennaCore logger
#include <vcLogger.hpp>

vtkStandardNewMacro(vtkViennaPSProcessFilter);

vtkViennaPSProcessFilter::vtkViennaPSProcessFilter()
{
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);

  this->PropertyManager->Initialize(false);

  auto& registry = vtkViennaPSModelRegistry::getInstance();
  if (registry.getProcessModelNames().empty()) {
    registry.initializeBuiltinModels(false);
  }

  InitializeDynamicProperties();
}

void vtkViennaPSProcessFilter::InitializeDynamicProperties()
{
  auto& registry = vtkViennaPSModelRegistry::getInstance();
  std::string modelName = registry.getProcessModelByIndex(this->ModelType);
  
  if (!modelName.empty() && modelName != currentModelName_) {
    currentModelName_ = modelName;
    PropertyManager->OnModelChanged(modelName);
  }
}

vtkViennaPSProcessFilter::~vtkViennaPSProcessFilter()
{
}

int vtkViennaPSProcessFilter::FillInputPortInformation(int port, vtkInformation* info)
{
  if (port == 0) {
    // Accept vtkViennaPSDomainObject or VTK meshes
    info->Remove(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE());
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkViennaPSDomainObject");
    info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPolyData");
    info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkUnstructuredGrid");
    info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 0);
    return 1;
  }
  return 0;
}

int vtkViennaPSProcessFilter::FillOutputPortInformation(int port, vtkInformation* info)
{
  if (port == 0) {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkViennaPSDomainObject");
    return 1;
  }
  return 0;
}

int vtkViennaPSProcessFilter::ProcessRequest(
    vtkInformation* request,
    vtkInformationVector** inputVector,
    vtkInformationVector* outputVector)
{
  if (request->Has(vtkDemandDrivenPipeline::REQUEST_DATA_OBJECT()))
  {
    VPSLOG_DEBUG(nullptr, "ProcessFilter::ProcessRequest - REQUEST_DATA_OBJECT");

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

int vtkViennaPSProcessFilter::RequestData(
  vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  vtkDataObject* input = vtkDataObject::GetData(inputVector[0], 0);
  vtkViennaPSDomainObject* output =
      vtkViennaPSDomainObject::SafeDownCast(vtkDataObject::GetData(outputVector, 0));

  if (!input) {
    vtkErrorMacro("No input data");
    return 0;
  }

  if (!output) {
    vtkErrorMacro("No domain object output");
    return 0;
  }

  VPSLOG_DEBUG(nullptr, "Input type: ", input->GetClassName());

  try {
    auto& registry = vtkViennaPSModelRegistry::getInstance();
    std::string modelName = registry.getProcessModelByIndex(this->ModelType);
    if (!modelName.empty() && modelName != currentModelName_) {
      currentModelName_ = modelName;
      PropertyManager->OnModelChanged(modelName);
    }

    if (currentModelName_.empty()) {
      vtkErrorMacro("No model selected");
      return 0;
    }

    // Check if input is a domain object
    vtkViennaPSDomainObject* domainInput = vtkViennaPSDomainObject::SafeDownCast(input);

    if (domainInput && domainInput->HasDomain()) {
      ExecuteModelWithDomain(domainInput, output);
    } else {
      ExecuteModelWithMesh(input, output);
    }

  } catch(const std::exception& e) {
    vtkErrorMacro("Error in process filter: " << e.what());
    return 0;
  }

  return 1;
}

//----------------------------------------------------------------------------
void vtkViennaPSProcessFilter::ExecuteModelWithDomain(
    vtkViennaPSDomainObject* domainInput,
    vtkViennaPSDomainObject* domainOutput)
{
  if (currentModelName_.empty()) {
    VPSLOG_ERROR(this, "No model selected");
    return;
  }

  if (!PropertyManager->ValidateParameters()) {
    VPSLOG_ERROR(this, "Parameter validation failed: ", PropertyManager->GetValidationErrors());
    return;
  }

  this->ProcessTime = GetParameterDouble("ProcessTime");

  auto parameters = PropertyManager->GetAllParameters();
  VPSLOG_INFO(nullptr, "ExecuteModelWithDomain: ", ViennaPSMeta::ParameterMapToString(parameters));

  // Get domain from input
  int dim = domainInput->GetDimension();
  VPSLOG_INFO(nullptr, "Processing domain with dimension: ", dim);

  auto& registry = vtkViennaPSModelRegistry::getInstance();

  if (dim == 3) {
    // Deep copy to preserve upstream domain
    auto inputDomain = domainInput->GetDomain<NumericType, D>();
    auto psDomain = std::make_shared<viennaps::Domain<NumericType, D>>(*inputDomain);
    VPSLOG_INFO(nullptr, "Deep copied 3D domain from input");

    registry.executeModel(
        currentModelName_,
        psDomain,
        dim,
        domainOutput,
        parameters
    );
    // Set output format before SetDomain (SetDomain triggers UpdateVisualization)
    domainOutput->SetOutputFormat(this->OutputFormat);
    domainOutput->SetDomain<NumericType, D>(psDomain);
    this->psDomain = psDomain;
    VPSLOG_INFO(nullptr, "3D domain processed and set to output");
  } else if (dim == 2) {
    // Deep copy to preserve upstream domain
    auto inputDomain2D = domainInput->GetDomain<NumericType, 2>();
    auto psDomain2D = std::make_shared<viennaps::Domain<NumericType, 2>>(*inputDomain2D);
    VPSLOG_INFO(nullptr, "Deep copied 2D domain from input");

    registry.executeModel(
        currentModelName_,
        psDomain2D,
        dim,
        domainOutput,
        parameters
    );
    // Set output format before SetDomain (SetDomain triggers UpdateVisualization)
    domainOutput->SetOutputFormat(this->OutputFormat);
    domainOutput->SetDomain<NumericType, 2>(psDomain2D);
    VPSLOG_INFO(nullptr, "2D domain processed and set to output");
  }
}

//----------------------------------------------------------------------------
void vtkViennaPSProcessFilter::ExecuteModelWithMesh(
    vtkDataObject* meshInput,
    vtkViennaPSDomainObject* domainOutput)
{
  if (currentModelName_.empty()) {
    VPSLOG_ERROR(this, "No model selected");
    return;
  }

  if (!PropertyManager->ValidateParameters()) {
    VPSLOG_ERROR(this, "Parameter validation failed: ", PropertyManager->GetValidationErrors());
    return;
  }

  this->ProcessTime = GetParameterDouble("ProcessTime");

  auto parameters = PropertyManager->GetAllParameters();
  VPSLOG_INFO(nullptr, "ExecuteModelWithMesh: ", ViennaPSMeta::ParameterMapToString(parameters));

  this->UpdateProgress(0.0);

  double gridDelta = this->GridDelta;
  double xExtent = this->XExtent;
  double yExtent = this->YExtent;

  auto psDomain = vtkViennaPSUtils::ConvertVTKToDomain<NumericType, D>(
      meshInput, gridDelta, xExtent, yExtent);

  VPSLOG_INFO(nullptr, "Converted mesh to domain");

  auto& registry = vtkViennaPSModelRegistry::getInstance();
  registry.executeModel(
      currentModelName_,
      psDomain,
      D,  // dimension
      domainOutput,
      parameters
  );

  // Set output format before SetDomain (SetDomain triggers UpdateVisualization)
  domainOutput->SetOutputFormat(this->OutputFormat);
  domainOutput->SetDomain<NumericType, D>(psDomain);
  this->psDomain = psDomain;
  VPSLOG_INFO(nullptr, "Domain processed and set to output");

  this->UpdateProgress(1.0);
}
