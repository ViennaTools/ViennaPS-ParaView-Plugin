#include "vtkViennaPSDomainObject.h"
#include "vtkViennaPSUtils.h"
#include "vtkViennaPSLogger.h"

#include <vtkObjectFactory.h>
#include <vtkInformation.h>
#include <vtkPolyData.h>

// ViennaPS includes
#include <psDomain.hpp>
#include <lsDomain.hpp>

vtkStandardNewMacro(vtkViennaPSDomainObject);

//----------------------------------------------------------------------------
vtkViennaPSDomainObject::vtkViennaPSDomainObject()
    : Holder(nullptr)
{
}

vtkViennaPSDomainObject::~vtkViennaPSDomainObject() = default;

//----------------------------------------------------------------------------
void vtkViennaPSDomainObject::ShallowCopy(vtkDataObject* src)
{
    this->Superclass::ShallowCopy(src);

    // Only modify Holder when copying FROM another DomainObject.
    // When src is a plain vtkUnstructuredGrid (e.g. UpdateMesh copying
    // converted mesh data), preserve our existing Holder.
    auto* srcDomain = vtkViennaPSDomainObject::SafeDownCast(src);
    if (srcDomain) {
        if (srcDomain->Holder) {
            this->Holder = srcDomain->Holder->Clone();
            VPSLOG_DEBUG(nullptr, "ShallowCopy: Domain holder cloned (dimension ",
                         this->GetDimension(), ")");
        } else {
            this->Holder.reset();
        }
        this->OutputFormat = srcDomain->OutputFormat;
    }

    this->Modified();
}

//----------------------------------------------------------------------------
void vtkViennaPSDomainObject::DeepCopy(vtkDataObject* src)
{
    this->Superclass::DeepCopy(src);

    // Only modify Holder when copying FROM another DomainObject.
    auto* srcDomain = vtkViennaPSDomainObject::SafeDownCast(src);
    if (srcDomain) {
        if (srcDomain->Holder) {
            this->Holder = srcDomain->Holder->DeepClone();
            VPSLOG_DEBUG(nullptr, "DeepCopy: Domain deep cloned (dimension ",
                         this->GetDimension(), ")");
        } else {
            this->Holder.reset();
        }
        this->OutputFormat = srcDomain->OutputFormat;
    }

    this->Modified();
}

//----------------------------------------------------------------------------
void vtkViennaPSDomainObject::Initialize()
{
    VPSLOG_DEBUG(nullptr, "Initialize: Clearing mesh (preserving domain)");
    this->Superclass::Initialize();
    // Domain holder is preserved — VTK pipeline calls Initialize() to prepare
    // output for new data, but domain is our primary payload and should only
    // be replaced via SetDomain(). Mesh data (Superclass) is cleared here and
    // regenerated via SetDomain → UpdateVisualization in RequestData.
}

//----------------------------------------------------------------------------
void vtkViennaPSDomainObject::PrintSelf(ostream& os, vtkIndent indent)
{
    this->Superclass::PrintSelf(os, indent);

    os << indent << "HasDomain: " << (this->Holder ? "Yes" : "No") << "\n";
    os << indent << "OutputFormat: " << this->OutputFormat << "\n";

    if (this->Holder) {
        os << indent << "Dimension: " << this->Holder->GetDimension() << "\n";
        os << indent << "DomainMemory: " << this->Holder->GetMemorySize() << " KB\n";
    }
}

//----------------------------------------------------------------------------
bool vtkViennaPSDomainObject::HasDomain() const
{
    return this->Holder != nullptr;
}

//----------------------------------------------------------------------------
int vtkViennaPSDomainObject::GetDimension() const
{
    return this->Holder ? this->Holder->GetDimension() : -1;
}

std::vector<std::string> vtkViennaPSDomainObject::GetMaterialNamesInDomain() const
{
    if (!this->Holder) {
        return {};
    }
    return this->Holder->GetMaterialNamesInDomain();
}

//----------------------------------------------------------------------------
void vtkViennaPSDomainObject::SetOutputFormat(int format)
{
    if (this->OutputFormat != format) {
        this->OutputFormat = format;
        VPSLOG_DEBUG(nullptr, "OutputFormat changed to ", format);

        this->UpdateVisualization();
    }
}

//----------------------------------------------------------------------------
void vtkViennaPSDomainObject::UpdateVisualization()
{
    if (!this->Holder) {
        VPSLOG_DEBUG(nullptr, "UpdateVisualization: No domain to visualize");
        return;
    }

    VPSLOG_DEBUG(nullptr, "UpdateVisualization: format=", this->OutputFormat);

    this->Reset();

    this->Holder->UpdateMesh(this, this->OutputFormat);

    VPSLOG_DEBUG(nullptr, "Domain converted to VTK.");
    VPSLOG_INFO(nullptr, "Mesh updated: ", this->GetNumberOfPoints(), " points, ",
                this->GetNumberOfCells(), " cells");

    this->Modified();
}
