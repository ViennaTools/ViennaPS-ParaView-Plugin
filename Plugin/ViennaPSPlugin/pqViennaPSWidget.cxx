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
#include <vtkAlgorithm.h>
#include "vtkViennaPSDomainObject.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QLineEdit>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QListWidget>
#include <QListWidgetItem>
#include <QStandardItemModel>
#include <QTimer>

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

  if (isSource) {
    for (const auto& modelName : registry.getGeometryModelNames()) {
      auto metadata = registry.getModelMetadata(modelName);
      modelSelector->addItem(QString::fromStdString(metadata.displayName),
                            QString::fromStdString(modelName));
    }
  } else {
    auto addGroup = [&](const QString& header, ViennaPSMeta::ModelType type) {
      bool headerAdded = false;
      for (const auto& modelName : registry.getProcessModelNames()) {
        auto metadata = registry.getModelMetadata(modelName);
        if (metadata.type != type) continue;
        if (!headerAdded) {
          modelSelector->addItem(header);  // no userData => header row
          if (auto* m = qobject_cast<QStandardItemModel*>(modelSelector->model())) {
            if (auto* item = m->item(modelSelector->count() - 1))
              item->setEnabled(false);
          }
          headerAdded = true;
        }
        modelSelector->addItem(QString::fromStdString(metadata.displayName),
                              QString::fromStdString(modelName));
      }
    };
    addGroup("\xE2\x80\x94 Emulation (analytical) \xE2\x80\x94",
             ViennaPSMeta::ModelType::EMULATION);
    addGroup("\xE2\x80\x94 Simulation (ray tracing) \xE2\x80\x94",
             ViennaPSMeta::ModelType::SIMULATION);

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
  }
  connect(outputFormatComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
  this, [this]() { emit changeAvailable(); });

  if (modelSelector->count() > 0) {
    onModelChanged(modelSelector->currentIndex());
  }

  restoreStateFromProxy();
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

pqViennaPSWidget::~pqViennaPSWidget() = default;

static bool dummy = false;

int pqViennaPSWidget::modelRegistryIndex(const QString& modelName) const
{
  auto& registry = vtkViennaPSModelRegistry::getInstance();
  auto names = isSource ? registry.getGeometryModelNames()
                        : registry.getProcessModelNames();
  std::string target = modelName.toStdString();
  for (int i = 0; i < static_cast<int>(names.size()); ++i) {
    if (names[i] == target)
      return i;
  }
  return 0;
}

void pqViennaPSWidget::onModelChanged(int index)
{
  if (index < 0) return;

  QString modelName = modelSelector->itemData(index).toString();
  if (modelName.isEmpty()) {
    return;
  }
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
        vtkSource->SetModelType(modelRegistryIndex(modelName));

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

QStringList pqViennaPSWidget::getDomainMaterialNames()
{
  QStringList result;
  if (isSource) {
    return result;  // sources create the domain, no upstream materials to query
  }

  vtkSMProxy* smProxy = this->proxy;
  if (!smProxy) {
    return result;
  }

  vtkSMInputProperty* inputProp = vtkSMInputProperty::SafeDownCast(
    smProxy->GetProperty("Input"));
  if (!inputProp || inputProp->GetNumberOfProxies() == 0) {
    return result;
  }

  vtkSMSourceProxy* inputProxy = vtkSMSourceProxy::SafeDownCast(
    inputProp->GetProxy(0));
  if (!inputProxy) {
    return result;
  }

  inputProxy->UpdatePipeline();

  vtkAlgorithm* alg = vtkAlgorithm::SafeDownCast(inputProxy->GetClientSideObject());
  if (!alg) {
    return result;
  }

  vtkViennaPSDomainObject* domainObj =
    vtkViennaPSDomainObject::SafeDownCast(alg->GetOutputDataObject(0));
  if (!domainObj || !domainObj->HasDomain()) {
    return result;
  }

  for (const auto& name : domainObj->GetMaterialNamesInDomain()) {
    result << QString::fromStdString(name);
  }
  return result;
}

void pqViennaPSWidget::loadModel(const QString& modelName)
{
  currentModel = modelName;
  auto& registry = vtkViennaPSModelRegistry::getInstance();
  
  try {
    auto metadata = registry.getModelMetadata(modelName.toStdString());

    // Materials present in the connected input domain (empty if unavailable);
    // used to restrict material-list choices to what the domain actually holds.
    QStringList domainMaterials = getDomainMaterialNames();

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
              if (!domainMaterials.isEmpty()) {
                options = domainMaterials;
              } else {
                for (const auto& [key, value] : param.materialMap) {
                  options << QString::fromStdString(value);
                }
              }
              widget = createMaterialListWidget(paramName, options);
            }
          break;

          case ViennaPSMeta::ParameterType::STRING:
            widget = createStringWidget(paramName,
                QString::fromStdString(std::get<std::string>(param.defaultValue)));
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

void pqViennaPSWidget::clearParameterWidgets()
{
  while (modelForm->rowCount() > 0) {
    modelForm->removeRow(0);
  }

  parameterWidgets.clear();
  parameterLabels.clear();
  parameterValues.clear();
}

void pqViennaPSWidget::restoreStateFromProxy()
{
  if (!proxy) return;

  vtkSMProperty* stateProp = proxy->GetProperty("State");
  if (!stateProp) return;

  vtkSMPropertyHelper helper(stateProp);
  if (helper.GetNumberOfElements() < 1) return;
  const char* raw = helper.GetAsString(0);
  if (!raw || raw[0] == '\0') return;

  ViennaPSMeta::WidgetState st = ViennaPSMeta::deserializeWidgetState(raw);
  if (!st.valid) return;

  int mi = modelSelector->findData(QString::fromStdString(st.model));
  if (mi >= 0) {
    if (mi != modelSelector->currentIndex())
      modelSelector->setCurrentIndex(mi);
    else
      onModelChanged(mi);
  }

  gridDeltaSpinBox->setValue(st.gridDelta);
  xExtentSpinBox->setValue(st.xExtent);
  yExtentSpinBox->setValue(st.yExtent);
  int di = targetDimensionComboBox->findData(st.targetDim);
  if (di >= 0) targetDimensionComboBox->setCurrentIndex(di);
  if (!isSource && processTimeSpinBox) processTimeSpinBox->setValue(st.processTime);
  if (st.outputFormat >= 0 && st.outputFormat < outputFormatComboBox->count())
    outputFormatComboBox->setCurrentIndex(st.outputFormat);

  for (const auto& kv : st.params) {
    const QString qn = QString::fromStdString(kv.first);
    const ViennaPSMeta::ParameterValue& val = kv.second;
    QWidget* w = parameterWidgets.value(qn, nullptr);
    if (!w) continue;

    if (auto* ds = qobject_cast<QDoubleSpinBox*>(w)) {
      if (std::holds_alternative<double>(val)) ds->setValue(std::get<double>(val));
    } else if (auto* is = qobject_cast<QSpinBox*>(w)) {
      if (std::holds_alternative<int>(val)) is->setValue(std::get<int>(val));
    } else if (auto* cb = qobject_cast<QCheckBox*>(w)) {
      if (std::holds_alternative<bool>(val)) cb->setChecked(std::get<bool>(val));
    } else if (auto* combo = qobject_cast<QComboBox*>(w)) {
      if (std::holds_alternative<int>(val)) combo->setCurrentIndex(std::get<int>(val));
    } else if (auto* le = qobject_cast<QLineEdit*>(w)) {
      if (std::holds_alternative<std::string>(val))
        le->setText(QString::fromStdString(std::get<std::string>(val)));
    } else if (std::holds_alternative<ViennaPSMeta::MaterialListValue>(val)) {
      QListWidget* lw = w->property("listWidget").value<QListWidget*>();
      if (lw) {
        lw->clear();
        QList<QVariant> ids;
        for (int id : std::get<ViennaPSMeta::MaterialListValue>(val).materialIds) {
          auto mat = viennaps::MaterialMap::mapToMaterial(id);
          QListWidgetItem* item =
            new QListWidgetItem(QString::fromStdString(viennaps::MaterialMap::toString(mat)));
          item->setData(Qt::UserRole, id);
          lw->addItem(item);
          ids.append(id);
        }
        parameterValues[qn] = ids;
      }
    }
  }

  updateParameterVisibility();

  QTimer::singleShot(0, this, [this]() { emit changeAvailable(); });
}

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

QWidget* pqViennaPSWidget::createStringWidget(const QString& name,
                                              const QString& value)
{
  QLineEdit* lineEdit = new QLineEdit(this);
  lineEdit->setText(value);
  lineEdit->setObjectName(name);

  connect(lineEdit, &QLineEdit::textChanged,
          this, &pqViennaPSWidget::onStringParameterChanged);

  parameterValues[name] = value;
  return lineEdit;
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
        int materialId = static_cast<int>(
            ViennaPSMeta::resolveMaterialFromString(options.at(i).toStdString()).legacyId());
        materialCombo->addItem(options.at(i), materialId);
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

void pqViennaPSWidget::onEnumParameterChanged(int index)
{
  QComboBox* comboBox = qobject_cast<QComboBox*>(sender());
  if (comboBox) {
    QString name = comboBox->objectName();
    parameterValues[name] = index;
    emit changeAvailable();
  }
}

void pqViennaPSWidget::onStringParameterChanged(const QString& value)
{
  QLineEdit* lineEdit = qobject_cast<QLineEdit*>(sender());
  if (lineEdit) {
    QString name = lineEdit->objectName();
    parameterValues[name] = value;
    emit changeAvailable();
  }
}

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

bool pqViennaPSWidget::evaluateCondition(const QString& condition)
{
  // Simple parser for "param <op> value" conditions. Operators are checked
  // longest-first so ">=" / "<=" are matched before ">" / "<".
  const QStringList operators = {"==", ">=", "<=", ">", "<"};
  for (const QString& op : operators) {
    if (!condition.contains(op)) {
      continue;
    }
    QStringList parts = condition.split(op);
    if (parts.size() != 2) {
      continue;
    }
    QString paramName = parts[0].trimmed();
    QString expectedValue = parts[1].trimmed();

    if (!parameterValues.contains(paramName)) {
      return true;
    }
    QVariant currentValue = parameterValues[paramName];

    if (op == "==") {
      if (expectedValue == "true" && currentValue.userType() == QMetaType::Bool) {
        return currentValue.toBool();
      }
      if (expectedValue == "false" && currentValue.userType() == QMetaType::Bool) {
        return !currentValue.toBool();
      }
      if (currentValue.userType() == QMetaType::Int) {
        return currentValue.toInt() == expectedValue.toInt();
      }
    }

    bool curOk = false, expOk = false;
    double cur = currentValue.toDouble(&curOk);
    double exp = expectedValue.toDouble(&expOk);
    if (curOk && expOk) {
      if (op == "==") return cur == exp;
      if (op == ">")  return cur > exp;
      if (op == "<")  return cur < exp;
      if (op == ">=") return cur >= exp;
      if (op == "<=") return cur <= exp;
    }
    return true;
  }

  return true;
}

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

void pqViennaPSWidget::apply()
{
  applyChanges();
  Superclass::apply();
}

void pqViennaPSWidget::applyChanges()
{
  if (!proxy) return;

  vtkSMSourceProxy* sourceProxy = vtkSMSourceProxy::SafeDownCast(proxy);
  if (!sourceProxy) return;
  
  vtkObjectBase* obj = sourceProxy->GetClientSideObject();
  vtkViennaPSParameterInterface* vtkSource = 
    vtkViennaPSParameterInterface::SafeDownCast(obj);
  
  if (vtkSource) {
    QString curName =
      modelSelector->itemData(modelSelector->currentIndex()).toString();
    vtkSource->SetModelType(modelRegistryIndex(curName));

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
      else if (it.value().userType() == QMetaType::QString) {
        std::string val = it.value().toString().toStdString();
        vtkSource->SetParameterString(paramName.c_str(), val.c_str());
        if (propertyManager) {
          propertyManager->UpdateParameterValue(paramName, val);
        }
      }
    }

    if (!isSource) {
      double processTime = processTimeSpinBox->value();
      vtkSource->SetParameterDouble("ProcessTime", processTime);
      if (propertyManager) {
        propertyManager->UpdateParameterValue("ProcessTime", processTime);
      }
    }

    int outputFormat = outputFormatComboBox->currentIndex();
    vtkSource->SetParameterInt("outputFormat", outputFormat);

    vtkSource->Modified();
  }

  // Serialize the current panel into the "State" property so ParaView's
  // Save State persists it (and Load State can rebuild the panel from it).
  {
    ViennaPSMeta::WidgetState st;
    st.model = modelSelector->itemData(modelSelector->currentIndex()).toString().toStdString();
    st.gridDelta = gridDeltaSpinBox->value();
    st.xExtent = xExtentSpinBox->value();
    st.yExtent = yExtentSpinBox->value();
    st.targetDim = targetDimensionComboBox->currentData().toInt();
    st.processTime = (!isSource && processTimeSpinBox) ? processTimeSpinBox->value() : 0.0;
    st.outputFormat = outputFormatComboBox->currentIndex();
    for (auto it = parameterValues.begin(); it != parameterValues.end(); ++it) {
      const std::string nm = it.key().toStdString();
      const QVariant& v = it.value();
      switch (v.userType()) {
        case QMetaType::Double:  st.params[nm] = v.toDouble(); break;
        case QMetaType::Int:     st.params[nm] = v.toInt(); break;
        case QMetaType::Bool:    st.params[nm] = v.toBool(); break;
        case QMetaType::QString: st.params[nm] = v.toString().toStdString(); break;
        case QMetaType::QVariantList: {
          ViennaPSMeta::MaterialListValue m;
          for (const auto& e : v.toList()) m.materialIds.push_back(e.toInt());
          st.params[nm] = m;
          break;
        }
        default: break;
      }
    }
    std::string serialized = ViennaPSMeta::serializeWidgetState(st);
    vtkSMPropertyHelper(proxy, "State").Set(serialized.c_str());
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
