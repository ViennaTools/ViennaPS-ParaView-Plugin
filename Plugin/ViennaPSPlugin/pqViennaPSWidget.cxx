#include "pqViennaPSWidget.h"
#include "vtkViennaPSModelRegistry.h"
#include "vtkViennaPSMetadata.h"
#include "vtkViennaPSParameterInterface.h"
#include "vtkViennaPSGeometrySource.h"
#include "vtkViennaPSProcessFilter.h"
#include "vtkViennaPSLogger.h"

#include <vtkSMProxy.h>
#include <vtkSMSourceProxy.h>
#include <vtkSMProperty.h>
#include <vtkSMPropertyGroup.h>
#include <vtkSMPropertyHelper.h>
#include <vtkSMInputProperty.h>
#include <vtkSMOutputPort.h>
#include <vtkPVDataInformation.h>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QListWidgetItem>

#include <pqApplicationCore.h>
#include <pqServerManagerModel.h>
#include <pqView.h>

#include <iostream>
#include <cstring>

pqViennaPSWidget::pqViennaPSWidget(vtkSMProxy* smProxy,
                                   QWidget* parentObject)
  : Superclass(smProxy, parentObject), proxy(smProxy)
{
  initialize();
}

//----------------------------------------------------------------------------
pqViennaPSWidget::pqViennaPSWidget(vtkSMProxy* smProxy, 
                                   vtkSMProperty* smProperty,
                                   QWidget* parentObject)
  : Superclass(smProxy, parentObject), proxy(smProxy)
{
  initialize();
}

void pqViennaPSWidget::initialize()
{
  determineWidgetType();
  VPSLOG_DEBUG(nullptr, isSource ? "Widget is a source" : "Widget is a filter");
  auto& registry = vtkViennaPSModelRegistry::getInstance();
  if ((isSource && registry.getGeometryModelNames().empty())){
    registry.initializeBuiltinModels(true);
  } else if (!isSource && registry.getProcessModelNames().empty()) {
    registry.initializeBuiltinModels(false);
  }
  
  mainLayout = new QVBoxLayout(this);

  QGroupBox* modelGroup = new QGroupBox("Model Selection", this);
  QVBoxLayout* modelLayout = new QVBoxLayout(modelGroup);

  modelSelector = new QComboBox(this);
  modelSelector->setToolTip(isSource ? "Select the geometry model to create"
                                     : "Select process to apply");

  auto models = isSource ? registry.getGeometryModelNames() : registry.getProcessModelNames();
  for (const auto& modelName : models) {
    auto metadata = registry.getModelMetadata(modelName);
    modelSelector->addItem(QString::fromStdString(metadata.displayName),
                          QString::fromStdString(modelName));
  }

  // Default the process selector to IsotropicProcess when available.
  if (!isSource) {
    int isoIdx = modelSelector->findData(QString::fromStdString("IsotropicProcess"));
    if (isoIdx >= 0)
      modelSelector->setCurrentIndex(isoIdx);
  }

  modelLayout->addWidget(modelSelector);
  mainLayout->addWidget(modelGroup);

  commonParamsGroup = new QGroupBox("Domain Settings", this);
  commonForm = new QFormLayout(commonParamsGroup);

  gridDeltaSpinBox = new QDoubleSpinBox(this);
  gridDeltaSpinBox->setRange(0.001, 10000.0);
  gridDeltaSpinBox->setSingleStep(0.05);
  gridDeltaSpinBox->setValue(0.25);
  gridDeltaSpinBox->setToolTip("Grid resolution for simulation (in consistent domain units)");
  commonForm->addRow("Grid Resolution:", gridDeltaSpinBox);

  xExtentSpinBox = new QDoubleSpinBox(this);
  xExtentSpinBox->setRange(0.001, 1000000.0);
  xExtentSpinBox->setSingleStep(1.0);
  xExtentSpinBox->setValue(10.0);
  xExtentSpinBox->setToolTip("Domain extent in X direction (in consistent domain units)");
  commonForm->addRow("X Extent:", xExtentSpinBox);

  yExtentSpinBox = new QDoubleSpinBox(this);
  yExtentSpinBox->setRange(0.0, 1000000.0);
  yExtentSpinBox->setSingleStep(1.0);
  yExtentSpinBox->setValue(10.0);
  yExtentSpinBox->setToolTip("Domain extent in Y direction (in consistent domain units)");
  commonForm->addRow("Y Extent:", yExtentSpinBox);

  targetDimensionComboBox = new QComboBox(this);
  targetDimensionComboBox->addItem("2D", 2);
  targetDimensionComboBox->addItem("3D", 3);
  targetDimensionComboBox->setCurrentIndex(1);
  targetDimensionComboBox->setToolTip("Domain dimension for creation");
  commonForm->addRow("Target Dimension:", targetDimensionComboBox);

  mainLayout->addWidget(commonParamsGroup);

  updateDomainSettingsVisibility();

  if (! isSource) {
    commonProcessGroup = new QGroupBox("Process Settings", this);
    commonProcessForm = new QFormLayout(commonProcessGroup);

    processTimeSpinBox = new QDoubleSpinBox(this);
    processTimeSpinBox->setRange(0.0, 1e9);
    processTimeSpinBox->setSingleStep(1.0);
    processTimeSpinBox->setValue(5.0);
    processTimeSpinBox->setToolTip("Simulation time for the process (in consistent domain units)");
    commonProcessForm->addRow("Process Time:", processTimeSpinBox);

    numRaysSpinBox = new QSpinBox(this);
    numRaysSpinBox->setRange(1, 100000);
    numRaysSpinBox->setSingleStep(100);
    numRaysSpinBox->setValue(1000);
    numRaysSpinBox->setToolTip("Number of rays per surface point for ray tracing. Higher = more accurate but slower. Ignored by analytic models.");
    commonProcessForm->addRow("Rays Per Point:", numRaysSpinBox);

    mainLayout->addWidget(commonProcessGroup);
  }

  QGroupBox* displayGroup = new QGroupBox("Display Settings", this);
  QFormLayout* displayForm = new QFormLayout(displayGroup);

  outputFormatComboBox = new QComboBox(this);
  outputFormatComboBox->addItem("Volume", 0);
  outputFormatComboBox->addItem("Surface", 1);
  outputFormatComboBox->addItem("Hull", 2);
  outputFormatComboBox->setCurrentIndex(1);
  outputFormatComboBox->setToolTip("Output mesh format for visualization");
  displayForm->addRow("Output Format:", outputFormatComboBox);

  mainLayout->addWidget(displayGroup);

  modelParamsGroup = new QGroupBox("Model Parameters", this);
  modelForm = new QFormLayout(modelParamsGroup);

  QScrollArea* scrollArea = new QScrollArea(this);
  scrollArea->setWidget(modelParamsGroup);
  scrollArea->setWidgetResizable(true);
  scrollArea->setMinimumHeight(200);
  mainLayout->addWidget(scrollArea);

  mainLayout->addStretch();

  connect(modelSelector, QOverload<int>::of(&QComboBox::currentIndexChanged),
  this, &pqViennaPSWidget::onModelChanged);

  connect(gridDeltaSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
  this, [this]() { emit changeAvailable(); });
  connect(xExtentSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
  this, [this]() { emit changeAvailable(); });
  connect(yExtentSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
  this, [this]() { emit changeAvailable(); });
  connect(targetDimensionComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
  this, [this]() { emit changeAvailable(); });
  if (! isSource) {
    connect(processTimeSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
    this, [this]() { emit changeAvailable(); });
    connect(numRaysSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
    this, [this]() { emit changeAvailable(); });
  }
  connect(outputFormatComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
  this, [this]() { emit changeAvailable(); });

  if (modelSelector->count() > 0) {
    onModelChanged(modelSelector->currentIndex());
  }
}

void pqViennaPSWidget::determineWidgetType()
{
  vtkSMSourceProxy* sourceProxy = vtkSMSourceProxy::SafeDownCast(proxy);
  if (!sourceProxy) return;
  
  vtkObjectBase* obj = sourceProxy->GetClientSideObject();
  vtkViennaPSParameterInterface* vtkSource =  vtkViennaPSParameterInterface::SafeDownCast(obj);
  if (vtkSource) {
    if (vtkSource->IsGeometrySource()) {
      isSource = true;
    }
    else if (vtkSource->IsProcessFilter()){
      isSource = false;
    }
  }
}

pqPropertyWidget* pqViennaPSWidget::createWidget(vtkSMProxy* proxy,
                                                 QWidget* parent)
{
  VPSLOG_DEBUG(nullptr, "CreateWidget only proxy");
  std::string proxyName = proxy->GetXMLName();
  
  if ((proxyName == "ViennaPSGeometrySource") ||
  (proxyName == "ViennaPSProcessFilter")) {
    return new pqViennaPSWidget(proxy, parent);
  }
  
  return nullptr;
}


pqPropertyWidget* pqViennaPSWidget::createWidget(vtkSMProxy* proxy,
  vtkSMProperty* property,
  QWidget* parent)
{
  VPSLOG_DEBUG(nullptr, "CreateWidget with property");
  std::string propName = property->GetXMLName();
  std::string proxyName = proxy->GetXMLName();

  VPSLOG_DEBUG(nullptr, "PropName: ", propName, " proxyName: ", proxyName);
  
  if ((propName == "GeometryType" && proxyName == "ViennaPSGeometrySource") ||
      (propName == "ProcessType" && proxyName == "ViennaPSProcessFilter")) {
    return new pqViennaPSWidget(proxy, property, parent);
  }
  
  return nullptr;
}

//----------------------------------------------------------------------------
pqViennaPSWidget::~pqViennaPSWidget() = default;

static bool dummy = false;
//----------------------------------------------------------------------------
void pqViennaPSWidget::onModelChanged(int index)
{
  if (index < 0) return;
  
  QString modelName = modelSelector->itemData(index).toString();
  VPSLOG_DEBUG(nullptr, "Model changed to: ", modelName.toStdString());

  clearParameterWidgets();

  loadModel(modelName);
  VPSLOG_DEBUG(nullptr, "Model ", modelName.toStdString(), " loaded");

  if (proxy) {
    vtkSMSourceProxy* sourceProxy = vtkSMSourceProxy::SafeDownCast(proxy);
    if (sourceProxy) {
      vtkObjectBase* obj = sourceProxy->GetClientSideObject();
      vtkViennaPSParameterInterface* vtkSource = 
        vtkViennaPSParameterInterface::SafeDownCast(obj);
      
      if (vtkSource) {
        vtkSource->SetModelType(index);
        
        auto propertyManager = vtkSource->GetPropertyManager();
        if (propertyManager) {
          propertyManager->OnModelChanged(modelName.toStdString());
        }
        
        vtkSource->Modified();
      }
    }
  }

  emit changeAvailable();
}

//----------------------------------------------------------------------------
bool pqViennaPSWidget::hasInputDomainInfo()
{
  if (isSource) {
    return false;  // Sources always display domain settings
  }

  vtkSMProxy* smProxy = this->proxy;
  if (!smProxy) {
    VPSLOG_DEBUG(nullptr, "hasInputDomainInfo: smProxy is null");
    return false;
  }

  vtkSMInputProperty* inputProp = vtkSMInputProperty::SafeDownCast(
    smProxy->GetProperty("Input"));

  if (!inputProp || inputProp->GetNumberOfProxies() == 0) {
    VPSLOG_DEBUG(nullptr, "hasInputDomainInfo: No input connected");
    return false;
  }

  vtkSMSourceProxy* inputProxy = vtkSMSourceProxy::SafeDownCast(
    inputProp->GetProxy(0));

  if (!inputProxy) {
    VPSLOG_DEBUG(nullptr, "hasInputDomainInfo: Input proxy is null");
    return false;
  }

  const char* proxyClassName = inputProxy->GetXMLName();
  VPSLOG_DEBUG(nullptr, "hasInputDomainInfo: Input proxy class = ",
               proxyClassName ? proxyClassName : "null");

  if (proxyClassName &&
      (strcmp(proxyClassName, "ViennaPSGeometrySource") == 0 ||
       strcmp(proxyClassName, "ViennaPSProcessFilter") == 0)) {
    VPSLOG_INFO(nullptr, "hasInputDomainInfo: Input is ", proxyClassName,
                " - outputs domain object, hiding domain settings");
    return true;
  }

  vtkSMOutputPort* outputPort = inputProxy->GetOutputPort(0u);
  if (outputPort) {
    vtkPVDataInformation* dataInfo = outputPort->GetDataInformation();
    if (dataInfo) {
      const char* dataClassName = dataInfo->GetDataClassName();
      VPSLOG_DEBUG(nullptr, "hasInputDomainInfo: Data class name = ",
                   dataClassName ? dataClassName : "null");

      if (dataClassName && strcmp(dataClassName, "vtkViennaPSDomainObject") == 0) {
        VPSLOG_INFO(nullptr, "hasInputDomainInfo: Input data is vtkViennaPSDomainObject - hiding domain settings");
        return true;
      }
    }
  }

  VPSLOG_DEBUG(nullptr, "hasInputDomainInfo: Input does not provide domain info - showing domain settings");
  return false;
}

//----------------------------------------------------------------------------
void pqViennaPSWidget::loadModel(const QString& modelName)
{
  currentModel = modelName;
  auto& registry = vtkViennaPSModelRegistry::getInstance();
  
  try {
    auto metadata = registry.getModelMetadata(modelName.toStdString());

    QMap<QString, QList<ViennaPSMeta::ParameterMetadata>> categorized;
    for (const auto& param : metadata.parameters) {
      QString category;
      switch (param.category) {
        case ViennaPSMeta::ParameterCategory::BASIC:
          category = "Basic Parameters";
          break;
        case ViennaPSMeta::ParameterCategory::ADVANCED:
          category = "Advanced Parameters";
          break;
        case ViennaPSMeta::ParameterCategory::MASK:
          category = "Mask Settings";
          break;
        default:
          category = "Other Parameters";
      }
      categorized[category].append(param);
    }

    const QStringList categoryOrder = {
      "Basic Parameters", "Mask Settings", "Advanced Parameters", "Other Parameters"
    };
    bool firstCategory = true;
    for (const auto& catName : categoryOrder) {
      auto it = categorized.find(catName);
      if (it == categorized.end()) continue;
      if (!firstCategory) {
        modelForm->addRow(new QLabel(""));
      }
      firstCategory = false;

      QLabel* categoryLabel = new QLabel("<b>" + catName + "</b>");
      modelForm->addRow(categoryLabel);

      for (const auto& param : it.value()) {
        QString paramName = QString::fromStdString(param.name);
        QString displayName = QString::fromStdString(param.displayName);
        QString tooltip = QString::fromStdString(param.documentation);
        
        QWidget* widget = nullptr;
        
        switch (param.type) {
          case ViennaPSMeta::ParameterType::DOUBLE:
            widget = createDoubleWidget(paramName,
                                       std::get<double>(param.defaultValue),
                                       std::get<double>(param.minValue),
                                       std::get<double>(param.maxValue),
                                       QString::fromStdString(param.unit),
                                       param.stepSize);
            break;
            
          case ViennaPSMeta::ParameterType::INTEGER:
            widget = createIntWidget(paramName,
                                   std::get<int>(param.defaultValue),
                                   std::get<int>(param.minValue),
                                   std::get<int>(param.maxValue));
            break;
            
          case ViennaPSMeta::ParameterType::BOOLEAN:
            widget = createBoolWidget(paramName,
                                     std::get<bool>(param.defaultValue));
            break;
            
          case ViennaPSMeta::ParameterType::ENUM:
          case ViennaPSMeta::ParameterType::MATERIAL:
            {
              QStringList options;
              for (const auto& opt : param.enumOptions) {
                options << QString::fromStdString(opt);
              }
              widget = createEnumWidget(paramName, options,
                                      std::get<int>(param.defaultValue));
            }
            break;
          case ViennaPSMeta::ParameterType::MATERIAL_LIST:
            {
              QStringList options;
              for (const auto& [key, value] : param.materialMap) {
                options << QString::fromStdString(value);
              }
              widget = createMaterialListWidget(paramName, options);
            }
          break;
        }
        
        if (widget) {
          widget->setToolTip(tooltip);
          parameterWidgets[paramName] = widget;
          
          QLabel* label = new QLabel(displayName + ":");
          parameterLabels[paramName] = label;
          modelForm->addRow(label, widget);

          if (!param.visibilityCondition.empty()) {
            widget->setProperty("visibilityCondition", 
                              QString::fromStdString(param.visibilityCondition));
          }
        }
      }
    }

    updateParameterVisibility();

  } catch (const std::exception& e) {
    VPSLOG_ERROR(nullptr, "Error loading model: ", e.what());
  }
}

//----------------------------------------------------------------------------
void pqViennaPSWidget::clearParameterWidgets()
{
  while (modelForm->rowCount() > 0) {
    modelForm->removeRow(0);
  }

  parameterWidgets.clear();
  parameterLabels.clear();
  parameterValues.clear();
}

//----------------------------------------------------------------------------
QWidget* pqViennaPSWidget::createDoubleWidget(const QString& name,
                                              double value,
                                              double min, double max,
                                              const QString& suffix,
                                              double stepSize)
{
  QDoubleSpinBox* spinBox = new QDoubleSpinBox(this);
  spinBox->setRange(min, max);
  spinBox->setValue(value);
  spinBox->setSingleStep(stepSize > 0.0 ? stepSize : (max - min) / 100.0);
  if (!suffix.isEmpty()) {
    spinBox->setSuffix(" " + suffix);
  }
  spinBox->setObjectName(name);
  
  connect(spinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
          this, &pqViennaPSWidget::onDoubleParameterChanged);
  
  parameterValues[name] = value;
  return spinBox;
}

//----------------------------------------------------------------------------
QWidget* pqViennaPSWidget::createIntWidget(const QString& name,
                                          int value, int min, int max)
{
  QSpinBox* spinBox = new QSpinBox(this);
  spinBox->setRange(min, max);
  spinBox->setValue(value);
  spinBox->setObjectName(name);
  
  connect(spinBox, QOverload<int>::of(&QSpinBox::valueChanged),
          this, &pqViennaPSWidget::onIntParameterChanged);
  
  parameterValues[name] = value;
  return spinBox;
}

//----------------------------------------------------------------------------
QWidget* pqViennaPSWidget::createBoolWidget(const QString& name, bool value)
{
  QCheckBox* checkBox = new QCheckBox(this);
  checkBox->setChecked(value);
  checkBox->setObjectName(name);
  
  connect(checkBox, &QCheckBox::toggled,
          this, &pqViennaPSWidget::onBoolParameterChanged);
  
  parameterValues[name] = value;
  return checkBox;
}

//----------------------------------------------------------------------------
QWidget* pqViennaPSWidget::createEnumWidget(const QString& name,
                                           const QStringList& options,
                                           int value)
{
  QComboBox* comboBox = new QComboBox(this);
  comboBox->addItems(options);
  comboBox->setCurrentIndex(value);
  comboBox->setObjectName(name);
  
  connect(comboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
          this, &pqViennaPSWidget::onEnumParameterChanged);
  
  parameterValues[name] = value;
  return comboBox;
}

QWidget* pqViennaPSWidget::createMaterialListWidget(const QString& paramName, 
                                 const QStringList& options)
{
    QWidget* container = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(container);

    QListWidget* selectedList = new QListWidget();
    selectedList->setMaximumHeight(100);

    QHBoxLayout* buttonLayout = new QHBoxLayout();
    QPushButton* addButton = new QPushButton("Add Material");
    QPushButton* removeButton = new QPushButton("Remove");

    QComboBox* materialCombo = new QComboBox();
    for (int i=0; i< options.length(); i++) {
        materialCombo->addItem(options.at(i), i);
    }

    buttonLayout->addWidget(materialCombo);
    buttonLayout->addWidget(addButton);
    buttonLayout->addWidget(removeButton);

    layout->addWidget(selectedList);
    layout->addLayout(buttonLayout);

    auto updateMaterialList = [=, this]() {
        QList<QVariant> materialIds;
        for (int i = 0; i < selectedList->count(); ++i) {
            materialIds.append(selectedList->item(i)->data(Qt::UserRole));
        }
        parameterValues[paramName] = materialIds;
        emit changeAvailable();
    };

    connect(addButton, &QPushButton::clicked, [=]() {
        QString text = materialCombo->currentText();
        int value = materialCombo->currentData().toInt();

        for (int i = 0; i < selectedList->count(); ++i) {
            if (selectedList->item(i)->data(Qt::UserRole).toInt() == value) {
                return;
            }
        }

        QListWidgetItem* item = new QListWidgetItem(text);
        item->setData(Qt::UserRole, value);
        selectedList->addItem(item);

        updateMaterialList();
    });

    connect(removeButton, &QPushButton::clicked, [=]() {
        int currentRow = selectedList->currentRow();
        if (currentRow >= 0) {
            delete selectedList->takeItem(currentRow);
            updateMaterialList();
        }
    });

    parameterValues[paramName] = QList<QVariant>();

    container->setProperty("paramName", paramName);
    container->setProperty("listWidget", QVariant::fromValue(selectedList));
    
    return container;
}

//----------------------------------------------------------------------------
void pqViennaPSWidget::onDoubleParameterChanged(double value)
{
  QDoubleSpinBox* spinBox = qobject_cast<QDoubleSpinBox*>(sender());
  if (spinBox) {
    QString name = spinBox->objectName();
    parameterValues[name] = value;
    updateParameterVisibility();
    emit changeAvailable();
  }
}

//----------------------------------------------------------------------------
void pqViennaPSWidget::onIntParameterChanged(int value)
{
  QSpinBox* spinBox = qobject_cast<QSpinBox*>(sender());
  if (spinBox) {
    QString name = spinBox->objectName();
    parameterValues[name] = value;
    updateParameterVisibility();
    emit changeAvailable();
  }
}

//----------------------------------------------------------------------------
void pqViennaPSWidget::onBoolParameterChanged(bool value)
{
  QCheckBox* checkBox = qobject_cast<QCheckBox*>(sender());
  if (checkBox) {
    QString name = checkBox->objectName();
    parameterValues[name] = value;
    updateParameterVisibility();
    emit changeAvailable();
  }
}

//----------------------------------------------------------------------------
void pqViennaPSWidget::onEnumParameterChanged(int index)
{
  QComboBox* comboBox = qobject_cast<QComboBox*>(sender());
  if (comboBox) {
    QString name = comboBox->objectName();
    parameterValues[name] = index;
    emit changeAvailable();
  }
}

//----------------------------------------------------------------------------
void pqViennaPSWidget::updateParameterVisibility()
{
  for (auto it = parameterWidgets.begin(); it != parameterWidgets.end(); ++it) {
    QString condition = it.value()->property("visibilityCondition").toString();
    
    bool visible = true;
    if (!condition.isEmpty()) {
      visible = evaluateCondition(condition);
    }
    
    it.value()->setVisible(visible);
    if (parameterLabels.contains(it.key())) {
      parameterLabels[it.key()]->setVisible(visible);
    }
  }
}

//----------------------------------------------------------------------------
bool pqViennaPSWidget::evaluateCondition(const QString& condition)
{
  // Simple parser for "param==value" conditions
  if (condition.contains("==")) {
    QStringList parts = condition.split("==");
    if (parts.size() == 2) {
      QString paramName = parts[0].trimmed();
      QString expectedValue = parts[1].trimmed();
      
      if (parameterValues.contains(paramName)) {
        QVariant currentValue = parameterValues[paramName];
        
        if (expectedValue == "true" && currentValue.userType() == QMetaType::Bool) {
          return currentValue.toBool();
        }
        else if (expectedValue == "false" && currentValue.userType() == QMetaType::Bool) {
          return !currentValue.toBool();
        }
        else if (currentValue.userType() == QMetaType::Int) {
          return currentValue.toInt() == expectedValue.toInt();
        }
      }
    }
  }
  
  return true;
}

//----------------------------------------------------------------------------
void pqViennaPSWidget::updateDomainSettingsVisibility()
{
  if (!commonParamsGroup) {
    return;
  }

  if (hasInputDomainInfo()) {
    commonParamsGroup->hide();
    VPSLOG_DEBUG(nullptr, "Domain Settings hidden - input has domain info");
  } else {
    commonParamsGroup->show();
    VPSLOG_DEBUG(nullptr, "Domain Settings visible - no domain info in input");
  }
}

//----------------------------------------------------------------------------
void pqViennaPSWidget::apply()
{
  applyChanges();
  Superclass::apply();
}

//----------------------------------------------------------------------------
void pqViennaPSWidget::applyChanges()
{
  if (!proxy) return;

  vtkSMSourceProxy* sourceProxy = vtkSMSourceProxy::SafeDownCast(proxy);
  if (!sourceProxy) return;
  
  vtkObjectBase* obj = sourceProxy->GetClientSideObject();
  vtkViennaPSParameterInterface* vtkSource = 
    vtkViennaPSParameterInterface::SafeDownCast(obj);
  
  if (vtkSource) {
    vtkSource->SetModelType(modelSelector->currentIndex());

    // Set domain settings only for GeometrySource
    vtkViennaPSGeometrySource* geometrySource =
      vtkViennaPSGeometrySource::SafeDownCast(vtkSource);
    if (geometrySource) {
      geometrySource->SetGridDelta(gridDeltaSpinBox->value());
      geometrySource->SetXExtent(xExtentSpinBox->value());
      geometrySource->SetYExtent(yExtentSpinBox->value());
      geometrySource->SetTargetDimension(targetDimensionComboBox->currentData().toInt());
    }

    // Set domain settings for ProcessFilter (used in mesh path)
    vtkViennaPSProcessFilter* processFilter =
      vtkViennaPSProcessFilter::SafeDownCast(vtkSource);
    if (processFilter) {
      processFilter->SetGridDelta(gridDeltaSpinBox->value());
      processFilter->SetXExtent(xExtentSpinBox->value());
      processFilter->SetYExtent(yExtentSpinBox->value());
      processFilter->SetTargetDimension(targetDimensionComboBox->currentData().toInt());
    }

    auto propertyManager = vtkSource->GetPropertyManager();
    
    for (auto it = parameterValues.begin(); it != parameterValues.end(); ++it) {
      std::string paramName = it.key().toStdString();
      
      if (it.value().userType() == QMetaType::Double) {
        double val = it.value().toDouble();
        vtkSource->SetParameterDouble(paramName.c_str(), val);
        if (propertyManager) {
          propertyManager->UpdateParameterValue(paramName, val);
        }
      }
      else if (it.value().userType() == QMetaType::Int) {
        int val = it.value().toInt();
        vtkSource->SetParameterInt(paramName.c_str(), val);
        if (propertyManager) {
          propertyManager->UpdateParameterValue(paramName, val);
        }
      }
      else if (it.value().userType() == QMetaType::Bool) {
        bool val = it.value().toBool();
        vtkSource->SetParameterBool(paramName.c_str(), val);
        if (propertyManager) {
          propertyManager->UpdateParameterValue(paramName, val);
        }
      }
      else if (it.value().userType() == QMetaType::QVariantList) {
        QList<QVariant> list = it.value().toList();
        std::vector<int> materialIds;
        for (const auto& item : list) {
          materialIds.push_back(item.toInt());
        }
        vtkSource->SetParameterMaterialList(paramName.c_str(), materialIds);
        if (propertyManager) {
          ViennaPSMeta::MaterialListValue matList;
          matList.materialIds = materialIds;
          propertyManager->UpdateParameterValue(paramName, matList);
        }
      }
    }

    if (!isSource) {
      double processTime = processTimeSpinBox->value();
      vtkSource->SetParameterDouble("ProcessTime", processTime);
      if (propertyManager) {
        propertyManager->UpdateParameterValue("ProcessTime", processTime);
      }

      int numRays = numRaysSpinBox->value();
      vtkSource->SetParameterInt("NumRaysPerPoint", numRays);
      if (propertyManager) {
        propertyManager->UpdateParameterValue("NumRaysPerPoint", numRays);
      }
    }

    int outputFormat = outputFormatComboBox->currentIndex();
    vtkSource->SetParameterInt("outputFormat", outputFormat);

    vtkSource->Modified();
  }
  
  vtkSMPropertyHelper(proxy, "Dummy").Set(!dummy);
  dummy = !dummy;
  proxy->UpdateVTKObjects();

  // Mark the proxy as modified so ParaView's pipeline knows it must re-execute.
  // Without this, the representation considers the pipeline up-to-date and
  // never sends REQUEST_DATA on subsequent applies.
  vtkSMSourceProxy* srcProxy = vtkSMSourceProxy::SafeDownCast(proxy);
  if (srcProxy) {
    srcProxy->MarkModified(srcProxy);
  }
}
