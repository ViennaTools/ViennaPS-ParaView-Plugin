#include "vtkViennaPSPropertyManager.h"
#include "vtkViennaPSModelRegistry.h"
#include "vtkViennaPSGeometrySource.h"
#include "vtkViennaPSProcessFilter.h"
#include "vtkViennaPSLogger.h"
#include "vtkViennaPSMetadata.h"

#include <vtkObjectFactory.h>
#include <vtkSMProxy.h>
#include <vtkSMProperty.h>
#include <vtkSMPropertyHelper.h>
#include <vtkSMIntVectorProperty.h>
#include <vtkSMDoubleVectorProperty.h>
#include <vtkSMStringVectorProperty.h>

#include <iostream>
#include <sstream>
#include <algorithm>

using namespace ViennaPSLogger;

vtkStandardNewMacro(vtkViennaPSPropertyManager);

vtkViennaPSPropertyManager::vtkViennaPSPropertyManager()
    : isSourceProxy_(true)
{
}

vtkViennaPSPropertyManager::~vtkViennaPSPropertyManager()
{
}

void vtkViennaPSPropertyManager::Initialize(bool isSource)
{
    this->isSourceProxy_ = isSource;
    VPSLOG_DEBUG(nullptr, "PropertyManager: Initialized with ", (isSource ? "source" : "filter"), " proxy" );
}

void vtkViennaPSPropertyManager::OnModelChanged(const std::string& newModelName)
{
    if (currentModel_ == newModelName) {
        return; // No change
    }
    VPSLOG_DEBUG(nullptr, "PropertyManager: Model changed from '", currentModel_ , "' to '", newModelName, "'");
    
    currentModel_ = newModelName;

    auto& registry = vtkViennaPSModelRegistry::getInstance();
    if (registry.hasModel(newModelName)) {
        currentMetadata_ = registry.getModelMetadata(newModelName);

        VPSLOG_INFO(nullptr, "PropertyManager currentMetaData: ", ViennaPSMeta::ModelMetadataToString(currentMetadata_));

        InitializeParameterDefaults();

        UpdateParameterVisibility();
    } else {
        VPSLOG_ERROR(nullptr, "PropertyManager: Model not found: ", newModelName);
    }
}

void vtkViennaPSPropertyManager::OnModelIndexChanged(int modelIndex)
{
    auto& registry = vtkViennaPSModelRegistry::getInstance();
    std::string modelName;
    
    if (isSourceProxy_) {
        modelName = registry.getGeometryModelByIndex(modelIndex);
    } else {
        modelName = registry.getProcessModelByIndex(modelIndex);
    }
    
    if (!modelName.empty()) {
        OnModelChanged(modelName);
    }
}

void vtkViennaPSPropertyManager::InitializeParameterDefaults()
{
    currentValues_.clear();    
    for (const auto& param : currentMetadata_.parameters) {
        currentValues_[param.name] = param.defaultValue;
    }

    VPSLOG_DEBUG(nullptr, "vtkViennaPSPropertyManager::InitializeParameterDefaults: ",ViennaPSMeta::ParameterMapToString(currentValues_));
}

void vtkViennaPSPropertyManager::UpdateParameterVisibility()
{
    for (const auto& param : currentMetadata_.parameters) {
        bool visible = true;

        if (!param.visibilityCondition.empty()) {
            visible = EvaluateCondition(param.visibilityCondition);
        }
        
        propertyVisibility_[param.name] = visible;
    }
}

bool vtkViennaPSPropertyManager::IsPropertyVisible(const std::string& propertyName) const
{
    auto it = propertyVisibility_.find(propertyName);
    if (it != propertyVisibility_.end()) {
        return it->second;
    }
    return true; // Default to visible
}

bool vtkViennaPSPropertyManager::EvaluateCondition(const std::string& condition)
{
    // Simple condition parser for "property==value" format
    auto tokens = TokenizeCondition(condition);
    return EvaluateTokens(tokens);
}

std::vector<vtkViennaPSPropertyManager::ConditionToken> 
vtkViennaPSPropertyManager::TokenizeCondition(const std::string& condition)
{
    std::vector<ConditionToken> tokens;
    std::stringstream ss(condition);
    std::string part;
    
    // Simple tokenizer for "property==value" format
    size_t pos = condition.find("==");
    if (pos != std::string::npos) {
        tokens.push_back({ConditionToken::IDENTIFIER, condition.substr(0, pos)});
        tokens.push_back({ConditionToken::OPERATOR, "=="});
        tokens.push_back({ConditionToken::VALUE, condition.substr(pos + 2)});
    }
    
    tokens.push_back({ConditionToken::END, ""});
    return tokens;
}

bool vtkViennaPSPropertyManager::EvaluateTokens(const std::vector<ConditionToken>& tokens)
{
    if (tokens.size() < 3) return true;
    
    if (tokens[0].type == ConditionToken::IDENTIFIER && 
        tokens[1].type == ConditionToken::OPERATOR &&
        tokens[1].value == "==" &&
        tokens[2].type == ConditionToken::VALUE) {
        
        std::string propName = tokens[0].value;
        std::string expectedValue = tokens[2].value;

        auto it = currentValues_.find(propName);
        if (it != currentValues_.end()) {
            if (std::holds_alternative<bool>(it->second)) {
                bool val = std::get<bool>(it->second);
                return (expectedValue == "true" && val) ||
                       (expectedValue == "false" && !val);
            }
            else if (std::holds_alternative<int>(it->second)) {
                int val = std::get<int>(it->second);
                try {
                    return val == std::stoi(expectedValue);
                } catch (...) {
                    return false;
                }
            }
            else if (std::holds_alternative<double>(it->second)) {
                double val = std::get<double>(it->second);
                try {
                    return std::abs(val - std::stod(expectedValue)) < 1e-6;
                } catch (...) {
                    return false;
                }
            }
        }
    }

    return true;
}

void vtkViennaPSPropertyManager::UpdateParameterValue(
    const std::string& paramName, 
    const ViennaPSMeta::ParameterValue& value)
{
    currentValues_[paramName] = value;
    
    UpdateParameterVisibility();
}

ViennaPSMeta::ParameterValue vtkViennaPSPropertyManager::GetParameterValue(
    const std::string& paramName) const
{
    auto it = currentValues_.find(paramName);
    if (it != currentValues_.end()) {
        return it->second;
    }

    for (const auto& param : currentMetadata_.parameters) {
        if (param.name == paramName) {
            return param.defaultValue;
        }
    }

    return 0.0;
}

ViennaPSMeta::ParameterMap vtkViennaPSPropertyManager::GetAllParameters() const
{
    return currentValues_;
}

bool vtkViennaPSPropertyManager::ValidateParameters()
{
    validationErrors_.clear();
    bool valid = true;
    
    for (const auto& param : currentMetadata_.parameters) {
        if (param.required && IsPropertyVisible(param.name)) {
            auto it = currentValues_.find(param.name);
            if (it != currentValues_.end()) {
                if (!ValidateParameter(param, it->second)) {
                    valid = false;
                }
            } else {
                validationErrors_ += "Missing required parameter: " + param.name + "\n";
                valid = false;
            }
        }
    }
    
    return valid;
}

bool vtkViennaPSPropertyManager::ValidateParameter(
    const ViennaPSMeta::ParameterMetadata& param,
    const ViennaPSMeta::ParameterValue& value)
{
    if (param.type == ViennaPSMeta::ParameterType::DOUBLE) {
        if (std::holds_alternative<double>(value)) {
            double val = std::get<double>(value);
            double min = std::get<double>(param.minValue);
            double max = std::get<double>(param.maxValue);
            
            if (val < min || val > max) {
                validationErrors_ += param.name + " out of range [" + 
                                    std::to_string(min) + ", " + 
                                    std::to_string(max) + "]\n";
                return false;
            }
        }
    }
    return true;
}

