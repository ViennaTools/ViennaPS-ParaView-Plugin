#ifndef vtkViennaPSDomainObject_h
#define vtkViennaPSDomainObject_h

#include "ViennaPSPluginModule.h"
#include <vtkUnstructuredGrid.h>
#include <vtkSmartPointer.h>
#include <memory>

#include <psDomain.hpp>

class vtkDataSet;

/**
 * vtkUnstructuredGrid subclass that also carries a ViennaPS Domain so that
 * filters can pass the domain along the pipeline without converting to and
 * from a mesh on every step. The inherited mesh data is the visualization,
 * refreshed via UpdateVisualization(); the domain itself is held through a
 * small type-erasure layer so the dimension (2D/3D) does not leak into the
 * VTK class type.
 */
class VIENNAPSPLUGIN_EXPORT vtkViennaPSDomainObject : public vtkUnstructuredGrid
{
public:
    static vtkViennaPSDomainObject* New();
    vtkTypeMacro(vtkViennaPSDomainObject, vtkUnstructuredGrid);
    void PrintSelf(ostream& os, vtkIndent indent) override;

    void ShallowCopy(vtkDataObject* src) override;
    void DeepCopy(vtkDataObject* src) override;
    void Initialize() override;

#ifndef __WRAP__
    template<typename T, int D>
    void SetDomain(std::shared_ptr<viennaps::Domain<T, D>> domain);
#endif

#ifndef __WRAP__
    template<typename T, int D>
    std::shared_ptr<viennaps::Domain<T, D>> GetDomain();
#endif

    bool HasDomain() const;

    int GetDimension() const;

    int GetOutputFormat() const { return this->OutputFormat; }

    /**
     * Set output format and refresh the cached mesh.
     * @param format 0=Volume, 1=Surface, 2=Hull
     */
    void SetOutputFormat(int format);

    /**
     * Rebuild the inherited vtkUnstructuredGrid (points, cells, arrays) from
     * the stored ViennaPS domain.
     */
    void UpdateVisualization();

protected:
    vtkViennaPSDomainObject();
    ~vtkViennaPSDomainObject() override;

private:
    vtkViennaPSDomainObject(const vtkViennaPSDomainObject&) = delete;
    void operator=(const vtkViennaPSDomainObject&) = delete;

    struct DomainHolder {
        virtual ~DomainHolder() = default;
        virtual std::unique_ptr<DomainHolder> Clone() const = 0;
        virtual std::unique_ptr<DomainHolder> DeepClone() const = 0;
        virtual int GetDimension() const = 0;
        virtual size_t GetMemorySize() const = 0;
        virtual void UpdateMesh(vtkUnstructuredGrid* output, int format) = 0;
    };

    template<typename T, int D> struct DomainHolderImpl;

    std::unique_ptr<DomainHolder> Holder;
    int OutputFormat = 1;
};

#ifndef __WRAP__
#include "vtkViennaPSDomainObject.txx"
#endif

#endif
