#ifndef vtkViennaPSParameterInterface_h
#define vtkViennaPSParameterInterface_h

#include <vtkAlgorithm.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include "ViennaPSPluginModule.h"
#include "vtkViennaPSMetadata.h"
#include "vtkViennaPSPropertyManager.h"
#include "vtkViennaPSModelRegistry.h"
#include <vtkSmartPointer.h>
#include <memory>

class VIENNAPSPLUGIN_EXPORT vtkViennaPSParameterInterface : public vtkAlgorithm
{
public:
  vtkTypeMacro(vtkViennaPSParameterInterface, vtkAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  vtkSetMacro(Dummy, bool);
  vtkGetMacro(Dummy, bool);

  vtkSetMacro(ModelType, int);
  vtkGetMacro(ModelType, int);

  vtkGetObjectMacro(PropertyManager, vtkViennaPSPropertyManager);

  virtual void SetParameterDouble(const char* name, double value);
  virtual void SetParameterInt(const char* name, int value);
  virtual void SetParameterBool(const char* name, bool value);
  virtual void SetParameterMaterialList(const char* name, const std::vector<int>& values);

  virtual double GetParameterDouble(const char* name);
  virtual int GetParameterInt(const char* name);
  virtual bool GetParameterBool(const char* name);
  virtual std::vector<int> GetParameterMaterialList(const char* name);

  virtual void InitializeDynamicProperties() = 0;

  virtual bool IsGeometrySource() const { return false; }
  virtual bool IsProcessFilter() const { return false; }

  const std::string& GetCurrentModelName() const { return currentModelName_; }

protected:
  vtkViennaPSParameterInterface();
  ~vtkViennaPSParameterInterface() override;

  int ProcessRequest(vtkInformation* request,
                    vtkInformationVector** inputVector,
                    vtkInformationVector* outputVector) override;

  int FillOutputPortInformation(int port, vtkInformation* info) override;
  int RequestDataObject(vtkInformation* request,
                        vtkInformationVector** inputVector,
                        vtkInformationVector* outputVector);
  int RequestInformation(vtkInformation* request,
                        vtkInformationVector** inputVector,
                        vtkInformationVector* outputVector);
  virtual int RequestData(vtkInformation* request,
                 vtkInformationVector** inputVector,
                 vtkInformationVector* outputVector) = 0;

  const char* GetOutputDataType() const;

  vtkSmartPointer<vtkViennaPSPropertyManager> PropertyManager;
  std::string currentModelName_;

  bool Dummy = false;
  int ModelType = 0;
  int OutputFormat = 1;  // Default to Surface (matches DomainObject default)

private:
  vtkViennaPSParameterInterface(const vtkViennaPSParameterInterface&) = delete;
  void operator=(const vtkViennaPSParameterInterface&) = delete;
};

#endif