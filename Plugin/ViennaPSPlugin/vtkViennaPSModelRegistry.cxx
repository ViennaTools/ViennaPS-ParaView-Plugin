#include "vtkViennaPSModelRegistry.h"
#include "vtkViennaPSLogger.h"
#include <stdexcept>

vtkViennaPSModelRegistry& vtkViennaPSModelRegistry::getInstance() {
    static vtkViennaPSModelRegistry instance;
    return instance;
}

vtkViennaPSModelRegistry::vtkViennaPSModelRegistry() {
    // loaded lazily via initializeBuiltinModels
}

void vtkViennaPSModelRegistry::registerGeometryModel(
    const std::string& name,
    const ViennaPSMeta::ModelMetadata& metadata,
    ModelFactory factory) 
{
    if (geometryModels_.find(name) != geometryModels_.end()) {
        VPSLOG_DEBUG(nullptr, "Warning: Geometry model '", name, "' already registered. Overwriting.");

        auto it = std::find(geometryModelOrder_.begin(),
                           geometryModelOrder_.end(), name);
        if (it != geometryModelOrder_.end()) {
            geometryModelOrder_.erase(it);
        }
    }

    ModelEntry entry;
    entry.metadata = metadata;
    entry.factory = factory;

    geometryModels_[name] = entry;
    geometryModelOrder_.push_back(name);
    
    VPSLOG_DEBUG(nullptr, "Registered geometry model: ", name, " (", metadata.displayName, ")");
}

void vtkViennaPSModelRegistry::registerProcessModel(
    const std::string& name,
    const ViennaPSMeta::ModelMetadata& metadata,
    ModelFactory factory) 
{
    if (processModels_.find(name) != processModels_.end()) {
        VPSLOG_DEBUG(nullptr, "Warning: Process model '", name, "' already registered. Overwriting.");
        
        auto it = std::find(processModelOrder_.begin(), 
                           processModelOrder_.end(), name);
        if (it != processModelOrder_.end()) {
            processModelOrder_.erase(it);
        }
    }

    ModelEntry entry;
    entry.metadata = metadata;
    entry.factory = factory;

    processModels_[name] = entry;
    processModelOrder_.push_back(name);

    VPSLOG_DEBUG(nullptr, "Registered process model: ", name, " (", metadata.displayName, ")");
}

std::vector<std::string> vtkViennaPSModelRegistry::getGeometryModelNames() const {
    return geometryModelOrder_;
}

std::vector<std::string> vtkViennaPSModelRegistry::getProcessModelNames() const {
    return processModelOrder_;
}

ViennaPSMeta::ModelMetadata vtkViennaPSModelRegistry::getModelMetadata(
    const std::string& name) const 
{
    auto geoIt = geometryModels_.find(name);
    if (geoIt != geometryModels_.end()) {
        return geoIt->second.metadata;
    }
    
    auto procIt = processModels_.find(name);
    if (procIt != processModels_.end()) {
        return procIt->second.metadata;
    }
    
    throw std::runtime_error("Model not found: " + name);
}

bool vtkViennaPSModelRegistry::hasModel(const std::string& name) const {
    return (geometryModels_.find(name) != geometryModels_.end()) ||
           (processModels_.find(name) != processModels_.end());
}

vtkViennaPSModelRegistry::ModelFactory 
vtkViennaPSModelRegistry::getModelFactory(const std::string& name) const {
    auto geoIt = geometryModels_.find(name);
    if (geoIt != geometryModels_.end()) {
        return geoIt->second.factory;
    }
    
    auto procIt = processModels_.find(name);
    if (procIt != processModels_.end()) {
        return procIt->second.factory;
    }
    
    throw std::runtime_error("Model factory not found: " + name);
}

void vtkViennaPSModelRegistry::executeModel(
    const std::string& modelName,
    std::shared_ptr<void> psDomain,
    int dimension,
    vtkDataObject* output,
    const ViennaPSMeta::ParameterMap& parameters)
{
    if (!hasModel(modelName)) {
        throw std::runtime_error("Model not found: " + modelName);
    }

    auto factory = getModelFactory(modelName);

    VPSLOG_DEBUG(nullptr, "Executing model: ", modelName,
                 " (dimension=", dimension, "D, parameters=", parameters.size(),
                 ", output=", (output ? output->GetClassName() : "null"), ")");
    VPSLOG_DEBUG(nullptr, "vtkViennaPSModelRegistry::executeModel: ", ViennaPSMeta::ParameterMapToString(parameters));

    try {
        factory(psDomain, dimension, output, parameters);
        VPSLOG_DEBUG(nullptr, "Model execution successful: ", modelName);
    } catch (const std::exception& e) {
        VPSLOG_ERROR(nullptr, "Model execution failed: ", e.what());
        throw;
    }
}

std::string vtkViennaPSModelRegistry::getGeometryModelByIndex(int index) const {
    if (index >= 0 && index < static_cast<int>(geometryModelOrder_.size())) {
        return geometryModelOrder_[index];
    }
    return "";
}

std::string vtkViennaPSModelRegistry::getProcessModelByIndex(int index) const {
    if (index >= 0 && index < static_cast<int>(processModelOrder_.size())) {
        return processModelOrder_[index];
    }
    return "";
}

void vtkViennaPSModelRegistry::clear() {
    geometryModels_.clear();
    processModels_.clear();
    geometryModelOrder_.clear();
    processModelOrder_.clear();
    VPSLOG_DEBUG(nullptr, "vtkViennaPSModelRegistry::clear");
}

