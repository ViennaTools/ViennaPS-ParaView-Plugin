#ifndef vtkViennaPSMetadata_h
#define vtkViennaPSMetadata_h

#include <materials/psMaterialMap.hpp>

#include <string>
#include <vector>
#include <variant>
#include <map>
#include <iostream>
#include <sstream>

namespace ViennaPSMeta {

using NumericType = double;
static constexpr int D = 3;

enum class OutputFormatType {
    VOLUME = 0,
    SURFACE = 1,
    HULL = 2
};

enum class ParameterType {
    DOUBLE,
    INTEGER,
    BOOLEAN,
    ENUM,
    MATERIAL,
    MATERIAL_LIST,
    STRING
};

enum class ParameterCategory {
    BASIC,
    ADVANCED,
    MASK,
    EXPERT
};

inline std::vector<std::string> getAllMaterialNames() {
    std::vector<std::string> materials;
    
    for (int i = -1; i <= 19; ++i) {
        auto material = viennaps::MaterialMap::mapToMaterial(i);
        std::string name = viennaps::MaterialMap::toString(material);
        if (name != "Unknown" && name != "Undefined") {
            materials.push_back(name);
        }
    }
    
    return materials;
}

inline viennaps::Material getMaterialFromString(const std::string& matStr) {
    for (int i = -1; i <= 19; ++i) {
        auto material = viennaps::MaterialMap::mapToMaterial(i);
        if (viennaps::MaterialMap::toString(material) == matStr) {
            return material;
        }
    }
    return viennaps::Material::Si; // default
}

inline viennaps::Material resolveMaterialFromString(const std::string& name) {
    if (name.empty()) {
        return viennaps::Material::SiO2;
    }
    return viennaps::MaterialRegistry::instance().registerMaterial(name);
}

inline int getMaterialIndex(viennaps::Material material) {
    std::vector<std::string> materials = getAllMaterialNames();
    std::string materialName = viennaps::MaterialMap::toString(material);
    
    for (size_t i = 0; i < materials.size(); ++i) {
        if (materials[i] == materialName) {
            return static_cast<int>(i);
        }
    }
    return 1;
}

struct MaterialListValue {
    std::vector<int> materialIds;
    
    operator std::vector<int>() const { return materialIds; }
};

struct ParameterMetadata {
    std::string name;
    std::string displayName;
    std::string documentation;
    ParameterType type;

    std::variant<double, int, bool, std::string, MaterialListValue> defaultValue;
    std::variant<double, int, bool, std::string, MaterialListValue> minValue;
    std::variant<double, int, bool, std::string, MaterialListValue> maxValue;

    std::vector<std::string> enumOptions;

    std::map<int, std::string> materialMap;

    ParameterCategory category;

    std::string visibilityCondition;

    std::string unit;

    double stepSize = 0.1;

    bool required = true;
};

inline ParameterMetadata makeNumRaysPerPointParam() {
    ParameterMetadata p;
    p.name = "NumRaysPerPoint";
    p.displayName = "Rays Per Point";
    p.documentation =
        "Number of rays traced per surface point for Monte-Carlo particle "
        "transport. Higher values reduce statistical noise at the cost of "
        "runtime.";
    p.type = ParameterType::INTEGER;
    p.defaultValue = 1000;
    p.minValue = 1;
    p.maxValue = 100000;
    p.category = ParameterCategory::ADVANCED;
    p.stepSize = 100;
    p.required = false;
    return p;
}

enum class ModelType {
    GEOMETRY,
    EMULATION,
    SIMULATION
};

struct ModelMetadata {
    std::string className;
    std::string displayName;
    std::string description;
    ModelType type;

    std::vector<ParameterMetadata> parameters;
};

using ParameterValue = std::variant<double, int, bool, std::string, MaterialListValue>;
using ParameterMap = std::map<std::string, ParameterValue>;

inline std::string ParameterValueToString(const ParameterValue& value) {
    std::ostringstream oss;
    std::visit([&oss](const auto& v) {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, double>) {
            oss << v << " (double)";
        } else if constexpr (std::is_same_v<T, int>) {
            oss << v << " (int)";
        } else if constexpr (std::is_same_v<T, bool>) {
            oss << (v ? "true" : "false") << " (bool)";
        } else if constexpr (std::is_same_v<T, std::string>) {
            oss << "\"" << v << "\" (string)";
        } else if constexpr (std::is_same_v<T, MaterialListValue>) {
            oss << "[";
            for (size_t i = 0; i < v.materialIds.size(); ++i) {
                oss << v.materialIds[i];
                if (i + 1 < v.materialIds.size()) oss << ", ";
            }
            oss << "] (MaterialList)";
        }
    }, value);
    return oss.str();
}

inline std::string ParameterMapToString(const ParameterMap& params, const std::string& title = "Parameters") {
    std::ostringstream oss;
    oss << title << ":" << std::endl;

    if (params.empty()) {
        oss << "  (empty)" << std::endl;
        return oss.str();
    }

    for (const auto& [key, value] : params) {
        oss << "  " << key << " = ";
        oss << ParameterValueToString(value);
        oss << std::endl;
    }

    return oss.str();
}

inline std::string ModelMetadataToString(const ModelMetadata& model, const std::string& title = "Model Metadata") {
    std::ostringstream oss;
    oss << title << ":" << std::endl;
    oss << "  Class Name: " << model.className << std::endl;
    oss << "  Display Name: " << model.displayName << std::endl;
    oss << "  Description: " << model.description << std::endl;
    const char* typeStr = model.type == ModelType::GEOMETRY   ? "GEOMETRY"
                        : model.type == ModelType::EMULATION  ? "EMULATION"
                                                              : "SIMULATION";
    oss << "  Type: " << typeStr << std::endl;
    oss << "  Parameters (" << model.parameters.size() << "):" << std::endl;
    
    for (size_t i = 0; i < model.parameters.size(); ++i) {
        const auto& param = model.parameters[i];
        oss << "    [" << i << "] " << param.name << " (" << param.displayName << ")" << std::endl;
        oss << "        Type: ";
        switch (param.type) {
            case ParameterType::DOUBLE: oss << "DOUBLE"; break;
            case ParameterType::INTEGER: oss << "INTEGER"; break;
            case ParameterType::BOOLEAN: oss << "BOOLEAN"; break;
            case ParameterType::ENUM: oss << "ENUM"; break;
            case ParameterType::MATERIAL: oss << "MATERIAL"; break;
            case ParameterType::MATERIAL_LIST: oss << "MATERIAL_LIST"; break;
        }
        oss << std::endl;
        oss << "        Default: " << ParameterValueToString(param.defaultValue) << std::endl;
        if (!param.unit.empty()) {
            oss << "        Unit: " << param.unit << std::endl;
        }
    }

    return oss.str();
}

} // namespace ViennaPSMeta

#endif