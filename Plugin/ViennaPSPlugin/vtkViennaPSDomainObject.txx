#ifndef vtkViennaPSDomainObject_txx
#define vtkViennaPSDomainObject_txx

#include "vtkViennaPSDomainObject.h"
#include "vtkViennaPSLogger.h"
#include "vtkViennaPSUtils.h"

#include <vtkUnstructuredGrid.h>
#include <vtkPolyData.h>
#include <vtkPoints.h>
#include <vtkCellArray.h>

#include <psDomain.hpp>
#include <lsDomain.hpp>

#include <map>
#include <cmath>

template<typename T, int D>
struct vtkViennaPSDomainObject::DomainHolderImpl : vtkViennaPSDomainObject::DomainHolder {
    std::shared_ptr<viennaps::Domain<T, D>> domain;

    explicit DomainHolderImpl(std::shared_ptr<viennaps::Domain<T, D>> d)
        : domain(d)
    {
    }

    std::unique_ptr<DomainHolder> Clone() const override {
        return std::make_unique<DomainHolderImpl<T, D>>(domain);
    }

    std::unique_ptr<DomainHolder> DeepClone() const override {
        if (!domain) {
            return std::make_unique<DomainHolderImpl<T, D>>(nullptr);
        }

        auto grid = domain->getGrid();
        auto newDomain = std::make_shared<viennaps::Domain<T, D>>(
            grid.getGridDelta(),
            grid.getMaxGridPoint()[0] * grid.getGridDelta(),
            grid.getMaxGridPoint()[1] * grid.getGridDelta()
        );

        auto& levelSets = domain->getLevelSets();
        auto matMap = domain->getMaterialMap();
        for (std::size_t i = 0; i < levelSets.size(); i++) {
            auto newLS = viennacore::SmartPointer<viennals::Domain<T, D>>::New(
                newDomain->getGrid()
            );
            newLS->deepCopy(levelSets[i]);
            auto material = matMap ? matMap->getMaterialAtIdx(i)
                                   : viennaps::Material::Si;
            newDomain->insertNextLevelSetAsMaterial(newLS, material, false);
        }

        if (domain->getMaterialMap()) {
            newDomain->setMaterialMap(domain->getMaterialMap());
        }

        return std::make_unique<DomainHolderImpl<T, D>>(newDomain);
    }

    int GetDimension() const override {
        return D;
    }

    std::vector<std::string> GetMaterialNamesInDomain() const override {
        std::vector<std::string> names;
        if (!domain) {
            return names;
        }
        const auto& registry = viennaps::MaterialRegistry::instance();
        for (const auto& material : domain->getMaterialsInDomain()) {
            if (registry.hasMaterial(material)) {
                names.push_back(std::string(registry.getName(material)));
            }
        }
        return names;
    }

    size_t GetMemorySize() const override {
        if (!domain) return 0;

        size_t size = 0;

        size += sizeof(domain->getGrid());

        for (const auto& ls : domain->getLevelSets()) {
            if (ls) {
                size += ls->getNumberOfPoints() * sizeof(T);
            }
        }

        if (domain->getMaterialMap()) {
            size += sizeof(*domain->getMaterialMap());
        }

        return size / 1024;
    }

    void UpdateMesh(vtkUnstructuredGrid* output, int format) override {
        if (!domain) {
            VPSLOG_ERROR(nullptr, "Cannot visualize null domain");
            return;
        }

        // Always use UnstructuredGrid as intermediate to avoid PolyData->UGrid
        // conversion via vtkAppendFilter which can cause allocation issues
        auto tempData = vtkSmartPointer<vtkUnstructuredGrid>::New();

        try {
            vtkViennaPSUtils::ConvertDomainToVTK<T, D>(
                domain,
                tempData,
                static_cast<ViennaPSMeta::OutputFormatType>(format)
            );
        } catch (const std::exception& e) {
            VPSLOG_ERROR(nullptr, "Failed to convert domain to VTK: ", e.what());
            return;
        }

        output->ShallowCopy(tempData);
    }
};

template<typename T, int D>
void vtkViennaPSDomainObject::SetDomain(std::shared_ptr<viennaps::Domain<T, D>> domain)
{
    if (!domain) {
        vtkErrorMacro("Cannot set null domain");
        VPSLOG_ERROR(this, "Attempted to set null domain");
        return;
    }

    VPSLOG_INFO(nullptr, "Setting domain with dimension ", D);

    this->Holder = std::make_unique<DomainHolderImpl<T, D>>(domain);

    this->UpdateVisualization();

    this->Modified();
}

//----------------------------------------------------------------------------
template<typename T, int D>
std::shared_ptr<viennaps::Domain<T, D>> vtkViennaPSDomainObject::GetDomain()
{
    if (!this->Holder) {
        vtkErrorMacro("No domain stored");
        VPSLOG_ERROR(this, "Attempted to get domain from empty object");
        return nullptr;
    }

    int storedDim = this->Holder->GetDimension();
    if (storedDim != D) {
        vtkErrorMacro("Dimension mismatch: stored=" << storedDim
                      << ", requested=" << D);
        VPSLOG_ERROR(this, "Dimension mismatch: stored=", storedDim,
                     ", requested=", D);
        return nullptr;
    }

    auto* impl = dynamic_cast<DomainHolderImpl<T, D>*>(this->Holder.get());
    if (!impl) {
        vtkErrorMacro("Type mismatch in domain holder");
        VPSLOG_ERROR(this, "Type mismatch in domain holder");
        return nullptr;
    }

    return impl->domain;
}

#endif
