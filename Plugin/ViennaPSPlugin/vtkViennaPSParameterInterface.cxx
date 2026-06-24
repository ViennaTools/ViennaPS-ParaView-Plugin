#include "vtkViennaPSParameterInterface.h"
#include "vtkViennaPSLogger.h"
#include <vtkObjectFactory.h>
#include <vtkPolyData.h>
#include <vtkDataObject.h>
#include <vtkUnstructuredGrid.h>
#include <vtkDemandDrivenPipeline.h> 
#include <vtkStreamingDemandDrivenPipeline.h>
#include <vtkInformation.h>
#include <vtkExecutive.h>
#include <vtkAlgorithm.h>
#include <vtkDataObjectTypes.h>
#include <vtkType.h>
#include <iostream>

using namespace ViennaPSLogger;

vtkViennaPSParameterInterface::vtkViennaPSParameterInterface()
{
  this->PropertyManager = vtkSmartPointer<vtkViennaPSPropertyManager>::New();
}

vtkViennaPSParameterInterface::~vtkViennaPSParameterInterface() = default;

void vtkViennaPSParameterInterface::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Dummy: " << this->Dummy << "\n";
  os << indent << "ModelType: " << this->ModelType << "\n";
  os << indent << "Current Model: " << this->currentModelName_ << "\n";
  os << indent << "Parameters: " << PropertyManager->GetAllParameters().size() << "\n";
}

//----------------------------------------------------------------------------
void vtkViennaPSParameterInterface::SetParameterDouble(const char* name, double value)
{
  if (!name) return;

  std::string paramName(name);
  PropertyManager->UpdateParameterValue(paramName, value);
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkViennaPSParameterInterface::SetParameterInt(const char* name, int value)
{
  if (!name) return;

  std::string paramName(name);
  if (paramName == "outputFormat" && this->OutputFormat != value) {
    this->OutputFormat = value;
    this->Modified();
    PropertyManager->UpdateParameterValue(paramName, value);
    return;
  }

  PropertyManager->UpdateParameterValue(paramName, value);
  this->Modified();
}

void vtkViennaPSParameterInterface::SetParameterBool(const char* name, bool value)
{
  if (!name) return;

  std::string paramName(name);
  PropertyManager->UpdateParameterValue(paramName, value);
  this->Modified();
}

void vtkViennaPSParameterInterface::SetParameterMaterialList(const char* name, const std::vector<int>& values)
{
  if (!name) return;

  std::string paramName(name);

  ViennaPSMeta::MaterialListValue matList;
  matList.materialIds = values;
  PropertyManager->UpdateParameterValue(paramName, matList);
  this->Modified();
}

void vtkViennaPSParameterInterface::SetParameterString(const char* name, const char* value)
{
  if (!name) return;

  std::string paramName(name);
  PropertyManager->UpdateParameterValue(paramName, std::string(value ? value : ""));
  this->Modified();
}

//----------------------------------------------------------------------------
double vtkViennaPSParameterInterface::GetParameterDouble(const char* name)
{
  if (!name) return 0.0;
  
  auto value = PropertyManager->GetParameterValue(std::string(name));
  if (std::holds_alternative<double>(value)) {
    return std::get<double>(value);
  }
  return 0.0;
}

int vtkViennaPSParameterInterface::GetParameterInt(const char* name)
{
  if (!name) return 0;
  
  auto value = PropertyManager->GetParameterValue(std::string(name));
  if (std::holds_alternative<int>(value)) {
    return std::get<int>(value);
  }
  return 0;
}

bool vtkViennaPSParameterInterface::GetParameterBool(const char* name)
{
  if (!name) return false;
  
  auto value = PropertyManager->GetParameterValue(std::string(name));
  if (std::holds_alternative<bool>(value)) {
    return std::get<bool>(value);
  }
  return false;
}

std::vector<int> vtkViennaPSParameterInterface::GetParameterMaterialList(const char* name)
{
  if (!name) return {};
  
  auto value = PropertyManager->GetParameterValue(std::string(name));
  if (std::holds_alternative<ViennaPSMeta::MaterialListValue>(value)) {
    return std::get<ViennaPSMeta::MaterialListValue>(value).materialIds;
  }
  return {};
}

std::string vtkViennaPSParameterInterface::GetParameterString(const char* name)
{
  if (!name) return {};

  auto value = PropertyManager->GetParameterValue(std::string(name));
  if (std::holds_alternative<std::string>(value)) {
    return std::get<std::string>(value);
  }
  return {};
}

int vtkViennaPSParameterInterface::ProcessRequest(
    vtkInformation* request,
    vtkInformationVector** inputVector,
    vtkInformationVector* outputVector)
{
  if (request->Has(vtkDemandDrivenPipeline::REQUEST_DATA_OBJECT()))
  {
    return this->RequestDataObject(request, inputVector, outputVector);
  }

  if (request->Has(vtkDemandDrivenPipeline::REQUEST_INFORMATION()))
  {
    return this->RequestInformation(request, inputVector, outputVector);
  }

  if (request->Has(vtkStreamingDemandDrivenPipeline::REQUEST_UPDATE_EXTENT()))
  {
    return 1;
  }

  if (request->Has(vtkDemandDrivenPipeline::REQUEST_DATA()))
  {
    return this->RequestData(request, inputVector, outputVector);
  }

  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

const char* vtkViennaPSParameterInterface::GetOutputDataType() const
{
  // Volume mode needs UnstructuredGrid, others use PolyData
  if (this->OutputFormat == static_cast<int>(ViennaPSMeta::OutputFormatType::VOLUME)) {
    return "vtkUnstructuredGrid";
  }
  return "vtkPolyData";
}

int vtkViennaPSParameterInterface::FillOutputPortInformation(
    int port, vtkInformation* info)
{
  if (port == 0) {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkDataObject");
    return 1;
  }
  return 0;
}

int vtkViennaPSParameterInterface::RequestDataObject(
    vtkInformation* request,
    vtkInformationVector** inputVector,
    vtkInformationVector* outputVector)
{
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkDataObject* currentOutput = outInfo->Get(vtkDataObject::DATA_OBJECT());
  
  const char* outType = GetOutputDataType();
  
  if (!currentOutput || !currentOutput->IsA(outType)) {
    vtkDataObject* newOutput = nullptr;
    if (strcmp(outType, "vtkUnstructuredGrid") == 0) {
      newOutput = vtkUnstructuredGrid::New();
    } else {
      newOutput = vtkPolyData::New();
    }
    
    outInfo->Set(vtkDataObject::DATA_OBJECT(), newOutput);
    newOutput->Delete();
    
    VPSLOG_DEBUG(nullptr, "NEW output created: ", newOutput->GetClassName());
  } else {
    VPSLOG_DEBUG(nullptr, "Output type OK, reusing");
  }
  
  return 1;
}

//----------------------------------------------------------------------------
int vtkViennaPSParameterInterface::RequestInformation(
    vtkInformation* request,
    vtkInformationVector** inputVector,
    vtkInformationVector* outputVector)
{
  return 1;
}
