#ifndef vtkViennaPSProcessFilter_h
#define vtkViennaPSProcessFilter_h

#include "vtkViennaPSParameterInterface.h"
#include <vtkSmartPointer.h>
#include <memory>

#include <psDomain.hpp>

class vtkViennaPSDomainObject;

class VIENNAPSPLUGIN_EXPORT vtkViennaPSProcessFilter : public vtkViennaPSParameterInterface
{
public:
  static vtkViennaPSProcessFilter* New();
  vtkTypeMacro(vtkViennaPSProcessFilter, vtkViennaPSParameterInterface);
  
  vtkSetMacro(ProcessTime, double);
  vtkGetMacro(ProcessTime, double);

  vtkSetMacro(GridDelta, double);
  vtkGetMacro(GridDelta, double);
  vtkSetMacro(XExtent, double);
  vtkGetMacro(XExtent, double);
  vtkSetMacro(YExtent, double);
  vtkGetMacro(YExtent, double);
  vtkSetMacro(TargetDimension, int);
  vtkGetMacro(TargetDimension, int);

  void InitializeDynamicProperties() override;
  bool IsProcessFilter() const override { return true; }

protected:
  vtkViennaPSProcessFilter();
  ~vtkViennaPSProcessFilter() override;

  int RequestData(vtkInformation*, vtkInformationVector**,
                  vtkInformationVector*) override;

  int ProcessRequest(vtkInformation*, vtkInformationVector**,
                     vtkInformationVector*) override;

  int FillInputPortInformation(int port, vtkInformation* info) override;
  int FillOutputPortInformation(int port, vtkInformation* info) override;

private:
  vtkViennaPSProcessFilter(const vtkViennaPSProcessFilter&) = delete;
  void operator=(const vtkViennaPSProcessFilter&) = delete;

  using NumericType = double;
  static constexpr int D = 3;
  std::shared_ptr<viennaps::Domain<NumericType, D>> psDomain;

  // Execute model with domain input
  void ExecuteModelWithDomain(vtkViennaPSDomainObject* domainInput,
                               vtkViennaPSDomainObject* domainOutput);

  // Execute model with mesh input
  void ExecuteModelWithMesh(vtkDataObject* meshInput,
                            vtkViennaPSDomainObject* domainOutput);

  double ProcessTime = 5.0;
  double GridDelta = 0.25;
  double XExtent = 10.0;
  double YExtent = 10.0;
  int TargetDimension = 3;
};

#endif
