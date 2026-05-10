#ifndef vtkViennaPSPropertyManager_h
#define vtkViennaPSPropertyManager_h

#include <vtkObject.h>
#include <vtkSmartPointer.h>
#include "vtkViennaPSMetadata.h"
#include <map>
#include <string>
#include <functional>

class vtkSMProxy;
class vtkSMProperty;
class vtkViennaPSGeometrySource;
class vtkViennaPSProcessFilter;

class vtkViennaPSPropertyManager : public vtkObject {
public:
    static vtkViennaPSPropertyManager* New();
    vtkTypeMacro(vtkViennaPSPropertyManager, vtkObject);
    
    void Initialize(bool isSource = true);

    void OnModelChanged(const std::string& newModelName);
    void OnModelIndexChanged(int modelIndex);

    void UpdateParameterVisibility();
    void UpdateParameterValue(const std::string& paramName, const ViennaPSMeta::ParameterValue& value);
    ViennaPSMeta::ParameterValue GetParameterValue(const std::string& paramName) const;

    ViennaPSMeta::ParameterMap GetAllParameters() const;

    bool ValidateParameters();
    std::string GetValidationErrors() const { return validationErrors_; }

    bool IsPropertyVisible(const std::string& propertyName) const;

    bool EvaluateCondition(const std::string& condition);

protected:
    vtkViennaPSPropertyManager();
    ~vtkViennaPSPropertyManager() override;
    
private:
    vtkViennaPSPropertyManager(const vtkViennaPSPropertyManager&) = delete;
    void operator=(const vtkViennaPSPropertyManager&) = delete;

    std::string currentModel_;
    bool isSourceProxy_;
    ViennaPSMeta::ModelMetadata currentMetadata_;

    ViennaPSMeta::ParameterMap currentValues_;
    std::map<std::string, bool> propertyVisibility_;

    vtkSmartPointer<vtkSMProxy> proxy_;

    std::string validationErrors_;
    bool ValidateParameter(const ViennaPSMeta::ParameterMetadata& param,
                          const ViennaPSMeta::ParameterValue& value);

    void InitializeParameterDefaults();

    struct ConditionToken {
        enum Type { IDENTIFIER, OPERATOR, VALUE, END };
        Type type;
        std::string value;
    };
    
    std::vector<ConditionToken> TokenizeCondition(const std::string& condition);
    bool EvaluateTokens(const std::vector<ConditionToken>& tokens);
};

#endif
