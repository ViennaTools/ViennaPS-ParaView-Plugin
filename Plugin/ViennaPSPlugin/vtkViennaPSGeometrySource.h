#ifndef vtkViennaPSGeometrySource_h
#define vtkViennaPSGeometrySource_h

#include "vtkViennaPSParameterInterface.h"
#include <vtkSmartPointer.h>
#include <memory>
#include <map>

#include <psDomain.hpp>

class vtkViennaPSDomainObject;

class VIENNAPSPLUGIN_EXPORT vtkViennaPSGeometrySource : public vtkViennaPSParameterInterface
{
public:
  static vtkViennaPSGeometrySource* New();
  vtkTypeMacro(vtkViennaPSGeometrySource, vtkViennaPSParameterInterface);

  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);

  vtkSetMacro(GridDelta, double);
  vtkGetMacro(GridDelta, double);

  vtkSetMacro(XExtent, double);
  vtkGetMacro(XExtent, double);

  vtkSetMacro(YExtent, double);
  vtkGetMacro(YExtent, double);

  vtkSetMacro(TargetDimension, int);
  vtkGetMacro(TargetDimension, int);

  vtkSetMacro(ImportMaterial, int);
  vtkGetMacro(ImportMaterial, int);

  void InitializeDynamicProperties() override;
  bool IsGeometrySource() const override { return true; }

protected:
  vtkViennaPSGeometrySource();
  ~vtkViennaPSGeometrySource() override;

  int RequestData(vtkInformation*, vtkInformationVector**,
                  vtkInformationVector*) override;

  int ProcessRequest(vtkInformation*, vtkInformationVector**,
                     vtkInformationVector*) override;

  int FillInputPortInformation(int port, vtkInformation* info) override;
  int FillOutputPortInformation(int port, vtkInformation* info) override;

private:
  vtkViennaPSGeometrySource(const vtkViennaPSGeometrySource&) = delete;
  void operator=(const vtkViennaPSGeometrySource&) = delete;

  using NumericType = ViennaPSMeta::NumericType;
  static constexpr int D = ViennaPSMeta::D;
  std::shared_ptr<viennaps::Domain<NumericType, D>> psDomain;
  std::shared_ptr<viennaps::Domain<NumericType, 2>> psDomain2D;

  void ExecuteModel(vtkViennaPSDomainObject* output);
  void LoadFromFile(vtkViennaPSDomainObject* output);

  char* FileName;

  double GridDelta = 0.25;
  double XExtent = 10.0;
  double YExtent = 10.0;
  int TargetDimension = 3;
  int ImportMaterial = 1;
};

#endif