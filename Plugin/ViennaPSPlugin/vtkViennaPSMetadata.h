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

// --------------------------------------------------------------------------
// Serializable snapshot of a source/filter panel, used so that ParaView's
// Save State / Load State round-trips the dynamically generated parameters.
// The whole state is encoded into a single string that is carried by a
// standard Server-Manager StringVectorProperty ("State").
// --------------------------------------------------------------------------
struct WidgetState {
    std::string model;
    double gridDelta = 0.25;
    double xExtent = 10.0;
    double yExtent = 10.0;
    double processTime = 5.0;
    int targetDim = 3;
    int outputFormat = 1;
    ParameterMap params;
    bool valid = false;
};

inline std::string serializeWidgetState(const WidgetState& s) {
    std::ostringstream oss;
    oss.precision(17);
    oss << "model=" << s.model
        << ";;gridDelta=" << s.gridDelta
        << ";;xExtent=" << s.xExtent
        << ";;yExtent=" << s.yExtent
        << ";;targetDim=" << s.targetDim
        << ";;processTime=" << s.processTime
        << ";;outputFormat=" << s.outputFormat;
    for (const auto& [name, val] : s.params) {
        oss << ";;param:";
        std::visit([&](const auto& v) {
            using T = std::decay_t<decltype(v)>;
            if constexpr (std::is_same_v<T, double>)      oss << "d:" << name << "=" << v;
            else if constexpr (std::is_same_v<T, int>)    oss << "i:" << name << "=" << v;
            else if constexpr (std::is_same_v<T, bool>)   oss << "b:" << name << "=" << (v ? 1 : 0);
            else if constexpr (std::is_same_v<T, std::string>) oss << "s:" << name << "=" << v;
            else if constexpr (std::is_same_v<T, MaterialListValue>) {
                oss << "m:" << name << "=";
                for (size_t i = 0; i < v.materialIds.size(); ++i) {
                    if (i) oss << ",";
                    oss << v.materialIds[i];
                }
            }
        }, val);
    }
    return oss.str();
}

inline WidgetState deserializeWidgetState(const std::string& str) {
    WidgetState s;
    if (str.empty()) return s;

    std::vector<std::string> records;
    size_t pos = 0;
    while (true) {
        size_t next = str.find(";;", pos);
        if (next == std::string::npos) { records.push_back(str.substr(pos)); break; }
        records.push_back(str.substr(pos, next - pos));
        pos = next + 2;
    }

    for (const auto& rec : records) {
        if (rec.empty()) continue;
        size_t eq = rec.find('=');
        if (eq == std::string::npos) continue;
        std::string key = rec.substr(0, eq);
        std::string val = rec.substr(eq + 1);
        try {
            if (key == "model") s.model = val;
            else if (key == "gridDelta")   s.gridDelta = std::stod(val);
            else if (key == "xExtent")     s.xExtent = std::stod(val);
            else if (key == "yExtent")     s.yExtent = std::stod(val);
            else if (key == "targetDim")   s.targetDim = std::stoi(val);
            else if (key == "processTime") s.processTime = std::stod(val);
            else if (key == "outputFormat") s.outputFormat = std::stoi(val);
            else if (key.rfind("param:", 0) == 0) {
                std::string rest = key.substr(6);       // "<t>:<name>"
                if (rest.size() < 2 || rest[1] != ':') continue;
                char t = rest[0];
                std::string name = rest.substr(2);
                switch (t) {
                    case 'd': s.params[name] = std::stod(val); break;
                    case 'i': s.params[name] = std::stoi(val); break;
                    case 'b': s.params[name] = (val == "1" || val == "true"); break;
                    case 's': s.params[name] = val; break;
                    case 'm': {
                        MaterialListValue m;
                        size_t p = 0;
                        while (!val.empty()) {
                            size_t c = val.find(',', p);
                            std::string tok = (c == std::string::npos) ? val.substr(p)
                                                                       : val.substr(p, c - p);
                            if (!tok.empty()) m.materialIds.push_back(std::stoi(tok));
                            if (c == std::string::npos) break;
                            p = c + 1;
                        }
                        s.params[name] = m;
                        break;
                    }
                    default: break;
                }
            }
        } catch (...) {
        }
    }
    s.valid = !s.model.empty();
    return s;
}

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