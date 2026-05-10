#ifndef vtkViennaPSModelRegistry_h
#define vtkViennaPSModelRegistry_h

#include "vtkViennaPSMetadata.h"
#include <vtkPolyData.h>
#include <vtkDataObject.h>
#include <memory>
#include <functional>
#include <map>
#include <vector>

#include <psDomain.hpp>

class vtkViennaPSModelRegistry {
public:
    using NumericType = ViennaPSMeta::NumericType;
    static constexpr int D = ViennaPSMeta::D;

    // Factory function type — receives pre-created domain
    using ModelFactory = std::function<void(
        std::shared_ptr<void> psDomain,
        int dimension,
        vtkDataObject* output,
        const ViennaPSMeta::ParameterMap& parameters
    )>;

    static vtkViennaPSModelRegistry& getInstance();

    void registerGeometryModel(
        const std::string& name,
        const ViennaPSMeta::ModelMetadata& metadata,
        ModelFactory factory
    );
    
    void registerProcessModel(
        const std::string& name,
        const ViennaPSMeta::ModelMetadata& metadata,
        ModelFactory factory
    );

    std::vector<std::string> getGeometryModelNames() const;
    std::vector<std::string> getProcessModelNames() const;

    ViennaPSMeta::ModelMetadata getModelMetadata(const std::string& name) const;
    bool hasModel(const std::string& name) const;

    ModelFactory getModelFactory(const std::string& name) const;

    void executeModel(
        const std::string& modelName,
        std::shared_ptr<void> psDomain,
        int dimension,
        vtkDataObject* output,
        const ViennaPSMeta::ParameterMap& parameters
    );
    
    std::string getGeometryModelByIndex(int index) const;
    std::string getProcessModelByIndex(int index) const;

    void clear();

    void initializeBuiltinModels(bool geometry);

private:
    vtkViennaPSModelRegistry();
    ~vtkViennaPSModelRegistry() = default;

    vtkViennaPSModelRegistry(const vtkViennaPSModelRegistry&) = delete;
    vtkViennaPSModelRegistry& operator=(const vtkViennaPSModelRegistry&) = delete;

    struct ModelEntry {
        ViennaPSMeta::ModelMetadata metadata;
        ModelFactory factory;
    };

    std::map<std::string, ModelEntry> geometryModels_;
    std::map<std::string, ModelEntry> processModels_;

    std::vector<std::string> geometryModelOrder_;
    std::vector<std::string> processModelOrder_;

public:
    template<typename T>
    static T getParameter(const ViennaPSMeta::ParameterMap& params,
                         const std::string& name,
                         T defaultValue);
};

#ifndef __WRAP__
template<typename T>
T vtkViennaPSModelRegistry::getParameter(
    const ViennaPSMeta::ParameterMap& params,
    const std::string& name,
    T defaultValue) 
{
    auto it = params.find(name);
    if (it != params.end()) {
        try {
            return std::get<T>(it->second);
        } catch (const std::bad_variant_access&) {
            // Type mismatch - return default
        }
    }
    return defaultValue;
}
#endif

#endif