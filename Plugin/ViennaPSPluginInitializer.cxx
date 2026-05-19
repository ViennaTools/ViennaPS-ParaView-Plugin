#include "vtkViennaPSDomainObject.h"
#include "vtkViennaPSLogger.h"
#include <vtkObjectFactory.h>

VTK_ABI_NAMESPACE_BEGIN

class vtkViennaPSDomainObjectFactory : public vtkObjectFactory
{
public:
  static vtkViennaPSDomainObjectFactory* New();
  vtkTypeMacro(vtkViennaPSDomainObjectFactory, vtkObjectFactory);

  const char* GetDescription() override { return "ViennaPS Domain Object Factory"; }
  const char* GetVTKSourceVersion() override { return "VTK 9.0"; }

  vtkViennaPSDomainObjectFactory()
  {
    this->RegisterOverride("vtkDataObject",
                          "vtkViennaPSDomainObject",
                          "ViennaPS Domain Object",
                          1,
                          vtkObjectFactoryCreatevtkViennaPSDomainObject);
  }

private:
  static vtkObject* vtkObjectFactoryCreatevtkViennaPSDomainObject()
  {
    return vtkViennaPSDomainObject::New();
  }
};

vtkStandardNewMacro(vtkViennaPSDomainObjectFactory);

VTK_ABI_NAMESPACE_END

static vtkViennaPSDomainObjectFactory* viennaPSDomainObjectFactory = nullptr;

class ViennaPSPluginAutoInit {
public:
  ViennaPSPluginAutoInit() {
    if (!viennaPSDomainObjectFactory) {
      viennaPSDomainObjectFactory = vtkViennaPSDomainObjectFactory::New();
      vtkObjectFactory::RegisterFactory(viennaPSDomainObjectFactory);

      VPSLOG_INFO(nullptr, "Registered vtkViennaPSDomainObject factory with VTK");
    }
  }

  ~ViennaPSPluginAutoInit() {
    if (viennaPSDomainObjectFactory) {
      vtkObjectFactory::UnRegisterFactory(viennaPSDomainObjectFactory);
      viennaPSDomainObjectFactory->Delete();
      viennaPSDomainObjectFactory = nullptr;
    }
  }
};

static ViennaPSPluginAutoInit viennaPSAutoInit;
